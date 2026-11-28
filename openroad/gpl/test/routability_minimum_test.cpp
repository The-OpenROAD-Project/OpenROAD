// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "openroad/gpl/src/graphicsNone.h"
#include "openroad/gpl/src/nesterovBase.h"
#include "openroad/gpl/src/nesterovPlace.h"
#include "openroad/gpl/src/placerBase.h"
#include "openroad/gpl/src/point.h"
#include "openroad/gpl/src/routeBase.h"
#include "tst/IntegratedFixture.h"

namespace gpl {

// A routability phase measures the congestion of one placement per pass and
// keeps the sizes of the pass that measured the least. These tests drive the
// real controller in the main loop's order, routabilitySnapshot(),
// runRoutability() and isConverged(), on gcd with routability on and timing
// off. Friend access stands in for the Nesterov steps between the passes: it
// places the cells and sets the overflow the iteration measured on them.
// RUDY's wire width sets each pass's congestion: a wider wire reads worse.
class RoutabilityMinimumTest : public tst::IntegratedFixture
{
 protected:
  // A wire width that reads as congested on every placement below.
  static constexpr int kWidth = 200;
  // The placements: cell centers on a lattice, in instance order, shifted
  // right by a number of sites. The routability snapshot's is left of the
  // passes', and each pass's is its own.
  static constexpr int kSnapshotShift = -8;
  static constexpr int kColumns = 17;
  static constexpr int kSiteWidth = 380;
  static constexpr int kRowHeight = 2800;

  RoutabilityMinimumTest() : IntegratedFixture(Technology::kNangate45, "") {}

  void SetUp() override
  {
    block_ = readDef(getFilePath("_main/openroad/gpl/test/gcd.def"), {lib_});
    ASSERT_NE(block_, nullptr);
    const odb::Point center = block_->getCoreArea().center();
    for (odb::dbInst* inst : block_->getInsts()) {
      inst->setLocation(center.x(), center.y());
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    sta_->postReadDb(db_.get());
    sta_->postReadDef(block_);
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    options_.routabilityDrivenMode = true;
    options_.timingDrivenMode = false;
  }

  void makePlacer()
  {
    pbc_ = std::make_shared<PlacerBaseCommon>(
        db_.get(), PlacerBaseVars(options_), &logger_);
    pb_vec_.push_back(std::make_shared<PlacerBase>(
        db_.get(), pbc_, &logger_, false, nullptr));
    const NesterovBaseVars nb_vars(options_);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, 1, Clusters{});
    nb_vec_.push_back(
        std::make_shared<NesterovBase>(nb_vars, pb_vec_[0], nbc_, &logger_));
    rb_ = std::make_shared<RouteBase>(
        RouteBaseVars(options_), db_.get(), &grt_, nbc_, nb_vec_, &logger_);
    np_ = std::make_unique<NesterovPlace>(
        NesterovPlaceVars(options_, nbc_->getHpwl()),
        pbc_,
        nbc_,
        pb_vec_,
        nb_vec_,
        rb_,
        nullptr,
        nullptr,
        nullptr,
        std::make_unique<GraphicsNone>(),
        &logger_);
  }

  NesterovBase& nb() const { return *nb_vec_.front(); }

  // Puts every standard cell on the lattice shifted by `shift` sites, as the
  // Nesterov steps between two passes would have moved them.
  void placeCells(const int shift)
  {
    NesterovBase& region = nb();
    const odb::Rect core = block_->getCoreArea();
    const int x0
        = core.xMin() + core.dx() / 2 - (kColumns / 2) * 3 * kSiteWidth;
    const int y0 = core.yMin() + 2 * kRowHeight + kRowHeight / 2;
    int index = 0;
    for (size_t i = 0; i < region.nb_gcells_.size(); ++i) {
      if (!region.nb_gcells_[i]->isStdInstance()) {
        continue;
      }
      const FloatPoint center(
          x0 + (index % kColumns) * 3 * kSiteWidth + shift * kSiteWidth,
          y0 + (index / kColumns) * kRowHeight);
      region.curCoordi_[i] = center;
      region.curSLPCoordi_[i] = center;
      ++index;
    }
    region.updateGCellDensityCenterLocation(region.curCoordi_);
    region.updateDensityFieldBin();
  }

  // What the iteration's updateNextIter() measures, set by hand: the overflow
  // of the placement as it now is.
  void measure(const float overflow)
  {
    for (const auto& region : nb_vec_) {
      region->sum_overflow_ = overflow;
      region->sum_overflow_unscaled_ = overflow;
    }
    np_->average_overflow_ = overflow;
    np_->average_overflow_unscaled_ = overflow;
    np_->overflow_update_is_stale_ = false;
  }

  // Cells moving at the given fraction of their peak displacement.
  void setDisplacementRatio(const float ratio)
  {
    for (const auto& region : nb_vec_) {
      region->peak_coordi_distance_ = 1.0f;
      region->coordiDistance_ = ratio;
    }
  }

