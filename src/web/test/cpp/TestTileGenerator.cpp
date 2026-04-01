// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "lodepng.h"
#include "odb/db.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

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

  std::unique_ptr<TileGenerator> tile_gen_;
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

TEST_F(TileGeneratorTest, GetLayers)
{
  makeTileGen();
  std::vector<std::string> layers = tile_gen_->getLayers();
  // 10 routing + 9 cut layers
  EXPECT_EQ(layers.size(), 19);
  EXPECT_EQ(layers.front(), "metal1");
  EXPECT_EQ(layers.back(), "metal10");
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
  EXPECT_TRUE(vis.blockages);
  EXPECT_TRUE(vis.net_signal);
  EXPECT_TRUE(vis.net_power);
  EXPECT_TRUE(vis.net_ground);
  EXPECT_TRUE(vis.net_clock);
  EXPECT_TRUE(vis.phys_fill);
  EXPECT_TRUE(vis.phys_endcap);
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
  vis.parseFromJson(
      "{\"rows\":1,\"stdcells\":0,"
      "\"site_FreePDK45_38x28_10R_NP_162NW_34O\":1}");

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
  vis.parseFromJson("{\"rows\":1,\"stdcells\":0}");

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
  vis.parseFromJson(
      "{\"rows\":1,\"stdcells\":0,"
      "\"site_FreePDK45_38x28_10R_NP_162NW_34O\":1}");

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

}  // namespace
}  // namespace web
