// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "DelayEstimator.hh"
#include "rsz/Resizer.hh"
#define private public
#define protected public
#include "SetupLegacyMtPolicy.hh"
#undef protected
#undef private
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sta.hh"
#define protected public
#include "tst/IntegratedFixture.h"
#undef protected
#include "MoveGenerator.hh"

#define private public
#define protected public
#include "SetupMt1Policy.hh"
#undef protected
#undef private

#include "utl/ThreadPool.h"

namespace rsz {

class TestResizerMt : public tst::IntegratedFixture
{
 protected:
  TestResizerMt()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
    readLiberty(getFilePath("_main/test/Nangate45/Nangate45_lvt.lib"));
    if (!updateLib(lib_, "_main/test/Nangate45/Nangate45_lvt.lef")) {
      throw std::runtime_error("failed to load Nangate45_lvt.lef");
    }
  }

  void SetUp() override
  {
    readVerilogAndSetup("TestResizerMt_DelayEstimator.v");
    sta_->updateTiming(true);
  }

  sta::Vertex* findEndpointVertex(const char* endpoint_name) const
  {
    for (sta::Vertex* endpoint : sta_->endpoints()) {
      const sta::Pin* pin = endpoint != nullptr ? endpoint->pin() : nullptr;
      if (pin != nullptr
          && std::string(sta_->network()->pathName(pin)) == endpoint_name) {
        return endpoint;
      }
    }
    return nullptr;
  }

  Target makeTarget(const char* endpoint_name,
                    const char* inst_name,
                    const char* pin_name) const
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    if (db_inst == nullptr) {
      ADD_FAILURE() << "missing db instance " << inst_name;
      return {};
    }

    sta::Instance* inst = db_network_->dbToSta(db_inst);
    if (inst == nullptr) {
      ADD_FAILURE() << "missing sta instance " << inst_name;
      return {};
    }

    sta::Pin* pin = db_network_->findPin(inst, pin_name);
    if (pin == nullptr) {
      ADD_FAILURE() << "missing pin " << inst_name << "/" << pin_name;
      return {};
    }

    sta::Vertex* endpoint = findEndpointVertex(endpoint_name);
    if (endpoint == nullptr) {
      ADD_FAILURE() << "missing endpoint " << endpoint_name;
      return {};
    }

    sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
    if (path == nullptr) {
      ADD_FAILURE() << "missing worst slack path for " << endpoint_name;
      return {};
    }

    sta::PathExpanded expanded(path, sta_.get());
    int path_index = -1;
    for (int index = expanded.startIndex(); index < expanded.size(); ++index) {
      const sta::Path* path_vertex = expanded.path(index);
      if (path_vertex != nullptr && path_vertex->pin(sta_.get()) == pin) {
        path_index = index;
        break;
      }
    }
    if (path_index < 0) {
      ADD_FAILURE() << "missing target pin on path for " << inst_name << "/"
                    << pin_name;
      return {};
    }

    Target target;
    target.kind = TargetKind::kPathDriver;
    target.endpoint_path = path;
    target.driver_path = expanded.path(path_index);
    target.scene = path->scene(sta_.get());
    target.driver_pin = pin;
    target.path_index = path_index;
    target.slack = path->slack(sta_.get());
    return target;
  }

  GeneratorContext makeGeneratorContext(
      Resizer& resizer,
      MoveCommitter& committer,
      const OptimizerRunConfig& run_config,
      const OptPolicyConfig& policy_config) const
  {
    return GeneratorContext{.resizer = resizer,
                            .committer = committer,
                            .run_config = run_config,
                            .policy_config = policy_config};
  }

  void placeInst(const char* inst_name, int x, int y) const
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    ASSERT_NE(db_inst, nullptr);
    db_inst->setLocation(x, y);
    db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
};

std::vector<std::string> evaluationSignature(
    const std::vector<TargetEvaluation>& evaluations)
{
  std::vector<std::string> signature;
  signature.reserve(evaluations.size());
  for (const TargetEvaluation& evaluation : evaluations) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(9);
    oss << "c=" << evaluation.candidates.size();
    oss << ";e=" << evaluation.estimates.size();
    oss << ";best=" << evaluation.hasBestCandidate();
    oss << ";bestLegal=" << evaluation.best_estimate.legal;
    oss << ";bestScore=" << evaluation.best_estimate.score;
    for (size_t index = 0; index < evaluation.candidates.size(); ++index) {
      oss << "|" << moveName(evaluation.candidates[index]->type());
      oss << ":" << evaluation.estimates[index].legal;
      oss << ":" << evaluation.estimates[index].score;
    }
    signature.push_back(oss.str());
  }
  return signature;
}

