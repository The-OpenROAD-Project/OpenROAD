// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Checks that the PlaceResult returned by Replace describes the run: the
// final values match the placement written back to odb, and the trace
// holds every Nesterov iteration without graphics or debug enabled.

#include <iterator>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/util.h"
#include "tst/fixture.h"

namespace gpl {
namespace {

class PlaceResultTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib
        = loadTechAndLib("ng45", "ng45", "_main/src/gpl/test/nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath("_main/src/gpl/test/gcd.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
  }

  // Every iteration from first to last appears once, in order.
  static void expectConsecutive(const std::vector<PlaceIteration>& trace,
                                const int first,
                                const int last)
  {
    ASSERT_EQ(std::ssize(trace), last - first + 1);
    for (int i = 0; i < std::ssize(trace); ++i) {
      EXPECT_EQ(trace[i].iteration, first + i);
    }
  }

  // A placer that has not seen the design, as a separate command would be.
  Replace freshPlacer()
  {
    return Replace{db_.get(), sta_.get(), nullptr, nullptr, nullptr, &logger_};
  }

  odb::dbBlock* block_ = nullptr;
  Replace replace_{db_.get(), sta_.get(), nullptr, nullptr, nullptr, &logger_};
};

TEST_F(PlaceResultTest, DoPlaceReportsTheFinalPlacement)
{
  PlaceOptions options;
  const PlaceResult result = replace_.doPlace(1, options);

  EXPECT_EQ(result.stop, PlaceStop::kConverged);
  EXPECT_GT(result.final_iteration, 0);
  EXPECT_LT(result.final_iteration, options.nesterovPlaceMaxIter);
  EXPECT_LE(result.overflow, options.overflow);
  EXPECT_EQ(result.hpwl, odb::WireLengthEvaluator(block_).hpwl());
}

// The run converges at the iteration where overflow first drops below the
// target, and that is the last one in the trace.  The density penalty ramps
// up while cells spread.
TEST_F(PlaceResultTest, TraceHoldsEveryIteration)
{
  PlaceOptions options;
  const PlaceResult result = replace_.doPlace(1, options);

  expectConsecutive(result.trace, 0, result.final_iteration);
  EXPECT_EQ(result.trace.back().overflow, result.overflow);
  for (int i = 0; i + 1 < std::ssize(result.trace); ++i) {
    EXPECT_GT(result.trace[i].overflow, options.overflow)
        << "iteration " << result.trace[i].iteration;
  }
  for (const PlaceIteration& it : result.trace) {
    EXPECT_GT(it.hpwl, 0) << "iteration " << it.iteration;
    EXPECT_GT(it.density_penalty, 0) << "iteration " << it.iteration;
  }
  EXPECT_GT(result.trace.back().density_penalty,
            result.trace.front().density_penalty);
}

// A run that runs out of iterations stops at the limit, one past the last
// iteration it ran, and reports the overflow it reached.
TEST_F(PlaceResultTest, IterationLimitStopsOnePastTheTrace)
{
  PlaceOptions options;
  options.nesterovPlaceMaxIter = 5;
  const PlaceResult result = replace_.doPlace(1, options);

  EXPECT_EQ(result.stop, PlaceStop::kIterationLimit);
  EXPECT_EQ(result.final_iteration, 5);
  expectConsecutive(result.trace, 0, 4);
  EXPECT_GT(result.overflow, options.overflow);
  EXPECT_EQ(result.overflow, result.trace.back().overflow);
  EXPECT_EQ(result.hpwl, odb::WireLengthEvaluator(block_).hpwl());
}

TEST_F(PlaceResultTest, IncrementalWithEverythingPlacedGivesAnEmptyResult)
{
  replace_.doPlace(1, PlaceOptions{});

  const PlaceResult result = freshPlacer().doIncrementalPlace(1);

  EXPECT_EQ(result.stop, PlaceStop::kNotRun);
  EXPECT_EQ(result.final_iteration, 0);
  EXPECT_TRUE(result.trace.empty());
}

// Incremental placement places the unplaced cells, and its result describes
// the final placement whether or not it needed its second phase.
TEST_F(PlaceResultTest, IncrementalReportsTheFinalPlacement)
{
  PlaceOptions options;
  replace_.doPlace(1, options);
  int unplaced = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (unplaced < 20 && !inst->isFixed()) {
      inst->setPlacementStatus(odb::dbPlacementStatus::NONE);
      ++unplaced;
    }
  }
  ASSERT_EQ(unplaced, 20);

  const PlaceResult result = freshPlacer().doIncrementalPlace(1, options);

  EXPECT_EQ(result.stop, PlaceStop::kConverged);
  ASSERT_FALSE(result.trace.empty());
  for (int i = 0; i + 1 < std::ssize(result.trace); ++i) {
    EXPECT_LT(result.trace[i].iteration, result.trace[i + 1].iteration);
  }
  EXPECT_LE(result.overflow, options.overflow);
  EXPECT_EQ(result.hpwl, odb::WireLengthEvaluator(block_).hpwl());
}

TEST_F(PlaceResultTest, NoPlaceableInstancesGivesAnEmptyResult)
{
  for (odb::dbInst* inst : block_->getInsts()) {
    inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }
  const PlaceResult result = replace_.doNesterovPlace(1);

  EXPECT_EQ(result.stop, PlaceStop::kNotRun);
  EXPECT_EQ(result.final_iteration, 0);
  EXPECT_TRUE(result.trace.empty());
}

}  // namespace
}  // namespace gpl