  // One iteration of the main loop from the routability snapshot on, with
  // RUDY at the given wire width. Returns whether the placement converged.
  bool runIteration(const int width, float& current_a)
  {
    grt_.getRudy()->setWireWidth(width);
    np_->routabilitySnapshot(iter_, current_a, "", passes_, 0);
    np_->runRoutability(iter_, 0, "", passes_, current_a);
    return np_->isConverged(iter_++, 0);
  }

  // Takes the routability snapshot on the snapshot's placement, at the
  // snapshot overflow, as the main loop does on the way down.
  void saveRoutabilitySnapshot(float& current_a)
  {
    placeCells(kSnapshotShift);
    measure(options_.routabilitySnapshotOverflow);
    ASSERT_FALSE(runIteration(kWidth, current_a));
    ASSERT_TRUE(np_->is_routability_snapshot_saved_);
    ASSERT_EQ(passes_, 0);
  }

  bool routabilityNeeded() const { return np_->is_routability_need_; }

  int64_t cellAreaSum() const
  {
    int64_t area = 0;
    for (const auto& cell : nb().getGCells()) {
      if (cell->isStdInstance()) {
        area += static_cast<int64_t>(cell->dx()) * cell->dy();
      }
    }
    return area;
  }

  PlaceOptions options_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pb_vec_;
  std::vector<std::shared_ptr<NesterovBase>> nb_vec_;
  std::shared_ptr<RouteBase> rb_;
  std::unique_ptr<NesterovPlace> np_;
  int iter_ = 0;
  int passes_ = 0;
};

// The ways a routability phase ends by going back to its minimum, with the
// minimum at a later pass than the first.
enum class Ending
{
  // Three passes in a row do not improve on the minimum (GPL-0054).
  kNoImprovement,
  // The inflation budget is spent (GPL-0097). Every third pass improves, so
  // no three in a row fail to.
  kBudgetSpent
};

class RevertToMinimumTest : public RoutabilityMinimumTest,
                            public ::testing::WithParamInterface<Ending>
{
 protected:
  // Pass 1 is congested and pass 2 less so: an inflation helped. The other
  // passes read as pass 1 does, except that, when the budget is to end the
  // phase, every third pass improves on the one three before it.
  static int widthFor(const Ending ending, const int pass)
  {
    if (pass % 3 == 2 && (pass == 2 || ending == Ending::kBudgetSpent)) {
      return static_cast<int>(2 * kWidth * std::pow(0.9, (pass - 2) / 3));
    }
    return 4 * kWidth;
  }
};

// After the phase goes back to its minimum the cells have the minimum's
// sizes, and the overflow from there on is measured against their area, not
// the area of the sizes they just left.
TEST_P(RevertToMinimumTest, MeasuresOverflowOnTheCellsAsTheyAre)
{
  makePlacer();
  float current_a = 1.0f;
  ASSERT_NO_FATAL_FAILURE(saveRoutabilitySnapshot(current_a));

  constexpr int kMaxPasses = 20;
  int64_t min_area = 0;
  int min_pass = 0;
  logger_.teeStringBegin();
  while (routabilityNeeded() && passes_ < kMaxPasses) {
    const int pass = passes_ + 1;
    placeCells(2 * pass);
    measure(options_.routabilityCheckOverflow);
    setDisplacementRatio(0.0f);
    const int64_t area = cellAreaSum();
    ASSERT_FALSE(runIteration(widthFor(GetParam(), pass), current_a));
    ASSERT_EQ(passes_, pass) << "every iteration must run a pass";
    if (rb_->isMinRc()) {
      min_area = area;
      min_pass = pass;
    }
  }
  const std::string log = logger_.teeStringEnd();
  ASSERT_FALSE(routabilityNeeded());
  ASSERT_NE(
      log.find(GetParam() == Ending::kNoImprovement ? "GPL-0054" : "GPL-0097"),
      std::string::npos);
  ASSERT_NE(log.find("GPL-0089"), std::string::npos)
      << "the phase must end by going back to its minimum";
  ASSERT_GE(min_pass, 2);

  const NesterovBase& region = nb();
  EXPECT_EQ(cellAreaSum(), min_area)
      << "the cells must have the sizes of pass " << min_pass;
  EXPECT_EQ(region.getStdInstArea(), cellAreaSum())
      << "the standard-cell area must be that of the cells as they are";
  EXPECT_EQ(region.getMovableArea(),
            static_cast<int64_t>(region.getWhiteSpaceArea()
                                 * region.getTargetDensity()));
  EXPECT_FLOAT_EQ(
      region.getCurrentSumOverflowUnscaled(iter_),
      region.getOverflowAreaUnscaled() / static_cast<float>(cellAreaSum()))
      << "the overflow must be a fraction of the cells' area as they are";
}

INSTANTIATE_TEST_SUITE_P(Endings,
                         RevertToMinimumTest,
                         ::testing::Values(Ending::kNoImprovement,
                                           Ending::kBudgetSpent));

}  // namespace gpl
