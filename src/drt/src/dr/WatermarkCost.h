// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "frBaseTypes.h"

namespace drt {

// The configuration stores a float, while maze costs use frCost. Use the
// largest float below the cost limit so conversion cannot round above it.
inline float maxWatermarkStrength()
{
  return std::nextafter(static_cast<float>(std::numeric_limits<frCost>::max()),
                        0.0f);
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
