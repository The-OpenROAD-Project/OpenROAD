// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <numbers>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "color.h"
#include "gtest/gtest.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "third-party/lodepng/lodepng.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

// Helper: parse a JSON literal into a boost::json::object for tests.
boost::json::object parseObj(std::string_view json)
{
  return boost::json::parse(json).as_object();
}

// Fixed tile dimensions produced by TileGenerator (kTileSizeInPixel).
constexpr int kTileSize = 256;
// Square die side used by the tile-seam tests (DBU; 45 um at 2000 dbu/um).
constexpr int kSeamDieSide = 90000;

// The (CSS tile size, device pixel ratio) pairs the viewer actually asks for.
//
// A tile's CSS box is a whole number of device pixels only when the two
// multiply out whole, which is why the viewer uses 240 rather than 256 (see
// TILE_SIZE_CSS in tile-request.js).  The ratios are real ones: display
// scaling, browser zoom, and the two multiplied.  1.6666666269302368 is
// verbatim from the display the tile seams were reported on — a float32 5/3,
// which is why nothing here can be asserted bit-exactly.
//
// Rendering is checked across all of them because every dpr bug in this
// pipeline was invisible at 1 and 2 and only appeared at a fractional ratio.
struct DprCase
{
  int css_tile_size;
  double dpr;
  const char* what;
};

constexpr DprCase kDprCases[] = {
    {240, 1.0, "no scaling"},
    {240, 1.25, "125% display"},
    {240, 1.3333333333333333, "133%, or 166% at 80% zoom"},
    {240, 1.5, "150% display"},
    {240, 1.6666666269302368, "166% display -- the reported case"},
    {240, 1.75, "175% display"},
    {240, 2.0, "200%"},
    {240, 3.0, "300%"},
    // The size a static report bakes its tiles at, which the viewer keeps.
    {256, 1.0, "static report"},
};

// What the client asks the server to render for a case: the exact device-pixel
// square the tile will occupy, from the REAL ratio (mirrors tileDevicePx()).
int tilePxFor(const DprCase& c)
{
  return static_cast<int>(std::lround(c.css_tile_size * c.dpr));
}

enum class Axis
{
  kColumn,
  kRow
};

// Minimal concrete heat map with a single populated bin, used to exercise
// number rendering across tile boundaries (issue #10925).  getBounds() returns
// the block bbox passed by the caller (which the tests align to a tile seam);
// the tile grid uses TileGenerator::getBounds(), which adds a symmetric
// pin-label margin, so the seam stays at the bbox center where the bin sits.
class BoundaryHeatMap : public gui::HeatMapDataSource
{
 public:
  BoundaryHeatMap(utl::Logger* logger,
                  const odb::Rect& bounds,
                  const odb::Rect& cell)
      : gui::HeatMapDataSource(logger,
                               "Boundary HM",
                               "BoundaryHM",
                               "BoundaryHM"),
        bounds_(bounds),
        cell_(cell)
  {
  }

  odb::Rect getBounds() const override { return bounds_; }

  // The label text is hardcoded here, so the numeric bin value is arbitrary --
  // it only needs to mark the bin populated (see populateMap).
  std::string formatValue(double /*value*/, bool /*legend*/) const override
  {
    return "29.89";
  }

 protected:
  bool populateMap() override
  {
    // A sub-rectangle strictly inside the target bin, so addToMap (which marks
    // every bin returned by getMapView, including zero-overlap neighbors)
    // populates only that single bin.  The value is arbitrary (see
    // formatValue).
    addToMap(odb::Rect(cell_.xMin() + 1,
                       cell_.yMin() + 1,
                       cell_.xMax() - 1,
                       cell_.yMax() - 1),
             1.0);
    return true;
  }

  void combineMapData(bool /*base_has_value*/,
                      double& base,
                      double new_data,
                      double /*data_area*/,
                      double /*intersection_area*/,
                      double /*rect_area*/) override
  {
    base = new_data;
  }

 private:
  odb::Rect bounds_;
  odb::Rect cell_;
};

// Return the set of columns (Axis::kColumn) or rows (Axis::kRow) where two RGBA
// tile buffers differ.  Toggling "show numbers" leaves the bin fill untouched,
// so the diff isolates the rendered text pixels regardless of the fill color.
std::set<int> textPixels(const std::vector<unsigned char>& a,
                         const std::vector<unsigned char>& b,
                         Axis axis)
{
  EXPECT_EQ(a.size(), b.size());
  std::set<int> result;
  const size_t num_pixels = std::min(a.size(), b.size()) / 4;
  for (size_t p = 0; p < num_pixels; ++p) {
    const size_t i = p * 4;
    if (std::memcmp(&a[i], &b[i], 4) != 0) {
      result.insert(axis == Axis::kColumn ? static_cast<int>(p % kTileSize)
                                          : static_cast<int>(p / kTileSize));
    }
  }
  return result;
}

// Sub-pixel x of a vertical coverage edge along `row`: the total UNCOVERED area
// to the left of it, in pixels.  For a monotone left-to-right transition that
// integral *is* the edge position, whatever the reconstruction filter spreads
// over the pixels either side of it — a normalized filter preserves total
// coverage.  The row must be fully covered at its right end; the alpha there is
// taken as the "covered" reference, so a fill drawn at any constant alpha
// works.
double coverageEdgeX(const std::vector<unsigned char>& rgba,
                     const int dim,
                     const int row)
{
  const size_t base = static_cast<size_t>(row) * dim * 4;
  const double full = rgba[base + static_cast<size_t>(dim - 1) * 4 + 3];
  EXPECT_GT(full, 0.0) << "row " << row << " is not covered at its right edge";
  if (full <= 0.0) {
    return -1.0;
  }
  double uncovered = 0.0;
  for (int x = 0; x < dim; ++x) {
    const double a = rgba[base + static_cast<size_t>(x) * 4 + 3];
    uncovered += 1.0 - std::min(1.0, a / full);
  }
  return uncovered;
}

// Columns of `row` with any coverage at all.  Used where the fill is a faint
// wash under an opaque border (overlay highlights are alpha 30 with an alpha
// 255 outline), which defeats an alpha-weighted integral -- the border would
// count for eight times the fill it encloses.
int coveredColumns(const std::vector<unsigned char>& rgba,
                   const int dim,
                   const int row)
{
  const size_t base = static_cast<size_t>(row) * dim * 4;
  int covered = 0;
  for (int x = 0; x < dim; ++x) {
    if (rgba[base + static_cast<size_t>(x) * 4 + 3] > 0) {
      covered++;
    }
  }
  return covered;
}

// Covered width over columns [x0, x1) of `row`, in pixels: the coverage
// integral, normalized by the row's strongest alpha (the fill's own).
// Filter-independent for the same reason as coverageEdgeX.
double coveredWidthPx(const std::vector<unsigned char>& rgba,
                      const int dim,
                      const int row,
                      const int x0,
                      const int x1)
{
  const size_t base = static_cast<size_t>(row) * dim * 4;
  double full = 0.0;
  for (int x = 0; x < dim; ++x) {
    full = std::max(
        full, static_cast<double>(rgba[base + static_cast<size_t>(x) * 4 + 3]));
  }
  if (full <= 0.0) {
    return 0.0;
  }
  double covered = 0.0;
  for (int x = x0; x < x1; ++x) {
    covered += rgba[base + static_cast<size_t>(x) * 4 + 3] / full;
  }
  return covered;
}

constexpr double kPi = std::numbers::pi;

// Fraction of AC energy that sits in the moiré "beat band" (spatial periods
// 16..128 px), computed from the per-column and per-row alpha profiles (max of
// the two, so vertical/horizontal/diagonal beats are all caught).  A real beat
// concentrates energy at long periods → high fraction; a finely-resolved grid
// concentrates at short periods → low fraction.  This is the metric that
// distinguishes aliasing from legitimate detail (block-CV alone cannot).
double beatBandFraction1D(const std::vector<double>& sig)
{
  const int n = static_cast<int>(sig.size());
  if (n < 4) {
    return 0.0;
  }
  double mean = 0.0;
  for (const double v : sig) {
    mean += v;
  }
  mean /= n;
  double total = 0.0;
  double band = 0.0;
  for (int k = 1; k <= n / 2; ++k) {
    double re = 0.0;
    double im = 0.0;
    for (int x = 0; x < n; ++x) {
      const double ang = -2.0 * kPi * k * x / n;
      const double centered = sig[x] - mean;
      re += centered * std::cos(ang);
      im += centered * std::sin(ang);
    }
    const double power = re * re + im * im;
    total += power;
    const double period = static_cast<double>(n) / k;
    if (period >= 16.0 && period <= 128.0) {
      band += power;
    }
  }
  return total > 0.0 ? band / total : 0.0;
}

// Beat-band fraction measured over a sub-window of the tile.  Measuring a
// central macro-uniform window (rather than the whole tile) avoids the
// low-frequency envelope from the array's outer edge / surrounding empty
// margin, which would otherwise masquerade as a beat.  (x0,y0)-(x1,y1) half-
// open in pixels.
double beatFracWindow(const std::vector<unsigned char>& rgba,
                      int w,
                      int x0,
                      int y0,
                      int x1,
                      int y1)
{
  const int ww = x1 - x0;
  const int hh = y1 - y0;
  std::vector<double> cols(ww, 0.0);
  std::vector<double> rows(hh, 0.0);
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const double a = rgba[(static_cast<size_t>(y) * w + x) * 4 + 3];
      cols[x - x0] += a;
      rows[y - y0] += a;
    }
  }
  for (double& v : cols) {
    v /= hh;
  }
  for (double& v : rows) {
    v /= ww;
  }
  return std::max(beatBandFraction1D(cols), beatBandFraction1D(rows));
}

// Coefficient of variation of per-block mean alpha.  High when the image has
// structure at the block scale (a resolved grid); ~0 for a uniform tint.
double blockAlphaCV(const std::vector<unsigned char>& rgba,
                    int w,
                    int h,
                    int block)
{
  std::vector<double> means;
  for (int by = 0; by + block <= h; by += block) {
    for (int bx = 0; bx + block <= w; bx += block) {
      double s = 0.0;
      for (int y = by; y < by + block; ++y) {
        for (int x = bx; x < bx + block; ++x) {
          s += rgba[(static_cast<size_t>(y) * w + x) * 4 + 3];
        }
      }
      means.push_back(s / (block * block));
    }
  }
  if (means.empty()) {
    return 0.0;
  }
  double mean = 0.0;
  for (const double v : means) {
    mean += v;
  }
  mean /= means.size();
  if (mean <= 0.0) {
    return 0.0;
  }
  double var = 0.0;
  for (const double v : means) {
    var += (v - mean) * (v - mean);
  }
  var /= means.size();
  return std::sqrt(var) / mean;
}

class TileGeneratorTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    // Nangate45Fixture gives us a chip + block with die area (0,0)-(1000,1000).
    // Enlarge to fit standard cells (Nangate45 LEF units = 2000, so
    // 100000 dbu = 50 um).
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
  }

  // Create TileGenerator.  Call this after placing any instances so
  // that the block BBox (used by getBounds) is up to date.
  void makeTileGen()
  {
    tile_gen_ = std::make_unique<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
  }

  // Decode a PNG byte vector into raw RGBA pixels.
  std::vector<unsigned char> decodePng(
      const std::vector<unsigned char>& png_data,
      unsigned& width,
      unsigned& height)
  {
    std::vector<unsigned char> pixels;
    unsigned err = lodepng::decode(pixels, width, height, png_data);
    EXPECT_EQ(err, 0u) << lodepng_error_text(err);
    return pixels;
  }

  // Return true if any pixel in the RGBA buffer has alpha > 0.
  static bool hasNonTransparentPixel(const std::vector<unsigned char>& rgba)
  {
    for (size_t i = 3; i < rgba.size(); i += 4) {
      if (rgba[i] > 0) {
        return true;
      }
    }
    return false;
  }

  static size_t countNonTransparentPixels(
      const std::vector<unsigned char>& rgba)
  {
    size_t count = 0;
    for (size_t i = 3; i < rgba.size(); i += 4) {
      if (rgba[i] > 0) {
        ++count;
      }
    }
    return count;
  }

  // Return true if any visible pixel is NOT the gray die/core outline
  // ({128,128,128,255}) drawn on the _instances pass.
  // True if any visible pixel isn't part of the always-on die/core outline.
  // The outline is neutral gray (kOutlineGray); alpha is NOT checked because
  // tiles are rasterized supersampled and Lanczos-decimated, so edge pixels
  // come back with partial coverage (observed 64..197) while the RGB stays
  // 128,128,128.
  static bool hasNonOutlinePixel(const std::vector<unsigned char>& rgba)
  {
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
      if (rgba[i + 3] == 0) {
        continue;
      }
      if (rgba[i] != 128 || rgba[i + 1] != 128 || rgba[i + 2] != 128) {
        return true;
      }
    }
    return false;
  }

  odb::dbInst* placeInst(const char* master_name,
                         const char* inst_name,
                         int x,
                         int y)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    EXPECT_NE(master, nullptr) << "Master not found: " << master_name;
    odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst;
  }

  // Create a BTerm pin on a metal layer at the die boundary.
  void makeBTermAtEdge(const char* name,
                       const char* layer_name,
                       int x,
                       int y,
                       int w,
                       int h,
                       odb::dbIoType io_type = odb::dbIoType::INPUT)
  {
    odb::dbNet* net = odb::dbNet::create(block_, name);
    odb::dbBTerm* bterm = odb::dbBTerm::create(net, name);
    bterm->setIoType(io_type);
    odb::dbBPin* bpin = odb::dbBPin::create(bterm);
    odb::dbTechLayer* layer = getDb()->getTech()->findLayer(layer_name);
    ASSERT_NE(layer, nullptr);
    odb::dbBox::create(bpin, layer, x, y, x + w, y + h);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  // Create a BTerm on an existing net (for net-type filtering tests).
  void makeBTermOnNet(const char* name,
                      odb::dbNet* net,
                      const char* layer_name,
                      int x,
                      int y,
                      int w,
                      int h)
  {
    odb::dbBTerm* bterm = odb::dbBTerm::create(net, name);
    bterm->setIoType(odb::dbIoType::INPUT);
    odb::dbBPin* bpin = odb::dbBPin::create(bterm);
    odb::dbTechLayer* layer = getDb()->getTech()->findLayer(layer_name);
    ASSERT_NE(layer, nullptr);
    odb::dbBox::create(bpin, layer, x, y, x + w, y + h);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  // Build a square design whose z=1 tile seam is centered on the die, attach a
  // single-bin heat map (bin grid over the block bbox) whose populated bin is
  // `cell`, and stash it in heatmap_.  Invoke via ASSERT_NO_FATAL_FAILURE so a
  // geometry-assert failure aborts the caller.
  void buildSeamDesign(const odb::Rect& cell)
  {
    odb::dbMaster* master = lib_->findMaster("BUF_X16");
    ASSERT_NE(master, nullptr);
    const int w = master->getWidth();
    const int h = master->getHeight();
    block_->setDieArea(odb::Rect(0, 0, kSeamDieSide, kSeamDieSide));
    placeInst("BUF_X16", "buf_ll", 0, 0);
    placeInst("BUF_X16", "buf_ur", kSeamDieSide - w, kSeamDieSide - h);

    makeTileGen();
    // The bin grid uses the (clean) block bbox; the tile grid uses getBounds(),
    // which adds a symmetric pin-label margin, so both seams stay at the bbox
    // center -- where the target bin is centered -- and bins/tiles agree there.
    const odb::Rect blk = block_->getBBox()->getBox();
    ASSERT_EQ(blk.xMin(), 0);
    ASSERT_EQ(blk.yMin(), 0);
    ASSERT_EQ(blk.xMax(), kSeamDieSide);
    ASSERT_EQ(blk.yMax(), kSeamDieSide);
    const odb::Rect bounds = tile_gen_->getBounds();
    ASSERT_EQ(bounds.dx(), bounds.dy());  // square => seams at center...
    ASSERT_EQ(bounds.xMin() + bounds.xMax(), kSeamDieSide);  // ...x = kSide/2
    ASSERT_EQ(bounds.yMin() + bounds.yMax(), kSeamDieSide);  // ...y = kSide/2

    heatmap_ = std::make_unique<BoundaryHeatMap>(getLogger(), blk, cell);
    heatmap_->setChip(chip_);
    heatmap_->setGridSizes(15.0, 15.0);  // 15 um bins -> 30000 DBU (3x3 grid)
    // Never gate the bin out of the visible map on value range.
    heatmap_->setDrawBelowRangeMin(true);
    heatmap_->setDrawAboveRangeMax(true);
  }

  // Render tile (zoom,x,y) of heatmap_ with numbers on and off and return the
  // columns/rows (per `axis`) whose pixels the label adds.
  std::set<int> seamTextPixels(int zoom, int x, int y, Axis axis)
  {
    unsigned width = 0;
    unsigned height = 0;
    heatmap_->setShowNumbers(true);
    const std::vector<unsigned char> on = decodePng(
        tile_gen_->generateHeatMapTile(*heatmap_, zoom, x, y), width, height);
    heatmap_->setShowNumbers(false);
    const std::vector<unsigned char> off = decodePng(
        tile_gen_->generateHeatMapTile(*heatmap_, zoom, x, y), width, height);
    return textPixels(on, off, axis);
  }

  // Create an IO pin (net+bterm+bpin box on metal1) carrying one
  // dbAccessPoint at (2000,2000) with access granted.  Returns nullptr
  // if metal1 is missing.
  odb::dbAccessPoint* makeMetal1AccessPoint()
  {
    odb::dbNet* net = odb::dbNet::create(block_, "ap_pin");
    odb::dbBTerm* bterm = odb::dbBTerm::create(net, "ap_pin");
    bterm->setIoType(odb::dbIoType::INPUT);
    odb::dbBPin* bpin = odb::dbBPin::create(bterm);
    odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
    if (!metal1) {
      return nullptr;
    }
    odb::dbBox::create(bpin, metal1, 1900, 1900, 2100, 2100);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    odb::dbAccessPoint* ap = odb::dbAccessPoint::create(bpin);
    ap->setPoint(odb::Point(2000, 2000));
    ap->setLayer(metal1);
    ap->setAccess(true, odb::dbDirection::EAST);
    return ap;
  }

  std::unique_ptr<TileGenerator> tile_gen_;
  std::unique_ptr<BoundaryHeatMap> heatmap_;
};

TEST_F(TileGeneratorTest, HasStaFalseWhenNull)
{
  makeTileGen();
  EXPECT_FALSE(tile_gen_->hasSta());
}

TEST_F(TileGeneratorTest, GetBoundsReflectsInstances)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  odb::Rect bounds = tile_gen_->getBounds();
  // Bounds should encompass the placed instance.
  EXPECT_GT(bounds.dx(), 0);
  EXPECT_GT(bounds.dy(), 0);
  EXPECT_LE(bounds.xMin(), 10000);
  EXPECT_LE(bounds.yMin(), 10000);
}

TEST_F(TileGeneratorTest, BoundsIncludeLabelMargin)
{
  // Place instances to fill the BBox across the die.
  placeInst("BUF_X16", "buf_ll", 0, 0);
  placeInst("BUF_X16", "buf_ur", 90000, 90000);

  // Create a BTerm pin at the right die edge.
  const char* pin_name = "my_long_pin_name";
  odb::dbNet* net = odb::dbNet::create(block_, pin_name);
  odb::dbBTerm* bterm = odb::dbBTerm::create(net, pin_name);
  bterm->setIoType(odb::dbIoType::INPUT);
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  odb::dbTechLayer* m1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(m1, nullptr);
  // Place at right die edge (x=99800..100000).
  odb::dbBox::create(bpin, m1, 99800, 50000, 100000, 50200);
  bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  makeTileGen();
  const odb::Rect die = block_->getDieArea();
  const odb::Rect bounds = tile_gen_->getBounds();

  // The margin should be larger than just the pin marker size,
  // because it now accounts for the label text width.
  const int pin_max = tile_gen_->getPinMaxSize();
  const int margin = bounds.xMax() - die.xMax();
  EXPECT_GT(margin, pin_max);
}

TEST_F(TileGeneratorTest, GetLayers)
{
  makeTileGen();
  std::vector<std::string> layers = tile_gen_->getLayers();
  // getLayers() now returns every tech layer (all types) so the Implant/
  // Other categories and the saveReport prerender can cover them: Nangate45
  // has 10 routing + 9 cut plus poly/active (MASTERSLICE) and OVERLAP = 22,
  // ordered bottom-up starting at "poly" and ending at "OVERLAP".
  EXPECT_EQ(layers.size(), 22);
  EXPECT_EQ(layers.front(), "poly");
  EXPECT_EQ(layers.back(), "OVERLAP");
}

