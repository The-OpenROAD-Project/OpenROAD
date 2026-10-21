// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "fsv/Geometry.h"

namespace fsv {

// Nearest conductor to a point in the L-infinity metric, which is the
// half-size of the largest cube centered on the point that touches no
// conductor. The ground plane z = 0 counts as a conductor.
class SpatialIndex
{
 public:
  static constexpr int kGround = -1;

  struct Nearest
  {
    double dist;
    int box;  // index into the boxes, or kGround
  };

  explicit SpatialIndex(const std::vector<Box>& boxes);

  Nearest nearest(const Vec3& p) const;

 private:
  // Bounding-volume hierarchy node; a leaf when count > 0.
  struct Node
  {
    Box bounds;
    int left = -1;
    int right = -1;
    int first = 0;
    int count = 0;
  };

  // Traversal stack size; the tree depth is capped well below it.
  static constexpr int kStackSize = 128;
  // Below this depth splits are chosen by cost; deeper ones are medians,
  // which bounds the depth at kMaxCostDepth + log2(#boxes).
  static constexpr int kMaxCostDepth = 64;

  int build(int first, int last, int depth);
  // Reorders order_[first, last) into two non-empty halves and returns the
  // split point.
  int partition(int first, int last, bool by_cost);

  std::vector<Box> boxes_;
  std::vector<int> order_;
  std::vector<Node> nodes_;
};

}  // namespace fsv
