// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cstdint>

#include "gtest/gtest.h"
#include "packing.h"

namespace mpl {

// Independent legality: inside the outline, an allowed shape, no overlap
// between blocks that are not both fixed, fixed blocks where they were put.
inline ::testing::AssertionResult isLegal(const PackingProblem& problem,
                                          const PackingResult& result)
{
  if (result.status != PackingStatus::kFits) {
    return ::testing::AssertionFailure()
           << "status is not kFits " << result.solver_status;
  }
  const int n = static_cast<int>(problem.blocks.size());
  if (static_cast<int>(result.placement.size()) != n) {
    return ::testing::AssertionFailure() << "placement size";
  }
  for (int i = 0; i < n; i++) {
    const PackingBlock& block = problem.blocks[i];
    const PlacedBlock& p = result.placement[i];
    if (block.isFixed()) {
      if (p.x != *block.fixed_x || p.y != *block.fixed_y
          || p.width != block.shapes.front().width()
          || p.height != block.shapes.front().height()) {
        return ::testing::AssertionFailure() << "fixed block " << i << " moved";
      }
      continue;
    }
    if (p.x < 0 || p.y < 0 || p.x + p.width > problem.width
        || p.y + p.height > problem.height) {
      return ::testing::AssertionFailure() << "block " << i << " outside";
    }
    if (block.isSoft()) {
      // mpl's SoftMacro: a width in one piece, height = floor(area / width).
      bool shaped = false;
      for (const SoftShape& shape : block.soft_shapes) {
        shaped |= shape.width.min <= p.width && p.width <= shape.width.max
                  && p.height == shape.area / p.width;
      }
      if (!shaped) {
        return ::testing::AssertionFailure() << "soft block " << i << " shape";
      }
    } else if (std::find(block.shapes.begin(),
                         block.shapes.end(),
                         Tiling(p.width, p.height))
               == block.shapes.end()) {
      return ::testing::AssertionFailure() << "block " << i << " shape";
    }
  }
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (problem.blocks[i].isFixed() && problem.blocks[j].isFixed()) {
        continue;
      }
      const PlacedBlock& a = result.placement[i];
      const PlacedBlock& b = result.placement[j];
      if (a.width * static_cast<int64_t>(a.height) == 0
          || b.width * static_cast<int64_t>(b.height) == 0) {
        continue;
      }
      const bool apart = a.x + a.width <= b.x || b.x + b.width <= a.x
                         || a.y + a.height <= b.y || b.y + b.height <= a.y;
      if (!apart) {
        return ::testing::AssertionFailure()
               << "blocks " << i << " and " << j << " overlap";
      }
    }
  }
  return ::testing::AssertionSuccess();
}

}  // namespace mpl
