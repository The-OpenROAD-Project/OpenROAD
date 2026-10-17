// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "OptimizerTypes.hh"
#include "sta/Delay.hh"

namespace odb {
class dbBlock;
}  // namespace odb

namespace sta {
class dbSta;
class Graph;
class Instance;
class LibertyCell;
class MinMax;
class Network;
class Path;
class PathExpanded;
class Pin;
class Scene;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rsz {

class Resizer;

// === Diagnostic estimator-accuracy reporter =================================
//
// Compares predicted ECO timing gain with STA measured inside a reverted ECO.
// The report_delay_estimator_accuracy command runs this on the main thread.
class DelayEstimatorReporter
{
 public:
  enum class EstimatorKind
  {
    // The estimate of the serial moves: the gate delay at the current load
    // plus the fanin drive resistance times the input pin cap.
    kLegacy,
    // Liberty tables propagated over a window of stages around the target,
    // as the Mt moves estimate.
    kStageWindow
  };

  // Canonical user-facing names accepted by the Tcl command and the
  // C++/Tcl name parser.  Single source of truth for command argument
  // validation.
  static const std::vector<std::string_view>& knownEstimatorNames();
  static std::optional<EstimatorKind> parseEstimatorKind(
      const std::string& estimator);

  explicit DelayEstimatorReporter(Resizer& resizer);

  // Compare a temporary cell swap's predicted and STA-measured path gain.
  void reportAccuracyForSizing(sta::Instance* inst,
                               sta::LibertyCell* replacement,
                               const std::string& estimator,
                               int delay_levels);

 private:
  // === Per-stage roles in a path window ====================================
  enum class Role
  {
    kFanin,
    kTarget,
    kFanout
  };
  static const char* roleName(Role role);
  static Role roleFor(int stage_index, int target_index);

  // === Diagnostic data structures ==========================================
  // Worst matching target path through the requested instance.
  struct TargetMatch
  {
    Target target;
    std::string endpoint_name;
    sta::Slack slack{0.0};
  };

  // Electrical inputs and delay of one reported timing stage.
  struct StageProfileRow
  {
    int path_index{-1};
    Role role{Role::kTarget};
    std::string pin_name;
    std::string cell_name;
    float input_slew{0.0f};
    float cell_delay{0.0f};
    float wire_delay{0.0f};
    float load_cap{0.0f};
    float output_slew{0.0f};
    float extra_delay{0.0f};
    // True when this diagnostic row used relaxed fallback arc matching.
    bool arc_match_relaxed{false};
  };

  // Model-predicted stage rows and legality for one candidate.
  struct EstimatorProfile
  {
    std::vector<StageProfileRow> current_stages;
    std::vector<StageProfileRow> candidate_stages;
    float current_delay{0.0f};
    float candidate_delay{0.0f};
    float arrival_impr{0.0f};
    bool legal{false};
    FailReason fail_reason{FailReason::kNone};
    // True when the estimator failed mid-walk: candidate_stages contains
    // only the prefix that succeeded, so its sum does NOT represent the
    // full path and should be reported as "(incomplete)" rather than as a
    // misleading partial total.
    bool candidate_stages_incomplete{false};
  };

  // STA-measured path stages and arrival change around a temporary ECO.
  struct GoldenProfile
  {
    std::vector<StageProfileRow> before_stages;
    std::vector<StageProfileRow> after_stages;
    std::string arrival_reference_pin_name;
    float before_arrival{0.0f};
    float after_arrival{0.0f};
    float arrival_impr{0.0f};
  };

  // Stable pin identities and prepared timing for one path stage.
  struct FixedStage
  {
    DelayStageState state;
    Role role{Role::kTarget};
    std::string input_pin_name;
    std::string driver_pin_name;
    std::string next_pin_name;
  };

