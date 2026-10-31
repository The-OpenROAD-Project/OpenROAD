// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "ParallelEvalState.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "OptimizationPolicyConfig.hh"
#include "RepairSetupProfiler.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

std::vector<sta::Instance*> keptInstances(
    const sta::dbNetwork* network,
    const std::vector<InstanceRecord>& records)
{
  std::vector<sta::Instance*> kept;
  for (const InstanceRecord& record : records) {
    odb::dbInst* db_inst = network->block()->findInst(record.name.c_str());
    if (db_inst != nullptr && network->dbToSta(db_inst) == record.inst) {
      kept.push_back(record.inst);
    }
  }
  return kept;
}

// === WorkMeter ==============================================================

void WorkMeter::reset(const size_t graph_vertices)
{
  *this = WorkMeter();
  vertices_ = static_cast<double>(graph_vertices);
}

void WorkMeter::addTimingUpdate(const size_t pending_nets)
{
  ++timing_updates_;
  pending_nets_ += pending_nets;
  RepairSetupProfiler::event("work_timing_updates");
  RepairSetupProfiler::event("work_pending_nets", pending_nets);
}

void WorkMeter::addEstimates(const size_t estimates)
{
  estimates_ += estimates;
  RepairSetupProfiler::event("work_estimates", estimates);
}

void WorkMeter::addPlanRequest()
{
  ++plan_requests_;
  RepairSetupProfiler::event("work_plan_requests");
}

double WorkMeter::seconds(const ParallelEvalPolicyConfig& config) const
{
  return config.work_sta_per_vertex * vertices_
             * static_cast<double>(timing_updates_)
         + config.work_per_pending_net * static_cast<double>(pending_nets_)
         + config.work_per_estimate * static_cast<double>(estimates_)
         + config.work_per_plan * static_cast<double>(plan_requests_)
         + config.work_per_endpoint * static_cast<double>(endpoints_);
}

// === StallMonitor ===========================================================

void StallMonitor::reset(const sta::Slack initial_tns,
                         const float reference_period)
{
  initial_tns_ = initial_tns;
  reference_period_ = reference_period;
  stalled_batches_ = 0;
  stalled_moves_ = 0;
  last_batch_moves_ = 0;
  wns_stall_batches_ = 0;
  window_.clear();
  window_stalled_ = false;
}

void StallMonitor::onMissed()
{
  ++stalled_batches_;
  stalled_moves_ += last_batch_moves_;
  last_batch_moves_ = 0;
}

void StallMonitor::onSaved(const sta::Slack prev_tns,
                           const sta::Slack best_wns,
                           const sta::Slack wns,
                           const sta::Slack tns,
                           const double work)
{
  // Progress must be a visible part of the reference clock period or of the
  // initial TNS; otherwise tiny gains keep paying full timing updates.
  const float tns_step = std::abs(sta::delayAsFloat(initial_tns_))
                         * config_.progress_tns_pct / 100.0f;
  const float wns_step
      = reference_period_ * config_.progress_wns_period_pct / 100.0f;
  const bool wns_progressed
      = sta::delayAsFloat(wns) - sta::delayAsFloat(best_wns) > wns_step;
  const bool tns_progressed
      = sta::delayAsFloat(tns) - sta::delayAsFloat(prev_tns) > tns_step;
  const bool progressed = wns_progressed || tns_progressed;
  updateWindow(wns, tns_progressed, work);

  // Once the batches stop moving WNS, the score order alone will not reach
  // the worst path again, so the next batches reserve slots for it.
  wns_stall_batches_ = wns_progressed ? 0 : wns_stall_batches_ + 1;
  // A salvaged batch without progress counts twice, at its miss and here,
  // so a run of misses ends the batches sooner.
  stalled_batches_ = progressed ? 0 : stalled_batches_ + 1;
  stalled_moves_ = progressed ? 0 : stalled_moves_ + last_batch_moves_;
  last_batch_moves_ = 0;
  RepairSetupProfiler::event("stall_moves_after_save", stalled_moves_);
}

bool StallMonitor::weightsPriority() const
{
  return wns_stall_batches_ > 0
         && wns_stall_batches_
                <= config_.priority.bottleneck_max_wns_stall_batches;
}

