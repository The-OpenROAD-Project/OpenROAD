// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>

#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

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
struct Target;

// === Delay estimator API ====================================================

// Stateless helper for building timing-arc contexts and comparing candidate
// cell delays without mutating STA or OpenDB state.
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
  static std::optional<ArcDelayState> buildContext(const Resizer& resizer,
                                                   const Target& target,
                                                   int delay_levels,
                                                   FailReason* fail_reason
                                                   = nullptr);

  // Lower-level overload used by tests and policy prepare code.
  static std::optional<ArcDelayState> buildContext(
      const Resizer& resizer,
      sta::Instance* inst,
      sta::Pin* driver_pin,
      const sta::PathExpanded& expanded,
      int path_index,
      const sta::Scene* scene,
      const sta::MinMax* min_max,
      int delay_levels,
      FailReason* fail_reason = nullptr);

  // === Delay estimation =====================================================
  static DelayEstimate estimate(const ArcDelayState& context,
                                const sta::LibertyCell* candidate_cell);
  static DelayEstimate estimate(const ArcDelayState& context,
                                const sta::LibertyCell* candidate_cell,
                                float load_cap);
  static DelayEstimate estimate(const SelectedArc& arc,
                                float input_slew,
                                float load_cap,
                                float current_delay,
                                const sta::LibertyCell* candidate_cell);
};

}  // namespace rsz
