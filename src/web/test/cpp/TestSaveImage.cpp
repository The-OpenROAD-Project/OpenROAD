// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "boost/json/parse.hpp"
#include "color.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "third-party/lodepng/lodepng.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"
#include "web/heatMap.h"

namespace web {
namespace {

class SaveImageTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    placeInst("BUF_X16", "buf1", 10000, 10000);
    makeTileGen();
  }

  void TearDown() override
  {
    // Clean up any output files.
    for (const auto& path : output_files_) {
      std::filesystem::remove(path);
    }
  }

  void makeTileGen()
  {
    tile_gen_ = std::make_unique<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    tile_gen_->eagerInit();
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

  // Save to a temp file and register for cleanup.
  std::string tempPng(const std::string& label)
  {
    std::string path
        = std::filesystem::temp_directory_path()
          / ("web_test_" + label + "_" + std::to_string(::getpid()) + ".png");
    output_files_.push_back(path);
    return path;
  }

  // Decode a PNG file from disk; returns RGBA pixels.
  std::vector<unsigned char> decodePngFile(const std::string& path,
                                           unsigned& width,
                                           unsigned& height)
  {
    std::vector<unsigned char> pixels;
    unsigned err = lodepng::decode(pixels, width, height, path);
    EXPECT_EQ(err, 0u) << lodepng_error_text(err);
    return pixels;
  }

  static bool hasNonTransparentPixel(const std::vector<unsigned char>& rgba)
  {
    for (size_t i = 3; i < rgba.size(); i += 4) {
      if (rgba[i] > 0) {
        return true;
      }
    }
    return false;
  }

  // True if any visible pixel isn't part of the always-on die/core outline,
  // which getBounds() now guarantees is in every saved image.  Matches
  // TileGeneratorTest::hasNonOutlinePixel: the outline is kOutlineGray and
  // alpha is NOT checked, because tiles are rasterized supersampled and
  // decimated, so its edge pixels come back at partial coverage while the RGB
  // stays put.  Testing by colour rather than by carving out a border keeps
  // the die edge itself in scope — that is where pin markers are drawn.
  static bool hasNonOutlinePixel(const std::vector<unsigned char>& rgba)
  {
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
      if (rgba[i + 3] == 0) {
        continue;
      }
      if (rgba[i] != kOutlineGray.r || rgba[i + 1] != kOutlineGray.g
          || rgba[i + 2] != kOutlineGray.b) {
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

  std::unique_ptr<TileGenerator> tile_gen_;
  std::vector<std::string> output_files_;
};

// ─── Basic functionality ─────────────────────────────────────────────────────

TEST_F(SaveImageTest, DefaultProducesValidPng)
{
  const std::string path = tempPng("default");
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 0, 0, {});

  ASSERT_TRUE(std::filesystem::exists(path));
  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_GT(w, 0u);
  EXPECT_GT(h, 0u);
  // Should contain visible content (placed instance).
  EXPECT_TRUE(hasNonTransparentPixel(pixels));
}

// Tiles are rasterized on transparency, so the pixels no layer covers come out
// transparent by default.  A caller saving what a viewer shows passes that
// viewer's background instead -- what WebServer::saveImage does, so that
// `save_image -web` matches the Qt GUI's opaque background rather than writing
// a transparent PNG.
TEST_F(SaveImageTest, BackgroundFillsUncoveredPixels)
{
  const std::string default_path = tempPng("bg_default");
  tile_gen_->saveImage(default_path, odb::Rect(0, 0, 0, 0), 256, 0, {});

  // Deliberately not black: black would also be what a transparent pixel
  // decodes to, and this has to show the fill color is the one honoured.
  constexpr Color kMagenta{.r = 255, .g = 0, .b = 255, .a = 255};
  const std::string filled_path = tempPng("bg_filled");
  tile_gen_->saveImage(
      filled_path, odb::Rect(0, 0, 0, 0), 256, 0, {}, kMagenta);

  unsigned default_w = 0, default_h = 0;
  const auto default_pixels = decodePngFile(default_path, default_w, default_h);
  EXPECT_LT(countNonTransparentPixels(default_pixels),
            default_pixels.size() / 4)
      << "the default should leave the uncovered pixels transparent";

  unsigned filled_w = 0, filled_h = 0;
  const auto filled_pixels = decodePngFile(filled_path, filled_w, filled_h);
  ASSERT_EQ(filled_w, default_w);
  ASSERT_EQ(filled_h, default_h);
  EXPECT_EQ(countNonTransparentPixels(filled_pixels), filled_pixels.size() / 4)
      << "a background makes every pixel opaque";
  // The margin corner is outside the die, so no layer draws there and the
  // background is all that is left.
  ASSERT_GE(filled_pixels.size(), 4u);
  EXPECT_EQ(filled_pixels[0], kMagenta.r);
  EXPECT_EQ(filled_pixels[1], kMagenta.g);
  EXPECT_EQ(filled_pixels[2], kMagenta.b);
  EXPECT_EQ(filled_pixels[3], kMagenta.a);
}

TEST_F(SaveImageTest, WidthOption)
{
  const std::string path = tempPng("width");
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 512, 0, {});

  unsigned w = 0, h = 0;
  decodePngFile(path, w, h);
  EXPECT_EQ(w, 512u);
}

TEST_F(SaveImageTest, ResolutionOption)
{
  const std::string path = tempPng("resolution");
  // 10 dbu per pixel on a 100000 dbu (+margin) design → ~10000+ pixels wide.
  // Use a coarser resolution to keep the test fast.
  const double dbu_per_pixel = 100.0;
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 0, dbu_per_pixel, {});

