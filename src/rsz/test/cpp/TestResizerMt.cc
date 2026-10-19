// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Estimator basics, cell-swap candidates, thread pool and STA warm-up
// concurrency.

#include "TestResizerMtFixture.hh"

namespace rsz {

static void warmupAllSwappableCells(Resizer& resizer, sta::Network* network)
{
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      const sta::LibertyCellSeq swappable_cells
          = resizer.getSwappableCells(cell);
      for (sta::LibertyCell* swappable_cell : swappable_cells) {
        static_cast<void>(resizer.getSwappableCells(swappable_cell));
      }
    }
  }
}

static std::vector<sta::LibertyCell*> collectLibertyCells(sta::Network* network)
{
  std::vector<sta::LibertyCell*> cells;
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      cells.push_back(cell_iter.next());
    }
  }
  return cells;
}

static std::vector<sta::Pin*> collectDriverPins(sta::Network* network)
{
  std::vector<sta::Pin*> pins;
  sta::LeafInstanceIterator* inst_iter = network->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network->pinIterator(inst));
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      if (network->isDriver(pin)) {
        pins.push_back(pin);
      }
    }
  }
  delete inst_iter;
  return pins;
}

static std::string swappableCellsSignature(Resizer& resizer,
                                           sta::LibertyCell* cell)
{
  const sta::LibertyCellSeq swappable_cells = resizer.getSwappableCells(cell);
  std::vector<std::string> names;
  names.reserve(swappable_cells.size());
  for (sta::LibertyCell* swappable_cell : swappable_cells) {
    names.push_back(swappable_cell->name());
  }
  std::ranges::sort(names);

  std::ostringstream oss;
  oss << cell->name() << ':' << names.size();
  for (const std::string& name : names) {
    oss << ':' << name;
  }
  return oss.str();
}

static std::string capacitanceSignature(sta::dbSta* sta, const sta::Pin* pin)
{
  float cap = 0.0f;
  float max_cap = 0.0f;
  float cap_slack = 0.0f;
  const sta::RiseFall* tr = nullptr;
  const sta::Scene* corner = nullptr;
  sta->checkCapacitance(pin,
                        sta->scenes(),
                        sta::MinMax::max(),
                        cap,
                        max_cap,
                        cap_slack,
                        tr,
                        corner);

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(12);
  oss << sta->network()->pathName(pin) << ':' << cap << ':' << max_cap << ':'
      << cap_slack << ':' << (tr != nullptr ? tr->name() : "null") << ':'
      << (corner != nullptr ? "corner" : "null");
  return oss.str();
}