// The per-layer "pattern" request field maps to TileVisibility::fill_pattern,
// with out-of-range values clamped to solid so a bad payload can't index
// outside the FillPattern enum.
TEST_F(TileGeneratorTest, FillPatternParsingClampsToEnum)
{
  // Absent → solid (the historical default).
  TileVisibility vis_default;
  vis_default.parseFromJson(parseObj(R"({})"));
  EXPECT_EQ(vis_default.fill_pattern, FillPattern::kSolid);

  // In-range values map straight through.
  TileVisibility vis_none;
  vis_none.parseFromJson(parseObj(R"({"pattern":0})"));
  EXPECT_EQ(vis_none.fill_pattern, FillPattern::kNone);

  TileVisibility vis_dots;
  vis_dots.parseFromJson(parseObj(R"({"pattern":4})"));
  EXPECT_EQ(vis_dots.fill_pattern, FillPattern::kDots);

  // Out-of-range (above and below) clamps back to solid.
  TileVisibility vis_high;
  vis_high.parseFromJson(parseObj(R"({"pattern":99})"));
  EXPECT_EQ(vis_high.fill_pattern, FillPattern::kSolid);

  TileVisibility vis_neg;
  vis_neg.parseFromJson(parseObj(R"({"pattern":-1})"));
  EXPECT_EQ(vis_neg.fill_pattern, FillPattern::kSolid);
}

// A non-solid fill pattern thins a layer's own shapes (fewer painted pixels
// than solid, but still some), and kNone paints nothing.  Uses a large metal1
// BTerm as the only content so the pixel counts reflect just the pattern.
TEST_F(TileGeneratorTest, FillPatternControlsShapeCoverage)
{
  makeBTermAtEdge("pad", "metal1", 30000, 30000, 40000, 40000);
  makeTileGen();
  tile_gen_->eagerInit();

  auto paintedPixels = [&](const FillPattern pattern) {
    TileVisibility vis;
    vis.fill_pattern = pattern;
    const auto png = tile_gen_->generateTile("metal1", 0, 0, 0, vis);
    unsigned w = 0, h = 0;
    const auto px = decodePng(png, w, h);
    size_t painted = 0;
    for (size_t i = 3; i < px.size(); i += 4) {
      if (px[i] > 0) {
        ++painted;
      }
    }
    return painted;
  };

  const size_t solid = paintedPixels(FillPattern::kSolid);
  const size_t diagonal = paintedPixels(FillPattern::kDiagonal);
  const size_t none = paintedPixels(FillPattern::kNone);

  EXPECT_GT(solid, 0u) << "solid fill should paint the shape";
  EXPECT_EQ(none, 0u) << "kNone should paint nothing";
  EXPECT_GT(diagonal, 0u) << "a hatch should still paint some pixels";
  EXPECT_LT(diagonal, solid) << "a hatch should paint fewer pixels than solid";
}

// Layer colors must mirror gui::DisplayControls::techInit so the GUI and the
// web frontend show the same color for the same layer.  Nangate45 only has 10
// routing + 9 cut layers, all within the 14-entry built-in palettes, so we
// extend the tech to 20 routing + 19 cut layers to also exercise the overflow
// path: layers past the palette get deterministic mt19937(1)-seeded random
// colors.  The expected RGB values below were computed by replaying the exact
// blue/green/red draw order (matching gui::DisplayControls::techInit) over the
// full getLayers() iteration, including the MASTERSLICE/OVERLAP layers that
// also consume random draws.
TEST_F(TileGeneratorTest, GetLayerColorMapMatchesGuiPalette)
{
  odb::dbTech* tech = getDb()->getTech();
  ASSERT_NE(tech, nullptr);

  // Grow the stack to 20 routing + 19 cut layers (metal11..metal20 +
  // via10..via19), created interleaved (metalN, via(N-1)) just like a real
  // LEF, so getLayers() yields them in that order.
  for (int i = 11; i <= 20; ++i) {
    odb::dbTechLayer::create(tech,
                             ("metal" + std::to_string(i)).c_str(),
                             odb::dbTechLayerType::ROUTING);
    odb::dbTechLayer::create(tech,
                             ("via" + std::to_string(i - 1)).c_str(),
                             odb::dbTechLayerType::CUT);
  }

  makeTileGen();
  const auto& colors = tile_gen_->getLayerColorMap();

  // Helper: assert a layer's color matches an expected RGB (alpha is always
  // 180 in both the GUI and the web palette).
  auto expectColor = [&](const char* name, int r, int g, int b) {
    odb::dbTechLayer* layer = tech->findLayer(name);
    ASSERT_NE(layer, nullptr) << "missing layer " << name;
    const Color c = colors.at(layer);
    EXPECT_EQ(c.r, r) << name << " red";
    EXPECT_EQ(c.g, g) << name << " green";
    EXPECT_EQ(c.b, b) << name << " blue";
    EXPECT_EQ(c.a, 180) << name << " alpha";
  };

  struct LayerColor
  {
    const char* name;
    int r;
    int g;
    int b;
  };

  // All 20 routing layers: metal1..metal14 are the seeded kMetalColors palette
  // (#00F, #F00, #0D0, ...), metal15..metal20 are the mt19937(1) overflow.
  const LayerColor kRouting[] = {
      {"metal1", 0, 0, 254},
      {"metal2", 254, 0, 0},
      {"metal3", 9, 221, 0},
      {"metal4", 190, 244, 81},
      {"metal5", 222, 33, 96},
      {"metal6", 32, 216, 253},
      {"metal7", 253, 108, 160},
      {"metal8", 117, 63, 194},
      {"metal9", 128, 155, 49},
      {"metal10", 234, 63, 252},
      {"metal11", 9, 96, 19},
      {"metal12", 214, 120, 239},
      {"metal13", 192, 222, 164},
      {"metal14", 110, 68, 107},
      // Overflow (random_color past the 14-entry palette).
      {"metal15", 99, 98, 82},
      {"metal16", 63, 193, 166},
      {"metal17", 200, 166, 92},
      {"metal18", 124, 126, 173},
      {"metal19", 137, 246, 68},
      {"metal20", 242, 216, 153},
  };

  // All 19 cut layers: via1..via14 are the seeded kCutColors palette,
  // via15..via19 are the mt19937(1) overflow.
  const LayerColor kCut[] = {
      {"via1", 126, 126, 255},
      {"via2", 255, 126, 126},
      {"via3", 4, 110, 0},
      {"via4", 95, 122, 40},
      {"via5", 111, 17, 48},
      {"via6", 16, 108, 126},
      {"via7", 126, 54, 80},
      {"via8", 58, 32, 97},
      {"via9", 225, 255, 136},
      {"via10", 117, 32, 126},
      {"via11", 18, 192, 38},
      {"via12", 107, 60, 119},
      {"via13", 96, 111, 82},
      {"via14", 220, 136, 214},
      // Overflow (random_color past the 14-entry palette).
      {"via15", 171, 152, 190},
      {"via16", 54, 196, 143},
      {"via17", 104, 79, 102},
      {"via18", 123, 187, 153},
      {"via19", 179, 175, 160},
  };

  for (const LayerColor& lc : kRouting) {
    expectColor(lc.name, lc.r, lc.g, lc.b);
  }
  for (const LayerColor& lc : kCut) {
    expectColor(lc.name, lc.r, lc.g, lc.b);
  }
}

// Only frontside metals should consume the palette colors.
TEST_F(TileGeneratorTest, GetLayerColorMapWithBacksideMetals)
{
  odb::dbTech* tech = getDb()->getTech();
  ASSERT_NE(tech, nullptr);

  // make metals 1 -> 3 backside
  for (const char* name :
       {"metal1", "via1", "metal2", "via2", "metal3", "via3"}) {
    odb::dbTechLayer* layer = tech->findLayer(name);
    ASSERT_NE(layer, nullptr) << "missing layer " << name;
    layer->setBackside(true);
  }

  makeTileGen();
  const auto& colors = tile_gen_->getLayerColorMap();

  // Helper: assert a layer's color matches an expected RGB (alpha is always
  // 180 in both the GUI and the web palette).
  auto expectColor = [&](const char* name, int r, int g, int b) {
    odb::dbTechLayer* layer = tech->findLayer(name);
    ASSERT_NE(layer, nullptr) << "missing layer " << name;
    const Color c = colors.at(layer);
    EXPECT_EQ(c.r, r) << name << " red";
    EXPECT_EQ(c.g, g) << name << " green";
    EXPECT_EQ(c.b, b) << name << " blue";
    EXPECT_EQ(c.a, 180) << name << " alpha";
  };

  struct LayerColor
  {
    const char* name;
    int r;
    int g;
    int b;
  };

  // All 20 routing layers: metal1..metal14 are the seeded kMetalColors palette
  // (#00F, #F00, #0D0, ...), metal15..metal20 are the mt19937(1) overflow.
  const LayerColor kRouting[] = {// Backside
                                 {"metal1", 209, 191, 141},
                                 {"metal2", 63, 193, 166},
                                 {"metal3", 200, 166, 92},
                                 // Frontside
                                 {"metal4", 0, 0, 254},
                                 {"metal5", 254, 0, 0},
                                 {"metal6", 9, 221, 0},
                                 {"metal7", 190, 244, 81},
                                 {"metal8", 222, 33, 96},
                                 {"metal9", 32, 216, 253}};

  // All 19 cut layers: via1..via14 are the seeded kCutColors palette,
  // via15..via19 are the mt19937(1) overflow.
  const LayerColor kCut[] = {// Backside
                             {"via1", 99, 98, 82},
                             {"via2", 171, 152, 190},
                             {"via3", 54, 196, 143},
                             // Frontside
                             {"via4", 126, 126, 255},
                             {"via5", 255, 126, 126},
                             {"via6", 4, 110, 0},
                             {"via7", 95, 122, 40},
                             {"via8", 111, 17, 48},
                             {"via9", 16, 108, 126}};

  for (const LayerColor& lc : kRouting) {
    expectColor(lc.name, lc.r, lc.g, lc.b);
  }
  for (const LayerColor& lc : kCut) {
    expectColor(lc.name, lc.r, lc.g, lc.b);
  }
}

TEST_F(TileGeneratorTest, GetLayerColorMapIsCached)
{
  makeTileGen();
  // Identity check: same tech ⇒ same map object.  This is the contract that
  // makes caching observable to callers (no rebuild between tile renders).
  const auto& first = tile_gen_->getLayerColorMap();
  const auto& second = tile_gen_->getLayerColorMap();
  EXPECT_EQ(&first, &second);
}

TEST_F(TileGeneratorTest, EagerInitClearsLayerColorCache)
{
  makeTileGen();
  // Prime the cache.
  tile_gen_->getLayerColorMap();
  // eagerInit must drop cached entries so a reloaded design with a new
  // dbTech allocated at the same address can't read stale colors.
  tile_gen_->eagerInit();
  // Recomputing still produces correct values.
  const auto& colors = tile_gen_->getLayerColorMap();
  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  EXPECT_EQ(colors.at(metal1).r, 0);
  EXPECT_EQ(colors.at(metal1).g, 0);
  EXPECT_EQ(colors.at(metal1).b, 254);
}

// The per-instance render pass reads master OBS and pin shapes out of the
// layer-bucketed geometry cache, so a master's shapes must appear on the layer
// they belong to and nowhere else.  Nangate45 cells carry pin geometry on
// metal1 only, so a metal2 tile over the same instance must come back empty.
TEST_F(TileGeneratorTest, MasterPinGeometryOnlyDrawnOnItsOwnLayer)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  unsigned w = 0, h = 0;
  auto m1 = decodePng(tile_gen_->generateTile("metal1", 0, 0, 0), w, h);
  EXPECT_TRUE(hasNonTransparentPixel(m1))
      << "metal1 tile should show the cell's pin shapes";

  auto m2 = decodePng(tile_gen_->generateTile("metal2", 0, 0, 0), w, h);
  EXPECT_FALSE(hasNonTransparentPixel(m2))
      << "metal2 tile should be empty: no Nangate45 master has geometry there";
}

// The cache is handed out as a snapshot; repeat calls with no intervening edit
// must return the same one, otherwise every tile would rewalk every master.
TEST_F(TileGeneratorTest, GeomCacheReusedWhenDesignUnchanged)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  EXPECT_EQ(tile_gen_->geomCache(), tile_gen_->geomCache());
}

// Regression: the geometry cache must not be tied to the design-changed
// callback, which is debounced to a valid→invalid index transition.  The second
// edit below leaves the instance index already invalid, so that callback stays
// silent -- and a cache keyed on it would keep serving a pre-edit snapshot,
// silently dropping the geometry of any master or via the edit introduced.
TEST_F(TileGeneratorTest, GeomCacheRebuiltAfterDebouncedEdit)
{
  odb::dbInst* inst = placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  // Registers Search as a db callback object and builds the indices, so the
  // first edit below is the valid→invalid transition and the second is not.
  tile_gen_->eagerInit();

  auto before = tile_gen_->geomCache();
  ASSERT_NE(before, nullptr);

  // First edit: index was valid, so the debounced callback does fire.
  inst->setLocation(2000, 2000);
  auto after_first = tile_gen_->geomCache();
  EXPECT_NE(before, after_first);

  // Second edit: index is already invalid, so the callback does NOT fire.  The
  // cache still has to notice, via Search::revision().
  inst->setLocation(4000, 4000);
  auto after_second = tile_gen_->geomCache();
  EXPECT_NE(after_first, after_second)
      << "geometry cache went stale across an edit that the debounced "
         "design-changed callback does not report";
}

// Build a HIER root chip holding `num_insts` instances of the fixture's chip,
// and make it the top chip so chiplets() traverses down into that one block
// once per instance.  Returns the root.
odb::dbChip* makeSharedChipletRoot(odb::dbDatabase* db,
                                   odb::dbChip* master,
                                   const int num_insts)
{
  odb::dbChip* root
      = odb::dbChip::create(db, nullptr, "root", odb::dbChip::ChipType::HIER);
  db->setTopChip(root);
  for (int i = 0; i < num_insts; ++i) {
    odb::dbChipInst::create(root, master, "die" + std::to_string(i));
  }
  return root;
}

// Add a block via with one cut box on `layer`, which is what the geometry
// cache's via_boxes map is built from.
odb::dbVia* makeBlockVia(odb::dbBlock* block,
                         odb::dbTechLayer* layer,
                         const char* name)
{
  odb::dbVia* via = odb::dbVia::create(block, name);
  odb::dbBox::create(via, layer, -50, -50, 50, 50);
  return via;
}

// Regression: chiplets() reports one node per dbChipInst, so instances sharing
// a master chip all report the same block.  Collecting that block's vias once
// per instance would leave the render pass redrawing every box once per
// instance of the chiplet -- invisible in the output (fills are opaque) but
// quadratic in the repeat count, and it multiplies the cache's memory by it
// too.
TEST_F(TileGeneratorTest, GeomCacheVisitsASharedChipletBlockOnce)
{
  odb::dbTechLayer* via1 = getDb()->getTech()->findLayer("via1");
  ASSERT_NE(via1, nullptr);
  odb::dbVia* via = makeBlockVia(block_, via1, "V1");
  makeSharedChipletRoot(getDb(), chip_, /*num_insts=*/3);
  makeTileGen();

  auto cache = tile_gen_->geomCache();
  ASSERT_NE(cache, nullptr);
  const auto layer_it = cache->via_boxes.find(via1);
  ASSERT_NE(layer_it, cache->via_boxes.end());
  const auto via_it = layer_it->second.find(via);
  ASSERT_NE(via_it, layer_it->second.end());
  EXPECT_EQ(via_it->second.size(), 1u)
      << "block via decomposed once per dbChipInst instead of once per block";
}

// Regression: creating a dbChipInst fires no dbBlockCallBackObj, so it cannot
// move Search::revision() -- but it does make an already-populated block's vias
// newly reachable, which is what via_boxes is keyed off.  A cache keyed on the
// revision alone keeps a snapshot built before the chiplet existed, and the new
// chiplet's special-net vias silently stop drawing.
TEST_F(TileGeneratorTest, GeomCacheRebuiltAfterChipletInstCreated)
{
  odb::dbTechLayer* via1 = getDb()->getTech()->findLayer("via1");
  ASSERT_NE(via1, nullptr);

  // A second chip, off to the side of the hierarchy and carrying a via of its
  // own, so the cache built below provably cannot contain it yet.
  odb::dbChip* other
      = odb::dbChip::create(getDb(), getDb()->getTech(), "other");
  odb::dbBlock* other_block = odb::dbBlock::create(other, "other_top");
  other_block->setDieArea(odb::Rect(0, 0, 1000, 1000));
  odb::dbVia* other_via = makeBlockVia(other_block, via1, "V1_other");

  odb::dbChip* root = makeSharedChipletRoot(getDb(), chip_, /*num_insts=*/1);
  makeTileGen();
  tile_gen_->eagerInit();

  auto before = tile_gen_->geomCache();
  ASSERT_NE(before, nullptr);
  {
    const auto layer_it = before->via_boxes.find(via1);
    if (layer_it != before->via_boxes.end()) {
      EXPECT_EQ(layer_it->second.find(other_via), layer_it->second.end())
          << "unreachable chip's via cached before its chiplet existed";
    }
  }

  odb::dbChipInst::create(root, other, "other_die");

  auto after = tile_gen_->geomCache();
  EXPECT_NE(before, after)
      << "geometry cache went stale across a chiplet-hierarchy edit, which no "
         "block callback reports";
  const auto layer_it = after->via_boxes.find(via1);
  ASSERT_NE(layer_it, after->via_boxes.end());
  EXPECT_NE(layer_it->second.find(other_via), layer_it->second.end())
      << "new chiplet's block vias missing from the cache, so its special-net "
         "vias would not draw";
}

TEST_F(TileGeneratorTest, SerializeTechResponseIncludesLayerColors)
{
  makeTileGen();
  const std::string json
      = boost::json::serialize(serializeTechResponse(*tile_gen_));

  EXPECT_NE(json.find("\"layer_colors\""), std::string::npos)
      << "tech response missing layer_colors key; got: " << json;
  // The metal1 color [0,0,254] should appear since metal1 is layers[0].
  EXPECT_NE(json.find("[0,0,254]"), std::string::npos)
      << "tech response missing metal1 color [0,0,254]; got: " << json;
}

TEST_F(TileGeneratorTest, GenerateTileReturnsValidPng)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  auto png = tile_gen_->generateTile("metal1", 0, 0, 0);
  ASSERT_FALSE(png.empty());

  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 256u);
  EXPECT_EQ(h, 256u);
}

// A tile must render exactly the DBU window the client's coordinate transform
// assigns to it: [xMin + x*T, xMin + (x+1)*T) with T = maxDXDY/2^z.  T is
// fractional, so the window's origin is fractional too, and truncating it to an
// int (as this code used to) shifts the tile's content by frac(origin) DBU — by
// a DIFFERENT amount in each tile, since each has its own fractional part. That
// is what tears content apart along a shared edge and shows up as a hairline
// seam in the viewer.  The shift is frac * (256*dpr/T) device px: invisible
// while a DBU is smaller than a pixel, and past a pixel wide once you zoom in —
// sooner, and twice as wide, on a HiDPI display.
TEST_F(TileGeneratorTest, TileContentRegistersWithIdealGrid)
{
  constexpr int kZoom = 10;
  const int num_tiles = 1 << kZoom;

  // Pin the block bbox to a known square: getBounds() follows the bbox (the
  // die area alone does not move it), and the tile grid is derived from it.
  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  ASSERT_NE(master, nullptr);
  block_->setDieArea(odb::Rect(0, 0, kSeamDieSide, kSeamDieSide));
  placeInst("BUF_X16", "buf_ll", 0, 0);
  placeInst("BUF_X16",
            "buf_ur",
            kSeamDieSide - master->getWidth(),
            kSeamDieSide - master->getHeight());

  makeTileGen();
  const odb::Rect bounds = tile_gen_->getBounds();
  ASSERT_EQ(bounds.dx(), bounds.dy()) << "test derives T from a square bounds";
  const double tile_dbu = static_cast<double>(bounds.maxDXDY()) / num_tiles;
  // A fractional tile size is the whole point: with an integer one every origin
  // is exact and there is nothing to get wrong.
  ASSERT_NE(tile_dbu, std::floor(tile_dbu));

  // The two tile columns whose ideal origins have the smallest and the largest
  // fractional part.  Their truncation errors differ the most, so content in
  // one is offset from content in the other by the most the bug can produce.
  int k_lo = -1;
  int k_hi = -1;
  double frac_lo = 2.0;
  double frac_hi = -1.0;
  for (int k = num_tiles / 4; k < num_tiles / 2; ++k) {
    const double org = bounds.xMin() + k * tile_dbu;
    const double frac = org - std::floor(org);
    if (frac < frac_lo) {
      frac_lo = frac;
      k_lo = k;
    }
    if (frac > frac_hi) {
      frac_hi = frac;
      k_hi = k;
    }
  }
  ASSERT_GE(k_lo, 0);
  ASSERT_GE(k_hi, 0);
  // Each stripe below spans 1.5 tiles, so the two must not be neighbours.
  ASSERT_GE(std::abs(k_hi - k_lo), 2);

  // One metal1 stripe per column, its left edge at the column's mid-point.  The
  // stripe runs past the tile's right edge and past both its horizontal edges,
  // so the measured row is uncovered left of the edge and fully covered right
  // of it all the way to the tile border.
  const int row_tile = num_tiles / 2;
  const int y_lo = static_cast<int>(
      std::llround(bounds.yMin() + (row_tile - 1) * tile_dbu));
  const int y_hi = static_cast<int>(
      std::llround(bounds.yMin() + (row_tile + 2) * tile_dbu));
  odb::dbTechLayer* m1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(m1, nullptr);
  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);
  odb::dbSWire* swire = odb::dbSWire::create(pwr, odb::dbWireType::ROUTED);
  const std::array<int, 2> columns = {k_lo, k_hi};
  std::array<int, 2> edge_dbu = {0, 0};
  for (size_t i = 0; i < columns.size(); ++i) {
    const double org = bounds.xMin() + columns[i] * tile_dbu;
    edge_dbu[i] = static_cast<int>(std::llround(org + tile_dbu / 2));
    odb::dbSBox::create(swire,
                        m1,
                        edge_dbu[i],
                        y_lo,
                        static_cast<int>(std::llround(org + 1.5 * tile_dbu)),
                        y_hi,
                        odb::dbWireShapeType::STRIPE);
  }

  makeTileGen();
  // The stripes sit inside the die, so they must not have moved the bounds the
  // placements above were derived from.
  ASSERT_EQ(tile_gen_->getBounds(), bounds);

  // Client tile y is top-down, the renderer's is bottom-up.
  const int tile_y = num_tiles - 1 - row_tile;
  const TileVisibility vis;
  for (const DprCase& dpr_case : kDprCases) {
    const int dim = tilePxFor(dpr_case);
    const double dpr = dpr_case.dpr;
    const double px_per_dbu = dim / tile_dbu;
    for (size_t i = 0; i < columns.size(); ++i) {
      unsigned w = 0;
      unsigned h = 0;
      const std::vector<unsigned char> rgba
          = decodePng(tile_gen_->generateTile("metal1",
                                              kZoom,
                                              columns[i],
                                              tile_y,
                                              vis,
                                              {},
                                              {},
                                              {},
                                              {},
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              dpr,
                                              dim),
                      w,
                      h);
      ASSERT_EQ(w, static_cast<unsigned>(dim));
      const double org = bounds.xMin() + columns[i] * tile_dbu;
      const double expected = (edge_dbu[i] - org) * px_per_dbu;
      const double measured = coverageEdgeX(rgba, dim, dim / 2);
      EXPECT_NEAR(measured, expected, 1.0)
          << dpr_case.what << ": tile column " << columns[i] << " at dpr "
          << dpr << " (" << dim << " px)" << ": stripe edge at " << edge_dbu[i]
          << " dbu renders " << (measured - expected)
          << " px from where the ideal tile origin " << org
          << " puts it (tile size " << tile_dbu << " dbu)";
    }
  }
}