  unsigned w = 0, h = 0;
  decodePngFile(path, w, h);
  // Expected: ~100000 / 100 * 1.05 (bloat) ≈ 1050.
  // Allow some tolerance for rounding and bloat margin.
  EXPECT_GT(w, 500u);
  EXPECT_LT(w, 2000u);
}

// A zero-area request means the rect the viewer frames on -- the die area
// unioned with the block bbox -- plus 5% of its smaller dimension.  That is
// the rule Gui::saveImage gives the Qt path, so the two renderers frame a
// default save_image identically; framing on the die alone (or bloating by the
// LARGER dimension) put them a percent apart.
TEST_F(SaveImageTest, ZeroAreaFramesDieUnionBBoxWithMargin)
{
  // Place an instance past the right die edge so the bbox is not contained in
  // the die and the union is the only rect that covers both.
  placeInst("BUF_X16", "overhang", 99000, 50000);
  makeTileGen();

  odb::Rect expected = block_->getBBox()->getBox();
  expected.merge(block_->getDieArea());
  ASSERT_GT(expected.xMax(), block_->getDieArea().xMax())
      << "the overhanging instance should widen the union";
  expected.bloat(
      static_cast<int>(std::min(expected.dx(), expected.dy()) * 0.05),
      expected);

  const std::string zero_area_path = tempPng("frame_zero_area");
  tile_gen_->saveImage(zero_area_path, odb::Rect(0, 0, 0, 0), 512, 0, {});

  const std::string explicit_path = tempPng("frame_explicit");
  tile_gen_->saveImage(explicit_path, expected, 512, 0, {});

  unsigned zero_w = 0, zero_h = 0;
  const auto zero_pixels = decodePngFile(zero_area_path, zero_w, zero_h);
  unsigned explicit_w = 0, explicit_h = 0;
  const auto explicit_pixels
      = decodePngFile(explicit_path, explicit_w, explicit_h);

  EXPECT_EQ(zero_w, explicit_w);
  EXPECT_EQ(zero_h, explicit_h);
  EXPECT_EQ(zero_pixels, explicit_pixels)
      << "a zero-area save should render exactly that rect";
}

