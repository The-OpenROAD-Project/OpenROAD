// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "BatchSelector.hh"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rsz {

namespace {

// True when the two lists share an element.
template <typename T>
bool intersects(const std::vector<T>& lhs, const std::vector<T>& rhs)
{
  return std::ranges::any_of(lhs, [&rhs](const T& value) {
    return std::ranges::find(rhs, value) != rhs.end();
  });
}

}  // namespace

const char* batchConflictReasonName(const BatchConflictReason reason)
{
  switch (reason) {
    case BatchConflictReason::kIncompleteImpact:
      return "incomplete_impact";
    case BatchConflictReason::kWriteInstance:
      return "write_instance";
    case BatchConflictReason::kWriteNet:
      return "write_net";
    case BatchConflictReason::kReconnectPin:
      return "reconnect_pin";
    case BatchConflictReason::kWriteReconnectInstance:
      return "write_reconnect_instance";
    case BatchConflictReason::kForwardDependency:
      return "forward_dependency";
    case BatchConflictReason::kCount:
      break;
  }

  return "unknown";
}

BatchSelector::BatchSelector(BatchSelectorConfig config)
    : config_(std::move(config))
{
}

BatchSelection BatchSelector::select(
    const std::vector<BatchCandidate>& candidates,
    const int batch_cap,
    const bool reserve_worst) const
{
  std::vector<const BatchCandidate*> ranked;
  ranked.reserve(candidates.size());
  for (const BatchCandidate& candidate : candidates) {
    ranked.push_back(&candidate);
  }

  std::ranges::stable_sort(
      ranked, [](const BatchCandidate* lhs, const BatchCandidate* rhs) {
        return lhs->priority_score > rhs->priority_score;
      });

  Admission admission;
  admission.impacts.reserve(ranked.size());
  fillWorstReserve(
      ranked, worstReserve(ranked.size(), batch_cap, reserve_worst), admission);
  fillByScore(ranked, batch_cap, admission);
  if (admission.selection.stats.wns_reserved > 0) {
    orderByScore(candidates, admission.selection);
  }

  return admission.selection;
}

bool BatchSelector::admit(const BatchCandidate& candidate,
                          Admission& admission) const
{
  BatchSelection& selection = admission.selection;
  ++selection.stats.considered;
  if (!candidate.impact.complete) {
    ++selection.stats.deferred[static_cast<size_t>(
        BatchConflictReason::kIncompleteImpact)];
    return false;
  }

  std::optional<BatchConflictReason> conflict;
  for (const MoveImpact* selected_impact : admission.impacts) {
    const BatchConflictReason reason
        = hardConflict(*selected_impact, candidate.impact);
    if (reason != BatchConflictReason::kCount) {
      conflict = reason;
      break;
    }
  }

  // The check is one way.  A member whose estimate read timing this candidate
  // changes does not stop it, though that estimate goes stale too: every
  // estimate reads the pre-batch timing, and the batch verdict and the salvage
  // catch a batch that misses.  A two-way check would defer more candidates.
  if (!conflict.has_value()) {
    for (const MoveImpact* selected_impact : admission.impacts) {
      if (dependsOn(candidate.impact, *selected_impact)) {
        conflict = BatchConflictReason::kForwardDependency;
        break;
      }
    }
  }

  if (conflict.has_value()) {
    ++selection.stats.deferred[static_cast<size_t>(*conflict)];
    return false;
  }

  selection.evaluation_indices.push_back(candidate.evaluation_index);
  admission.impacts.push_back(&candidate.impact);
  admission.taken.insert(&candidate);

  return true;
}

int BatchSelector::worstReserve(const size_t candidate_count,
                                const int batch_cap,
                                const bool reserve_worst) const
{
  // The score order favors TNS: with many winners per batch, the small-gain
  // moves on the worst path never get in and WNS stalls.  The reserve matters
  // only when the batch is oversubscribed.
  const bool oversubscribed = static_cast<int>(candidate_count) > batch_cap;
  if (!reserve_worst || config_.wns_reserve_pct <= 0 || !oversubscribed) {
    return 0;
  }

  return std::clamp(batch_cap * config_.wns_reserve_pct / 100, 1, batch_cap);
}

