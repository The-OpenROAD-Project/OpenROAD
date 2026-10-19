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
class Edge;
class Graph;
class Instance;
class LibertyCell;
class MinMax;
class Net;
class Network;
class Path;
class PathExpanded;
class Pin;
class RiseFall;
class Scene;
class TimingArc;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rsz {

class RebufferMtCandidate;
class Resizer;
struct BufferTreePathStage;
struct BufferTreeSidePathContext;
struct BufferTreeSidePathEvaluation;
struct RebufferTreePlan;
struct StageEvaluation;

// === Diagnostic estimator-accuracy reporter =================================
//
// Compares predicted ECO timing gain with STA measured inside a reverted ECO.
// The report_delay_estimator_accuracy and
// report_delay_estimator_accuracy_for_rebuffering commands run this on the
// main thread.
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
  // Compare a planned buffer tree's target and side-path estimates with STA.
  void reportAccuracyForRebufferTree(sta::Instance* inst,
                                     sta::Pin* driver_pin,
                                     int delay_levels);

 private:
  // Brackets a diagnostic ECO: beginEco on construction; on destruction the
  // ECO is undone and timing restored, even when the diagnostic body throws.
  class EcoJournalScope
  {
   public:
    explicit EcoJournalScope(DelayEstimatorReporter& reporter);
    ~EcoJournalScope();
    EcoJournalScope(const EcoJournalScope&) = delete;
    EcoJournalScope& operator=(const EcoJournalScope&) = delete;

