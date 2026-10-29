// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <functional>
#include <stdexcept>
#include <vector>

#include "dpl/Opendp.h"
#include "drt/PinAccessService.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "tst/nangate45_fixture.h"

namespace dpl {
namespace {

class RepairService : public drt::PinAccessService,
                      public odb::dbBlockCallBackObj
{
 public:
  bool hasPinAccess() const override { return ready; }
  bool isPinAccessRequired(odb::dbITerm* term) const override
  {
    return required && term != skipped_term;
  }
  void inDbPostMoveInst(odb::dbInst* inst) override
  {
    dirty |= recovering || inst == target;
    if (on_move) {
      on_move(inst);
    }
  }
  void updateDirtyPinAccess() override
  {
    if (!dirty) {
      return;
    }
    ++updates;
    if (refresh) {
      refresh();
    }
    dirty = false;
  }
  void beginPinAccessRecovery(int min_access_points = 1) override
  {
    minimum = min_access_points;
    ++begins;
    ready = false;
    recovering = true;
    dirty = false;
  }
  std::vector<odb::dbITerm*> getMissingPinAccess() const override
  {
    return missing_terms ? missing_terms() : std::vector<odb::dbITerm*>{};
  }
  void updatePinAccessRecovery() override
  {
    recovery_locations.push_back(target->getLocation());
    if (recovery_refresh) {
      recovery_refresh();
    }
    dirty = false;
  }
  void finishPinAccessRecovery() override
  {
    ++finishes;
    if (finish_failure) {
      throw std::runtime_error("recovery pattern failure");
    }
    recovering = false;
    ready = true;
  }
  void cancelPinAccessRecovery() override
  {
    ++cancels;
    recovering = false;
    ready = false;
    dirty = false;
  }
  odb::dbInst* target = nullptr;
  odb::dbITerm* skipped_term = nullptr;
  bool dirty = false;
  bool ready = true;
  bool required = true;
  int updates = 0;
  std::function<void()> refresh;
  std::function<void(odb::dbInst*)> on_move;
  bool recovering = false;
  bool finish_failure = false;
  int begins = 0;
  int minimum = 1;
  int finishes = 0;
  int cancels = 0;
  std::vector<odb::Point> recovery_locations;
  std::function<std::vector<odb::dbITerm*>()> missing_terms;
  std::function<void()> recovery_refresh;
};

class PinAccessRepairTest : public tst::Nangate45Fixture
{
 protected:
  static constexpr int kSite = 380;
  static constexpr int kHeight = 2800;
  static constexpr int kX = 10 * kSite;

  void SetUp() override
  {
    odb::dbTech* tech = db_->getTech();
    bottom_ = odb::dbTechLayer::create(
        tech, "repair_bottom", odb::dbTechLayerType::ROUTING);
    cut_ = odb::dbTechLayer::create(
        tech, "repair_cut", odb::dbTechLayerType::CUT);
    top_ = odb::dbTechLayer::create(
        tech, "repair_top", odb::dbTechLayerType::ROUTING);
    for (auto layer : {bottom_, cut_, top_}) {
      layer->setSpacing(0);
    }
    via_ = odb::dbTechVia::create(tech, "repair_via");
    for (auto layer : {bottom_, cut_, top_}) {
      odb::dbBox::create(via_, layer, -50, -50, 50, 50);
    }
    auto site = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site, nullptr);
    block_->setDieArea({0, 0, 32 * kSite, kHeight});
    odb::dbRow::create(block_,
                       "ROW",
                       site,
                       0,
                       0,
                       odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       32,
                       kSite);
    block_->setCoreArea(block_->computeCoreArea());
    auto master = odb::dbMaster::create(lib_, "REPAIR_CELL");
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(kSite);
    master->setHeight(kHeight);
    master->setSite(site);
    master->setSymmetryY();
    auto term = odb::dbMTerm::create(
        master, "A", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
    pin_ = odb::dbMPin::create(term);
    auto second_pin = odb::dbMPin::create(odb::dbMTerm::create(
        master, "B", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL));
    // The physical pin is away from the first-hop footprint under test.
    odb::dbBox::create(pin_, bottom_, 10, 10, 30, 30);
    odb::dbBox::create(second_pin, bottom_, 50, 10, 70, 30);
    master->setFrozen();
    inst_ = makeInst(block_,
                     master,
                     "target",
                     {.location = {kX, 0},
                      .status = odb::dbPlacementStatus::PLACED,
                      .iterms = {}});
    inst_->setPinAccessIdx(0);
    net_ = odb::dbNet::create(block_, "signal");
    inst_->findITerm("A")->connect(net_);
    supply_ = odb::dbNet::create(block_, "VDD");
    supply_->setSigType(odb::dbSigType::POWER);
    supply_->setSpecial();
    wire_ = odb::dbSWire::create(supply_, odb::dbWireType::ROUTED);
    ap_ = makeAP({190, 1400});
    inst_->findITerm("A")->setAccessPoint(pin_, ap_);
    service_.target = inst_;
    service_.addOwner(block_);
  }

