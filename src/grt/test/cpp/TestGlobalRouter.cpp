// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "tst/IntegratedFixture.h"

namespace grt {
namespace {

class GlobalRouterTest : public tst::IntegratedFixture
{
 protected:
  GlobalRouterTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/grt/test/")
  {
  }

  void SetUp() override
  {
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::defin def_reader(db_.get(), &logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    const std::string def = getFilePath("_main/src/grt/test/gcd.def");
    def_reader.readChip(search_libs, def.c_str(), chip);
    block_ = db_->getChip()->getBlock();
    sta_->postReadDef(block_);

    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(10);
  }
};

TEST_F(GlobalRouterTest, DesignIsPlacedChecksSignalNetPins)
{
  EXPECT_TRUE(grt_.designIsPlaced());

  // A signal-net BTerm with no dbBPin (placement status NONE) must report the
  // design as unplaced even before FastRoute initializes db_net_map_.
  odb::dbNet* signal_net = block_->findNet("req_val");
  ASSERT_NE(signal_net, nullptr);
  odb::dbBTerm* bterm = odb::dbBTerm::create(signal_net, "unplaced_port");
  ASSERT_NE(bterm, nullptr);
  EXPECT_FALSE(grt_.designIsPlaced());

  // A dbBPin with UNPLACED status is still unplaced.
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  odb::dbTechLayer* metal3 = db_->getTech()->findLayer("metal3");
  ASSERT_NE(metal3, nullptr);
  odb::dbBox::create(bpin, metal3, 0, 0, 100, 100);
  bpin->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);
  EXPECT_FALSE(grt_.designIsPlaced());

  // Placing the pin restores designIsPlaced().
  bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  EXPECT_TRUE(grt_.designIsPlaced());

  // Unplaced BTerms on special supply nets are ignored.
  odb::dbNet* power_net = odb::dbNet::create(block_, "VDD_PORT_NET");
  power_net->setSpecial();
  power_net->setSigType(odb::dbSigType::POWER);
  odb::dbBTerm::create(power_net, "VDD_PORT");
  EXPECT_TRUE(grt_.designIsPlaced());

  // Unconnected BTerms (not attached to any net, skipped by IO placement) are
  // ignored.
  odb::dbNet* temp_net = odb::dbNet::create(block_, "DISCONNECTED_PORT_NET");
  odb::dbBTerm* disconnected_bterm
      = odb::dbBTerm::create(temp_net, "DISCONNECTED_PORT");
  disconnected_bterm->disconnect();
  ASSERT_EQ(disconnected_bterm->getNet(), nullptr);
  EXPECT_TRUE(grt_.designIsPlaced());
}

TEST_F(GlobalRouterTest, RudySkipsUnplacedPinsWhenPinPlacementCheckDisabled)
{
  // BTerm without any dbBPin geometries (previously triggered GRT-0042 because
  // initNets() called findNets() before updating check_pin_placement_).
  odb::dbNet* req_val = block_->findNet("req_val");
  ASSERT_NE(req_val, nullptr);
  odb::dbBTerm::create(req_val, "port_without_bpin");

  // BTerm with an UNPLACED dbBPin (previously triggered GRT-0011).
  odb::dbNet* resp_val = block_->findNet("resp_val");
  ASSERT_NE(resp_val, nullptr);
  odb::dbBTerm* unplaced_bterm
      = odb::dbBTerm::create(resp_val, "port_with_unplaced_bpin");
  odb::dbBPin* bpin = odb::dbBPin::create(unplaced_bterm);
  odb::dbTechLayer* metal3 = db_->getTech()->findLayer("metal3");
  ASSERT_NE(metal3, nullptr);
  odb::dbBox::create(bpin, metal3, 0, 0, 100, 100);
  bpin->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);

  grt::Rudy* rudy = nullptr;
  ASSERT_NO_THROW(rudy = grt_.getRudy());
  ASSERT_NE(rudy, nullptr);
  EXPECT_NO_THROW(rudy->calculateRudy());
  const auto [grid_x, grid_y] = rudy->getGridSize();
  EXPECT_GT(grid_x, 0);
  EXPECT_GT(grid_y, 0);

