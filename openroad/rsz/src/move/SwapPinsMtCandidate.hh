// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class LibertyPort;
class Pin;
}  // namespace sta

namespace rsz {

// The arc delays the main thread measured on one target gate: through the
// input the critical path enters and through each equivalent input.  With the
// side model, each equivalent input also holds its pin and that pin's slack.
struct SwapPinsMtInput
{
  // Equivalent gate input with its measured delay and side-path slack.
  struct Equivalent
  {
    sta::LibertyPort* port{nullptr};
    float delay{0.0f};
    const sta::Pin* pin{nullptr};
    float slack{0.0f};
  };

  sta::LibertyPort* input_port{nullptr};
  float input_delay{0.0f};
  std::vector<Equivalent> equivalents;
};

// MT-safe pin swap: the critical signal moves to a faster equivalent input.
// estimate() compares the measured arc delays.  With the side model, the
// displaced signal pays the same difference.
class SwapPinsMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SwapPinsMtCandidate(Resizer& resizer,
                      const Target& target,
                      std::shared_ptr<const SwapPinsMtInput> input,
                      const SwapPinsMtInput::Equivalent& swap);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSwapPins; }
  MoveImpact impact() const override;
  std::vector<const sta::Pin*> guardedSidePins() const override
  {
    if (swap_.pin == nullptr) {
      return {};
    }
    return {swap_.pin};
  }

 private:
  // === Candidate state ======================================================
  std::shared_ptr<const SwapPinsMtInput> input_;
  // The equivalent input the critical signal moves to.
  SwapPinsMtInput::Equivalent swap_;
};

}  // namespace rsz
