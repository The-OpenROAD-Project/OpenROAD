// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <limits>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/gpl/src/timingBase.h"
#include "sta/Fuzzy.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "tst/IntegratedFixture.h"
#include "tst/fixture.h"

namespace gpl {
namespace {

class IncrementalTimingNoLibertyTest : public tst::Fixture
{
};

TEST_F(IncrementalTimingNoLibertyTest, DoesNotQueryTimingOrParasitics)
{
  ASSERT_EQ(sta_->getDbNetwork()->defaultLibertyLibrary(), nullptr);
  // Missing timing libraries must be rejected before consulting the block
  // or parasitics, or entering a timing query that requires Liberty.
  IncrementalTimingGuard guard(sta_.get(), nullptr, nullptr);
  EXPECT_FALSE(guard.checkpoint());
  EXPECT_FALSE(guard.restoreIfWorse());
}

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
  const float trial_ws = worstSlack();
  const float trial_tns = tns();
  EXPECT_TRUE(guard.restoreIfWorse());
  EXPECT_FLOAT_EQ(guard.lockedTiming().worst_slack, baseline);
  EXPECT_FLOAT_EQ(guard.lockedTiming().total_negative_slack, baseline_tns);
  EXPECT_FLOAT_EQ(guard.trialTiming().worst_slack, trial_ws);
  EXPECT_FLOAT_EQ(guard.trialTiming().total_negative_slack, trial_tns);
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

TEST_F(IncrementalTimingTest, InvalidWireRCIsNotGuarded)
{
  initStaDefaultSdc();
  for (double invalid : {0.0,
                         -1.0,
                         std::numeric_limits<double>::quiet_NaN(),
                         std::numeric_limits<double>::infinity()}) {
    IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
    setWireRC(invalid, 2e-10);
    EXPECT_FALSE(guard.checkpoint());
    setWireRC(1e8, invalid);
    EXPECT_FALSE(guard.checkpoint());
  }
}

TEST_F(IncrementalTimingTest, AcceptRejectDecisionsAgreeAcrossThreadCounts)
{
  initStaDefaultSdc();
  for (int threads : {1, 2, 4}) {
    SCOPED_TRACE(threads);
    sta_->setThreadCount(threads);
    buffer_->setLocation(20000, 20000);
    IncrementalTimingGuard reject(sta_.get(), &ep_, block_);
    ASSERT_TRUE(reject.checkpoint());
    buffer_->setLocation(2000000, 20000);
    EXPECT_TRUE(reject.restoreIfWorse());
    EXPECT_EQ(buffer_->getLocation().x(), 20000);

    buffer_->setLocation(2000000, 20000);
    IncrementalTimingGuard accept(sta_.get(), &ep_, block_);
    ASSERT_TRUE(accept.checkpoint());
    buffer_->setLocation(20000, 20000);
    EXPECT_FALSE(accept.restoreIfWorse());
    EXPECT_EQ(buffer_->getLocation().x(), 20000);
  }
}

TEST(IncrementalTimingMetrics, InvalidAggregatesAreRejected)
{
  const IncrementalTimingMetrics baseline{-1e-9f, -2e-9f};
  for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity(),
                        sta::INF,
                        -sta::INF}) {
    for (const auto candidate : {IncrementalTimingMetrics{invalid, -2e-9f},
                                 IncrementalTimingMetrics{-1e-9f, invalid}}) {
      EXPECT_FALSE(candidate.isValid());
      EXPECT_TRUE(candidate.degrades(baseline));
      EXPECT_TRUE(baseline.degrades(candidate));
    }
  }
  EXPECT_FALSE(baseline.degrades(baseline));
}

class IncrementalMulticlockTest : public IncrementalTimingTest
{
 protected:
  void SetUp() override
  {
    readVerilogAndSetup("incremental_timing_multiclock.v", false);
    for (auto inst : block_->getInsts()) {
      inst->setLocation(20000, 20000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    buffer_ = block_->findInst("buffer0");
    ASSERT_NE(buffer_, nullptr);
    setWireRC(1e8, 2e-10);
  }

  void clock(const char* port, const char* name, float period)
  {
    sta::PinSet pins(db_network_);
    pins.insert(db_network_->dbToSta(block_->findBTerm(port)));
    sta::FloatSeq waveform{0.0f, period / 2};
    sta_->makeClock(name, pins, false, period, waveform, "", sta_->cmdMode());
  }

  float netSlack(const char* name)
  {
    return sta_->slack(db_network_->dbToSta(block_->findNet(name)),
                       sta::MinMax::max());
  }
};

TEST_F(IncrementalMulticlockTest, UnconstrainedConeDoesNotDisableGuard)
{
  clock("clk", "first", 0.5e-9f);
  clock("clk_other", "second", 0.8e-9f);
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  ASSERT_TRUE(sta::fuzzyInf(netSlack("async_net")));
  const float ws = worstSlack();
  const float total = tns();
  auto unconstrained = block_->findInst("unconstrained_buffer");
  ASSERT_NE(unconstrained, nullptr);
  unconstrained->setLocation(2000000, 20000);
  EXPECT_FALSE(guard.restoreIfWorse());
  EXPECT_FLOAT_EQ(worstSlack(), ws);
  EXPECT_FLOAT_EQ(tns(), total);
  EXPECT_EQ(unconstrained->getLocation().x(), 2000000);
}

TEST_F(IncrementalMulticlockTest, SecondClockTnsRegressionIsRejected)
{
  clock("clk", "first", 0.5e-9f);
  clock("clk_other", "second", 0.8e-9f);
  buffer_->setLocation(4000000, 20000);
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  const float ws = worstSlack();
  const float total = tns();
  auto second = block_->findInst("buffer1");
  ASSERT_NE(second, nullptr);
  second->setLocation(2000000, 20000);
  estimate();
  ASSERT_NEAR(worstSlack(), ws, 1e-13);
  ASSERT_LT(tns(), total - 1e-10);
  EXPECT_TRUE(guard.restoreIfWorse());
  EXPECT_EQ(second->getLocation().x(), 20000);
  EXPECT_NEAR(tns(), total, 1e-13);
}

TEST_F(IncrementalMulticlockTest, AggregateProtectionIsNotAPerClockGuarantee)
{
  clock("clk", "first", 0.5e-9f);
  clock("clk_other", "second", 1e-6f);
  buffer_->setLocation(4000000, 20000);
  IncrementalTimingGuard guard(sta_.get(), &ep_, block_);
  ASSERT_TRUE(guard.checkpoint());
  const float ws = worstSlack();
  const float total = tns();
  const float second_slack = netSlack("d1");
  auto second = block_->findInst("buffer1");
  ASSERT_NE(second, nullptr);
  second->setLocation(2000000, 20000);
  estimate();
  ASSERT_GT(netSlack("d1"), 0.0f);
  ASSERT_LT(netSlack("d1"), second_slack - 1e-10);
  ASSERT_NEAR(worstSlack(), ws, 1e-13);
  ASSERT_NEAR(tns(), total, 1e-13);
  EXPECT_FALSE(guard.restoreIfWorse());
  EXPECT_EQ(second->getLocation().x(), 2000000);
}

}  // namespace
}  // namespace gpl