TEST_F(SaveImageTest, ExplicitAreaOption)
{
  const std::string path = tempPng("area");
  // Render only the bottom-left quadrant.
  const odb::Rect area(0, 0, 50000, 50000);
  tile_gen_->saveImage(path, area, 256, 0, {});

  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_EQ(w, 256u);
  // Aspect ratio should be ~1:1 for a square area.
  EXPECT_EQ(h, 256u);
}

// ─── Visibility options ──────────────────────────────────────────────────────

TEST_F(SaveImageTest, VisibilityStdcellsOff)
{
  const std::string path = tempPng("vis_off");
  TileVisibility vis;
  vis.stdcells = false;
  // With stdcells hidden and no routing, the _instances layer holds nothing
  // but the die outline, which Qt draws regardless of instance visibility.
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 256, 0, vis);

  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_FALSE(hasNonOutlinePixel(pixels));
}

TEST_F(SaveImageTest, VisibilityPinsOff_Markers)
{
  // Place a BTerm to generate pin markers.
  makeBTermAtEdge("clk", "metal1", 0, 50000, 200, 200);
  makeTileGen();

  const std::string path_on = tempPng("pins_on");
  TileVisibility vis_on;
  vis_on.stdcells = false;
  tile_gen_->saveImage(path_on, odb::Rect(0, 0, 0, 0), 512, 0, vis_on);

  const std::string path_off = tempPng("pins_off");
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.pins = false;
  tile_gen_->saveImage(path_off, odb::Rect(0, 0, 0, 0), 512, 0, vis_off);

  unsigned w1 = 0, h1 = 0, w2 = 0, h2 = 0;
  auto pixels_on = decodePngFile(path_on, w1, h1);
  auto pixels_off = decodePngFile(path_off, w2, h2);

  // With pins on, there should be visible content from the marker.
  // With it off, no content.
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on));
  EXPECT_NE(pixels_on, pixels_off);
}

TEST_F(SaveImageTest, VisibilityPinsOff)
{
  // BTerm shapes (tech layers + markers) should be hidden when vis.pins=false.
  makeBTermAtEdge("clk", "metal1", 0, 50000, 5000, 5000);
  makeTileGen();

  const std::string path_on = tempPng("bterm_on");
  TileVisibility vis_on;
  vis_on.stdcells = false;
  vis_on.pins = true;
  tile_gen_->saveImage(path_on, odb::Rect(0, 0, 0, 0), 512, 0, vis_on);

  const std::string path_off = tempPng("bterm_off");
  TileVisibility vis_off;
  vis_off.stdcells = false;
  vis_off.pins = false;
  tile_gen_->saveImage(path_off, odb::Rect(0, 0, 0, 0), 512, 0, vis_off);

  unsigned w1 = 0, h1 = 0, w2 = 0, h2 = 0;
  auto pixels_on = decodePngFile(path_on, w1, h1);
  auto pixels_off = decodePngFile(path_off, w2, h2);

  // The die and core outlines are drawn unconditionally on the _instances
  // pass (Qt drawChip parity), so the "pins off" image is never fully
  // transparent.  What the toggle must guarantee is that hiding pins only
  // ever takes pixels away — every BTerm shape and marker disappears and
  // nothing new is drawn in their place.
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on))
      << "BTerm shapes should appear with vis.pins=true";
  EXPECT_LT(countNonTransparentPixels(pixels_off),
            countNonTransparentPixels(pixels_on))
      << "BTerm shapes should be hidden with vis.pins=false";
}

// ─── Edge cases ──────────────────────────────────────────────────────────────

TEST_F(SaveImageTest, EmptyDesign)
{
  // Create a fresh block with no instances.
  odb::dbChip::destroy(chip_);
  chip_ = odb::dbChip::create(getDb(), getDb()->getTech());
  block_ = odb::dbBlock::create(chip_, "empty");
  block_->setDefUnits(lib_->getTech()->getLefUnits());
  block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
  makeTileGen();

  const std::string path = tempPng("empty");
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 256, 0, {});

  ASSERT_TRUE(std::filesystem::exists(path));
  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_EQ(w, 256u);
  // A design with no shapes still has a floorplan: the die outline is drawn
  // and nothing else.
  EXPECT_FALSE(hasNonOutlinePixel(pixels));
  EXPECT_TRUE(hasNonTransparentPixel(pixels))
      << "the die outline should still be drawn";
}