TEST_F(TestResizerMt, BuildEstimatorContextAndEstimateLvtCandidate)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();

  const Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, 0, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }

  const EstimatorContext& estimator_context = context.value();
  const DelayStageState& target_stage = estimator_context.target();
  ASSERT_NE(target_stage.arc.inputPort(), nullptr);
  ASSERT_NE(target_stage.arc.outputPort(), nullptr);
  EXPECT_EQ(target_stage.arc.inputPort()->name(), "A");
  EXPECT_EQ(target_stage.arc.outputPort()->name(), "Z");
  ASSERT_EQ(estimator_context.path_stages.size(), 1);
  EXPECT_EQ(estimator_context.target_stage_index, 0);
  EXPECT_EQ(estimator_context.delay_estimation_levels, 0);
  EXPECT_GT(target_stage.load_cap, 0.0f);
  EXPECT_GT(target_stage.current_delay, 0.0f);

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("BUF_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(estimator_context, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_GT(estimate.arrival_impr, 0.0f);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, BuildEstimatorContextWithOneLevelPathWindow)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");

  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> context
      = DelayEstimator::buildContext(resizer, target, 1, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }

  const EstimatorContext& estimator_context = context.value();
  EXPECT_EQ(estimator_context.delay_estimation_levels, 1);
  ASSERT_EQ(estimator_context.path_stages.size(), 3);
  ASSERT_EQ(estimator_context.target_stage_index, 1);
  EXPECT_EQ(estimator_context.path_stages[0].arc.outputPort()->name(), "Z");
  EXPECT_EQ(estimator_context.path_stages[1].arc.outputPort()->name(), "ZN");
  EXPECT_EQ(estimator_context.path_stages[2].arc.outputPort()->name(), "Z");
  for (const DelayStageState& stage : estimator_context.path_stages) {
    EXPECT_GT(stage.current_delay, 0.0f);
    EXPECT_GT(stage.current_slew, 0.0f);
  }

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(estimator_context, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, SlewBiasContextBuildLeavesTimingUpdateUsable)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");

  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> context
      = DelayEstimator::buildContext(resizer, target, 1, &fail_reason, true);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);

  // STA slew-bias sampling mutates and restores a live reduced Pi model during
  // prepare; a fresh STA update after context construction must remain valid.
  EXPECT_NO_THROW(sta_->updateTiming(true));

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(*context, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, SlewBiasCacheReusesSamplingUntilClear)
{
  resizer_.runRepairSetupPreamble();
  sta::Scene* scene = sta_->cmdScene();
  for (const char* name : {"path_pre1/Z", "path_side_buf/Z"}) {
    const sta::Pin* driver = sta_->network()->findPin(name);
    ASSERT_NE(driver, nullptr);
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      float pin_cap = 0.0f;
      float sdc_wire_cap = 0.0f;
      float fanout = 0.0f;
      bool has_net_load = false;
      sta_->graphDelayCalc()->netCaps(driver,
                                      rf,
                                      scene,
                                      sta::MinMax::max(),
                                      pin_cap,
                                      sdc_wire_cap,
                                      fanout,
                                      has_net_load);
      scene->parasitics(sta::MinMax::max())
          ->makePiElmore(driver, rf, sta::MinMax::max(), 1e-15f, 1.0f, pin_cap);
    }
  }
  sta_->updateTiming(true);
  const Target target = makeTarget("path_out", "path_target", "ZN");
  const std::optional<EstimatorContext> context
      = DelayEstimator::buildContext(resizer_, target, 1);
  ASSERT_TRUE(context.has_value());
  ASSERT_GT(context->target_stage_index, 0);

  DelayEstimatorCache cache;
  const float max_load_delta
      = cache.maxEquivalentInputCapIncrease(resizer_, context->target());
  ASSERT_GT(max_load_delta, 0.0f);
  const DelayStageState& fanin_stage
      = context->path_stages[context->target_stage_index - 1];

  ProfileTestDirectory directory;
  {
    RepairSetupProfiler profiler(resizer_,
                                 RepairSetupProfiler::Mode::kTiming,
                                 directory.root().string());
    profiler.beginPhase("PARALLEL_EVAL", 1);
    const ProfileIteration iteration("test_pass");
    const SlewBiasModel first
        = cache.getOrSampleSlewBias(resizer_, fanin_stage, max_load_delta);
    const SlewBiasModel cached
        = cache.getOrSampleSlewBias(resizer_, fanin_stage, max_load_delta);
    EXPECT_EQ(cached.valid, first.valid);
    cache.clear();
    const SlewBiasModel after_clear
        = cache.getOrSampleSlewBias(resizer_, fanin_stage, max_load_delta);
    EXPECT_EQ(after_clear.valid, first.valid);
    profiler.complete(true);
  }

  const std::string stats
      = readProfileFile(directory.root() / "run_1/stats.csv");
  EXPECT_NE(stats.find(",\"count\",\"slew_bias_model_hits\",\"\",1,1,1\n"),
            std::string::npos)
      << stats;
  EXPECT_NE(stats.find(",\"count\",\"slew_bias_model_misses\",\"\",2,2,2\n"),
            std::string::npos)
      << stats;
}