std::vector<TargetEvaluation> generateAndEstimateTargetsInParallel(
    SetupMt1Policy& policy,
    const std::vector<Target>& targets)
{
  return policy.thread_pool_->parallelMap(
      targets, [&policy](const Target& target) -> TargetEvaluation {
        return policy.generateAndEstimateTarget(target);
      });
}

void warmupAllSwappableCells(Resizer& resizer, sta::Network* network)
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

TEST_F(TestResizerMt, BuildArcDelayStateAndEstimateLvtCandidate)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();

  const Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }
  const ArcDelayState& arc_delay = context.value();
  ASSERT_NE(arc_delay.arc.input_port, nullptr);
  ASSERT_NE(arc_delay.arc.output_port, nullptr);
  EXPECT_EQ(arc_delay.arc.input_port->name(), "A");
  EXPECT_EQ(arc_delay.arc.output_port->name(), "Z");
  EXPECT_GT(arc_delay.load_cap, 0.0f);
  EXPECT_GT(arc_delay.current_delay, 0.0f);

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("BUF_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(arc_delay, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_GT(estimate.arrival_impr, 0.0f);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, RejectCandidateWithMismatchedOutputPort)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }
  const ArcDelayState& arc_delay = context.value();

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(arc_delay, candidate_cell);
  EXPECT_FALSE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kMissingCandidatePort);
}

TEST_F(TestResizerMt, BuildArcDelayStateSamplesOnlyPathArcOnMultiArcCell)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("path_out", "path_target", "ZN");

  FailReason fail_reason = FailReason::kNone;
  auto context = DelayEstimator::buildContext(resizer, target, &fail_reason);
  ASSERT_TRUE(context.has_value()) << failReasonName(fail_reason);
  if (!context.has_value()) {
    return;
  }
  const ArcDelayState& arc_delay = context.value();
  ASSERT_NE(arc_delay.arc.input_port, nullptr);
  ASSERT_NE(arc_delay.arc.output_port, nullptr);
  EXPECT_EQ(arc_delay.arc.input_port->name(), "A1");
  EXPECT_EQ(arc_delay.arc.output_port->name(), "ZN");

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("NAND2_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  const DelayEstimate estimate
      = DelayEstimator::estimate(arc_delay, candidate_cell);
  EXPECT_TRUE(estimate.legal);
  EXPECT_EQ(estimate.reason, FailReason::kEstimateLegal);
}