// The client names the pixel count it will display the tile in, instead of the
// server deriving it from a rounded dpr.  A tile's CSS box is a whole number of
// device pixels only when tileSize*dpr is an integer: at a 1.6667 display scale
// (a 166% desktop, and the ratio this was reported on) 256 CSS px is 426.67
// device px, so any size derived here is one the browser has to resample —
// which softens every tile edge and puts the boundaries off the device grid.
TEST_F(TileGeneratorTest, ExplicitTilePixelCountIsHonoured)
{
  constexpr int kZoom = 10;
  const int num_tiles = 1 << kZoom;
  // What a 255 CSS px tile is worth on a 1.6667 display: whole, unlike 256.
  constexpr int kDeviceExactPx = 425;
  const double dpr = 425.0 / 255.0;

  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  ASSERT_NE(master, nullptr);
  block_->setDieArea(odb::Rect(0, 0, kSeamDieSide, kSeamDieSide));
  placeInst("BUF_X16", "buf_ll", 0, 0);
  placeInst("BUF_X16",
            "buf_ur",
            kSeamDieSide - master->getWidth(),
            kSeamDieSide - master->getHeight());

  makeTileGen();
  const odb::Rect bounds = tile_gen_->getBounds();
  const double tile_dbu = static_cast<double>(bounds.maxDXDY()) / num_tiles;

  const int column = num_tiles / 2;
  const int row_tile = num_tiles / 2;
  const double org = bounds.xMin() + column * tile_dbu;
  const int edge_dbu = static_cast<int>(std::llround(org + tile_dbu / 2));
  odb::dbTechLayer* m1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(m1, nullptr);
  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);
  odb::dbSWire* swire = odb::dbSWire::create(pwr, odb::dbWireType::ROUTED);
  odb::dbSBox::create(
      swire,
      m1,
      edge_dbu,
      static_cast<int>(std::llround(bounds.yMin() + (row_tile - 1) * tile_dbu)),
      static_cast<int>(std::llround(org + 1.5 * tile_dbu)),
      static_cast<int>(std::llround(bounds.yMin() + (row_tile + 2) * tile_dbu)),
      odb::dbWireShapeType::STRIPE);

  makeTileGen();
  ASSERT_EQ(tile_gen_->getBounds(), bounds);

  const TileVisibility vis;
  unsigned w = 0;
  unsigned h = 0;
  const std::vector<unsigned char> rgba
      = decodePng(tile_gen_->generateTile("metal1",
                                          kZoom,
                                          column,
                                          num_tiles - 1 - row_tile,
                                          vis,
                                          {},
                                          {},
                                          {},
                                          {},
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          dpr,
                                          kDeviceExactPx),
                  w,
                  h);
  // Exactly the count asked for -- NOT lround(256*dpr), which would be 427.
  EXPECT_EQ(w, static_cast<unsigned>(kDeviceExactPx));
  EXPECT_EQ(h, static_cast<unsigned>(kDeviceExactPx));
  EXPECT_NE(w, static_cast<unsigned>(std::lround(kTileSize * dpr)));

  // ...and the content still registers on the ideal grid at that size.
  const double expected = (edge_dbu - org) * (kDeviceExactPx / tile_dbu);
  EXPECT_NEAR(
      coverageEdgeX(rgba, kDeviceExactPx, kDeviceExactPx / 2), expected, 1.0);
}

// The symptom the registration above is the cause of: a shape crossing a tile
// boundary must arrive whole.  Two neighbours each rendering their own
// slightly-shifted DBU window either skip a strip of the design between them
// (the dark hairline the viewer shows) or draw one strip twice.
//
// Measured differentially, against an identical stripe that crosses no seam:
// a stripe's own edges cost a pixel or two of coverage to the band-limiting
// filter (whose undershoot clips at alpha 0), and charging that to the seam
// would make this test fail on a perfectly continuous tiling.  The difference
// between the two isolates what the seam alone costs.
TEST_F(TileGeneratorTest, ShapeCrossingTileSeamStaysWhole)
{
  constexpr int kZoom = 10;
  const int num_tiles = 1 << kZoom;

  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  ASSERT_NE(master, nullptr);
  block_->setDieArea(odb::Rect(0, 0, kSeamDieSide, kSeamDieSide));
  placeInst("BUF_X16", "buf_ll", 0, 0);
  placeInst("BUF_X16",
            "buf_ur",
            kSeamDieSide - master->getWidth(),
            kSeamDieSide - master->getHeight());

  makeTileGen();
  const odb::Rect bounds = tile_gen_->getBounds();
  const double tile_dbu = static_cast<double>(bounds.maxDXDY()) / num_tiles;
  ASSERT_NE(tile_dbu, std::floor(tile_dbu));

  // Two stripes of the same width in the same tile row: the reference sits
  // inside one tile, the subject straddles that tile's right edge.  A vertical
  // gap between them (0.7T .. 0.75T) keeps their coverage separable by column.
  const int column = num_tiles / 2;
  const double org = bounds.xMin() + column * tile_dbu;
  const int ref_lo = static_cast<int>(std::llround(org + 0.2 * tile_dbu));
  const int ref_hi = static_cast<int>(std::llround(org + 0.7 * tile_dbu));
  const int seam_lo = static_cast<int>(std::llround(org + 0.75 * tile_dbu));
  const int seam_hi = static_cast<int>(std::llround(org + 1.25 * tile_dbu));
  const int row_tile = num_tiles / 2;
  const int y_lo = static_cast<int>(
      std::llround(bounds.yMin() + (row_tile - 1) * tile_dbu));
  const int y_hi = static_cast<int>(
      std::llround(bounds.yMin() + (row_tile + 2) * tile_dbu));

  odb::dbTechLayer* m1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(m1, nullptr);
  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);
  odb::dbSWire* swire = odb::dbSWire::create(pwr, odb::dbWireType::ROUTED);
  odb::dbSBox::create(
      swire, m1, ref_lo, y_lo, ref_hi, y_hi, odb::dbWireShapeType::STRIPE);
  odb::dbSBox::create(
      swire, m1, seam_lo, y_lo, seam_hi, y_hi, odb::dbWireShapeType::STRIPE);

  makeTileGen();
  ASSERT_EQ(tile_gen_->getBounds(), bounds);

  const int tile_y = num_tiles - 1 - row_tile;
  const TileVisibility vis;
  for (const DprCase& dpr_case : kDprCases) {
    const int dim = tilePxFor(dpr_case);
    const double dpr = dpr_case.dpr;
    const double px_per_dbu = dim / tile_dbu;
    std::vector<std::vector<unsigned char>> tiles;
    for (const int tx : {column, column + 1}) {
      unsigned w = 0;
      unsigned h = 0;
      tiles.push_back(decodePng(tile_gen_->generateTile("metal1",
                                                        kZoom,
                                                        tx,
                                                        tile_y,
                                                        vis,
                                                        {},
                                                        {},
                                                        {},
                                                        {},
                                                        nullptr,
                                                        nullptr,
                                                        nullptr,
                                                        dpr,
                                                        dim),
                                w,
                                h));
      ASSERT_EQ(w, static_cast<unsigned>(dim));
    }
    // Split the first tile's row in the gap between the two stripes.
    const int split = static_cast<int>(0.725 * tile_dbu * px_per_dbu);
    const double ref_covered = coveredWidthPx(tiles[0], dim, dim / 2, 0, split);
    const double seam_covered
        = coveredWidthPx(tiles[0], dim, dim / 2, split, dim)
          + coveredWidthPx(tiles[1], dim, dim / 2, 0, dim);
    const double ref_loss = ref_covered - (ref_hi - ref_lo) * px_per_dbu;
    const double seam_loss = seam_covered - (seam_hi - seam_lo) * px_per_dbu;
    EXPECT_NEAR(seam_loss, ref_loss, 1.0)
        << dpr_case.what << ": at dpr " << dpr
        << " a stripe across the seam between tiles " << column << " and "
        << (column + 1) << " renders " << (seam_loss - ref_loss)
        << " px differently from the same stripe inside one tile: the tiles "
        << (seam_loss < ref_loss ? "skip" : "repeat")
        << " a strip of the design at their shared edge";
  }
}

// Every ratio the viewer can ask for produces a tile of exactly the requested
// size -- the invariant the whole seam fix rests on, since a tile that is not
// the size of its box gets resampled by the browser and its edges fade into its
// neighbours.
TEST_F(TileGeneratorTest, TilePixelCountIsExactAcrossDprMatrix)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  for (const DprCase& dpr_case : kDprCases) {
    const int expected_px = tilePxFor(dpr_case);
    unsigned w = 0;
    unsigned h = 0;
    const std::vector<unsigned char> rgba
        = decodePng(tile_gen_->generateTile("_instances",
                                            0,
                                            0,
                                            0,
                                            TileVisibility{},
                                            {},
                                            {},
                                            {},
                                            {},
                                            nullptr,
                                            nullptr,
                                            nullptr,
                                            dpr_case.dpr,
                                            expected_px),
                    w,
                    h);
    EXPECT_EQ(w, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_EQ(h, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_EQ(rgba.size(), static_cast<size_t>(expected_px) * expected_px * 4)
        << dpr_case.what;
  }
}

// A client that names no pixel count still gets the historical 256*dpr, so an
// older viewer served by a newer binary is unaffected.
TEST_F(TileGeneratorTest, TilePixelCountFallsBackToDprWhenUnspecified)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  for (const double dpr : {1.0, 1.25, 1.6666666269302368, 2.0, 3.0}) {
    unsigned w = 0;
    unsigned h = 0;
    decodePng(tile_gen_->generateTile("_instances",
                                      0,
                                      0,
                                      0,
                                      TileVisibility{},
                                      {},
                                      {},
                                      {},
                                      {},
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      dpr,
                                      /*tile_px=*/0),
              w,
              h);
    EXPECT_EQ(w, static_cast<unsigned>(std::lround(kTileSize * dpr)))
        << "dpr " << dpr;
    EXPECT_EQ(h, w) << "dpr " << dpr;
  }
}

// The pixel count sizes the tile; the ratio scales what is authored in CSS px
// (fonts, stroke widths, the sub-resolution cull).  They are independent inputs
// -- rendering the same tile at the same size with a different ratio must not
// change its dimensions, only that CSS-authored detail.
TEST_F(TileGeneratorTest, TilePixelCountAndDprAreIndependent)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  constexpr int kPx = 400;
  for (const double dpr : {1.0, 1.6666666269302368, 3.0}) {
    unsigned w = 0;
    unsigned h = 0;
    decodePng(tile_gen_->generateTile("_instances",
                                      0,
                                      0,
                                      0,
                                      TileVisibility{},
                                      {},
                                      {},
                                      {},
                                      {},
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      dpr,
                                      kPx),
              w,
              h);
    EXPECT_EQ(w, static_cast<unsigned>(kPx)) << "dpr " << dpr;
  }
}

// Overlay tiles (selection, DRC markers, timing paths, route guides) are drawn
// on top of the layer tiles, so they have to be rendered at the same pixel
// count and on the same grid.  A 256 px overlay stretched over a 400 px layer
// tile is both blurry and misregistered against the shapes it annotates.
TEST_F(TileGeneratorTest, OverlayTileHonoursTheRequestedPixelCount)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  const odb::Rect bounds = tile_gen_->getBounds();
  const std::vector<odb::Rect> highlight = {bounds};

  for (const DprCase& dpr_case : kDprCases) {
    const int expected_px = tilePxFor(dpr_case);
    unsigned w = 0;
    unsigned h = 0;
    const std::vector<unsigned char> rgba
        = decodePng(tile_gen_->generateOverlayTile(0,
                                                   0,
                                                   0,
                                                   highlight,
                                                   {},
                                                   {},
                                                   {},
                                                   nullptr,
                                                   false,
                                                   {},
                                                   dpr_case.dpr,
                                                   expected_px),
                    w,
                    h);
    EXPECT_EQ(w, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_EQ(h, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_TRUE(hasNonTransparentPixel(rgba))
        << dpr_case.what << ": highlight must be drawn at every size";
  }
}

// The highlight lands where the layer tile puts the shape, at every size: both
// paths derive their frame from the same exact tile origin.
TEST_F(TileGeneratorTest, OverlayTileRegistersWithTheLayerTileGrid)
{
  constexpr int kZoom = 4;
  const int num_tiles = 1 << kZoom;
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  const odb::Rect bounds = tile_gen_->getBounds();
  const double tile_dbu = static_cast<double>(bounds.maxDXDY()) / num_tiles;

  // A highlight covering the left half of one tile: its right edge is a
  // measurable feature at a known place in the tile.
  const int column = num_tiles / 2;
  const int row = num_tiles / 2;
  const double org_x = bounds.xMin() + column * tile_dbu;
  const double org_y = bounds.yMin() + row * tile_dbu;
  const int edge_dbu = static_cast<int>(std::llround(org_x + tile_dbu / 2));
  const std::vector<odb::Rect> highlight
      = {odb::Rect(static_cast<int>(std::llround(org_x - tile_dbu)),
                   static_cast<int>(std::llround(org_y - tile_dbu)),
                   edge_dbu,
                   static_cast<int>(std::llround(org_y + 2 * tile_dbu)))};

  for (const DprCase& dpr_case : kDprCases) {
    const int dim = tilePxFor(dpr_case);
    unsigned w = 0;
    unsigned h = 0;
    const std::vector<unsigned char> rgba
        = decodePng(tile_gen_->generateOverlayTile(kZoom,
                                                   column,
                                                   num_tiles - 1 - row,
                                                   highlight,
                                                   {},
                                                   {},
                                                   {},
                                                   nullptr,
                                                   false,
                                                   {},
                                                   dpr_case.dpr,
                                                   dim),
                    w,
                    h);
    ASSERT_EQ(w, static_cast<unsigned>(dim)) << dpr_case.what;
    // Covered from the tile's left edge up to the highlight's right edge, so
    // the covered-column count is that edge's position in pixels.
    const double expected = (edge_dbu - org_x) * (dim / tile_dbu);
    const double measured = coveredColumns(rgba, dim, dim / 2);
    EXPECT_NEAR(measured, expected, 2.0)
        << dpr_case.what << ": highlight edge at " << edge_dbu
        << " dbu renders " << (measured - expected) << " px from where the "
        << "layer tile grid puts it";
  }
}

TEST_F(TileGeneratorTest, OverlayTileFallsBackToDprWhenUnspecified)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  for (const double dpr : {1.0, 1.25, 1.6666666269302368, 2.0}) {
    unsigned w = 0;
    unsigned h = 0;
    decodePng(tile_gen_->generateOverlayTile(0,
                                             0,
                                             0,
                                             {tile_gen_->getBounds()},
                                             {},
                                             {},
                                             {},
                                             nullptr,
                                             false,
                                             {},
                                             dpr,
                                             /*tile_px=*/0),
              w,
              h);
    EXPECT_EQ(w, static_cast<unsigned>(std::lround(kTileSize * dpr)))
        << "dpr " << dpr;
  }
}

// Heat-map tiles sit over the layer tiles like overlays do, and were the last
// path still rendering a flat 256 px whatever the display was doing.
TEST_F(TileGeneratorTest, HeatMapTileHonoursTheRequestedPixelCount)
{
  ASSERT_NO_FATAL_FAILURE(
      buildSeamDesign(odb::Rect(30000, 30000, 60000, 60000)));

  for (const DprCase& dpr_case : kDprCases) {
    const int expected_px = tilePxFor(dpr_case);
    unsigned w = 0;
    unsigned h = 0;
    const std::vector<unsigned char> rgba
        = decodePng(tile_gen_->generateHeatMapTile(
                        *heatmap_, 0, 0, 0, dpr_case.dpr, expected_px),
                    w,
                    h);
    EXPECT_EQ(w, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_EQ(h, static_cast<unsigned>(expected_px)) << dpr_case.what;
    EXPECT_TRUE(hasNonTransparentPixel(rgba))
        << dpr_case.what << ": the populated bin must be drawn at every size";
  }
}

// The bin lands on the same grid as the layers under it, at every size.
TEST_F(TileGeneratorTest, HeatMapTileRegistersWithTheLayerTileGrid)
{
  // A bin covering the middle third of the design, so its edges are interior
  // features whose pixel positions are predictable from the tile frame.
  ASSERT_NO_FATAL_FAILURE(
      buildSeamDesign(odb::Rect(30000, 30000, 60000, 60000)));
  const odb::Rect bounds = tile_gen_->getBounds();
  // A bin is 15 um = 30000 DBU (buildSeamDesign's setGridSizes) and at zoom 0
  // one tile spans the whole bounds, so the populated bin covers this fraction
  // of the tile however many pixels wide it is.
  constexpr double kBinDbu = 30000.0;
  const double bin_fraction = kBinDbu / bounds.maxDXDY();

  for (const DprCase& dpr_case : kDprCases) {
    const int dim = tilePxFor(dpr_case);
    unsigned w = 0;
    unsigned h = 0;
    const std::vector<unsigned char> rgba = decodePng(
        tile_gen_->generateHeatMapTile(*heatmap_, 0, 0, 0, dpr_case.dpr, dim),
        w,
        h);
    ASSERT_EQ(w, static_cast<unsigned>(dim)) << dpr_case.what;
    const int covered = coveredColumns(rgba, dim, dim / 2);
    const double expected = bin_fraction * dim;
    EXPECT_NEAR(covered, expected, 0.06 * dim)
        << dpr_case.what << ": bin covers " << covered << " of " << dim
        << " px, expected about " << expected;
  }
}

// Labels are authored in CSS px, so they have to scale with the display: a
// fixed 14 px label on a 3x tile is a third the size it should be.
TEST_F(TileGeneratorTest, HeatMapLabelsScaleWithTheDisplay)
{
  ASSERT_NO_FATAL_FAILURE(
      buildSeamDesign(odb::Rect(30000, 30000, 60000, 60000)));

  const auto labelPixels = [&](const double dpr, const int px) {
    unsigned w = 0;
    unsigned h = 0;
    heatmap_->setShowNumbers(true);
    const std::vector<unsigned char> on = decodePng(
        tile_gen_->generateHeatMapTile(*heatmap_, 0, 0, 0, dpr, px), w, h);
    heatmap_->setShowNumbers(false);
    const std::vector<unsigned char> off = decodePng(
        tile_gen_->generateHeatMapTile(*heatmap_, 0, 0, 0, dpr, px), w, h);
    // Pixels the label adds, whatever the fill under it is.
    return textPixels(on, off, Axis::kColumn).size();
  };

  const size_t at_1x = labelPixels(1.0, 256);
  const size_t at_2x = labelPixels(2.0, 512);
  ASSERT_GT(at_1x, 0u) << "the label must render at all";
  // Twice the pixels per CSS px in each direction, so the label spans about
  // twice the columns.  Loose bounds: glyph rasterization is not linear.
  EXPECT_GT(at_2x, at_1x * 3 / 2)
      << "label spanned " << at_2x << " columns at 2x vs " << at_1x
      << " at 1x -- it is not scaling with the display";
  EXPECT_LT(at_2x, at_1x * 3)
      << "label spanned " << at_2x << " columns at 2x vs " << at_1x << " at 1x";
}

TEST_F(TileGeneratorTest, HeatMapTileFallsBackToDprWhenUnspecified)
{
  ASSERT_NO_FATAL_FAILURE(
      buildSeamDesign(odb::Rect(30000, 30000, 60000, 60000)));
  for (const double dpr : {1.0, 1.25, 1.6666666269302368, 2.0}) {
    unsigned w = 0;
    unsigned h = 0;
    decodePng(tile_gen_->generateHeatMapTile(*heatmap_, 0, 0, 0, dpr, 0), w, h);
    EXPECT_EQ(w, static_cast<unsigned>(std::lround(kTileSize * dpr)))
        << "dpr " << dpr;
  }
}

TEST_F(TileGeneratorTest, EmptyDesignProducesTransparentTile)
{
  makeTileGen();

  // No instances or routing, so the tile should be transparent.
  auto png = tile_gen_->generateTile("metal1", 0, 0, 0);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels));
}

