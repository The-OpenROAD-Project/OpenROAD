// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "frBaseTypes.h"

namespace drt {

// Upper bound on the routing watermark multiplier.  Maze costs are frUInt4
// and a path accumulates many edges, so a multiplier this large already
// saturates any path with a few hundred wrong-way edges on it; a larger one
// would only make distinct expensive paths indistinguishable sooner.
inline float maxWatermarkStrength()
{
  return 10000.0f;
}

inline bool isValidWatermarkStrength(double strength)
{
  return std::isfinite(strength) && strength >= 0.0
         && strength <= maxWatermarkStrength();
}

inline frCost saturateWatermarkCost(uint64_t cost)
{
  return static_cast<frCost>(
      std::min(cost, uint64_t{std::numeric_limits<frCost>::max()}));
}

// Callers validate the multiplier before storing it. Widen the base cost
// before multiplying, and clamp before conversion to avoid undefined behavior.
inline frCost scaledWatermarkCost(uint64_t cost, float multiplier)
{
  const double scaled = static_cast<double>(cost) * multiplier;
  const auto limit = std::numeric_limits<frCost>::max();
  return scaled >= limit ? limit : static_cast<frCost>(scaled);
}

inline frCost addWatermarkCosts(frCost path_cost, frCost estimated_cost)
{
  return saturateWatermarkCost(uint64_t{path_cost} + estimated_cost);
}

}  // namespace drt
