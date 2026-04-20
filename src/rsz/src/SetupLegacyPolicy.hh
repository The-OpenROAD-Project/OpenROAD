// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace sta {
class PathExpanded;
}

namespace est {
class EstimateParasitics;
}

namespace rsz {
class Resizer;
}

namespace rsz {

// Single-threaded setup-repair policy (default, RSZ_POLICY=legacy or unset).
//
// This policy replicates the original pre-optimizer repair_setup flow so that
// existing regression results are preserved when no MT policy is requested.
//
// Three-phase structure:
//   Phase 1 – Main repair loop: endpoints are sorted by decreasing violation,
//     each expanded into path-driver targets.  For each target the policy
//     tries every type in `move_sequence_` order; the first accepted move
//     counts, then the endpoint is re-evaluated.  ECO journals checkpoint
//     every pass and are rolled back if slack degrades.
//   Phase 2 – Custom phase schedule (if `phases` is non-empty): WNS, TNS,
//     directional (fanin/fanout), and startpoint-driven sub-phases.
//   Phase 3 – Last-gasp: a lighter single-pass sweep over still-violating
//     endpoints with VT-swap-only or a broad sequence, plus critical-cell
//     VT fanin-cone sweep (swapVTCritCells).
//
// All work runs on the calling thread; no ThreadPool is used.
class SetupLegacyPolicy : public OptPolicy
{
 public:
  // === OptPolicy entry points ==============================================
  SetupLegacyPolicy(Resizer& resizer, MoveCommitter& committer);

  const char* name() const override { return "SetupLegacyPolicy"; }
  void start(const OptimizerRunConfig& config) override;
  void iterate() override;

  // === Single-endpoint repair API ==========================================
  bool repairSetupPin(const sta::Pin* end_pin);

 protected:
  using ViolatingEnds = std::vector<std::pair<sta::Vertex*, sta::Slack>>;

  // === Phase state ==========================================================

  // Mutable progress counters for the main endpoint repair loop (Phase 1).
  // Resets on each outer iteration when endpoints are re-sorted.  The
  // two_cons_terminations flag triggers convergence: two consecutive
  // iterations where TNS improvement falls below fix_rate_threshold.
  struct MainRepairState
  {
    int end_index{0};
    int max_end_count{0};
    int num_viols{0};
    int opto_iteration{0};
    float initial_tns{0.0f};
    float prev_tns{0.0f};
    float fix_rate_threshold{0.0f};
    bool prev_termination{false};
    bool two_cons_terminations{false};
    char phase_marker{'*'};
  };

  // Per-endpoint pass state managed under an ECO journal.
  // At the start of each endpoint the committer opens a journal
  // (journal_open=true).  Each pass records end_slack / worst_slack and
  // compares them to prev_*_slack; if slack degrades for
  // decreasing_slack_max_passes_ consecutive passes, the journal is
  // restored to the last saved checkpoint and the endpoint is abandoned.
  struct EndpointRepairState
  {
    sta::Vertex* end{nullptr};
    sta::Vertex* worst_vertex{nullptr};
    sta::Slack end_slack{0.0};
    sta::Slack worst_slack{0.0};
    sta::Slack prev_end_slack{0.0};
    sta::Slack prev_worst_slack{0.0};
    int pass{1};
    int decreasing_slack_passes{0};
    bool journal_open{false};
  };

  // Mutable progress for the last-gasp phase (Phase 3) executed after the
  // main repair loop has converged or hit its iteration limit.
  struct LastGaspState
  {
    int end_index{0};
    int max_end_count{0};
    int num_viols{0};
    int opto_iteration{0};
    float initial_tns{0.0f};
    float prev_tns{0.0f};
    float fix_rate_threshold{0.0f};
    sta::Slack prev_worst_slack{0.0};
    bool prev_termination{false};
    bool two_cons_terminations{false};
    char phase_marker{'+'};
  };

  // === Run setup ============================================================
  virtual void init();
  virtual bool runSetup();
  virtual void initializeSetupServices();
  virtual void resetMovedBufferFlag();
  virtual bool hasVtSwapCells() const;

  // === Move-sequence configuration =========================================
  virtual void buildMainMoveSequence();
  void buildLastGaspMoveSequence(const RepairSetupParams& params);
  virtual void activateMoveSequence(bool log_sequence);
  void pushMoveIfEnabled(bool enabled, MoveType type);
  void logMoveSequence() const;
  RepairSetupParams makeRepairSetupParams(float setup_slack_margin) const;
  ViolatingEnds collectViolatingEndpoints(float setup_slack_margin) const;