TEST_F(TestResizerMt, RejectCandidateWithMismatchedOutputPort)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, 0, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }

  const EstimatorContext& estimator_context = context.value();

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(estimator_context, candidate_cell);
  EXPECT_FALSE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kMissingCandidatePort);
}

TEST_F(TestResizerMt, BuildEstimatorContextCapturesOutputSlewMergeArcs)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, 0, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }

  const EstimatorContext& estimator_context = context.value();
  const SelectedArc& target_arc = estimator_context.target().arc;
  ASSERT_NE(target_arc.inputPort(), nullptr);
  ASSERT_NE(target_arc.outputPort(), nullptr);
  EXPECT_EQ(target_arc.inputPort()->name(), "A1");
  EXPECT_EQ(target_arc.outputPort()->name(), "ZN");
  EXPECT_GE(estimator_context.target().output_slew_merge_arcs.size(), 1);
  EXPECT_GT(estimator_context.target().current_model_slew, 0.0f);

  sta::Vertex* driver_vertex = sta_->graph()->pinDrvrVertex(target.driver_pin);
  ASSERT_NE(driver_vertex, nullptr);
  const int dcalc_ap
      = target_arc.scene->dcalcAnalysisPtIndex(target_arc.min_max);
  const float graph_slew = sta::delayAsFloat(sta_->graph()->slew(
      driver_vertex, target_arc.outputRiseFall(), dcalc_ap));
  EXPECT_FLOAT_EQ(estimator_context.target().current_slew, graph_slew);

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(estimator_context, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, MtCandidateEstimateUsesSelectedArcAndCachedState)
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
  sta::LibertyCell* current_cell = sta_->network()->libertyCell(target_inst);
  ASSERT_NE(current_cell, nullptr);

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("BUF_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  VtSwapMtCandidate candidate(resizer, target, target_inst, candidate_cell);

  const Estimate estimate = candidate.estimate();
  ASSERT_TRUE(estimate.legal);
  EXPECT_GT(estimate.score, 0.0f);
}

TEST_F(TestResizerMt, ConcurrentMaxCapChecksAfterStaWarmup)
{
  struct MaxCapCheckCase
  {
    sta::Instance* inst;
    sta::LibertyCell* replacement;
    bool expected;
  };

  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  sta_->updateTiming(true);

  sta::LibertyCell* strong_buffer = sta_->network()->findLibertyCell("BUF_X16");
  ASSERT_NE(strong_buffer, nullptr);

  std::vector<const char*> inst_names{"target", "path_side_buf"};
  std::vector<MaxCapCheckCase> checks;
  checks.reserve(inst_names.size());
  for (const char* inst_name : inst_names) {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    ASSERT_NE(db_inst, nullptr);
    sta::Instance* inst = db_network_->dbToSta(db_inst);
    ASSERT_NE(inst, nullptr);
    const bool expected
        = resizer.replacementPreservesMaxCap(inst, strong_buffer);
    checks.push_back(
        {.inst = inst, .replacement = strong_buffer, .expected = expected});
  }

  // Warm up the STA/max-cap query path on the main thread before stressing
  // concurrent reads.  If this test fails after warmup, the likely root-cause
  // is live mutable state inside Resizer::replacementPreservesMaxCap(),
  // Resizer::checkMaxCapOK(), or Sta::checkCapacitance().
  constexpr int kWarmupChecks = 256;
  for (int index = 0; index < kWarmupChecks; ++index) {
    const MaxCapCheckCase& check = checks[index % checks.size()];
    EXPECT_EQ(resizer.replacementPreservesMaxCap(check.inst, check.replacement),
              check.expected);
  }

  constexpr int kThreadedChecks = 8192;
  std::vector<int> jobs(kThreadedChecks);
  for (int index = 0; index < kThreadedChecks; ++index) {
    jobs[index] = index;
  }

  std::atomic<int> mismatches{0};
  utl::ThreadPool thread_pool(4);
  thread_pool.parallelFor(
      jobs, [&resizer, &checks, &mismatches](const int& job) {
        const MaxCapCheckCase& check = checks[job % checks.size()];
        const bool actual
            = resizer.replacementPreservesMaxCap(check.inst, check.replacement);
        if (actual != check.expected) {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
      });

  EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0)
      << "Concurrent max-cap check diverged after STA warmup; inspect "
         "Resizer::replacementPreservesMaxCap(), Resizer::checkMaxCapOK(), "
         "and Sta::checkCapacitance().";
}