TEST_F(TestResizerMt, Mt1CandidateEstimateUsesSelectedArcAndCachedState)
{
  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  const Target target = makeTarget("out0", "target", "Z");

  FailReason fail_reason = FailReason::kNone;
  const std::optional<ArcDelayState> arc_delay
      = DelayEstimator::buildContext(resizer, target, &fail_reason);
  ASSERT_TRUE(arc_delay.has_value()) << failReasonName(fail_reason);
  if (!arc_delay.has_value()) {
    return;
  }

  sta::Instance* target_inst = target.inst(resizer);
  sta::LibertyCell* current_cell = sta_->network()->libertyCell(target_inst);
  ASSERT_NE(current_cell, nullptr);

  sta::LibertyCell* candidate_cell
      = sta_->network()->findLibertyCell("BUF_X1_L");
  ASSERT_NE(candidate_cell, nullptr);

  VtSwapMtCandidate candidate(resizer,
                              target,
                              target.driver_pin,
                              target_inst,
                              current_cell,
                              candidate_cell,
                              arc_delay.value());

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

  // Model the Mt1 nesting pattern: outer multi-target fanout, two generate
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

TEST_F(TestResizerMt,
       SetupMt1PolicyThreadPoolSupportsNestedMultiTargetGenerateEstimate)
{
  sta_->setThreadCount(4);

  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();

  MoveCommitter committer(resizer);
  SetupMt1Policy policy(resizer, committer);

  OptimizerRunConfig config;
  config.setup_slack_margin = 1.0;
  policy.start(config);

  ASSERT_NE(policy.thread_pool_, nullptr);
  ASSERT_GE(policy.thread_pool_->threadCount(), 1u);

  std::vector<Target> targets;
  targets.push_back(makeTarget("path_out", "path_pre1", "Z"));
  targets.push_back(makeTarget("path_out", "path_target", "ZN"));
  targets.push_back(makeTarget("out0", "target", "Z"));
  ASSERT_EQ(targets.size(), 3u);

  // Fully warm up swappable-cell and leakage caches on the main thread before
  // worker threads perform target-local generation and estimation.
  warmupAllSwappableCells(resizer, sta_->network());
  policy.prewarmTargets(targets);
  policy.prepareTargets(targets);
  for (const Target& target : targets) {
    ASSERT_NE(target.driver_pin, nullptr);
    ASSERT_TRUE(target.isPrepared(PrepareCacheKind::kArcDelayState));
  }

  // Exercise the same production path: prepared targets are walked serially,
  // while each target uses the shared pool for move-type and candidate fanout.
  std::vector<TargetEvaluation> evaluations
      = policy.generateAndEstimateTargets(targets);

  ASSERT_EQ(evaluations.size(), targets.size());

  int evaluations_with_candidates = 0;
  int evaluations_with_multiple_estimates = 0;
  int evaluations_with_legal_best = 0;
  for (size_t index = 0; index < evaluations.size(); ++index) {
    const TargetEvaluation& evaluation = evaluations[index];
    ASSERT_NE(targets[index].driver_pin, nullptr);
    EXPECT_EQ(evaluation.candidates.size(), evaluation.estimates.size());

    if (!evaluation.candidates.empty()) {
      ++evaluations_with_candidates;
    }
    if (evaluation.estimates.size() >= 2) {
      ++evaluations_with_multiple_estimates;
    }

    for (const std::unique_ptr<MoveCandidate>& candidate :
         evaluation.candidates) {
      ASSERT_NE(candidate, nullptr);
      EXPECT_TRUE(candidate->type() == MoveType::kVtSwap
                  || candidate->type() == MoveType::kSizeUp);
    }

    if (evaluation.hasBestCandidate()) {
      ++evaluations_with_legal_best;
      const MoveCandidate* best_candidate = &evaluation.bestCandidate();
      size_t best_candidate_index = 0;
      while (best_candidate_index < evaluation.candidates.size()
             && evaluation.candidates[best_candidate_index].get()
                    != best_candidate) {
        ++best_candidate_index;
      }
      ASSERT_LT(best_candidate_index, evaluation.candidates.size());
      ASSERT_LT(best_candidate_index, evaluation.estimates.size());
      EXPECT_TRUE(evaluation.best_estimate.legal);
      EXPECT_FLOAT_EQ(evaluation.best_estimate.score,
                      evaluation.estimates[best_candidate_index].score);
    }
  }

  EXPECT_GE(evaluations_with_candidates, 2);
  EXPECT_GE(evaluations_with_multiple_estimates, 2);
  EXPECT_GE(evaluations_with_legal_best, 1);
}

TEST_F(TestResizerMt,
       NestedTargetParallelGenerateEstimateIsDeterministic32Threads)
{
  sta_->setThreadCount(33);

  Resizer& resizer = resizer_;
  resizer.runRepairSetupPreamble();
  sta_->updateTiming(true);

  MoveCommitter committer(resizer);
  SetupMt1Policy policy(resizer, committer);

  OptimizerRunConfig config;
  config.setup_slack_margin = 1.0;
  policy.start(config);

  ASSERT_NE(policy.thread_pool_, nullptr);
  ASSERT_EQ(policy.thread_pool_->threadCount(), 32u);

  std::vector<Target> targets;
  targets.reserve(12);
  for (int repeat = 0; repeat < 4; ++repeat) {
    targets.push_back(makeTarget("path_out", "path_pre1", "Z"));
    targets.push_back(makeTarget("path_out", "path_target", "ZN"));
    targets.push_back(makeTarget("out0", "target", "Z"));
  }

  // Match the production Mt1 pipeline: prewarm shared Liberty/cell caches
  // before worker threads perform target-local generation and estimation.
  policy.prewarmTargets(targets);
  policy.prepareTargets(targets);
  for (const Target& target : targets) {
    ASSERT_NE(target.driver_pin, nullptr);
    ASSERT_TRUE(target.isPrepared(PrepareCacheKind::kArcDelayState));
  }

  const std::vector<TargetEvaluation> baseline_evaluations
      = generateAndEstimateTargetsInParallel(policy, targets);
  const std::vector<std::string> baseline_signature
      = evaluationSignature(baseline_evaluations);
  ASSERT_EQ(baseline_signature.size(), targets.size());

  constexpr int kRepeatCount = 100;
  for (int repeat = 0; repeat < kRepeatCount; ++repeat) {
    const std::vector<TargetEvaluation> evaluations
        = generateAndEstimateTargetsInParallel(policy, targets);
    const std::vector<std::string> signature = evaluationSignature(evaluations);
    EXPECT_EQ(signature, baseline_signature)
        << "Nested parallel target generate/estimate produced a different "
           "signature at repeat "
        << repeat
        << ". If this fails or crashes, inspect live STA access in "
           "SizeUpMtGenerator::generate(), VtSwapMtCandidate::estimate(), "
           "Resizer::replacementPreservesMaxCap(), and "
           "Sta::checkCapacitance().";
  }
}

}  // namespace rsz
