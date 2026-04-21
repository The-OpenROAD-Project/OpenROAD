// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveCommitter.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveTracker.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using std::pair;
using std::string;
using std::vector;
using utl::RSZ;

MoveStateType moveTrackerState(const TrackedMoveStateType state)
{
  switch (state) {
    case TrackedMoveStateType::ATTEMPT:
      return MoveStateType::ATTEMPT;
    case TrackedMoveStateType::ATTEMPT_REJECT:
      return MoveStateType::ATTEMPT_REJECT;
    case TrackedMoveStateType::ATTEMPT_COMMIT:
      return MoveStateType::ATTEMPT_COMMIT;
  }
  return MoveStateType::ATTEMPT;
}

}  // namespace

MoveCommitter::MoveCommitter(Resizer& resizer) : resizer_(resizer)
{
}

void MoveCommitter::init()
{
  journal_open_ = false;
  std::ranges::fill(summary_carried_by_type_, 0);
  summary_carried_by_type_[typeIndex(MoveType::kSizeUpMatch)]
      = persisted_summary_by_type_[typeIndex(MoveType::kSizeUpMatch)];
  std::ranges::fill(pending_by_type_, 0);
  std::ranges::fill(committed_by_type_, 0);
  for (size_t index = 0; index < pending_instances_by_type_.size(); ++index) {
    pending_instances_by_type_[index].clear();
    all_instances_by_type_[index].clear();
  }

  const bool report_enabled
      = resizer_.logger()->debugCheck(RSZ, "move_tracker", 1);
  if (report_enabled) {
    move_tracker_ = std::make_unique<MoveTracker>(resizer_, report_enabled);
  } else {
    move_tracker_.reset();
  }
}

bool MoveCommitter::moveTrackerEnabled(const int level) const
{
  return move_tracker_ != nullptr && move_tracker_->reportEnabled()
         && resizer_.logger()->debugCheck(RSZ, "move_tracker", level);
}

MoveResult MoveCommitter::commit(MoveCandidate& candidate)
{
  MoveResult result = candidate.apply();
  if (result.accepted) {
    recordAcceptedResult(result);
  }
  return result;
}

void MoveCommitter::setCurrentEndpoint(const sta::Pin* endpoint_pin)
{
  if (moveTrackerEnabled()) {
    move_tracker_->setCurrentEndpoint(endpoint_pin);
  }
}

void MoveCommitter::trackCriticalPins(
    const std::vector<const sta::Pin*>& critical_pins)
{
  if (moveTrackerEnabled()) {
    move_tracker_->trackCriticalPins(critical_pins);
  }
}

void MoveCommitter::clearTrackedMoves()
{
  if (moveTrackerEnabled()) {
    move_tracker_->clear();
  }
}

void MoveCommitter::trackViolator(const sta::Pin* pin)
{
  if (moveTrackerEnabled()) {
    move_tracker_->trackViolator(pin);
  }
}

void MoveCommitter::trackViolatorWithInfo(const sta::Pin* pin,
                                          const std::string& gate_type,
                                          const float load_delay,
                                          const float intrinsic_delay,
                                          const float pin_slack,
                                          const float endpoint_slack)
{
  if (moveTrackerEnabled()) {
    move_tracker_->trackViolatorWithInfo(
        pin, gate_type, load_delay, intrinsic_delay, pin_slack, endpoint_slack);
  }
}

void MoveCommitter::trackViolatorWithTimingInfo(
    const sta::Pin* pin,
    sta::Vertex* vertex,
    const float endpoint_slack,
    RepairTargetCollector& violator_collector)
{
  if (!moveTrackerEnabled(2)) {
    return;
  }

  sta::LibertyPort* driver_port = resizer_.network()->libertyPort(pin);
  const std::string gate_type
      = driver_port != nullptr ? driver_port->libertyCell()->name() : "unknown";

  float load_delay = 0.0f;
  float intrinsic_delay = 0.0f;
  if (!violator_collector.getPinData(pin, load_delay, intrinsic_delay)) {
    const std::pair<sta::Delay, sta::Delay> effort_delays
        = violator_collector.getEffortDelays(pin);
    load_delay = effort_delays.first;
    intrinsic_delay = effort_delays.second;
  }

  const sta::Slack pin_slack
      = vertex != nullptr
            ? resizer_.sta()->slack(vertex, resizer_.maxAnalysisMode())
            : sta::Slack(0.0f);
  trackViolatorWithInfo(
      pin, gate_type, load_delay, intrinsic_delay, pin_slack, endpoint_slack);
}

