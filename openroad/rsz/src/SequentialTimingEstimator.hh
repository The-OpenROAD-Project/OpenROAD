// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace sta {
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace rsz {
class Resizer;
struct Target;

// Why output of cell is not an edge-triggered register output; kNone when it
// is: a clk->Q arc, no latch arc, and no arc from a setup/hold-checked data
// input (a transparent latch path).  Statetables are not read: a cell may
// hold several and OpenSTA keeps only the last.
enum class RegisterOutputReject : uint8_t
{
  kNone,
  kNoCellOrOutput,
  kClockGate,
  kLatchGroup,
  kLatchArc,
  kTransparentDataArc,
  kNoClkToQ,
};
RegisterOutputReject registerOutputRejectReason(const sta::LibertyCell* cell,
                                                const sta::LibertyPort* output);
// A stable label for a reject reason, for debug output.
const char* registerOutputRejectName(RegisterOutputReject reason);
bool isRegisterOutput(const sta::LibertyCell* cell,
                      const sta::LibertyPort* output);

// Estimates setup/hold impact at a flip-flop's data and clock inputs when
// scoring a Q-path size or VT swap. Uses local load and Liberty-check deltas
// against current STA timing; post-ECO STA determines the actual result.
class SequentialTimingEstimator
{
 public:
  // Scoring verdict and input-side slack loss for one candidate swap.
  struct Result
  {
    // False when input-side timing cannot be scored or is illegal.
    bool legal{false};
    // Maximum criticality-weighted side slack loss, in seconds; reduces gain.
    float weighted_side_slack_loss{0.0f};
    // Maximum unweighted side slack loss, in seconds; for diagnostics.
    float max_side_slack_loss{0.0f};
    // Static reason label; it remains valid after the estimator is destroyed.
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
  // Prepared STA and Liberty context shared by candidate evaluations.
  struct PreparedFlipFlopTiming;
  // Retains the immutable context while worker threads score candidates.
  explicit SequentialTimingEstimator(
      std::shared_ptr<const PreparedFlipFlopTiming> prepared_timing);
  // Timing and equivalent-cell models captured on the main thread.
  const std::shared_ptr<const PreparedFlipFlopTiming> prepared_timing_;
};

}  // namespace rsz
