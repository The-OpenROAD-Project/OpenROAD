// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "AbstractGraphicsFactory.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "utl/ServiceRegistry.h"

namespace drt {
namespace {

class NoGraphicsFactory : public AbstractGraphicsFactory
{
 public:
  void reset(frDebugSettings* settings,
             frDesign* design,
             odb::dbDatabase* db,
             utl::Logger* logger,
             RouterConfiguration* router_cfg) override
  {
  }
  bool guiActive() override { return false; }
  std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() override
  {
    return nullptr;
  }
};

class ChipBumpPortTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = loadTechLef("ng45",
                        "_main/src/drt/test/Nangate45/Nangate45_tech.lef");
    ASSERT_NE(tech_, nullptr);
    odb::dbLib* lib = odb::dbLib::create(db_.get(), "bumps", tech_, ',');
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 80000, 80000));
    block_->setMinRoutingLayer(2);
    block_->setMaxRoutingLayer(10);
    makeTracks();

    odb::dbMaster* master = odb::dbMaster::create(lib, "BUMP");
    master->setType(odb::dbMasterType::COVER_BUMP);
    master->setWidth(kBumpSize);
    master->setHeight(kBumpSize);
    odb::dbMTerm* mterm = odb::dbMTerm::create(
        master, "PAD", odb::dbIoType::INOUT, odb::dbSigType::SIGNAL);
    odb::dbBox::create(odb::dbMPin::create(mterm),
                       tech_->findLayer("metal10"),
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
  }

  void makeTracks()
  {
    const odb::Rect die = block_->getDieArea();
    for (odb::dbTechLayer* layer : tech_->getLayers()) {
      if (layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, layer);
      const int pitch_x = layer->getPitchX();
      const int pitch_y = layer->getPitchY();
      grid->addGridPatternX(pitch_x / 2, die.dx() / pitch_x, pitch_x);
      grid->addGridPatternY(pitch_y / 2, die.dy() / pitch_y, pitch_y);
    }
  }

  void carryPortOnBump()
  {
    odb::dbChipRegion* region
        = odb::dbChipRegion::create(block_->getChip(),
                                    "bumps",
                                    odb::dbChipRegion::Side::FRONT,
                                    tech_->findLayer("metal10"));
    odb::dbChipBump::create(region, bump_inst_)->setBTerm(port_);
  }

  void pinAccess()
  {
    utl::ServiceRegistry registry(&logger_);
    dst::Distributed dist(&logger_);
    TritonRoute router(db_.get(), &logger_, &registry, &dist, nullptr);
    router.initGraphics(std::make_unique<NoGraphicsFactory>());
    ParamStruct params;
    params.verbose = 0;
    router.setParams(params);
    router.pinAccess();
  }

  static constexpr int kBumpSize = 10000;

  odb::dbTech* tech_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::dbInst* bump_inst_ = nullptr;
  odb::dbBTerm* port_ = nullptr;
};

TEST_F(ChipBumpPortTest, ShapelessChipBumpPortNeedsNoAccess)
{
  carryPortOnBump();
  ASSERT_TRUE(port_->isShapelessChipBumpPort());
  EXPECT_NO_THROW(pinAccess());
}

TEST_F(ChipBumpPortTest, PinlessPortWithoutChipBumpFails)
{
  ASSERT_FALSE(port_->isShapelessChipBumpPort());
  EXPECT_ANY_THROW(pinAccess());
}

}  // namespace
}  // namespace drt