  // === Main repair loop =====================================================
  bool initializeMainRepair(float setup_slack_margin,
                            double repair_tns_end_percent,
                            MainRepairState& main_state,
                            ViolatingEnds& violating_ends);
  void runMainRepairLoop(const ViolatingEnds& violating_ends,
                         float setup_slack_margin,
                         int max_passes,
                         int max_iterations,
                         bool verbose,
                         MainRepairState& main_state);
  bool beginJournaledEndpointSearch(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      int max_end_count,
      int& end_index,
      EndpointRepairState& endpoint_state);
  virtual bool beginEndpointRepair(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      MainRepairState& main_state,
      EndpointRepairState& endpoint_state);
  virtual void repairEndpoint(EndpointRepairState& endpoint_state,
                              MainRepairState& main_state,
                              float setup_slack_margin,
                              int max_passes,
                              int max_iterations,
                              bool verbose);
  virtual bool shouldStopEndpointRepair(EndpointRepairState& endpoint_state,
                                        float setup_slack_margin);
  void recordTermination(bool& prev_termination, bool& two_cons_terminations);
  void acceptEndpointState(EndpointRepairState& endpoint_state);
  void restoreEndpointState(EndpointRepairState& endpoint_state);
  void finishEndpointSearch(EndpointRepairState& endpoint_state);
  void saveImprovedCheckpoint(EndpointRepairState& endpoint_state,
                              int max_passes);
  void refreshEndpointSlacks(EndpointRepairState& endpoint_state);

  // === Last-gasp repair loop ===============================================
  bool initializeLastGaspRepair(const RepairSetupParams& params,
                                int opto_iteration,
                                float initial_tns,
                                LastGaspState& last_gasp_state,
                                ViolatingEnds& violating_ends);
  void runLastGaspLoop(const ViolatingEnds& violating_ends,
                       const RepairSetupParams& params,
                       int max_iterations,
                       LastGaspState& last_gasp_state);
  bool beginLastGaspEndpoint(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      LastGaspState& last_gasp_state,
      EndpointRepairState& endpoint_state);
  void repairLastGaspEndpoint(EndpointRepairState& endpoint_state,
                              LastGaspState& last_gasp_state,
                              const RepairSetupParams& params,
                              int max_iterations);
  bool advanceLastGaspProgress(EndpointRepairState& endpoint_state,
                               LastGaspState& last_gasp_state,
                               const RepairSetupParams& params,
                               float curr_tns);
  bool shouldStopMainRepair(const MainRepairState& main_state) const;
  bool shouldStopLastGasp(const LastGaspState& last_gasp_state,
                          int max_iterations) const;
  bool reachedIterationLimit(int iteration, int max_iterations) const;

  // === Progress and improvement checks =====================================
  bool pathImproved(int end_index,
                    sta::Slack end_slack,
                    sta::Slack worst_slack,
                    sta::Slack prev_end_slack,
                    sta::Slack prev_worst_slack) const;
  bool lastGaspImproved(sta::Slack worst_slack,
                        float curr_tns,
                        sta::Slack prev_worst_slack,
                        float prev_tns) const;
  bool reportRepairSummary(float setup_slack_margin);

  // === Target construction and path repair =================================
  virtual bool repairPath(sta::Path* path, sta::Slack path_slack);
  virtual bool repairPins(
      const std::vector<const sta::Pin*>& pins,
      const sta::Path* focus_path,
      const std::unordered_map<const sta::Pin*, std::unordered_set<MoveType>>*
          rejected_moves,
      std::vector<std::pair<const sta::Pin*, MoveType>>* chosen_moves);
  std::vector<std::pair<int, sta::Delay>> rankPathDrivers(
      sta::PathExpanded& expanded,
      const sta::Scene* corner,
      int lib_ap) const;
  int repairBudget(sta::Slack path_slack) const;
  bool makePinTargetOnPath(const sta::Pin* pin,
                           const sta::Path* path,
                           sta::Slack focus_slack,
                           Target& target) const;
  bool makePinTarget(const sta::Pin* pin,
                     sta::Slack focus_slack,
                     Target& target) const;
  virtual void makePathDriverTarget(const sta::Path* path,
                                    sta::PathExpanded& expanded,
                                    int drvr_index,
                                    sta::Slack path_slack,
                                    Target& target) const;
  virtual void logRepairTarget(const Target& target) const;
  static int repairProgressIncrement(MoveType type, int repairs_per_pass);
  static bool allowsBatchRepair(MoveType type);
  bool tryRepairTarget(const Target& target,
                       int repairs_per_pass,
                       int& changed,
                       const std::unordered_set<MoveType>* rejected_types);
  virtual bool tryRepairTarget(
      const Target& target,
      int repairs_per_pass,
      int& changed,
      const std::unordered_set<MoveType>* rejected_types,
      std::optional<MoveType>& accepted_type);
  bool tryRepairPathTarget(const Target& target,
                           sta::Slack path_slack,
                           int repairs_per_pass,
                           int& changed);
  bool trySizeDownBatch(MoveGenerator& generator,
                        const Target& target,
                        int repairs_per_pass,
                        int& changed,
                        std::optional<MoveType>& accepted_type);
  bool tryCandidateSequence(MoveGenerator& generator,
                            const Target& target,
                            int repairs_per_pass,
                            int& changed,
                            std::optional<MoveType>& accepted_type);
  int fanout(sta::Vertex* vertex) const;

