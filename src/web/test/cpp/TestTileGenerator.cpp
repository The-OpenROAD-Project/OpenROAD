// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
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

constexpr double kPi = 3.14159265358979323846;

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
  // 10 routing + 9 cut layers
  EXPECT_EQ(layers.size(), 19);
  EXPECT_EQ(layers.front(), "metal1");
  EXPECT_EQ(layers.back(), "metal10");
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
  EXPECT_FALSE(hasNonTransparentPixel(pixels));
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
  EXPECT_FALSE(hasNonTransparentPixel(pixels))
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
    auto pixels
        = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
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
    auto pixels
        = decodePng(tile_gen_->generateTile("_instances", 3, 4, 4), w, h);
    // The resolved grid's fundamental (~16 px pitch) legitimately lives in the
    // beat band, so beatFrac is NOT a valid check here — the point is only that
    // the structure survived (high block-CV), i.e. it wasn't smeared to a tint.
    EXPECT_GT(blockAlphaCV(pixels, w, h, 8), 0.10)
        << "resolved grid was over-blurred into a flat tint";
  }

  TEST_F(MoireArrayTest, BumpArrayBelowThresholdIsFaithfulTintNotSheet)
  {
    // Mark the small master as a bump so classifyInstance() returns kPhysBump
    // (the fixture has no STA, so it falls back to the COVER_BUMP master type).
    odb::dbMaster* m = lib_->findMaster("INV_X1");
    ASSERT_NE(m, nullptr);
    m->setType(odb::dbMasterType::COVER_BUMP);

    buildArray(
        /*n=*/128);  // bumps render ~1 px at z=0 → below the LOD threshold
    makeTileGen();
    unsigned w = 0;
    unsigned h = 0;
    auto pixels
        = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
    const int iw = static_cast<int>(w);
    const int ih = static_cast<int>(h);

    // The sub-resolution array is NOT collapsed into one opaque slab over the
    // gaps (the rejected "merged sheet").  Each bump becomes a discrete
    // coverage mark, so the central interior is a FAITHFUL FAINT tint: its mean
    // alpha reflects the real ~25% fill (pitch = 2x cell) and stays well below
    // the opaque layer color, while still being non-transparent (bumps
    // present). The supersample + Lanczos low-pass keeps the beat band empty
    // (DenseArraySubPixelHasNoBeat); here we guard against the opaque sheet.
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
    // _instances overview tints at gray alpha=160; a solid sheet would average
    // ~160.  Faithful ~25% coverage averages far lower.
    EXPECT_GT(mean_alpha, 5.0) << "sub-resolution bumps vanished (no coverage)";
    EXPECT_LT(mean_alpha, 100.0)
        << "sub-resolution bump array collapsed into an opaque merged sheet";
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
    auto pixels
        = decodePng(tile_gen_->generateTile("_instances", 0, 0, 0), w, h);
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

  TEST_F(MoireArrayTest, BumpArrayContinuesAcrossTileSeam)
  {
    // Removing the global edge-snap must not reintroduce a black seam: a bump
    // whose footprint straddles the tile boundary is clamped to the edge, so
    // the array texture continues across adjacent tiles with no dead
    // transparent band wider than the normal inter-bump gap.
    odb::dbMaster* m = lib_->findMaster("INV_X1");
    ASSERT_NE(m, nullptr);
    m->setType(odb::dbMasterType::COVER_BUMP);

    buildArray(/*n=*/128);
    makeTileGen();
    unsigned w = 0;
    unsigned h = 0;
    // Two horizontally adjacent z=1 tiles sharing a boundary inside the array.
    auto left = decodePng(tile_gen_->generateTile("_instances", 1, 0, 0), w, h);
    auto right
        = decodePng(tile_gen_->generateTile("_instances", 1, 1, 0), w, h);
    const int iw = static_cast<int>(w);
    const int ih = static_cast<int>(h);

    auto coverage = [&](const std::vector<unsigned char>& px, int xa, int xb) {
      int nz = 0;
      int tot = 0;
      for (int y = 0; y < ih; ++y) {
        for (int x = xa; x < xb; ++x) {
          ++tot;
          if (px[(static_cast<size_t>(y) * iw + x) * 4 + 3] > 0) {
            ++nz;
          }
        }
      }
      return tot > 0 ? static_cast<double>(nz) / tot : 0.0;
    };

    // Interior coverage (reference) vs the seam neighborhood: a few columns on
    // each side of the shared edge.  No systematic dropout at the boundary.
    const double interior = coverage(left, iw / 4, 3 * iw / 4);
    int seam_nz = 0;
    int seam_tot = 0;
    for (int y = 0; y < ih; ++y) {
      for (int x = iw - 4; x < iw; ++x) {  // left tile, right edge
        ++seam_tot;
        if (left[(static_cast<size_t>(y) * iw + x) * 4 + 3] > 0) {
          ++seam_nz;
        }
      }
      for (int x = 0; x < 4; ++x) {  // right tile, left edge
        ++seam_tot;
        if (right[(static_cast<size_t>(y) * iw + x) * 4 + 3] > 0) {
          ++seam_nz;
        }
      }
    }
    const double seam = static_cast<double>(seam_nz) / seam_tot;
    ASSERT_GT(interior, 0.0);
    EXPECT_GT(seam, 0.3 * interior)
        << "transparent band at the tile boundary → black seam between tiles";
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

}  // namespace
}  // namespace web
