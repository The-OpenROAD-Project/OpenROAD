// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "OptimizerTypes.hh"

namespace sta {
class Instance;
class LibertyCell;
class LibertyPort;
class MinMax;
class PathExpanded;
class Pin;
class Pvt;
class RiseFall;
class Scene;
class TimingArc;
}  // namespace sta

namespace rsz {
class Resizer;
struct RebufferTreePlan;
struct Target;

// Negative-region slack lost by the opposite output transition when its delay
// grows by delay_delta; zero when the transition stays non-critical.
float oppositeRfSideLoss(float opposite_slack, float delay_delta);

// Weights of the fanin side loads in buildContext().  Loads in
// critical_load_pins, the worst paths of all violating endpoints, keep weight
// 1 and the others take off_path_weight.  A null set keeps weight 1 for all.
struct FaninSideLoadOptions
{
  const std::unordered_set<const sta::Pin*>* critical_load_pins{nullptr};
  float off_path_weight{0.0f};
};

// The TNS weight of a side load: 1 on the worst path of a violating endpoint,
// off_path_weight elsewhere.  A null set puts every load on a worst path.
inline float sideLoadWeight(
    const sta::Pin* pin,
    const std::unordered_set<const sta::Pin*>* critical_load_pins,
    const float off_path_weight)
{
  return critical_load_pins == nullptr || critical_load_pins->contains(pin)
             ? 1.0f
             : off_path_weight;
}

// Per-stage trace entry that the estimate functions (estimate() and the
// buffer-tree estimates) produce when given a trace.  Captures the inputs and
// outputs of the table-model lookup that was applied (with the candidate at the
// target stage) so diagnostic tools can present per-stage detail without
// re-implementing the estimator's stage walk.
struct StageEvaluation
{
  int path_index{-1};
  const sta::LibertyCell* cell{nullptr};
  float input_slew{0.0f};
  float load_cap{0.0f};
  // Cell delay, not wire delay, except for the wire and via entries of a
  // buffer tree, which have no cell and carry their wire delay here.
  float stage_delay{0.0f};
  float output_slew{0.0f};
  float wire_delay{0.0f};
  // True when the target candidate lookup used relaxed (port-name+RF)
  // matching because no exact conditional/default arc was found.
  bool arc_match_relaxed{false};
};

// === Buffer-tree side paths ================================================

// A side path of a buffer tree: the worst path through one load the tree
// moves, with its current timing.
struct BufferTreeSidePathContext
{
  const sta::Pin* load_pin{nullptr};
  float current_driver_to_load_delay{0.0f};
  float current_slack{0.0f};
  // TNS-loss weight of the side load.  The side slack floor ignores it.
  float weight{1.0f};
};

// The estimate of one side path, for the accuracy report.
struct BufferTreeSidePathEvaluation
{
  const sta::Pin* load_pin{nullptr};
  // False when the side path has no estimate or falls below the side slack
  // floor.
  bool legal{false};
  float arrival_impr{0.0f};
  float slack_after{0.0f};
  std::vector<StageEvaluation> trace;
};

// === Delay estimator API ====================================================

// Reuses delay-estimator preparation results across targets in one policy run.
class DelayEstimatorCache
{
 public:
  // Discards results from the previous policy run.
  void clear();

  // Computes or reuses the maximum cap increase for a target input port.
  float maxEquivalentInputCapIncrease(Resizer& resizer,
                                      const DelayStageState& target_stage);

  // Reuses a slew-bias model or samples live STA on a main-thread miss.
  SlewBiasModel getOrSampleSlewBias(Resizer& resizer,
                                    const DelayStageState& stage,
                                    float max_load_delta);

 private:
  // Identifies one fanin-driver slew-bias sample by its electrical state.
  struct SlewBiasModelKey
  {
    // One arc contributing to the driver's merged output slew.
    struct MergeArc
    {
      // Liberty arc contributing to the merged output slew.
      const sta::TimingArc* ref_arc{nullptr};
      // Input slew used for this arc.
      float input_slew{0.0f};

      // Compares the arc and its input slew.
      bool operator==(const MergeArc&) const = default;
    };

    // Output pin of the fanin driver being sampled.
    const sta::Pin* driver_pin{nullptr};
    // Selected timing arc of that driver.
    const sta::TimingArc* ref_arc{nullptr};
    // STA scene containing the selected arc.
    const sta::Scene* scene{nullptr};
    // Analysis sense used for the timing sample.
    const sta::MinMax* min_max{nullptr};
    // Current Liberty cell of the fanin driver in this scene.
    const sta::LibertyCell* current_cell{nullptr};
    // Input slew at the fanin driver.
    float input_slew{0.0f};
    // Current load capacitance at the fanin driver.
    float load_cap{0.0f};
    // Maximum load increase covered by the sample.
    float max_load_delta{0.0f};
    // c2 capacitance of the reduced Pi parasitic.
    float pi_c2{0.0f};
    // Resistance of the reduced Pi parasitic.
    float pi_r{0.0f};
    // c1 capacitance of the reduced Pi parasitic.
    float pi_c1{0.0f};
    // Additional arcs that affect the driver's merged output slew.
    std::vector<MergeArc> merge_arcs;

