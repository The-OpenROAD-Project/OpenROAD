// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// VtSwapMt and SizeUpMt moves.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, SizeUpMtCandidateAppliesScoreScale)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> estimator_context
      = DelayEstimator::buildContext(resizer, target, 0, &fail_reason);
  ASSERT_TRUE(estimator_context.has_value()) << failReasonName(fail_reason);
  if (!estimator_context.has_value()) {
    return;
  }
  target.estimator_context = estimator_context;

  sta::Instance* target_inst = target.inst(resizer);
  ASSERT_NE(target_inst, nullptr);

  sta::LibertyCell* replacement_cell
      = sta_->network()->findLibertyCell("BUF_X16");
  ASSERT_NE(replacement_cell, nullptr);

  const DelayEstimate raw_estimate
      = DelayEstimator::estimate(estimator_context.value(), replacement_cell);
  ASSERT_TRUE(raw_estimate.legal);

  SizeUpMtCandidate default_scale_candidate(
      resizer, target, target_inst, replacement_cell, 1.0f);
  const Estimate default_scale = default_scale_candidate.estimate();
  EXPECT_EQ(default_scale.legal, raw_estimate.legal);
  EXPECT_FLOAT_EQ(default_scale.score, raw_estimate.arrival_impr);

  SizeUpMtCandidate half_scale_candidate(
      resizer, target, target_inst, replacement_cell, 0.5f);
  const Estimate half_scale = half_scale_candidate.estimate();
  EXPECT_EQ(half_scale.legal, raw_estimate.legal);
  EXPECT_FLOAT_EQ(half_scale.score, raw_estimate.arrival_impr * 0.5f);
}

TEST_F(TestResizerMt, DualRfDriveGuardRequiresBothTransitions)
{
  // A stronger buffer dominates on both transitions, a weaker one on neither,
  // and a port dominates itself.
  sta::LibertyPort* weak
      = sta_->network()->findLibertyCell("BUF_X1")->findLibertyPort("Z");
  sta::LibertyPort* strong
      = sta_->network()->findLibertyCell("BUF_X16")->findLibertyPort("Z");
  EXPECT_TRUE(driveResNoHigherOnBothRf(weak, strong));
  EXPECT_FALSE(driveResNoHigherOnBothRf(strong, weak));
  EXPECT_TRUE(driveResNoHigherOnBothRf(weak, weak));
}

TEST_F(TestResizerMt, SizeUpMtGeneratorCachesLibraryOptionsPerDriverPort)
{
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  OptimizerRunConfig run_config;
  OptimizationPolicyConfig policy_config;
  SizeUpMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));

  Target target = makeTarget("out0", "target", "Z");
  target.estimator_context = DelayEstimator::buildContext(resizer_, target, 0);
  ASSERT_TRUE(target.estimator_context.has_value());
  const SelectedArc& arc = target.estimator_context->target().arc;

  const SizeUpMtGenerator::SizeUpOptions& first
      = generator.findSizeUpOptions(arc.outputPort(), arc.scene, arc.min_max);
  const SizeUpMtGenerator::SizeUpOptions& second
      = generator.findSizeUpOptions(arc.outputPort(), arc.scene, arc.min_max);
  EXPECT_EQ(&first, &second);
  EXPECT_EQ(generator.size_up_options_.size(), 1u);
  EXPECT_FALSE(first.cells.empty());
  EXPECT_EQ(
      first.cells,
      generator.buildSizeUpOptions(arc.outputPort(), arc.scene, arc.min_max)
          .cells);

  // Generation consumes the cached options without changing the candidates;
  // generate() reads the prepared fanin headroom.
  target.input_cap_budgets = resizer_.inputCapBudgets(target.inst(resizer_));
  const CandidateVector candidates = generator.generate(target);
  EXPECT_EQ(candidates.size(), first.cells.size());
  EXPECT_EQ(generator.size_up_options_.size(), 1u);
}

}  // namespace rsz
