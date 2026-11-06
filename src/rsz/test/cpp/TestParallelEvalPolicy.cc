// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// PARALLEL_EVAL policy: presets, batch control, stop rules, selection
// and cone weights.

#include <utility>

#include "TestResizerMtFixture.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Transition.hh"

namespace rsz {

TEST_F(TestResizerMt,
       SetupParallelEvalPolicyPreservesEstimatesAcrossThreadCounts)
{
  ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  resizer_.runRepairSetupPreamble();
  std::vector<std::vector<Estimate>> reference;
  for (const int threads : {1, 4}) {
    sta_->setThreadCount(threads);
    MoveCommitter committer(resizer_);
    RepairSetupContext setup_context(resizer_);
    OptimizerRunConfig config;
    config.setup_slack_margin = 1.0;
    SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
    ASSERT_TRUE(policy.start());
    std::vector<Target> targets{makeTarget("out0", "target", "Z"),
                                makeTarget("path_out", "path_pre0", "Z"),
                                makeTarget("path_out", "path_target", "ZN")};
    ResizerTestPeer::prewarmTargets(policy, targets);
    ResizerTestPeer::prepareTargets(policy, targets);

    std::vector<Target*> scored;
    for (Target& target : targets) {
      scored.push_back(&target);
    }

    std::vector<TargetEvaluation> evaluations
        = ResizerTestPeer::generateCandidates(policy, scored);
    ResizerTestPeer::estimateTargets(policy, evaluations);
    for (size_t index = 0; index < evaluations.size(); ++index) {
      const std::vector<Estimate>& estimates = evaluations[index].estimates;
      ASSERT_FALSE(estimates.empty());
      if (threads == 1) {
        reference.push_back(estimates);
        continue;
      }

      ASSERT_EQ(estimates.size(), reference[index].size());
      for (size_t move = 0; move < estimates.size(); ++move) {
        EXPECT_EQ(estimates[move].legal, reference[index][move].legal);
        EXPECT_EQ(estimates[move].reason, reference[index][move].reason);
        EXPECT_EQ(estimates[move].raw_gain, reference[index][move].raw_gain);
        EXPECT_EQ(estimates[move].score, reference[index][move].score);
      }
    }
  }
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyCountsWorkerPlannerWarnings)
{
  // Placed loads give RebufferMt a Steiner tree to plan on.
  int x = 1000;
  for (odb::dbInst* inst : block_->getInsts()) {
    inst->setLocation(x, 1000);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    x += 20000;
  }
  ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.estimateWireParasitics();
  resizer_.runRepairSetupPreamble();
  // No buffer tree meets a 1 fs design slew limit, so the tree planning of
  // RebufferMt fails with RSZ-2009 and RSZ-2021.
  sta::Network* network = sta_->network();
  sta_->setSlewLimit(network->cell(network->topInstance()),
                     sta::MinMax::max(),
                     1e-15,
                     sta_->cmdMode()->sdc());

  for (const int threads : {1, 4}) {
    sta_->setThreadCount(threads);
    MoveCommitter committer(resizer_);
    RepairSetupContext setup_context(resizer_);
    OptimizerRunConfig config;
    config.sequence = {MoveType::kBuffer};
    SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
    ASSERT_TRUE(policy.start());
    std::vector<Target> targets{makeTarget("out0", "target", "Z")};
    targets.front().fanout
        = ResizerTestPeer::targetFanout(policy, targets.front());
    ResizerTestPeer::prewarmTargets(policy, targets);
    ResizerTestPeer::prepareTargets(policy, targets);

    logger_.redirectStringBegin();
    ResizerTestPeer::generateCandidates(policy, {&targets.front()});
    const std::string generation_log = logger_.redirectStringEnd();
    logger_.redirectStringBegin();
    policy.finish();
    const std::string finish_log = logger_.redirectStringEnd();

    if (threads == 1) {
      // The main thread plans and logs each warning.
      EXPECT_NE(generation_log.find("[WARNING RSZ-2009]"), std::string::npos);
      EXPECT_NE(generation_log.find("[WARNING RSZ-2021]"), std::string::npos);
      EXPECT_EQ(finish_log, "");
    } else {
      // A worker planner counts its warnings, and the phase end logs one line.
      EXPECT_EQ(generation_log, "");
      EXPECT_EQ(finish_log,
                "[WARNING RSZ-2034] 2 RebufferMt planning warnings on worker "
                "threads were not logged: RSZ-2009 x1, RSZ-2021 x1. Run with "
                "set_thread_count 1 to log them.\n");
    }
  }
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyDefaultSequenceIsFiltered)
{
  // The default set contains RebufferMt, whose planning needs wire RC.
  ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);

  // One VT category drops the default VtSwapMt, as it drops a requested one.
  // repair_setup_parallel_eval_default_sequence covers the multi-VT case.
  ASSERT_EQ(resizer_.vtCategoryCount(), 1);

  OptimizerRunConfig config;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());
  const std::vector<MoveType> without_vt_swap{MoveType::kUnbuffer,
                                              MoveType::kSizeUp,
                                              MoveType::kBuffer,
                                              MoveType::kSplitLoad};
  EXPECT_EQ(ResizerTestPeer::moveSequence(policy), without_vt_swap);

  OptimizerRunConfig requested;
  requested.sequence = {MoveType::kVtSwap, MoveType::kSizeUp};
  SetupParallelEvalPolicy explicit_policy(
      resizer_, committer, setup_context, requested);
  ASSERT_TRUE(explicit_policy.start());
  const std::vector<MoveType> size_up_only{MoveType::kSizeUp};
  EXPECT_EQ(ResizerTestPeer::moveSequence(explicit_policy), size_up_only);

  // -skip_buffering removes the default RebufferMt and SplitLoadMt the same
  // way.
  OptimizerRunConfig skipped;
  skipped.skip_buffering = true;
  SetupParallelEvalPolicy filtered(resizer_, committer, setup_context, skipped);
  ASSERT_TRUE(filtered.start());
  const std::vector<MoveType> unbuffer_and_size_up{MoveType::kUnbuffer,
                                                   MoveType::kSizeUp};
  EXPECT_EQ(ResizerTestPeer::moveSequence(filtered), unbuffer_and_size_up);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyAcceptsSwapPinsInSequence)
{
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.sequence = {MoveType::kSwapPins, MoveType::kSizeUp};
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());
  ASSERT_EQ(ResizerTestPeer::moveSequence(policy).size(), 2u);
  EXPECT_EQ(ResizerTestPeer::moveSequence(policy).front(), MoveType::kSwapPins);
  ASSERT_EQ(ResizerTestPeer::moveGenerators(policy).size(), 2u);
  EXPECT_EQ(ResizerTestPeer::moveGenerators(policy).front()->type(),
            MoveType::kSwapPins);
}

namespace {

// Move types of the generators a PARALLEL_EVAL run holds, in order.
std::vector<MoveType> generatorTypes(const SetupParallelEvalPolicy& policy)
{
  std::vector<MoveType> types;
  for (const std::unique_ptr<MoveGenerator>& generator :
       ResizerTestPeer::moveGenerators(policy)) {
    types.push_back(generator->type());
  }
  return types;
}

}  // namespace

