// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ant/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/ServiceRegistry.h"

namespace grt {
namespace {

class ChipBumpPortRouteTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib = loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbTech* tech = lib->getTech();
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 80000, 80000));
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, layer);
      const int pitch_x = layer->getPitchX();
      const int pitch_y = layer->getPitchY();
      grid->addGridPatternX(pitch_x / 2, 80000 / pitch_x, pitch_x);
      grid->addGridPatternY(pitch_y / 2, 80000 / pitch_y, pitch_y);
    }

    odb::dbMaster* master = odb::dbMaster::create(lib, "BUMP");
    master->setType(odb::dbMasterType::COVER_BUMP);
    master->setWidth(kBumpSize);
    master->setHeight(kBumpSize);
    odb::dbMTerm* mterm = odb::dbMTerm::create(
        master, "PAD", odb::dbIoType::INOUT, odb::dbSigType::SIGNAL);
    odb::dbBox::create(odb::dbMPin::create(mterm),
                       tech->findLayer("metal10"),
                       0,
                       0,
                       kBumpSize,
                       kBumpSize);
    master->setFrozen();
    bump_inst_ = makeInst(block_,
                          master,
                          "bump",
                          {.location = odb::Point(50000, 30000),
                           .status = odb::dbPlacementStatus::FIRM,
                           .iterms = {{.net_name = "a", .term_name = "PAD"}}});
    makeBTerm(block_,
              "a",
              {.bpins = {{.layer_name = "metal2",
                          .rect = odb::Rect(9800, 39800, 10200, 40200),
                          .status = odb::dbPlacementStatus::FIRM}}});
    port_ = odb::dbBTerm::create(block_->findNet("a"), "a_bump");
    region_ = odb::dbChipRegion::create(chip,
                                        "bumps",
                                        odb::dbChipRegion::Side::FRONT,
                                        tech->findLayer("metal10"));
    sta_->postReadDef(block_);

    router_.setMinRoutingLayer(2);
    router_.setMaxRoutingLayer(10);
  }

  static constexpr int kBumpSize = 10000;

  odb::dbBlock* block_ = nullptr;
  odb::dbInst* bump_inst_ = nullptr;
  odb::dbBTerm* port_ = nullptr;
  odb::dbChipRegion* region_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_{&logger_};
  ant::AntennaChecker ant_{db_.get(), &logger_};
  dpl::Opendp dpl_{db_.get(), &logger_};
  GlobalRouter
      router_{&logger_, &registry_, &stt_, db_.get(), sta_.get(), &ant_, &dpl_};
};

TEST_F(ChipBumpPortRouteTest, ShapelessChipBumpPortIsRouted)
{
  odb::dbChipBump::create(region_, bump_inst_)->setBTerm(port_);
  ASSERT_TRUE(port_->isShapelessChipBumpPort());
  EXPECT_NO_THROW(router_.globalRoute(/*save_guides=*/true));
  EXPECT_FALSE(block_->findNet("a")->getGuides().empty());
}

TEST_F(ChipBumpPortRouteTest, DanglingBumpPinNetNeedsNoGuides)
{
  odb::dbChipBump::create(region_, bump_inst_)->setBTerm(port_);
  odb::dbInst* bump2
      = makeInst(block_,
                 bump_inst_->getMaster(),
                 "bump2",
                 {.location = odb::Point(20000, 60000),
                  .status = odb::dbPlacementStatus::FIRM,
                  .iterms = {{.net_name = "b", .term_name = "PAD"}}});
  odb::dbBTerm* port_b = odb::dbBTerm::create(block_->findNet("b"), "b_bump");
  odb::dbChipBump::create(region_, bump2)->setBTerm(port_b);
  ASSERT_TRUE(port_b->isShapelessChipBumpPort());

  router_.globalRoute(/*save_guides=*/true);
  EXPECT_FALSE(block_->findNet("a")->getGuides().empty());
  EXPECT_TRUE(block_->findNet("b")->getGuides().empty());

  EXPECT_NO_THROW(ant_.check());

  router_.clear();
  router_.loadGuidesFromDB();
  EXPECT_FALSE(router_.getRoutes().empty());
}

TEST_F(ChipBumpPortRouteTest, PinlessPortWithoutChipBumpFails)
{
  ASSERT_FALSE(port_->isShapelessChipBumpPort());
  EXPECT_ANY_THROW(router_.globalRoute(/*save_guides=*/true));
}

}  // namespace
}  // namespace grt
