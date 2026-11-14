// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <vector>

#include "../../src/SACoreHardMacro.h"
#include "../../src/clusterEngine.h"
#include "../../src/mpl-util.h"
#include "../../src/object.h"
#include "MplTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace mpl {
namespace {

// Exercises SACoreHardMacro's cost/weight contracts entirely through its
// public API (construction, setWeights(), getNormCost(), initialize()).
// No golden numeric values are asserted; see docs/agents/testing-strategy.md.
class TestSACorePenalties : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    // calAreaPenalty() converts through dbuAreaToMicrons(), which divides by
    // dbu-per-micron squared; it defaults to 0 and MplTest never sets it
    // since no other mpl test exercises that conversion.
    db_->setDbuPerMicron(1000);
    tree_.die_area = odb::Rect(0, 0, die_width_, die_height_);
  }

  odb::dbBlock* block() { return db_->getChip()->getBlock(); }

  std::unique_ptr<SACoreHardMacro> makeCore(const SACoreWeights& weights)
  {
    const std::vector<HardMacro> macros{
        HardMacro(macro_width_, macro_height_, "macro1"),
        HardMacro(macro_width_, macro_height_, "macro2"),
    };

    return std::make_unique<SACoreHardMacro>(&tree_,
                                             outline_,
                                             macros,
                                             weights,
                                             /*pos_swap_prob=*/0.25,
                                             /*neg_swap_prob=*/0.25,
                                             /*double_swap_prob=*/0.25,
                                             /*exchange_prob=*/0.25,
                                             /*init_prob=*/0.1,
                                             /*max_num_step=*/1,
                                             /*num_perturb_per_step=*/10,
                                             /*seed=*/42,
                                             /*graphics=*/nullptr,
                                             &logger_,
                                             block());
  }

  PhysicalHierarchy tree_;
  const odb::Rect outline_{0, 0, 100000, 100000};
  const int macro_width_ = 20000;
  const int macro_height_ = 20000;
};

TEST_F(TestSACorePenalties, ConstructorStoresAllMacros)
{
  const std::unique_ptr<SACoreHardMacro> core = makeCore(SACoreWeights{});

  EXPECT_EQ(core->getMacros().size(), 2);
}

// With every penalty weight at zero, the weighted sum has nothing to
// contribute regardless of what the individual penalty values settle on.
TEST_F(TestSACorePenalties, AllZeroWeightsYieldZeroCost)
{
  const std::unique_ptr<SACoreHardMacro> core = makeCore(SACoreWeights{});
  core->initialize();

  EXPECT_EQ(core->getNormCost(), 0.0f);
}

// setWeights() re-applied with the same values it was constructed with
// must not perturb the cost it already reported.
TEST_F(TestSACorePenalties, ReapplyingSameWeightsLeavesCostUnchanged)
{
  const SACoreWeights weights{.area = 1.0f,
                              .outline = 1.0f,
                              .wirelength = 1.0f,
                              .guidance = 1.0f,
                              .fence = 1.0f};
  const std::unique_ptr<SACoreHardMacro> core = makeCore(weights);
  core->initialize();

  const float cost_before = core->getNormCost();
  core->setWeights(weights);

  EXPECT_EQ(core->getNormCost(), cost_before);
}

// Starting from an all-zero-weight (and so zero-cost) core, giving the area
// penalty a positive weight must make it a positive contributor: area is
// never weight-gated during computation (unlike wirelength/guidance/fence),
// and a placed macro's bounding box is always non-zero area.
TEST_F(TestSACorePenalties, GivingAZeroedWeightAPositiveValueMovesCostOffZero)
{
  const std::unique_ptr<SACoreHardMacro> core = makeCore(SACoreWeights{});
  core->initialize();
  ASSERT_EQ(core->getNormCost(), 0.0f);

  core->setWeights(SACoreWeights{.area = 1.0f});

  EXPECT_GT(core->getNormCost(), 0.0f);
}

// Whatever footprint the core settles on, it must consider itself as fitting
// in an outline of exactly that size.
TEST_F(TestSACorePenalties, FitsWithinItsOwnReportedDimensions)
{
  const std::unique_ptr<SACoreHardMacro> core
      = makeCore(SACoreWeights{.area = 1.0f});
  core->initialize();

  EXPECT_TRUE(
      core->fitsIn(odb::Rect(0, 0, core->getWidth(), core->getHeight())));
}

}  // namespace
}  // namespace mpl
