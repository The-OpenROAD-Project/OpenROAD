// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "boost/json/parse.hpp"
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
  // With stdcells hidden and no routing, the _instances layer should be empty.
  tile_gen_->saveImage(path, odb::Rect(0, 0, 0, 0), 256, 0, vis);

  unsigned w = 0, h = 0;
  auto pixels = decodePngFile(path, w, h);
  EXPECT_FALSE(hasNonTransparentPixel(pixels));
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
  // Empty design should produce a transparent image.
  EXPECT_FALSE(hasNonTransparentPixel(pixels));
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

class TestRUDYHeatMap : public gui::HeatMapDataSource
{
 public:
  explicit TestRUDYHeatMap(utl::Logger* logger)
      : gui::HeatMapDataSource(logger,
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
  gui::registerHeatMapSource(
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

}  // namespace
}  // namespace web