StallMonitor::Stall StallMonitor::stall() const
{
  if (stalled_moves_ >= moveBudget()) {
    return Stall::kMoves;
  }
  if (stalled_batches_ >= config_.stall_max_batches) {
    return Stall::kBatches;
  }
  return Stall::kNone;
}

int StallMonitor::moveBudget() const
{
  return config_.stall_batches * std::max(config_.min_batch_cap, last_cap_);
}

void StallMonitor::updateWindow(const sta::Slack wns,
                                const bool tns_progressed,
                                const double work)
{
  const int window = config_.stall_window;
  window_.push_back({.wns = sta::delayAsFloat(wns),
                     .failing_endpoints = failing_endpoints_,
                     .work = work});
  if (static_cast<int>(window_.size()) <= window) {
    return;
  }

  window_.pop_front();
  if (tns_progressed) {
    return;
  }

  // Cheap batches are never cut; the rule is for runs whose timing updates
  // cost real work.
  if (window_.back().work - window_.front().work
      < config_.stall_window_min_work) {
    return;
  }

  // The average gain per saved batch over the window: WNS against the
  // reference clock period, the failing endpoint count against its current
  // value.  The window holds `window` checkpoints, so window - 1 intervals.
  const SavedCheckpoint& first = window_.front();
  const SavedCheckpoint& last = window_.back();
  const float intervals = static_cast<float>(window_.size() - 1);
  const float wns_gain = (last.wns - first.wns) / intervals;
  const float failing_endpoint_gain
      = static_cast<float>(first.failing_endpoints)
        - static_cast<float>(last.failing_endpoints);
  const bool wns_moving
      = wns_gain >= reference_period_ * config_.stall_wns_period_pct / 100.0f;
  const bool failing_endpoints_moving
      = failing_endpoint_gain / intervals
        >= config_.stall_failing_endpoint_pct / 100.0f
               * static_cast<float>(last.failing_endpoints);
  if (!wns_moving && !failing_endpoints_moving) {
    window_stalled_ = true;
    RepairSetupProfiler::event("stall_window_triggered");
  }
}

// === WnsTail ================================================================

void WnsTail::reset()
{
  active_ = false;
  started_ = false;
  batches_ = 0;
  batches_without_gain_ = 0;
  endpoints_ = 0;
  max_batch_ = 0;
  batch_cap_ = 0;
  work_start_ = 0.0;
  budget_work_ = 0.0;
  min_gain_ = 0.0f;
  speculative_batches_ = 0;
  speculative_instances_.clear();
  greedy_stalled_ = false;
}

bool WnsTail::start(const double work, const float reference_period)
{
  if (config_.wns_tail_endpoints <= 0 || started_) {
    return false;
  }

  reset();
  active_ = true;
  started_ = true;
  endpoints_ = config_.wns_tail_endpoints;
  max_batch_ = config_.wns_tail_batch;
  batch_cap_ = 1;
  work_start_ = work;

  // The tail budget is a percentage of the batch stage's work, with a floor.
  budget_work_ = std::max(work_start_ * config_.wns_tail_work_pct / 100.0,
                          static_cast<double>(config_.wns_tail_min_work));
  min_gain_ = reference_period * config_.wns_tail_min_gain_period_pct / 100.0f;
  return true;
}

void WnsTail::afterBatch(const sta::Slack best_wns_before,
                         const sta::Slack best_wns_after,
                         const bool batch_saved)
{
  ++batches_;

  // Only a visible WNS gain counts in the tail, not TNS-only saves or tiny
  // gains.
  const bool tail_progressed
      = sta::delayAsFloat(best_wns_after) - sta::delayAsFloat(best_wns_before)
        > min_gain_;

  // A run of speculative batches is judged when it ends.
  if (speculative_batches_ == 0) {
    batches_without_gain_ = tail_progressed ? 0 : batches_without_gain_ + 1;
  }

  // A saved batch earns a larger next batch; a miss drops back to single
  // moves.
  batch_cap_ = batch_saved ? std::min(max_batch_, batch_cap_ * 2) : 1;
}