void MoveCommitter::trackPreparedViolator(const Target& target)
{
  if (!moveTrackerEnabled()) {
    return;
  }

  if (!target.arc_delay.has_value() || !target.arc_delay->isValid()) {
    return;
  }

  const sta::Pin* endpoint_pin = target.endpointPin(resizer_);
  if (endpoint_pin != nullptr) {
    setCurrentEndpoint(endpoint_pin);
  }

  const ArcDelayState& arc_delay = target.arc_delay.value();
  const sta::LibertyCell* current_cell
      = arc_delay.arc.output_port->libertyCell();
  trackViolatorWithInfo(target.driver_pin,
                        current_cell->name(),
                        arc_delay.current_delay,
                        0.0f,
                        target.slack,
                        target.slack);
}

void MoveCommitter::trackMove(const sta::Pin* pin,
                              const MoveType move_type,
                              const TrackedMoveStateType state)
{
  if (moveTrackerEnabled()) {
    move_tracker_->trackMove(pin, moveName(move_type), moveTrackerState(state));
  }
}

void MoveCommitter::trackMoveAttempt(const sta::Pin* pin,
                                     const MoveType move_type)
{
  if (!moveTrackerEnabled(2)) {
    return;
  }
  trackMove(pin, move_type, TrackedMoveStateType::ATTEMPT);
}

void MoveCommitter::trackMoveAttempt(const MoveCandidate& candidate,
                                     const sta::Pin* pin,
                                     const sta::Pin* endpoint_pin)
{
  if (!moveTrackerEnabled(2)) {
    return;
  }
  if (endpoint_pin != nullptr) {
    setCurrentEndpoint(endpoint_pin);
  }
  trackMove(pin, candidate.type(), TrackedMoveStateType::ATTEMPT);
}

void MoveCommitter::commitTrackedMoves()
{
  if (moveTrackerEnabled()) {
    move_tracker_->commitMoves();
  }
}

void MoveCommitter::rejectTrackedMoves()
{
  if (moveTrackerEnabled()) {
    move_tracker_->rejectMoves();
  }
}

void MoveCommitter::printTrackerPhaseSummary(
    const char* move_summary_title,
    const char* endpoint_summary_title,
    const bool include_endpoint_summary)
{
  if (!moveTrackerEnabled()) {
    return;
  }

  if (move_summary_title != nullptr) {
    printMoveSummary(move_summary_title);
  }
  if (include_endpoint_summary && endpoint_summary_title != nullptr) {
    printEndpointSummary(endpoint_summary_title);
  }
  clearTrackedMoves();
}

void MoveCommitter::printTrackerFinalReports(
    const std::vector<const sta::Pin*>& critical_pins)
{
  if (!moveTrackerEnabled()) {
    return;
  }

  trackCriticalPins(critical_pins);
  resizer_.logger()->info(utl::RSZ, 211, "");
  resizer_.logger()->info(
      utl::RSZ, 212, "=== Optimization Analysis Reports ===");
  printSlackDistribution("Pin Slack Distribution");
  printTopBinEndpoints("Most Critical Endpoints After Optimization");
  printCriticalEndpointPathHistogram("Critical Endpoint Path Distribution");
  printSuccessReport("Successful Optimizations Report");
  printFailureReport("Unsuccessful Optimizations Report");
  printMissedOpportunitiesReport("Missed Opportunities Report");
  resizer_.logger()->info(utl::RSZ, 213, "");
}

int MoveCommitter::getVisitCount(const sta::Pin* pin) const
{
  return move_tracker_ != nullptr ? move_tracker_->getVisitCount(pin) : 0;
}

void MoveCommitter::printMoveSummary(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printMoveSummary(title);
  }
}

void MoveCommitter::printEndpointSummary(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printEndpointSummary(title);
  }
}

void MoveCommitter::printSuccessReport(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printSuccessReport(title);
  }
}

void MoveCommitter::printFailureReport(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printFailureReport(title);
  }
}

void MoveCommitter::printMissedOpportunitiesReport(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printMissedOpportunitiesReport(title);
  }
}

void MoveCommitter::captureOriginalEndpointSlack()
{
  if (moveTrackerEnabled()) {
    move_tracker_->captureOriginalEndpointSlack();
  }
}