  odb::dbAccessPoint* makeAP(odb::Point point,
                             bool up = true,
                             odb::dbMPin* pin = nullptr)
  {
    auto ap = odb::dbAccessPoint::create(block_, pin ? pin : pin_, 0);
    ap->setPoint(point);
    ap->setLayer(bottom_);
    ap->setAccess(true, up ? odb::dbDirection::UP : odb::dbDirection::EAST);
    if (up) {
      ap->addTechVia(1, via_);
    }
    return ap;
  }

  void stripe(odb::dbTechLayer* layer, int local_x = 190, int y = 1400)
  {
    ASSERT_NE(odb::dbSBox::create(wire_,
                                  layer,
                                  kX + local_x - 60,
                                  y - 60,
                                  kX + local_x + 60,
                                  y + 60,
                                  odb::dbWireShapeType::STRIPE),
              nullptr);
  }

  int repair(int dx = 1, int rounds = 3, int min_access_points = 1)
  {
    return dp_.repairPinAccess(&service_, dx, 0, rounds, 0, min_access_points);
  }

  int recover(int dx = 3, int attempts = 3, int min_access_points = 1)
  {
    return dp_.repairPinAccess(
        &service_, dx, 0, 3, attempts, min_access_points);
  }

  odb::dbTechLayer* bottom_;
  odb::dbTechLayer* cut_;
  odb::dbTechLayer* top_;
  odb::dbTechVia* via_;
  odb::dbMPin* pin_;
  odb::dbAccessPoint* ap_;
  odb::dbInst* inst_;
  odb::dbNet* net_;
  odb::dbNet* supply_;
  odb::dbSWire* wire_;
  RepairService service_;
  Opendp dp_{db_.get(), &logger_};
};

TEST_F(PinAccessRepairTest, NotReadyFailsBeforeMoving)
{
  stripe(top_);
  service_.ready = false;
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
  EXPECT_THROW(dp_.repairPinAccess(nullptr, 1, 0, 1), std::runtime_error);
}

TEST_F(PinAccessRepairTest, MissingAPIsUnresolved)
{
  odb::dbAccessPoint::destroy(ap_);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, NativeSkippedPinDoesNotNeedAP)
{
  odb::dbAccessPoint::destroy(ap_);
  service_.required = false;
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, FixedPhysicalCellOutsideRowsIsPreserved)
{
  auto master = odb::dbMaster::create(lib_, "OFF_ROW_TAP");
  master->setType(odb::dbMasterType::CORE_WELLTAP);
  master->setWidth(kSite);
  master->setHeight(kHeight);
  master->setSite(inst_->getMaster()->getSite());
  master->setFrozen();
  block_->setDieArea({0, 0, 32 * kSite, 2 * kHeight});
  const odb::Point location(2 * kSite + kSite / 2, kHeight / 2);
  auto fixed = makeInst(block_,
                        master,
                        "off_row_tap",
                        {.physical_only = true,
                         .location = location,
                         .status = odb::dbPlacementStatus::FIRM,
                         .iterms = {}});
  EXPECT_THROW(dp_.checkPlacement(false), std::runtime_error);

  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(fixed->getLocation(), location);
  EXPECT_EQ(fixed->getPlacementStatus(), odb::dbPlacementStatus::FIRM);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(inst_->getPlacementStatus(), odb::dbPlacementStatus::PLACED);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, MovableCellOutsideRowsIsRejected)
{
  inst_->setLocation(kX, kHeight);
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, kHeight));
  EXPECT_EQ(inst_->getPlacementStatus(), odb::dbPlacementStatus::PLACED);
}