TEST_F(TileGeneratorTest, PlacedInstanceDrawsPixels)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  // Use the special "_instances" layer to draw instance borders.
  auto png = tile_gen_->generateTile("_instances", 0, 0, 0);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels));
}

TEST_F(TileGeneratorTest, StdcellVisibilityFilter)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  TileVisibility vis;
  vis.stdcells = false;

  auto png = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  // The _instances pass always draws the gray die/core outline (Qt
  // parity); with stdcells hidden nothing else may be visible.
  EXPECT_FALSE(hasNonOutlinePixel(pixels));
}

TEST_F(TileGeneratorTest, IsNetVisibleRespectsSignalType)
{
  odb::dbNet* sig_net = odb::dbNet::create(block_, "sig");
  sig_net->setSigType(odb::dbSigType::SIGNAL);

  odb::dbNet* pwr_net = odb::dbNet::create(block_, "vdd");
  pwr_net->setSigType(odb::dbSigType::POWER);

  odb::dbNet* clk_net = odb::dbNet::create(block_, "clk");
  clk_net->setSigType(odb::dbSigType::CLOCK);

  // Default visibility: all visible
  TileVisibility vis;
  EXPECT_TRUE(vis.isNetVisible(sig_net));
  EXPECT_TRUE(vis.isNetVisible(pwr_net));
  EXPECT_TRUE(vis.isNetVisible(clk_net));

  // Disable signal nets
  vis.net_signal = false;
  EXPECT_FALSE(vis.isNetVisible(sig_net));
  EXPECT_TRUE(vis.isNetVisible(pwr_net));

  // Disable power nets
  vis.net_power = false;
  EXPECT_FALSE(vis.isNetVisible(pwr_net));

  // Disable clock nets
  vis.net_clock = false;
  EXPECT_FALSE(vis.isNetVisible(clk_net));
}

TEST_F(TileGeneratorTest, TileVisibilityDefaultAllTrue)
{
  TileVisibility vis;
  EXPECT_TRUE(vis.stdcells);
  EXPECT_TRUE(vis.macros);
  EXPECT_TRUE(vis.routing);
  EXPECT_TRUE(vis.special_nets);
  EXPECT_TRUE(vis.pins);
  EXPECT_TRUE(vis.pin_markers);
  EXPECT_TRUE(vis.pin_names);
  EXPECT_TRUE(vis.inst_pins);
  EXPECT_TRUE(vis.inst_pin_names);
  EXPECT_TRUE(vis.blockages);
  EXPECT_TRUE(vis.net_signal);
  EXPECT_TRUE(vis.net_power);
  EXPECT_TRUE(vis.net_ground);
  EXPECT_TRUE(vis.net_clock);
  EXPECT_TRUE(vis.phys_fill);
  EXPECT_TRUE(vis.phys_endcap);
  EXPECT_FALSE(vis.has_visible_layers);
}

//------------------------------------------------------------------------------
// BTerm / ITerm pin visibility tests
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, BTermShapesGatedByPinsNotRouting)
{
  // BTerm shapes on tech layers should be controlled by vis.pins,
  // independently of vis.routing.
  makeBTermAtEdge("clk", "metal1", 0, 40000, 5000, 5000);
  makeTileGen();
  tile_gen_->eagerInit();

  // pins=true, routing=false → BTerm shapes should appear.
  TileVisibility vis_pins_on;
  vis_pins_on.stdcells = false;
  vis_pins_on.routing = false;
  vis_pins_on.special_nets = false;
  vis_pins_on.pins = true;
  auto png_on = tile_gen_->generateTile("metal1", 0, 0, 0, vis_pins_on);
  unsigned w = 0, h = 0;
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "BTerm shapes should appear when vis.pins is true";

  // pins=false, routing=false → no BTerm shapes.
  TileVisibility vis_pins_off;
  vis_pins_off.stdcells = false;
  vis_pins_off.routing = false;
  vis_pins_off.special_nets = false;
  vis_pins_off.pins = false;
  auto png_off = tile_gen_->generateTile("metal1", 0, 0, 0, vis_pins_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "BTerm shapes should be hidden when vis.pins is false";
}

TEST_F(TileGeneratorTest, VisibleLayersFiltersPinMarkers)
{
  // Pin markers on _pins layer should respect visible_layers filtering.
  makeBTermAtEdge("pin_m1", "metal1", 0, 40000, 200, 200);
  makeBTermAtEdge("pin_m3", "metal3", 0, 60000, 200, 200);
  makeTileGen();
  tile_gen_->eagerInit();

  // Default (no visible_layers) → both pins rendered.
  TileVisibility vis_default;
  vis_default.stdcells = false;
  auto png_default = tile_gen_->generateTile("_pins", 0, 0, 0, vis_default);
  unsigned w = 0, h = 0;
  auto pixels_default = decodePng(png_default, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_default))
      << "Pin markers should render with default visibility";

  // visible_layers = ["metal1"] → only metal1 pin rendered.
  TileVisibility vis_m1;
  vis_m1.stdcells = false;
  vis_m1.parseFromJson(
      parseObj(R"({"pins":true,"visible_layers":["metal1"]})"));
  auto png_m1 = tile_gen_->generateTile("_pins", 0, 0, 0, vis_m1);
  auto pixels_m1 = decodePng(png_m1, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_m1))
      << "metal1 pin should render when visible_layers includes metal1";
  EXPECT_NE(pixels_default, pixels_m1)
      << "Filtering to metal1 should differ from rendering both pins";

  // visible_layers = ["metal5"] → neither pin rendered.
  TileVisibility vis_m5;
  vis_m5.stdcells = false;
  vis_m5.parseFromJson(
      parseObj(R"({"pins":true,"visible_layers":["metal5"]})"));
  auto png_m5 = tile_gen_->generateTile("_pins", 0, 0, 0, vis_m5);
  auto pixels_m5 = decodePng(png_m5, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_m5))
      << "No pins should render when visible_layers has no matching layers";

  // visible_layers = [] (empty) → all layers hidden.
  TileVisibility vis_empty;
  vis_empty.stdcells = false;
  vis_empty.parseFromJson(parseObj(R"({"pins":true,"visible_layers":[]})"));
  auto png_empty = tile_gen_->generateTile("_pins", 0, 0, 0, vis_empty);
  auto pixels_empty = decodePng(png_empty, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_empty))
      << "Empty visible_layers should hide all pin markers";
}

TEST_F(TileGeneratorTest, PinMarkersRespectNetVisibility)
{
  // Pin markers on _pins layer should respect net type visibility.
  odb::dbNet* pwr_net = odb::dbNet::create(block_, "VDD");
  pwr_net->setSigType(odb::dbSigType::POWER);
  makeBTermOnNet("vdd_pin", pwr_net, "metal1", 0, 40000, 200, 200);

  odb::dbNet* sig_net = odb::dbNet::create(block_, "data");
  sig_net->setSigType(odb::dbSigType::SIGNAL);
  makeBTermOnNet("data_pin", sig_net, "metal1", 0, 60000, 200, 200);

  makeTileGen();
  tile_gen_->eagerInit();

  // Default: both visible.
  TileVisibility vis_all;
  vis_all.stdcells = false;
  auto png_all = tile_gen_->generateTile("_pins", 0, 0, 0, vis_all);
  unsigned w = 0, h = 0;
  auto pixels_all = decodePng(png_all, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_all));

  // Hide power nets → only signal pin.
  TileVisibility vis_no_pwr;
  vis_no_pwr.stdcells = false;
  vis_no_pwr.net_power = false;
  auto png_no_pwr = tile_gen_->generateTile("_pins", 0, 0, 0, vis_no_pwr);
  auto pixels_no_pwr = decodePng(png_no_pwr, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_no_pwr))
      << "Signal pin should still be visible";
  EXPECT_NE(pixels_all, pixels_no_pwr)
      << "Hiding power net should change the output";

  // Hide both power and signal → transparent.
  TileVisibility vis_none;
  vis_none.stdcells = false;
  vis_none.net_power = false;
  vis_none.net_signal = false;
  auto png_none = tile_gen_->generateTile("_pins", 0, 0, 0, vis_none);
  auto pixels_none = decodePng(png_none, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_none))
      << "Both net types hidden → no pin markers";
}

TEST_F(TileGeneratorTest, PinNamesGatesBTermLabels)
{
  // Use a tiny die so that pin markers are large enough for labels.
  // die_pin_size = max(0.02 * 100, 8) = 8; scale = 256/100 = 2.56;
  // 8 * 2.56 = 20.48 >= kMinPinNameSizePixels (20) → labels render.
  block_->setDieArea(odb::Rect(0, 0, 100, 100));
  makeBTermAtEdge("label_test_pin", "metal1", 0, 40, 10, 10);
  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis_names_on;
  vis_names_on.stdcells = false;
  vis_names_on.pin_names = true;
  auto png_on = tile_gen_->generateTile("_pins", 0, 0, 0, vis_names_on);

  TileVisibility vis_names_off;
  vis_names_off.stdcells = false;
  vis_names_off.pin_names = false;
  auto png_off = tile_gen_->generateTile("_pins", 0, 0, 0, vis_names_off);

  // The two should differ because labels are suppressed in the second.
  EXPECT_NE(png_on, png_off)
      << "pin_names=false should suppress BTerm name labels";
}

TEST_F(TileGeneratorTest, InstPinsGatesItermShapes)
{
  // ITerm (cell pin) shapes should be controlled by vis.inst_pins.
  // Use a small die so that cell pin geometry occupies visible pixels.
  block_->setDieArea(odb::Rect(0, 0, 2000, 2000));
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();  // build search R-trees for tech-layer rendering

  // inst_pins on, other shapes off → ITerm geometry visible on metal1.
  // stdcells must be true so isInstVisible() allows the instance through.
  TileVisibility vis_on;
  vis_on.routing = false;
  vis_on.special_nets = false;
  vis_on.pins = false;

  vis_on.blockages = false;
  vis_on.inst_pins = true;
  auto png_on = tile_gen_->generateTile("metal1", 0, 0, 0, vis_on);
  unsigned w = 0, h = 0;
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "ITerm shapes should appear when vis.inst_pins is true";

  // inst_pins off → no pin geometry, but instance still visible for other
  // sub-shapes.  With blockages also off, metal1 should be transparent.
  TileVisibility vis_off;
  vis_off.routing = false;
  vis_off.special_nets = false;
  vis_off.pins = false;

  vis_off.blockages = false;
  vis_off.inst_pins = false;
  auto png_off = tile_gen_->generateTile("metal1", 0, 0, 0, vis_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "ITerm shapes should be hidden when vis.inst_pins is false";
}

TEST_F(TileGeneratorTest, InstPinNamesRendered)
{
  // Use a small die so cell pin geometry fills enough pixels for labels.
  block_->setDieArea(odb::Rect(0, 0, 2000, 2000));
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis_on;
  vis_on.routing = false;
  vis_on.special_nets = false;
  vis_on.pins = false;

  vis_on.blockages = false;
  vis_on.inst_pins = true;
  vis_on.inst_pin_names = true;
  auto png_on = tile_gen_->generateTile("metal1", 0, 0, 0, vis_on);

  TileVisibility vis_off;
  vis_off.routing = false;
  vis_off.special_nets = false;
  vis_off.pins = false;

  vis_off.blockages = false;
  vis_off.inst_pins = true;
  vis_off.inst_pin_names = false;
  auto png_off = tile_gen_->generateTile("metal1", 0, 0, 0, vis_off);

  // Labels should make the two outputs differ.
  EXPECT_NE(png_on, png_off)
      << "inst_pin_names should add ITerm labels to tile output";

  // With inst_pins=false, labels should not appear even if inst_pin_names=true.
  TileVisibility vis_no_pins;
  vis_no_pins.routing = false;
  vis_no_pins.special_nets = false;
  vis_no_pins.pins = false;

  vis_no_pins.blockages = false;
  vis_no_pins.inst_pins = false;
  vis_no_pins.inst_pin_names = true;
  auto png_no_pins = tile_gen_->generateTile("metal1", 0, 0, 0, vis_no_pins);
  unsigned w = 0, h = 0;
  auto pixels_no_pins = decodePng(png_no_pins, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_no_pins))
      << "ITerm labels should not render when inst_pins is false";
}

//------------------------------------------------------------------------------
// Access-point overlay tests (_access_points pseudo-layer)
//------------------------------------------------------------------------------

// One table-driven test for the overlay visibility flags: default value,
// explicit set, and omitted-key fallback (same table shape as kFields).
TEST_F(TileGeneratorTest, OverlayFlagsParsedFromJson)
{
  struct FlagCase
  {
    const char* key;
    bool TileVisibility::*field;
    bool default_val;
  };
  const FlagCase cases[] = {
      {"access_points", &TileVisibility::access_points, false},
      {"regions", &TileVisibility::regions, true},
      {"mfg_grid", &TileVisibility::mfg_grid, false},
      {"gcell_grid", &TileVisibility::gcell_grid, false},
  };
  for (const auto& c : cases) {
    TileVisibility vis_default;
    EXPECT_EQ(vis_default.*c.field, c.default_val) << c.key;

    // Explicitly set to the opposite of the default.
    TileVisibility vis_set;
    const std::string json = std::string("{\"") + c.key
                             + "\":" + (c.default_val ? "false" : "true") + "}";
    vis_set.parseFromJson(parseObj(json));
    EXPECT_EQ(vis_set.*c.field, !c.default_val) << c.key;

    // Omitting the key falls back to the default.
    TileVisibility vis_omitted;
    vis_omitted.parseFromJson(parseObj(R"({"pins":true})"));
    EXPECT_EQ(vis_omitted.*c.field, c.default_val) << c.key;
  }
}

TEST_F(TileGeneratorTest, AccessPointsOverlayGatedByFlag)
{
  // Small die so the fixed 100-DBU marker is well above the sub-pixel LOD.
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));

  ASSERT_NE(makeMetal1AccessPoint(), nullptr);

  makeTileGen();
  tile_gen_->eagerInit();

  unsigned w = 0, h = 0;

  // access_points=true → marker rendered.
  TileVisibility vis_on;
  vis_on.stdcells = false;
  vis_on.access_points = true;
  auto png_on = tile_gen_->generateTile("_access_points", 0, 0, 0, vis_on);
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "Access-point marker should render when vis.access_points is true";

  // access_points=false → nothing on the pseudo-layer.
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.access_points = false;
  auto png_off = tile_gen_->generateTile("_access_points", 0, 0, 0, vis_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "Access points should be hidden when vis.access_points is false";
}

TEST_F(TileGeneratorTest, AccessPointsRespectLayerVisibility)
{
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));

  ASSERT_NE(makeMetal1AccessPoint(), nullptr);

  makeTileGen();
  tile_gen_->eagerInit();

  unsigned w = 0, h = 0;

  // visible_layers = ["metal1"] → the metal1 access point renders.
  TileVisibility vis_m1;
  vis_m1.stdcells = false;
  vis_m1.parseFromJson(
      parseObj(R"({"access_points":true,"visible_layers":["metal1"]})"));
  auto png_m1 = tile_gen_->generateTile("_access_points", 0, 0, 0, vis_m1);
  auto pixels_m1 = decodePng(png_m1, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_m1))
      << "AP on metal1 should render when metal1 is visible";

  // visible_layers = ["metal5"] → the metal1 access point is hidden.
  TileVisibility vis_m5;
  vis_m5.stdcells = false;
  vis_m5.parseFromJson(
      parseObj(R"({"access_points":true,"visible_layers":["metal5"]})"));
  auto png_m5 = tile_gen_->generateTile("_access_points", 0, 0, 0, vis_m5);
  auto pixels_m5 = decodePng(png_m5, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_m5))
      << "AP on metal1 should be hidden when only metal5 is visible";
}

//------------------------------------------------------------------------------
// Region overlay tests (_regions pseudo-layer)
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, RegionsOverlayGatedByFlag)
{
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));
  // Anchor the block bbox (getBounds uses content, not the die area).
  placeInst("BUF_X16", "buf1", 0, 0);

  odb::dbRegion* region = odb::dbRegion::create(block_, "test_dom");
  ASSERT_NE(region, nullptr);
  odb::dbBox::create(region, 1000, 1000, 3000, 3000);

  makeTileGen();
  tile_gen_->eagerInit();

  unsigned w = 0, h = 0;

  // regions=true → boundary rendered on the _regions pseudo-layer.
  TileVisibility vis_on;
  vis_on.stdcells = false;
  vis_on.regions = true;
  auto png_on = tile_gen_->generateTile("_regions", 0, 0, 0, vis_on);
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "Region boundary should render when vis.regions is true";

  // regions=false → nothing on the pseudo-layer.
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.regions = false;
  auto png_off = tile_gen_->generateTile("_regions", 0, 0, 0, vis_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "Regions should be hidden when vis.regions is false";
}

TEST_F(TileGeneratorTest, RegionsSkipZeroAreaBoundaries)
{
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));
  placeInst("BUF_X16", "buf1", 0, 0);

  // Degenerate boundary (zero width) must be skipped (GUI parity:
  // drawRegions only draws boundaries with area() > 0).
  odb::dbRegion* region = odb::dbRegion::create(block_, "empty_dom");
  ASSERT_NE(region, nullptr);
  odb::dbBox::create(region, 2000, 1000, 2000, 3000);

  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;
  vis.regions = true;
  auto png = tile_gen_->generateTile("_regions", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels))
      << "Zero-area region boundaries should not be drawn";
}

