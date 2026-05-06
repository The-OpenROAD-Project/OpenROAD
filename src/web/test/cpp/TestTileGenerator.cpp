// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "color.h"
#include "gtest/gtest.h"
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
// web frontend show the same color for the same layer.  Spot-check the first
// few entries of each palette and exercise the type-based assignment rules.
TEST_F(TileGeneratorTest, GetLayerColorMapMatchesGuiPalette)
{
  makeTileGen();
  const auto& colors = tile_gen_->getLayerColorMap();

  odb::dbTech* tech = getDb()->getTech();
  ASSERT_NE(tech, nullptr);

  odb::dbTechLayer* metal1 = tech->findLayer("metal1");
  odb::dbTechLayer* metal2 = tech->findLayer("metal2");
  odb::dbTechLayer* metal3 = tech->findLayer("metal3");
  odb::dbTechLayer* via1 = tech->findLayer("via1");
  odb::dbTechLayer* via2 = tech->findLayer("via2");
  ASSERT_NE(metal1, nullptr);
  ASSERT_NE(metal2, nullptr);
  ASSERT_NE(metal3, nullptr);
  ASSERT_NE(via1, nullptr);
  ASSERT_NE(via2, nullptr);

  // First three metals match the GUI's seeded #00F, #F00, #0D0 entries.
  const Color m1 = colors.at(metal1);
  EXPECT_EQ(m1.r, 0);
  EXPECT_EQ(m1.g, 0);
  EXPECT_EQ(m1.b, 254);
  EXPECT_EQ(m1.a, 180);

  const Color m2 = colors.at(metal2);
  EXPECT_EQ(m2.r, 254);
  EXPECT_EQ(m2.g, 0);
  EXPECT_EQ(m2.b, 0);

  const Color m3 = colors.at(metal3);
  EXPECT_EQ(m3.r, 9);
  EXPECT_EQ(m3.g, 221);
  EXPECT_EQ(m3.b, 0);

  // First two cuts match the GUI's cut palette (light blue, light red).
  const Color v1 = colors.at(via1);
  EXPECT_EQ(v1.r, 126);
  EXPECT_EQ(v1.g, 126);
  EXPECT_EQ(v1.b, 255);
  EXPECT_EQ(v1.a, 180);

  const Color v2 = colors.at(via2);
  EXPECT_EQ(v2.r, 255);
  EXPECT_EQ(v2.g, 126);
  EXPECT_EQ(v2.b, 126);
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

}  // namespace
}  // namespace web
