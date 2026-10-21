// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Delay estimator: fanin side loads, sequential timing and the opposite
// transition.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, FaninSideLoadsChargeSiblingSlackLoss)
{
  // out2 is the only negative endpoint; load1/load2 are the fanin-net
  // siblings of the load0 target input.
  prepareUniqueViolatingEndpoint();
  const Target target = makeTarget("out0", "load0", "Z");

  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> plain
      = DelayEstimator::buildContext(resizer_, target, 1, &fail_reason);
  ASSERT_TRUE(plain.has_value()) << failReasonName(fail_reason);
  EXPECT_TRUE(plain->fanin_side_nets.empty());

  // Only load2/A sits on a violating worst path; load1/A is off-path.
  sta::Instance* load1 = db_network_->dbToSta(block_->findInst("load1"));
  sta::Instance* load2 = db_network_->dbToSta(block_->findInst("load2"));
  const sta::Pin* load1_pin = db_network_->findPin(load1, "A");
  const sta::Pin* load2_pin = db_network_->findPin(load2, "A");
  const std::unordered_set<const sta::Pin*> critical_pins{load2_pin};
  const FaninSideLoadOptions options{.critical_load_pins = &critical_pins,
                                     .off_path_weight = 0.25f};
  std::optional<EstimatorContext> context = DelayEstimator::buildContext(
      resizer_, target, 1, &fail_reason, false, false, nullptr, &options);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  ASSERT_EQ(context->target_stage_index, 1);

  // A buffer has one input: the selected-arc net with two sibling loads.
  ASSERT_EQ(context->fanin_side_nets.size(), 1u);
  const FaninSideNet& fanin_net = context->fanin_side_nets.front();
  EXPECT_TRUE(fanin_net.selected_arc);
  EXPECT_EQ(fanin_net.input_port->name(), std::string("A"));
  ASSERT_EQ(fanin_net.loads.size(), 2u);

  sta::Vertex* load1_input = sta_->graph()->pinLoadVertex(load1_pin);
  sta::Vertex* load2_input = sta_->graph()->pinLoadVertex(load2_pin);
  const float offpath_slack
      = sta::delayAsFloat(sta_->slack(load1_input, resizer_.maxAnalysisMode()));
  const float sibling_slack
      = sta::delayAsFloat(sta_->slack(load2_input, resizer_.maxAnalysisMode()));
  EXPECT_LT(sibling_slack, 0.0f);
  EXPECT_GT(offpath_slack, 0.0f);
  float critical_weight = -1.0f;
  float offpath_weight = -1.0f;
  for (const SideLoad& side : fanin_net.loads) {
    if (side.slack == sibling_slack) {
      critical_weight = side.weight;
    } else if (side.slack == offpath_slack) {
      offpath_weight = side.weight;
    }
  }

  EXPECT_FLOAT_EQ(critical_weight, 1.0f);
  EXPECT_FLOAT_EQ(offpath_weight, 0.25f);

  sta::LibertyCell* bigger = sta_->network()->findLibertyCell("BUF_X32");
  ASSERT_NE(bigger, nullptr);
  const DelayEstimate baseline = DelayEstimator::estimate(*plain, bigger);
  EXPECT_EQ(baseline.side_paths_evaluated, 0);
  EXPECT_FLOAT_EQ(baseline.side_loss, 0.0f);

  // The window gain is unchanged.  The side loss, reported apart, is the fanin
  // delay growth charged to the negative sibling only.
  const DelayEstimate charged = DelayEstimator::estimate(*context, bigger);
  EXPECT_EQ(charged.side_paths_evaluated, 2);
  EXPECT_FLOAT_EQ(charged.arrival_impr, baseline.arrival_impr);
  EXPECT_GT(charged.side_loss, 0.0f);
  EXPECT_EQ(charged.legal, baseline.legal);

  // The weight scales the charged loss linearly.
  context->fanin_side_loss_weight = 2.0f;
  EXPECT_FLOAT_EQ(DelayEstimator::estimate(*context, bigger).side_loss,
                  2.0f * charged.side_loss);

  // A sibling at the worst slack may lose at most the margin.  With no margin,
  // the candidate is rejected.
  context->side_slack_floor = sibling_slack;
  context->side_floor_margin = 0.0f;
  const DelayEstimate rejected = DelayEstimator::estimate(*context, bigger);
  EXPECT_FALSE(rejected.legal);
  EXPECT_EQ(rejected.reason, FailReason::kSideSlackFloorViolation);

  // A margin covering the whole drop keeps the candidate legal.
  context->side_floor_margin
      = charged.side_loss / std::max(charged.arrival_impr, 1e-15f) + 1.0f;
  EXPECT_EQ(DelayEstimator::estimate(*context, bigger).legal, baseline.legal);
  context->side_floor_margin = 1.0f;

  // A smaller replacement lowers the fanin load and charges nothing.
  sta::LibertyCell* smaller = sta_->network()->findLibertyCell("BUF_X4");
  ASSERT_NE(smaller, nullptr);
  const DelayEstimate relieved = DelayEstimator::estimate(*context, smaller);
  EXPECT_EQ(relieved.side_paths_evaluated, 0);
  EXPECT_FLOAT_EQ(relieved.side_loss, 0.0f);

  // Without a critical pin set every sibling keeps full weight.
  const FaninSideLoadOptions all_critical{};
  const std::optional<EstimatorContext> unweighted
      = DelayEstimator::buildContext(resizer_,
                                     target,
                                     1,
                                     &fail_reason,
                                     false,
                                     false,
                                     nullptr,
                                     &all_critical);
  ASSERT_TRUE(unweighted.has_value()) << failReasonName(fail_reason);
  ASSERT_EQ(unweighted->fanin_side_nets.size(), 1u);
  for (const SideLoad& side : unweighted->fanin_side_nets.front().loads) {
    EXPECT_FLOAT_EQ(side.weight, 1.0f);
  }
}

