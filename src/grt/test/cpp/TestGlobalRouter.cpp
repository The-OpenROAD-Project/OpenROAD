// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdexcept>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
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

}  // namespace
}  // namespace grt