TEST_F(TestResizerMt, SetupParallelEvalPolicyWnsTailHonorsSequenceAndSkips)
{
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);

  // -skip_buffering keeps RebufferMt, which needs wire RC, out of every run
  // below.  The default sequence then holds UnbufferMt and SizeUpMt.
  OptimizerRunConfig base;
  base.skip_buffering = true;
  const std::vector<MoveType> size_up_only{MoveType::kSizeUp};

  // Under the default sequence the tail adds its pin swaps and size down.
  SetupParallelEvalPolicy default_policy(
      resizer_, committer, setup_context, base);
  ASSERT_TRUE(default_policy.start());
  ASSERT_TRUE(ResizerTestPeer::startWnsTail(default_policy, "test"));
  const std::vector<MoveType> default_tail{MoveType::kUnbuffer,
                                           MoveType::kSizeUp,
                                           MoveType::kSwapPins,
                                           MoveType::kSizeDownFanout};
  EXPECT_EQ(generatorTypes(default_policy), default_tail);

  // The -skip_* options keep each of them out.
  OptimizerRunConfig skipped = base;
  skipped.skip_pin_swap = true;
  skipped.skip_buffer_removal = true;
  skipped.skip_size_down_fanout = true;
  SetupParallelEvalPolicy skipped_policy(
      resizer_, committer, setup_context, skipped);
  ASSERT_TRUE(skipped_policy.start());
  ASSERT_TRUE(ResizerTestPeer::startWnsTail(skipped_policy, "test"));
  EXPECT_EQ(generatorTypes(skipped_policy), size_up_only);

  // An explicit -sequence is the complete move set, tail included.
  OptimizerRunConfig requested = base;
  requested.sequence = {MoveType::kSizeUp};
  SetupParallelEvalPolicy requested_policy(
      resizer_, committer, setup_context, requested);
  ASSERT_TRUE(requested_policy.start());
  ASSERT_TRUE(ResizerTestPeer::startWnsTail(requested_policy, "test"));
  EXPECT_EQ(generatorTypes(requested_policy), size_up_only);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyRepairTnsLimitsTargetEndpoints)
{
  // Every output port violates.
  resizer_.runRepairSetupPreamble();
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  for (const char* port : {"out0", "out1", "out2", "path_out"}) {
    sta_->setOutputDelay(db_network_->findPin(port),
                         sta::RiseFallBoth::riseFall(),
                         sdc->findClock("clk"),
                         sta::RiseFall::rise(),
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         false,
                         1e-9,
                         sdc);
  }

  resizer_.updateParasiticsAndTiming();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.skip_buffering = true;

  // The default -repair_tns of 100% targets every violating endpoint.
  SetupParallelEvalPolicy all_policy(
      resizer_, committer, setup_context, config);
  ASSERT_TRUE(all_policy.start());
  const size_t failing_endpoint_count
      = ResizerTestPeer::collectTargetEndpoints(all_policy);
  ASSERT_GE(failing_endpoint_count, 4u);

  // A smaller share targets the worst endpoints only, as many as LEGACY.
  config.repair_tns_end_percent = 0.5;
  SetupParallelEvalPolicy half_policy(
      resizer_, committer, setup_context, config);
  ASSERT_TRUE(half_policy.start());
  EXPECT_EQ(ResizerTestPeer::collectTargetEndpoints(half_policy),
            failing_endpoint_count / 2);

  // -repair_tns 0 still repairs the worst endpoint.
  config.repair_tns_end_percent = 0.0;
  SetupParallelEvalPolicy worst_policy(
      resizer_, committer, setup_context, config);
  ASSERT_TRUE(worst_policy.start());
  EXPECT_EQ(ResizerTestPeer::collectTargetEndpoints(worst_policy), 1u);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyHonorsMaxUtilization)
{
  // -max_utilization is a share of the core area, which the fixture lacks.
  const odb::Rect core(0, 0, 200000, 200000);
  block_->setDieArea(core);
  block_->setCoreArea(core);
  prepareUniqueViolatingEndpoint();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.skip_buffering = true;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);

  // Below -max_utilization, every batch is legal.
  EXPECT_TRUE(ResizerTestPeer::withinMaxUtilization(policy));

  // A -max_utilization the design already exceeds fails every batch, and the
  // run stops before it commits any move.
  resizer_.setMaxUtilization(1.0e-6);
  ASSERT_TRUE(resizer_.overMaxArea());
  EXPECT_FALSE(ResizerTestPeer::withinMaxUtilization(policy));
  ASSERT_TRUE(policy.start());
  policy.iterate();
  EXPECT_TRUE(policy.converged());
  EXPECT_EQ(ResizerTestPeer::committedMoves(policy), 0);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyHasNoUtilizationCap)
{
  // A core far smaller than the design puts utilization way above 100%.
  const odb::Rect core(0, 0, 1000, 1000);
  block_->setDieArea(core);
  block_->setCoreArea(core);
  prepareUniqueViolatingEndpoint();
  ASSERT_GT(resizer_.utilization(), 1.0);
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.skip_buffering = true;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);

  // Without -max_utilization, no utilization rejects a batch.
  EXPECT_TRUE(ResizerTestPeer::withinMaxUtilization(policy));
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyAreaStopCountsARemoval)
{
  // -max_utilization is a share of the core area, which the fixture lacks.
  const odb::Rect core(0, 0, 200000, 200000);
  block_->setDieArea(core);
  block_->setCoreArea(core);
  prepareUniqueViolatingEndpoint();
  Target removal = makeTarget("path_out", "path_pre1", "Z");
  Target upsize = makeTarget("out2", "target", "Z");
  sta::LibertyCell* larger = db_network_->findLibertyCell("BUF_X2");
  ASSERT_NE(larger, nullptr);

  // Put the limit half an upsize above the current area, which counts the LEF
  // master areas.
  auto master_area = [this](const char* name) {
    const odb::dbMaster* master = db_->findMaster(name);
    return resizer_.dbuToMeters(master->getWidth())
           * resizer_.dbuToMeters(master->getHeight());
  };
  const double area = resizer_.designArea();
  const double upsize_area = master_area("BUF_X2") - master_area("BUF_X1");
  resizer_.setMaxUtilization((area + 0.5 * upsize_area) / resizer_.coreArea());
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.sequence = {MoveType::kUnbuffer, MoveType::kSizeUp};
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());

  // A requested unbuffer move shares a batch with an upsize.  The removal frees
  // more area than the upsize takes, so the design stays below the limit and
  // the run goes on.
  ResizerTestPeer::openBatchJournal(policy);
  UnbufferMtCandidate unbuffer(resizer_, removal);
  SizeUpMtCandidate size_up(
      resizer_, upsize, upsize.inst(resizer_), larger, 1.0f);
  ASSERT_TRUE(committer.commit(unbuffer).accepted);
  ASSERT_TRUE(committer.commit(size_up).accepted);
  EXPECT_FALSE(resizer_.overMaxArea());
  EXPECT_TRUE(ResizerTestPeer::withinMaxUtilization(policy));
  EXPECT_FALSE(ResizerTestPeer::finishAtMaxArea(policy));
  EXPECT_FALSE(policy.converged());

  // A limit the batch really crosses ends the run at the checkpoint, which
  // undoes the open batch.
  resizer_.setMaxUtilization(0.5 * area / resizer_.coreArea());
  EXPECT_TRUE(ResizerTestPeer::finishAtMaxArea(policy));
  EXPECT_TRUE(policy.converged());
  EXPECT_FALSE(ResizerTestPeer::batchJournalOpen(policy));
  EXPECT_NE(block_->findInst("path_pre1"), nullptr);
  EXPECT_EQ(block_->findInst("target")->getMaster(), db_->findMaster("BUF_X1"));
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyRollbackRecountsDesignArea)
{
  // -max_utilization is a share of the core area, which the fixture lacks.
  const odb::Rect core(0, 0, 200000, 200000);
  block_->setDieArea(core);
  block_->setCoreArea(core);
  prepareUniqueViolatingEndpoint();
  sta::Instance* target = db_network_->findInstance("target");
  sta::LibertyCell* original = db_network_->libertyCell(target);
  sta::LibertyCell* larger = db_network_->findLibertyCell("BUF_X16");

  // Put the -max_utilization limit just above the current area.
  const double area = resizer_.designArea();
  resizer_.setMaxUtilization(area * 1.001 / resizer_.coreArea());
  ASSERT_FALSE(resizer_.overMaxArea());

  // Undoing a batch that crossed the limit puts the design back below it, so
  // later phases and the final report do not see the undone upsize.
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.skip_buffering = true;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());
  ResizerTestPeer::openBatchJournal(policy);
  ASSERT_TRUE(resizer_.replaceCell(target, larger));
  ASSERT_TRUE(resizer_.overMaxArea());
  ResizerTestPeer::undoBatch(policy);
  ASSERT_EQ(db_network_->libertyCell(target), original);
  EXPECT_FALSE(resizer_.overMaxArea());
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyEffortPresets)
{
  // The stop knobs a PARALLEL_EVAL run starts with for one effort.
  struct StopKnobs
  {
    int stall_max_batches;
    float progress_tns_pct;
    float progress_wns_period_pct;
    int endpoints;
    int max_endpoints;
    float work_pct;
    float min_work;
  };

  const std::function<StopKnobs(RepairEffort)> knobs
      = [](const RepairEffort effort) {
          const ParallelEvalPolicyConfig tuned
              = OptimizationPolicyConfig::forParallelEval(effort).parallel_eval;
          return StopKnobs{tuned.stall_max_batches,
                           tuned.progress_tns_pct,
                           tuned.progress_wns_period_pct,
                           tuned.wns_tail_endpoints,
                           tuned.wns_tail_max_endpoints,
                           tuned.wns_tail_work_pct,
                           tuned.wns_tail_min_work};
        };

  // The preset values are tuning, so only their shape is checked: high is
  // the default and the reference, and the others only stop sooner.
  EXPECT_EQ(OptimizerRunConfig{}.effort, RepairEffort::kHigh);
  const StopKnobs high = knobs(RepairEffort::kHigh);
  EXPECT_GT(high.endpoints, 0);

  // medium keeps the tail, loosens nothing, and tightens at least one rule
  // on the batch side and one on the tail side.
  const StopKnobs medium = knobs(RepairEffort::kMedium);
  EXPECT_EQ(medium.endpoints, high.endpoints);
  EXPECT_LE(medium.stall_max_batches, high.stall_max_batches);
  EXPECT_GE(medium.progress_tns_pct, high.progress_tns_pct);
  EXPECT_GE(medium.progress_wns_period_pct, high.progress_wns_period_pct);
  EXPECT_LE(medium.max_endpoints, high.max_endpoints);
  EXPECT_LE(medium.work_pct, high.work_pct);
  EXPECT_LE(medium.min_work, high.min_work);
  EXPECT_TRUE(medium.stall_max_batches < high.stall_max_batches
              || medium.progress_tns_pct > high.progress_tns_pct);
  EXPECT_TRUE(medium.max_endpoints < high.max_endpoints
              || medium.work_pct < high.work_pct
              || medium.min_work < high.min_work);

  // low never starts the tail and leaves the batch stall as high has it.
  const StopKnobs low = knobs(RepairEffort::kLow);
  EXPECT_EQ(low.endpoints, 0);
  EXPECT_EQ(low.stall_max_batches, high.stall_max_batches);
  EXPECT_FLOAT_EQ(low.progress_tns_pct, high.progress_tns_pct);
  EXPECT_FLOAT_EQ(low.progress_wns_period_pct, high.progress_wns_period_pct);

  EXPECT_EQ(resizer_.repairEffortFromString("low"), RepairEffort::kLow);
  EXPECT_EQ(resizer_.repairEffortFromString("medium"), RepairEffort::kMedium);
  EXPECT_EQ(resizer_.repairEffortFromString("high"), RepairEffort::kHigh);
  EXPECT_THROW(resizer_.repairEffortFromString("max"), std::runtime_error);
}

