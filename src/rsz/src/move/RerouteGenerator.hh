// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// Produces one resistance-aware reroute candidate for a path-driver net when
// the existing GRT topology suggests a meaningful resistance reduction.
class RerouteGenerator : public MoveGenerator
{
 public:
  explicit RerouteGenerator(const GeneratorContext& context);

  MoveType type() const override { return MoveType::kReroute; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
