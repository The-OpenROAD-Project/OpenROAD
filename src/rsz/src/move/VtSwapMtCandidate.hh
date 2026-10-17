// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// MT-safe VT-swap candidate scored from a prepared EstimatorContext.
//
// Same concept as VtSwapCandidate but estimate() reads from a prepared
// EstimatorContext (built on the main thread and stored on Target) and
// evaluates the Liberty table model locally, making it thread-safe for
// parallel evaluation on worker threads. apply() still runs on the main
// thread via Resizer::replaceCell.
class VtSwapMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  // Scores from target.estimator_context, which must be prepared.
  VtSwapMtCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Instance* inst,
                    sta::LibertyCell* candidate_cell);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveImpact impact() const override;
  std::vector<const sta::Pin*> guardedSidePins() const override
  {
    return faninSidePins(target().estimator_context.value());
  }
  MoveType type() const override { return MoveType::kVtSwap; }

 private:
  // === Candidate state ======================================================
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* candidate_cell_{nullptr};
};

}  // namespace rsz
