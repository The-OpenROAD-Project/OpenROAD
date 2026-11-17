// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <ostream>
#include <string>
#include <vector>

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

struct PinAccessCase
{
  const char* name;
  const char* pin_layer;
  int max_routing_layer;
  bool as_bterm;
  const char* ap_layer;
  bool planar;
  bool up;
  bool down;
};

void PrintTo(const PinAccessCase& test, std::ostream* os)
{
  *os << test.name;
}

class BumpPinAccessTest : public tst::DbFixture,
                          public testing::WithParamInterface<PinAccessCase>
{
 protected:
  void SetUp() override
  {
    tech_ = loadTechLef("ng45",
                        "_main/src/drt/test/Nangate45/Nangate45_tech.lef");
    ASSERT_NE(tech_, nullptr);
    lib_ = odb::dbLib::create(db_.get(), "bumps", tech_, ',');
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 80000, 80000));
    makeTracks();
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

  odb::dbMaster* makeBumpMaster(const char* pin_layer)
  {
    odb::dbMaster* master = odb::dbMaster::create(lib_, "BUMP");
    master->setType(odb::dbMasterType::COVER_BUMP);
    master->setWidth(kBumpSize);
    master->setHeight(kBumpSize);
    odb::dbMTerm* mterm = odb::dbMTerm::create(
        master, "PAD", odb::dbIoType::INOUT, odb::dbSigType::SIGNAL);
    odb::dbMPin* mpin = odb::dbMPin::create(mterm);
    odb::dbBox::create(
        mpin, tech_->findLayer(pin_layer), 0, 0, kBumpSize, kBumpSize);
    master->setFrozen();
    return master;
  }

  odb::dbBTerm* makeBumpBTerm(const char* pin_layer, odb::dbNet* net)
  {
    odb::dbBTerm* bump = odb::dbBTerm::create(net, "bump");
    bump->setIoType(odb::dbIoType::INOUT);
    odb::dbBPin* bpin = odb::dbBPin::create(bump);
    odb::dbBox::create(bpin,
                       tech_->findLayer(pin_layer),
                       kBumpX,
                       kBumpY,
                       kBumpX + kBumpSize,
                       kBumpY + kBumpSize);
    bpin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    return bump;
  }

  std::vector<odb::dbAccessPoint*> bumpAccessPoints(const PinAccessCase& test)
  {
    odb::dbBTerm* pin
        = makeBTerm(block_,
                    "a",
                    {.bpins = {{.layer_name = "metal2",
                                .rect = odb::Rect(9800, 39800, 10200, 40200),
                                .status = odb::dbPlacementStatus::FIRM}}});
    odb::dbMaster* master = nullptr;
    odb::dbInst* bump_inst = nullptr;
    odb::dbBTerm* bump_bterm = nullptr;
    if (test.as_bterm) {
      bump_bterm = makeBumpBTerm(test.pin_layer, pin->getNet());
    } else {
      master = makeBumpMaster(test.pin_layer);
      bump_inst = makeInst(block_,
                           master,
                           "bump",
                           {.location = odb::Point(kBumpX, kBumpY),
                            .status = odb::dbPlacementStatus::FIRM,
                            .iterms = {{.net_name = "a", .term_name = "PAD"}}});
    }
    block_->setMinRoutingLayer(2);
    block_->setMaxRoutingLayer(test.max_routing_layer);

    utl::ServiceRegistry registry(&logger_);
    dst::Distributed dist(&logger_);
    TritonRoute router(db_.get(), &logger_, &registry, &dist, nullptr);
    router.initGraphics(std::make_unique<NoGraphicsFactory>());
    ParamStruct params;
    params.verbose = 0;
    router.setParams(params);
    router.pinAccess();

    std::vector<odb::dbAccessPoint*> aps;
    if (bump_bterm) {
      for (odb::dbBPin* bpin : bump_bterm->getBPins()) {
        const std::vector<odb::dbAccessPoint*> bpin_aps
            = bpin->getAccessPoints();
        aps.insert(aps.end(), bpin_aps.begin(), bpin_aps.end());
      }
      return aps;
    }
    for (odb::dbMPin* mpin : master->findMTerm("PAD")->getMPins()) {
      const auto pin_access = mpin->getPinAccess();
      const int idx = bump_inst->getPinAccessIdx();
      if (idx < 0 || idx >= static_cast<int>(pin_access.size())) {
        continue;
      }
      aps.insert(aps.end(), pin_access[idx].begin(), pin_access[idx].end());
    }
    return aps;
  }

  static constexpr int kBumpSize = 10000;
  static constexpr int kBumpX = 50000;
  static constexpr int kBumpY = 30000;

  odb::dbTech* tech_ = nullptr;
  odb::dbLib* lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

TEST_P(BumpPinAccessTest, AccessFollowsLayerRoutability)
{
  const PinAccessCase& test = GetParam();
  const std::vector<odb::dbAccessPoint*> aps = bumpAccessPoints(test);
  ASSERT_FALSE(aps.empty());

  bool planar = false;
  bool up = false;
  bool down = false;
  for (odb::dbAccessPoint* ap : aps) {
    planar |= ap->hasAccess(odb::dbDirection::NORTH)
              || ap->hasAccess(odb::dbDirection::SOUTH)
              || ap->hasAccess(odb::dbDirection::EAST)
              || ap->hasAccess(odb::dbDirection::WEST);
    up |= ap->hasAccess(odb::dbDirection::UP);
    down |= ap->hasAccess(odb::dbDirection::DOWN);
    EXPECT_EQ(ap->getLayer()->getName(), test.ap_layer);
  }
  EXPECT_EQ(planar, test.planar);
  EXPECT_EQ(up, test.up);
  EXPECT_EQ(down, test.down);
}

const PinAccessCase kCases[] = {
    {"TopLayer", "metal10", 10, false, "metal10", true, false, true},
    {"AboveTopRoutingLayer", "metal10", 9, true, "metal9", true, false, false},
    {"FarAboveTopRoutingLayer",
     "metal10",
     8,
     true,
     "metal8",
     true,
     false,
     false},
    {"MiddleLayer", "metal6", 10, false, "metal6", true, true, true},
    {"BottomLayer", "metal2", 10, false, "metal2", true, true, false},
};

INSTANTIATE_TEST_SUITE_P(LayerRoutability,
                         BumpPinAccessTest,
                         testing::ValuesIn(kCases),
                         [](const testing::TestParamInfo<PinAccessCase>& info) {
                           return std::string(info.param.name);
                         });

}  // namespace
}  // namespace drt