TEST_F(TestResizerMtSequential,
       SequentialPolicyOffParityAndCandidateIntegration)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  setDataSlack(5e-12, sta::MinMax::max());
  setDataSlack(5e-12, sta::MinMax::min());
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X1");
  sta::LibertyCell* larger = db_network_->findLibertyCell("DFF_X2");
  sta::LibertyCell* low_vt = db_network_->findLibertyCell("DFF_X1_L");
  ASSERT_NE(larger, nullptr);
  ASSERT_NE(low_vt, nullptr);
  copyFlipFlopChecks(current, low_vt);
  ASSERT_FALSE(HasFatalFailure());

  // Isolate a small D-load loss while retaining the candidate's faster Q model.
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    for (const sta::MinMax* min_max : sta::MinMax::range()) {
      low_vt->findLibertyPort("D")->setCapacitance(
          rf,
          min_max,
          current->findLibertyPort("D")->capacitance(rf, min_max) * 1.05f);
    }
  }

  for (const bool enabled : {true, false}) {
    SCOPED_TRACE(enabled ? "sequential" : "combinational");
    MoveCommitter committer(resizer_);
    RepairSetupContext setup_context(resizer_);
    OptimizerRunConfig config;
    config.setup_slack_margin = 1.0;
    config.sequence = {MoveType::kSizeUp};
    SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
    ASSERT_TRUE(policy.start());
    ResizerTestPeer::policyConfig(policy).delay_estimation_levels = 0;
    Target target = makeTarget("out0", "dq_seq", "Q");
    ResizerTestPeer::prepareTarget(
        policy, target, kEstimatorContextCache | kSequentialEstimatorCache);
    ASSERT_TRUE(target.estimator_context.has_value());
    ASSERT_NE(target.sequential_timing_estimator, nullptr);
    const std::shared_ptr<const SequentialTimingEstimator> prepared
        = target.sequential_timing_estimator;
    ResizerTestPeer::prepareTarget(
        policy, target, kEstimatorContextCache | kSequentialEstimatorCache);
    EXPECT_EQ(target.sequential_timing_estimator, prepared);
    // A target without the sequential estimator scores the plain window.
    if (!enabled) {
      target.sequential_timing_estimator = nullptr;
    }
    const std::optional<EstimatorContext> baseline
        = DelayEstimator::buildContext(resizer_, target, 0);
    ASSERT_TRUE(baseline.has_value());
    expectSameDelayContext(*target.estimator_context, *baseline);

    for (const bool size_up : {true, false}) {
      SCOPED_TRACE(size_up ? "size_up" : "vt_swap");
      sta::LibertyCell* replacement = size_up ? larger : low_vt;
      const float scale = size_up ? 0.5f : 1.0f;
      const DelayEstimate raw
          = DelayEstimator::estimate(*target.estimator_context, replacement);
      ASSERT_TRUE(raw.legal) << failReasonName(raw.reason);
      SizeUpMtCandidate size(
          resizer_, target, target.inst(resizer_), replacement, scale);
      VtSwapMtCandidate vt(
          resizer_, target, target.inst(resizer_), replacement);
      const Estimate estimate = size_up ? size.estimate() : vt.estimate();
      ASSERT_TRUE(estimate.legal) << estimate.reason;
      ASSERT_TRUE(estimate.raw_gain.has_value());
      EXPECT_FLOAT_EQ(*estimate.raw_gain, raw.arrival_impr);
      if (!enabled) {
        EXPECT_FALSE(estimate.sequential_penalty.has_value());
        EXPECT_FALSE(estimate.sequential_side_loss.has_value());
        EXPECT_FLOAT_EQ(estimate.score, raw.arrival_impr * scale);
        continue;
      }

      const SequentialTimingEstimator::Result side
          = prepared->estimate(replacement);
      ASSERT_TRUE(side.legal) << side.reason;
      ASSERT_GT(side.weighted_side_slack_loss, 0.0f);
      ASSERT_TRUE(estimate.sequential_penalty.has_value());
      ASSERT_TRUE(estimate.sequential_side_loss.has_value());
      EXPECT_FLOAT_EQ(*estimate.sequential_penalty,
                      side.weighted_side_slack_loss);
      EXPECT_FLOAT_EQ(*estimate.sequential_side_loss, side.max_side_slack_loss);
      const float adjusted_gain
          = raw.arrival_impr - side.weighted_side_slack_loss;
      EXPECT_FLOAT_EQ(estimate.score, adjusted_gain * scale);
    }
  }

  EXPECT_EQ(block_->findInst("dq_seq")->getMaster(), db_->findMaster("DFF_X1"));
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyAdaptiveBatchControlsAndRetry)
{
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ResizerTestPeer::policyConfig(policy).parallel_eval.min_batch_cap = 256;
  ResizerTestPeer::policyConfig(policy).parallel_eval.retry_batch_cap = 8;
  ResizerTestPeer::startSmallBatchRetry(policy);
  std::vector<BatchCandidate> candidates(12);
  for (size_t index = 0; index < candidates.size(); ++index) {
    candidates[index].evaluation_index = index;
    candidates[index].local_score = index == 0 ? 100.0f : 1.0f;
    candidates[index].priority_score = candidates[index].local_score;
    candidates[index].impact.complete = true;
  }

  EXPECT_TRUE(ResizerTestPeer::smallBatchRetryStarted(policy));

  // The retry ignores adaptive growth and keeps its small fixed cap.
  EXPECT_EQ(ResizerTestPeer::effectiveBatchCap(policy, 100000), 8);
  EXPECT_EQ(ResizerTestPeer::batchSelector(policy)
                .select(candidates,
                        ResizerTestPeer::effectiveBatchCap(policy, 100000),
                        false)
                .evaluation_indices.size(),
            8);
  ResizerTestPeer::smallBatchRetryStarted(policy) = false;
  EXPECT_EQ(ResizerTestPeer::effectiveBatchCap(policy, 100000),
            ResizerTestPeer::policyConfig(policy).parallel_eval.max_batch_cap);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyStallBudgetCountsMoves)
{
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ParallelEvalPolicyConfig& parallel_eval
      = ResizerTestPeer::policyConfig(policy).parallel_eval;
  parallel_eval.min_batch_cap = 64;
  parallel_eval.stall_batches = 6;

  // Budget: stall iterations worth of full batches.
  EXPECT_EQ(ResizerTestPeer::stall(policy).moveBudget(), 384);

  // A grown batch scales the budget so one missed batch cannot spend it.
  ResizerTestPeer::lastCap(ResizerTestPeer::stall(policy)) = 512;
  EXPECT_EQ(ResizerTestPeer::stall(policy).moveBudget(), 3072);

  // A small retry batch keeps the configured size as the floor.
  ResizerTestPeer::lastCap(ResizerTestPeer::stall(policy)) = 8;
  EXPECT_EQ(ResizerTestPeer::stall(policy).moveBudget(), 384);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyRetriesWhenNoMoveIsKept)
{
  prepareUniqueViolatingEndpoint();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.skip_buffering = true;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  sta::Instance* target = db_network_->findInstance("target");

  // Missed batches of 512 moves spend the stall budget of three batches
  // before the miss limit of five; the retry runs batches of four moves.
  ParallelEvalPolicyConfig& parallel_eval
      = ResizerTestPeer::policyConfig(policy).parallel_eval;
  parallel_eval.min_batch_cap = 64;
  parallel_eval.stall_batches = 3;
  parallel_eval.stall_max_batches = 24;
  parallel_eval.max_checkpoint_misses = 5;
  parallel_eval.retry_batch_cap = 4;

  // Each batch commits `cap` moves and misses the checkpoint.
  const auto miss_batches = [&](const int count, const int cap) {
    for (int batch = 0; batch < count; ++batch) {
      ResizerTestPeer::stall(policy).beginBatch(cap, 1000);
      ResizerTestPeer::stall(policy).endBatch(cap);
      ResizerTestPeer::noteMissedBatch(policy, true);
      ResizerTestPeer::applyStopRules(policy, cap);
    }
  };

  // A run that kept a move hands the batches to the WNS tail at the third
  // miss, which spends the stall budget.
  ASSERT_TRUE(policy.start());
  ResizerTestPeer::committedMoves(policy) = 1;
  miss_batches(2, 512);
  EXPECT_FALSE(ResizerTestPeer::tail(policy).active());
  miss_batches(1, 512);
  EXPECT_FALSE(ResizerTestPeer::smallBatchRetryStarted(policy));
  EXPECT_TRUE(ResizerTestPeer::tail(policy).active());

  // A run that kept no move retries with small batches instead, with a fresh
  // stall budget and the tabu targets back.
  ASSERT_TRUE(policy.start());
  ResizerTestPeer::tabu(policy).addTabu(target,
                                        ResizerTestPeer::batchIndex(policy));
  miss_batches(3, 512);
  ASSERT_TRUE(ResizerTestPeer::smallBatchRetryStarted(policy));
  EXPECT_FALSE(ResizerTestPeer::tail(policy).active());
  EXPECT_FALSE(policy.converged());
  EXPECT_EQ(ResizerTestPeer::effectiveBatchCap(policy, 100000), 4);
  EXPECT_EQ(ResizerTestPeer::stall(policy).stall(), StallMonitor::Stall::kNone);
  EXPECT_FALSE(ResizerTestPeer::tabu(policy).isTabu(target));

  // The retry runs once: its fifth miss hands the run to the tail.
  miss_batches(4, 4);
  EXPECT_FALSE(ResizerTestPeer::tail(policy).active());
  miss_batches(1, 4);
  EXPECT_TRUE(ResizerTestPeer::tail(policy).active());
}

