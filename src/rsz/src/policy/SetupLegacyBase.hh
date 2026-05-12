// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <map>
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
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
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

// Shared single-threaded setup-repair implementation used by the legacy phase
// policies.  It owns the legacy move sequence, target construction, endpoint
// journal handling, progress reporting, and repair_setup_pin flow.
class SetupLegacyBase : public OptimizationPolicy
{
 public:
  // === OptimizationPolicy entry points
  // ==============================================
  SetupLegacyBase(Resizer& resizer,
                  MoveCommitter& committer,
                  RepairSetupContext& setup_context,
                  const OptimizerRunConfig& config);

  const char* name() const override { return "SetupLegacyBase"; }
  bool start() override;
  void iterate() override;

  // === Single-endpoint repair API ==========================================
  bool repairSetupPin(const sta::Pin* end_pin);

 protected:
  using ViolatingEnds = std::vector<std::pair<sta::Vertex*, sta::Slack>>;

  // === Phase state ==========================================================

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
    bool force_single_repair{false};
  };

  // === Run setup ============================================================
  virtual void initializeSetupServices();
  virtual bool hasVtSwapCells() const;
  // Prep work that all legacy-derived phase tokens depend on.  Called once per
  // Optimizer::run by the first legacy-derived policy start().
  bool prepareForPhasePipeline();

  // === Move-sequence configuration =========================================
  virtual void buildMainMoveSequence(bool log_sequence);
  virtual void activateMoveSequence(bool log_sequence);
  void pushMoveIfEnabled(bool enabled, MoveType type);
  void logMoveSequence() const;
  ViolatingEnds collectViolatingEndpoints(float setup_slack_margin) const;

  // === Endpoint journal helpers ============================================
  bool beginJournaledEndpointSearch(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      int max_end_count,
      int& end_index,
      EndpointRepairState& endpoint_state);
  void recordTermination(bool& prev_termination, bool& two_cons_terminations);
  void acceptEndpointState(EndpointRepairState& endpoint_state);
  void restoreEndpointState(EndpointRepairState& endpoint_state);
  void finishEndpointSearch(EndpointRepairState& endpoint_state);
  void saveImprovedCheckpoint(EndpointRepairState& endpoint_state);
  void refreshEndpointSlacks(EndpointRepairState& endpoint_state);

  bool reachedIterationLimit(int iteration, int max_iterations) const;

  // === Target construction and path repair =================================
  virtual bool repairPath(sta::Path* path,
                          sta::Slack path_slack,
                          bool force_single_repair);
  virtual bool repairPins(
      const std::vector<const sta::Pin*>& pins,
      const sta::Path* focus_path,
      const std::unordered_map<const sta::Pin*, std::unordered_set<MoveType>>*
          rejected_moves,
      std::vector<std::pair<const sta::Pin*, MoveType>>* chosen_moves,
      bool force_single_repair);
  std::vector<std::pair<int, sta::Delay>> rankPathDrivers(
      sta::PathExpanded& expanded,
      const sta::Scene* corner,
      int lib_ap) const;
  int repairBudget(sta::Slack path_slack, bool force_single_repair) const;
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
  void printProgress(int iteration,
                     bool force,
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

  int committedMoves(MoveType type) const;
  int totalMoves(MoveType type) const;

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
};

}  // namespace rsz