TEST_F(TestResizerMt, FaninSideLoadsCoverEveryTargetInput)
{
  // path_target is a NAND2: A1 is the selected arc (path_pre1 net), A2 comes
  // from path_side_buf whose only load is the target itself.
  resizer_.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");
  FailReason fail_reason = FailReason::kNone;
  const FaninSideLoadOptions options{};
  const std::optional<EstimatorContext> context = DelayEstimator::buildContext(
      resizer_, target, 1, &fail_reason, false, false, nullptr, &options);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  ASSERT_EQ(context->target_stage_index, 1);

  // The selected-arc net has no sibling, so only the A2 net is recorded.
  ASSERT_EQ(context->fanin_side_nets.size(), 1u);
  const FaninSideNet& side_net = context->fanin_side_nets.front();
  EXPECT_FALSE(side_net.selected_arc);
  EXPECT_EQ(side_net.input_port->name(), std::string("A2"));
  EXPECT_GT(side_net.delay_per_cap, 0.0f);
  ASSERT_EQ(side_net.loads.size(), 1u);

  // A larger NAND2 raises A2 capacitance, so the off-arc fanin driver slows:
  // with the floor at the side load's slack, the candidate is rejected.
  sta::LibertyCell* bigger = sta_->network()->findLibertyCell("NAND2_X4");
  ASSERT_NE(bigger, nullptr);
  EstimatorContext floored = *context;
  floored.side_slack_floor = side_net.loads.front().slack;
  floored.side_floor_margin = 0.0f;
  const DelayEstimate charged = DelayEstimator::estimate(floored, bigger);
  EXPECT_EQ(charged.side_paths_evaluated, 1);
  EXPECT_FALSE(charged.legal);
  EXPECT_EQ(charged.reason, FailReason::kSideSlackFloorViolation);
}