//------------------------------------------------------------------------------
// Manufacturing-grid overlay tests (_mfg_grid pseudo-layer)
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, MfgGridGatedByFlagAndLod)
{
  // Nangate45 fixture LEF has MANUFACTURINGGRID 0.0050 (= 10 DBU).
  ASSERT_TRUE(getDb()->getTech()->hasManufacturingGrid());
  placeInst("BUF_X16", "buf1", 0, 0);  // anchor block bbox
  makeTileGen();
  tile_gen_->eagerInit();

  unsigned w = 0, h = 0;

  // Deep zoom (z=5): grid spacing >= 5 px → dots rendered.
  TileVisibility vis_on;
  vis_on.stdcells = false;
  vis_on.mfg_grid = true;
  auto png_on = tile_gen_->generateTile("_mfg_grid", 5, 0, 0, vis_on);
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "Grid dots should render at deep zoom when vis.mfg_grid is true";

  // Same zoom, flag off → transparent.
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.mfg_grid = false;
  auto png_off = tile_gen_->generateTile("_mfg_grid", 5, 0, 0, vis_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "Grid dots should be hidden when vis.mfg_grid is false";
}

// Mirrors kMinViewablePx in tile_generator.cpp (the on-screen spacing the
// decimation keeps between grid dots).
constexpr int kMinViewablePxForTest = 5;

// The marker must survive tile seams.  Culling access points on their CENTRE
// made the neighbouring tile skip the marker entirely, so the X was chopped
// along every seam (reported on the PR #10806 review).
//
// The access point sits JUST INSIDE the left tile, close enough to the seam
// that the right leg of its X reaches into the right tile.  The right tile does
// not contain the centre, so with the old centre-based cull it came back empty
// — which is exactly the truncated X from the report.  (Placing the point
// exactly on the seam would not test anything: Rect::intersects is inclusive on
// edges, so every neighbouring tile would "contain" it and draw.)
TEST_F(TileGeneratorTest, AccessPointXCompleteAcrossTileSeams)
{
  constexpr int kExtent = 4000;  // anchors the block bbox → z=1 seam at 2000
  constexpr int kApX = 1990;     // 10 DBU left of the seam; marker reach is 50
  constexpr int kApY = 1000;
  block_->setDieArea(odb::Rect(0, 0, kExtent, kExtent));
  odb::dbMaster* m = lib_->findMaster("INV_X1");
  ASSERT_NE(m, nullptr);
  placeInst("INV_X1", "anchor_ll", 0, 0);
  placeInst("INV_X1",
            "anchor_ur",
            kExtent - static_cast<int>(m->getWidth()),
            kExtent - static_cast<int>(m->getHeight()));

  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbNet* net = odb::dbNet::create(block_, "seam_pin");
  odb::dbBTerm* bterm = odb::dbBTerm::create(net, "seam_pin");
  bterm->setIoType(odb::dbIoType::INPUT);
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  odb::dbBox::create(bpin, metal1, kApX - 20, kApY - 20, kApX + 20, kApY + 20);
  bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  odb::dbAccessPoint* ap = odb::dbAccessPoint::create(bpin);
  ASSERT_NE(ap, nullptr);
  ap->setPoint(odb::Point(kApX, kApY));
  ap->setLayer(metal1);
  ap->setAccess(true, odb::dbDirection::EAST);

  makeTileGen();
  tile_gen_->eagerInit();
  const odb::Rect b = tile_gen_->getBounds();
  const int seam = (b.xMin() + b.xMax()) / 2;
  ASSERT_GT(kApX, seam - 50)
      << "access point must be within marker reach of the "
         "seam for this test to mean anything";
  ASSERT_LT(kApX, seam) << "access point must sit inside the LEFT tile";

  TileVisibility vis;
  vis.stdcells = false;
  vis.access_points = true;

  auto green_px = [&](int tx) {
    unsigned w = 0;
    unsigned h = 0;
    auto px = decodePng(
        tile_gen_->generateTile("_access_points", 1, tx, 1, vis), w, h);
    int n = 0;
    for (size_t i = 0; i + 3 < px.size(); i += 4) {
      if (px[i + 3] > 0 && px[i] == 0 && px[i + 1] == 255 && px[i + 2] == 0) {
        ++n;
      }
    }
    return n;
  };
  EXPECT_GT(green_px(0), 0)
      << "left tile (owns the centre) must draw the marker";
  EXPECT_GT(green_px(1), 0)
      << "right tile drew nothing: the leg crossing the seam is being dropped, "
         "so the X renders chopped";
}

// Below the legibility limit the overlay DECIMATES instead of hiding (this
// replaces the old Qt-parity behaviour of showing nothing: a manufacturing grid
// is so much finer than a die that the Qt rule made the overlay unreachable in
// practice — see the PR #10806 review).  What is drawn there is a subgrid.
TEST_F(TileGeneratorTest, MfgGridDecimatesBelowLodInsteadOfHiding)
{
  ASSERT_TRUE(getDb()->getTech()->hasManufacturingGrid());
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  // z=0: whole design in one tile, so the raw 10 DBU grid is far below one
  // pixel — the old code returned an empty tile here.
  TileVisibility vis;
  vis.stdcells = false;
  vis.mfg_grid = true;
  unsigned w = 0;
  unsigned h = 0;
  auto pixels
      = decodePng(tile_gen_->generateTile("_mfg_grid", 0, 0, 0, vis), w, h);
  ASSERT_TRUE(hasNonTransparentPixel(pixels))
      << "the grid must stay reachable at zoom-out via decimation";

  // And it must be a readable lattice, not a smear: consecutive dot columns
  // have to sit at least ~kMinViewablePx apart.
  const int iw = static_cast<int>(w);
  std::set<int> cols;
  for (int y = 0; y < static_cast<int>(h); ++y) {
    for (int x = 0; x < iw; ++x) {
      if (pixels[(static_cast<size_t>(y) * iw + x) * 4 + 3] > 0) {
        cols.insert(x);
      }
    }
  }
  ASSERT_GE(cols.size(), 2u) << "expected several dot columns";
  // Measure the PERIOD between dots (distance between the starts of runs of
  // contiguous lit columns), not the gap between lit columns: each dot is
  // itself a couple of pixels wide, so the gap understates the spacing.
  std::vector<int> run_starts;
  int prev = -2;
  for (const int c : cols) {
    if (c != prev + 1) {
      run_starts.push_back(c);
    }
    prev = c;
  }
  ASSERT_GE(run_starts.size(), 2u) << "expected at least two dot columns";
  int min_period = iw;
  for (size_t i = 1; i < run_starts.size(); ++i) {
    min_period = std::min(min_period, run_starts[i] - run_starts[i - 1]);
  }
  EXPECT_GE(min_period, static_cast<int>(kMinViewablePxForTest))
      << "dot period is " << min_period
      << " px — that is a smear, not a readable grid";
}

// "Detailed view" tightens the decimation target from kMinViewablePx (5 px) to
// kDetailedGridPx (4 px), so the lattice gets denser and closer to the real
// manufacturing grid — mirroring what the toggle already does to shapes.  It
// deliberately stops short of a 1 px target: that lights every pixel of the
// tile, and the raw grid's loop is O(points in tile), unbounded at zoom-out.
TEST_F(TileGeneratorTest, MfgGridDenserUnderDetailedView)
{
  ASSERT_TRUE(getDb()->getTech()->hasManufacturingGrid());
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  auto dots = [&](bool detailed) {
    TileVisibility vis;
    vis.stdcells = false;
    vis.mfg_grid = true;
    vis.detailed = detailed;
    unsigned w = 0;
    unsigned h = 0;
    auto px
        = decodePng(tile_gen_->generateTile("_mfg_grid", 0, 0, 0, vis), w, h);
    return static_cast<int>(countNonTransparentPixels(px));
  };
  const int off = dots(false);
  const int on = dots(true);
  ASSERT_GT(off, 0) << "baseline grid must be visible via decimation";
  EXPECT_GT(on, off) << "detailed view must draw a denser lattice (" << on
                     << " vs " << off << " dots)";
  // Denser must still be a lattice: with a tighter target the dots merge into a
  // solid sheet that hides the design, so cap the coverage well below full.
  const int tile_px = kTileSize * kTileSize;
  EXPECT_LT(on, tile_px / 2)
      << "detailed grid covers " << on << " of " << tile_px
      << " pixels — that is a solid sheet, not a grid";
}

// The decimation step must depend only on the zoom, never on the tile, or
// neighbouring tiles would land on different lattices and the seam would jump.
TEST_F(TileGeneratorTest, MfgGridLatticeIsSeamlessAcrossTiles)
{
  ASSERT_TRUE(getDb()->getTech()->hasManufacturingGrid());
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;
  vis.mfg_grid = true;

  // Two horizontally adjacent tiles at the same zoom.  Their dot rows must
  // coincide: same absolute lattice, so the same y positions light up.
  unsigned w = 0;
  unsigned h = 0;
  auto left
      = decodePng(tile_gen_->generateTile("_mfg_grid", 1, 0, 0, vis), w, h);
  auto right
      = decodePng(tile_gen_->generateTile("_mfg_grid", 1, 1, 0, vis), w, h);
  const int iw = static_cast<int>(w);
  auto rows = [&](const std::vector<unsigned char>& px) {
    std::set<int> r;
    for (int y = 0; y < static_cast<int>(h); ++y) {
      for (int x = 0; x < iw; ++x) {
        if (px[(static_cast<size_t>(y) * iw + x) * 4 + 3] > 0) {
          r.insert(y);
          break;
        }
      }
    }
    return r;
  };
  const std::set<int> lr = rows(left);
  const std::set<int> rr = rows(right);
  ASSERT_FALSE(lr.empty());
  ASSERT_FALSE(rr.empty());
  EXPECT_EQ(lr, rr)
      << "dot rows differ between adjacent tiles — the lattice is "
         "tile-dependent and the seam will visibly jump";
}

//------------------------------------------------------------------------------
// Die / core outline tests (_instances pass, always on — Qt parity)
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, DieAndCoreOutlinesOnInstancesLayer)
{
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));
  block_->setCoreArea(odb::Rect(500, 500, 3500, 3500));
  placeInst("BUF_X16", "buf1", 0, 0);  // anchor block bbox
  makeTileGen();
  tile_gen_->eagerInit();

  // Everything hidden — only the die/core outlines may remain.
  TileVisibility vis;
  vis.stdcells = false;
  auto png = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);

  EXPECT_TRUE(hasNonTransparentPixel(pixels))
      << "Die/core outlines should be drawn on the _instances pass";
  EXPECT_FALSE(hasNonOutlinePixel(pixels))
      << "Only the gray outline color may be visible";

  // Two nested frames -> some row crosses 4 vertical outline pixels
  // (die left/right + core left/right).  Find a row with >= 4 gray pixels.
  // Alpha varies with the decimation coverage, so only the RGB is matched.
  int max_gray_in_row = 0;
  for (unsigned yy = 0; yy < h; ++yy) {
    int gray = 0;
    for (unsigned xx = 0; xx < w; ++xx) {
      const size_t i = 4UL * (yy * w + xx);
      if (pixels[i] == 128 && pixels[i + 1] == 128 && pixels[i + 2] == 128
          && pixels[i + 3] > 0) {
        ++gray;
      }
    }
    max_gray_in_row = std::max(max_gray_in_row, gray);
  }
  EXPECT_GE(max_gray_in_row, 4)
      << "Expected die + core vertical edges crossing the same row";
}

TEST_F(TileGeneratorTest, NoOutlineOnTechLayerTiles)
{
  // Guard against the regression that motivated the original multi-die-only
  // gating: tech-layer tiles must stay transparent (no gray frame).
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));
  block_->setCoreArea(odb::Rect(500, 500, 3500, 3500));
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;
  vis.routing = false;
  vis.special_nets = false;
  vis.pins = false;
  vis.inst_pins = false;
  vis.blockages = false;
  auto png = tile_gen_->generateTile("metal1", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels))
      << "Tech-layer tiles must not carry the die/core outline";
}

//------------------------------------------------------------------------------
// Track rendering: the tracks must stop at the die area, as in the Qt GUI
// (RenderThread::drawTracks clips to block->getDieArea()).
//------------------------------------------------------------------------------

constexpr int kTrackDieSide = 40000;  // die: (0,0)-(40000,40000)
constexpr int kTrackPitch = 2000;     // 21 tracks per axis across the die

// Visibility that draws the tracks and nothing else, so any lit pixel in the
// assertions below is a track.
TileVisibility trackOnlyVisibility()
{
  TileVisibility vis;
  vis.stdcells = false;
  vis.routing = false;
  vis.special_nets = false;
  vis.pins = false;
  vis.inst_pins = false;
  vis.blockages = false;
  vis.tracks_pref = true;
  vis.tracks_non_pref = true;
  return vis;
}

TEST_F(TileGeneratorTest, TracksAreClippedToTheDieArea)
{
  block_->setDieArea(odb::Rect(0, 0, kTrackDieSide, kTrackDieSide));
  // getBounds() follows the block bbox, and dbBlock::getBBox() covers the
  // SHAPES, not the die area: an instance at the origin anchors the viewport
  // to the die, and a second one beyond the die stretches the bbox past it.
  // That gap outside the die is where the tracks used to run on, drawn to the
  // tile edge instead of stopping at the die boundary.
  placeInst("BUF_X16", "inside", 0, 0);
  placeInst("BUF_X16", "outside", kTrackDieSide + 20000, kTrackDieSide + 20000);

  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, metal1);
  grid->addGridPatternX(0, kTrackDieSide / kTrackPitch + 1, kTrackPitch);
  grid->addGridPatternY(0, kTrackDieSide / kTrackPitch + 1, kTrackPitch);

  makeTileGen();
  tile_gen_->eagerInit();

  ASSERT_NE(block_->findTrackGrid(metal1), nullptr)
      << "precondition: the track grid must be reachable from the block";

  auto png = tile_gen_->generateTile("metal1", 0, 0, 0, trackOnlyVisibility());
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  ASSERT_GT(w, 0u);

  // Map pixels back to DBU exactly as the renderer does at z=0: one tile
  // spanning getBounds().maxDXDY(), Y flipped.
  const odb::Rect bounds = tile_gen_->getBounds();
  const double dbu_per_px = static_cast<double>(bounds.maxDXDY()) / w;
  ASSERT_GT(dbu_per_px, 0.0);
  // Three pixels of slack.  The tile is rasterized supersampled and then
  // Lanczos-2 decimated, and that filter spreads a hairline about two output
  // pixels either way, so a track sitting on the die edge tints just past it.
  // The defect this guards against is nothing like that: it drew tracks to the
  // tile edge, tens of pixels beyond the die.
  const double slack = 3 * dbu_per_px;

  size_t inside = 0;
  size_t outside = 0;
  // First offender only: enough to point at the failure, and cheaper than
  // tracking the whole bounding box of the strays.
  double stray_x = 0;
  double stray_y = 0;
  for (unsigned py = 0; py < h; ++py) {
    for (unsigned px = 0; px < w; ++px) {
      if (pixels[4UL * (py * w + px) + 3] == 0) {
        continue;
      }
      const double dbu_x = bounds.xMin() + px * dbu_per_px;
      const double dbu_y = bounds.yMin() + (h - 1 - py) * dbu_per_px;
      const bool in_die = dbu_x >= -slack && dbu_x <= kTrackDieSide + slack
                          && dbu_y >= -slack && dbu_y <= kTrackDieSide + slack;
      if (in_die) {
        ++inside;
      } else if (outside++ == 0) {
        stray_x = dbu_x;
        stray_y = dbu_y;
      }
    }
  }

  EXPECT_EQ(outside, 0u) << "tracks must stop at the die area (die side "
                         << kTrackDieSide << ", slack " << slack
                         << " dbu; first stray pixel at " << stray_x << ","
                         << stray_y << "; inside=" << inside << ")";
  EXPECT_GT(inside, 0u) << "the tracks inside the die must still be drawn";
}

TEST_F(TileGeneratorTest, TracksSpanTheWholeTileWhenTheDieCoversIt)
{
  // The common case — every design in the flow has die == bbox — must be
  // untouched by the clip: the tracks still run edge to edge.
  // Anchor the viewport to the die corners (the bbox covers shapes, not the
  // die area).
  placeInst("BUF_X16", "ll", 0, 0);
  placeInst("BUF_X16", "ur", 90000, 90000);

  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, metal1);
  // The fixture's die, set in SetUp(); cover it entirely.
  constexpr int kFixtureDieSide = 100000;
  grid->addGridPatternX(0, kFixtureDieSide / kTrackPitch + 1, kTrackPitch);
  grid->addGridPatternY(0, kFixtureDieSide / kTrackPitch + 1, kTrackPitch);

  makeTileGen();
  tile_gen_->eagerInit();

  auto png = tile_gen_->generateTile("metal1", 0, 0, 0, trackOnlyVisibility());
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  ASSERT_GT(w, 0u);

  // Rows reuse the fixture's coveredColumns(); columns have no equivalent.
  const auto row_has_pixel
      = [&](unsigned py) { return coveredColumns(pixels, w, py) > 0; };
  const auto col_has_pixel = [&](unsigned px) {
    for (unsigned py = 0; py < h; ++py) {
      if (pixels[4UL * (py * w + px) + 3] > 0) {
        return true;
      }
    }
    return false;
  };

  // getBounds() adds a symmetric pin-label margin, so the die does not reach
  // the tile edge; sample just inside each die border instead of at pixel 0.
  const odb::Rect bounds = tile_gen_->getBounds();
  const double dbu_per_px = static_cast<double>(bounds.maxDXDY()) / w;
  const auto col_of = [&](int dbu) {
    const double px = (dbu - bounds.xMin()) / dbu_per_px;
    return static_cast<unsigned>(std::clamp(px, 0.0, w - 1.0));
  };
  const auto row_of = [&](int dbu) {
    const double py = (h - 1) - (dbu - bounds.yMin()) / dbu_per_px;
    return static_cast<unsigned>(std::clamp(py, 0.0, h - 1.0));
  };

  EXPECT_TRUE(row_has_pixel(row_of(2000)) && row_has_pixel(row_of(98000)))
      << "horizontal tracks must still reach both ends of the die";
  EXPECT_TRUE(col_has_pixel(col_of(2000)) && col_has_pixel(col_of(98000)))
      << "vertical tracks must still reach both ends of the die";
}

//------------------------------------------------------------------------------
// GCell-grid overlay tests (_gcell_grid pseudo-layer)
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, GcellGridGatedByFlag)
{
  placeInst("BUF_X16", "buf1", 0, 0);  // anchor block bbox

  // Create a GCell grid the same way grt does (absolute-DBU patterns).
  odb::dbGCellGrid* grid = odb::dbGCellGrid::create(block_);
  ASSERT_NE(grid, nullptr);
  grid->addGridPatternX(0, 5, 1000);
  grid->addGridPatternY(0, 5, 1000);

  makeTileGen();
  tile_gen_->eagerInit();

  unsigned w = 0, h = 0;

  // No LOD: grid lines render even at z=0 (unlike the mfg grid).
  TileVisibility vis_on;
  vis_on.stdcells = false;
  vis_on.gcell_grid = true;
  auto png_on = tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis_on);
  auto pixels_on = decodePng(png_on, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "GCell grid lines should render when vis.gcell_grid is true";

  // Flag off → transparent.
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.gcell_grid = false;
  auto png_off = tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis_off);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "GCell grid should be hidden when vis.gcell_grid is false";
}

TEST_F(TileGeneratorTest, GcellGridClosedAtDieBoundary)
{
  // The dbGCellGrid stores only the gcell START edges, so the top/right
  // die edges have no grid line.  The web renderer must close the mesh at
  // the die boundary (the Qt GUI gets this from its separate die outline).
  block_->setDieArea(odb::Rect(0, 0, 4000, 4000));
  placeInst("BUF_X16", "buf1", 0, 0);  // bbox ~7000 DBU wide, die inside tile

  odb::dbGCellGrid* grid = odb::dbGCellGrid::create(block_);
  ASSERT_NE(grid, nullptr);
  // Single interior line per axis at 2000 — far from the die top/right.
  grid->addGridPatternX(2000, 1, 1);
  grid->addGridPatternY(2000, 1, 1);

  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;
  vis.gcell_grid = true;
  auto png = tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);

  // Count rows containing a long horizontal run of white pixels: expect 3
  // (die bottom edge, interior line at y=2000, die top edge).  Vertical
  // lines only contribute isolated pixels per row, so a >=20px run filter
  // isolates the horizontal lines.
  // A single 1-CSS-px line lands on more than one output row: the tile is
  // rasterized supersampled and Lanczos-decimated, which spreads each line
  // over ~3 rows with partial alpha.  Count contiguous BANDS of such rows,
  // not the rows themselves.
  int bands = 0;
  bool in_band = false;
  for (unsigned yy = 0; yy < h; ++yy) {
    int run = 0, best = 0;
    for (unsigned xx = 0; xx < w; ++xx) {
      const size_t i = 4UL * (yy * w + xx);
      // Alpha varies with the decimation coverage; match the RGB only.
      const bool white = pixels[i] == 255 && pixels[i + 1] == 255
                         && pixels[i + 2] == 255 && pixels[i + 3] > 0;
      run = white ? run + 1 : 0;
      best = std::max(best, run);
    }
    const bool row_has_line = best >= 20;
    if (row_has_line && !in_band) {
      ++bands;
    }
    in_band = row_has_line;
  }
  EXPECT_EQ(bands, 3)
      << "Expected bottom edge + interior line + top edge horizontal lines";
}

TEST_F(TileGeneratorTest, GcellGridAbsentWithoutGrid)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  // No dbGCellGrid created (pre-global-route design) → nothing to draw.
  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;
  vis.gcell_grid = true;
  auto png = tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels))
      << "GCell grid layer should be empty when the block has no grid";
}

// The per-block gcell cache is only correct as long as the grid it copied is
// unchanged, so it must be dropped on a design change, not only on a design
// reload.  Rerouting a live session changes the grid (global_route replaces the
// patterns), and before the fix the overlay kept drawing the OLD lattice until
// the page was reloaded (reported on the PR #10806 review).
TEST_F(TileGeneratorTest, GcellGridCacheInvalidatedOnDesignChange)
{
  constexpr int kExtent = 10000;
  block_->setDieArea(odb::Rect(0, 0, kExtent, kExtent));
  placeInst("BUF_X16", "buf1", 0, 0);

  // A coarse grid: 2 lines per axis.
  odb::dbGCellGrid* grid = odb::dbGCellGrid::create(block_);
  ASSERT_NE(grid, nullptr);
  grid->addGridPatternX(0, 2, 5000);
  grid->addGridPatternY(0, 2, 5000);

  makeTileGen();
  tile_gen_->eagerInit();  // also installs the design-changed hook

  TileVisibility vis;
  vis.stdcells = false;
  vis.gcell_grid = true;

  // First render copies the coarse grid into the cache.
  unsigned w = 0, h = 0;
  auto pixels_before
      = decodePng(tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis), w, h);
  const size_t lit_before = countNonTransparentPixels(pixels_before);
  ASSERT_GT(lit_before, 0u) << "the coarse grid should be drawn";

  // Reroute: a much finer grid, plus the routing whose odb callback is what
  // announces the design change (Search::inDbSWireCreate -> clearShapes ->
  // on_modified).  Dropping the PNG cache alone is not enough here — the
  // overlay cache still holds the coarse lattice.
  grid->addGridPatternX(0, 40, 250);
  grid->addGridPatternY(0, 40, 250);

  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbNet* net = odb::dbNet::create(block_, "grt_special");
  net->setSpecial();
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  ASSERT_NE(swire, nullptr);
  odb::dbSBox::create(
      swire, metal1, 0, 0, 1000, 100, odb::dbWireShapeType::NONE);

  auto pixels_after
      = decodePng(tile_gen_->generateTile("_gcell_grid", 0, 0, 0, vis), w, h);
  EXPECT_GT(countNonTransparentPixels(pixels_after), lit_before)
      << "the re-created, denser grid must replace the cached one";
}

