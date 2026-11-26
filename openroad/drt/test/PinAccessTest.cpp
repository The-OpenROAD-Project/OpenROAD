// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "db/obj/frAccess.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "frDesign.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
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
                        "_main/openroad/drt/test/Nangate45/Nangate45_tech.lef");
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

class AccessPointObserver : public odb::dbBlockCallBackObj
{
 public:
  void inDbITermPostSetAccessPoints(odb::dbITerm* iterm) override
  {
    ++counts_[iterm->getInst()->getName()];
  }

  int count(const std::string& inst_name) const
  {
    const auto it = counts_.find(inst_name);
    return it != counts_.end() ? it->second : 0;
  }

  void reset() { counts_.clear(); }

 private:
  std::map<std::string, int> counts_;
};

class MasterSwapPinAccessTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = loadTechLef("ng45",
                        "_main/openroad/drt/test/Nangate45/Nangate45_tech.lef");
    ASSERT_NE(tech_, nullptr);
    lib_ = odb::dbLib::create(db_.get(), "stdcells", tech_, ',');
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 20000, 20000));
    block_->setMinRoutingLayer(2);
    block_->setMaxRoutingLayer(6);

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

    odb::dbTechLayer* lvt_implant
        = odb::dbTechLayer::create(tech_, "LVT", odb::dbTechLayerType::IMPLANT);
    odb::dbTechLayer* hvt_implant
        = odb::dbTechLayer::create(tech_, "HVT", odb::dbTechLayerType::IMPLANT);

    inv_lvt_ = makeInvMaster("INV_LVT",
                             1520,
                             odb::Rect(200, 200, 500, 1200),
                             odb::Rect(800, 200, 1100, 1200),
                             lvt_implant);
    inv_hvt_ = makeInvMaster("INV_HVT",
                             1520,
                             odb::Rect(200, 200, 500, 1200),
                             odb::Rect(800, 200, 1100, 1200),
                             hvt_implant);
    inv_shifted_ = makeInvMaster("INV_SHIFTED",
                                 1520,
                                 odb::Rect(200, 200, 500, 1200),
                                 odb::Rect(1100, 200, 1400, 1200),
                                 lvt_implant);
    inv_wide_ = makeInvMaster("INV_WIDE",
                              2280,
                              odb::Rect(200, 200, 500, 1200),
                              odb::Rect(800, 200, 1100, 1200),
                              lvt_implant);

    odb::dbNet::create(block_, "in");
    odb::dbNet::create(block_, "mid");
    odb::dbNet::create(block_, "out");
    makeBTerm(block_,
              "in",
              {.bpins = {{.layer_name = "metal2",
                          .rect = odb::Rect(0, 4000, 200, 4200),
                          .status = odb::dbPlacementStatus::FIRM}}});
    makeBTerm(block_,
              "out",
              {.bpins = {{.layer_name = "metal2",
                          .rect = odb::Rect(19800, 4000, 20000, 4200),
                          .status = odb::dbPlacementStatus::FIRM}}});

    u1_ = makeInst(block_,
                   inv_lvt_,
                   "u1",
                   {.location = odb::Point(3800, 2800),
                    .status = odb::dbPlacementStatus::PLACED,
                    .iterms = {{.net_name = "in", .term_name = "A"},
                               {.net_name = "mid", .term_name = "Y"}}});
    u2_ = makeInst(block_,
                   inv_lvt_,
                   "u2",
                   {.location = odb::Point(5320, 2800),
                    .status = odb::dbPlacementStatus::PLACED,
                    .iterms = {{.net_name = "mid", .term_name = "A"},
                               {.net_name = "out", .term_name = "Y"}}});
  }

  odb::dbMaster* makeInvMaster(const char* name,
                               const int width,
                               const odb::Rect& a_rect,
                               const odb::Rect& y_rect,
                               odb::dbTechLayer* implant)
  {
    odb::dbMaster* master = odb::dbMaster::create(lib_, name);
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(width);
    master->setHeight(2800);
    odb::dbTechLayer* m1 = tech_->findLayer("metal1");
    auto add_pin = [&](const char* pin_name,
                       odb::dbIoType io_type,
                       const odb::Rect& rect) {
      odb::dbMTerm* mterm = odb::dbMTerm::create(
          master, pin_name, io_type, odb::dbSigType::SIGNAL);
      odb::dbMPin* mpin = odb::dbMPin::create(mterm);
      odb::dbBox::create(
          mpin, m1, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
    };
    add_pin("A", odb::dbIoType::INPUT, a_rect);
    add_pin("Y", odb::dbIoType::OUTPUT, y_rect);
    if (implant != nullptr) {
      odb::dbBox::create(master, implant, 0, 0, width, 2800);
    }
    master->setFrozen();
    return master;
  }

  std::unique_ptr<TritonRoute> makeRouter()
  {
    registry_ = std::make_unique<utl::ServiceRegistry>(&logger_);
    dist_ = std::make_unique<dst::Distributed>(&logger_);
    auto router = std::make_unique<TritonRoute>(
        db_.get(), &logger_, registry_.get(), dist_.get(), nullptr);
    router->initGraphics(std::make_unique<NoGraphicsFactory>());
    ParamStruct params;
    params.verbose = 0;
    router->setParams(params);
    router->pinAccess();
    return router;
  }

  static const frAccessPoint* firstAccessPoint(TritonRoute* router,
                                               const char* inst_name,
                                               const char* term_name)
  {
    frInst* inst = router->getDesign()->getTopBlock()->getInst(inst_name);
    if (inst == nullptr) {
      return nullptr;
    }
    for (const auto& inst_term : inst->getInstTerms()) {
      if (inst_term->getTerm()->getName() == term_name) {
        const auto& aps = inst_term->getAccessPoints();
        if (!aps.empty()) {
          return aps.front();
        }
      }
    }
    return nullptr;
  }

  static odb::Point firstDbPrefAccessPoint(odb::dbInst* inst,
                                           const char* term_name)
  {
    odb::dbITerm* iterm = inst->findITerm(term_name);
    if (iterm == nullptr) {
      return odb::Point();
    }
    const auto aps = iterm->getPrefAccessPoints();
    return aps.empty() ? odb::Point() : aps.front()->getPoint();
  }

  odb::dbTech* tech_ = nullptr;
  odb::dbLib* lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::dbMaster* inv_lvt_ = nullptr;
  odb::dbMaster* inv_hvt_ = nullptr;
  odb::dbMaster* inv_shifted_ = nullptr;
  odb::dbMaster* inv_wide_ = nullptr;
  odb::dbInst* u1_ = nullptr;
  odb::dbInst* u2_ = nullptr;
  std::unique_ptr<utl::ServiceRegistry> registry_;
  std::unique_ptr<dst::Distributed> dist_;
};