TEST_F(TestResizerMt, SequentialTimingOffPreservesCombinationalWindow)
{
  resizer_.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");
  const sta::PathExpanded expanded(target.endpoint_path, sta_.get());
  for (const int levels : {0, 1, 2}) {
    SCOPED_TRACE(levels);
    const std::optional<EstimatorContext> baseline
        = DelayEstimator::buildContext(resizer_, target, levels);
    const std::optional<EstimatorContext> disabled
        = DelayEstimator::buildContext(
            resizer_, target, levels, nullptr, false, false);
    const std::optional<EstimatorContext> enabled
        = DelayEstimator::buildContext(
            resizer_, target, levels, nullptr, false, true);
    const std::optional<EstimatorContext> lower_default
        = DelayEstimator::buildContext(resizer_,
                                       target.inst(resizer_),
                                       target.driver_pin,
                                       expanded,
                                       target.path_index,
                                       target.scene,
                                       target.minMax(resizer_),
                                       levels);
    const std::optional<EstimatorContext> lower_disabled
        = DelayEstimator::buildContext(resizer_,
                                       target.inst(resizer_),
                                       target.driver_pin,
                                       expanded,
                                       target.path_index,
                                       target.scene,
                                       target.minMax(resizer_),
                                       levels,
                                       nullptr,
                                       false,
                                       false);

    ASSERT_TRUE(baseline.has_value());
    ASSERT_TRUE(disabled.has_value());
    ASSERT_TRUE(enabled.has_value());
    ASSERT_TRUE(lower_default.has_value());
    ASSERT_TRUE(lower_disabled.has_value());
    expectSameDelayContext(*baseline, *disabled);
    expectSameDelayContext(*baseline, *enabled);
    expectSameDelayContext(*baseline, *lower_default);
    expectSameDelayContext(*baseline, *lower_disabled);
    sta::LibertyCell* candidate = db_network_->findLibertyCell("NAND2_X1_L");
    const DelayEstimate before = DelayEstimator::estimate(*baseline, candidate);
    const DelayEstimate after = DelayEstimator::estimate(*disabled, candidate);
    EXPECT_EQ(before.legal, after.legal);
    EXPECT_EQ(before.reason, after.reason);
    EXPECT_FLOAT_EQ(before.candidate_delay, after.candidate_delay);
    EXPECT_FLOAT_EQ(before.arrival_impr, after.arrival_impr);
  }
}

TEST_F(TestResizerMtSequential, LatchWindowIncludesPhysicalDataFanin)
{
  prepareSequential("DLH_X1", "G");
  ASSERT_FALSE(HasFatalFailure());
  sta::Vertex* endpoint = findEndpointVertex("out0");
  ASSERT_NE(endpoint, nullptr);
  sta::Pin* data_driver = db_network_->findPin("target/Z");
  sta::Pin* clock_driver = db_network_->findPin("dq_clock_driver/Z");
  odb::dbInst* latch = block_->findInst("dq_seq");
  ASSERT_EQ(latch->findITerm("D")->getNet(),
            block_->findInst("target")->findITerm("Z")->getNet());
  ASSERT_NE(latch->findITerm("D")->getNet(), latch->findITerm("G")->getNet());
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DLH_X2");
  ASSERT_NE(candidate, nullptr);

  // The worst slack paths read required times.
  sta_->findRequireds();
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    SCOPED_TRACE(rf->index());
    const sta::Path* path
        = sta_->vertexWorstSlackPath(endpoint, rf, sta::MinMax::max());
    ASSERT_NE(path, nullptr);
    const sta::PathExpanded expanded(path, sta_.get());
    const sta::Path* data_path = latchDataPath(expanded);
    ASSERT_NE(data_path, nullptr);
    ASSERT_NE(data_path->prevPath(), nullptr);
    ASSERT_EQ(data_path->prevPath()->pin(sta_.get()), data_driver);
    const Target target = makePathDriverTarget(path,
                                               expanded,
                                               expanded.startIndex(),
                                               path->slack(sta_.get()),
                                               resizer_);
    ASSERT_EQ(target.driver_pin, db_network_->findPin("dq_seq/Q"));

    for (const int levels : {0, 1, 2}) {
      SCOPED_TRACE(levels);
      const std::optional<EstimatorContext> baseline
          = DelayEstimator::buildContext(resizer_, target, levels);
      const std::optional<EstimatorContext> disabled
          = DelayEstimator::buildContext(
              resizer_, target, levels, nullptr, false, false);

      FailReason reason = FailReason::kNone;
      const std::optional<EstimatorContext> enabled
          = DelayEstimator::buildContext(
              resizer_, target, levels, &reason, false, true);
      const std::optional<EstimatorContext> lower_enabled
          = DelayEstimator::buildContext(resizer_,
                                         target.inst(resizer_),
                                         target.driver_pin,
                                         expanded,
                                         target.path_index,
                                         target.scene,
                                         sta::MinMax::max(),
                                         levels,
                                         nullptr,
                                         false,
                                         true);

      ASSERT_TRUE(baseline.has_value());
      ASSERT_TRUE(disabled.has_value());
      ASSERT_TRUE(enabled.has_value()) << failReasonName(reason);
      ASSERT_TRUE(lower_enabled.has_value());
      expectSameDelayContext(*baseline, *disabled);
      expectSameDelayContext(*enabled, *lower_enabled);
      EXPECT_EQ(disabled->target_stage_index, 0);
      ASSERT_EQ(enabled->target_stage_index, 1);
      ASSERT_EQ(enabled->path_stages.size(), disabled->path_stages.size() + 1);
      const DelayStageState& fanin = enabled->path_stages.front();
      const DelayStageState& sequential = enabled->target();
      EXPECT_EQ(fanin.driver_pin, data_driver);
      EXPECT_EQ(sequential.arc.inputPort()->name(), "D");
      EXPECT_EQ(fanin.arc.outputRiseFall(), sequential.arc.inputRiseFall());
      EXPECT_EQ(sequential.arc.outputRiseFall(), rf);
      int data_driver_count = 0;
      float total_delay = 0.0f;
      for (const DelayStageState& stage : enabled->path_stages) {
        data_driver_count += stage.driver_pin == data_driver;
        EXPECT_NE(stage.driver_pin, clock_driver);
        EXPECT_EQ(stage.arc.scene, target.scene);
        EXPECT_EQ(stage.arc.min_max, sta::MinMax::max());
        total_delay += stage.current_delay;
      }

      EXPECT_EQ(data_driver_count, 1);
      EXPECT_FLOAT_EQ(enabled->current_total_delay, total_delay);

      const float input_cap_delta
          = candidate->sceneCell(target.scene, sta::MinMax::max())
                ->findLibertyPort("D")
                ->capacitance()
            - sequential.arc.inputPort()->capacitance();
      ASSERT_GT(input_cap_delta, 0.0f);
      std::vector<StageEvaluation> trace;
      const DelayEstimate charged
          = DelayEstimator::estimate(*enabled, candidate, &trace);
      const DelayEstimate uncharged
          = DelayEstimator::estimate(*disabled, candidate);

      ASSERT_EQ(trace.size(), enabled->path_stages.size());
      EXPECT_FLOAT_EQ(trace.front().load_cap, fanin.load_cap + input_cap_delta);
      EXPECT_GT(trace.front().stage_delay, fanin.current_delay);
      EXPECT_LT(charged.arrival_impr, uncharged.arrival_impr);
    }
  }
}