  // Full global routing still enforces pin placement checks.
  EXPECT_THROW(grt_.globalRoute(), std::runtime_error);
}

// RUDY demand a two-pin net should deposit in one tile: the net needs
// hpwl * wire_width of wire area, spread uniformly over its terminal bounding
// box, and the tile gets the share of that box it covers, relative to the
// tile's own area. The share is an area ratio for a box with area, and a
// length ratio for a box that is a line (terminals aligned on x or y).
float expectedDemand(const odb::Rect& net_box,
                     const odb::Rect& tile,
                     const int wire_width)
{
  if (!net_box.overlaps(tile)) {
    return 0;
  }
  const odb::Rect inside = net_box.intersect(tile);
  const float hpwl = net_box.dx() + net_box.dy();
  const float share = net_box.area() > 0
                          ? static_cast<float>(inside.area()) / net_box.area()
                          : (inside.dx() + inside.dy()) / hpwl;
  return hpwl * wire_width * share / tile.area() * 100;
}

TEST_F(GlobalRouterTest, RudyDepositsWireAreaPerTile)
{
  constexpr int kWireWidth = 100;
  grt::Rudy* rudy = grt_.getRudy();
  rudy->setWireWidth(kWireWidth);
  odb::dbTechLayer* metal3 = db_->getTech()->findLayer("metal3");
  ASSERT_NE(metal3, nullptr);
  const auto [grid_x, grid_y] = rudy->getGridSize();
  ASSERT_GE(grid_x, 8);
  ASSERT_GE(grid_y, 8);

  // Tile centers, so a line net lies strictly inside the tiles it crosses.
  auto center = [&](int x, int y) {
    const odb::Rect tile = rudy->getTile(x, y).getRect();
    return odb::Point(tile.xCenter(), tile.yCenter());
  };
  auto make_net = [&](const std::string& name, odb::Point a, odb::Point b) {
    odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
    for (const odb::Point& p : {a, b}) {
      const std::string pin_name
          = name + "_" + std::to_string(p.x()) + "_" + std::to_string(p.y());
      odb::dbBTerm* bterm = odb::dbBTerm::create(net, pin_name.c_str());
      EXPECT_NE(bterm, nullptr);
      odb::dbBPin* bpin = odb::dbBPin::create(bterm);
      odb::dbBox::create(
          bpin, metal3, p.x() - 100, p.y() - 100, p.x() + 100, p.y() + 100);
      bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    return net;
  };

  // Interior tiles only: the last row and column absorb the die remainder, so
  // every tile the nets touch has the same area as tile (0, 0).
  const struct
  {
    const char* name;
    odb::Point a;
    odb::Point b;
  } cases[] = {
      {"horizontal_line", center(1, 2), center(6, 2)},
      {"vertical_line", center(3, 1), center(3, 6)},
      {"rectangle", center(1, 1), center(5, 4)},
  };
  const float tile_area = rudy->getTile(0, 0).getRect().area();
  for (const auto& [name, a, b] : cases) {
    SCOPED_TRACE(name);
    odb::dbNet* net = make_net(name, a, b);
    const odb::Rect net_box = net->getTermBBox();
    odb::PtrSet<odb::dbNet> selection{net};
    rudy->calculateRudy(&selection);

    float total = 0;
    int touched = 0;
    for (int x = 0; x < grid_x; x++) {
      for (int y = 0; y < grid_y; y++) {
        const odb::Rect tile = rudy->getTile(x, y).getRect();
        const float demand = rudy->getTile(x, y).getDemand();
        EXPECT_FLOAT_EQ(demand, expectedDemand(net_box, tile, kWireWidth))
            << "tile (" << x << ", " << y << ")";
        total += demand;
        touched += demand > 0;
      }
    }
    // The net spans several tiles, and its wire area adds up to the whole
    // hpwl * wire_width whether the box is a line or a rectangle.
    EXPECT_GT(touched, 1);
    const float hpwl = net_box.dx() + net_box.dy();
    EXPECT_NEAR(total, hpwl * kWireWidth / tile_area * 100, total * 1e-5);
  }
}

TEST_F(GlobalRouterTest, RudyLineNetIsTheLimitOfAThinRectangle)
{
  grt::Rudy* rudy = grt_.getRudy();
  odb::dbTechLayer* metal3 = db_->getTech()->findLayer("metal3");
  ASSERT_NE(metal3, nullptr);

  // Total RUDY demand of a two-pin net from (20000, 50000) to
  // (180000, 50000 + dy).
  auto total_demand = [&](const std::string& name, int dy) {
    odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
    const std::pair<int, int> pins[] = {{20000, 50000}, {180000, 50000 + dy}};
    for (const auto& [x, y] : pins) {
      odb::dbBTerm* bterm
          = odb::dbBTerm::create(net, (name + "_" + std::to_string(x)).c_str());
      odb::dbBPin* bpin = odb::dbBPin::create(bterm);
      odb::dbBox::create(bpin, metal3, x - 100, y - 100, x + 100, y + 100);
      bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    odb::PtrSet<odb::dbNet> selection{net};
    rudy->calculateRudy(&selection);
    const auto [grid_x, grid_y] = rudy->getGridSize();
    float demand = 0;
    for (int x = 0; x < grid_x; x++) {
      for (int y = 0; y < grid_y; y++) {
        demand += rudy->getTile(x, y).getDemand();
      }
    }
    return demand;
  };

  const float aligned = total_demand("aligned", 0);
  const float almost_aligned = total_demand("almost_aligned", 1);
  EXPECT_GT(aligned, 0);
  // A 1 DBU tall box over a 160000 DBU run is the line up to 1/160000.
  EXPECT_NEAR(aligned, almost_aligned, almost_aligned * 1e-4);
}

}  // namespace
}  // namespace grt
