// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>

#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/gpl/src/timingBase.h"
#include "sta/MinMax.hh"
#include "tst/IntegratedFixture.h"

namespace gpl {
namespace {

class IncrementalTimingTest : public tst::IntegratedFixture
{
 public:
  IncrementalTimingTest()
      : IntegratedFixture(Technology::kNangate45, "_main/src/gpl/test/")
  {
  }

 protected:
  void SetUp() override
  {
    readVerilogAndSetup("incremental_timing.v", false);
    for (auto inst : block_->getInsts()) {
      inst->setLocation(20000, 20000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    buffer_ = block_->findInst("buffer0");
    ASSERT_NE(buffer_, nullptr);
    setWireRC(1e8, 2e-10);
  }

  void setWireRC(double resistance, double capacitance)
  {
    for (auto scene : sta_->scenes()) {
      ep_.setHWireSignalRC(nullptr, scene, resistance, capacitance);
      ep_.setVWireSignalRC(nullptr, scene, resistance, capacitance);
      ep_.setHWireClkRC(nullptr, scene, resistance, capacitance);
      ep_.setVWireClkRC(nullptr, scene, resistance, capacitance);
    }
  }

  void estimate() { ep_.estimateParasitics(est::ParasiticsSrc::kPlacement); }

  float worstSlack() { return sta_->worstSlack(sta::MinMax::max()); }
  float tns() { return sta_->totalNegativeSlack(sta::MinMax::max()); }

  odb::dbInst* buffer_ = nullptr;
};

TEST_F(IncrementalTimingTest, RejectsHarmfulMoveAndRefreshesTiming)
{
  initStaDefaultSdc();
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  const float baseline = worstSlack();
  const float baseline_tns = tns();
  auto master = buffer_->getMaster();
  const auto count = block_->getInsts().size();

  buffer_->setLocation(2000000, 20000);
  estimate();
  ASSERT_LT(worstSlack(), baseline - 1e-10);
  ASSERT_LT(tns(), baseline_tns);
  EXPECT_TRUE(guard.restoreIfWorse());
  EXPECT_EQ(buffer_->getLocation().x(), 20000);
  EXPECT_EQ(buffer_->getLocation().y(), 20000);
  EXPECT_NEAR(worstSlack(), baseline, 1e-13);
  EXPECT_NEAR(tns(), baseline_tns, 1e-13);
  EXPECT_EQ(buffer_->getMaster(), master);
  EXPECT_EQ(block_->getInsts().size(), count);
  EXPECT_EQ(buffer_->getPlacementStatus(), odb::dbPlacementStatus::PLACED);
  EXPECT_FALSE(guard.restoreIfWorse());
}

TEST_F(IncrementalTimingTest, KeepsImprovedPlacement)
{
  buffer_->setLocation(2000000, 20000);
  initStaDefaultSdc();
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  const float baseline = worstSlack();
  const float baseline_tns = tns();
  buffer_->setLocation(20000, 20000);
  estimate();
  ASSERT_GT(worstSlack(), baseline + 1e-10);
  ASSERT_GT(tns(), baseline_tns);
  EXPECT_FALSE(guard.restoreIfWorse());
  EXPECT_EQ(buffer_->getLocation().x(), 20000);
}

TEST_F(IncrementalTimingTest, RejectsTnsRegressionWithUnchangedWorstSlack)
{
  buffer_->setLocation(4000000, 20000);
  initStaDefaultSdc();
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  const float baseline = worstSlack();
  const float baseline_tns = tns();
  auto other_buffer = block_->findInst("buffer1");
  ASSERT_NE(other_buffer, nullptr);
  other_buffer->setLocation(2000000, 20000);
  estimate();
  ASSERT_NEAR(worstSlack(), baseline, 1e-13);
  ASSERT_LT(tns(), baseline_tns - 1e-10);
  EXPECT_TRUE(guard.restoreIfWorse());
  EXPECT_EQ(other_buffer->getLocation().x(), 20000);
  EXPECT_NEAR(tns(), baseline_tns, 1e-13);
}

TEST_F(IncrementalTimingTest, UnchangedPlacementIsAccepted)
{
  initStaDefaultSdc();
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  EXPECT_FALSE(guard.restoreIfWorse());
}

TEST_F(IncrementalTimingTest, UnconstrainedDesignIsNotGuarded)
{
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  EXPECT_FALSE(guard.checkpoint());
  EXPECT_FALSE(guard.restoreIfWorse());
}

TEST_F(IncrementalTimingTest, MissingWireRCIsNotGuarded)
{
  initStaDefaultSdc();
  setWireRC(0.0, 0.0);
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  EXPECT_FALSE(guard.checkpoint());
}

}  // namespace
}  // namespace gpl