TEST_F(TestResizerMtSequential, LatchContextBuildAndEstimatePreserveLiveTiming)
{
  prepareSequential("DLH_X1", "G");
  ASSERT_FALSE(HasFatalFailure());
  const Target target = makeTarget("out0", "dq_seq", "Q");
  odb::dbInst* latch = block_->findInst("dq_seq");
  odb::dbMaster* master = latch->getMaster();
  odb::dbNet* data_net = latch->findITerm("D")->getNet();
  odb::dbNet* output_net = latch->findITerm("Q")->getNet();
  const sta::Slack slack = target.endpoint_path->slack(sta_.get());
  const sta::Arrival arrival = target.endpoint_path->arrival();
  const float load_cap = sta_->graphDelayCalc()->loadCap(
      target.driver_pin, target.scene, sta::MinMax::max());
  const std::optional<EstimatorContext> context
      = DelayEstimator::buildContext(resizer_, target, 1, nullptr, false, true);
  ASSERT_TRUE(context.has_value());
  const EstimatorContext captured = *context;
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DLH_X2");
  const DelayEstimate first = DelayEstimator::estimate(*context, candidate);
  for (int repeat = 0; repeat < 3; ++repeat) {
    const DelayEstimate repeated
        = DelayEstimator::estimate(*context, candidate);
    EXPECT_EQ(repeated.legal, first.legal);
    EXPECT_FLOAT_EQ(repeated.arrival_impr, first.arrival_impr);
  }

  expectSameDelayContext(*context, captured);
  EXPECT_EQ(latch->getMaster(), master);
  EXPECT_EQ(latch->findITerm("D")->getNet(), data_net);
  EXPECT_EQ(latch->findITerm("Q")->getNet(), output_net);
  EXPECT_FLOAT_EQ(target.endpoint_path->slack(sta_.get()), slack);
  EXPECT_FLOAT_EQ(target.endpoint_path->arrival(), arrival);
  EXPECT_FLOAT_EQ(sta_->graphDelayCalc()->loadCap(
                      target.driver_pin, target.scene, sta::MinMax::max()),
                  load_cap);

  EXPECT_NO_THROW(sta_->updateTiming(true));
  const Target refreshed = makeTarget("out0", "dq_seq", "Q");
  EXPECT_NEAR(refreshed.endpoint_path->slack(sta_.get()), slack, 1e-15);
  EXPECT_NEAR(refreshed.endpoint_path->arrival(), arrival, 1e-15);
  EXPECT_FLOAT_EQ(DelayEstimator::estimate(*context, candidate).arrival_impr,
                  first.arrival_impr);
}