// A hairline is authored as one CSS pixel, and drawLine's brush has to cover
// that many pixels: its radius came out (width-1)/2, so an EVEN width -- which
// is what hairlineCss() returns on the supersampled path -- lost a pixel and
// the stroke went down at half its width.
//
// Measured on the GCell grid, which still strokes into the supersampled buffer
// (the die outline moved to the output-resolution pass, where the hairline is
// an odd 1 px and the bug cannot show).  Alpha is the tell: the grid line is
// written opaque, and half a super-pixel decimates to a third of the colour
// where a full one keeps about two thirds.
TEST_F(SaveImageTest, HairlineStrokesKeepTheirWidth)
{
  odb::dbGCellGrid* grid = odb::dbGCellGrid::create(block_);
  ASSERT_NE(grid, nullptr);
  grid->addGridPatternX(0, 11, 10000);
  grid->addGridPatternY(0, 11, 10000);
  makeTileGen();

  TileVisibility vis;
  vis.gcell_grid = true;
  // 256 px is one whole tile, so the only resample in play is the tile's own
  // decimation -- what the stroke width has to survive.
  const std::string path = tempPng("hairline");
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 256, 0, vis);

  unsigned w = 0, h = 0;
  const auto pixels = decodePngFile(path, w, h);
  // Sampled on rows that hold only the vertical lines: a row ALONG a
  // horizontal line is solid white, and every crossing blends two strokes into
  // one pixel and reaches full alpha whatever width they were drawn at.
  unsigned char peak_alpha = 0;
  for (unsigned y = 0; y < h; ++y) {
    const size_t row = static_cast<size_t>(y) * w * 4;
    std::vector<unsigned char> white_alphas;
    for (unsigned x = 0; x < w; ++x) {
      const size_t i = row + static_cast<size_t>(x) * 4;
      if (pixels[i] == 255 && pixels[i + 1] == 255 && pixels[i + 2] == 255
          && pixels[i + 3] > 0) {
        white_alphas.push_back(pixels[i + 3]);
      }
    }
    // A horizontal line paints the whole row; the vertical lines paint a
    // handful of pixels.  11 grid lines, so allow a little smearing.
    if (white_alphas.empty() || white_alphas.size() > 30) {
      continue;
    }
    for (const unsigned char a : white_alphas) {
      peak_alpha = std::max(peak_alpha, a);
    }
  }
  EXPECT_GT(static_cast<int>(peak_alpha), 130)
      << "the grid lines are thinner than the hairline width asked for";
}

// The die outline is a one-pixel stroke, and it has to survive BOTH resamples
// a saved image goes through: the tile's Lanczos decimation (which is why it is
// drawn after that, at output resolution) and the mosaic-to-image step, which
// picked a single nearest sample and so dropped whole edges at some widths.
// Checked across widths because which edge fell in a skipped column depended on
// the step between the two scales.
TEST_F(SaveImageTest, DieOutlineSurvivesEveryWidth)
{
  odb::dbChip::destroy(chip_);
  chip_ = odb::dbChip::create(getDb(), getDb()->getTech());
  block_ = odb::dbBlock::create(chip_, "outline_only");
  block_->setDefUnits(lib_->getTech()->getLefUnits());
  block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
  makeTileGen();

  for (const int width : {300, 512, 700, 1024}) {
    const std::string path = tempPng("die_outline_" + std::to_string(width));
    tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), width, 0, {});

    unsigned w = 0, h = 0;
    const auto pixels = decodePngFile(path, w, h);
    ASSERT_EQ(w, static_cast<unsigned>(width));

    // The middle row crosses the left and right edges of the die and nothing
    // else, so it must carry exactly two runs of outline.
    int lit = 0;
    const size_t mid_row = static_cast<size_t>(h / 2) * w * 4;
    for (unsigned x = 0; x < w; ++x) {
      if (pixels[mid_row + x * 4 + 3] > 0) {
        ++lit;
      }
    }
    EXPECT_GE(lit, 2) << "at width " << width
                      << " the die outline lost an edge";
  }
}

