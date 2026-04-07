// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
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
    std::string path = std::filesystem::temp_directory_path()
                       / ("web_test_" + label + ".png");
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

TEST_F(SaveImageTest, VisibilityPinMarkersOff)
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
  vis_off.pin_markers = false;
  tile_gen_->saveImage(path_off, odb::Rect(0, 0, 0, 0), 512, 0, vis_off);

  unsigned w1 = 0, h1 = 0, w2 = 0, h2 = 0;
  auto pixels_on = decodePngFile(path_on, w1, h1);
  auto pixels_off = decodePngFile(path_off, w2, h2);

  // With pin_markers on, there should be visible content from the marker.
  // With it off, less or no content.
  EXPECT_TRUE(hasNonTransparentPixel(pixels_on));
  // The two images should differ.
  EXPECT_NE(pixels_on, pixels_off);
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

}  // namespace
}  // namespace web