TEST_F(TestResizerMtSequential, FlipFlopSetupPenaltyUsesCapturedDataMargin)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X1");
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DFF_X2");
  ASSERT_NE(candidate, nullptr);
  ASSERT_LT(candidate->findLibertyPort("D")->capacitance(),
            current->findLibertyPort("D")->capacitance());

  odb::dbInst* flip_flop = block_->findInst("dq_seq");
  odb::dbMaster* original = flip_flop->getMaster();
  odb::dbNet* data_net = flip_flop->findITerm("D")->getNet();
  odb::dbNet* clock_net = flip_flop->findITerm("CK")->getNet();
  sta::Vertex* data_vertex
      = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));

  setDataSlack(5e-12, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  Target target = makeTarget("out0", "dq_seq", "Q");
  const float setup_before = sta_->slack(data_vertex, sta::MinMax::max());
  const float hold_before = sta_->slack(data_vertex, sta::MinMax::min());
  const float data_arrival = sta_->arrival(db_network_->findPin("dq_seq/D"),
                                           sta::RiseFallBoth::riseFall(),
                                           sta::MinMax::max());

  const std::shared_ptr<const SequentialTimingEstimator> near
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(near, nullptr);
  const SequentialTimingEstimator::Result identity = near->estimate(current);
  ASSERT_TRUE(identity.legal) << identity.reason;
  EXPECT_FLOAT_EQ(identity.weighted_side_slack_loss, 0.0f);
  EXPECT_FLOAT_EQ(identity.max_side_slack_loss, 0.0f);
  const SequentialTimingEstimator::Result risk = near->estimate(candidate);
  ASSERT_TRUE(risk.legal) << risk.reason;
  EXPECT_GT(risk.weighted_side_slack_loss, 0.0f);
  EXPECT_GT(risk.max_side_slack_loss, 0.0f);
  EXPECT_LT(risk.weighted_side_slack_loss, risk.max_side_slack_loss);
  EXPECT_EQ(flip_flop->getMaster(), original);
  EXPECT_EQ(flip_flop->findITerm("D")->getNet(), data_net);
  EXPECT_EQ(flip_flop->findITerm("CK")->getNet(), clock_net);
  EXPECT_FLOAT_EQ(sta_->slack(data_vertex, sta::MinMax::max()), setup_before);
  EXPECT_FLOAT_EQ(sta_->slack(data_vertex, sta::MinMax::min()), hold_before);
  EXPECT_FLOAT_EQ(sta_->arrival(db_network_->findPin("dq_seq/D"),
                                sta::RiseFallBoth::riseFall(),
                                sta::MinMax::max()),
                  data_arrival);

  setDataSlack(100e-12, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> ample
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(ample, nullptr);
  const SequentialTimingEstimator::Result safe = ample->estimate(candidate);
  ASSERT_TRUE(safe.legal) << safe.reason;
  EXPECT_FLOAT_EQ(safe.weighted_side_slack_loss, 0.0f);
  EXPECT_FLOAT_EQ(safe.max_side_slack_loss, risk.max_side_slack_loss);
  const SequentialTimingEstimator::Result cached = near->estimate(candidate);
  EXPECT_EQ(cached.legal, risk.legal);
  EXPECT_FLOAT_EQ(cached.weighted_side_slack_loss,
                  risk.weighted_side_slack_loss);
  EXPECT_FLOAT_EQ(cached.max_side_slack_loss, risk.max_side_slack_loss);
  EXPECT_EQ(cached.reason, risk.reason);
  std::vector<SequentialTimingEstimator::Result> parallel_results(4);
  std::vector<std::thread> workers;
  for (size_t index = 0; index < parallel_results.size(); ++index) {
    workers.emplace_back(
        [&, index] { parallel_results[index] = near->estimate(candidate); });
  }

  for (std::thread& worker : workers) {
    worker.join();
  }

  for (const SequentialTimingEstimator::Result& result : parallel_results) {
    EXPECT_EQ(result.legal, risk.legal);
    EXPECT_FLOAT_EQ(result.weighted_side_slack_loss,
                    risk.weighted_side_slack_loss);
    EXPECT_FLOAT_EQ(result.max_side_slack_loss, risk.max_side_slack_loss);
    EXPECT_EQ(result.reason, risk.reason);
  }

  setDataSlack(0.0f, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> critical
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(critical, nullptr);
  const SequentialTimingEstimator::Result violation
      = critical->estimate(candidate);
  EXPECT_FALSE(violation.legal);
  EXPECT_EQ(violation.reason, "sequential_setup_violation");
  EXPECT_GT(violation.weighted_side_slack_loss, 0.0f);
  EXPECT_FLOAT_EQ(violation.weighted_side_slack_loss,
                  violation.max_side_slack_loss);

  ASSERT_TRUE(resizer_.replaceCell(target.inst(resizer_), candidate, false));
  resizer_.updateParasiticsAndTiming();
  data_vertex = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));
  EXPECT_LT(sta_->slack(data_vertex, sta::MinMax::max()), 0.0f);
  EXPECT_FLOAT_EQ(near->estimate(candidate).weighted_side_slack_loss,
                  risk.weighted_side_slack_loss);
}