TEST(TestParallelEvalRules, ParallelEvalCheckpointPrioritizesWns)
{
  EXPECT_TRUE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -9.0f, -110.0f, -10.0f, 0.0f));
  EXPECT_FALSE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -11.0f, -90.0f, -10.0f, 0.0f));
  EXPECT_TRUE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -10.0f, -90.0f, -10.0f, 0.0f));
  EXPECT_FALSE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -10.0f, -110.0f, -10.0f, 0.0f));

  // Within the tolerance band below the best WNS, TNS progress is enough.
  EXPECT_TRUE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -10.05f, -90.0f, -10.0f, 0.1f));
  EXPECT_FALSE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -10.05f, -100.0f, -10.0f, 0.1f));
  EXPECT_FALSE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.0f, -100.0f, -10.2f, -90.0f, -10.0f, 0.1f));

  // The band is anchored on the best checkpoint, not the last one.
  EXPECT_FALSE(SetupParallelEvalPolicy::batchImprovesCheckpoint(
      -10.1f, -100.0f, -10.15f, -90.0f, -10.0f, 0.1f));
}

// The WNS progress step is a share of the reference clock period, so the
// same gain counts the same at any WNS, also near zero.
TEST(TestParallelEvalRules, ParallelEvalStallProgressFollowsThePeriod)
{
  const ParallelEvalPolicyConfig config;
  const float period = 500e-12f;
  const float step = period * config.progress_wns_period_pct / 100.0f;
  StallMonitor stall(config);
  stall.reset(-1e-9f, period);

  // TNS stays flat, so the WNS gain alone decides.
  for (const float wns : {-100e-12f, -1e-12f, -0.01e-12f}) {
    stall.onSaved(-1e-9f, wns, wns + 2.0f * step, -1e-9f, 0.0);
    EXPECT_FALSE(stall.wnsStalled()) << wns;
    stall.onSaved(-1e-9f, wns, wns + 0.5f * step, -1e-9f, 0.0);
    EXPECT_TRUE(stall.wnsStalled()) << wns;
  }

  // A shorter period lowers the step with it.
  stall.reset(-1e-9f, period / 10.0f);
  stall.onSaved(-1e-9f, -1e-12f, -1e-12f + 0.5f * step, -1e-9f, 0.0);
  EXPECT_FALSE(stall.wnsStalled());
}

namespace {

// True when the stall window stops the batches after saves that each gain
// `gain` from `wns`, with TNS and the failing endpoint count unchanged.
bool windowStallsAfterGains(const ParallelEvalPolicyConfig& config,
                            const float period,
                            const float wns,
                            const float gain)
{
  StallMonitor stall(config);
  stall.reset(-1e-9f, period);
  float best_wns = wns;
  for (int save = 0; save <= config.stall_window; ++save) {
    stall.beginBatch(config.min_batch_cap, 10);
    stall.onSaved(-1e-9f, best_wns, best_wns + gain, -1e-9f, 100.0 * save);
    best_wns += gain;
  }
  return stall.windowStalled();
}

}  // namespace