TEST_F(TestResizerMt, ConcurrentSwappableCellsAfterFullWarmup)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  warmupAllSwappableCells(resizer, sta_->network());

  const std::vector<sta::LibertyCell*> cells
      = collectLibertyCells(sta_->network());
  ASSERT_FALSE(cells.empty());

  std::vector<std::string> baseline;
  baseline.reserve(cells.size());
  for (sta::LibertyCell* cell : cells) {
    baseline.push_back(swappableCellsSignature(resizer, cell));
  }

  utl::ThreadPool thread_pool(32);
  constexpr int kRepeatCount = 100;
  for (int repeat = 0; repeat < kRepeatCount; ++repeat) {
    std::vector<std::string> signature = thread_pool.parallelMap(
        cells, [&resizer](sta::LibertyCell* cell) -> std::string {
          return swappableCellsSignature(resizer, cell);
        });
    EXPECT_EQ(signature, baseline)
        << "Concurrent getSwappableCells() diverged after full ST warmup at "
           "repeat "
        << repeat
        << ". If this fails or crashes, inspect Resizer::getSwappableCells() "
           "and Resizer::cellLeakage() cache access.";
  }
}

TEST_F(TestResizerMt, ConcurrentCheckCapacitanceAfterWarmup)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->updateTiming(true);

  const std::vector<sta::Pin*> driver_pins = collectDriverPins(sta_->network());
  ASSERT_FALSE(driver_pins.empty());
  for (const sta::Pin* pin : driver_pins) {
    static_cast<void>(sta_->network()->drivers(pin));
    static_cast<void>(capacitanceSignature(sta_.get(), pin));
  }

  std::vector<std::string> baseline;
  baseline.reserve(driver_pins.size());
  for (const sta::Pin* pin : driver_pins) {
    baseline.push_back(capacitanceSignature(sta_.get(), pin));
  }

  utl::ThreadPool thread_pool(32);
  constexpr int kRepeatCount = 100;
  for (int repeat = 0; repeat < kRepeatCount; ++repeat) {
    std::vector<std::string> signature = thread_pool.parallelMap(
        driver_pins, [this](const sta::Pin* pin) -> std::string {
          return capacitanceSignature(sta_.get(), pin);
        });
    EXPECT_EQ(signature, baseline)
        << "Concurrent checkCapacitance() diverged after ST warmup at repeat "
        << repeat
        << ". If this fails or crashes, inspect Sta::checkCapacitance() and "
           "GraphDelayCalc/Sdc capacitance query state.";
  }
}

TEST(TestResizerMtThreadPool, ParallelForSupportsVoidTasks)
{
  std::vector<int> items(8);
  for (int index = 0; index < static_cast<int>(items.size()); ++index) {
    items[index] = index;
  }

  std::vector<int> visited(items.size(), 0);
  std::atomic<int> visit_count{0};
  utl::ThreadPool thread_pool(4);
  thread_pool.parallelFor(items, [&visited, &visit_count](const int& item) {
    visited[item] = item + 1;
    visit_count.fetch_add(1, std::memory_order_relaxed);
  });

  EXPECT_EQ(visit_count.load(std::memory_order_relaxed),
            static_cast<int>(items.size()));
  for (size_t index = 0; index < visited.size(); ++index) {
    EXPECT_EQ(visited[index], static_cast<int>(index + 1));
  }

  const std::thread::id caller_thread = std::this_thread::get_id();
  bool ran_on_caller_thread = true;
  utl::ThreadPool inline_pool(0);
  inline_pool.parallelFor(
      items, [&ran_on_caller_thread, caller_thread](const int&) {
        ran_on_caller_thread = ran_on_caller_thread
                               && std::this_thread::get_id() == caller_thread;
      });
  EXPECT_TRUE(ran_on_caller_thread);
}

