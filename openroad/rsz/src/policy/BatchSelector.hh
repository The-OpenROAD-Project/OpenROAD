// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <unordered_set>
#include <vector>

#include "OptimizationPolicyConfig.hh"
#include "OptimizerTypes.hh"

namespace rsz {

enum class BatchConflictReason
{
  kIncompleteImpact,
  kWriteInstance,
  kWriteNet,
  kReconnectPin,
  kWriteReconnectInstance,
  kForwardDependency,
  kCount
};

const char* batchConflictReasonName(BatchConflictReason reason);

// One scored move and the conflicts it would introduce into a batch.
struct BatchCandidate
{
  // Index of the owning TargetEvaluation in the batch pool.
  size_t evaluation_index{0};
  // Candidate score before cross-target priority weighting.
  float local_score{0.0f};
  // Score used to order candidates across targets.
  float priority_score{0.0f};
  // Slack of the endpoint this target sits on; the reserve ranks on it.
  float endpoint_slack{0.0f};
  MoveImpact impact;
};

// Counts batch admission decisions and conflict deferrals.
struct BatchSelectionStats
{
  int considered{0};
  int batch_limited{0};
  int score_limited{0};
  int wns_reserved{0};
  std::array<int, static_cast<size_t>(BatchConflictReason::kCount)> deferred{};
};

// Holds selected evaluation indices and their admission statistics.
struct BatchSelection
{
  std::vector<size_t> evaluation_indices;
  BatchSelectionStats stats;
};

// Selects a conflict-free batch in score order.  A candidate waits when it
// changes what a batch member changes, or when its timing depends on a batch
// member that its estimate did not see.
class BatchSelector
{
 public:
  explicit BatchSelector(BatchSelectorConfig config);

  // Selects at most batch_cap candidates.  reserve_worst turns on the
  // worst-endpoint reserve; the policy asks for it once WNS stops improving.
  BatchSelection select(const std::vector<BatchCandidate>& candidates,
                        int batch_cap,
                        bool reserve_worst) const;

 private:
  // The batch under construction: the selection, the impacts of its members
  // for the conflict checks and the candidates already admitted.
  struct Admission
  {
    BatchSelection selection;
    std::vector<const MoveImpact*> impacts;
    std::unordered_set<const BatchCandidate*> taken;
  };

  // Takes one candidate when it does not conflict with the batch.
  bool admit(const BatchCandidate& candidate, Admission& admission) const;
  // Slots reserved for the worst endpoints; zero when the reserve is off or
  // every candidate fits anyway.
  int worstReserve(size_t candidate_count,
                   int batch_cap,
                   bool reserve_worst) const;
  // Fills the reserve with the candidates of the most negative endpoints.
  void fillWorstReserve(const std::vector<const BatchCandidate*>& ranked,
                        int reserve,
                        Admission& admission) const;
  // Takes the other candidates in score order, up to the cap and the score
  // floor.
  void fillByScore(const std::vector<const BatchCandidate*>& ranked,
                   int batch_cap,
                   Admission& admission) const;
  // Orders the selection by priority score, so the reserve changes which moves
  // are in the batch, not the order they are applied in.
  static void orderByScore(const std::vector<BatchCandidate>& candidates,
                           BatchSelection& selection);
  // Returns the first write or reconnect conflict between two moves.
  static BatchConflictReason hardConflict(const MoveImpact& selected,
                                          const MoveImpact& candidate);
  // Tests whether the candidate's estimate read timing changed by a member.
  static bool dependsOn(const MoveImpact& candidate,
                        const MoveImpact& selected);

  // Score floor and worst-endpoint reserve used for each selection.
  BatchSelectorConfig config_;
};

}  // namespace rsz