// The window weighs the average WNS gain against the reference clock period,
// so gains far below it stop the batches near WNS zero as well.
TEST(TestParallelEvalRules, ParallelEvalStallWindowFollowsThePeriod)
{
  const ParallelEvalPolicyConfig config;
  const float period = 500e-12f;
  const float step = period * config.stall_wns_period_pct / 100.0f;
  EXPECT_TRUE(windowStallsAfterGains(config, period, -0.5e-12f, 0.5f * step));
  EXPECT_TRUE(windowStallsAfterGains(config, period, -100e-12f, 0.5f * step));
  EXPECT_FALSE(windowStallsAfterGains(config, period, -0.5e-12f, 2.0f * step));
  EXPECT_FALSE(windowStallsAfterGains(config, period, -100e-12f, 2.0f * step));
}

// A tail batch is WNS progress only when it gains a share of the reference
// clock period, whatever the WNS.
TEST(TestParallelEvalRules, ParallelEvalTailGainFollowsThePeriod)
{
  const ParallelEvalPolicyConfig config;
  const float period = 500e-12f;
  const float min_gain = period * config.wns_tail_min_gain_period_pct / 100.0f;
  WnsTail tail(config);
  ASSERT_TRUE(tail.start(0.0, period));
  for (const float wns : {-100e-12f, -0.01e-12f}) {
    tail.afterBatch(wns, wns + 0.5f * min_gain, true);
    EXPECT_EQ(ResizerTestPeer::batchesWithoutGain(tail), 1) << wns;
    tail.afterBatch(wns, wns + 2.0f * min_gain, true);
    EXPECT_EQ(ResizerTestPeer::batchesWithoutGain(tail), 0) << wns;
  }
}

// The reference clock period is the shortest period of a clock with source
// pins, and it scales the checkpoint tolerance.  Without clocks the shortest
// set_max_delay sets it.
TEST_F(TestResizerMt, SetupParallelEvalPolicyReferencePeriod)
{
  prepareUniqueViolatingEndpoint();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.sequence = {MoveType::kSizeUp};
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());

  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  const float period = sdc->findClock("clk")->period();
  const ParallelEvalPolicyConfig& parallel_eval
      = ResizerTestPeer::policyConfig(policy).parallel_eval;
  EXPECT_FLOAT_EQ(ResizerTestPeer::referencePeriod(policy), period);
  EXPECT_FLOAT_EQ(
      ResizerTestPeer::checkpointWnsTolerance(policy),
      period * parallel_eval.checkpoint_wns_tolerance_period_pct / 100.0f);

  // A faster virtual clock times only IO constraints and does not count.
  const sta::FloatSeq waveform{0.0f, period / 8.0f};
  sta_->makeClock("vclk",
                  sta::PinSet(db_network_),
                  /*add_to_pins=*/false,
                  period / 4.0f,
                  waveform,
                  /*comment=*/"",
                  sta_->cmdMode());
  EXPECT_FLOAT_EQ(ResizerTestPeer::findReferencePeriod(policy, -1e-12f),
                  period);

  // Without a clock with source pins the virtual clock sets the scale, and
  // without any clock the initial WNS does.
  sta_->removeClock(sdc->findClock("clk"), sdc);
  EXPECT_FLOAT_EQ(ResizerTestPeer::findReferencePeriod(policy, -1e-12f),
                  period / 4.0f);
  sta_->removeClock(sdc->findClock("vclk"), sdc);
  EXPECT_FLOAT_EQ(ResizerTestPeer::findReferencePeriod(policy, -1e-12f),
                  1e-12f);

  // The shortest set_max_delay stands in for the period; a shorter
  // set_min_delay bounds hold and does not count.
  for (const sta::MinMax* min_max : {sta::MinMax::max(), sta::MinMax::min()}) {
    sta::PinSet* to_pins = new sta::PinSet(db_network_);
    to_pins->insert(db_network_->findPin("out2"));
    sta::ExceptionTo* to = sta_->makeExceptionTo(to_pins,
                                                 /*to_clks=*/nullptr,
                                                 /*to_insts=*/nullptr,
                                                 sta::RiseFallBoth::riseFall(),
                                                 sta::RiseFallBoth::riseFall(),
                                                 sdc);
    sta_->makePathDelay(
        /*from=*/nullptr,
        /*thrus=*/nullptr,
        to,
        min_max,
        /*ignore_clk_latency=*/false,
        /*break_path=*/false,
        min_max == sta::MinMax::max() ? period / 2.0f : period / 8.0f,
        /*comment=*/"",
        sdc);
  }
  EXPECT_FLOAT_EQ(ResizerTestPeer::findReferencePeriod(policy, -1e-12f),
                  period / 2.0f);

  // A clock outranks set_max_delay even when the max delay is shorter.
  sta::PinSet clk_pins(db_network_);
  clk_pins.insert(db_network_->findPin("clk"));
  sta_->makeClock("clk",
                  clk_pins,
                  /*add_to_pins=*/false,
                  period,
                  sta::FloatSeq{0.0f, period / 2.0f},
                  /*comment=*/"",
                  sta_->cmdMode());
  EXPECT_FLOAT_EQ(ResizerTestPeer::findReferencePeriod(policy, -1e-12f),
                  period);
}

// The tabu takes only instances a rollback kept: a deleted instance can hand
// its address to the next instance OpenDB creates.
TEST_F(TestResizerMt, ParallelEvalKeptInstancesSkipDeletedOnes)
{
  odb::dbInst* kept_db_inst = block_->findInst("target");
  ASSERT_NE(kept_db_inst, nullptr);
  odb::dbInst* deleted_db_inst
      = odb::dbInst::create(block_, db_->findMaster("BUF_X1"), "deleted_probe");
  ASSERT_NE(deleted_db_inst, nullptr);
  const std::vector<InstanceRecord> records{
      {.inst = db_network_->dbToSta(kept_db_inst), .name = "target"},
      {.inst = db_network_->dbToSta(deleted_db_inst), .name = "deleted_probe"}};

  // The next instance may take the address of the deleted one.
  odb::dbInst::destroy(deleted_db_inst);
  ASSERT_NE(odb::dbInst::create(block_, db_->findMaster("BUF_X1"), "new_probe"),
            nullptr);

  EXPECT_EQ(keptInstances(db_network_, records),
            std::vector<sta::Instance*>{records[0].inst});
}

// A new instance at the address of a deleted one does not inherit its tabu,
// undo count or exclusion.
TEST_F(TestResizerMt, ParallelEvalTabuSkipsReusedAddress)
{
  const ParallelEvalPolicyConfig config;
  ASSERT_GE(config.max_undos_per_instance, 2);
  MissTabu tabu(config);
  tabu.reset(db_network_);
  odb::dbMaster* buffer = db_->findMaster("BUF_X1");
  odb::dbInst* deleted_db_inst
      = odb::dbInst::create(block_, buffer, "deleted_probe");
  ASSERT_NE(deleted_db_inst, nullptr);
  sta::Instance* deleted = db_network_->dbToSta(deleted_db_inst);
  for (int undo = 0; undo < config.max_undos_per_instance; ++undo) {
    tabu.recordUndo(deleted, 0);
  }
  ASSERT_TRUE(tabu.isTabu(deleted));
  ASSERT_TRUE(tabu.isExcluded(deleted));

  // The table hands the freed slot to the next instance.
  odb::dbInst::destroy(deleted_db_inst);
  odb::dbInst* new_db_inst = odb::dbInst::create(block_, buffer, "new_probe");
  sta::Instance* reused = db_network_->dbToSta(new_db_inst);
  ASSERT_EQ(reused, deleted);
  EXPECT_FALSE(tabu.isTabu(reused));
  EXPECT_FALSE(tabu.isExcluded(reused));

  // Its undo count starts from zero.
  EXPECT_FALSE(tabu.recordUndo(reused, 1));
  EXPECT_TRUE(tabu.isTabu(reused));
  EXPECT_FALSE(tabu.isExcluded(reused));
}