TEST(TestResizerMtThreadPool,
     ParallelMapSupportsNestedTargetGenerateEstimateFanout)
{
  utl::ThreadPool thread_pool(4);

  std::vector<int> targets(8);
  for (int index = 0; index < static_cast<int>(targets.size()); ++index) {
    targets[index] = index;
  }

  // Model a nested pattern: outer multi-target fanout, two generate
  // tasks per target, and ten estimate tasks per generate task.
  std::vector<std::vector<std::vector<int>>> nested_results
      = thread_pool.parallelMap(targets, [&thread_pool](const int& target) {
          std::vector<int> generators{0, 1};
          return thread_pool.parallelMap(
              generators, [&thread_pool, target](const int& generator) {
                std::vector<int> estimates(10);
                for (int estimate_index = 0;
                     estimate_index < static_cast<int>(estimates.size());
                     ++estimate_index) {
                  estimates[estimate_index] = estimate_index;
                }

                return thread_pool.parallelMap(
                    estimates,
                    [target, generator](const int& estimate_index) -> int {
                      return target * 100 + generator * 10 + estimate_index;
                    });
              });
        });

  ASSERT_EQ(nested_results.size(), targets.size());
  for (size_t target_index = 0; target_index < nested_results.size();
       ++target_index) {
    ASSERT_EQ(nested_results[target_index].size(), 2u);
    for (size_t generator_index = 0;
         generator_index < nested_results[target_index].size();
         ++generator_index) {
      ASSERT_EQ(nested_results[target_index][generator_index].size(), 10u);
      for (size_t estimate_index = 0;
           estimate_index
           < nested_results[target_index][generator_index].size();
           ++estimate_index) {
        EXPECT_EQ(nested_results[target_index][generator_index][estimate_index],
                  static_cast<int>(target_index * 100 + generator_index * 10
                                   + estimate_index));
      }
    }
  }
}

TEST(TestResizerMtThreadPool, FutureRemainsUsableAfterPoolDestruction)
{
  utl::ThreadPoolFuture<int> future = []() {
    utl::ThreadPool thread_pool(1);
    // NOLINTNEXTLINE(clang-analyzer-core.StackAddressEscape)
    return thread_pool.submit([]() -> int {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      return 7;
    });
  }();

  EXPECT_EQ(future.get(), 7);
}

TEST(TestResizerMtThreadPool, ParallelForWaitsForAllTasksBeforeRethrow)
{
  std::vector<int> items{0, 1, 2, 3};
  std::atomic<int> completed{0};
  utl::ThreadPool thread_pool(4);

  EXPECT_THROW(thread_pool.parallelFor(
                   items,
                   [&completed](const int& item) {
                     if (item == 0) {
                       throw std::runtime_error("parallelFor failure");
                     }
                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
                     completed.fetch_add(1, std::memory_order_relaxed);
                   }),
               std::runtime_error);

  EXPECT_EQ(completed.load(std::memory_order_relaxed), 3);
}

TEST(TestResizerMtThreadPool, ParallelMapWaitsForAllTasksBeforeRethrow)
{
  std::vector<int> items{0, 1, 2, 3};
  std::atomic<int> completed{0};
  utl::ThreadPool thread_pool(4);

  EXPECT_THROW(thread_pool.parallelMap(
                   items,
                   [&completed](const int& item) -> int {
                     if (item == 0) {
                       throw std::runtime_error("parallelMap failure");
                     }
                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
                     completed.fetch_add(1, std::memory_order_relaxed);
                     return item;
                   }),
               std::runtime_error);

  EXPECT_EQ(completed.load(std::memory_order_relaxed), 3);
}

}  // namespace rsz