// toPxX/toPxY saturate each axis on its own, so feeding them a segment whose
// endpoints are far outside the tile used to CHANGE ITS SLOPE (only one axis
// clipped) instead of shortening it.  Flight lines therefore have to go through
// the double conversion + drawLineF (reported on the PR #10806 review).
TEST_F(TileGeneratorTest, FlywireSlopePreservedAtExtremeZoom)
{
  constexpr int kSpan = 100000;
  block_->setDieArea(odb::Rect(0, 0, kSpan, kSpan));
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();
  tile_gen_->eagerInit();

  // Pick an exact DBU point on the bounds' diagonal, then ask for the tile that
  // contains it — aiming at a fixed tile index instead would not work, because
  // beyond z~17 a tile is narrower than one DBU and an odb::Point cannot be
  // placed inside a chosen one.
  //
  // The segment must be SHALLOW, not diagonal: saturating both axes at the same
  // +/-1e7 turns any segment into a 45-degree one, so a 45-degree input is the
  // single slope the old conversion happened to get right.  Here dy is an
  // eighth of dx, and both endpoints are far enough out (~10^8 px at z=15) that
  // both axes used to saturate.
  constexpr int kZoom = 15;
  constexpr int kSlopeDivisor = 8;
  const odb::Rect bounds = tile_gen_->getBounds();
  const int num_tiles = 1 << kZoom;
  const double tile_dbu = bounds.maxDXDY() / static_cast<double>(num_tiles);
  // Offsets are relative to the bounds, which follow the block BBox (and so the
  // instance above), not the die area.
  const int diag_offset = bounds.maxDXDY() / 4;
  const int far = 100 * bounds.maxDXDY();
  const odb::Point on_diagonal(bounds.xMin() + diag_offset,
                               bounds.yMin() + diag_offset);
  // Same index on both axes, so the tile origin is on the bounds' diagonal too.
  const int tile_idx = static_cast<int>(diag_offset / tile_dbu);
  ASSERT_LT(tile_idx, num_tiles);
  const std::vector<FlightLine> lines
      = {FlightLine{.p1 = odb::Point(on_diagonal.x() - far,
                                     on_diagonal.y() - far / kSlopeDivisor),
                    .p2 = odb::Point(on_diagonal.x() + far,
                                     on_diagonal.y() + far / kSlopeDivisor),
                    .color = Color{.r = 255, .g = 255, .b = 0, .a = 255}}};

  // Leaflet y counts from the top, hence the flip on the y index only.
  auto png = tile_gen_->generateOverlayTile(kZoom,
                                            tile_idx,
                                            num_tiles - 1 - tile_idx,
                                            /*highlight_rects=*/{},
                                            /*highlight_polys=*/{},
                                            /*colored_rects=*/{},
                                            lines);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  ASSERT_GT(w, 0u);

  // The segment crosses the whole tile, so it must span the full width while
  // rising only about 1/kSlopeDivisor of it.  A rotated (45-degree) segment
  // would span both dimensions equally.
  int min_x = static_cast<int>(w);
  int max_x = -1;
  int min_y = static_cast<int>(h);
  int max_y = -1;
  for (unsigned yy = 0; yy < h; ++yy) {
    for (unsigned xx = 0; xx < w; ++xx) {
      if (pixels[4UL * (yy * w + xx) + 3] == 0) {
        continue;
      }
      min_x = std::min(min_x, static_cast<int>(xx));
      max_x = std::max(max_x, static_cast<int>(xx));
      min_y = std::min(min_y, static_cast<int>(yy));
      max_y = std::max(max_y, static_cast<int>(yy));
    }
  }
  ASSERT_GE(max_x, 0) << "the flywire must cross the requested tile";
  const int x_span = max_x - min_x + 1;
  const int y_span = max_y - min_y + 1;
  EXPECT_GT(x_span, static_cast<int>(w) / 2)
      << "the segment should run across the tile";
  // Half-way between the true ratio (1/8) and the rotated one (1/1).
  EXPECT_LT(y_span * 4, x_span)
      << "the segment's slope must survive the DBU->pixel conversion";
}

TEST_F(TileGeneratorTest, InvalidLayerProducesValidPng)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  auto png = tile_gen_->generateTile("nonexistent_layer", 0, 0, 0);
  ASSERT_FALSE(png.empty());

  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 256u);
  EXPECT_EQ(h, 256u);
}

TEST_F(TileGeneratorTest, OutOfBoundsTileIsTransparent)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  // At zoom=1 valid tiles are (0,0),(0,1),(1,0),(1,1).  Tile (5,5) is out.
  auto png = tile_gen_->generateTile("_instances", 1, 5, 5);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels));
}

TEST_F(TileGeneratorTest, DebugModeDrawsBorder)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  TileVisibility vis;
  vis.debug = true;

  auto png = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  ASSERT_EQ(w, 256u);
  ASSERT_EQ(h, 256u);

  // Check corners for yellow border pixels (R=255, G=255, B=0, A=255).
  // Pixel at (0,0):
  EXPECT_EQ(pixels[0], 255);  // R
  EXPECT_EQ(pixels[1], 255);  // G
  EXPECT_EQ(pixels[2], 0);    // B
  EXPECT_EQ(pixels[3], 255);  // A

  // Pixel at (255,255):
  const int last = (255 * 256 + 255) * 4;
  EXPECT_EQ(pixels[last + 0], 255);  // R
  EXPECT_EQ(pixels[last + 1], 255);  // G
  EXPECT_EQ(pixels[last + 2], 0);    // B
  EXPECT_EQ(pixels[last + 3], 255);  // A
}

TEST_F(TileGeneratorTest, DebugBorderTracesFullHiDpiTile)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  TileVisibility vis;
  vis.debug = true;

  // dpr=2 → a 512px tile.  The border must trace the 512px edges: drawing it
  // at a hardcoded 256 boxed the outline into the top-left quadrant, so the
  // debug "tile" was only 1/dpr of the tile it claimed to outline.
  auto png = tile_gen_->generateTile("_instances",
                                     0,
                                     0,
                                     0,
                                     vis,
                                     /*highlight_rects=*/{},
                                     /*highlight_polys=*/{},
                                     /*colored_rects=*/{},
                                     /*flight_lines=*/{},
                                     /*module_colors=*/nullptr,
                                     /*focus_net_ids=*/nullptr,
                                     /*route_guide_net_ids=*/nullptr,
                                     /*dpr=*/2.0);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  ASSERT_EQ(w, 512u);
  ASSERT_EQ(h, 512u);

  const auto is_yellow = [&pixels, w](const unsigned px, const unsigned py) {
    const size_t i = (static_cast<size_t>(py) * w + px) * 4;
    return pixels[i] == 255 && pixels[i + 1] == 255 && pixels[i + 2] == 0
           && pixels[i + 3] == 255;
  };

  // All four true corners of the 512px tile.
  EXPECT_TRUE(is_yellow(0, 0));
  EXPECT_TRUE(is_yellow(w - 1, 0));
  EXPECT_TRUE(is_yellow(0, h - 1));
  EXPECT_TRUE(is_yellow(w - 1, h - 1));

  // Midpoints of the right and bottom edges, which the 256px border missed
  // entirely.
  EXPECT_TRUE(is_yellow(w - 1, h / 2));
  EXPECT_TRUE(is_yellow(w / 2, h - 1));
}

TEST_F(TileGeneratorTest, DebugDefaultOff)
{
  TileVisibility vis;
  EXPECT_FALSE(vis.debug);
}

//------------------------------------------------------------------------------
// Focus net filtering tests
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, FocusNetEmptySetSameAsNull)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  // Empty focus_net_ids should behave the same as nullptr (all nets visible).
  std::set<uint32_t> empty_set;
  auto png = tile_gen_->generateTile(
      "metal1", 0, 0, 0, {}, {}, {}, {}, {}, nullptr, &empty_set);
  ASSERT_FALSE(png.empty());

  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 256u);
  EXPECT_EQ(h, 256u);
}

TEST_F(TileGeneratorTest, FocusNetNonMatchingIdProducesValidTile)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  // Focus on a net ID that doesn't correspond to any routing.
  // Should produce a valid tile (instances still drawn, just net shapes
  // filtered).
  std::set<uint32_t> focus_ids{99999};
  auto png = tile_gen_->generateTile(
      "metal1", 0, 0, 0, {}, {}, {}, {}, {}, nullptr, &focus_ids);
  ASSERT_FALSE(png.empty());

  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 256u);
  EXPECT_EQ(h, 256u);
}

TEST_F(TileGeneratorTest, FocusNetWithRealNetId)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  odb::dbNet* net = odb::dbNet::create(block_, "focus_test_net");
  makeTileGen();

  // Focus on the created net's ID.  Even without routing shapes,
  // the tile should be generated without errors.
  std::set<uint32_t> focus_ids{net->getId()};
  auto png = tile_gen_->generateTile(
      "metal1", 0, 0, 0, {}, {}, {}, {}, {}, nullptr, &focus_ids);
  ASSERT_FALSE(png.empty());

  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 256u);
  EXPECT_EQ(h, 256u);
}

TEST_F(TileGeneratorTest, FocusNetNullPtrAllowsAllNets)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  // nullptr means no focus filtering — should match default behavior.
  auto png_default = tile_gen_->generateTile("metal1", 0, 0, 0);
  auto png_null = tile_gen_->generateTile(
      "metal1", 0, 0, 0, {}, {}, {}, {}, {}, nullptr, nullptr);
  EXPECT_EQ(png_default, png_null);
}

TEST_F(TileGeneratorTest, SemiTransparentOverlayUsesStraightAlpha)
{
  placeInst("BUF_X16", "buf0", 0, 0);
  placeInst("BUF_X16", "buf1", 90000, 90000);
  makeTileGen();

  const odb::Rect rect(0, 0, 100000, 100000);
  auto png
      = tile_gen_->generateTile("nonexistent_layer", 0, 0, 0, {}, {rect}, {});

  unsigned w = 0;
  unsigned h = 0;
  const auto pixels = decodePng(png, w, h);
  ASSERT_EQ(w, 256u);
  ASSERT_EQ(h, 256u);

  const int center = (128 * 256 + 128) * 4;
  EXPECT_EQ(pixels[center + 0], 255);
  EXPECT_EQ(pixels[center + 1], 255);
  EXPECT_EQ(pixels[center + 2], 0);
  EXPECT_EQ(pixels[center + 3], 30);
}

//------------------------------------------------------------------------------
// Via enclosure rendering tests
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, SpecialNetViaEnclosureDrawnOnMetalLayer)
{
  // Create a power net with a special wire containing a tech via.
  // via1_0 has boxes on via1 (cut), metal1 (enclosure), metal2 (enclosure).
  // The search index stores vias under the cut layer.  The renderer must
  // look up adjacent cut layers when rendering a metal layer to find and
  // draw the enclosure boxes.

  odb::dbTech* tech = getDb()->getTech();
  odb::dbTechVia* via_def = tech->findVia("via1_0");
  ASSERT_NE(via_def, nullptr);
  odb::dbTechLayer* m1 = tech->findLayer("metal1");
  ASSERT_NE(m1, nullptr);

  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);
  odb::dbSWire* swire = odb::dbSWire::create(pwr, odb::dbWireType::ROUTED);

  // Add a metal1 power strap that defines a small bounding box (1000 dbu)
  // so the via enclosure (280 dbu) occupies many pixels at zoom 0.
  odb::dbSBox::create(
      swire, m1, 0, 0, 1000, 1000, odb::dbWireShapeType::STRIPE);
  odb::dbSBox* sbox = odb::dbSBox::create(
      swire, via_def, 500, 500, odb::dbWireShapeType::IOWIRE);
  ASSERT_NE(sbox, nullptr);

  makeTileGen();
  tile_gen_->eagerInit();

  TileVisibility vis;
  vis.stdcells = false;

  // Sanity check: the cut layer itself should have pixels (existing code).
  auto png_cut = tile_gen_->generateTile("via1", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels_cut = decodePng(png_cut, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_cut))
      << "Via cut should be drawn on via1 (sanity check)";

  // Render metal1 with special_nets enabled — should see the enclosure.
  auto png = tile_gen_->generateTile("metal1", 0, 0, 0, vis);
  auto pixels = decodePng(png, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels))
      << "Via enclosure should be drawn on metal1";

  // Also check metal2 enclosure.
  auto png_m2 = tile_gen_->generateTile("metal2", 0, 0, 0, vis);
  auto pixels_m2 = decodePng(png_m2, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_m2))
      << "Via enclosure should be drawn on metal2";

  // Disable special_nets — tile should be transparent.
  vis.special_nets = false;
  auto png_off = tile_gen_->generateTile("metal1", 0, 0, 0, vis);
  auto pixels_off = decodePng(png_off, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels_off))
      << "Via enclosure should be hidden when special_nets is off";
}

//------------------------------------------------------------------------------
// Row and site rendering tests
//------------------------------------------------------------------------------

// Helper to create a row with the Nangate45 site.
class RowRenderingTest : public TileGeneratorTest
{
 protected:
  void SetUp() override
  {
    TileGeneratorTest::SetUp();
    site_ = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site_, nullptr);
    // Site is 380 x 2800 DBU (0.19 x 1.4 um at 2000 DBU/um).
    // Create a row with 100 sites starting at origin.
    row_ = odb::dbRow::create(block_,
                              "row0",
                              site_,
                              0,
                              0,
                              odb::dbOrientType::R0,
                              odb::dbRowDir::HORIZONTAL,
                              100,
                              site_->getWidth());
    ASSERT_NE(row_, nullptr);
  }

  odb::dbSite* site_ = nullptr;
  odb::dbRow* row_ = nullptr;
};

TEST_F(RowRenderingTest, RowOutlineDrawnWhenVisible)
{
  makeTileGen();

  TileVisibility vis;
  vis.rows = true;
  vis.stdcells = false;
  // Enable site visibility via raw JSON.
  vis.parseFromJson(parseObj(
      R"({"rows":true,"stdcells":false,"site_FreePDK45_38x28_10R_NP_162NW_34O":true})"));

  auto png = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels))
      << "Row outline should be drawn when rows are visible";
}

TEST_F(RowRenderingTest, RowHiddenWhenSiteNotVisible)
{
  makeTileGen();

  TileVisibility vis;
  vis.rows = true;
  vis.stdcells = false;
  // Rows enabled but this specific site is not visible.
  vis.parseFromJson(parseObj(R"({"rows":true,"stdcells":false})"));

  auto png = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w = 0, h = 0;
  auto pixels = decodePng(png, w, h);
  // Ignore the always-on gray die/core outline (Qt parity).
  EXPECT_FALSE(hasNonOutlinePixel(pixels))
      << "Row should be hidden when its site is not in the visibility list";
}

TEST_F(RowRenderingTest, IndividualSitesDrawnWhenZoomedIn)
{
  makeTileGen();

  TileVisibility vis;
  vis.parseFromJson(parseObj(
      R"({"rows":true,"stdcells":false,"site_FreePDK45_38x28_10R_NP_162NW_34O":true})"));

  // At zoom 0, tile covers the full design. Site is 380 DBU wide.
  // site_px = 380 * (256 / ~104000) ≈ 0.9 → no individual sites.
  auto png_z0 = tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  unsigned w0 = 0, h0 = 0;
  auto pixels_z0 = decodePng(png_z0, w0, h0);
  EXPECT_TRUE(hasNonTransparentPixel(pixels_z0))
      << "Row outline should be visible at zoom 0";

  // At a high zoom, site_px should exceed the 5px threshold and
  // individual sites should be drawn.  Use renderTileBuffer to scan
  // for the tile that contains our row at y=[0, 2800].
  const int zoom = 8;  // 256 tiles, ~400 DBU per tile → site_px ≈ 240
  const int num_tiles = 1 << zoom;
  const odb::Rect bounds = tile_gen_->getBounds();
  const double tile_dbu = static_cast<double>(bounds.maxDXDY()) / num_tiles;

  // Find the tile column/row containing the row origin (0,0).
  const int tx = static_cast<int>((0 - bounds.xMin()) / tile_dbu);
  // Leaflet y is flipped: dbu_y_index = num_tiles - 1 - leaflet_y.
  const int dbu_y_idx = static_cast<int>((0 - bounds.yMin()) / tile_dbu);
  const int ly = num_tiles - 1 - dbu_y_idx;

  ASSERT_GE(tx, 0);
  ASSERT_LT(tx, num_tiles);
  ASSERT_GE(ly, 0);
  ASSERT_LT(ly, num_tiles);

  auto png_hi = tile_gen_->generateTile("_instances", zoom, tx, ly, vis);
  unsigned wh = 0, hh = 0;
  auto pixels_hi = decodePng(png_hi, wh, hh);

  int count_hi = 0;
  for (size_t i = 3; i < pixels_hi.size(); i += 4) {
    if (pixels_hi[i] > 0) {
      ++count_hi;
    }
  }
  EXPECT_GT(count_hi, 0)
      << "Zoomed-in tile at row origin should have site outlines";
}

TEST_F(RowRenderingTest, RowsDefaultOff)
{
  TileVisibility vis;
  EXPECT_FALSE(vis.rows);
}

//------------------------------------------------------------------------------
// serializeTechResponse — exercises the contract main.js relies on for the
// document title (techData.block_name).
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Selectability — parallel column added to the display panel, mirroring the
// Qt GUI's selectability column.  Picks (selectAt) require both visible AND
// selectable, but rendering ignores the selectability flags.
//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, SelectableDefaultAllTrue)
{
  TileVisibility vis;
  EXPECT_TRUE(vis.stdcells_selectable);
  EXPECT_TRUE(vis.macros_selectable);
  EXPECT_TRUE(vis.net_signal_selectable);
  EXPECT_TRUE(vis.net_power_selectable);
  EXPECT_TRUE(vis.net_clock_selectable);
  EXPECT_TRUE(vis.pins_selectable);
  EXPECT_TRUE(vis.inst_pins_selectable);
  EXPECT_TRUE(vis.placement_blockages_selectable);
  EXPECT_TRUE(vis.routing_obstructions_selectable);
  EXPECT_FALSE(vis.has_selectable_layers);
}

TEST_F(TileGeneratorTest, ParseFromJsonReadsSelectableKeys)
{
  TileVisibility vis;
  vis.parseFromJson(
      parseObj(R"({"s_stdcells":false,"s_macros":true,"s_net_signal":false,)"
               R"("s_pins":false,"s_inst_pins":false,)"
               R"("selectable_layers":["metal1","metal2"]})"));
  EXPECT_FALSE(vis.stdcells_selectable);
  EXPECT_TRUE(vis.macros_selectable);
  EXPECT_FALSE(vis.net_signal_selectable);
  EXPECT_FALSE(vis.pins_selectable);
  EXPECT_FALSE(vis.inst_pins_selectable);
  EXPECT_TRUE(vis.has_selectable_layers);
  EXPECT_TRUE(vis.isLayerSelectable("metal1"));
  EXPECT_TRUE(vis.isLayerSelectable("metal2"));
  EXPECT_FALSE(vis.isLayerSelectable("metal3"));
}

TEST_F(TileGeneratorTest, IsNetSelectableRespectsSignalType)
{
  odb::dbNet* sig_net = odb::dbNet::create(block_, "sig");
  sig_net->setSigType(odb::dbSigType::SIGNAL);
  odb::dbNet* pwr_net = odb::dbNet::create(block_, "vdd");
  pwr_net->setSigType(odb::dbSigType::POWER);

  TileVisibility vis;
  EXPECT_TRUE(vis.isNetSelectable(sig_net));
  EXPECT_TRUE(vis.isNetSelectable(pwr_net));

  vis.net_signal_selectable = false;
  EXPECT_FALSE(vis.isNetSelectable(sig_net));
  EXPECT_TRUE(vis.isNetSelectable(pwr_net));
}

TEST_F(TileGeneratorTest, IsLayerSelectableDefaultsTrueWhenUnspecified)
{
  TileVisibility vis;
  // No selectable_layers list ⇒ every layer is selectable.
  EXPECT_TRUE(vis.isLayerSelectable("metal1"));
  EXPECT_TRUE(vis.isLayerSelectable("anything"));
}