void MoveCommitter::capturePrePhaseSlack()
{
  if (moveTrackerEnabled()) {
    move_tracker_->capturePrePhaseSlack();
  }
}

void MoveCommitter::captureInitialSlackDistribution()
{
  if (moveTrackerEnabled()) {
    move_tracker_->captureInitialSlackDistribution();
  }
}

void MoveCommitter::printSlackDistribution(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printSlackDistribution(title);
  }
}

void MoveCommitter::printTopBinEndpoints(const std::string& title,
                                         const int max_endpoints)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printTopBinEndpoints(title, max_endpoints);
  }
}

void MoveCommitter::printCriticalEndpointPathHistogram(const std::string& title)
{
  if (moveTrackerEnabled()) {
    move_tracker_->printCriticalEndpointPathHistogram(title);
  }
}

bool MoveCommitter::canManageJournal() const
{
  return resizer_.block() != nullptr;
}

void MoveCommitter::recordAcceptedResult(const MoveResult& result)
{
  // Track committed instances by move type so later passes can avoid
  // conflicts.
  const size_t index = typeIndex(result.type);
  pending_by_type_[index] += result.move_count;
  for (sta::Instance* inst : result.touched_instances) {
    pending_instances_by_type_[index].insert(inst);
    // Preserve legacy BaseMove semantics: once a move type touched an
    // instance in this repair session, later guards still see it even if the
    // journal branch is restored.
    all_instances_by_type_[index].insert(inst);
  }
}

bool MoveCommitter::ecoHasPendingChanges() const
{
  return !odb::dbDatabase::ecoEmpty(resizer_.block());
}

void MoveCommitter::beginEcoJournal() const
{
  debugPrint(resizer_.logger(), RSZ, "journal", 1, "journal begin");
  odb::dbDatabase::beginEco(resizer_.block());
}

void MoveCommitter::commitEcoJournal() const
{
  debugPrint(resizer_.logger(), RSZ, "journal", 1, "journal end");
  // Refresh timing only once after a batch of ECO edits becomes permanent.
  if (ecoHasPendingChanges()) {
    resizer_.updateParasiticsAndTiming();
  }
  odb::dbDatabase::commitEco(resizer_.block());
}

void MoveCommitter::restoreEcoJournal() const
{
  odb::dbDatabase::undoEco(resizer_.block());
}

int MoveCommitter::pendingMoveCount() const
{
  int move_count = 0;
  for (int pending_count : pending_by_type_) {
    move_count += pending_count;
  }
  return move_count;
}

void MoveCommitter::logPendingMoves(const char* action,
                                    const int move_count) const
{
  debugPrint(resizer_.logger(),
             RSZ,
             "opt_moves",
             2,
             "{} {} moves: up {} up_match {} down {} buffer {} clone {} swap "
             "{} vt_swap {} unbuf {} split {}",
             action,
             move_count,
             pendingMoves(MoveType::kSizeUp),
             pendingMoves(MoveType::kSizeUpMatch),
             pendingMoves(MoveType::kSizeDown),
             pendingMoves(MoveType::kBuffer),
             pendingMoves(MoveType::kClone),
             pendingMoves(MoveType::kSwapPins),
             pendingMoves(MoveType::kVtSwap),
             pendingMoves(MoveType::kUnbuffer),
             pendingMoves(MoveType::kSplitLoad));
}

void MoveCommitter::logCommittedTotals() const
{
  debugPrint(
      resizer_.logger(),
      RSZ,
      "opt_moves",
      1,
      "TOTAL {} moves (acc {} rej {}): up {} up_match {} down {} buffer "
      "{} clone {} swap {} vt_swap {} unbuf {} split {}",
      resizer_.acceptedLegacyMoveCount() + resizer_.rejectedLegacyMoveCount(),
      resizer_.acceptedLegacyMoveCount(),
      resizer_.rejectedLegacyMoveCount(),
      committedMoves(MoveType::kSizeUp),
      committedMoves(MoveType::kSizeUpMatch),
      committedMoves(MoveType::kSizeDown),
      committedMoves(MoveType::kBuffer),
      committedMoves(MoveType::kClone),
      committedMoves(MoveType::kSwapPins),
      committedMoves(MoveType::kVtSwap),
      committedMoves(MoveType::kUnbuffer),
      committedMoves(MoveType::kSplitLoad));
}

