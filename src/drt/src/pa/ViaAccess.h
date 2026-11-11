// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include "odb/geom.h"

namespace drt {

// Origins fitting the lower landing in pin and both landings in cell.
inline std::optional<odb::Rect> viaAccessOriginWindow(const odb::Rect& source,
                                                      const odb::Rect& lower,
                                                      const odb::Rect& upper,
                                                      const odb::Rect& boundary)
{
  using I = int64_t;
  const I xl = std::max({I(source.xMin()),
                         I(source.xMin()) - lower.xMin(),
                         I(boundary.xMin()) - lower.xMin(),
                         I(boundary.xMin()) - upper.xMin()});
  const I xh = std::min({I(source.xMax()),
                         I(source.xMax()) - lower.xMax(),
                         I(boundary.xMax()) - lower.xMax(),
                         I(boundary.xMax()) - upper.xMax()});
  const I yl = std::max({I(source.yMin()),
                         I(source.yMin()) - lower.yMin(),
                         I(boundary.yMin()) - lower.yMin(),
                         I(boundary.yMin()) - upper.yMin()});
  const I yh = std::min({I(source.yMax()),
                         I(source.yMax()) - lower.yMax(),
                         I(boundary.yMax()) - lower.yMax(),
                         I(boundary.yMax()) - upper.yMax()});
  if (xl > xh || yl > yh) {
    return std::nullopt;
  }
  return odb::Rect(static_cast<int>(xl),
                   static_cast<int>(yl),
                   static_cast<int>(xh),
                   static_cast<int>(yh));
}

inline std::vector<int> sampleViaAccessOrigins(const int low,
                                               const int high,
                                               const int grid)
{
  constexpr int kDenseSampleLimit = 16;
  if (grid <= 0 || low > high) {
    return {};
  }
  const auto floor_grid = [grid](const int64_t value) {
    const int64_t rem = value % grid;
    return value - rem - (rem < 0 ? grid : 0);
  };
  const auto ceil_grid = [&](const int64_t value) {
    const int64_t floor = floor_grid(value);
    return floor == value ? floor : floor + grid;
  };
  const int64_t first = ceil_grid(low);
  const int64_t last = floor_grid(high);
  if (first > last) {
    return {};
  }
  std::vector<int> result;
  if ((last - first) / grid + 1 <= kDenseSampleLimit) {
    for (int64_t position = first; position <= last; position += grid) {
      result.push_back(static_cast<int>(position));
    }
  } else {
    const int64_t sum = int64_t(low) + high;
    // floor(sum/2), rather than C++ division toward zero.
    const int64_t midpoint_low = sum / 2 - (sum < 0 && sum % 2 != 0);
    const int64_t midpoint_high = sum / 2 + (sum > 0 && sum % 2 != 0);
    std::vector<int64_t> positions{first, last};
    positions.push_back(floor_grid(midpoint_low));
    positions.push_back(ceil_grid(midpoint_high));
    for (const int64_t position : positions) {
      if (position >= first && position <= last) {
        result.push_back(static_cast<int>(position));
      }
    }
    std::ranges::sort(result);
    result.erase(std::unique(result.begin(), result.end()), result.end());
  }
  return result;
}

}  // namespace drt