namespace {

// The loads the RebufferMt candidate trees put behind a buffer, in order.
std::vector<const sta::Pin*> loadsBehindBuffers(
    const CandidateVector& candidates)
{
  std::vector<const sta::Pin*> loads;
  for (const std::unique_ptr<MoveCandidate>& candidate : candidates) {
    const RebufferTreePlan& plan
        = static_cast<const RebufferMtCandidate&>(*candidate).plan();
    for (const BufferTreeLoadPath& load_path : plan.load_paths) {
      if (std::ranges::any_of(
              load_path.stages, [](const BufferTreePathStage& stage) {
                return stage.type == BufferTreeStageType::kBuffer;
              })) {
        loads.push_back(load_path.load_pin);
      }
    }
  }
  return loads;
}

}  // namespace

// The tree export leaves a dont-touch load on the driver net, so RebufferMt
// makes no candidate from a tree that puts one behind a buffer.
TEST_F(TestResizerMt, RebufferMtDropsTreeWithDontTouchLoadBehindBuffer)
{
  // Placed loads give RebufferMt a Steiner tree to plan on.
  int x = 1000;
  for (odb::dbInst* inst : block_->getInsts()) {
    inst->setLocation(x, 1000);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    x += 200000;
  }
  ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.estimateWireParasitics();
  resizer_.runRepairSetupPreamble();

  MoveCommitter committer(resizer_);
  committer.init();
  OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config
      = OptimizationPolicyConfig::forParallelEval(RepairEffort::kHigh);
  RebufferMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));
  generator.startPhase(0);
  Target target = makeTarget("out0", "target", "Z");
  target.fanout = 3;
  target.estimator_context = DelayEstimator::buildContext(resizer_, target, 0);
  ASSERT_TRUE(generator.isApplicable(target));
  generator.prepareInputs(target);

  // Mark a load that a tree puts behind a buffer as dont-touch and plan again.
  const std::vector<const sta::Pin*> buffered
      = loadsBehindBuffers(generator.generate(target));
  ASSERT_FALSE(buffered.empty());
  const sta::Pin* load_pin = buffered.front();
  db_network_->staToDb(db_network_->instance(load_pin))->setDoNotTouch(true);
  const std::vector<const sta::Pin*> buffered_after
      = loadsBehindBuffers(generator.generate(target));
  generator.finishPhase();
  EXPECT_EQ(std::ranges::count(buffered_after, load_pin), 0);
}

TEST_F(TestResizerMt, BottleneckAnalysisWeighsTheViolatingCone)
{
  resizer_.runRepairSetupPreamble();
  // A margin above every slack makes every endpoint violating.
  const float margin = 1.0f;
  std::vector<BottleneckAnalysis::EndpointSlack> endpoints;
  for (sta::Vertex* endpoint : sta_->endpoints()) {
    endpoints.emplace_back(endpoint->pin(),
                           sta_->slack(endpoint, sta::MinMax::max()));
  }
  std::ranges::sort(endpoints,
                    [](const BottleneckAnalysis::EndpointSlack& lhs,
                       const BottleneckAnalysis::EndpointSlack& rhs) {
                      return lhs.second < rhs.second;
                    });
  ASSERT_FALSE(endpoints.empty());

  BottleneckAnalysis analysis(sta_.get(), sta::MinMax::max());
  analysis.analyze(endpoints, margin, 1024, 1024);
  EXPECT_GT(analysis.coneSize(), endpoints.size());

  // The driver on out0's worst path sits in the cone, so its rank grows by
  // 1 + bottleneck_scale * severity_ratio^bottleneck_exponent,
  // with severity_ratio in (0, 1].
  const BatchPriorityConfig priority{.bottleneck_scale = 2.0f,
                                     .bottleneck_exponent = 2.0f};
  const Target target = makeTarget("out0", "target", "Z");
  const float driver = analysis.bottleneckPriorityScore(
      1.0f, target.vertex(resizer_), priority);
  EXPECT_GT(driver, 1.0f);
  EXPECT_LE(driver, 3.0f);

  // A vertex outside the cone keeps its local score.
  EXPECT_FLOAT_EQ(analysis.bottleneckPriorityScore(5.0f, nullptr, priority),
                  5.0f);
}

// Vertex ids start at 1, so the newest vertex has an id equal to the vertex
// count.  A buffer inserted in the violating cone gets its cone weight.
TEST_F(TestResizerMt, BottleneckAnalysisWeighsTheNewestVertex)
{
  resizer_.runRepairSetupPreamble();
  sta::Instance* buffer = resizer_.insertBufferAfterDriver(
      db_network_->findNet("n1"), db_network_->findLibertyCell("BUF_X1"));
  ASSERT_NE(buffer, nullptr);
  resizer_.updateParasiticsAndTiming();

  sta::Graph* graph = sta_->graph();
  sta::Vertex* newest = nullptr;
  sta::VertexIterator vertex_iter(graph);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    if (newest == nullptr || graph->id(vertex) > graph->id(newest)) {
      newest = vertex;
    }
  }
  ASSERT_NE(newest, nullptr);
  ASSERT_EQ(db_network_->instance(newest->pin()), buffer);
  ASSERT_GE(graph->id(newest), graph->vertexCount());

  // A margin above every slack makes every endpoint violating.
  std::vector<BottleneckAnalysis::EndpointSlack> endpoints;
  for (sta::Vertex* endpoint : sta_->endpoints()) {
    endpoints.emplace_back(endpoint->pin(),
                           sta_->slack(endpoint, sta::MinMax::max()));
  }
  BottleneckAnalysis analysis(sta_.get(), sta::MinMax::max());
  analysis.analyze(endpoints, 1.0f, 1024, 1024);
  const BatchPriorityConfig priority{.bottleneck_scale = 2.0f,
                                     .bottleneck_exponent = 2.0f};
  EXPECT_GT(analysis.bottleneckPriorityScore(1.0f, newest, priority), 1.0f);
}

// Levelization skips a disabled preset/clear arc, so RN can sit above Q.  The
// cone skips that arc too: RN reaches only its own recovery check, not the
// endpoints behind Q.
TEST_F(TestResizerMtSequential, BottleneckAnalysisSkipsPresetClearArcs)
{
  prepareSequential("DFFR_X1", "CK");
  ASSERT_FALSE(HasFatalFailure());
  block_->findInst("dq_seq")->findITerm("RN")->connect(
      block_->findNet("path_n3"));
  ep_.estimateWireParasitics();
  sta_->updateTiming(true);
  ASSERT_FALSE(sta_->presetClrArcsEnabled());

  sta::Graph* graph = sta_->graph();
  sta::Vertex* reset = graph->pinLoadVertex(db_network_->findPin("dq_seq/RN"));
  sta::Vertex* q = graph->pinDrvrVertex(db_network_->findPin("dq_seq/Q"));
  ASSERT_NE(reset, nullptr);
  ASSERT_NE(q, nullptr);
  ASSERT_GT(reset->level(), q->level());

  // A margin above every slack makes every endpoint violating.
  std::vector<BottleneckAnalysis::EndpointSlack> endpoints;
  for (sta::Vertex* endpoint : sta_->endpoints()) {
    endpoints.emplace_back(endpoint->pin(),
                           sta_->slack(endpoint, sta::MinMax::max()));
  }
  BottleneckAnalysis analysis(sta_.get(), sta::MinMax::max());
  analysis.analyze(endpoints, 1.0f, 1024, 1024);

  const std::optional<ResizerTestPeer::ConeDeficits> reset_deficits
      = ResizerTestPeer::coneDeficits(analysis, reset);
  const std::optional<ResizerTestPeer::ConeDeficits> q_deficits
      = ResizerTestPeer::coneDeficits(analysis, q);
  ASSERT_TRUE(reset_deficits.has_value());
  ASSERT_TRUE(q_deficits.has_value());
  EXPECT_GT(q_deficits->endpoint_deficit_sum, 0.0f);
  EXPECT_FLOAT_EQ(reset_deficits->endpoint_deficit_sum,
                  reset_deficits->slack_deficit);
}