TEST_F(SaveImageTest, LargeWidthClamped)
{
  const std::string path = tempPng("clamped");
  // Request a very large image; should be clamped to max dimension.
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 100000, 0, {});

  unsigned w = 0, h = 0;
  decodePngFile(path, w, h);
  EXPECT_LE(w, 16384u);
  EXPECT_LE(h, 16384u);
}

TEST_F(SaveImageTest, PinMarkersRendered)
{
  makeBTermAtEdge("in_pin", "metal1", 0, 40000, 200, 200, odb::dbIoType::INPUT);
  makeBTermAtEdge(
      "out_pin", "metal1", 99800, 60000, 200, 200, odb::dbIoType::OUTPUT);
  makeTileGen();

  const std::string path = tempPng("pin_markers");
  TileVisibility vis;
  vis.stdcells = false;
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 512, 0, vis);

  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels));
}

// The marker's size follows the region being drawn, as Qt's does
// (RenderThread::setupIOPins takes min(die, bounds)).  A tile's own span is
// that region only for a client showing a handful of tiles; saveImage
// composites every tile of the level, so sizing off one tile shrank the
// markers by 2^z -- a 2 px nub where Qt draws a 20 px arrow.
//
// Measured against the SAME image with the markers off, so only the arrows are
// in the difference: the BTerm shapes and the die outline cancel out.
TEST_F(SaveImageTest, PinMarkersSizedForTheImageNotTheTile)
{
  makeBTermAtEdge("in_pin", "metal1", 0, 40000, 200, 200, odb::dbIoType::INPUT);
  makeBTermAtEdge(
      "out_pin", "metal1", 99800, 60000, 200, 200, odb::dbIoType::OUTPUT);
  makeTileGen();

  TileVisibility vis;
  vis.stdcells = false;
  const std::string with_markers = tempPng("markers_on");
  tile_gen_->saveImage(with_markers, odb::Rect(0, 0, 0, 0), 512, 0, vis);

  vis.pin_markers = false;
  const std::string without_markers = tempPng("markers_off");
  tile_gen_->saveImage(without_markers, odb::Rect(0, 0, 0, 0), 512, 0, vis);

  unsigned on_w = 0, on_h = 0, off_w = 0, off_h = 0;
  const auto on = decodePngFile(with_markers, on_w, on_h);
  const auto off = decodePngFile(without_markers, off_w, off_h);
  ASSERT_EQ(on_w, off_w);
  ASSERT_EQ(on_h, off_h);

  const size_t marker_px
      = countNonTransparentPixels(on) - countNonTransparentPixels(off);

  // Two markers, each an arrow of pin_max_size = 0.02 * 100000 DBU rendered at
  // 512px / (die + 5%), i.e. ~9 px long and half that across: ~20 px of
  // triangle apiece.  Sized off a tile instead it is ~6 px apiece, so the
  // threshold below separates the two cases with room for rasterization.
  EXPECT_GT(marker_px, 25u)
      << "IO pin markers are too small for the image they are drawn in";
}

