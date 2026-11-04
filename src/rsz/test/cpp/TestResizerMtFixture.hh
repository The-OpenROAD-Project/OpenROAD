// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Shared fixtures and helpers of the TestResizerMt test binary.  Include this
// header first: it opens the private members the tests inspect.

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BatchSelector.hh"
#include "BottleneckAnalysis.hh"
#include "BufferGenerator.hh"
#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RebufferMtCandidate.hh"
#include "RebufferMtGenerator.hh"
#include "RepairSetupAnalysisTarget.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupMoveSampler.hh"
#include "RepairSetupProfiler.hh"
#include "SequentialTimingEstimator.hh"
#include "SetupParallelEvalPolicy.hh"
#include "SizeUpCandidate.hh"
#include "SizeUpMtCandidate.hh"
#include "SizeUpMtGenerator.hh"
#include "SplitLoadMtCandidate.hh"
#include "SplitLoadMtGenerator.hh"
#include "SwapPinsMtCandidate.hh"
#include "UnbufferMtCandidate.hh"
#include "UnbufferMtGenerator.hh"
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "tst/IntegratedFixture.h"
#include "utl/ThreadPool.h"

namespace rsz {

// The tests reach the internals of the rsz classes through this peer, which
// SetupParallelEvalPolicy, StallMonitor, WnsTail, SizeUpMtGenerator,
// RepairTargetCollector and BottleneckAnalysis befriend.
class ResizerTestPeer
{
 public:
  // === SetupParallelEvalPolicy ==============================================
  static std::vector<MoveType>& moveSequence(SetupParallelEvalPolicy& policy)
  {
    return policy.move_sequence_;
  }
  static const OptimizationPolicy::GeneratorVector& moveGenerators(
      const SetupParallelEvalPolicy& policy)
  {
    return policy.move_generators_;
  }
  static OptimizationPolicyConfig& policyConfig(SetupParallelEvalPolicy& policy)
  {
    return policy.policy_config_;
  }
  static void prewarmTargets(SetupParallelEvalPolicy& policy,
                             const std::vector<Target>& targets)
  {
    policy.prewarmTargets(targets);
  }
  static void prepareTargets(SetupParallelEvalPolicy& policy,
                             std::vector<Target>& targets)
  {
    policy.prepareTargets(targets);
  }
  static void prepareTarget(SetupParallelEvalPolicy& policy,
                            Target& target,
                            const PrepareCacheMask mask)
  {
    policy.prepareTarget(target, mask);
  }
  static std::vector<TargetEvaluation> generateCandidates(
      SetupParallelEvalPolicy& policy,
      const std::vector<Target*>& targets)
  {
    return policy.generateCandidates(targets);
  }
  static void estimateTargets(SetupParallelEvalPolicy& policy,
                              std::vector<TargetEvaluation>& evaluations)
  {
    policy.estimateTargets(evaluations);
  }
  static int targetFanout(SetupParallelEvalPolicy& policy, const Target& target)
  {
    return policy.targetFanout(target);
  }
  static bool startWnsTail(SetupParallelEvalPolicy& policy, const char* reason)
  {
    return policy.startWnsTail(reason);
  }
  static size_t collectTargetEndpoints(SetupParallelEvalPolicy& policy)
  {
    return policy.collectTargetEndpoints().target_endpoints;
  }
  static bool withinMaxUtilization(SetupParallelEvalPolicy& policy)
  {
    return policy.withinMaxUtilization();
  }
  static void openBatchJournal(SetupParallelEvalPolicy& policy)
  {
    policy.openBatchJournal();
  }
  static bool finishAtMaxArea(SetupParallelEvalPolicy& policy)
  {
    return policy.finishAtMaxArea();
  }
  static void undoBatch(SetupParallelEvalPolicy& policy) { policy.undoBatch(); }
  static void startSmallBatchRetry(SetupParallelEvalPolicy& policy)
  {
    policy.startSmallBatchRetry();
  }
  static int effectiveBatchCap(SetupParallelEvalPolicy& policy,
                               const size_t failing_endpoint_count)
  {
    return policy.effectiveBatchCap(failing_endpoint_count);
  }
  static void noteMissedBatch(SetupParallelEvalPolicy& policy,
                              const bool utilization_legal)
  {
    policy.noteMissedBatch(utilization_legal);
  }
  static void applyStopRules(SetupParallelEvalPolicy& policy,
                             const int batch_moves)
  {
    policy.applyStopRules(batch_moves);
  }
  static int& committedMoves(SetupParallelEvalPolicy& policy)
  {
    return policy.committed_moves_;
  }
  static int& batchIndex(SetupParallelEvalPolicy& policy)
  {
    return policy.batch_index_;
  }
  static bool& smallBatchRetryStarted(SetupParallelEvalPolicy& policy)
  {
    return policy.small_batch_retry_started_;
  }
  static bool batchJournalOpen(SetupParallelEvalPolicy& policy)
  {
    return policy.batch_journal_open_;
  }
  static BatchSelector& batchSelector(SetupParallelEvalPolicy& policy)
  {
    return *policy.batch_selector_;
  }
  static StallMonitor& stall(SetupParallelEvalPolicy& policy)
  {
    return policy.stall_;
  }
  static WnsTail& tail(SetupParallelEvalPolicy& policy) { return policy.tail_; }
  static float referencePeriod(const SetupParallelEvalPolicy& policy)
  {
    return policy.reference_period_;
  }
  static float findReferencePeriod(const SetupParallelEvalPolicy& policy,
                                   const sta::Slack initial_wns)
  {
    return policy.referencePeriod(initial_wns);
  }
  static sta::Slack checkpointWnsTolerance(
      const SetupParallelEvalPolicy& policy)
  {
    return policy.checkpointWnsTolerance();
  }
  static MissTabu& tabu(SetupParallelEvalPolicy& policy)
  {
    return policy.tabu_;
  }

