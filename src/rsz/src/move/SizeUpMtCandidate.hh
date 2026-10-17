// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class LibertyCell;
class Pin;
class Instance;
}  // namespace sta

namespace rsz {

// MT-safe size-up candidate scored from a prepared EstimatorContext.
//
// Unlike the legacy SizeUpCandidate which queries STA on the fly, this
// variant receives a state of the timing arc (EstimatorContext) built on
// the main thread and stored in Target. estimate() evaluates the Liberty table
// model purely from that snapshot (thread-safe; no STA mutation), so MT
// policies can run it on worker threads. apply() still
// executes on the main thread via Resizer::replaceCell.
class SizeUpMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  // Scores from target.estimator_context, which must be prepared.
  SizeUpMtCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Instance* inst,
                    sta::LibertyCell* replacement,
                    float score_scale);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSizeUp; }

 private:
  std::string logName() const;

  // === Candidate state ======================================================
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* replacement_{nullptr};
  const float score_scale_;
};

}  // namespace rsz