TEST_F(TestResizerMtSequential, FlipFlopHoldCheckLossHasCorrectSign)
{
  prepareFlipFlop("DFF_X2");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X2");
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DFF_X1");
  ASSERT_NE(candidate, nullptr);
  ASSERT_GT(candidate->findLibertyPort("D")->capacitance(),
            current->findLibertyPort("D")->capacitance());

  setDataSlack(0.0f, sta::MinMax::min());
  ASSERT_FALSE(HasFatalFailure());
  const Target target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> estimator
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(estimator, nullptr);
  const SequentialTimingEstimator::Result identity
      = estimator->estimate(current);
  ASSERT_TRUE(identity.legal) << identity.reason;
  EXPECT_FLOAT_EQ(identity.weighted_side_slack_loss, 0.0f);
  const SequentialTimingEstimator::Result result
      = estimator->estimate(candidate);
  EXPECT_FALSE(result.legal);
  EXPECT_EQ(result.reason, "sequential_hold_violation");
  EXPECT_GT(result.weighted_side_slack_loss, 0.0f);
  EXPECT_LE(result.weighted_side_slack_loss, result.max_side_slack_loss);

  ASSERT_TRUE(resizer_.replaceCell(target.inst(resizer_), candidate, false));
  resizer_.updateParasiticsAndTiming();
  sta::Vertex* data_vertex
      = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));
  EXPECT_LT(sta_->slack(data_vertex, sta::MinMax::min()), 0.0f);
  EXPECT_GT(sta_->slack(data_vertex, sta::MinMax::max()), 0.0f);
}

TEST_F(TestResizerMtSequential, FlipFlopEstimatorExcludesLatchAndCombinational)
{
  prepareSequential("DLH_X1", "G");
  ASSERT_FALSE(HasFatalFailure());
  const Target latch = makeTarget("out0", "dq_seq", "Q");
  const Target combinational = makeTarget("path_out", "path_target", "ZN");
  EXPECT_EQ(SequentialTimingEstimator::prepare(resizer_, latch), nullptr);
  EXPECT_EQ(SequentialTimingEstimator::prepare(resizer_, combinational),
            nullptr);
}

TEST_F(TestResizerMtSequential, FlipFlopPreparationCountsCandidateCells)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  ProfileTestDirectory directory;
  {
    RepairSetupProfiler profiler(resizer_,
                                 RepairSetupProfiler::Mode::kTiming,
                                 directory.root().string());
    profiler.beginPhase("PARALLEL_EVAL", 1);
    const Target target = makeTarget("out0", "dq_seq", "Q");
    ASSERT_NE(SequentialTimingEstimator::prepare(resizer_, target), nullptr);
    profiler.complete(true);
  }

  const std::string stats
      = readProfileFile(directory.root() / "run_1/stats.csv");
  EXPECT_NE(stats.find("\"sequential_candidate_cells_prepared\""),
            std::string::npos);
  EXPECT_EQ(stats.find("\"sequential_cells_snapshotted\""), std::string::npos);
}

