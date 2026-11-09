// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "PowerModel.hh"
#include "PowerSwapCandidate.hh"
#include "VtSwapMtGenerator.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// The cells each instance failed to estimate to or broke a batch with.
// Power swaps only spend slack, so those stay out of reach for the run.
using RejectedSwaps = std::unordered_map<const sta::Instance*,
                                         std::vector<const sta::LibertyCell*>>;

bool isRejected(const RejectedSwaps& rejected,
                const sta::Instance* inst,
                const sta::LibertyCell* cell);

// The setup slack a pin may fall to.  Without a tolerance it is the margin
// everywhere.  With one, an endpoint may fall that far below the lower of the
// margin and its slack at the start, and a driver's floor is its slack less
// the room of the endpoints it reaches.  Pins not set fall to margin -
// tolerance.
class SlackFloors
{
 public:
  SlackFloors(float margin, float tolerance);

  bool tolerant() const { return tolerance_ > 0.0f; }
  // Only with a tolerance.
  void recordEndpoint(const sta::Pin* pin, float start_slack);
  void setDriver(const sta::Pin* pin, float floor) { drivers_[pin] = floor; }
  float endpoint(const sta::Pin* pin) const;
  float driver(const sta::Pin* pin) const;
  // The lowest endpoint floor.
  float lowest() const { return lowest_; }

 private:
  float margin_;
  float tolerance_;
  float lowest_;
  std::unordered_map<const sta::Pin*, float> endpoints_;
  std::unordered_map<const sta::Pin*, float> drivers_;
};

// The VT-equivalent cells of cell that leak less, slowest first.
// getVTEquivCells() puts the lowest-leakage VT first.
std::vector<sta::LibertyCell*> higherVtCells(Resizer& resizer,
                                             sta::LibertyCell* cell);

// The repair_power form of VtSwapMt: every higher VT of the same size, which
// leaks less and spends the target's slack.
class PowerVtSwapGenerator : public VtSwapMtGenerator
{
 public:
  PowerVtSwapGenerator(const GeneratorContext& context,
                       PowerModel& power_model,
                       const RejectedSwaps& rejected,
                       const SlackFloors& floors);

  bool isApplicable(const Target& target) const override;
  // Also fills the leakage cache and the slew headroom the workers read in
  // generate(), and lowers the estimator's side slack floor to the lowest
  // floor.  Requires Sta::checkSlewsPreamble().
  void prepareInputs(Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  SlewHeadroom slewHeadroom(const Target& target) const;
  // The smallest slew limit to slew ratio among the pins of the net, and the
  // smallest limit minus slew in margin.
  float netSlewHeadroom(const sta::Pin* driver_pin, float& margin) const;

  PowerModel& power_model_;
  const RejectedSwaps& rejected_;
  const SlackFloors& floors_;
  mutable std::unordered_map<const sta::Pin*, SlewHeadroom> slew_headroom_;
};

}  // namespace rsz