WnsTail::Step WnsTail::nextStep(const int committed_moves,
                                const double work,
                                const char*& reason) const
{
  // The first rule that fires is the reason.
  const char* const kNoMoves = "wns_tail_no_committed_moves";
  const char* const kStalled = "wns_tail_stalled";
  const char* const kMaxBatches = "wns_tail_max_batches";
  const char* const kWorkBudget = "wns_tail_work_budget";
  reason = nullptr;
  if (committed_moves == 0) {
    reason = kNoMoves;
  } else if (batches_without_gain_ >= config_.wns_tail_stall) {
    reason = kStalled;
  } else if (batches_ >= config_.wns_tail_max_batches) {
    reason = kMaxBatches;
  } else if (work - work_start_ > budget_work_) {
    reason = kWorkBudget;
  }

  if (reason == nullptr) {
    return Step::kContinue;
  }

  // The work budget (when it is the reason) and the batch limit end the tail;
  // the stage rules below may only widen it.
  if (reason == kWorkBudget || batches_ >= config_.wns_tail_max_batches) {
    return Step::kStop;
  }

  // The greedy stage is stuck at this width: retry the same endpoints with
  // speculation before widening, as the serial loop does with non-improving
  // passes.
  if (!greedy_stalled_ && reason == kStalled) {
    return Step::kSpeculate;
  }

  // Nothing more on these endpoints; widen the tail before giving up.
  if (endpoints_ < config_.wns_tail_max_endpoints) {
    return Step::kWiden;
  }

  return Step::kStop;
}

void WnsTail::widen()
{
  // The caller's journal restore drops an open speculative run.
  speculative_batches_ = 0;
  speculative_instances_.clear();
  endpoints_ = std::min(config_.wns_tail_max_endpoints,
                        endpoints_ * config_.wns_tail_grow);
  max_batch_ *= config_.wns_tail_grow;
  batches_without_gain_ = 0;
  greedy_stalled_ = true;
}

bool WnsTail::maySpeculate() const
{
  // Speculation starts only once a greedy tail stage has stalled.
  return speculative_batches_ < config_.wns_tail_speculate && greedy_stalled_;
}

void WnsTail::keepSpeculative(const std::vector<InstanceRecord>& instances)
{
  ++speculative_batches_;
  speculative_instances_.insert(
      speculative_instances_.end(), instances.begin(), instances.end());
}

std::vector<InstanceRecord> WnsTail::endSpeculation()
{
  speculative_batches_ = 0;
  return std::exchange(speculative_instances_, {});
}

// === MissTabu ===============================================================

void MissTabu::reset(const sta::dbNetwork* network)
{
  network_ = network;
  until_.clear();
  undo_counts_.clear();
  excluded_.clear();
}

void MissTabu::addTabu(sta::Instance* inst, const int now)
{
  until_[inst] = {.name = name(inst), .value = now + config_.miss_tabu_batches};
}

bool MissTabu::recordUndo(sta::Instance* inst, const int now)
{
  addTabu(inst, now);
  // An entry left by a deleted instance at this address starts over.
  if (!hasEntry(undo_counts_, inst)) {
    undo_counts_[inst] = {.name = name(inst), .value = 0};
  }
  if (++undo_counts_[inst].value >= config_.max_undos_per_instance) {
    excluded_[inst] = {.name = name(inst)};
    return true;
  }
  return false;
}

void MissTabu::expire(const int now)
{
  std::erase_if(until_,
                [now](const std::pair<sta::Instance* const, Entry>& entry) {
                  return entry.second.value <= now;
                });
}

bool MissTabu::isExcluded(sta::Instance* inst) const
{
  return hasEntry(excluded_, inst);
}

bool MissTabu::isTabu(sta::Instance* inst) const
{
  return hasEntry(until_, inst);
}

std::string MissTabu::name(sta::Instance* inst) const
{
  return network_->staToDb(inst)->getName();
}

bool MissTabu::hasEntry(
    const std::unordered_map<sta::Instance*, Entry>& entries,
    sta::Instance* inst) const
{
  // Only the live `inst` is read; a stale key is compared, never read.
  if (entries.empty()) {
    return false;
  }
  const std::unordered_map<sta::Instance*, Entry>::const_iterator entry
      = entries.find(inst);
  return entry != entries.end()
         && entry->second.name == network_->staToDb(inst)->getConstName();
}

}  // namespace rsz