  // === StallMonitor and WnsTail =============================================
  static int& lastCap(StallMonitor& stall) { return stall.last_cap_; }
  static int& batches(WnsTail& tail) { return tail.batches_; }
  static int& batchesWithoutGain(WnsTail& tail)
  {
    return tail.batches_without_gain_;
  }
  static int& endpoints(WnsTail& tail) { return tail.endpoints_; }
  static bool& greedyStalled(WnsTail& tail) { return tail.greedy_stalled_; }

  // === SizeUpMtGenerator ====================================================
  using SizeUpOptions = SizeUpMtGenerator::SizeUpOptions;
  static const SizeUpOptions& findSizeUpOptions(
      const SizeUpMtGenerator& generator,
      const sta::LibertyPort* drvr_port,
      const sta::Scene* scene,
      const sta::MinMax* min_max)
  {
    return generator.findSizeUpOptions(drvr_port, scene, min_max);
  }
  static SizeUpOptions buildSizeUpOptions(const SizeUpMtGenerator& generator,
                                          const sta::LibertyPort* drvr_port,
                                          const sta::Scene* scene,
                                          const sta::MinMax* min_max)
  {
    return generator.buildSizeUpOptions(drvr_port, scene, min_max);
  }
  static size_t sizeUpOptionCount(const SizeUpMtGenerator& generator)
  {
    return generator.size_up_options_.size();
  }

  // === RepairTargetCollector ================================================
  static sta::Slack endpointPathSlack(const RepairTargetCollector& collector,
                                      sta::Vertex* endpoint)
  {
    return collector.endpointPathSlack(endpoint);
  }
  static utl::ThreadPool* scanThreadPool(const RepairTargetCollector& collector)
  {
    return collector.scan_thread_pool_;
  }

  // === BottleneckAnalysis ===================================================
  // The vertex deficit and accumulated downstream endpoint deficits.
  struct ConeDeficits
  {
    float slack_deficit;
    float endpoint_deficit_sum;
  };
  static std::optional<ConeDeficits> coneDeficits(
      const BottleneckAnalysis& analysis,
      const sta::Vertex* vertex)
  {
    const int32_t node_index = analysis.nodeOf(vertex);
    if (node_index < 0) {
      return std::nullopt;
    }
    const BottleneckAnalysis::Node& node = analysis.nodes_[node_index];
    return ConeDeficits{.slack_deficit = node.slack_deficit,
                        .endpoint_deficit_sum = node.endpoint_deficit_sum};
  }
};

// Shared Nangate45 fixture for MT estimator, policy, and profiler tests.
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

