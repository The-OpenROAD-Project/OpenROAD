// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "dpl/Opendp.h"
#include "drt/PinAccessService.h"
#include "gtest/gtest.h"
#include "infrastructure/Grid.h"
#include "odb/db.h"
#include "tst/nangate45_fixture.h"

namespace dpl {
namespace {

class RepairService : public drt::PinAccessService
{
 public:
  bool hasPinAccess() const override { return ready; }
  bool hasMinimumPinAccess(int min_access_points) const override
  {
    return minimum(min_access_points);
  }
  bool hasUsablePinAccess(odb::dbInst* inst,
                          const odb::Point& location) const override
  {
    return usable(inst, location);
  }
  std::vector<odb::dbInst*> findPinAccessFailures(
      const std::vector<odb::dbInst*>& instances) const override
  {
    std::vector<odb::dbInst*> failures;
    for (auto* inst : instances) {
      if (!hasUsablePinAccess(inst, inst->getLocation())) {
        failures.push_back(inst);
      }
    }
    return failures;
  }
  void updateDirtyPinAccess() override
  {
    ++updates;
    if (refresh) {
      refresh();
    }
  }
  std::vector<odb::dbITerm*> beginPinAccessRecovery(int minimum) override
  {
    ++begins;
    requested_minimum = minimum;
    if (!missing) {
      return {};
    }
    auto result = missing();
    active = !result.empty();
    ready = !active;
    return result;
  }
  std::vector<odb::dbITerm*> updatePinAccessRecovery() override
  {
    ++native_updates;
    return missing();
  }
  void finishPinAccessRecovery() override
  {
    if (finish) {
      finish();
    }
    ++finishes;
    active = false;
    ready = true;
  }
  void cancelPinAccessRecovery() noexcept override
  {
    if (active) {
      ++cancels;
      active = false;
      ready = false;
      if (cancel) {
        cancel();
      }
    }
  }
  bool ready = true;
  bool active = false;
  int begins = 0;
  int requested_minimum = 0;
  int native_updates = 0;
  int finishes = 0;
  int cancels = 0;
  std::function<std::vector<odb::dbITerm*>()> missing;
  std::function<void()> finish;
  std::function<void()> cancel;
  int updates = 0;
  std::function<bool(odb::dbInst*, const odb::Point&)> usable;
  std::function<void()> refresh;
  std::function<bool(int)> minimum = [](int) { return true; };
};

class PinAccessRepairTest : public tst::Nangate45Fixture
{
 protected:
  static constexpr int kSite = 380;
  static constexpr int kHeight = 2800;
  static constexpr int kX = 10 * kSite;