TEST_F(MasterSwapPinAccessTest, SameGeometrySwapSkipsRowPatternAndPreservesAps)
{
  auto router = makeRouter();
  const frAccessPoint* orig_a = firstAccessPoint(router.get(), "u1", "A");
  const frAccessPoint* orig_y = firstAccessPoint(router.get(), "u1", "Y");
  ASSERT_NE(orig_a, nullptr);
  ASSERT_NE(orig_y, nullptr);
  const odb::Point orig_a_pt = orig_a->getPoint();
  const auto orig_a_layer = orig_a->getLayerNum();
  const odb::Point orig_y_pt = orig_y->getPoint();
  const auto orig_y_layer = orig_y->getLayerNum();
  const odb::Point orig_db_y = firstDbPrefAccessPoint(u1_, "Y");

  AccessPointObserver observer;
  observer.addOwner(block_);

  // Swap to a different geometry and back to same-geometry HVT before update:
  // saved access points from the first swap should still be restored and the
  // abutting neighbor u2 should not have its row pattern regenerated.
  u1_->swapMaster(inv_shifted_);
  u1_->swapMaster(inv_hvt_);
  router->updateDirtyPinAccess();

  const frAccessPoint* after_a = firstAccessPoint(router.get(), "u1", "A");
  const frAccessPoint* after_y = firstAccessPoint(router.get(), "u1", "Y");
  ASSERT_NE(after_a, nullptr);
  ASSERT_NE(after_y, nullptr);
  EXPECT_EQ(after_a->getPoint(), orig_a_pt);
  EXPECT_EQ(after_a->getLayerNum(), orig_a_layer);
  EXPECT_EQ(after_y->getPoint(), orig_y_pt);
  EXPECT_EQ(after_y->getLayerNum(), orig_y_layer);
  EXPECT_EQ(firstDbPrefAccessPoint(u1_, "Y"), orig_db_y);
  EXPECT_EQ(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, DifferentGeometrySwapFallsBackToRowPattern)
{
  auto router = makeRouter();
  const frAccessPoint* orig_y = firstAccessPoint(router.get(), "u1", "Y");
  ASSERT_NE(orig_y, nullptr);
  const odb::Point orig_y_pt = orig_y->getPoint();

  AccessPointObserver observer;
  observer.addOwner(block_);

  u1_->swapMaster(inv_shifted_);
  router->updateDirtyPinAccess();

  const frAccessPoint* after_y = firstAccessPoint(router.get(), "u1", "Y");
  ASSERT_NE(after_y, nullptr);
  EXPECT_NE(after_y->getPoint(), orig_y_pt);
  EXPECT_GE(after_y->getPoint().x(), 1100);
  EXPECT_LE(after_y->getPoint().x(), 1400);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, WiderMasterSwapFallsBackToRowPattern)
{
  // Place a cell earlier in insts_set_ so u1 is not insts_set_.begin() when
  // getAdjacentInstancesCluster(u2) walks left to its abutting neighbor u1.
  makeInst(block_,
           inv_lvt_,
           "u0",
           {.location = odb::Point(2280, 2800),
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "in", .term_name = "A"},
                       {.net_name = "mid", .term_name = "Y"}}});
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  // Swap u2 in place (unchanged transform) to wider INV_WIDE while abutting
  // u1 on the left so hasSameMasterGeometry() — not transform mismatch —
  // triggers fallback to row-pattern generation.
  u2_->swapMaster(inv_wide_);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u2", "Y"), nullptr);
  EXPECT_GT(observer.count("u1"), 0);
}

