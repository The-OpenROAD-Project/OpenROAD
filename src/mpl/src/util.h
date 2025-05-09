// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <utility>

#include "shapes.h"

namespace mpl {

struct SACoreWeights
{
  float area{0.0f};
  float outline{0.0f};
  float wirelength{0.0f};
  float guidance{0.0f};
  float fence{0.0f};
};

// The cost of a certain penalty is:
//   cost = weight * normalized_penalty
//
// Where the normalized_penalty is:
//   normalized_penalty = value / normalization_factor
//
// Note: the normalization factor is generated during the
// annealer core initialization.
struct PenaltyData
{
  std::string name;
  float weight{0.0f};
  float value{0.0f};
  float normalization_factor{0.0f};
};

// Utility to help sorting width intervals.
inline bool isMinWidthSmaller(const Interval& width_interval_a,
                              const Interval& width_interval_b)
{
  return width_interval_a.min < width_interval_b.min;
}

// Utility to help sorting tilings.
inline bool isAreaSmaller(const Tiling& tiling_a, const Tiling& tiling_b)
{
  if (tiling_a.area() != tiling_b.area()) {
    return tiling_a.area() < tiling_b.area();
  }

  return tiling_a.width() < tiling_b.width();
}

}  // namespace mpl