   private:
    DelayEstimatorReporter& reporter_;
  };

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
    bool valid{true};
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
    sta::Pin* input_pin{nullptr};
    sta::Pin* driver_pin{nullptr};
    sta::Pin* next_pin{nullptr};
    std::string input_pin_name;
    std::string driver_pin_name;
    std::string next_pin_name;
  };

  // Golden measurement of one side path of a buffer tree: the rows before
  // and after the tree and the arrival at the load pin.
  struct SideGoldenRow
  {
    const sta::Pin* load_pin{nullptr};
    std::string load_pin_name;
    std::vector<StageProfileRow> before_stage_rows;
    std::vector<StageProfileRow> after_stage_rows;
    float before_arrival{0.0f};
    float after_arrival{0.0f};
    float current_slack{0.0f};
  };

  // One side load of the reported tree: its context, its estimate (null when
  // the target estimate failed) and its golden measurement.
  struct SidePathReport
  {
    const BufferTreeSidePathContext* context{nullptr};
    const BufferTreeSidePathEvaluation* evaluation{nullptr};
    const SideGoldenRow* golden{nullptr};

    // The arrival the side load gains in STA, and its slack after the tree.
    float goldenDelta() const
    {
      return golden->before_arrival - golden->after_arrival;
    }
    float goldenSlackAfter() const
    {
      return golden->current_slack + goldenDelta();
    }
  };

  // === Rebuffer tree accuracy =============================================
  // The side paths with the smallest estimated gain and the smallest estimated
  // slack after the tree, each next to its golden value.
  struct WorstSidePath
  {
    float golden_delta{0.0f};
    float estimated_delta{0.0f};
    float golden_slack_after{0.0f};
    float estimated_slack_after{0.0f};
  };

  // One rebuffer-tree accuracy run: the target path, its tree plan and
  // where the report measures.
  struct RebufferRun
  {
    sta::Instance* inst{nullptr};
    sta::Pin* driver_pin{nullptr};
    std::string driver_pin_name;
    std::string endpoint_name;
    int report_levels{0};
    const sta::Pin* target_load_pin{nullptr};
    std::string target_load_pin_name;
    int target_index{0};
    const RebufferTreePlan* plan{nullptr};
    const std::vector<FixedStage>* fixed_stages{nullptr};
    // The target load is the last captured stage, so its wire is split off.
    bool split_target_load_wire{false};
    const sta::Scene* scene{nullptr};
  };

  // Select the side path with the worst estimated or measured impact.
  static WorstSidePath worstSidePath(
      const std::vector<SidePathReport>& side_paths);
  // One report entry per side context.  The golden rows and the side
  // estimates come in side context order, and the estimates are missing when
  // the target estimate failed.
  static std::vector<SidePathReport> sidePathReports(
      const std::vector<BufferTreeSidePathContext>& side_contexts,
      const std::vector<BufferTreeSidePathEvaluation>& side_evaluations,
      const std::vector<SideGoldenRow>& side_golden_rows);
  // The estimated rows of the current path, with the target-load wire split
  // off and the tree placeholders after the target stage.
  std::vector<StageProfileRow> buildRebufferCurrentStages(
      const EstimatorContext& context,
      const RebufferTreePlan& plan,
      const std::string& target_load_pin_name,
      bool split_target_load_wire) const;
  // Report target and worst-side estimation error for a tree plan.
  void printRebufferSummary(const RebufferRun& run,
                            const DelayEstimate& estimate,
                            float golden_delta,
                            const WorstSidePath& worst_side,
                            const EstimatorProfile& estimator_profile,
                            const GoldenProfile& golden_profile) const;
  // Measure current STA timing through the tree plan's side loads.
  std::vector<SideGoldenRow> captureSideGoldenRows(
      sta::Pin* driver_pin,
      const std::vector<BufferTreeSidePathContext>& side_contexts,
      int report_levels,
      const sta::Scene* scene) const;
  // The estimated rows of the path through the planned tree, from the
  // estimator trace.
  std::vector<StageProfileRow> buildRebufferCandidateStages(
      const EstimatorContext& context,
      const RebufferTreePlan& plan,
      const std::vector<StageEvaluation>& trace,
      const std::string& target_load_pin_name,
      bool split_target_load_wire) const;
  // Applies the tree inside an ECO journal, measures the golden rows and
  // arrivals of the path and its side paths, and undoes the tree.  Returns
  // the number of inserted buffers.
  int applyRebufferTreeForGolden(const RebufferRun& run,
                                 std::vector<StageProfileRow>& candidate_stages,
                                 GoldenProfile& golden_profile,
                                 std::vector<SideGoldenRow>& side_golden_rows);
  // One row per side load: the estimated and the STA side-path change.
  void printSidePathImpactTable(
      const std::vector<SidePathReport>& side_paths) const;
  void printRebufferSidePaths(
      const std::string& rebuffer_driver_pin_name,
      const EstimatorContext& context,
      const RebufferTreePlan& plan,
      const std::vector<SidePathReport>& side_paths) const;

  // The RebufferMt choice for the report target: the candidates the
  // generator makes from the PARALLEL_EVAL context and the one it would
  // commit.  The candidates reference `target`, so the struct stays put.
  struct RebufferReportPlan
  {
    Target target;
    CandidateVector candidates;
    const RebufferMtCandidate* best{nullptr};
  };

  // The PARALLEL_EVAL context the report scores against.
  std::optional<EstimatorContext> rebufferReportContext(
      const Target& target,
      const OptimizationPolicyConfig& policy_config) const;
  // Fills `report_plan` for `report_target` scored in `context`.
  void planRebufferReport(const Target& report_target,
                          const EstimatorContext& context,
                          const OptimizationPolicyConfig& policy_config,
                          RebufferReportPlan& report_plan) const;
  // The estimated rows of the current and the planned path, and the golden
  // rows before the tree is applied.
  void buildRebufferProfiles(const RebufferRun& run,
                             const EstimatorContext& context,
                             const DelayEstimate& estimate,
                             const std::vector<StageEvaluation>& trace,
                             EstimatorProfile& estimator_profile,
                             GoldenProfile& golden_profile) const;

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
  // Both reports stop the same way when the instance drives no setup path.
  void errorNoSetupPathStage(sta::Instance* inst) const;
  std::optional<TargetMatch> findWorstTargetForInstance(sta::Instance* inst,
                                                        int delay_levels,
                                                        sta::Pin* driver_pin
                                                        = nullptr) const;
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
  // The worst-delay arc from input_vertex to driver_vertex, among the
  // relaxed matches of stage's arc or among all arcs; false when none.
  bool pickWorstArc(const DelayStageState& stage,
                    sta::Vertex* input_vertex,
                    sta::Vertex* driver_vertex,
                    int dcalc_ap,
                    bool relaxed_only,
                    sta::Edge*& gate_edge,
                    const sta::TimingArc*& gate_arc,
                    float& delay) const;
  StageProfileRow buildEstimatedCurrentStageRow(const DelayStageState& stage,
                                                int stage_index,
                                                int target_index) const;
  StageProfileRow makeMissingStageRow(Role role,
                                      const std::string& pin_name,
                                      int path_index = -1) const;
  std::string stageDriverPinName(const DelayStageState& stage) const;
  std::string rebufferTreeStageName(const RebufferTreePlan& plan,
                                    int buffer_stage_index) const;
  std::string rebufferTreeStageName(const RebufferTreePlan& plan,
                                    const sta::Pin* load_pin,
                                    int buffer_stage_index) const;
  bool isRebufferTreeWireStage(const BufferTreePathStage& tree_stage) const;
  std::vector<StageProfileRow> buildRebufferTreePlaceholderRows(
      const RebufferTreePlan& plan) const;
  void insertRebufferTreePlaceholders(std::vector<StageProfileRow>& rows,
                                      int insert_index,
                                      const RebufferTreePlan& plan) const;
  // The stages of one side path from the driver through the load pin, plus
  // delay_levels fanout stages past it.
  std::vector<FixedStage> collectSidePathStages(
      const sta::PathExpanded& expanded,
      const sta::Path* path,
      int driver_path_index,
      int load_path_index,
      int delay_levels) const;
  // Resolves the next pin of each captured stage: the input pin of the next
  // stage, the side-path load pin for the stages before it, else the first
  // downstream receiver outside the driver instance.
  void assignNextPins(std::vector<FixedStage>& fixed_stages,
                      const sta::PathExpanded& expanded,
                      const sta::Pin* load_pin,
                      int load_path_index) const;
  // The stages from driver_pin to load_pin on the worst STA path through
  // load_pin, replaced by the OpenDB connectivity chain when that resolves.
  std::vector<FixedStage> captureSidePathFixedStages(
      sta::Pin* driver_pin,
      const sta::Pin* load_pin,
      int delay_levels,
      const FixedStage* reference_stage = nullptr,
      const std::vector<FixedStage>* reference_path_stages = nullptr) const;
  // The stages from driver_pin to load_pin along the OpenDB connectivity, so
  // the rows include the buffers an ECO just inserted.
  std::vector<FixedStage> captureSidePathConnectivityFixedStages(
      sta::Pin* driver_pin,
      const sta::Pin* load_pin,
      const std::vector<FixedStage>& path_fixed_stages,
      const FixedStage* reference_stage = nullptr,
      const std::vector<FixedStage>* reference_path_stages = nullptr) const;
  // The driver of net in the STA network, and the driver of load_pin's net
  // in OpenDB, which already has the buffers an ECO inserted.
  sta::Pin* netDriver(sta::Net* net) const;
  sta::Pin* dbNetDriver(sta::Pin* load_pin) const;
  // The graph arc from input_pin to driver_pin for the two transitions.
  const sta::TimingArc* firstGraphArc(sta::Pin* input_pin,
                                      sta::Pin* driver_pin,
                                      const sta::RiseFall* input_rf,
                                      const sta::RiseFall* output_rf) const;
  std::vector<StageProfileRow> buildSidePathCurrentRows(
      const EstimatorContext& target_context,
      const BufferTreeSidePathContext& side_context,
      const RebufferTreePlan& plan) const;
  std::vector<StageProfileRow> buildSidePathCandidateRows(
      const BufferTreeSidePathContext& side_context,
      const BufferTreeSidePathEvaluation& side_eval,
      const RebufferTreePlan& plan) const;
  // The fanout row of the wire into a load.
  static StageProfileRow loadWireRow(const std::string& pin_name,
                                     int path_index,
                                     float input_slew,
                                     float wire_delay,
                                     float load_cap,
                                     bool arc_match_relaxed);
  // Adds one wire stage of the estimator trace to row.
  static void foldWire(StageProfileRow& row, const StageEvaluation& eval);
  void splitFinalLoadWire(std::vector<StageProfileRow>& rows,
                          const std::string& load_pin_name,
                          const char* row_label) const;
  // Placeholder rows after the driver for the tree buffers the pre-ECO path
  // does not have.
  void insertSidePathPlaceholders(std::vector<StageProfileRow>& rows,
                                  int branch_row_count) const;
  // Names the estimated tree rows after the golden rows of the ECO path.
  void alignSidePathCandidateNames(
      std::vector<StageProfileRow>& candidate_rows,
      const std::vector<StageProfileRow>& golden_after_rows) const;

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
  // The row at index when it exists and is valid, else null.
  static const StageProfileRow* validRow(
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
