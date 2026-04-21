// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class LibertyCell;
class LibertyPort;
class MinMax;
class Scene;
}  // namespace sta

namespace rsz {

// MT-safe generator that enumerates all same-VT stronger cells for one
// target using a prepared ArcDelayState.
//
// prepareRequirements() requests kArcDelayState so worker threads can read the
// prepared timing snapshot without touching shared STA state. Produces multiple
// SizeUpMtCandidate instances (up to max_candidate_generation) so the policy
// can evaluate them in parallel and pick the best. Main-thread commit still
// performs the live max-cap check before applying the winning replacement.
class SizeUpMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeUpMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeUp; }
  PrepareCacheMask prepareRequirements() const override
  {
    return prepareCacheMask(PrepareCacheKind::kArcDelayState);
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Candidate-cell selection ============================================
  std::vector<sta::LibertyCell*> findSizeUpOptions(
      const sta::LibertyPort* drvr_port,
      const sta::Scene* scene,
      const sta::MinMax* min_max) const;
};

}  // namespace rsz