void BatchSelector::fillWorstReserve(
    const std::vector<const BatchCandidate*>& ranked,
    const int reserve,
    Admission& admission) const
{
  if (reserve <= 0) {
    return;
  }

  std::vector<const BatchCandidate*> critical = ranked;
  std::ranges::stable_sort(
      critical, [](const BatchCandidate* lhs, const BatchCandidate* rhs) {
        return lhs->endpoint_slack < rhs->endpoint_slack;
      });
  BatchSelectionStats& stats = admission.selection.stats;
  for (const BatchCandidate* candidate : critical) {
    if (stats.wns_reserved >= reserve
        || candidate->endpoint_slack >= config_.slack_margin) {
      break;
    }
    if (admit(*candidate, admission)) {
      ++stats.wns_reserved;
    }
  }
}

void BatchSelector::fillByScore(
    const std::vector<const BatchCandidate*>& ranked,
    const int batch_cap,
    Admission& admission) const
{
  float best_local_score = 0.0f;
  bool local_score_ordered = true;
  for (size_t index = 0; index < ranked.size(); ++index) {
    best_local_score = std::max(best_local_score, ranked[index]->local_score);
    if (index > 0
        && ranked[index]->local_score > ranked[index - 1]->local_score) {
      local_score_ordered = false;
    }
  }

  BatchSelection& selection = admission.selection;
  for (size_t ranked_index = 0; ranked_index < ranked.size(); ++ranked_index) {
    if (static_cast<int>(selection.evaluation_indices.size()) >= batch_cap) {
      selection.stats.batch_limited
          = static_cast<int>(ranked.size() - ranked_index);
      break;
    }

    if (admission.taken.contains(ranked[ranked_index])) {
      continue;
    }

    if (config_.min_score_ratio > 0.0f && best_local_score > 0.0f
        && static_cast<int>(selection.evaluation_indices.size())
               >= config_.score_floor_start
        && ranked[ranked_index]->local_score
               < best_local_score * config_.min_score_ratio) {
      if (local_score_ordered) {
        selection.stats.score_limited
            = static_cast<int>(ranked.size() - ranked_index);
        break;
      }

      ++selection.stats.score_limited;
      continue;
    }

    admit(*ranked[ranked_index], admission);
  }
}

void BatchSelector::orderByScore(const std::vector<BatchCandidate>& candidates,
                                 BatchSelection& selection)
{
  std::unordered_map<size_t, float> score_of;
  score_of.reserve(candidates.size());
  for (const BatchCandidate& candidate : candidates) {
    score_of[candidate.evaluation_index] = candidate.priority_score;
  }

  std::ranges::stable_sort(selection.evaluation_indices,
                           [&score_of](const size_t lhs, const size_t rhs) {
                             return score_of[lhs] > score_of[rhs];
                           });
}

BatchConflictReason BatchSelector::hardConflict(const MoveImpact& selected,
                                                const MoveImpact& candidate)
{
  if (intersects(selected.write_instances, candidate.write_instances)) {
    return BatchConflictReason::kWriteInstance;
  }

  if (intersects(selected.write_nets, candidate.write_nets)) {
    return BatchConflictReason::kWriteNet;
  }

  if (intersects(selected.reconnect_pins, candidate.reconnect_pins)) {
    return BatchConflictReason::kReconnectPin;
  }

  if (intersects(selected.write_instances, candidate.reconnect_instances)
      || intersects(candidate.write_instances, selected.reconnect_instances)) {
    return BatchConflictReason::kWriteReconnectInstance;
  }

  return BatchConflictReason::kCount;
}

bool BatchSelector::dependsOn(const MoveImpact& candidate,
                              const MoveImpact& selected)
{
  return intersects(selected.timing_effect_instances,
                    candidate.timing_dependency_instances)
         || intersects(selected.timing_effect_nets,
                       candidate.timing_dependency_nets);
}

}  // namespace rsz