TEST_F(PinAccessRepairTest, MovableOverlapWithFixedCellIsRejected)
{
  auto fixed = makeInst(block_,
                        inst_->getMaster(),
                        "fixed_overlap",
                        {.location = {kX, 0},
                         .status = odb::dbPlacementStatus::FIRM,
                         .iterms = {}});
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(fixed->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(fixed->getPlacementStatus(), odb::dbPlacementStatus::FIRM);
}

TEST_F(PinAccessRepairTest, StripeOnlyTopCollisionMovesOnce)
{
  stripe(top_);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, VerticalMovesRespectRowAlignmentAndDisplacement)
{
  auto site = inst_->getMaster()->getSite();
  block_->setDieArea({0, 0, 32 * kSite, 3 * kHeight});
  odb::dbRow::create(block_,
                     "ROW1",
                     site,
                     0,
                     kHeight,
                     odb::dbOrientType::MX,
                     odb::dbRowDir::HORIZONTAL,
                     32,
                     kSite);
  odb::dbRow::create(block_,
                     "ROW2",
                     site,
                     0,
                     2 * kHeight,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     32,
                     kSite);
  block_->setCoreArea(block_->computeCoreArea());
  stripe(top_);

  EXPECT_EQ(dp_.repairPinAccess(&service_, 0, 1, 3), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(dp_.repairPinAccess(&service_, 0, 2, 3), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 2 * kHeight));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::R0);
}

TEST_F(PinAccessRepairTest, BottomCollisionIsDetected)
{
  stripe(bottom_);
  EXPECT_EQ(repair(0), 1);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, CutCollisionIsDetected)
{
  stripe(cut_);
  EXPECT_EQ(repair(0), 1);
}

TEST_F(PinAccessRepairTest, FixedViaComponentsAreDetected)
{
  auto fixed_via = odb::dbTechVia::create(db_->getTech(), "fixed_cut_via");
  odb::dbBox::create(fixed_via, cut_, -50, -50, 50, 50);
  // Only the cut component intersects the candidate via.
  for (auto layer : {bottom_, top_}) {
    odb::dbBox::create(fixed_via, layer, 500, 500, 600, 600);
  }
  ASSERT_NE(odb::dbSBox::create(
                wire_, fixed_via, kX + 190, 1400, odb::dbWireShapeType::STRIPE),
            nullptr);
  EXPECT_EQ(repair(0), 1);
}

TEST_F(PinAccessRepairTest, SpacingBoundariesAndDiagonalDistance)
{
  top_->setSpacing(100);
  struct Case
  {
    int x_gap;
    int y_gap;
    int unresolved;
  };
  // The AP's top rectangle ends at (kX + 240, 1450).
  for (const auto& test : {Case{99, -20, 1},
                           Case{100, -20, 0},
                           Case{101, -20, 0},
                           Case{60, 60, 1},
                           Case{80, 80, 0}}) {
    SCOPED_TRACE(::testing::Message() << test.x_gap << ", " << test.y_gap);
    const int x = kX + 240 + test.x_gap;
    const int y = 1450 + test.y_gap;
    auto obstacle = odb::dbSBox::create(
        wire_, top_, x, y, x + 20, y + 20, odb::dbWireShapeType::STRIPE);
    ASSERT_NE(obstacle, nullptr);
    EXPECT_EQ(repair(0), test.unresolved);
    EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
    odb::dbSBox::destroy(obstacle);
  }
}

