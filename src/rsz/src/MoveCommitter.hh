// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveTracker.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Instance;
class PathEnd;
class Pin;
class Vertex;
}  // namespace sta

namespace rsz {
class MoveCandidate;
class RepairTargetCollector;

enum class TrackedMoveStateType
{
  ATTEMPT = 0,
  ATTEMPT_REJECT = 1,
  ATTEMPT_COMMIT = 2
};

// Central commit/accounting owner for one repair_setup run.
//
// Everything in the optimizer that needs to change OpenDB, count a move, or
// report through the tracker goes through this class.  MoveCommitter delegates
// reporting details to MoveTracker while keeping ownership of ECO journal and
// move accounting.
// Owning the ECO journal here (instead of spreading beginJournal()/
// restoreJournal() calls across policies) guarantees that pending move
// counters and the OpenDB journal can never drift: both advance together
// inside commit() and reset together in rejectPendingMoves()/
// restoreJournal().
//
// Collaborators:
//   - Resizer        : backing OpenDB/STA handle.
//   - MoveTracker    : rich per-pin reporting for the RSZ move_tracker debug
//                      channel.
//   - MoveCandidate  : the commit() entry point calls candidate.apply()
//                      between beginEcoJournal() and commit/restore.
//
// Threading: MoveCommitter is single-threaded.  MT policies must merge
// worker-thread results back to the main thread before calling any track*() or
// commit() method.
class MoveCommitter
{
 public:
  // === Run lifecycle ========================================================
  explicit MoveCommitter(Resizer& resizer);
  ~MoveCommitter() = default;

  void init();
  MoveResult commit(MoveCandidate& candidate);

  // === MoveTracker reporting ===============================================
  // Tracker-style reporting is enabled by the existing RSZ move_tracker debug
  // channel so MoveCommitter can replace the old tracker without changing
  // user controls.
  bool moveTrackerEnabled(int level = 1) const;
  void setCurrentEndpoint(const sta::Pin* endpoint_pin);
  void trackCriticalPins(const std::vector<const sta::Pin*>& critical_pins);
  void trackViolator(const sta::Pin* pin);
  void trackViolatorWithInfo(const sta::Pin* pin,
                             const std::string& gate_type,
                             float load_delay,
                             float intrinsic_delay,
                             float pin_slack,
                             float endpoint_slack);
  void trackViolatorWithTimingInfo(const sta::Pin* pin,
                                   sta::Vertex* vertex,
                                   float endpoint_slack,
                                   RepairTargetCollector& violator_collector);
  void trackPreparedViolator(const Target& target);
  void trackMove(const sta::Pin* pin,
                 MoveType move_type,
                 TrackedMoveStateType state);
  void trackMoveAttempt(const sta::Pin* pin, MoveType move_type);
  void trackMoveAttempt(const MoveCandidate& candidate,
                        const sta::Pin* pin,
                        const sta::Pin* endpoint_pin);
  void commitTrackedMoves();
  void rejectTrackedMoves();
  void printTrackerPhaseSummary(const char* move_summary_title,
                                const char* endpoint_summary_title,
                                bool include_endpoint_summary);
  void printTrackerFinalReports(
      const std::vector<const sta::Pin*>& critical_pins);
  void printMoveSummary(const std::string& title);
  void printEndpointSummary(const std::string& title);
  void printSuccessReport(const std::string& title);
  void printFailureReport(const std::string& title);
  void printMissedOpportunitiesReport(const std::string& title);
  void printSlackDistribution(const std::string& title);
  void printTopBinEndpoints(const std::string& title, int max_endpoints = 20);
  void printCriticalEndpointPathHistogram(const std::string& title);
  void captureInitialSlackDistribution();
  void captureOriginalEndpointSlack();
  void capturePrePhaseSlack();
  int getVisitCount(const sta::Pin* pin) const;
  void clearTrackedMoves();

  // === ECO journal lifecycle ===============================================
  // ECO journal lifecycle for setup-repair endpoint loops.
  void beginJournal();
  void commitJournal();
  void restoreJournal();

  // === Move accounting ======================================================
  // In-memory accounting for native optimizer moves that were tentatively
  // applied.
  void acceptPendingMoves();
  void rejectPendingMoves();

  int pendingMoves(MoveType type) const;
  int committedMoves(MoveType type) const;
  int summaryCommittedMoves(MoveType type) const;
  int totalMoves(MoveType type) const;
  bool hasPendingMoves(MoveType type, sta::Instance* inst) const;
  bool hasMoves(MoveType type, sta::Instance* inst) const;
  bool hasBlockingBufferRemovalMove(sta::Instance* inst,
                                    std::string& reason) const;

 private:
  // === Commit result handling ==============================================
  bool canManageJournal() const;
  void recordAcceptedResult(const MoveResult& result);
  int pendingMoveCount() const;

  // === ECO journal internals ===============================================
  bool ecoHasPendingChanges() const;
  void beginEcoJournal() const;
  void commitEcoJournal() const;
  void restoreEcoJournal() const;
  void logPendingMoves(const char* action, int move_count) const;
  void logCommittedTotals() const;

  static size_t typeIndex(MoveType type) { return static_cast<size_t>(type); }

  static constexpr size_t kTypeCount = static_cast<size_t>(MoveType::kCount);
  inline static std::array<int, kTypeCount> persisted_summary_by_type_{};

  // === Shared services ======================================================
  Resizer& resizer_;

  // === Journal and move accounting state ===================================
  bool journal_open_{false};
  std::array<int, kTypeCount> pending_by_type_{};
  std::array<int, kTypeCount> committed_by_type_{};
  std::array<int, kTypeCount> summary_carried_by_type_{};
  std::array<std::unordered_set<sta::Instance*>, kTypeCount>
      pending_instances_by_type_{};
  std::array<std::unordered_set<sta::Instance*>, kTypeCount>
      all_instances_by_type_{};

  // === MoveTracker state ====================================================
  std::unique_ptr<MoveTracker> move_tracker_;
};

}  // namespace rsz