  // Make the unchanged sibling load the unique negative-slack endpoint.
  void prepareUniqueViolatingEndpoint()
  {
    resizer_.runRepairSetupPreamble();
    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    sta_->setOutputDelay(db_network_->findPin("out2"),
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

    resizer_.updateParasiticsAndTiming();
    ASSERT_LT(sta_->worstSlack(resizer_.maxAnalysisMode()), 0.0f);
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

    // The worst slack path reads required times, which updateTiming() leaves
    // to the caller.
    sta_->findRequireds();
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

    return makePathDriverTarget(
        path, expanded, path_index, path->slack(sta_.get()), resizer_);
  }

  GeneratorContext makeGeneratorContext(
      Resizer& resizer,
      MoveCommitter& committer,
      const OptimizerRunConfig& run_config,
      const OptimizationPolicyConfig& policy_config) const
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

// OpenSTA keeps its scaled-model hooks protected.  A derived class may take a
// pointer to a protected member and call it on any base object, so the
// flip-flop tests attach scaled models without changing OpenSTA.
class ScaledTimingArc : public sta::TimingArc
{
 public:
  static void addModel(sta::TimingArc* arc,
                       const sta::OperatingConditions* op_cond,
                       sta::TimingModel* model)
  {
    (arc->*&ScaledTimingArc::addScaledModel)(op_cond, model);
  }
};

class ScaledLibertyPort : public sta::LibertyPort
{
 public:
  static void addPort(sta::LibertyPort* port,
                      sta::OperatingConditions* op_cond,
                      sta::LibertyPort* scaled_port)
  {
    (port->*&ScaledLibertyPort::addScaledPort)(op_cond, scaled_port);
  }
};

// Extends the MT fixture with a sequential cell and connected timing paths.
class TestResizerMtSequential : public TestResizerMt
{
 protected:
  void SetUp() override
  {
    readVerilogAndSetup("TestResizerMt_DelayEstimator.v", true, false);
  }

  void prepareSequential(const char* cell_name, const char* clock_port)
  {
    odb::dbMaster* master = db_->findMaster(cell_name);
    ASSERT_NE(master, nullptr);
    odb::dbInst* sequential = odb::dbInst::create(block_, master, "dq_seq");
    odb::dbInst* clock_driver = odb::dbInst::create(
        block_, db_->findMaster("BUF_X1"), "dq_clock_driver");
    odb::dbNet* q_net = odb::dbNet::create(block_, "dq_q");
    odb::dbNet* clock_net = odb::dbNet::create(block_, "dq_clock");

    // Keep the actual D driver distinct from the propagated clock driver.
    sequential->findITerm("D")->connect(block_->findNet("n1"));
    sequential->findITerm("Q")->connect(q_net);
    sequential->findITerm(clock_port)->connect(clock_net);
    clock_driver->findITerm("A")->connect(block_->findNet("clk"));
    clock_driver->findITerm("Z")->connect(clock_net);
    for (const char* name : {"load0", "load1", "load2"}) {
      block_->findInst(name)->findITerm("A")->connect(q_net);
    }

    int x = 1000;
    for (odb::dbInst* inst : block_->getInsts()) {
      inst->setLocation(x, 1000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      x += 2000;
    }

    sta_->setPropagatedClock(sta_->cmdMode()->sdc()->findClock("clk"),
                             sta_->cmdMode());

    setDataInputDelay(0.10e-9, sta::MinMaxAll::all());
    ep_.estimateWireParasitics();
    resizer_.runRepairSetupPreamble();
    sta_->updateTiming(true);
  }

  void setDataInputDelay(float delay, const sta::MinMaxAll* min_max)
  {
    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    sta_->setInputDelay(db_network_->findPin("in"),
                        sta::RiseFallBoth::riseFall(),
                        sdc->findClock("clk"),
                        sta::RiseFall::rise(),
                        nullptr,
                        false,
                        false,
                        min_max,
                        false,
                        delay,
                        sdc);
  }

  void prepareFlipFlop(const char* cell_name)
  {
    prepareSequential(cell_name, "CK");
    ASSERT_FALSE(HasFatalFailure());
    sta_->removePropagatedClock(sta_->cmdMode()->sdc()->findClock("clk"),
                                sta_->cmdMode());

    sta_->updateTiming(true);
  }

  void setDataSlack(float slack, const sta::MinMax* min_max)
  {
    setDataInputDelay(0.10e-9, min_max->asMinMaxAll());
    sta_->updateTiming(true);
    sta::Vertex* data_vertex
        = sta_->graph()->pinLoadVertex(db_network_->findPin("dq_seq/D"));
    const float current_slack = sta_->slack(data_vertex, min_max);
    const float adjustment = min_max == sta::MinMax::max()
                                 ? current_slack - slack
                                 : slack - current_slack;
    setDataInputDelay(0.10e-9 + adjustment, min_max->asMinMaxAll());
    sta_->updateTiming(true);
    ASSERT_NEAR(sta_->slack(data_vertex, min_max), slack, 1e-15);
  }

  void copyFlipFlopChecks(sta::LibertyCell* source,
                          sta::LibertyCell* destination,
                          const float setup_shift = 0.0f)
  {
    int copied = 0;
    for (const sta::TimingArcSet* source_set : source->timingArcSets()) {
      if (source_set->role() != sta::TimingRole::setup()
          && source_set->role() != sta::TimingRole::hold()) {
        continue;
      }

      for (const sta::TimingArc* source_arc : source_set->arcs()) {
        const sta::CheckTableModel* source_model
            = dynamic_cast<const sta::CheckTableModel*>(source_arc->model());
        ASSERT_NE(source_model, nullptr);
        for (sta::TimingArcSet* destination_set :
             destination->timingArcSets()) {
          const sta::TimingArc* destination_arc
              = findMatchingTimingArc(source_arc, destination_set);
          if (destination_arc == nullptr) {
            continue;
          }

          const sta::CheckTableModel* destination_model
              = dynamic_cast<const sta::CheckTableModel*>(
                  destination_arc->model());
          ASSERT_NE(destination_model, nullptr);
          sta::Table* table = destination_model->checkModel()->table().get();

          *table = sta::Table(*source_model->checkModel()->table());
          ASSERT_EQ(table->order(), 2);
          if (source_set->role() == sta::TimingRole::setup()) {
            for (sta::FloatSeq& row : *table->values3()) {
              for (float& value : row) {
                value += setup_shift;
              }
            }
          }
          ++copied;
        }
      }
    }

    ASSERT_EQ(copied, 4);
  }
};

inline void expectSameDelayContext(const EstimatorContext& lhs,
                                   const EstimatorContext& rhs)
{
  EXPECT_EQ(lhs.target_stage_index, rhs.target_stage_index);
  EXPECT_EQ(lhs.delay_estimation_levels, rhs.delay_estimation_levels);
  EXPECT_FLOAT_EQ(lhs.current_total_delay, rhs.current_total_delay);
  ASSERT_EQ(lhs.path_stages.size(), rhs.path_stages.size());
  for (size_t index = 0; index < lhs.path_stages.size(); ++index) {
    SCOPED_TRACE(index);
    const DelayStageState& left = lhs.path_stages[index];
    const DelayStageState& right = rhs.path_stages[index];
    EXPECT_EQ(left.driver_pin, right.driver_pin);
    EXPECT_EQ(left.path_index, right.path_index);
    EXPECT_EQ(left.arc.ref_arc, right.arc.ref_arc);
    EXPECT_EQ(left.arc.scene, right.arc.scene);
    EXPECT_EQ(left.arc.min_max, right.arc.min_max);
    EXPECT_EQ(left.arc.pvt, right.arc.pvt);
    EXPECT_FLOAT_EQ(left.input_slew, right.input_slew);
    EXPECT_FLOAT_EQ(left.load_cap, right.load_cap);
    EXPECT_FLOAT_EQ(left.current_model_delay, right.current_model_delay);
    EXPECT_FLOAT_EQ(left.current_delay, right.current_delay);
    EXPECT_FLOAT_EQ(left.current_wire_delay, right.current_wire_delay);
    EXPECT_FLOAT_EQ(left.current_model_slew, right.current_model_slew);
    EXPECT_FLOAT_EQ(left.current_slew, right.current_slew);
    ASSERT_EQ(left.output_slew_merge_arcs.size(),
              right.output_slew_merge_arcs.size());
    for (size_t arc = 0; arc < left.output_slew_merge_arcs.size(); ++arc) {
      EXPECT_EQ(left.output_slew_merge_arcs[arc].ref_arc,
                right.output_slew_merge_arcs[arc].ref_arc);
      EXPECT_FLOAT_EQ(left.output_slew_merge_arcs[arc].input_slew,
                      right.output_slew_merge_arcs[arc].input_slew);
      EXPECT_FLOAT_EQ(left.output_slew_merge_arcs[arc].current_model_slew,
                      right.output_slew_merge_arcs[arc].current_model_slew);
    }
  }
}

// Deterministic buffer move used to verify committer accounting in tests.
class StubBufferMoveCandidate : public MoveCandidate
{
 public:
  StubBufferMoveCandidate(Resizer& resizer,
                          const Target& target,
                          const int move_count,
                          const bool accepted = true)
      : MoveCandidate(resizer, target),
        move_count_(move_count),
        accepted_(accepted)
  {
  }

  MoveResult apply() override
  {
    return {.accepted = accepted_,
            .type = MoveType::kBuffer,
            .move_count = accepted_ ? move_count_ : 0,
            .touched_instances = {}};
  }

  MoveType type() const override { return MoveType::kBuffer; }

 private:
  const int move_count_;
  const bool accepted_;
};

// Owns a temporary profiler output directory for one test scope.
class ProfileTestDirectory
{
 public:
  ProfileTestDirectory()
      : directory_(
            (std::filesystem::temp_directory_path() / "rsz-profile-test-XXXXXX")
                .string())
  {
    if (mkdtemp(directory_.data()) == nullptr) {
      throw std::runtime_error("cannot create profiler test directory");
    }
  }

  ~ProfileTestDirectory()
  {
    std::error_code error;
    std::filesystem::remove_all(directory_, error);
    EXPECT_FALSE(error) << error.message();
  }

  std::filesystem::path root() const
  {
    return std::filesystem::path(directory_) / "profile";
  }

 private:
  std::string directory_;
};

inline std::string readProfileFile(const std::filesystem::path& path)
{
  std::ifstream input(path);
  EXPECT_TRUE(input.is_open()) << path;
  std::ostringstream contents;
  contents << input.rdbuf();

  return contents.str();
}

}  // namespace rsz