TEST_F(PinAccessRepairTest, SurvivingAlternativePreventsMovement)
{
  stripe(top_);
  makeAP({190, 2200});
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, SurvivingViaAlternativePreventsMovement)
{
  stripe(top_, 290);
  EXPECT_EQ(repair(0), 1);

  auto alternate = odb::dbTechVia::create(db_->getTech(), "alternate_via");
  odb::dbBox::create(alternate, bottom_, -50, -50, 50, 50);
  odb::dbBox::create(alternate, cut_, -20, -20, 20, 20);
  odb::dbBox::create(alternate, top_, -30, -30, 30, 30);
  ap_->addTechVia(1, alternate);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
  EXPECT_EQ(repair(0, 3, 2), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, MinimumCountsOnlySurvivingAccessPoints)
{
  stripe(top_);
  auto second_pin = odb::dbMPin::create(pin_->getMTerm());
  odb::dbBox::create(second_pin, bottom_, 10, 2100, 30, 2300);
  makeAP({190, 2200}, true, second_pin);
  ASSERT_EQ(repair(0, 3, 2), 1);
  EXPECT_EQ(repair(1, 3, 2), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, MinimumCannotCountUnsupportedAccess)
{
  makeAP({190, 2200}, false);
  EXPECT_EQ(repair(1, 3, 2), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, SameNetIsExempt)
{
  stripe(top_);
  inst_->findITerm("A")->connect(supply_);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, SupplyViaCutRejectsNearestPhysicalCandidate)
{
  stripe(top_);
  ASSERT_NE(odb::dbBox::create(inst_->getMaster(), cut_, 190, 190, 210, 210),
            nullptr);
  ASSERT_NE(
      odb::dbSBox::create(
          wire_, via_, kX - kSite + 200, 200, odb::dbWireShapeType::STRIPE),
      nullptr);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, SameNetPhysicalContactRemainsLegalAfterMove)
{
  stripe(top_);
  auto term = inst_->findITerm("B");
  term->connect(supply_);
  service_.skipped_term = term;
  auto pin = *term->getMTerm()->getMPins().begin();
  auto metal1 = db_->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  ASSERT_NE(odb::dbBox::create(pin, metal1, 10, 10, 30, 30), nullptr);
  ASSERT_NE(odb::dbSBox::create(wire_,
                                metal1,
                                kX - kSite + 10,
                                10,
                                kX - kSite + 30,
                                30,
                                odb::dbWireShapeType::DRCFILL),
            nullptr);
  EXPECT_NO_THROW(dp_.checkPlacement(false));
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(service_.updates, 1);
  EXPECT_NO_THROW(dp_.checkPlacement(false));
}

TEST_F(PinAccessRepairTest, MirroredAPIsAlreadyOriented)
{
  inst_->setOrient(odb::dbOrientType::MY);
  inst_->setLocation(kX, 0);
  ap_->setPoint({80, 1400});
  service_.dirty = false;  // The fixture has published the oriented AP.
  stripe(top_, 80);
  EXPECT_EQ(repair(0), 1);
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
}

TEST_F(PinAccessRepairTest, MirroredMoveChecksPreservedPhysicalOrientation)
{
  inst_->setOrient(odb::dbOrientType::MY);
  inst_->setLocation(kX, 0);
  service_.dirty = false;  // The centered AP is unchanged by this mirror.
  stripe(top_);
  // Only the hypothetical R0 pin at the destination overlaps this via.
  ASSERT_NE(odb::dbSBox::create(
                wire_, via_, kX - kSite + 20, 20, odb::dbWireShapeType::STRIPE),
            nullptr);
  auto fixed = makeInst(block_,
                        inst_->getMaster(),
                        "right_neighbor",
                        {.location = {kX + kSite, 0},
                         .status = odb::dbPlacementStatus::FIRM,
                         .iterms = {}});
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
  EXPECT_EQ(fixed->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, UnsupportedPlanarPinRemainsUnresolved)
{
  odb::dbAccessPoint::destroy(ap_);
  makeAP({190, 1400}, false);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, NonflaggedAndFixedCellsStayPut)
{
  stripe(top_);
  auto master = inst_->getMaster();
  auto fixed = makeInst(block_,
                        master,
                        "fixed",
                        {.location = {5 * kSite, 0},
                         .status = odb::dbPlacementStatus::FIRM,
                         .iterms = {}});
  auto clear = makeInst(block_,
                        master,
                        "clear",
                        {.location = {20 * kSite, 0},
                         .status = odb::dbPlacementStatus::PLACED,
                         .iterms = {}});
  clear->setPinAccessIdx(0);
  clear->findITerm("A")->connect(net_);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(fixed->getLocation(), odb::Point(5 * kSite, 0));
  EXPECT_EQ(clear->getLocation(), odb::Point(20 * kSite, 0));
}

TEST_F(PinAccessRepairTest, FreshPhaseAPRejectsNonimprovingBatch)
{
  stripe(top_);
  service_.refresh = [&]() {
    odb::dbAccessPoint::destroy(ap_);
    const int shift = kX - inst_->getLocation().x();
    ap_ = makeAP({190 + shift, 1400});
    inst_->findITerm("A")->setAccessPoint(pin_, ap_);
  };
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 2);
}

TEST_F(PinAccessRepairTest, ExternalMoveRefreshesBeforeInspectingAPs)
{
  stripe(top_, kSite + 30);
  inst_->setLocation(kX + kSite, 0);
  service_.refresh = [&]() {
    odb::dbAccessPoint::destroy(ap_);
    ap_ = makeAP({30, 1400});
    inst_->findITerm("A")->setAccessPoint(pin_, ap_);
  };
  EXPECT_EQ(repair(0), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, EntryRefreshFailurePreservesInputPlacement)
{
  inst_->setLocation(kX + kSite, 0);
  service_.refresh
      = []() { throw std::runtime_error("entry refresh failure"); };
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(service_.updates, 1);
}

TEST_F(PinAccessRepairTest, FailedRefreshRestoresPlacementAndRefreshes)
{
  stripe(top_);
  service_.refresh = [&]() {
    if (service_.updates == 1) {
      throw std::runtime_error("test refresh failure");
    }
  };
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 2);
}

TEST_F(PinAccessRepairTest, RejectedBatchRefreshFailureRestoresOnce)
{
  stripe(top_);
  int restores = 0;
  service_.on_move = [&](odb::dbInst* moved) {
    if (moved == inst_ && moved->getLocation() == odb::Point(kX, 0)) {
      ++restores;
    }
  };
  service_.refresh = [&]() {
    if (service_.updates == 2) {
      throw std::runtime_error("rollback refresh failure");
    }
    odb::dbAccessPoint::destroy(ap_);
    const int shift = kX - inst_->getLocation().x();
    ap_ = makeAP({190 + shift, 1400});
    inst_->findITerm("A")->setAccessPoint(pin_, ap_);
  };
  try {
    repair();
    FAIL() << "Expected rollback refresh failure";
  } catch (const std::runtime_error& error) {
    EXPECT_STREQ(error.what(), "rollback refresh failure");
  }
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 2);
  EXPECT_EQ(restores, 1);
}

TEST_F(PinAccessRepairTest, RecoveryThirdCandidateSucceeds)
{
  service_.ready = false;
  service_.missing_terms = [&]() {
    return inst_->getLocation().x() == kX - 2 * kSite
               ? std::vector<odb::dbITerm*>{}
               : std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_EQ(recover(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - 2 * kSite, 0));
  EXPECT_EQ(service_.recovery_locations.size(), 5);
  EXPECT_EQ(service_.begins, 1);
  EXPECT_EQ(service_.finishes, 1);
  EXPECT_EQ(service_.cancels, 0);
  EXPECT_TRUE(service_.ready);
  EXPECT_EQ(inst_->findITerm("A")->getNet(), net_);
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::R0);
}

TEST_F(PinAccessRepairTest, RecoveryCanImproveAnExistingAccessPoint)
{
  service_.missing_terms = [&]() {
    return inst_->findITerm("A")->getAccessPoints().at(pin_).size()
                   < static_cast<size_t>(service_.minimum)
               ? std::vector<odb::dbITerm*>{inst_->findITerm("A")}
               : std::vector<odb::dbITerm*>{};
  };
  service_.recovery_refresh = [&]() {
    if (inst_->getLocation().x() == kX - kSite) {
      makeAP({190, 2200});
    }
  };
  EXPECT_EQ(recover(1, 1, 2), 0);
  EXPECT_EQ(service_.minimum, 2);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(service_.recovery_locations.size(), 1);
  EXPECT_TRUE(service_.ready);
}

TEST_F(PinAccessRepairTest, RecoveryAttemptLimitIsConfigurable)
{
  service_.missing_terms = [&]() {
    return inst_->getLocation().x() == kX + 2 * kSite
               ? std::vector<odb::dbITerm*>{}
               : std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_THROW(dp_.repairPinAccess(&service_, 3, 0, 3, 3), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.recovery_locations,
            (std::vector<odb::Point>{{kX - kSite, 0},
                                     {kX, 0},
                                     {kX + kSite, 0},
                                     {kX, 0},
                                     {kX - 2 * kSite, 0},
                                     {kX, 0}}));
  EXPECT_FALSE(service_.ready);
  EXPECT_EQ(service_.finishes, 0);
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_EQ(service_.updates, 0);

  service_.recovery_locations.clear();
  EXPECT_EQ(recover(3, 4), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + 2 * kSite, 0));
  EXPECT_EQ(service_.recovery_locations.size(), 7);
}

TEST_F(PinAccessRepairTest, RecoveryCheapRejectionsDoNotConsumeAttempts)
{
  auto fixed = makeInst(block_,
                        inst_->getMaster(),
                        "occupied",
                        {.location = {kX - kSite, 0},
                         .status = odb::dbPlacementStatus::FIRM,
                         .iterms = {}});
  // The next nearest site has a physical pin collision, independent of APs.
  ASSERT_NE(odb::dbSBox::create(wire_,
                                bottom_,
                                kX + kSite + 10,
                                10,
                                kX + kSite + 30,
                                30,
                                odb::dbWireShapeType::STRIPE),
            nullptr);
  service_.missing_terms = [&]() {
    return inst_->getLocation().x() == kX - 2 * kSite
               ? std::vector<odb::dbITerm*>{}
               : std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_EQ(recover(3, 1), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - 2 * kSite, 0));
  EXPECT_EQ(service_.recovery_locations,
            (std::vector<odb::Point>{{kX - 2 * kSite, 0}}));
  EXPECT_EQ(fixed->getLocation(), odb::Point(kX - kSite, 0));
}

TEST_F(PinAccessRepairTest, RecoveryRejectsLossOfHealthyPin)
{
  auto healthy = makeInst(block_,
                          inst_->getMaster(),
                          "healthy",
                          {.location = {20 * kSite, 0},
                           .status = odb::dbPlacementStatus::PLACED,
                           .iterms = {}});
  healthy->setPinAccessIdx(0);
  healthy->findITerm("A")->connect(net_);
  service_.missing_terms = [&]() {
    if (inst_->getLocation().x() == kX - kSite) {
      return std::vector<odb::dbITerm*>{healthy->findITerm("A")};
    }
    if (inst_->getLocation().x() == kX + kSite) {
      return std::vector<odb::dbITerm*>{};
    }
    return std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_EQ(recover(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(healthy->getLocation(), odb::Point(20 * kSite, 0));
  EXPECT_EQ(healthy->findITerm("A")->getNet(), net_);
}

TEST_F(PinAccessRepairTest, RecoveryRequiresAllMissingPinsOfTargetCell)
{
  auto second = inst_->findITerm("B");
  second->connect(net_);
  auto pin = *second->getMTerm()->getMPins().begin();
  auto ap = makeAP({190, 1400}, true, pin);
  second->setAccessPoint(pin, ap);
  service_.missing_terms = [&]() {
    if (inst_->getLocation().x() == kX + kSite) {
      return std::vector<odb::dbITerm*>{};
    }
    if (inst_->getLocation().x() == kX - kSite) {
      return std::vector<odb::dbITerm*>{second};
    }
    return std::vector<odb::dbITerm*>{inst_->findITerm("A"), second};
  };
  EXPECT_EQ(recover(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite, 0));
  EXPECT_EQ(service_.recovery_locations.size(), 3);
  EXPECT_EQ(second->getNet(), net_);
}

TEST_F(PinAccessRepairTest, RecoveryCannotBreakAnEarlierRecoveredPin)
{
  auto other = makeInst(block_,
                        inst_->getMaster(),
                        "other",
                        {.location = {20 * kSite, 0},
                         .status = odb::dbPlacementStatus::PLACED,
                         .iterms = {}});
  other->findITerm("A")->connect(net_);
  bool evaluated_later_trial = false;
  service_.missing_terms = [&]() {
    evaluated_later_trial |= inst_->getLocation().x() != kX
                             && other->getLocation().x() != 20 * kSite;
    std::vector<odb::dbITerm*> terms;
    if (inst_->getLocation().x() == kX
        || other->getLocation().x() != 20 * kSite) {
      terms.push_back(inst_->findITerm("A"));
    }
    if (other->getLocation().x() == 20 * kSite) {
      terms.push_back(other->findITerm("A"));
    }
    return terms;
  };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_TRUE(evaluated_later_trial);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(other->getLocation(), odb::Point(20 * kSite, 0));
  EXPECT_EQ(other->findITerm("A")->getNet(), net_);
  EXPECT_EQ(service_.finishes, 0);
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_FALSE(service_.ready);
}

TEST_F(PinAccessRepairTest, RecoveryFixedMissingPinFailsWithoutMoving)
{
  inst_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  service_.missing_terms
      = [&]() { return std::vector<odb::dbITerm*>{inst_->findITerm("A")}; };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_TRUE(service_.recovery_locations.empty());
  EXPECT_EQ(service_.cancels, 1);
}

TEST_F(PinAccessRepairTest, RecoveryPreservesMirrorOrientation)
{
  inst_->setOrient(odb::dbOrientType::MY);
  inst_->setLocation(kX, 0);
  service_.missing_terms = [&]() {
    EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
    return inst_->getLocation().x() == kX - kSite
               ? std::vector<odb::dbITerm*>{}
               : std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_EQ(recover(), 0);
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
  EXPECT_EQ(inst_->findITerm("A")->getNet(), net_);
}

TEST_F(PinAccessRepairTest, RecoveryRefreshExceptionCancelsBeforeRestoring)
{
  bool restored_after_cancel = false;
  service_.on_move = [&](odb::dbInst* moved) {
    if (moved == inst_ && moved->getLocation() == odb::Point(kX, 0)) {
      EXPECT_EQ(service_.cancels, 1);
      EXPECT_FALSE(service_.recovering);
      restored_after_cancel = true;
    }
  };
  service_.missing_terms
      = [&]() { return std::vector<odb::dbITerm*>{inst_->findITerm("A")}; };
  service_.recovery_refresh
      = []() { throw std::runtime_error("native error"); };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_EQ(service_.finishes, 0);
  EXPECT_FALSE(service_.ready);
  EXPECT_TRUE(restored_after_cancel);
}

TEST_F(PinAccessRepairTest, RecoveryRestoreMustReproduceMissingPinSet)
{
  service_.missing_terms = [&]() {
    if (service_.recovery_locations.size() >= 2) {
      return std::vector<odb::dbITerm*>{};
    }
    return std::vector<odb::dbITerm*>{inst_->findITerm("A")};
  };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.recovery_locations.size(), 2);
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_EQ(service_.finishes, 0);
  EXPECT_FALSE(service_.ready);
}

TEST_F(PinAccessRepairTest, RecoveryFinishExceptionRestoresMoves)
{
  service_.finish_failure = true;
  service_.missing_terms = [&]() {
    return inst_->getLocation().x() == kX
               ? std::vector<odb::dbITerm*>{inst_->findITerm("A")}
               : std::vector<odb::dbITerm*>{};
  };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_EQ(service_.finishes, 1);
  EXPECT_FALSE(service_.ready);
}

TEST_F(PinAccessRepairTest, RecoveryChecksPlacementBeforeStartingNativePA)
{
  inst_->setLocation(kX, kHeight);
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(service_.begins, 0);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, RecoveryRejectsNegativeAttemptLimit)
{
  EXPECT_THROW(recover(3, -1), std::runtime_error);
  EXPECT_EQ(service_.begins, 0);
}

TEST_F(PinAccessRepairTest, RejectsNonpositiveAccessPointMinimum)
{
  EXPECT_THROW(recover(1, 3, 0), std::runtime_error);
  EXPECT_THROW(recover(1, 3, -1), std::runtime_error);
  EXPECT_EQ(service_.begins, 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

}  // namespace
}  // namespace dpl