TEST_F(TileGeneratorTest, SelectAtGatesInstancesBySelectability)
{
  odb::dbInst* inst = placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  const odb::Rect bbox = inst->getBBox()->getBox();
  const int cx = (bbox.xMin() + bbox.xMax()) / 2;
  const int cy = (bbox.yMin() + bbox.yMax()) / 2;

  // Default visibility + selectability ⇒ the inst is picked.
  TileVisibility vis;
  auto results = tile_gen_->selectAt(cx, cy, /*zoom=*/0, vis);
  EXPECT_EQ(results.size(), 1u);

  // Visible but not selectable ⇒ no pick.
  TileVisibility vis_no_sel;
  vis_no_sel.stdcells_selectable = false;
  auto results_no_sel = tile_gen_->selectAt(cx, cy, /*zoom=*/0, vis_no_sel);
  EXPECT_EQ(results_no_sel.size(), 0u);

  // Confirm the path-through-parseFromJson works too.
  TileVisibility vis_json;
  vis_json.parseFromJson(parseObj(R"({"s_stdcells":false})"));
  auto results_json = tile_gen_->selectAt(cx, cy, /*zoom=*/0, vis_json);
  EXPECT_EQ(results_json.size(), 0u);
}

TEST_F(TileGeneratorTest, SelectAtGatesInstancesByLayerSelectability)
{
  // Layer selectability does NOT gate instance picks (insts aren't on a
  // layer) — only routing-shape picks.  Confirm an inst still picks when
  // the selectable_layers list is non-empty but doesn't list anything.
  odb::dbInst* inst = placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  const odb::Rect bbox = inst->getBBox()->getBox();
  const int cx = (bbox.xMin() + bbox.xMax()) / 2;
  const int cy = (bbox.yMin() + bbox.yMax()) / 2;

  TileVisibility vis;
  vis.parseFromJson(parseObj(R"({"selectable_layers":[]})"));
  EXPECT_TRUE(vis.has_selectable_layers);
  auto results = tile_gen_->selectAt(cx, cy, /*zoom=*/0, vis);
  EXPECT_EQ(results.size(), 1u);
}

//------------------------------------------------------------------------------
// Renderer::select — a click can hit an object a renderer owns.  Qt walks the
// layers in reverse, visible AND selectable only, then makes one pass with a
// null layer (LayoutViewer::selectAt).
//------------------------------------------------------------------------------

// Records the layers it is asked about and can claim the click.
struct RendererSelectRecorder
{
  std::vector<std::string> asked;   // "" for the layer-independent pass
  bool claim_on_null_pass = false;  // return an object on that pass

  void install()
  {
    TileGenerator::setRendererHooks(
        {.select = [this](odb::dbTechLayer* layer,
                          const odb::Rect& region,
                          std::vector<SelectionResult>& out) {
          asked.emplace_back(layer != nullptr ? layer->getName() : "");
          if (layer == nullptr && claim_on_null_pass) {
            out.push_back({std::any{},
                           "renderer-object",
                           "GCell",
                           region,
                           odb::dbTransform(),
                           /*is_inst=*/false});
          }
        }});
  }

  static void clear() { TileGenerator::setRendererHooks({}); }
};

TEST_F(TileGeneratorTest, RendererSelectAsksEveryLayerThenTheNullPassLast)
{
  placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  RendererSelectRecorder recorder;
  recorder.install();
  TileVisibility vis;
  tile_gen_->selectAt(20000, 20000, /*zoom=*/0, vis);
  RendererSelectRecorder::clear();

  ASSERT_FALSE(recorder.asked.empty());
  // psm::DebugGui::select clears its state on the null pass, so it has to be
  // the last thing asked.
  EXPECT_EQ(recorder.asked.back(), "")
      << "the layer-independent pass must come after every layer";
  EXPECT_EQ(std::ranges::count(recorder.asked, std::string()), 1)
      << "and it must happen exactly once";
  // Reverse layer order, as in Qt: metal2 is asked before metal1.
  const auto m1 = std::ranges::find(recorder.asked, std::string("metal1"));
  const auto m2 = std::ranges::find(recorder.asked, std::string("metal2"));
  ASSERT_NE(m1, recorder.asked.end());
  ASSERT_NE(m2, recorder.asked.end());
  EXPECT_LT(m2 - recorder.asked.begin(), m1 - recorder.asked.begin());
}

TEST_F(TileGeneratorTest, RendererSelectSkipsHiddenAndUnselectableLayers)
{
  placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  // Only metal1 visible.
  RendererSelectRecorder hidden;
  hidden.install();
  TileVisibility vis;
  tile_gen_->selectAt(20000, 20000, /*zoom=*/0, vis, {"metal1"});
  RendererSelectRecorder::clear();
  EXPECT_EQ(hidden.asked, (std::vector<std::string>{"metal1", ""}));

  // Visible but not selectable ⇒ not asked at all.
  RendererSelectRecorder unselectable;
  unselectable.install();
  TileVisibility vis_no_sel;
  vis_no_sel.parseFromJson(parseObj(R"({"selectable_layers":[]})"));
  tile_gen_->selectAt(20000, 20000, /*zoom=*/0, vis_no_sel, {"metal1"});
  RendererSelectRecorder::clear();
  EXPECT_EQ(unselectable.asked, (std::vector<std::string>{""}))
      << "only the layer-independent pass survives";
}

// Qt pushes renderer hits before searching the design, so a renderer's object
// wins the click.  Here the sort that promotes instances must not bury it.
TEST_F(TileGeneratorTest, RendererSelectResultsComeBeforeDesignObjects)
{
  odb::dbInst* inst = placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  const odb::Rect bbox = inst->getBBox()->getBox();
  const int cx = (bbox.xMin() + bbox.xMax()) / 2;
  const int cy = (bbox.yMin() + bbox.yMax()) / 2;

  RendererSelectRecorder recorder;
  recorder.claim_on_null_pass = true;
  recorder.install();
  TileVisibility vis;
  auto results = tile_gen_->selectAt(cx, cy, /*zoom=*/0, vis);
  RendererSelectRecorder::clear();

  ASSERT_GE(results.size(), 2u) << "the instance and the renderer object";
  EXPECT_EQ(results.front().name, "renderer-object");
  EXPECT_EQ(results.front().type_name, "GCell");
  EXPECT_TRUE(std::ranges::any_of(results, [](const SelectionResult& r) {
    return r.is_inst;
  })) << "the instance is still picked, just behind";
}

// With no callback installed nothing changes: the plain openroad binary with
// no web server never installs one.
TEST_F(TileGeneratorTest, RendererSelectIsANoOpWithoutACallback)
{
  odb::dbInst* inst = placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  tile_gen_->eagerInit();

  const odb::Rect bbox = inst->getBBox()->getBox();
  TileVisibility vis;
  auto results = tile_gen_->selectAt((bbox.xMin() + bbox.xMax()) / 2,
                                     (bbox.yMin() + bbox.yMax()) / 2,
                                     /*zoom=*/0,
                                     vis);
  EXPECT_EQ(results.size(), 1u);
}

//------------------------------------------------------------------------------

TEST_F(TileGeneratorTest, SerializeTechResponseContainsBlockName)
{
  // Nangate45Fixture creates the block with name "top".
  makeTileGen();
  const std::string json
      = boost::json::serialize(serializeTechResponse(*tile_gen_));
  // Field name and value should both appear.  Looser than a full JSON
  // parse but sufficient: this is the contract main.js consumes.
  EXPECT_NE(json.find("\"block_name\""), std::string::npos)
      << "tech response missing block_name key; got: " << json;
  EXPECT_NE(json.find("\"top\""), std::string::npos)
      << "tech response missing block name value \"top\"; got: " << json;
}

TEST_F(TileGeneratorTest, LayerHierarchyBacksideCategory)
{
  odb::dbTech* tech = getDb()->getTech();

  // Mark metal1 and via1 as backside.
  tech->findLayer("metal1")->setBackside(true);
  tech->findLayer("via1")->setBackside(true);

  makeTileGen();
  const auto resp = serializeTechResponse(*tile_gen_);
  ASSERT_TRUE(resp.contains("layer_hierarchy"));
  const auto& hier = resp.at("layer_hierarchy").as_object();

  // Top-level layers should NOT contain the backside layers.
  const auto& top_layers = hier.at("layers").as_array();
  for (const auto& l : top_layers) {
    const auto& name = l.as_object().at("name").as_string();
    EXPECT_NE(name, "metal1") << "backside metal1 should not be at top level";
    EXPECT_NE(name, "via1") << "backside via1 should not be at top level";
  }

  // A "Backside" category node should exist in instances.
  const auto& instances = hier.at("instances").as_array();
  const boost::json::object* backside_node = nullptr;
  for (const auto& inst : instances) {
    const auto& obj = inst.as_object();
    if (obj.at("name").as_string() == "Backside") {
      backside_node = &obj;
      break;
    }
  }
  ASSERT_NE(backside_node, nullptr)
      << "layer_hierarchy missing Backside category node";
  EXPECT_EQ(backside_node->at("type").as_string(), "category");

  // The backside node should contain exactly metal1 and via1.
  const auto& bs_layers = backside_node->at("layers").as_array();
  std::set<std::string> bs_names;
  for (const auto& l : bs_layers) {
    bs_names.insert(std::string(l.as_object().at("name").as_string()));
  }
  EXPECT_EQ(bs_names, (std::set<std::string>{"metal1", "via1"}));
}

TEST_F(TileGeneratorTest, LayerHierarchyNoBacksideCategory)
{
  // No layers marked backside — there should be no Backside category.
  makeTileGen();
  const auto resp = serializeTechResponse(*tile_gen_);
  const auto& hier = resp.at("layer_hierarchy").as_object();
  const auto& instances = hier.at("instances").as_array();
  for (const auto& inst : instances) {
    EXPECT_NE(inst.as_object().at("name").as_string(), "Backside")
        << "Backside category should not appear when no layers are backside";
  }
}

// ─── Anti-moiré band-limit (issue #10463) ────────────────────────────────

// Build a dense periodic array of small cells whose OUTPUT pitch lands in the
// sub-pixel regime that aliases into a moiré beat without band-limiting.  N
// cells per row over the die => output pitch ~ 256/N px at z=0.
class MoireArrayTest : public TileGeneratorTest
{
 protected:
  // Returns the cell pitch in DBU.
  int buildArray(int n)
  {
    odb::dbMaster* m = lib_->findMaster("INV_X1");
    EXPECT_NE(m, nullptr);
    const int pitch = 2 * std::max(m->getWidth(), m->getHeight());
    const int die = n * pitch;
    block_->setDieArea(odb::Rect(0, 0, die, die));
    int id = 0;
    for (int iy = 0; iy < n; ++iy) {
      for (int ix = 0; ix < n; ++ix) {
        odb::dbInst* inst = odb::dbInst::create(
            block_, m, ("d" + std::to_string(id++)).c_str());
        inst->setLocation(ix * pitch, iy * pitch);
        inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      }
    }
    return pitch;
  }

  // Build an n x n bump array (master tagged COVER_BUMP) sized so each bump
  // renders ~target_px CSS px at z=0 (where bounds ~= die, so output size =
  // cell*256/die).  Used to land bump sizes inside the LOD crossfade band.
  void buildBumpArrayTargetPx(int n, double target_px)
  {
    odb::dbMaster* m = lib_->findMaster("INV_X1");
    EXPECT_NE(m, nullptr);
    m->setType(odb::dbMasterType::COVER_BUMP);
    const int cell = std::max(m->getWidth(), m->getHeight());
    const int die = static_cast<int>(cell * 256.0 / target_px);
    const int pitch = die / n;  // output pitch = 256/n px; > cell ⇒ gaps
    block_->setDieArea(odb::Rect(0, 0, die, die));
    int id = 0;
    for (int iy = 0; iy < n; ++iy) {
      for (int ix = 0; ix < n; ++ix) {
        odb::dbInst* inst = odb::dbInst::create(
            block_, m, ("b" + std::to_string(id++)).c_str());
        inst->setLocation(ix * pitch, iy * pitch);
        inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      }
    }
  }
};

TEST_F(MoireArrayTest, DenseArraySubPixelHasNoBeat)
{
  buildArray(/*n=*/128);  // output pitch ~2 px — the regime that aliases
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  auto pixels = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
  EXPECT_EQ(w, 256u);
  const int iw = static_cast<int>(w);
  const int ih = static_cast<int>(h);
  // Measure the central macro-uniform window: the full-tile profile is
  // dominated by the array's outer edge / surrounding margin (a legitimate
  // low-frequency envelope, not a beat).  In the interior the supersample +
  // Lanczos-2 decimation must keep the beat band nearly empty — round-8 (1px
  // coverage) measured ~0.2-0.3 here; the fix drives it to <0.01.
  const double beat
      = beatFracWindow(pixels, iw, iw / 4, ih / 4, 3 * iw / 4, 3 * ih / 4);
  EXPECT_LT(beat, 0.06) << "moiré beat present in dense sub-pixel bump array";
}

TEST_F(MoireArrayTest, DenseBumpArrayOffGridPitchHasNoBeat)
{
  // Property guard: a kPhysBump array whose super-pixel pitch (512/n at z=0)
  // is off an integer (n=126 → 4.063, etc.) must stay beat-free.  NOTE: this
  // synthetic (INV_X1-as-bump) does NOT reproduce the strong beat seen on
  // real designs — that needed large near-pitch footprints whose floor/ceil
  // rounding closed the sub-pixel gaps (→ sheet) and jittered ±1 px (→ beat).
  // The AUTHORITATIVE regression check for this fix was a visual A/B on the
  // real multi_tech_stack.3dbx (RODADA 18): RODADA-17 rendered SUB_M2 as
  // solid blue sheets; exact-area coverage renders faithful discrete dots / a
  // faint tint with no beat.  Keep this as a cheap lower-bound guard; the
  // real gate stays visual (see plan).  Exact area coverage integrates each
  // pixel independent of sub-pixel phase → no jitter → no beat for any
  // off-grid pitch.
  odb::dbMaster* m = lib_->findMaster("INV_X1");
  ASSERT_NE(m, nullptr);
  m->setType(odb::dbMasterType::COVER_BUMP);

  for (const int n : {126, 127, 130}) {  // super-pitch 4.063 / 4.031 / 3.938
    std::vector<odb::dbInst*> existing;
    for (odb::dbInst* inst : block_->getInsts()) {
      existing.push_back(inst);
    }
    for (odb::dbInst* inst : existing) {
      odb::dbInst::destroy(inst);
    }
    buildArray(n);
    makeTileGen();
    unsigned w = 0;
    unsigned h = 0;
    auto pixels
        = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
    const int iw = static_cast<int>(w);
    const int ih = static_cast<int>(h);
    const double beat
        = beatFracWindow(pixels, iw, iw / 4, ih / 4, 3 * iw / 4, 3 * ih / 4);
    EXPECT_LT(beat, 0.06) << "moiré beat at off-grid bump array n=" << n;
  }
}

TEST_F(MoireArrayTest, ResolvedArrayStaysSharp)
{
  // Same array, but viewed zoomed-in (z=3) so the pitch resolves to ~16 px.
  // Band-limiting must NOT smear it into a flat tint: structure (high block
  // CV) survives while the beat band stays empty.
  buildArray(/*n=*/128);
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  // Central tile at z=3 (8x8 tiles); guaranteed to sit inside the array.
  auto pixels = decodePng(tile_gen_->generateTile("_instances", 3, 4, 4), w, h);
  // The resolved grid's fundamental (~16 px pitch) legitimately lives in the
  // beat band, so beatFrac is NOT a valid check here — the point is only that
  // the structure survived (high block-CV), i.e. it wasn't smeared to a tint.
  EXPECT_GT(blockAlphaCV(pixels, w, h, 8), 0.10)
      << "resolved grid was over-blurred into a flat tint";
}

TEST_F(MoireArrayTest, BumpArrayBelowThresholdIsCulled)
{
  // Mark the small master as a bump so classifyInstance() returns kPhysBump
  // (the fixture has no STA, so it falls back to the COVER_BUMP master type).
  odb::dbMaster* m = lib_->findMaster("INV_X1");
  ASSERT_NE(m, nullptr);
  m->setType(odb::dbMasterType::COVER_BUMP);

  buildArray(
      /*n=*/128);  // bumps render ~1 px at z=0 → below the cull threshold
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  auto pixels = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
  const int iw = static_cast<int>(w);
  const int ih = static_cast<int>(h);

  // Sub-resolution geometry (a dense bump array whose cells render ~1 px) is
  // culled at the RTree level by searchInsts(size_limit_dbu), matching the Qt
  // GUI: below the viewable threshold it is dropped entirely rather than drawn
  // as a faint coverage tint or a merged opaque sheet.  So the central
  // interior stays fully transparent — no tint, no sheet, no beat.  The
  // above-threshold and resolved-zoom regimes are guarded by
  // BandRendersDiscreteBumpsNotSlab / ResolvedArrayStaysSharp.
  const int x0 = iw / 4;
  const int x1 = 3 * iw / 4;
  const int y0 = ih / 4;
  const int y1 = 3 * ih / 4;
  double alpha_sum = 0.0;
  int n_px = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      alpha_sum += pixels[(static_cast<size_t>(y) * iw + x) * 4 + 3];
      ++n_px;
    }
  }
  const double mean_alpha = alpha_sum / n_px;
  EXPECT_EQ(mean_alpha, 0.0)
      << "sub-resolution bump array was not culled (Qt parity: it must vanish "
         "at zoom-out, not render a coverage tint or an opaque sheet)";
}

TEST_F(MoireArrayTest, DetailedViewRendersSubResolutionInstances)
{
  // With "Detailed view" on, the sub-resolution cull is relaxed
  // (instance_size_limit_dbu == 0, mirroring the Qt GUI's instanceSizeLimit()
  // in detailed view), so the same dense bump array that vanishes at zoom-out
  // by default is drawn instead.  Off by default so the moiré fix is
  // unchanged in the normal view.
  odb::dbMaster* m = lib_->findMaster("INV_X1");
  ASSERT_NE(m, nullptr);
  m->setType(odb::dbMasterType::COVER_BUMP);

  buildArray(
      /*n=*/128);  // bumps render ~1 px at z=0 → below the cull threshold
  makeTileGen();

  TileVisibility vis;
  vis.detailed = true;

  unsigned w = 0;
  unsigned h = 0;
  auto pixels
      = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0, vis), w, h);
  const int iw = static_cast<int>(w);
  const int ih = static_cast<int>(h);

  const int x0 = iw / 4;
  const int x1 = 3 * iw / 4;
  const int y0 = ih / 4;
  const int y1 = 3 * ih / 4;
  double alpha_sum = 0.0;
  int n_px = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      alpha_sum += pixels[(static_cast<size_t>(y) * iw + x) * 4 + 3];
      ++n_px;
    }
  }
  const double mean_alpha = alpha_sum / n_px;
  EXPECT_GT(mean_alpha, 0.0)
      << "detailed view must render sub-resolution instances that the default "
         "view culls (Qt parity: instanceSizeLimit() == 0)";
}

TEST_F(MoireArrayTest, BandRendersDiscreteBumpsNotSlab)
{
  // A bump that renders just below the LOD threshold (~6 px) is drawn as a
  // single discrete coverage mark at its real footprint — NOT a slab covering
  // the inter-bump gaps.  So the interior shows solid bumps separated by
  // transparent gaps: moderate coverage (well under a slab's ~full fill) with
  // some near-opaque bump pixels present.
  buildBumpArrayTargetPx(/*n=*/16, /*target_px=*/6.0);
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  auto pixels = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
  const int iw = static_cast<int>(w);
  const int ih = static_cast<int>(h);
  int nonzero = 0;
  int total = 0;
  int max_alpha = 0;
  for (int y = ih / 4; y < 3 * ih / 4; ++y) {
    for (int x = iw / 4; x < 3 * iw / 4; ++x) {
      ++total;
      const int a = pixels[(static_cast<size_t>(y) * iw + x) * 4 + 3];
      if (a > 0) {
        ++nonzero;
      }
      max_alpha = std::max(max_alpha, a);
    }
  }
  const double coverage = static_cast<double>(nonzero) / total;
  // ~6 px bumps on a ~16 px pitch fill ~14% of the area: discrete, with gaps.
  EXPECT_GT(coverage, 0.03) << "bumps were not drawn (empty interior)";
  EXPECT_LT(coverage, 0.5) << "interior was slabbed over the gaps (merged "
                              "sheet, not discrete bumps)";
  EXPECT_GT(max_alpha, 100)
      << "bumps are not drawn solid (expected discrete near-opaque marks)";
}