TEST_F(SaveImageTest, PinMarkersCanBeHidden)
{
  makeBTermAtEdge("in_pin", "metal1", 0, 40000, 200, 200, odb::dbIoType::INPUT);
  makeTileGen();

  TileVisibility vis;
  vis.stdcells = false;
  const std::string shown = tempPng("markers_shown");
  tile_gen_->saveImage(shown, odb::Rect(0, 0, 0, 0), 512, 0, vis);

  // pin_markers gates the direction arrow alone -- the BTerm's own shape stays,
  // because that is what vis.pins covers.
  vis.pin_markers = false;
  const std::string hidden = tempPng("markers_hidden");
  tile_gen_->saveImage(hidden, odb::Rect(0, 0, 0, 0), 512, 0, vis);

  unsigned w1 = 0, h1 = 0, w2 = 0, h2 = 0;
  const auto shown_px = decodePngFile(shown, w1, h1);
  const auto hidden_px = decodePngFile(hidden, w2, h2);
  EXPECT_LT(countNonTransparentPixels(hidden_px),
            countNonTransparentPixels(shown_px))
      << "pin_markers=false should remove the direction arrows";
  EXPECT_TRUE(hasNonTransparentPixel(hidden_px))
      << "the BTerm shape and die outline should survive";
}

TEST_F(SaveImageTest, MultipleLayersComposited)
{
  // Place instances to generate content on multiple layers.
  placeInst("BUF_X16", "buf2", 50000, 50000);
  makeTileGen();

  const std::string path = tempPng("multi_layer");
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 512, 0, {});

  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_TRUE(hasNonTransparentPixel(pixels));
}

// ─── Composition order ───────────────────────────────────────────────────────

// saveImage must composite in the same z order Leaflet stacks the layers in on
// screen (display-controls.js addPseudoLayer), otherwise the saved PNG is not
// the view the user was looking at.  Before the fix every pseudo layer was
// appended after the tech layers, which put the manufacturing grid over the
// routing and the pin markers over the tech layers (PR #10806 review).
//
// Asserted on the layer list rather than on pixels: every layer here is
// semi-transparent, so a dot drawn over the routing still blends with it and no
// pixel test can tell the two orders apart reliably.
TEST_F(SaveImageTest, LayerCompositionOrderMatchesClientZIndex)
{
  const std::vector<std::string> tech_layers = {"metal1", "metal2"};
  TileVisibility vis;
  vis.pins = true;
  vis.mfg_grid = true;
  vis.access_points = true;
  vis.regions = true;
  vis.gcell_grid = true;
  vis.rudy = true;

  // zIndex on screen: _instances 0, _pins 1, _mfg_grid 2, tech layers 3.., then
  // _access_points 1000, _regions 1001, _gcell_grid 1002, _rudy 1003.
  const std::vector<std::string> expected = {"_instances",
                                             "_pins",
                                             "_mfg_grid",
                                             "metal1",
                                             "metal2",
                                             "_access_points",
                                             "_regions",
                                             "_gcell_grid",
                                             "_rudy"};
  EXPECT_EQ(TileGenerator::saveImageLayerOrder(vis, tech_layers), expected);
}

TEST_F(SaveImageTest, LayerCompositionOrderHonorsVisibility)
{
  const std::vector<std::string> tech_layers = {"metal1"};
  TileVisibility vis;
  vis.pins = false;
  // `regions` is the one overlay that defaults ON (Qt parity), so turn the
  // whole set off explicitly rather than relying on the defaults.
  vis.regions = false;
  vis.mfg_grid = false;
  vis.access_points = false;
  vis.gcell_grid = false;
  vis.rudy = false;

  EXPECT_EQ(TileGenerator::saveImageLayerOrder(vis, tech_layers),
            (std::vector<std::string>{"_instances", "metal1"}))
      << "hidden overlays must not be composited at all";
}

TEST_F(SaveImageTest, ParseFromJsonRudyOption)
{
  TileVisibility vis;
  EXPECT_FALSE(vis.rudy);
  auto json_obj = boost::json::parse("{\"rudy\":true}").as_object();
  vis.parseFromJson(json_obj);
  EXPECT_TRUE(vis.rudy);
}

class TestRUDYHeatMap : public web::HeatMapDataSource
{
 public:
  explicit TestRUDYHeatMap(utl::Logger* logger)
      : web::HeatMapDataSource(logger,
                               "Estimated Congestion (RUDY)",
                               "RUDY",
                               "RUDY")
  {
  }

