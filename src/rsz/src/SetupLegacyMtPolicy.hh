// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SetupLegacyPolicy.hh"

namespace rsz {

class MoveCommitter;
class Resizer;
class SizeUpMtGenerator;
class VtSwapMtGenerator;

// Hybrid single/multi-threaded setup-repair policy (RSZ_POLICY=legacy_mt).
//
// SetupLegacyMtPolicy keeps SetupLegacyPolicy's serial endpoint, target, and
// move-type ordering.  Only the VtSwap and SizeUp move types use MT candidate
// expansion and parallel candidate scoring for one prepared target at a time.
// All other generators stay on the caller thread, which preserves their
// single-threaded assumptions while still allowing incremental MT expansion by
// move type.
class SetupLegacyMtPolicy : public SetupLegacyPolicy
{
 public:
  // === OptPolicy entry points ==============================================
  SetupLegacyMtPolicy(Resizer& resizer, MoveCommitter& committer);
  ~SetupLegacyMtPolicy() override;

  const char* name() const override { return "SetupLegacyMtPolicy"; }
  void start(const OptimizerRunConfig& config) override;

 protected:
  // === SetupLegacyPolicy hooks
  // ===================================================
  void buildMoveGenerators(const std::vector<MoveType>& move_types,
                           const GeneratorContext& context) override;
  bool targetPrewarmEnabled() const override { return true; }
  bool tryRepairTarget(const Target& target,
                       int repairs_per_pass,
                       int& changed,
                       const std::unordered_set<MoveType>* rejected_types,
                       std::optional<MoveType>& accepted_type) override;

 private:
  // === Per-target MT candidate scoring =====================================
  bool canTryGenerator(
      const MoveGenerator& generator,
      const Target& target,
      const std::unordered_set<MoveType>* rejected_types) const;
  void logConsideringGenerator(const MoveGenerator& generator,
                               const Target& target) const;
  bool usesMtCandidateScoring(MoveType type) const;
  MoveCandidate* estimateCandidatesMt(CandidateVector& candidates,
                                      std::vector<Estimate>& estimates,
                                      Estimate& best_estimate);
  bool estimateAndCommitCandidates(const Target& target,
                                   MoveType type,
                                   CandidateVector& candidates,
                                   int repairs_per_pass,
                                   int& changed,
                                   std::optional<MoveType>& accepted_type);
  bool estimateAndCommitMtCandidates(const Target& target,
                                     MoveType type,
                                     CandidateVector& candidates,
                                     int repairs_per_pass,
                                     int& changed,
                                     std::optional<MoveType>& accepted_type);
  bool estimateAndCommitSerialCandidates(
      const Target& target,
      MoveType type,
      CandidateVector& candidates,
      int repairs_per_pass,
      int& changed,
      std::optional<MoveType>& accepted_type);
  bool estimateAndCommitSizeDownBatch(MoveGenerator& generator,
                                      const Target& target,
                                      int repairs_per_pass,
                                      int& changed,
                                      std::optional<MoveType>& accepted_type);
  MoveResult commitCandidate(const Target& target,
                             MoveType type,
                             MoveCandidate& candidate);
};

}  // namespace rsz
