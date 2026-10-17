// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "odb/geom.h"

namespace rsz {

// Candidate that scores and applies one full Rebuffer-generated buffer tree.
class RebufferMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  // side_contexts lists the plan's other loads with their current slack; when
  // non-empty, estimate() charges their buffered-detour loss against the gain.
  RebufferMtCandidate(Resizer& resizer,
                      const Target& target,
                      const EstimatorContext& estimator_context,
                      const RebufferTreePlan& plan,
                      float score_scale,
                      float min_score,
                      std::vector<BufferTreeSidePathContext> side_contexts
                      = {});

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kBuffer; }
  float scaledScore() const { return raw_score_ * score_scale_; }

 private:
  std::string logName() const;

  // === Candidate state ======================================================
  const EstimatorContext& estimator_context_;
  RebufferTreePlan plan_;
  std::vector<BufferTreeSidePathContext> side_contexts_;
  const float score_scale_;
  const float min_score_;
  float raw_score_{0.0f};
};

}  // namespace rsz