    // Compares every fixed field and the merge-arc sequence.
    bool operator==(const SlewBiasModelKey&) const = default;

    // Hashes the same fields as operator== for the cache lookup.
    struct Hash
    {
      size_t operator()(const SlewBiasModelKey& key) const;
    };
  };

  // Captures the driver and parasitic state read by the slew-bias sampler.
  static std::optional<SlewBiasModelKey> makeSlewBiasModelKey(
      Resizer& resizer,
      const DelayStageState& stage,
      float max_load_delta);

  // Largest equivalent-cell input-cap increase per scene input port.
  std::unordered_map<const sta::LibertyPort*, float>
      max_equiv_input_cap_increase_by_port;
  // Reuses sampled slew-bias models for identical driver states.
  using SlewBiasModelMap = std::
      unordered_map<SlewBiasModelKey, SlewBiasModel, SlewBiasModelKey::Hash>;
  SlewBiasModelMap slew_bias_models;
};

// Stateless delay model of the MT moves.  buildContext() reads STA on the main
// thread into an EstimatorContext.  The estimate functions read only that
// snapshot and Liberty tables, so they are safe on worker threads.
//
// With use_sta_slew_bias, buildContext() samples by changing STA's reduced Pi
// model and restores it afterwards.
class DelayEstimator
{
 public:
  // === Arc delay lookup =====================================================
  static bool findArcDelay(const SelectedArc& arc,
                           float input_slew,
                           float load_cap,
                           const sta::LibertyCell* cell,
                           float& delay);
  // === Context construction =================================================
  // The window holds delay_levels stages before and after the target stage.
  // With include_latch_fanin, a latch at the path start also needs its D-side
  // driver, stitched in with path_index -1, or the context fails.
  // fanin_side_loads records the fanin nets' other loads to charge their loss.
  static std::optional<EstimatorContext> buildContext(
      Resizer& resizer,
      const Target& target,
      int delay_levels,
      FailReason* fail_reason = nullptr,
      bool use_sta_slew_bias = false,
      bool include_latch_fanin = false,
      DelayEstimatorCache* cache = nullptr,
      const FaninSideLoadOptions* fanin_side_loads = nullptr,
      bool dual_rf_eval = false);

  // Lower-level overload used by tests and policy prepare code.
  static std::optional<EstimatorContext> buildContext(
      Resizer& resizer,
      sta::Instance* inst,
      sta::Pin* driver_pin,
      const sta::PathExpanded& expanded,
      int path_index,
      const sta::Scene* scene,
      const sta::MinMax* min_max,
      int delay_levels,
      FailReason* fail_reason = nullptr,
      bool use_sta_slew_bias = false,
      bool include_latch_fanin = false,
      DelayEstimatorCache* cache = nullptr,
      const FaninSideLoadOptions* fanin_side_loads = nullptr,
      bool dual_rf_eval = false);

  // === Delay estimation =====================================================
  // When `trace` is non-null it is cleared and one StageEvaluation per
  // evaluated stage is appended; pass nullptr (the default) for normal
  // candidate ranking.
  static DelayEstimate estimate(const EstimatorContext& context,
                                const sta::LibertyCell* candidate_cell,
                                std::vector<StageEvaluation>* trace = nullptr);
  // Scores a load change of the target stage over the same window as
  // estimate(); the target keeps its cell.
  static DelayEstimate estimateLoadChange(const EstimatorContext& context,
                                          float target_load_delta);
  // Delay of buffer_cell driving load_cap, on the output transition of the
  // target stage.
  static bool findBufferDelay(const EstimatorContext& context,
                              const sta::LibertyCell* buffer_cell,
                              float input_slew,
                              float load_cap,
                              float& delay);
  // Charges the moved loads for an arrival change of delay_delta, like fanin
  // side loads: the weighted negative-region loss goes to side_loss, and a load
  // pushed below the side slack floor makes the estimate illegal.
  static void chargeMovedLoads(const EstimatorContext& context,
                               const std::vector<SideLoad>& loads,
                               float delay_delta,
                               DelayEstimate& estimate);

  // === Buffer-tree estimation (DelayEstimatorBufferTree.cc) =================
  // The target path through the tree, less the summed negative-region slack
  // loss of the side paths.  A side path pushed below the side_slack_floor
  // guard makes the estimate illegal, as a fanin side load does.
  static DelayEstimate estimateBufferTreeWithSidePaths(
      const EstimatorContext& context,
      const RebufferTreePlan& plan,
      const std::vector<BufferTreeSidePathContext>& side_contexts,
      std::vector<StageEvaluation>* trace = nullptr,
      std::vector<BufferTreeSidePathEvaluation>* side_evaluations = nullptr);
};

}  // namespace rsz
