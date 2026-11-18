// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <functional>
#include <stdexcept>

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
  bool ready = true;
  int updates = 0;
  std::function<bool(odb::dbInst*, const odb::Point&)> usable;
  std::function<void()> refresh;
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
    return dp_.repairPinAccess(&service_, dx, 0, rounds);
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

TEST_F(PinAccessRepairTest, RejectsInvalidOptions)
{
  EXPECT_THROW(repair(-1), std::runtime_error);
  EXPECT_THROW(repair(1, 0), std::runtime_error);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kX, 0));
}

}  // namespace

TEST_F(PinAccessRepairTest, ReleasesGridPixels)
{
  EXPECT_EQ(repair(), 0);
  EXPECT_TRUE(dp_.grid_->pixelsReleased());
}

}  // namespace dpl
