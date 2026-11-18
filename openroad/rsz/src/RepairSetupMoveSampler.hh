// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "RepairSetupMoveSnapshot.hh"

namespace sta {
class ClockEdge;
class MinMax;
class Path;
class Pin;
class PathExpanded;
class RiseFall;
class Scene;
class TimingArc;
}  // namespace sta

namespace rsz {

class Resizer;
struct Target;

// Captures timing and topology before and after one ECO move for analysis.
// Construct it before apply() on the main thread; the caller refreshes RC and
// STA before the second sample.
class RepairSetupMoveSampler
{
 public:
  // === Construction =========================================================
  // policy_endpoint is the endpoint the policy repairs; a path target that
  // does not carry its violated endpoint reports it.
  RepairSetupMoveSampler(Resizer& resizer,
                         const Target& target,
                         const sta::Pin* original_endpoint = nullptr,
                         const sta::Pin* policy_endpoint = nullptr);

  // === Sampling =============================================================
  // Return numeric QoR and target-window measurements in seconds; omit
  // unconstrained values.
  std::map<std::string, double> sample() const;
  // Capture the full path, pin, arc, and connectivity record for this move.
  AnalysisSnapshot snapshot();

  // === Move identity ========================================================
  const std::string& targetPin() const { return target_pin_name_; }
  const std::string& selectedEndpoint() const { return endpoint_pin_name_; }
  const std::string& violatedEndpoint() const
  {
    return violated_endpoint_pin_name_;
  }
  const std::string& selectedPathEnd() const { return selected_path_end_; }
  // Where the selected path ends: at the selected endpoint, at the target
  // driver (a prefix), elsewhere (a latch fanin segment), or nowhere.
  const char* pathContext() const
  {
    if (selected_path_end_.empty()) {
      return "instance_only";
    }
    if (selected_path_end_ == endpoint_pin_name_) {
      return "endpoint_path";
    }
    return selected_path_end_ == target_pin_name_ ? "driver_prefix"
                                                  : "latch_fanin_segment";
  }
  const std::string& selectedRf() const { return selected_rf_; }
  const std::string& selectedInput() const { return selected_input_; }
  const std::vector<AnalysisWindowStage>& selectedWindow() const
  {
    return selected_window_;
  }

 private:
  // Stable identity of a selected path point across timing graph refreshes.
  struct PathPoint
  {
    std::string pin_name;
    std::string scene_name;
    const sta::MinMax* min_max;
    const sta::RiseFall* rf;
    const sta::ClockEdge* clock_edge;
    // Liberty arcs survive cell replacement; no graph/path/pin pointers
    // persist.
    const sta::TimingArc* arc;
    bool wire;
    bool bidirect_driver;
  };

  // === Capture at construction =============================================
  // Save the original target path before the ECO invalidates path handles.
  void captureWindow(const Target& target);
  // Save the estimator stages corresponding to that target path.
  void captureSelectedWindow(const Target& target);
  // Track one pin and the role it plays in the move neighborhood.
  void watchPin(const sta::Pin* pin, const std::string& role);
  // Add connected side loads and nearby clock/data pins to the observation.
  void watchNeighborhood();

  // === Snapshot parts =======================================================
  // The timing of every watched pin, one row per scene, transition and
  // analysis.
  void capturePinStates(AnalysisSnapshot& snapshot) const;
  // One row of capturePinStates(): the pin's arrival, required, slack, slew
  // and loads.
  AnalysisPinState captureElectrical(const sta::Pin* pin,
                                     const std::set<std::string>& roles,
                                     const sta::Scene* scene,
                                     const sta::RiseFall* rf,
                                     const sta::MinMax* sense) const;
  // The graph delays of the timing arcs of the watched instances.
  void captureArcStates(AnalysisSnapshot& snapshot) const;
  // One path as named points, for the before -> after path tables.
  AnalysisPathState capturePath(const sta::Path* path,
                                const std::string& kind) const;
  // The pins on the target driver's net now.
  std::set<std::string> targetNetPins() const;
  // Require the original path identity before comparing its window delay.
  bool matchesPath(const sta::PathExpanded& expanded) const;
  // Sum delays of the prepared path stages after the timing refresh.
  std::optional<double> windowDelay() const;
  // The worst propagated slack at the named pin, without a path search;
  // unset when the pin is gone or unconstrained.
  std::optional<double> pinSlack(const std::string& pin_name) const;

  // === State ================================================================
  Resizer& resizer_;
  std::string target_pin_name_;
  std::string endpoint_pin_name_;
  std::string violated_endpoint_pin_name_;
  std::string selected_path_end_;
  // Original target-net membership; a structural ECO invalidates the window.
  std::set<std::string> target_net_pin_names_;
  // Path identity saved as names and Liberty arcs, not transient graph paths.
  std::vector<PathPoint> path_points_;
  // Indices of the stages charged by the estimator window.
  std::vector<std::size_t> stage_indices_;
  std::vector<AnalysisWindowStage> selected_window_;
  std::string selected_rf_;
  std::string selected_input_;
  // Full pin names and their diagnostic roles, resolved at sampling time.
  std::map<std::string, std::set<std::string>> watched_pins_;
  std::set<std::string> watched_nets_;
};

}  // namespace rsz
