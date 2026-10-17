// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string_view>

namespace sta {
class LibertyCell;
}

namespace rsz {
class Resizer;
struct Target;

// Estimates setup/hold impact at a flip-flop's data and clock inputs when
// scoring a Q-path size or VT swap. Uses local load and Liberty-check deltas
// against current STA timing; post-ECO STA determines the actual result.
class SequentialTimingEstimator
{
 public:
  struct Result
  {
    bool legal{false};
    // Maximum criticality-weighted side slack loss, in seconds; reduces gain.
    float weighted_side_slack_loss{0.0f};
    // Maximum unweighted side slack loss, in seconds; for diagnostics.
    float max_side_slack_loss{0.0f};
    // Static storage, so it outlives the estimator.
    std::string_view reason;
  };

  // Capture the current flip-flop's setup/hold timing and equivalent-cell
  // models for scoring Q-path size/VT swaps. Main thread only; non-flip-flop
  // targets return null, and missing timing context makes estimates unscorable.
  static std::shared_ptr<const SequentialTimingEstimator> prepare(
      Resizer& resizer,
      const Target& target);

  // Estimate a candidate flip-flop swap's effect on input-side setup and hold
  // checks. Return legality and the side slack losses used to score the Q-path
  // move; reject candidates that create or worsen a timing violation.
  Result estimate(const sta::LibertyCell* candidate_cell) const;

 private:
  struct PreparedFlipFlopTiming;
  explicit SequentialTimingEstimator(
      std::shared_ptr<const PreparedFlipFlopTiming> prepared_timing);
  const std::shared_ptr<const PreparedFlipFlopTiming> prepared_timing_;
};

}  // namespace rsz