TEST_F(MasterSwapPinAccessTest, SwapWithNetChangeFallsBackToRowPattern)
{
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  // Same-geometry swap combined with rewiring Y to a different net before
  // updateDirtyPinAccess() must fall back to row-pattern generation.
  odb::dbNet* alt_net = odb::dbNet::create(block_, "alt_out");
  u1_->swapMaster(inv_hvt_);
  u1_->findITerm("Y")->connect(alt_net);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, RewireThenSwapFallsBackToRowPattern)
{
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  // Rewiring Y before swapping master already marks u1 dirty; the subsequent
  // swap must not snapshot the post-rewire net and bypass row-pattern updates.
  odb::dbNet* alt_net = odb::dbNet::create(block_, "alt_out");
  u1_->findITerm("Y")->connect(alt_net);
  u1_->swapMaster(inv_hvt_);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, ObsMismatchAtSameTransformFallsBackToRowPattern)
{
  odb::dbMaster* inv_obs = makeInvMaster("INV_OBS",
                                         1520,
                                         odb::Rect(200, 200, 500, 1200),
                                         odb::Rect(800, 200, 1100, 1200),
                                         tech_->findLayer("HVT"));
  odb::dbBox::create(inv_obs, tech_->findLayer("metal1"), 600, 200, 700, 1200);

  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  // INV_OBS has identical size and pin shapes to INV_LVT at the same
  // transform, differing only by a metal1 obstruction.
  u1_->swapMaster(inv_obs);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, MoveThenSwapFallsBackToRowPattern)
{
  // Place u1 away from u2 initially, compute PA, then move u1 to abut u2 on
  // the left (x=3800) and VT-swap before updateDirtyPinAccess(). Moving u1
  // marks it dirty so saveSwappedInst() skips snapshotting and row pattern
  // generation runs.
  u1_->setLocation(7600, 2800);
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  u1_->setLocation(3800, 2800);
  u1_->swapMaster(inv_hvt_);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u1", "A"), nullptr);
  EXPECT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, SwapThenMoveFallsBackToRowPattern)
{
  // Swap u1 while away from u2 (saving its PA snapshot at x=7600), then move
  // u1 to abut u2 at x=3800 before updateDirtyPinAccess(). The transform
  // mismatch guard in restoreSwappedInst() must force row-pattern generation.
  u1_->setLocation(7600, 2800);
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  u1_->swapMaster(inv_hvt_);
  u1_->setLocation(3800, 2800);
  router->updateDirtyPinAccess();

  EXPECT_NE(firstAccessPoint(router.get(), "u1", "A"), nullptr);
  EXPECT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest,
       SameGeometrySwapReusesExistingUniqueClassAndConsecutiveBatch)
{
  // Instantiate INV_HVT upfront so its UniqueClass already exists before u1 is
  // swapped, then run two consecutive swap + updateDirtyPinAccess() batches to
  // verify both UniqueClass reuse and setLatestPATransform() persistence.
  makeInst(block_,
           inv_hvt_,
           "u_hvt",
           {.location = odb::Point(9120, 2800),
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "in", .term_name = "A"},
                       {.net_name = "out", .term_name = "Y"}}});
  auto router = makeRouter();
  const frAccessPoint* orig_y = firstAccessPoint(router.get(), "u1", "Y");
  ASSERT_NE(orig_y, nullptr);
  const odb::Point orig_y_pt = orig_y->getPoint();

  AccessPointObserver observer;
  observer.addOwner(block_);

  u1_->swapMaster(inv_hvt_);
  router->updateDirtyPinAccess();
  ASSERT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_EQ(firstAccessPoint(router.get(), "u1", "Y")->getPoint(), orig_y_pt);
  EXPECT_EQ(observer.count("u2"), 0);

  u1_->swapMaster(inv_lvt_);
  router->updateDirtyPinAccess();
  ASSERT_NE(firstAccessPoint(router.get(), "u1", "Y"), nullptr);
  EXPECT_EQ(firstAccessPoint(router.get(), "u1", "Y")->getPoint(), orig_y_pt);
  EXPECT_EQ(observer.count("u2"), 0);
}

TEST_F(MasterSwapPinAccessTest, DestroyAfterSwapClearsSavedState)
{
  auto router = makeRouter();
  AccessPointObserver observer;
  observer.addOwner(block_);

  u1_->swapMaster(inv_hvt_);
  odb::dbInst::destroy(u1_);
  // Recreate a same-geometry instance at u1's old location (reusing the freed
  // dbInst slot). Destroy cleanup must prevent reusing the destroyed inst's
  // saved access points.
  makeInst(block_,
           inv_hvt_,
           "u1_new",
           {.location = odb::Point(3800, 2800),
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "in", .term_name = "A"},
                       {.net_name = "mid", .term_name = "Y"}}});
  router->updateDirtyPinAccess();

  EXPECT_EQ(router->getDesign()->getTopBlock()->getInst("u1"), nullptr);
  EXPECT_NE(firstAccessPoint(router.get(), "u1_new", "A"), nullptr);
  EXPECT_GT(observer.count("u2"), 0);
}

}  // namespace
}  // namespace drt
