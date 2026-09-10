// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>
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
}  // namespace sta

namespace rsz {
class Resizer;
struct Target;

// Per-stage trace entry produced by the trace overload of
// DelayEstimator::estimate.  Captures the inputs and outputs of the
// table-model lookup that was applied (with the candidate at the target
// stage) so diagnostic tools can present per-stage detail without
// re-implementing the estimator's stage walk.
struct StageEvaluation
{
  int path_index{-1};
  const sta::LibertyCell* cell{nullptr};
  float input_slew{0.0f};
  float load_cap{0.0f};
  float stage_delay{0.0f};  // Calibrated cell delay, not wire delay.
  float output_slew{0.0f};
  // True when the target candidate lookup used relaxed (port-name+RF)
  // matching because no exact conditional/default arc was found.
  bool arc_match_relaxed{false};
};

// === Delay estimator API ====================================================

// Stateless helper for building timing-arc contexts and comparing candidate
// cell delays.  The default buildContext() path only reads STA/OpenDB state.
// The optional STA slew-bias path temporarily mutates and restores STA's
// reduced Pi model during main-thread prepare.
//
// All methods are static.  buildContext() queries STA (main-thread only) to
// capture arc, slew, and cap into a Target::arc_delay.  estimate()
// evaluates the Liberty table model for a candidate cell given that context
// and is thread-safe (reads only Liberty data structures, no STA graph
// mutation). This split lets MT generators call estimate() freely from worker
// threads as long as the ArcDelayState was prepared on the main thread first.
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
  // delay_levels=0 captures the target stage only; N>0 also captures up to N
  // valid fanin and N valid fanout stages from the selected path.
  static std::optional<ArcDelayState> buildContext(Resizer& resizer,
                                                   const Target& target,
                                                   int delay_levels,
                                                   FailReason* fail_reason
                                                   = nullptr,
                                                   bool use_sta_slew_bias
                                                   = false);

  // Lower-level overload used by tests and policy prepare code.
  static std::optional<ArcDelayState> buildContext(
      Resizer& resizer,
      sta::Instance* inst,
      sta::Pin* driver_pin,
      const sta::PathExpanded& expanded,
      int path_index,
      const sta::Scene* scene,
      const sta::MinMax* min_max,
      int delay_levels,
      FailReason* fail_reason = nullptr,
      bool use_sta_slew_bias = false);

  // === Delay estimation =====================================================
  // When `trace` is non-null it is cleared and one StageEvaluation per
  // evaluated stage is appended; pass nullptr (the default) for normal
  // candidate ranking.
  static DelayEstimate estimate(const ArcDelayState& context,
                                const sta::LibertyCell* candidate_cell,
                                std::vector<StageEvaluation>* trace = nullptr);
  static DelayEstimate estimate(const SelectedArc& arc,
                                float input_slew,
                                float load_cap,
                                float current_delay,
                                const sta::LibertyCell* candidate_cell);
};

}  // namespace rsz