// -max_iterations ends the run in the WNS tail as well.
TEST_F(TestResizerMt, ParallelEvalMaxIterationsEndsTheTail)
{
  prepareUniqueViolatingEndpoint();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.max_iterations = 2;
  config.sequence = {MoveType::kSizeUp};
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  policy.start();
  ASSERT_FALSE(policy.converged());
  ASSERT_TRUE(ResizerTestPeer::tail(policy).start(0.0, 0.0f));

  ResizerTestPeer::batchIndex(policy) = config.max_iterations;
  ResizerTestPeer::applyStopRules(policy, 1);
  EXPECT_FALSE(ResizerTestPeer::tail(policy).active());
  EXPECT_TRUE(policy.converged());
}

TEST(TestParallelEvalRules, ParallelEvalWnsTailStopPrecedence)
{
  const ParallelEvalPolicyConfig config;
  WnsTail tail(config);
  ASSERT_TRUE(tail.start(100.0, 0.0f));
  const char* reason = nullptr;
  EXPECT_EQ(tail.nextStep(1, 100.0, reason), WnsTail::Step::kContinue);
  EXPECT_EQ(reason, nullptr);

  // The work budget alone ends the tail.
  EXPECT_EQ(tail.nextStep(1, 1000.0, reason), WnsTail::Step::kStop);
  EXPECT_STREQ(reason, "wns_tail_work_budget");

  // A stall masks the spent budget and first retries with speculation.
  ResizerTestPeer::batchesWithoutGain(tail) = config.wns_tail_stall;
  EXPECT_EQ(tail.nextStep(1, 1000.0, reason), WnsTail::Step::kSpeculate);
  EXPECT_STREQ(reason, "wns_tail_stalled");

  // No committed move masks the stall, so the stage widens instead.
  EXPECT_EQ(tail.nextStep(0, 1000.0, reason), WnsTail::Step::kWiden);
  EXPECT_STREQ(reason, "wns_tail_no_committed_moves");

  // After the speculative stage, a stall widens.
  ResizerTestPeer::greedyStalled(tail) = true;
  EXPECT_EQ(tail.nextStep(1, 100.0, reason), WnsTail::Step::kWiden);

  // The batch limit ends the tail whatever rule fired first.
  ResizerTestPeer::batches(tail) = config.wns_tail_max_batches;
  EXPECT_EQ(tail.nextStep(1, 100.0, reason), WnsTail::Step::kStop);
  EXPECT_STREQ(reason, "wns_tail_stalled");

  // At the widest stage, a stall ends the tail.
  ResizerTestPeer::batches(tail) = 0;
  ResizerTestPeer::endpoints(tail) = config.wns_tail_max_endpoints;
  EXPECT_EQ(tail.nextStep(1, 100.0, reason), WnsTail::Step::kStop);
}

TEST(TestParallelEvalRules, ParallelEvalAdaptiveBatchSize)
{
  // The configured cap is the floor, the failing endpoint count grows the
  // batch and the adaptive maximum bounds it.
  const size_t max_failing_endpoints = std::numeric_limits<size_t>::max();
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(0, 64, 512), 64);
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(64, 64, 512), 64);
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(65, 64, 512), 65);
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(457, 64, 512), 457);
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(1826, 64, 512), 512);
  EXPECT_EQ(SetupParallelEvalPolicy::adaptiveBatchSize(
                max_failing_endpoints, 64, 512),
            512);
}

TEST(TestBatchSelector, BatchSelectorDefersForwardDependency)
{
  sta::Instance* inst1 = reinterpret_cast<sta::Instance*>(0x201);
  sta::Instance* inst2 = reinterpret_cast<sta::Instance*>(0x202);
  sta::Instance* inst3 = reinterpret_cast<sta::Instance*>(0x203);
  sta::Net* net1 = reinterpret_cast<sta::Net*>(0x301);
  sta::Net* net2 = reinterpret_cast<sta::Net*>(0x302);
  sta::Net* net3 = reinterpret_cast<sta::Net*>(0x303);

  // impact1 reads net1, which impact0 writes, so it waits.  impact2 reads
  // net3, which impact3 writes, but the check is one way, so impact3 still
  // joins the batch after impact2.
  MoveImpact impact0;
  impact0.complete = true;
  impact0.write_instances = {inst1};
  impact0.timing_effect_nets = {net1};
  MoveImpact impact1;
  impact1.complete = true;
  impact1.write_instances = {inst2};
  impact1.timing_dependency_nets = {net1};
  MoveImpact impact2;
  impact2.complete = true;
  impact2.write_instances = {inst2};
  impact2.timing_effect_nets = {net2};
  impact2.timing_dependency_nets = {net3};
  MoveImpact impact3;
  impact3.complete = true;
  impact3.write_instances = {inst3};
  impact3.timing_effect_nets = {net3};

  std::vector<BatchCandidate> candidates;
  candidates.reserve(4);
  candidates.push_back({.evaluation_index = 0,
                        .local_score = 10.0f,
                        .priority_score = 10.0f,
                        .impact = std::move(impact0)});
  candidates.push_back({.evaluation_index = 1,
                        .local_score = 9.0f,
                        .priority_score = 9.0f,
                        .impact = std::move(impact1)});
  candidates.push_back({.evaluation_index = 2,
                        .local_score = 8.0f,
                        .priority_score = 8.0f,
                        .impact = std::move(impact2)});
  candidates.push_back({.evaluation_index = 3,
                        .local_score = 7.0f,
                        .priority_score = 7.0f,
                        .impact = std::move(impact3)});

  const BatchSelector selector({});
  const BatchSelection selection = selector.select(candidates, 64, false);
  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{0, 2, 3}));
  EXPECT_EQ(selection.stats.deferred[static_cast<size_t>(
                BatchConflictReason::kForwardDependency)],
            1);
}

TEST(TestBatchSelector, BatchSelectorAlwaysRejectsHardConflicts)
{
  sta::Net* net = reinterpret_cast<sta::Net*>(0x401);
  MoveImpact impact;
  impact.complete = true;
  impact.write_nets = {net};
  std::vector<BatchCandidate> candidates{{.evaluation_index = 0,
                                          .local_score = 2.0f,
                                          .priority_score = 2.0f,
                                          .impact = impact},
                                         {.evaluation_index = 1,
                                          .local_score = 1.0f,
                                          .priority_score = 1.0f,
                                          .impact = impact}};

  BatchSelector selector({});
  const BatchSelection selection = selector.select(candidates, 64, false);

  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{0}));
  EXPECT_EQ(selection.stats
                .deferred[static_cast<size_t>(BatchConflictReason::kWriteNet)],
            1);
  const BatchSelection limited = selector.select(candidates, 2, false);
  EXPECT_EQ(limited.evaluation_indices, selection.evaluation_indices);
  EXPECT_EQ(limited.stats.deferred, selection.stats.deferred);
}

TEST(TestBatchSelector, BatchSelectorRejectsSameDriverInstance)
{
  sta::Instance* driver = reinterpret_cast<sta::Instance*>(0x451);
  sta::Net* net1 = reinterpret_cast<sta::Net*>(0x452);
  sta::Net* net2 = reinterpret_cast<sta::Net*>(0x453);
  MoveImpact impact1;
  impact1.complete = true;
  impact1.write_instances = {driver};
  impact1.write_nets = {net1};
  MoveImpact impact2;
  impact2.complete = true;
  impact2.write_instances = {driver};
  impact2.write_nets = {net2};
  std::vector<BatchCandidate> candidates{{.evaluation_index = 0,
                                          .local_score = 2.0f,
                                          .priority_score = 2.0f,
                                          .impact = impact1},
                                         {.evaluation_index = 1,
                                          .local_score = 1.0f,
                                          .priority_score = 1.0f,
                                          .impact = impact2}};

  BatchSelector selector({});
  const BatchSelection selection = selector.select(candidates, 64, false);

  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{0}));
  EXPECT_EQ(
      selection.stats
          .deferred[static_cast<size_t>(BatchConflictReason::kWriteInstance)],
      1);
}

