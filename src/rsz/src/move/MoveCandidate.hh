// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

// Appends a non-null value that the list does not hold yet.
template <typename T>
void appendUnique(std::vector<T>& values, const std::type_identity_t<T> value)
{
  if (value != nullptr && std::ranges::find(values, value) == values.end()) {
    values.push_back(value);
  }
}

// === Move candidate interface ==============================================

// Base class for one concrete ECO proposal (swap this cell to LVT, insert a
// buffer at that pin, etc.).
//
// Lifecycle:
//   constructed by a MoveGenerator -> estimate() scores the move without
//   mutating design state -> policy compares estimates and picks a winner
//   -> MoveCommitter::commit(candidate) opens an ECO journal, calls apply()
//   once, and commits or rolls back depending on MoveResult.accepted.
//
// Separation of concerns:
//   estimate() must be pure w.r.t. OpenDB/STA so MT policies can call it
//   from worker threads using only prepared Target data.  apply() runs under
//   an open ECO journal on the main thread and is the only place that may
//   mutate the database or perform live STA checks that are not worker-safe.
//
class MoveCandidate
{
 public:
  // === Candidate lifecycle ==================================================
  virtual ~MoveCandidate() = default;

  // Provide a dummy implementation for legacy policy
  virtual Estimate estimate() { return {.legal = true, .score = 1.0f}; }
  virtual MoveResult apply() = 0;
  virtual MoveType type() const = 0;
  // What applying the move writes and what its estimate read.  The base
  // impact holds the target path's stages the estimate read.
  virtual MoveImpact impact() const;
  // Side pins whose slack the estimate charged; PARALLEL_EVAL checks them
  // after the batch STA to find moves whose side effects were underestimated.
  virtual std::vector<const sta::Pin*> guardedSidePins() const { return {}; }
  static std::vector<const sta::Pin*> faninSidePins(
      const EstimatorContext& estimator_context);
  const Target& target() const { return target_; }

 protected:
  MoveCandidate(Resizer& resizer, const Target& target);
  // The base impact plus a new cell on inst: inst and every net on its pins.
  MoveImpact cellSwapImpact(sta::Instance* inst) const;
  MoveResult rejectedMove() const;
  // The side costs of swapping the target cell for `replacement`, charged
  // against the own-path gain of delay_est: the flip-flop input-side checks,
  // then the fanin side paths.  Updates estimate's penalties and score
  // (adjusted gain times score_scale) and rejects it once the gain is gone.
  void chargeCellSwapSideCosts(const DelayEstimate& delay_est,
                               const sta::LibertyCell* replacement,
                               float score_scale,
                               Estimate& estimate) const;

  // === Candidate identity ===================================================
  Resizer& resizer_;
  const Target& target_;  // From the active policy target vector.
};

}  // namespace rsz