TEST_F(TestResizerMtSequential, FlipFlopPropagatedClockUsesCheckSlewPoint)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  sta_->setAnalysisType(sta::AnalysisType::single, sta_->cmdMode()->sdc());
  sta_->updateTiming(true);
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X1");
  Target target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> ideal
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(ideal, nullptr);
  ASSERT_TRUE(ideal->estimate(current).legal);

  sta_->setPropagatedClock(sta_->cmdMode()->sdc()->findClock("clk"),
                           sta_->cmdMode());

  sta_->updateTiming(true);
  target = makeTarget("out0", "dq_seq", "Q");
  ASSERT_NE(target.scene->checkClkSlewMinMax(sta::MinMax::max()),
            target.minMax(resizer_));

  sta::Vertex* clock_vertex
      = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/CK"));
  ASSERT_GT(
      sta_->graph()->slew(clock_vertex,
                          sta::RiseFall::rise(),
                          target.scene->checkClkSlewIndex(sta::MinMax::max())),
      0.0f);
  const std::shared_ptr<const SequentialTimingEstimator> propagated
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(propagated, nullptr);
  const SequentialTimingEstimator::Result identity
      = propagated->estimate(current);
  EXPECT_TRUE(identity.legal) << identity.reason;
  EXPECT_FLOAT_EQ(identity.weighted_side_slack_loss, 0.0f);
  EXPECT_FLOAT_EQ(identity.max_side_slack_loss, 0.0f);
}

TEST_F(TestResizerMtSequential, FlipFlopCandidateUsesActiveScaledCheckModel)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X1");
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DFF_X1_L");
  sta::LibertyCell* scaled = db_network_->findLibertyCell("DFF_X2_L");
  copyFlipFlopChecks(current, candidate);
  copyFlipFlopChecks(current, scaled, 4e-12f);
  ASSERT_FALSE(HasFatalFailure());
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    for (const sta::MinMax* min_max : sta::MinMax::range()) {
      candidate->findLibertyPort("D")->setCapacitance(
          rf, min_max, current->findLibertyPort("D")->capacitance(rf, min_max));
    }
  }

  setDataSlack(2e-12f, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  sta::OperatingConditions* op_cond
      = current->libertyLibrary()->defaultOperatingConditions();
  ASSERT_NE(op_cond, nullptr);
  sta_->setOperatingConditions(
      op_cond, sta::MinMaxAll::all(), sta_->cmdMode()->sdc());

  sta_->updateTiming(true);
  const Target target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> nominal
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(nominal, nullptr);
  ASSERT_TRUE(nominal->estimate(candidate).legal);
  EXPECT_FLOAT_EQ(nominal->estimate(candidate).weighted_side_slack_loss, 0.0f);
  int scaled_checks = 0;
  for (sta::TimingArcSet* set : candidate->timingArcSets()) {
    if (set->role() != sta::TimingRole::setup()) {
      continue;
    }

    for (sta::TimingArc* arc : set->arcs()) {
      for (sta::TimingArcSet* scaled_set : scaled->timingArcSets()) {
        const sta::TimingArc* scaled_arc
            = findMatchingTimingArc(arc, scaled_set);
        if (scaled_arc != nullptr) {
          scaled_arc->model()->setIsScaled(true);
          arc->addScaledModel(op_cond, scaled_arc->model());
          ASSERT_NE(arc->checkModel(target.scene, sta::MinMax::max()),
                    arc->model());
          ++scaled_checks;
        }
      }
    }
  }

  ASSERT_EQ(scaled_checks, 2);
  const std::shared_ptr<const SequentialTimingEstimator> estimator
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(estimator, nullptr);
  const SequentialTimingEstimator::Result result
      = estimator->estimate(candidate);
  EXPECT_FALSE(result.legal);
  EXPECT_EQ(result.reason, "sequential_setup_violation");
  EXPECT_NEAR(result.weighted_side_slack_loss, 4e-12, 1e-15);
  EXPECT_TRUE(nominal->estimate(candidate).legal);
  ASSERT_TRUE(resizer_.replaceCell(target.inst(resizer_), candidate, false));
  resizer_.updateParasiticsAndTiming();
  sta::Vertex* data_vertex
      = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));
  EXPECT_NEAR(sta_->slack(data_vertex, sta::MinMax::max()), -2e-12, 1e-15);
}