TEST(TestBatchSelector, BatchSelectorSupportsBatchCap)
{
  MoveImpact impact;
  impact.complete = true;
  std::vector<BatchCandidate> candidates{{.evaluation_index = 0,
                                          .local_score = 3.0f,
                                          .priority_score = 3.0f,
                                          .impact = impact},
                                         {.evaluation_index = 1,
                                          .local_score = 2.0f,
                                          .priority_score = 2.0f,
                                          .impact = impact},
                                         {.evaluation_index = 2,
                                          .local_score = 1.0f,
                                          .priority_score = 1.0f,
                                          .impact = impact}};

  BatchSelector selector({});
  const BatchSelection selection = selector.select(candidates, 2, false);

  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{0, 1}));
  EXPECT_EQ(selection.stats.batch_limited, 1);
  EXPECT_EQ(selector.select(candidates, 1, false).evaluation_indices.size(),
            1u);
  EXPECT_EQ(selector.select(candidates, 1, false).stats.batch_limited, 2);
  EXPECT_EQ(selector.select(candidates, 3, false).evaluation_indices,
            (std::vector<size_t>{0, 1, 2}));
}

TEST(TestBatchSelector, BatchSelectorReservesSlotsForTheWorstPath)
{
  // Twenty candidates: the score order is the reverse of the criticality
  // order, so a cap of four never reaches the worst endpoints on its own.
  std::vector<BatchCandidate> candidates;
  candidates.reserve(20);
  for (size_t index = 0; index < 20; ++index) {
    MoveImpact impact;
    impact.complete = true;
    candidates.push_back({.evaluation_index = index,
                          .local_score = 100.0f - static_cast<float>(index),
                          .priority_score = 100.0f - static_cast<float>(index),
                          .endpoint_slack = -1e-9f * (1.0f + index),
                          .impact = std::move(impact)});
  }

  BatchSelector selector({.wns_reserve_pct = 25});

  // Off unless the policy asks for it.
  const BatchSelection plain = selector.select(candidates, 4, false);
  EXPECT_EQ(plain.evaluation_indices, (std::vector<size_t>{0, 1, 2, 3}));
  EXPECT_EQ(plain.stats.wns_reserved, 0);

  // One slot of four goes to the worst endpoint, and the batch still commits
  // in score order.
  const BatchSelection reserved = selector.select(candidates, 4, true);
  EXPECT_EQ(reserved.stats.wns_reserved, 1);
  EXPECT_EQ(reserved.evaluation_indices, (std::vector<size_t>{0, 1, 2, 19}));

  // A batch that holds every candidate has nothing to reserve against.
  const BatchSelection roomy = selector.select(candidates, 20, true);
  EXPECT_EQ(roomy.stats.wns_reserved, 0);
  EXPECT_EQ(roomy.evaluation_indices.size(), 20u);

  // Endpoints that already meet their constraint are never reserved.
  for (BatchCandidate& candidate : candidates) {
    candidate.endpoint_slack = 1e-12f;
  }

  EXPECT_EQ(selector.select(candidates, 4, true).stats.wns_reserved, 0);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyReservesInsideTheSetupMargin)
{
  // start() hands -setup_margin to the batch selector, so a stalled WNS
  // reserves a slot for an endpoint that meets timing by less than the
  // margin.
  ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig config;
  config.setup_slack_margin = 20e-12;
  SetupParallelEvalPolicy policy(resizer_, committer, setup_context, config);
  ASSERT_TRUE(policy.start());

  // Twenty candidates whose score order is the reverse of their criticality;
  // every endpoint meets timing, but by less than the margin.
  std::vector<BatchCandidate> candidates;
  candidates.reserve(20);
  for (size_t index = 0; index < 20; ++index) {
    MoveImpact impact;
    impact.complete = true;
    candidates.push_back(
        {.evaluation_index = index,
         .local_score = 100.0f - static_cast<float>(index),
         .priority_score = 100.0f - static_cast<float>(index),
         .endpoint_slack = 10e-12f - 0.1e-12f * static_cast<float>(index),
         .impact = std::move(impact)});
  }

  // selectBatch() turns the reserve on when WNS stalls: the worst endpoint
  // takes one of four slots.
  const BatchSelection selection
      = ResizerTestPeer::batchSelector(policy).select(candidates, 4, true);
  EXPECT_EQ(selection.stats.wns_reserved, 1);
  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{0, 1, 2, 19}));
}

TEST(TestBatchSelector, BatchSelectorStopsAtScoreTail)
{
  std::vector<BatchCandidate> candidates;
  candidates.reserve(20);
  for (size_t index = 0; index < 20; ++index) {
    MoveImpact impact;
    impact.complete = true;
    const float score = index < 16 ? 100.0f - static_cast<float>(index) : 10.0f;
    candidates.push_back({.evaluation_index = index,
                          .local_score = score,
                          .priority_score = score,
                          .impact = std::move(impact)});
  }

  BatchSelector selector({.score_floor_start = 16, .min_score_ratio = 0.25f});
  const BatchSelection selection = selector.select(candidates, 64, false);

  EXPECT_EQ(selection.evaluation_indices.size(), 16);
  EXPECT_EQ(selection.stats.score_limited, 4);
  for (size_t index = 8; index < candidates.size(); ++index) {
    candidates[index].local_score = 10.0f;
    candidates[index].priority_score = 10.0f;
  }

  const BatchSelection limited = selector.select(candidates, 12, false);
  EXPECT_EQ(limited.evaluation_indices.size(), 12u);
  EXPECT_EQ(limited.stats.score_limited, 0);
  EXPECT_EQ(limited.stats.batch_limited, 8);
  EXPECT_EQ(selector.select(candidates, 64, false).evaluation_indices.size(),
            16u);
}

TEST(TestBatchSelector, BatchSelectorRanksByPriorityAndKeepsLocalScoreTail)
{
  MoveImpact impact;
  impact.complete = true;
  const std::vector<BatchCandidate> candidates{{.evaluation_index = 0,
                                                .local_score = 100.0f,
                                                .priority_score = 100.0f,
                                                .impact = impact},
                                               {.evaluation_index = 1,
                                                .local_score = 90.0f,
                                                .priority_score = 200.0f,
                                                .impact = impact},
                                               {.evaluation_index = 2,
                                                .local_score = 10.0f,
                                                .priority_score = 190.0f,
                                                .impact = impact},
                                               {.evaluation_index = 3,
                                                .local_score = 80.0f,
                                                .priority_score = 180.0f,
                                                .impact = impact}};

  const BatchSelector selector(
      {.score_floor_start = 1, .min_score_ratio = 0.25f});
  const BatchSelection selection = selector.select(candidates, 4, false);

  EXPECT_EQ(selection.evaluation_indices, (std::vector<size_t>{1, 3, 0}));
  EXPECT_EQ(selection.stats.score_limited, 1);
}

TEST_F(TestResizerMt, SetupParallelEvalPolicyEvaluationEndpointLimit)
{
  EXPECT_EQ(SetupParallelEvalPolicy::evaluationEndpointLimit(64, 4, 256), 256u);
  EXPECT_EQ(SetupParallelEvalPolicy::evaluationEndpointLimit(64, 2, 256), 256u);
  EXPECT_EQ(SetupParallelEvalPolicy::evaluationEndpointLimit(8, 4, 0), 32u);
  EXPECT_EQ(SetupParallelEvalPolicy::evaluationEndpointLimit(512, 2, 256),
            1024u);
}

}  // namespace rsz