  // === Run-orchestration phases ============================================
  // Apply a temporary cell swap and measure the same path after timing update.
  void measureGoldenAfterSwap(const TargetMatch& before_match,
                              const std::vector<FixedStage>& fixed_stages,
                              GoldenProfile& golden_profile) const;
  void printSummary(sta::Instance* inst,
                    sta::LibertyCell* replacement,
                    const TargetMatch& before_match,
                    const std::string& estimator_label,
                    int report_levels,
                    const EstimatorProfile& estimator_profile,
                    const GoldenProfile& golden_profile) const;

  // === Target selection ====================================================
  // Find the worst path-driver target through the requested instance.
  std::optional<TargetMatch> findWorstTargetForInstance(sta::Instance* inst,
                                                        int delay_levels) const;
  std::vector<sta::Pin*> outputPins(sta::Instance* inst) const;

  // === Profile construction ================================================
  // Build the original serial estimator's stage prediction.
  EstimatorProfile buildLegacyProfile(const Target& target,
                                      sta::LibertyCell* replacement) const;
  // Build the path-window estimator's stage prediction.
  EstimatorProfile buildDelayEstimatorProfile(const Target& target,
                                              sta::LibertyCell* replacement,
                                              int delay_levels) const;
  // Preserve path stage identities before the ECO invalidates path handles.
  std::vector<FixedStage> captureFixedStages(const Target& target,
                                             int delay_levels) const;
  // Measure STA delay through the fixed stages in the current design.
  std::vector<StageProfileRow> buildGoldenStageRows(
      const std::vector<FixedStage>& fixed_stages) const;
  StageProfileRow buildGoldenStageRow(const FixedStage& fixed_stage) const;
  StageProfileRow buildEstimatedCurrentStageRow(
      const sta::PathExpanded& expanded,
      const DelayStageState& stage,
      int stage_index,
      int target_index) const;

  // === Path / context utilities ============================================
  // Prepare the same delay window used by MT candidate scoring.
  std::optional<EstimatorContext> contextForTarget(
      const Target& target,
      int delay_levels,
      FailReason* fail_reason) const;
  void prepareTargetContext(Target& target,
                            sta::Instance* inst,
                            sta::Pin* driver_pin,
                            const sta::PathExpanded& expanded,
                            int path_index,
                            const sta::Path* endpoint_path,
                            int delay_levels) const;
  float pathWireDelay(const sta::PathExpanded& expanded, int path_index) const;
  float fixedStageWireDelay(const FixedStage& fixed_stage) const;
  std::string stagePinName(const sta::PathExpanded& expanded,
                           int path_index) const;
  float arrivalAtPinName(const std::string& pin_name,
                         const sta::Scene* scene) const;
  float arrivalAtDriver(sta::Pin* driver_pin, const sta::Scene* scene) const;

  // === Print helpers =======================================================
  void printStageWindowDetail(
      const std::vector<StageProfileRow>& estimated_current,
      const std::vector<StageProfileRow>& estimated_candidate,
      const std::vector<StageProfileRow>& golden_before,
      const std::vector<StageProfileRow>& golden_after) const;

  // === Stage-row scalar helpers (need access to StageProfileRow) ==========
  static float estimatedStageScoreDelay(const StageProfileRow& row);
  static float goldenStagePathDelay(const StageProfileRow& row);
  static float sumEstimatedStageScoreDelays(
      const std::vector<StageProfileRow>& rows);
  static float sumGoldenStagePathDelays(
      const std::vector<StageProfileRow>& rows);
  static const StageProfileRow& rowOrEmpty(
      const std::vector<StageProfileRow>& rows,
      size_t index);

  // === Cached references to Resizer-owned services =========================
  Resizer& resizer_;
  sta::dbSta* sta_{nullptr};
  sta::Network* network_{nullptr};
  sta::Graph* graph_{nullptr};
  utl::Logger* logger_{nullptr};
  const sta::MinMax* max_min_max_{nullptr};
  odb::dbBlock* block_{nullptr};
};

}  // namespace rsz