TEST_F(TestResizerMtSequential,
       FlipFlopCandidateUsesActiveScaledDataCapacitance)
{
  prepareFlipFlop("DFF_X1");
  ASSERT_FALSE(HasFatalFailure());
  sta::LibertyCell* current = db_network_->findLibertyCell("DFF_X1");
  sta::LibertyCell* candidate = db_network_->findLibertyCell("DFF_X1_L");
  sta::LibertyCell* scaled = db_network_->findLibertyCell("DFF_X2_L");
  copyFlipFlopChecks(current, candidate);
  ASSERT_FALSE(HasFatalFailure());
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    for (const sta::MinMax* min_max : sta::MinMax::range()) {
      const float cap = current->findLibertyPort("D")->capacitance(rf, min_max);
      candidate->findLibertyPort("D")->setCapacitance(rf, min_max, cap);
      scaled->findLibertyPort("D")->setCapacitance(rf, min_max, cap * 1.05f);
    }
  }

  sta::OperatingConditions* op_cond
      = current->libertyLibrary()->defaultOperatingConditions();
  ASSERT_NE(op_cond, nullptr);
  sta_->setOperatingConditions(
      op_cond, sta::MinMaxAll::all(), sta_->cmdMode()->sdc());

  setDataSlack(5e-12f, sta::MinMax::max());
  ASSERT_FALSE(HasFatalFailure());
  const Target target = makeTarget("out0", "dq_seq", "Q");
  const std::shared_ptr<const SequentialTimingEstimator> nominal
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(nominal, nullptr);
  ASSERT_TRUE(nominal->estimate(candidate).legal);
  EXPECT_FLOAT_EQ(nominal->estimate(candidate).weighted_side_slack_loss, 0.0f);
  candidate->findLibertyPort("D")->addScaledPort(op_cond,
                                                 scaled->findLibertyPort("D"));

  const std::shared_ptr<const SequentialTimingEstimator> estimator
      = SequentialTimingEstimator::prepare(resizer_, target);
  ASSERT_NE(estimator, nullptr);
  const SequentialTimingEstimator::Result result
      = estimator->estimate(candidate);
  EXPECT_TRUE(result.legal) << result.reason;
  EXPECT_GT(result.weighted_side_slack_loss, 0.0f);
  EXPECT_GT(result.max_side_slack_loss, result.weighted_side_slack_loss);
  EXPECT_FLOAT_EQ(nominal->estimate(candidate).weighted_side_slack_loss, 0.0f);
  ASSERT_TRUE(resizer_.replaceCell(target.inst(resizer_), candidate, false));
  resizer_.updateParasiticsAndTiming();
  const sta::Pin* data_pin = db_network_->findPin("dq_seq/D");
  EXPECT_FLOAT_EQ(
      sta_->cmdMode()->sdc()->pinCapacitance(
          data_pin, sta::RiseFall::rise(), target.scene, sta::MinMax::max()),
      scaled->findLibertyPort("D")->capacitance(sta::RiseFall::rise(),
                                                sta::MinMax::max()));

  // Scaled-only fixture edits bypass STA's nominal-cap replacement test.
  ep_.estimateWireParasitics();
  sta_->delaysInvalid();
  sta_->findRequireds();
  sta::Vertex* data_vertex
      = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));
  EXPECT_LT(sta_->slack(data_vertex, sta::MinMax::max()), 5e-12f);
  EXPECT_GT(sta_->slack(data_vertex, sta::MinMax::max()), 0.0f);
}

TEST(TestDelayEstimatorRules, OppositeRfSideLossChargesNegativeRegionOnly)
{
  // Violating opposite transition: every picosecond of growth is lost.
  EXPECT_FLOAT_EQ(oppositeRfSideLoss(-10e-12f, 3e-12f), 3e-12f);

  // Positive slack absorbs part of the growth.
  EXPECT_FLOAT_EQ(oppositeRfSideLoss(2e-12f, 3e-12f), 1e-12f);

  // Enough slack, a faster opposite transition, or no timing: no loss.
  EXPECT_FLOAT_EQ(oppositeRfSideLoss(5e-12f, 3e-12f), 0.0f);
  EXPECT_FLOAT_EQ(oppositeRfSideLoss(-10e-12f, -1e-12f), 0.0f);
  EXPECT_FLOAT_EQ(
      oppositeRfSideLoss(std::numeric_limits<float>::infinity(), 3e-12f), 0.0f);
}

}  // namespace rsz