TEST_F(MoireArrayTest, BumpArrayBelowThresholdCulledUniformlyAcrossTileSeam)
{
  // The sub-resolution cull must apply uniformly across tile boundaries: a
  // below-threshold bump array is dropped in every tile, so neither the tile
  // interior nor the shared-seam neighborhood shows partial coverage.  Guards
  // against a boundary-only rendering artifact (e.g. a stray black/edge seam)
  // once the global edge-snap was removed in favor of the Qt-parity cull.
  odb::dbMaster* m = lib_->findMaster("INV_X1");
  ASSERT_NE(m, nullptr);
  m->setType(odb::dbMasterType::COVER_BUMP);

  buildArray(/*n=*/128);
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  // Two horizontally adjacent z=1 tiles sharing a boundary inside the array.
  auto left = decodePng(tile_gen_->generateTile("_instances", 1, 0, 0), w, h);
  auto right = decodePng(tile_gen_->generateTile("_instances", 1, 1, 0), w, h);
  const int iw = static_cast<int>(w);
  const int ih = static_cast<int>(h);

  // The _instances pass also draws the always-on gray die/core outline (Qt
  // drawChip parity).  That isn't array coverage, so exclude it: neutral
  // gray at any alpha (the supersampled render is decimated, so outline
  // pixels come back with partial coverage).
  auto is_array_pixel = [](const unsigned char* p) {
    if (p[3] == 0) {
      return false;
    }
    return p[0] != 128 || p[1] != 128 || p[2] != 128;
  };

  auto coverage = [&](const std::vector<unsigned char>& px, int xa, int xb) {
    int nz = 0;
    int tot = 0;
    for (int y = 0; y < ih; ++y) {
      for (int x = xa; x < xb; ++x) {
        ++tot;
        if (is_array_pixel(&px[(static_cast<size_t>(y) * iw + x) * 4])) {
          ++nz;
        }
      }
    }
    return tot > 0 ? static_cast<double>(nz) / tot : 0.0;
  };

  // Interior coverage vs the seam neighborhood: a few columns on each side of
  // the shared edge.  Under the sub-resolution cull both must be empty — the
  // array is dropped consistently, with no partial coverage leaking at the
  // boundary.
  const double interior = coverage(left, iw / 4, 3 * iw / 4);
  int seam_nz = 0;
  int seam_tot = 0;
  for (int y = 0; y < ih; ++y) {
    for (int x = iw - 4; x < iw; ++x) {  // left tile, right edge
      ++seam_tot;
      if (is_array_pixel(&left[(static_cast<size_t>(y) * iw + x) * 4])) {
        ++seam_nz;
      }
    }
    for (int x = 0; x < 4; ++x) {  // right tile, left edge
      ++seam_tot;
      if (is_array_pixel(&right[(static_cast<size_t>(y) * iw + x) * 4])) {
        ++seam_nz;
      }
    }
  }
  const double seam = static_cast<double>(seam_nz) / seam_tot;
  EXPECT_EQ(interior, 0.0)
      << "sub-resolution bump array was not culled in the tile interior";
  EXPECT_EQ(seam, 0.0)
      << "partial coverage leaked at the tile seam (cull not uniform across "
         "adjacent tiles)";
}

TEST_F(TileGeneratorTest, HiDpiTileRendersAtDeviceResolution)
{
  placeInst("BUF_X16", "buf1", 10000, 10000);
  makeTileGen();
  unsigned w = 0;
  unsigned h = 0;
  // dpr=2 → the tile is rendered at 256*2 physical pixels so it maps 1:1 onto
  // a HiDPI device grid (no browser resampling → no re-aliased moiré).
  auto png = tile_gen_->generateTile("_instances",
                                     0,
                                     0,
                                     0,
                                     /*vis=*/{},
                                     /*highlight_rects=*/{},
                                     /*highlight_polys=*/{},
                                     /*colored_rects=*/{},
                                     /*flight_lines=*/{},
                                     /*module_colors=*/nullptr,
                                     /*focus_net_ids=*/nullptr,
                                     /*route_guide_net_ids=*/nullptr,
                                     /*dpr=*/2.0);
  auto pixels = decodePng(png, w, h);
  EXPECT_EQ(w, 512u);
  EXPECT_EQ(h, 512u);
  EXPECT_TRUE(hasNonTransparentPixel(pixels));
}

TEST_F(TileGeneratorTest, TileCacheStoresEvictsAndPromotes)
{
  makeTileGen();
  constexpr size_t kCap = 512;  // mirrors TileGenerator::kTileCacheCap
  for (size_t i = 0; i < kCap + 10; ++i) {
    tile_gen_->tileCachePut("k" + std::to_string(i),
                            {static_cast<unsigned char>(i & 0xff),
                             static_cast<unsigned char>((i >> 8) & 0xff)});
  }
  EXPECT_EQ(tile_gen_->tileCacheSize(), kCap);

  std::vector<unsigned char> out;
  // The 10 oldest keys (k0..k9) were evicted.
  EXPECT_FALSE(tile_gen_->tileCacheGet("k0", out));
  EXPECT_FALSE(tile_gen_->tileCacheGet("k9", out));
  // A recent key still returns its exact bytes.
  ASSERT_TRUE(tile_gen_->tileCacheGet("k" + std::to_string(kCap + 9), out));
  EXPECT_EQ(out.size(), 2u);

  // Promotion (LRU): touch the oldest survivor (k10), then overflow by one.
  // k10 must survive because the touch made it most-recently-used; the next
  // oldest (k11) is evicted instead.
  ASSERT_TRUE(tile_gen_->tileCacheGet("k10", out));
  tile_gen_->tileCachePut("knew", {7});
  EXPECT_TRUE(tile_gen_->tileCacheGet("k10", out));
  EXPECT_FALSE(tile_gen_->tileCacheGet("k11", out));

  // Design reload clears the cache.
  tile_gen_->eagerInit();
  EXPECT_EQ(tile_gen_->tileCacheSize(), 0u);
}

// Heat-map value labels must render across tile boundaries.  A bin whose center
// falls on a tile seam previously had its number drawn only in the tile
// containing the center, clipping the digits on the other side (e.g. "29.89"
// showing as ".89").  See issue #10925.
TEST_F(TileGeneratorTest, HeatMapNumbersRenderAcrossTileBoundary)
{
  // Center column [30000,60000] is centered on the vertical seam (x=45000);
  // bottom row [0,30000] sits inside a single tile row (maps to tile y=1).
  ASSERT_NO_FATAL_FAILURE(buildSeamDesign(
      odb::Rect(kSeamDieSide / 3, 0, 2 * kSeamDieSide / 3, kSeamDieSide / 3)));

  const std::set<int> left = seamTextPixels(1, 0, 1, Axis::kColumn);
  const std::set<int> right = seamTextPixels(1, 1, 1, Axis::kColumn);

  // Regression check: the left tile (which does NOT contain the bin center)
  // must still render the leading digits.  Before the fix it drew nothing.
  ASSERT_FALSE(left.empty())
      << "left tile has no number pixels: leading digits were clipped";
  ASSERT_FALSE(right.empty()) << "right tile has no number pixels";

  // The left tile's text hugs its right edge and the right tile's hugs its left
  // edge -- together they form the full label across the seam.
  EXPECT_GE(*left.begin(), kTileSize / 2);
  EXPECT_LT(*right.rbegin(), kTileSize / 2);
}

// Same as above but for the horizontal seam: the fix clips the text box in y
// symmetrically with x, so a bin centered on a horizontal tile boundary must
// render its label in both vertically-adjacent tiles.
TEST_F(TileGeneratorTest, HeatMapNumbersRenderAcrossHorizontalTileBoundary)
{
  // Center row [30000,60000] is centered on the horizontal seam (y=45000);
  // left column [0,30000] sits inside a single tile column (tile x=0).
  ASSERT_NO_FATAL_FAILURE(buildSeamDesign(
      odb::Rect(0, kSeamDieSide / 3, kSeamDieSide / 3, 2 * kSeamDieSide / 3)));

  const std::set<int> top = seamTextPixels(1, 0, 0, Axis::kRow);
  const std::set<int> bottom = seamTextPixels(1, 0, 1, Axis::kRow);

  // Regression check: the bottom tile (whose DBU range excludes the bin center
  // at y=45000) must still render its half of the label.
  ASSERT_FALSE(bottom.empty())
      << "bottom tile has no number pixels: label was clipped at the seam";
  ASSERT_FALSE(top.empty()) << "top tile has no number pixels";

  // The top tile's text hugs its bottom edge and the bottom tile's hugs its top
  // edge -- together they form the full label across the seam.
  EXPECT_GE(*top.begin(), kTileSize / 2);
  EXPECT_LT(*bottom.rbegin(), kTileSize / 2);
}

TEST_F(TileGeneratorTest, InPlaceDesignEditInvalidatesTileCache)
{
  // A geometry edit that happens without a full reload (e.g. an instance moved
  // by placement) must drop the cached PNGs and notify clients — otherwise the
  // web viewer serves stale tiles.  This is maliberty's cache-invalidation
  // note.
  odb::dbInst* inst = placeInst("INV_X1", "i1", 10000, 10000);
  ASSERT_NE(inst, nullptr);
  makeTileGen();

  int refresh_calls = 0;
  tile_gen_->setDesignChangedCallback([&refresh_calls] { ++refresh_calls; });

  // Build the instance R-tree so the edit is a valid→invalid transition:
  // Search::announceModified debounces and only fires once the index exists.
  tile_gen_->generateTile("_instances", 0, 0, 0);

  tile_gen_->tileCachePut("dummy", {1, 2, 3});
  ASSERT_GT(tile_gen_->tileCacheSize(), 0u);

  // Move the placed instance: odb fires inDbPostMoveInst → Search::clearInsts →
  // announceModified → TileGenerator::onDesignChanged.
  inst->setLocation(20000, 20000);

  EXPECT_EQ(tile_gen_->tileCacheSize(), 0u)
      << "in-place design edit left stale PNGs in the tile cache";
  EXPECT_GE(refresh_calls, 1)
      << "design edit did not notify clients to re-request tiles";
}

TEST_F(TileGeneratorTest, DieAreaChangeInvalidatesTileCache)
{
  // A die-area resize moves the tile bounds (getBounds), so every cached PNG
  // (keyed by z/x/y) is stale afterwards.  It reaches Search only via
  // inDbBlockSetDieArea, whose setTopChip early-returns on the unchanged chip;
  // Search::notifyModified must still fire so the cache is dropped and clients
  // are told to re-request.  (Instance moves are covered separately; this
  // guards the geometry edits that don't map to a spatial index.)
  placeInst("INV_X1", "i1", 10000, 10000);
  makeTileGen();

  int refresh_calls = 0;
  tile_gen_->setDesignChangedCallback([&refresh_calls] { ++refresh_calls; });

  tile_gen_->tileCachePut("dummy", {1, 2, 3});
  ASSERT_GT(tile_gen_->tileCacheSize(), 0u);

  block_->setDieArea(odb::Rect(0, 0, 120000, 120000));

  EXPECT_EQ(tile_gen_->tileCacheSize(), 0u)
      << "die-area change left stale PNGs in the tile cache";
  EXPECT_GE(refresh_calls, 1)
      << "die-area change did not notify clients to re-request tiles";
}

TEST_F(TileGeneratorTest, EagerInitReindexDoesNotSpuriouslyNotify)
{
  // eagerInit() clears the cache itself and drives its own client refresh, so
  // its bulk reindex must NOT fire the design-changed callback again.
  placeInst("INV_X1", "i1", 10000, 10000);
  makeTileGen();
  tile_gen_->generateTile("_instances", 0, 0, 0);  // build the index once

  int refresh_calls = 0;
  tile_gen_->setDesignChangedCallback([&refresh_calls] { ++refresh_calls; });

  // Prove the callback is actually wired: a real design edit fires it.  Without
  // this, the test below could pass simply because the callback was never
  // installed.
  block_->setDieArea(odb::Rect(0, 0, 120000, 120000));
  ASSERT_GE(refresh_calls, 1);

  tile_gen_->tileCachePut("dummy", {1, 2, 3});
  refresh_calls = 0;
  tile_gen_->eagerInit();

  EXPECT_EQ(refresh_calls, 0)
      << "eagerInit reindex fired the design-changed callback";
  EXPECT_EQ(tile_gen_->tileCacheSize(), 0u)
      << "eagerInit did not clear the tile cache";
}

//------------------------------------------------------------------------------
// dbuPrecision / dbuToMicronString
//
// Both the inspector's property formatting (ScopedDbuFormat) and the WEB "tile"
// / "select" debug lines print DBU lengths in microns at this precision, so the
// contract is: never print two adjacent DBU as the same string.
//------------------------------------------------------------------------------

// One row per DATABASE MICRONS value a real PDK uses, plus the boundaries.
struct DbuScaleCase
{
  double dbu_per_micron;
  int precision;
  const char* one_dbu;  // 1 DBU rendered in microns
};

// 2000 (Nangate45) and 20000 are the rows that pin ceil() over round(): round()
// would give 3 and 4 here, which collapses 1 DBU onto 2 DBU.  1000 / 10000 /
// 100000 are the exact powers of ten, where a log10 that lands a hair high
// would ceil() to one digit too many.
constexpr DbuScaleCase kDbuScales[] = {
    {1.0, 0, "1"},
    {100.0, 2, "0.01"},
    {200.0, 3, "0.005"},
    {1000.0, 3, "0.001"},   // sky130, asap7, ihp-sg13g2
    {2000.0, 4, "0.0005"},  // Nangate45
    {4000.0, 4, "0.0003"},  // not a divisor of 10^4 — nearest grid point
    {10000.0, 4, "0.0001"},
    {20000.0, 5, "0.00005"},
    {100000.0, 5, "0.00001"},
};

TEST(DbuFormatTest, PrecisionMatchesTheDatabaseScale)
{
  for (const auto& c : kDbuScales) {
    EXPECT_EQ(dbuPrecision(c.dbu_per_micron), c.precision)
        << "dbu_per_micron=" << c.dbu_per_micron;
    EXPECT_EQ(dbuToMicronString(1, c.dbu_per_micron), c.one_dbu)
        << "dbu_per_micron=" << c.dbu_per_micron;
  }
}

// The invariant the precision exists for: adjacent DBU must stay distinct.
// This is what round() breaks at 2000 DBU/um.
TEST(DbuFormatTest, AdjacentDbuNeverCollapseOntoTheSameString)
{
  for (const auto& c : kDbuScales) {
    for (int dbu = 0; dbu < 8; ++dbu) {
      EXPECT_NE(dbuToMicronString(dbu, c.dbu_per_micron),
                dbuToMicronString(dbu + 1, c.dbu_per_micron))
          << "dbu_per_micron=" << c.dbu_per_micron << " dbu=" << dbu;
    }
  }
}

// A power of ten must not pick up a spurious extra digit from log10 rounding.
TEST(DbuFormatTest, PowersOfTenGetExactlyTheirExponent)
{
  double scale = 1.0;
  for (int exponent = 0; exponent <= 9; ++exponent) {
    EXPECT_EQ(dbuPrecision(scale), exponent) << "1e" << exponent;
    scale *= 10.0;
  }
}

// Whole microns and typical coordinates come out without trailing noise.
TEST(DbuFormatTest, WholeAndFractionalMicronsRoundTrip)
{
  EXPECT_EQ(dbuToMicronString(1000, 1000.0), "1");
  EXPECT_EQ(dbuToMicronString(974400, 1000.0), "974.4");
  EXPECT_EQ(dbuToMicronString(-5760, 1000.0), "-5.76");
  EXPECT_EQ(dbuToMicronString(0, 1000.0), "0");
  // 2000 DBU/um: a half-DBU-per-milli scale still prints exactly.
  EXPECT_EQ(dbuToMicronString(2000, 2000.0), "1");
  EXPECT_EQ(dbuToMicronString(1, 2000.0), "0.0005");
  EXPECT_EQ(dbuToMicronString(3, 2000.0), "0.0015");
}

// Before any LEF is read the database reports no scale; callers get raw DBU
// rather than a division by zero.
TEST(DbuFormatTest, NoScaleFallsBackToRawDbu)
{
  EXPECT_EQ(dbuPrecision(0.0), 0);
  EXPECT_EQ(dbuToMicronString(12345, 0.0), "12345");
  EXPECT_EQ(dbuPrecision(-1.0), 0);
  EXPECT_EQ(dbuToMicronString(12345, -1.0), "12345");
}

// The scale the tests above model is the one the fixture's tech actually has.
TEST_F(TileGeneratorTest, NangateScaleIsTheOneModelledAbove)
{
  EXPECT_EQ(getDb()->getDbuPerMicron(), 2000u);
  EXPECT_EQ(dbuPrecision(getDb()->getDbuPerMicron()), 4);
}

//------------------------------------------------------------------------------
// Debug-graphics overlay: the two halves of the gui::Renderer API.  Qt calls
// drawLayer once per tech layer and drawObjects once after the layers; the web
// used to call only drawObjects, and once per layer tile at that.
//------------------------------------------------------------------------------

// Records which layer each debug-overlay invocation was for.  nullptr stands
// for the layer-independent drawObjects pass.
struct DebugOverlayRecorder
{
  std::vector<std::string> layer_calls;  // one entry per drawLayer pass
  int object_calls = 0;                  // drawObjects passes

  void install()
  {
    TileGenerator::setRendererHooks({.draw = [this](std::vector<unsigned char>&,
                                                    const TileFrame&,
                                                    bool,
                                                    odb::dbTechLayer* layer) {
      if (layer != nullptr) {
        layer_calls.emplace_back(layer->getName());
      } else {
        ++object_calls;
      }
    }});
  }

  static void clear() { TileGenerator::setRendererHooks({}); }
};

TEST_F(TileGeneratorTest, DebugOverlayPassesTheTileTechLayer)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  DebugOverlayRecorder recorder;
  recorder.install();

  TileVisibility vis;
  vis.debug_renderers = true;
  vis.debug_live = true;
  tile_gen_->generateTile("metal1", 0, 0, 0, vis);
  tile_gen_->generateTile("metal3", 0, 0, 0, vis);
  DebugOverlayRecorder::clear();

  // Each layer tile drives Renderer::drawLayer for its OWN layer, so a
  // renderer that draws per layer lands on the right tile.
  EXPECT_EQ(recorder.layer_calls,
            (std::vector<std::string>{"metal1", "metal3"}));
  EXPECT_EQ(recorder.object_calls, 0)
      << "the layer tiles must not carry the drawObjects pass";
}

TEST_F(TileGeneratorTest, DebugOverlayObjectsPassRunsOncePerTile)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  DebugOverlayRecorder recorder;
  recorder.install();

  TileVisibility vis;
  vis.debug_renderers = true;
  vis.debug_live = true;
  // What a client does for one tile: a tile per visible layer, plus one
  // overlay tile.  drawObjects used to run once per layer here.
  for (const char* layer : {"metal1", "metal2", "metal3", "metal4"}) {
    tile_gen_->generateTile(layer, 0, 0, 0, vis);
  }
  tile_gen_->generateOverlayTile(0,
                                 0,
                                 0,
                                 /*highlight_rects=*/{},
                                 /*highlight_polys=*/{},
                                 /*colored_rects=*/{},
                                 /*flight_lines=*/{},
                                 /*route_guide_net_ids=*/nullptr,
                                 /*has_visible_layers=*/false,
                                 /*visible_layers=*/{},
                                 /*dpr=*/1.0,
                                 /*tile_px=*/0,
                                 /*colored_polys=*/{},
                                 /*labels=*/{},
                                 /*debug_renderers=*/true,
                                 /*debug_live=*/true);
  DebugOverlayRecorder::clear();

  EXPECT_EQ(recorder.object_calls, 1)
      << "drawObjects belongs to the overlay tile, not to every layer";
  EXPECT_EQ(recorder.layer_calls.size(), 4u);
}

// The pseudo layers ("_instances", the grid overlays) have no tech layer, so
// they take no part in the per-layer pass -- otherwise a renderer would be
// asked to draw a layer that does not exist.
TEST_F(TileGeneratorTest, DebugOverlaySkipsPseudoLayers)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  DebugOverlayRecorder recorder;
  recorder.install();

  TileVisibility vis;
  vis.debug_renderers = true;
  vis.debug_live = true;
  tile_gen_->generateTile("_instances", 0, 0, 0, vis);
  tile_gen_->generateTile("_mfg_grid", 0, 0, 0, vis);
  DebugOverlayRecorder::clear();

  EXPECT_TRUE(recorder.layer_calls.empty());
  EXPECT_EQ(recorder.object_calls, 0);
}

// An overlay tile with no shapes at all still has to render: while a tool is
// paused mid-run the debug graphics are the only thing on it.
TEST_F(TileGeneratorTest, DebugOverlayDefeatsTheEmptyOverlayShortCircuit)
{
  placeInst("BUF_X16", "buf1", 0, 0);
  makeTileGen();

  DebugOverlayRecorder recorder;
  recorder.install();
  tile_gen_->generateOverlayTile(0,
                                 0,
                                 0,
                                 {},
                                 {},
                                 {},
                                 {},
                                 nullptr,
                                 false,
                                 {},
                                 1.0,
                                 0,
                                 {},
                                 {},
                                 /*debug_renderers=*/true,
                                 /*debug_live=*/true);
  EXPECT_EQ(recorder.object_calls, 1);

  // And with the toggle off the short-circuit still applies.
  recorder.object_calls = 0;
  tile_gen_->generateOverlayTile(0,
                                 0,
                                 0,
                                 {},
                                 {},
                                 {},
                                 {},
                                 nullptr,
                                 false,
                                 {},
                                 1.0,
                                 0,
                                 {},
                                 {},
                                 /*debug_renderers=*/false,
                                 /*debug_live=*/false);
  DebugOverlayRecorder::clear();
  EXPECT_EQ(recorder.object_calls, 0);
}

}  // namespace
}  // namespace web