  void SetUp() override
  {
    auto* site = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
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
    auto* master = odb::dbMaster::create(lib_, "REPAIR_CELL");
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(kSite);
    master->setHeight(kHeight);
    master->setSite(site);
    master->setSymmetryY();
    for (const char* name : {"A", "B", "C"}) {
      odb::dbMTerm::create(
          master, name, odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
    }
    master->setFrozen();
    inst_ = makeCell("target", kX);
    service_.usable = [&](odb::dbInst* inst, const odb::Point& location) {
      return inst != inst_ || location.x() != kX;
    };
  }

  odb::dbInst* makeCell(const char* name, int x)
  {
    return makeInst(block_,
                    lib_->findMaster("REPAIR_CELL"),
                    name,
                    {.location = {x, 0},
                     .status = odb::dbPlacementStatus::PLACED,
                     .iterms = {}});
  }

  int repair(int dx = 1, int rounds = 3)
  {
    return dp_.repairPinAccess(&service_, dx, 0, rounds, 0);
  }

  void configureRecovery(std::function<std::vector<odb::dbITerm*>()> missing)
  {
    service_.ready = false;
    service_.missing = std::move(missing);
    service_.usable = [](odb::dbInst*, const odb::Point&) { return true; };
  }

  int recover(int attempts = 3, int dx = 4, int minimum = 1)
  {
    return dp_.repairPinAccess(&service_, dx, 0, 3, attempts, minimum);
  }

  std::vector<odb::dbITerm*> missingA() const
  {
    return {inst_->findITerm("A")};
  }

  odb::dbInst* inst_;
  RepairService service_;
  Opendp dp_{db_.get(), &logger_};
};

TEST_F(PinAccessRepairTest, RequiresLivePinAccess)
{
  service_.ready = false;
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_THROW(dp_.repairPinAccess(nullptr, 1, 0, 1), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, UsesNearestLegalSiteAndPreservesOrientation)
{
  inst_->setOrient(odb::dbOrientType::MY);
  inst_->setLocation(kX, 0);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
  EXPECT_EQ(service_.updates, 2);
  EXPECT_EQ(repair(), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
}

TEST_F(PinAccessRepairTest, VerticalMovesRespectRowOrientationAndDisplacement)
{
  auto* site = inst_->getMaster()->getSite();
  block_->setDieArea({0, 0, 32 * kSite, 3 * kHeight});
  for (int row = 1; row <= 2; ++row) {
    odb::dbRow::create(block_,
                       row == 1 ? "ROW1" : "ROW2",
                       site,
                       0,
                       row * kHeight,
                       row == 1 ? odb::dbOrientType::MX : odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       32,
                       kSite);
  }
  block_->setCoreArea(block_->computeCoreArea());
  service_.usable = [](odb::dbInst*, const odb::Point& location) {
    return location.y() != 0;
  };
  EXPECT_EQ(dp_.repairPinAccess(&service_, 0, 1, 3), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(dp_.repairPinAccess(&service_, 0, 2, 3), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 2 * kHeight));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::R0);
}

TEST_F(PinAccessRepairTest, FixedCellsAndHealthyCellsStayPut)
{
  inst_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  auto* healthy = makeCell("healthy", kX + 2 * kSite);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(healthy->getLocation(), odb::Point(kX + 2 * kSite, 0));
}

TEST_F(PinAccessRepairTest, ZeroDisplacementAndMissingAccessPreservePlacement)
{
  EXPECT_EQ(repair(0), 1);
  service_.usable = [](odb::dbInst*, const odb::Point&) { return false; };
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, PlacementOccupancyBoundsCandidateSearch)
{
  auto* left = makeCell("left", kX - kSite);
  auto* right = makeCell("right", kX + kSite);
  left->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(left->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(right->getLocation(), odb::Point(kX + kSite, 0));
}

TEST_F(PinAccessRepairTest, PaddingBoundsCandidateSearch)
{
  makeCell("left", kX - 2 * kSite);
  makeCell("right", kX + 2 * kSite);
  dp_.setPaddingGlobal(1, 0);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, RegionBoundsCandidateSearch)
{
  auto* region = odb::dbRegion::create(block_, "region");
  odb::dbBox::create(region, kX, 0, kX + kSite, kHeight);
  auto* group = odb::dbGroup::create(region, "group");
  group->addInst(inst_);
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, RejectsIllegalInputPlacement)
{
  inst_->setLocation(kX + kSite / 2, 0);
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX + kSite / 2, 0));
}

TEST_F(PinAccessRepairTest, AcceptsNonemptyProperSubset)
{
  auto* right = makeCell("right", kX + 4 * kSite);
  auto* left = makeCell("left", kX - 4 * kSite);
  right->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  left->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  service_.usable = [&](odb::dbInst* inst, const odb::Point& location) {
    return inst == inst_ && location.x() != kX;
  };
  EXPECT_EQ(repair(), 2);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
  EXPECT_EQ(right->getLocation(), odb::Point(kX + 4 * kSite, 0));
  EXPECT_EQ(left->getLocation(), odb::Point(kX - 4 * kSite, 0));
}

TEST_F(PinAccessRepairTest, RejectsNewHealthyNeighborFailure)
{
  auto* second = makeCell("second", kX + 6 * kSite);
  auto* healthy = makeCell("healthy", kX + 3 * kSite);
  service_.usable = [&](odb::dbInst* inst, const odb::Point& location) {
    if (inst == healthy) {
      return inst_->getLocation().x() == kX;
    }
    return location.x() != (inst == second ? kX + 6 * kSite : kX);
  };
  EXPECT_EQ(repair(), 2);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(second->getLocation(), odb::Point(kX + 6 * kSite, 0));
  EXPECT_EQ(healthy->getLocation(), odb::Point(kX + 3 * kSite, 0));
  EXPECT_EQ(service_.updates, 3);
}

TEST_F(PinAccessRepairTest, RejectsNonimprovingBatch)
{
  service_.usable = [&](odb::dbInst*, const odb::Point& location) {
    return inst_->getLocation().x() == kX && location.x() != kX;
  };
  EXPECT_EQ(repair(), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 3);
}

TEST_F(PinAccessRepairTest, QueryExceptionRestoresActiveTrial)
{
  service_.usable = [&](odb::dbInst*, const odb::Point& location) {
    if (location.x() != kX) {
      throw std::runtime_error("query failed");
    }
    return false;
  };
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, RefreshExceptionRestoresPlacement)
{
  service_.refresh = [&] {
    if (service_.updates == 2) {
      throw std::runtime_error("refresh failed");
    }
  };
  EXPECT_THROW(repair(), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.updates, 3);
}

TEST_F(PinAccessRepairTest, DefaultRecoveryReachesThirdUntriedSite)
{
  std::vector<int> visited;
  configureRecovery([&] {
    const int x = inst_->getLocation().x();
    if (x != kX) {
      visited.push_back(x);
    }
    return x == kX - 2 * kSite ? std::vector<odb::dbITerm*>{} : missingA();
  });
  inst_->setOrient(odb::dbOrientType::MY);
  inst_->setLocation(kX, 0);
  EXPECT_EQ(dp_.repairPinAccess(&service_, 4, 0, 3), 0);
  EXPECT_EQ(visited,
            (std::vector<int>{kX - kSite, kX + kSite, kX - 2 * kSite}));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MY);
  EXPECT_EQ(service_.native_updates, 5);
  EXPECT_EQ(service_.finishes, 1);
  EXPECT_EQ(service_.updates, 0);
  EXPECT_TRUE(service_.ready);
}

TEST_F(PinAccessRepairTest, RecoveryAttemptLimitRollsBackAndCanBeRaised)
{
  configureRecovery([&] {
    return inst_->getLocation().x() == kX - 2 * kSite
               ? std::vector<odb::dbITerm*>{}
               : missingA();
  });
  EXPECT_THROW(recover(2), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.native_updates, 4);
  EXPECT_EQ(service_.cancels, 1);
  EXPECT_FALSE(service_.ready);
  EXPECT_EQ(recover(3), 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - 2 * kSite, 0));
}

TEST_F(PinAccessRepairTest, DisabledRecoveryRequiresPublishedAccess)
{
  configureRecovery([&] { return missingA(); });
  EXPECT_THROW(recover(0), std::runtime_error);
  EXPECT_EQ(service_.begins, 0);
  EXPECT_EQ(service_.native_updates, 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  service_.ready = true;
  EXPECT_EQ(recover(0, 4, 1000), 0);
  EXPECT_EQ(service_.begins, 0);
}

TEST_F(PinAccessRepairTest, RecoverySkipsOccupiedSitesWithoutSpendingAttempts)
{
  makeCell("left", kX - kSite)
      ->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  makeCell("right", kX + kSite);
  configureRecovery([&] {
    return inst_->getLocation().x() == kX - 2 * kSite
               ? std::vector<odb::dbITerm*>{}
               : missingA();
  });
  EXPECT_EQ(recover(1, 2, 7), 0);
  EXPECT_EQ(service_.native_updates, 1);
  EXPECT_EQ(service_.requested_minimum, 7);
}

TEST_F(PinAccessRepairTest, RecoveryRejectsLossOfHealthySiblingPin)
{
  auto* a = inst_->findITerm("A");
  auto* b = inst_->findITerm("B");
  auto* c = inst_->findITerm("C");
  configureRecovery([&] {
    if (inst_->getLocation().x() != kX) {
      return std::vector<odb::dbITerm*>{b};
    }
    return std::vector<odb::dbITerm*>{a, c};
  });
  EXPECT_THROW(recover(1), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.finishes, 0);
}

TEST_F(PinAccessRepairTest, RecoveryPreservesPreviouslyRecoveredPins)
{
  auto* a = inst_->findITerm("A");
  auto* b = inst_->findITerm("B");
  auto* c = inst_->findITerm("C");
  configureRecovery([&] {
    const int x = inst_->getLocation().x();
    if (x == kX) {
      return std::vector<odb::dbITerm*>{c, a, b, a};
    }
    if (x == kX - kSite) {
      return std::vector<odb::dbITerm*>{c, b, b};
    }
    return std::vector<odb::dbITerm*>{a};
  });
  EXPECT_THROW(recover(2), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.native_updates, 3);
  EXPECT_EQ(service_.cancels, 1);
}

TEST_F(PinAccessRepairTest, RecoveryRejectsNewGeometryFailure)
{
  auto* healthy = makeCell("healthy", kX + 6 * kSite);
  configureRecovery([&] {
    return inst_->getLocation().x() == kX ? missingA()
                                          : std::vector<odb::dbITerm*>{};
  });
  service_.usable = [&](odb::dbInst* inst, const odb::Point&) {
    return inst != healthy || inst_->getLocation().x() == kX;
  };
  EXPECT_THROW(recover(1), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(healthy->getLocation(), odb::Point(kX + 6 * kSite, 0));
}

TEST_F(PinAccessRepairTest, RecoveryExceptionRestoresAllEarlierAcceptedMoves)
{
  auto* second = makeCell("second", kX + 6 * kSite);
  auto* a = inst_->findITerm("A");
  auto* b = second->findITerm("B");
  bool canceled_before_restore = false;
  configureRecovery([&] {
    if (service_.native_updates == 2) {
      throw std::runtime_error("native update failed");
    }
    std::vector<odb::dbITerm*> pins{b};
    if (inst_->getLocation().x() == kX) {
      pins.push_back(a);
    }
    return pins;
  });
  service_.cancel = [&] {
    canceled_before_restore = inst_->getLocation().x() == kX - kSite
                              && second->getLocation().x() == kX + 4 * kSite;
  };
  EXPECT_THROW(recover(3), std::runtime_error);
  EXPECT_TRUE(canceled_before_restore);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(second->getLocation(), odb::Point(kX + 6 * kSite, 0));
  EXPECT_EQ(service_.native_updates, 2);
}

TEST_F(PinAccessRepairTest, RecoveryPublicationExceptionCancelsBeforeRestore)
{
  bool canceled_before_restore = false;
  configureRecovery([&] {
    return inst_->getLocation().x() == kX ? missingA()
                                          : std::vector<odb::dbITerm*>{};
  });
  service_.finish
      = [] { throw std::runtime_error("pattern generation failed"); };
  service_.cancel
      = [&] { canceled_before_restore = inst_->getLocation().x() != kX; };
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_TRUE(canceled_before_restore);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.cancels, 1);
}

TEST_F(PinAccessRepairTest, RecoveryFailsForFixedCellsAndNoLegalSites)
{
  configureRecovery([&] { return missingA(); });
  inst_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  EXPECT_THROW(recover(), std::runtime_error);
  EXPECT_EQ(service_.native_updates, 0);
  inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  EXPECT_THROW(recover(3, 0), std::runtime_error);
  EXPECT_EQ(service_.native_updates, 0);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

TEST_F(PinAccessRepairTest, RecoveryPublishesDespiteExistingGeometryFailure)
{
  auto* blocker = makeCell("blocker", kX + 6 * kSite);
  blocker->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  configureRecovery([&] {
    return inst_->getLocation().x() == kX ? missingA()
                                          : std::vector<odb::dbITerm*>{};
  });
  service_.usable
      = [&](odb::dbInst* inst, const odb::Point&) { return inst != blocker; };
  EXPECT_EQ(recover(), 1);
  EXPECT_EQ(service_.finishes, 1);
  EXPECT_TRUE(service_.ready);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX - kSite, 0));
}

TEST_F(PinAccessRepairTest, ReadyRecoveryFastPathDoesNotRefreshNativeAccess)
{
  service_.usable = [](odb::dbInst*, const odb::Point&) { return true; };
  EXPECT_EQ(recover(), 0);
  EXPECT_EQ(service_.begins, 1);
  EXPECT_EQ(service_.native_updates, 0);
  EXPECT_EQ(service_.finishes, 0);
  EXPECT_EQ(service_.updates, 0);
}

TEST_F(PinAccessRepairTest, GeometryRepairPreservesRequestedMinimum)
{
  service_.minimum = [&](int minimum) {
    EXPECT_EQ(minimum, 2);
    return inst_->getLocation().x() == kX;
  };
  EXPECT_EQ(recover(3, 1, 2), 1);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
  EXPECT_EQ(service_.requested_minimum, 2);
  EXPECT_EQ(service_.native_updates, 0);
  EXPECT_EQ(service_.updates, 2);
  EXPECT_TRUE(service_.hasMinimumPinAccess(2));
}

TEST_F(PinAccessRepairTest, RejectsInvalidOptions)
{
  EXPECT_THROW(repair(-1), std::runtime_error);
  EXPECT_THROW(repair(1, 0), std::runtime_error);
  EXPECT_THROW(recover(-1), std::runtime_error);
  EXPECT_THROW(recover(3, 1, 0), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

}  // namespace

TEST_F(PinAccessRepairTest, ReleasesGridPixels)
{
  EXPECT_EQ(repair(), 0);
  EXPECT_TRUE(dp_.grid_->pixelsReleased());
}

}  // namespace dpl
