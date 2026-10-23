// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class LibertyCell;
class LibertyPort;
class Pin;
class MinMax;
class Scene;
class Instance;
}  // namespace sta

namespace rsz {

// MT-safe generator that enumerates same-VT cells that do not weaken output
// drive for one target using a prepared EstimatorContext.
//
// prepareRequirements() asks for the estimator context and the flip-flop
// input-side checks, and prepareInputs() captures the input-cap budgets and
// the replacement options, so generate() runs on worker threads without live
// STA or cache writes.  Produces multiple SizeUpMtCandidate instances so the
// policy can evaluate them in parallel and pick the best.
class SizeUpMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeUpMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeUp; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache | kSequentialEstimatorCache;
  }
  bool isApplicable(const Target& target) const override;
  void prepareInputs(Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // The unit tests (TestResizerMt) reach the internals through this peer.
  friend class ResizerTestPeer;

  // === Candidate-cell selection ============================================
  // Replacement cells for one driver port and analysis view.  The library and
  // dont_use set are fixed for a run, so the options are cached for the run.
  // prepareInputs() fills the cache on the main thread and generate() only
  // reads it.  Each generate() replays the dual-RF rejects to the profiler.
  struct SizeUpOptions
  {
    std::vector<sta::LibertyCell*> cells;
    int dual_rf_rejects{0};
  };

  using SizeUpOptionKey = std::
      tuple<const sta::LibertyPort*, const sta::Scene*, const sta::MinMax*>;

  // Main thread only: a new entry fills the Resizer's swappable-cell cache.
  const SizeUpOptions& findSizeUpOptions(const sta::LibertyPort* drvr_port,
                                         const sta::Scene* scene,
                                         const sta::MinMax* min_max) const;
  SizeUpOptions buildSizeUpOptions(const sta::LibertyPort* drvr_port,
                                   const sta::Scene* scene,
                                   const sta::MinMax* min_max) const;

  mutable std::map<SizeUpOptionKey, SizeUpOptions> size_up_options_;
};

}  // namespace rsz
