// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Statetable-modelled registers (e.g. MBFF libraries without ff/ff_bank
// groups) and the SequentialTimingEstimator.

#include "TestResizerMtFixture.hh"
// The fixture header must come first; see its comment.
#include "sta/Sequential.hh"

namespace rsz {

// TestResizerMtSequential plus statetable_dff.lib: STDFF_X1/STDFF_X2,
// STDFFR_X1 and STDLH_X1 are Nangate45 DFF_X1/DFF_X2/DFFR_X1/DLH_X1 with the
// ff/latch group replaced by a statetable; MB2STDFF_X1 is a 2-bit MBFF.
class TestResizerMtStatetable : public TestResizerMtSequential
{
 protected:
  TestResizerMtStatetable()
  {
    readLiberty(getFilePath("_main/src/rsz/test/statetable_dff.lib"));
    if (!updateLib(lib_, "_main/src/rsz/test/statetable_dff.lef")) {
      throw std::runtime_error("failed to load statetable_dff.lef");
    }
  }

  sta::LibertyCell* cell(const char* name) const
  {
    return db_network_->findLibertyCell(name);
  }

  static bool hasRole(const sta::LibertyCell* cell, const sta::TimingRole* role)
  {
    for (const sta::TimingArcSet* arc_set : cell->timingArcSets()) {
      if (arc_set->role()->genericRole() == role) {
        return true;
      }
    }
    return false;
  }
};

TEST_F(TestResizerMtStatetable, RegisterCellReadsStatetables)
{
  // Statetable registers have no ff/ff_bank groups, so sequentials() is
  // empty; the old isFlipFlop test rejected every one of them.
  for (const char* name :
       {"STDFF_X1", "STDFF_X2", "STDFFR_X1", "MB2STDFF_X1"}) {
    const sta::LibertyCell* reg = cell(name);
    ASSERT_NE(reg, nullptr) << name;
    ASSERT_NE(reg->statetable(), nullptr) << name;
    EXPECT_TRUE(reg->sequentials().empty()) << name;
    EXPECT_TRUE(statetableIsEdgeTriggered(*reg->statetable())) << name;
    EXPECT_TRUE(isRegisterCell(reg)) << name;
  }

  // Without read_liberty -infer_latches a statetable latch's enable arc is
  // regClkToQ like a flip-flop's, so only the table tells them apart.
  const sta::LibertyCell* latch = cell("STDLH_X1");
  ASSERT_NE(latch, nullptr);
  ASSERT_NE(latch->statetable(), nullptr);
  EXPECT_TRUE(hasRole(latch, sta::TimingRole::regClkToQ()));
  EXPECT_FALSE(statetableIsEdgeTriggered(*latch->statetable()));
  EXPECT_FALSE(isRegisterCell(latch));

  // Nangate45's integrated clock gate is a statetable latch.
  const sta::LibertyCell* clock_gate = cell("CLKGATE_X1");
  ASSERT_NE(clock_gate, nullptr);
  ASSERT_NE(clock_gate->statetable(), nullptr);
  EXPECT_FALSE(isRegisterCell(clock_gate));

  // ff/latch-group cells keep their classification.
  EXPECT_TRUE(isRegisterCell(cell("DFF_X1")));
  EXPECT_TRUE(isRegisterCell(cell("DFFR_X1")));
  EXPECT_FALSE(isRegisterCell(cell("DLH_X1")));
  EXPECT_FALSE(isRegisterCell(cell("BUF_X1")));
  EXPECT_FALSE(isRegisterCell(nullptr));
}

TEST_F(TestResizerMtStatetable, StatetableFlipFlopGetsSequentialEstimator)
{
  prepareFlipFlop("STDFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = cell("STDFF_X1");
  sta::LibertyCell* candidate = cell("STDFF_X2");
  ASSERT_NE(current, nullptr);
  ASSERT_NE(candidate, nullptr);

  // Statetable cells are size equivalents like their ff-group originals.
  const sta::LibertyCellSeq swappable = resizer_.getSwappableCells(current);
  EXPECT_NE(std::ranges::find(swappable, candidate), swappable.end());

  setDataSlack(5e-12, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  const Target target = makeTarget("out0", "dq_seq", "Q");

  // Before the fix prepare() returned null here: no estimator, so a Q-path
  // swap was scored without the D-side setup/hold effects.
  const std::shared_ptr<const SequentialTimingEstimator> estimator
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(estimator, nullptr);
  const SequentialTimingEstimator::Result identity
      = estimator->estimate(current);
  ASSERT_TRUE(identity.legal) << identity.reason;
  EXPECT_FLOAT_EQ(identity.weighted_side_slack_loss, 0.0f);
  EXPECT_FLOAT_EQ(identity.max_side_slack_loss, 0.0f);

  // Same timing as DFF_X1 -> DFF_X2 in
  // FlipFlopSetupPenaltyUsesCapturedDataMargin.
  const SequentialTimingEstimator::Result swap = estimator->estimate(candidate);
  ASSERT_TRUE(swap.legal) << swap.reason;
  EXPECT_GT(swap.max_side_slack_loss, 0.0f);
}

TEST_F(TestResizerMtStatetable, StatetableLatchGetsNoSequentialEstimator)
{
  prepareSequential("STDLH_X1", "G");
  ASSERT_FALSE(HasFatalFailure());
  const Target latch = makeTarget("out0", "dq_seq", "Q");
  EXPECT_EQ(SequentialTimingEstimator::prepare(resizer_, latch), nullptr);
}

TEST_F(TestResizerMtSequential, FaninSideLossSkipsOwnInputsScoredSequentially)
{
  prepareFlipFlop("DFF_X2");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X2");
  sta::LibertyCell* larger_d = db_network_->findLibertyCell("DFF_X1");
  ASSERT_NE(larger_d, nullptr);
  ASSERT_GT(larger_d->findLibertyPort("D")->capacitance(),
            current->findLibertyPort("D")->capacitance());

  setDataSlack(-5e-12, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  const Target target = makeTarget("out0", "dq_seq", "Q");

  const FaninSideLoadOptions all_critical{};
  FailReason fail_reason = FailReason::kNone;
  std::optional<EstimatorContext> context = DelayEstimator::buildContext(
      resizer_, target, 0, &fail_reason, false, false, nullptr, &all_critical);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  EXPECT_FALSE(context->skip_target_side_loads);

  // CK -> Q is the selected arc, so the D net is a side net and dq_seq/D, the
  // register's own setup-checked input, is one of its loads.
  const sta::Pin* own_d = db_network_->findPin("dq_seq/D");
  bool found_own_d = false;
  for (const FaninSideNet& side_net : context->fanin_side_nets) {
    for (const SideLoad& load : side_net.loads) {
      if (load.pin == own_d) {
        found_own_d = true;
        EXPECT_TRUE(load.on_target);
        EXPECT_LT(load.slack, 0.0f);
      } else {
        EXPECT_FALSE(load.on_target);
      }
    }
  }
  ASSERT_TRUE(found_own_d);

  // Without a sequential estimator the delay estimator still charges it.
  const DelayEstimate charged = DelayEstimator::estimate(*context, larger_d);
  EXPECT_GT(charged.side_paths_evaluated, 0);
  EXPECT_GT(charged.side_loss, 0.0f);

  // The sequential estimator scores dq_seq/D (data delay from the larger cap
  // and the setup margin), so the fanin side loss must not charge it again.
  ASSERT_NE(SequentialTimingEstimator::prepare(resizer_, target), nullptr);
  context->skip_target_side_loads = true;
  const DelayEstimate skipped = DelayEstimator::estimate(*context, larger_d);
  EXPECT_EQ(skipped.side_paths_evaluated, 0);
  EXPECT_FLOAT_EQ(skipped.side_loss, 0.0f);
  EXPECT_FLOAT_EQ(skipped.arrival_impr, charged.arrival_impr);
}

}  // namespace rsz
