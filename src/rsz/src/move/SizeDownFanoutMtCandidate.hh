// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"

namespace sta {
class Instance;
class LibertyCell;
class Pin;
}  // namespace sta

namespace rsz {

// The smaller cell a fanout load steps down to, measured on the main thread:
// the input capacitance it presents to the target driver, whether its outputs
// break a max-cap or max-slew limit, and the worst delay change over its
// outputs including the driver's gain from the smaller pin.
struct SizeDownFanoutMtCellOption
{
  sta::LibertyCell* cell{nullptr};
  float input_cap{0.0f};
  bool violates_output_limits{false};
  float worst_delay_change{0.0f};
  // The input pin capacitance of this cell less the current one, both on the
  // target scene; nullopt when either scene cell lacks the load port.
  std::optional<float> target_load_delta{};
};

// One fanout load of the target driver and the cell it steps down to,
// measured on the main thread.  generate() judges the step.
struct SizeDownFanoutMtLoad
{
  sta::Instance* load_inst{nullptr};
  sta::Pin* load_pin{nullptr};
  sta::LibertyPort* load_port{nullptr};
  sta::LibertyCell* current_cell{nullptr};
  sta::Slack load_slack{0.0};
  // The delay the load may add: its worst output slack for a sequential cell,
  // else the worse of its slack and its instance's worst input slack.
  sta::Slack delay_budget{0.0};
  float input_cap{0.0f};
  // One drive step down.  prepareInputs() picks it because a load has only
  // this one candidate cell and measuring it needs the main thread's delay
  // calculator.
  SizeDownFanoutMtCellOption step_down{};
};

// MT-safe size-down of one fanout load of the target driver to its prepared
// step-down cell.  estimate() scores the target path gain from the prepared
// EstimatorContext, and apply() swaps the cell.
class SizeDownFanoutMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SizeDownFanoutMtCandidate(Resizer& resizer,
                            const Target& target,
                            std::shared_ptr<const SizeDownFanoutMtLoad> load,
                            float score_scale);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSizeDownFanout; }
  MoveImpact impact() const override;

 private:
  // === Candidate state ======================================================
  std::shared_ptr<const SizeDownFanoutMtLoad> load_;
  float score_scale_{1.0f};
};

}  // namespace rsz
