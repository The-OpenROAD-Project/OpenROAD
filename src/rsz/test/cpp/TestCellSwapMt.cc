// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// VtSwapMt, SizeUpMt and SwapPinsMt moves.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, SwapPinsMtCandidateChargesDisplacedSignal)
{
  resizer_.runRepairSetupPreamble();
  Target target = makeTarget("path_out", "path_target", "ZN");
  sta::Instance* nand = db_network_->dbToSta(block_->findInst("path_target"));
  sta::LibertyCell* cell = sta_->network()->libertyCell(nand);
  ASSERT_NE(cell, nullptr);
  sta::LibertyPort* a1 = cell->findLibertyPort("A1");
  sta::LibertyPort* a2 = cell->findLibertyPort("A2");
  sta::LibertyPort* zn = cell->findLibertyPort("ZN");
  ASSERT_TRUE(a1 != nullptr && a2 != nullptr && zn != nullptr);
  const std::shared_ptr<const SwapPinsMtInput> input
      = std::make_shared<const SwapPinsMtInput>(SwapPinsMtInput{
          .input_port = a1, .input_delay = 50e-12f, .equivalents = {}});

  // Without a displaced signal the score is the plain arc delay difference.
  SwapPinsMtCandidate plain(
      resizer_, target, input, {.port = a2, .delay = 40e-12f});
  const Estimate plain_estimate = plain.estimate();
  EXPECT_TRUE(plain_estimate.legal);
  EXPECT_FLOAT_EQ(plain_estimate.score, 10e-12f);
  EXPECT_FALSE(plain_estimate.side_loss.has_value());

  // The side model reads its floor, margin and weight from the context.
  target.estimator_context = EstimatorContext{};
  target.estimator_context->side_slack_floor = -100e-12f;
  target.estimator_context->side_floor_margin = 1.0f;
  target.estimator_context->fanin_side_loss_weight = 1.0f;
  const sta::Pin* victim = sta_->network()->findPin(nand, a2);
  ASSERT_NE(victim, nullptr);

  // A positive-slack victim keeps the whole gain.
  SwapPinsMtCandidate safe(
      resizer_,
      target,
      input,
      {.port = a2, .delay = 40e-12f, .pin = victim, .slack = 20e-12f});
  const Estimate safe_estimate = safe.estimate();
  EXPECT_TRUE(safe_estimate.legal);
  EXPECT_FLOAT_EQ(safe_estimate.score, 10e-12f);
  EXPECT_FLOAT_EQ(*safe_estimate.side_loss, 0.0f);

  // A negative-slack victim pays the full gain back, so the swap is rejected.
  SwapPinsMtCandidate wash(
      resizer_,
      target,
      input,
      {.port = a2, .delay = 40e-12f, .pin = victim, .slack = -5e-12f});
  const Estimate wash_estimate = wash.estimate();
  EXPECT_FALSE(wash_estimate.legal);
  EXPECT_FLOAT_EQ(*wash_estimate.side_loss, 10e-12f);
  EXPECT_EQ(wash_estimate.reason, std::string("side_loss_non_improving"));

  // A victim dropping below the worst-slack floor is rejected even with gain.
  target.estimator_context->side_slack_floor = 0.0f;
  target.estimator_context->side_floor_margin = 0.0f;
  SwapPinsMtCandidate floored(
      resizer_,
      target,
      input,
      {.port = a2, .delay = 40e-12f, .pin = victim, .slack = 5e-12f});
  EXPECT_FALSE(floored.estimate().legal);
  target.estimator_context->side_floor_margin = 1.0f;
  EXPECT_TRUE(floored.estimate().legal);

  // Batch impact names the instance and both input pins for conflict checks.
  const MoveImpact impact = plain.impact();
  EXPECT_TRUE(impact.complete);
  ASSERT_EQ(impact.write_instances.size(), 1u);
  EXPECT_EQ(impact.write_instances.front(), nand);
  EXPECT_EQ(impact.reconnect_pins.size(), 2u);
  EXPECT_EQ(impact.timing_effect_nets.size(), 3u);
}

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
  generator.prepareInputs(target);
  const CandidateVector candidates = generator.generate(target);
  EXPECT_EQ(candidates.size(), first.cells.size());
  EXPECT_EQ(generator.size_up_options_.size(), 1u);
}

}  // namespace rsz
