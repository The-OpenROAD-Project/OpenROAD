// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_set>

#include "MoveCommitter.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Path;
class Vertex;
}  // namespace sta

namespace rsz {
class Resizer;
}  // namespace rsz

namespace utl {
class Logger;
}

namespace rsz {

class MeasuredVtSwapGenerator;

// Experimental single-threaded VT-swap-only policy
// (selected via -policy "MEASURED_VT_SWAP").
//
// Unlike the legacy path that relies on local delay-table estimation, this
// policy scores each VT-swap candidate by:
//   1. Open an ECO journal.
//   2. Apply the cell replacement to OpenDB.
//   3. Run incremental STA to measure real post-swap timing.
//   4. Record the TNS/WNS delta as the candidate score.
//   5. Restore the journal (undo the replacement).
//   6. Commit only the best-scoring candidate permanently.
//
// This "measure then decide" approach catches inter-cell coupling effects
// that a pure table-model estimator cannot predict, but is N× slower per
// target where N = number of VT-equivalent cells evaluated.
//
// Convergence: the policy stops when no violating endpoint can produce an
// accepted VT swap, when the positive max_committed_moves cap is reached, or
// when every endpoint/instance has been exhausted (tracked in
// exhausted_endpoints_ / exhausted_instances_).
class MeasuredVtSwapPolicy : public OptimizationPolicy
{
 public:
  // === OptimizationPolicy entry points
  // ==============================================
  MeasuredVtSwapPolicy(Resizer& resizer,
                       MoveCommitter& committer,
                       RepairSetupContext& setup_context,
                       const OptimizerRunConfig& config);
  ~MeasuredVtSwapPolicy() override;

  const char* name() const override { return "MeasuredVtSwapPolicy"; }
  bool start() override;
  void iterate() override;

 private:
  // === Run lifecycle and convergence =======================================
  int countViolatingEndpoints() const;
  void finishRun(bool result);

  // === Target selection =====================================================
  sta::Vertex* findWorstViolatingEndpoint() const;
  sta::Path* findWorstSlackPath(sta::Vertex* endpoint) const;
  bool selectLargestStageDelayTarget(sta::Path* path, Target& target) const;

  // === Candidate measurement and commit ====================================
  bool estimateAndCommitBestCandidate(const Target& target);

  // === Run configuration and generators ====================================
  std::unique_ptr<MeasuredVtSwapGenerator> generator_;

  // === Iteration progress ===================================================
  int committed_moves_{0};
  int iteration_index_{0};
  int attempt_index_{0};

  // === Exhaustion tracking ==================================================
  std::unordered_set<sta::Vertex*> exhausted_endpoints_;
  std::unordered_set<sta::Instance*> exhausted_instances_;
};

}  // namespace rsz