void MoveCommitter::beginJournal()
{
  if (!canManageJournal()) {
    return;
  }

  // Start one reversible ECO batch for the current search branch.
  beginEcoJournal();
  journal_open_ = true;
}

void MoveCommitter::commitJournal()
{
  if (!canManageJournal() || !journal_open_) {
    return;
  }

  // Close the reversible batch and fold the accepted moves into the totals.
  commitEcoJournal();
  acceptPendingMoves();
  journal_open_ = false;
}

void MoveCommitter::restoreJournal()
{
  if (!canManageJournal() || !journal_open_) {
    return;
  }

  // Undo both database edits and in-memory accounting for the rejected branch.
  debugPrint(
      resizer_.logger(), RSZ, "journal", 1, "journal restore starts >>>");
  resizer_.initForJournalRestore();

  const bool had_eco_changes = ecoHasPendingChanges();
  restoreEcoJournal();
  if (!had_eco_changes) {
    rejectPendingMoves();
    journal_open_ = false;
    debugPrint(resizer_.logger(),
               RSZ,
               "journal",
               1,
               "journal restore ends due to empty ECO >>>");
    return;
  }

  resizer_.updateParasiticsAndTiming();
  rejectPendingMoves();
  journal_open_ = false;

  debugPrint(resizer_.logger(), RSZ, "journal", 1, "journal restore ends <<<");
}

void MoveCommitter::acceptPendingMoves()
{
  const int move_count = pendingMoveCount();
  logPendingMoves("COMMIT", move_count);
  resizer_.addAcceptedLegacyMoveCount(move_count);

  for (size_t index = 0; index < pending_by_type_.size(); ++index) {
    if (index == typeIndex(MoveType::kSizeUpMatch)) {
      persisted_summary_by_type_[index] += pending_by_type_[index];
    }
    committed_by_type_[index] += pending_by_type_[index];
    pending_by_type_[index] = 0;
    all_instances_by_type_[index].insert(
        pending_instances_by_type_[index].begin(),
        pending_instances_by_type_[index].end());
    pending_instances_by_type_[index].clear();
  }

  commitTrackedMoves();
  logCommittedTotals();
}

void MoveCommitter::rejectPendingMoves()
{
  const int move_count = pendingMoveCount();
  logPendingMoves("UNDO", move_count);
  resizer_.addRejectedLegacyMoveCount(move_count);

  std::ranges::fill(pending_by_type_, 0);
  for (std::unordered_set<sta::Instance*>& pending_instances :
       pending_instances_by_type_) {
    pending_instances.clear();
  }

  rejectTrackedMoves();
  logCommittedTotals();
}

int MoveCommitter::pendingMoves(const MoveType type) const
{
  return pending_by_type_[typeIndex(type)];
}

int MoveCommitter::committedMoves(const MoveType type) const
{
  return committed_by_type_[typeIndex(type)];
}

int MoveCommitter::summaryCommittedMoves(const MoveType type) const
{
  const size_t index = typeIndex(type);
  return summary_carried_by_type_[index] + committed_by_type_[index];
}

int MoveCommitter::totalMoves(const MoveType type) const
{
  const size_t index = typeIndex(type);
  return summary_carried_by_type_[index] + committed_by_type_[index]
         + pending_by_type_[index];
}

bool MoveCommitter::hasPendingMoves(const MoveType type,
                                    sta::Instance* inst) const
{
  return pending_instances_by_type_[typeIndex(type)].contains(inst);
}

bool MoveCommitter::hasMoves(const MoveType type, sta::Instance* inst) const
{
  const size_t index = typeIndex(type);
  return all_instances_by_type_[index].contains(inst)
         || pending_instances_by_type_[index].contains(inst);
}

bool MoveCommitter::hasBlockingBufferRemovalMove(sta::Instance* inst,
                                                 std::string& reason) const
{
  reason.clear();
  if (hasMoves(MoveType::kSwapPins, inst)) {
    reason = "its pins have been swapped";
  } else if (hasMoves(MoveType::kClone, inst)) {
    reason = "it has been cloned";
  } else if (hasMoves(MoveType::kSplitLoad, inst)) {
    reason = "it was from split load buffering";
  } else if (hasMoves(MoveType::kBuffer, inst)) {
    reason = "it was from rebuffering";
  } else if (hasMoves(MoveType::kSizeUp, inst)) {
    reason = "it has been resized";
  }
  return !reason.empty();
}

}  // namespace rsz