  odb::Rect getBounds() const override
  {
    return getBlock() ? getBlock()->getDieArea()
                      : odb::Rect(0, 0, 100000, 100000);
  }

 protected:
  bool populateMap() override
  {
    addToMap(odb::Rect(20000, 20000, 80000, 80000), 10.0);
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
};

TEST_F(SaveImageTest, RudyHeatmapRendersInSavedImage)
{
  auto logger = getLogger();
  web::registerHeatMapSource(
      "Estimated Congestion (RUDY)", "RUDY", "RUDY", [logger]() {
        return std::make_shared<TestRUDYHeatMap>(logger);
      });

  const std::string path_no_rudy = tempPng("no_rudy");
  TileVisibility vis_off;
  vis_off.rudy = false;
  tile_gen_->saveImage(path_no_rudy, odb::Rect(0, 0, 0, 0), 512, 0, vis_off);

  const std::string path_rudy = tempPng("with_rudy");
  TileVisibility vis_on;
  vis_on.rudy = true;
  tile_gen_->saveImage(path_rudy, odb::Rect(0, 0, 0, 0), 512, 0, vis_on);

  ASSERT_TRUE(std::filesystem::exists(path_no_rudy));
  ASSERT_TRUE(std::filesystem::exists(path_rudy));

  unsigned w1 = 0, h1 = 0;
  auto pixels_no_rudy = decodePngFile(path_no_rudy, w1, h1);
  unsigned w2 = 0, h2 = 0;
  auto pixels_rudy = decodePngFile(path_rudy, w2, h2);

  EXPECT_EQ(w1, 512u);
  EXPECT_EQ(w2, 512u);
  EXPECT_GT(countNonTransparentPixels(pixels_rudy),
            countNonTransparentPixels(pixels_no_rudy))
      << "Enabling rudy heatmap should render additional heatmap pixels";
}

TEST_F(SaveImageTest, LayerCompositionOrderHonorsTechLayerVisibility)
{
  const std::vector<std::string> tech_layers = {"metal1", "metal2", "metal3"};
  TileVisibility vis;
  vis.has_visible_layers = true;
  vis.visible_layers = {"metal2"};
  vis.pins = false;
  vis.regions = false;
  vis.mfg_grid = false;
  vis.access_points = false;
  vis.gcell_grid = false;
  vis.rudy = false;

  EXPECT_EQ(TileGenerator::saveImageLayerOrder(vis, tech_layers),
            (std::vector<std::string>{"_instances", "metal2"}))
      << "tech layers hidden via visible_layers must not be composited";
}

TEST_F(SaveImageTest, HiddenTechLayerIsNotDrawn)
{
  placeInst("BUF_X16", "buf2", 50000, 50000);
  makeTileGen();
  const odb::Rect region = tile_gen_->getBounds();

  TileVisibility all;
  const auto with_all = tile_gen_->renderImageBuffer(region, 512, 0, all);
  TileVisibility none;
  none.has_visible_layers = true;  // visible_layers empty: hide every one
  const auto with_none = tile_gen_->renderImageBuffer(region, 512, 0, none);

  ASSERT_FALSE(with_all.empty());
  ASSERT_EQ(with_all.size(), with_none.size());
  EXPECT_LT(countNonTransparentPixels(with_none),
            countNonTransparentPixels(with_all))
      << "hiding all tech layers must remove their pixels";
}

// Tiles are rendered concurrently into disjoint output rectangles, so the
// result must not depend on the thread count.  This is the test that catches
// a shared-state race in the render path.
TEST_F(SaveImageTest, ThreadCountDoesNotChangeOutput)
{
  for (int i = 0; i < 40; ++i) {
    placeInst("BUF_X16", ("b" + std::to_string(i)).c_str(), 2000 * i, 3000 * i);
  }
  makeTileGen();
  const odb::Rect region = tile_gen_->getBounds();
  TileVisibility vis;

  tile_gen_->setThreadCount(1);
  const auto one = tile_gen_->renderImageBuffer(region, 1024, 0, vis);
  tile_gen_->setThreadCount(8);
  const auto eight = tile_gen_->renderImageBuffer(region, 1024, 0, vis);

  ASSERT_FALSE(one.empty());
  EXPECT_EQ(one, eight);
}

// Qt gates drawLabels on the Misc/"Labels" control, and its save_image goes
// through the same painter, so a saved image reproduces a view with labels
// hidden.  save_image -display_option {labels false} has to do the same here.
TEST_F(SaveImageTest, LabelsFollowTheVisibilityFlag)
{
  // Render the generator's own bounds: labels outside them fall off every
  // tile and would make this pass for the wrong reason.
  const odb::Rect region = tile_gen_->getBounds();
  ASSERT_GT(region.maxDXDY(), 0);
  const Color white{.r = 255, .g = 255, .b = 255, .a = 255};

  TileVisibility vis;
  const std::vector<unsigned char> before
      = tile_gen_->renderImagePng(region, 512, 0, vis);
  ASSERT_FALSE(before.empty());

  const odb::Point centre((region.xMin() + region.xMax()) / 2,
                          (region.yMin() + region.yMax()) / 2);
  ASSERT_FALSE(
      tile_gen_->addLabel(centre, "PROBE", white, 24, "center", "L").empty());

  const std::vector<unsigned char> shown
      = tile_gen_->renderImagePng(region, 512, 0, vis);
  vis.labels = false;
  const std::vector<unsigned char> hidden
      = tile_gen_->renderImagePng(region, 512, 0, vis);

  // Drawn when on...
  EXPECT_NE(shown, before) << "label did not change the image";
  // ...and with it off the image is the one from before the label existed.
  EXPECT_EQ(hidden, before) << "label still drawn with labels off";
}

//------------------------------------------------------------------------------
// Debug graphics in save_image.  The layer loop carries only the per-layer
// Renderer::drawLayer half, so the layer-independent drawObjects pass needs a
// composite step of its own -- once per tile, where it used to be stamped once
// per visible layer.
//------------------------------------------------------------------------------

TEST_F(SaveImageTest, DebugRendererObjectsPassCompositedOncePerTile)
{
  int object_calls = 0;
  std::vector<std::string> layer_calls;
  TileGenerator::setRendererHooks({.draw = [&](std::vector<unsigned char>&,
                                               const TileFrame&,
                                               bool,
                                               odb::dbTechLayer* layer) {
    if (layer != nullptr) {
      layer_calls.emplace_back(layer->getName());
    } else {
      ++object_calls;
    }
  }});

  TileVisibility vis;
  vis.debug_renderers = true;
  vis.debug_live = true;
  // 256 px wide => a single tile, so the count is exactly the per-tile count.
  tile_gen_->renderImagePng(odb::Rect(0, 0, 0, 0), 256, 0, vis);
  TileGenerator::setRendererHooks({});

  EXPECT_EQ(object_calls, 1)
      << "drawObjects must be composited once, not once per rendered layer";
  EXPECT_GT(layer_calls.size(), 1u)
      << "the per-layer drawLayer pass still runs for every layer";
}

TEST_F(SaveImageTest, DebugRendererPassIsSkippedWhenToggledOff)
{
  int calls = 0;
  TileGenerator::setRendererHooks(
      {.draw = [&calls](std::vector<unsigned char>&,
                        const TileFrame&,
                        bool,
                        odb::dbTechLayer*) { ++calls; }});

  TileVisibility vis;  // debug_renderers defaults off
  tile_gen_->renderImagePng(odb::Rect(0, 0, 0, 0), 256, 0, vis);
  TileGenerator::setRendererHooks({});

  EXPECT_EQ(calls, 0);
}

}  // namespace
}  // namespace web