  // === Progress reporting ===================================================
  void printProgress(int iteration, bool force, bool end, bool last_gasp) const;
  void printProgress(int iteration,
                     bool force,
                     bool end,
                     char phase_marker,
                     bool use_startpoint_metrics = false) const;
  bool terminateProgress(int iteration,
                         float initial_tns,
                         float& prev_tns,
                         float& fix_rate_threshold,
                         int endpt_index,
                         int num_endpts,
                         std::string_view phase_name,
                         char phase_marker);
  void reportCustomPhaseSetup() const;

  // === Custom repair phases =================================================
  void repairSetupWns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      int& opto_iteration,
                      float initial_tns,
                      float& prev_tns,
                      bool use_cone_collection,
                      char phase_marker,
                      rsz::ViolatorSortType sort_type);
  void repairSetupTns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      int& opto_iteration,
                      char phase_marker,
                      rsz::ViolatorSortType sort_type);
  void repairSetupEndpointFanin(float setup_slack_margin,
                                int max_passes_per_endpoint,
                                bool verbose,
                                int& opto_iteration,
                                char phase_marker);
  void repairSetupStartpointFanout(float setup_slack_margin,
                                   int max_passes_per_startpoint,
                                   bool verbose,
                                   int& opto_iteration,
                                   char phase_marker);
  void repairSetupDirectional(bool use_startpoints,
                              float setup_slack_margin,
                              int max_passes_per_point,
                              bool verbose,
                              int& opto_iteration,
                              char phase_marker);
  void repairSetupLastGasp(const RepairSetupParams& params,
                           int& num_viols,
                           int max_iterations,
                           int opto_iteration,
                           float initial_tns,
                           char phase_marker);

  // === Critical-cell VT sweep ==============================================
  bool swapVTCritCells(const RepairSetupParams& params, int& num_viols);
  sta::Pin* worstOutputPin(sta::Instance* inst);
  int committedMoves(MoveType type) const;
  int totalMoves(MoveType type) const;
  void traverseFaninCone(sta::Vertex* endpoint,
                         std::unordered_map<sta::Instance*, float>& crit_insts,
                         std::unordered_set<sta::Vertex*>& visited,
                         std::unordered_set<sta::Instance*>& notSwappable,
                         const RepairSetupParams& params);
  sta::Slack getInstanceSlack(sta::Instance* inst);

  // === Repair services ======================================================
  std::unique_ptr<rsz::RepairTargetCollector> target_collector_;

  // === Repair progress state ===============================================
  bool fallback_ = false;
  float min_viol_ = 0.0;
  float max_viol_ = 0.0;
  int max_repairs_per_pass_ = 1;
  int max_end_repairs_ = 1;
  int overall_no_progress_count_ = 0;
  double initial_design_area_ = 0.0;

  // === Rejection state ======================================================
  std::unordered_map<const sta::Pin*, std::unordered_set<MoveType>>
      rejected_pin_moves_current_endpoint_;
  // === Legacy repair constants =============================================
  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int initial_decreasing_slack_max_passes_ = 6;
  static constexpr int pass_limit_increment_ = 5;
  static constexpr int print_interval_ = 10;
  static constexpr int opto_small_interval_ = 100;
  static constexpr int opto_large_interval_ = 1000;
  static constexpr float inc_fix_rate_threshold_ = 0.0001;
  static constexpr int max_last_gasp_passes_ = 10;
};

}  // namespace rsz
