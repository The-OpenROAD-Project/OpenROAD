// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "VtSwapMtCandidate.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// The slew room among the pins of each net whose slews a swap changes: the
// smallest limit minus slew on the output net, and the smallest limit to slew
// ratio on each input's fanin net, whose load grows with the input pin.
struct SlewHeadroom
{
  struct Fanin
  {
    std::string port;
    float headroom{0.0f};
    float load_cap{0.0f};
  };
  float output_margin{0.0f};
  std::vector<Fanin> fanins;
};

// A VT swap that saves power by spending positive setup slack.  estimate()
// prices the slowdown with the same DelayEstimator window as the setup moves;
// the score is the power saved per second of slack spent.
class PowerSwapCandidate : public VtSwapMtCandidate
{
 public:
  // slack is the target's slack above its floor, of which the swap may spend
  // slack_ratio; power_gain the watts the swap saves.  cheapest_first scores
  // by the slack spent alone.
  PowerSwapCandidate(Resizer& resizer,
                     const Target& target,
                     sta::Instance* inst,
                     sta::LibertyCell* candidate_cell,
                     float power_gain,
                     float slack,
                     SlewHeadroom slew_headroom,
                     float slack_ratio = kSlackRatio,
                     bool cheapest_first = false);

  Estimate estimate() override;
  float powerGain() const { return power_gain_; }
  sta::LibertyCell* candidateCell() const { return candidate_cell_; }
  // The slack the swap spends; set by estimate().
  float cost() const { return cost_; }

  // Share of the target's slack a swap, and the swaps admitted in series with
  // it in one batch, may spend; the rest covers estimate error.  Slack seldom
  // grows, so a swap over it stays rejected.
  static constexpr float kSlackRatio = 0.5f;
  // The estimate reason of a swap over that share.
  static constexpr std::string_view kBudgetReason = "power_slack_budget";

 private:
  bool breaksDrvLimits(const EstimatorContext& context,
                       const std::vector<StageEvaluation>& trace) const;

  float power_gain_{0.0f};
  float slack_{0.0f};
  SlewHeadroom slew_headroom_;
  float slack_ratio_{kSlackRatio};
  bool cheapest_first_{false};
  float cost_{0.0f};
};

}  // namespace rsz
