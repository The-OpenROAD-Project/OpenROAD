// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <type_traits>
#include <vector>

#include "fsv/Geometry.h"

namespace fsv {

// Nearest conductor to a point in the L-infinity metric, which is the
// half-size of the largest cube (square, in 2D) centered on the point that
// touches no conductor. The ground plane (the last coordinate = 0) counts as
// a conductor. Also answers which boxes overlap a query box. Instantiated
// for D = 2 and 3.
template <int D>
class SpatialIndex
{
 public:
  static constexpr int kGround = -1;

  struct Nearest
  {
    double dist;
    int box;  // index into the boxes, or kGround
  };

  explicit SpatialIndex(const std::vector<AlignedBox<D>>& boxes);

  Nearest nearest(const Vec<D>& p) const;

  // Calls visit(i) for every box i that intersects `query` (closed boxes,
  // so touching counts), in no particular order. The ground plane is not
  // a box here. A visitor that returns bool stops the query by returning
  // false.
  template <typename Visit>
  void forEachOverlapping(const AlignedBox<D>& query, Visit visit) const
  {
    if (nodes_.empty()) {
      return;
    }
    std::array<int, kStackSize> stack;
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
      const Node& node = nodes_[stack[--top]];
      if (!intersects(node.bounds, query)) {
        continue;
      }
      if (node.count > 0) {
        for (int i = node.first; i < node.first + node.count; i++) {
          if (!intersects(boxes_[order_[i]], query)) {
            continue;
          }
          if constexpr (std::is_void_v<std::invoke_result_t<Visit, int>>) {
            visit(order_[i]);
          } else if (!visit(order_[i])) {
            return;
          }
        }
        continue;
      }
      stack[top++] = node.right;
      stack[top++] = node.left;
    }
  }

 private:
  // Bounding-volume hierarchy node; a leaf when count > 0.
  struct Node
  {
    AlignedBox<D> bounds;
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

  static bool intersects(const AlignedBox<D>& a, const AlignedBox<D>& b)
  {
    for (int k = 0; k < D; k++) {
      if (a.hi[k] < b.lo[k] || b.hi[k] < a.lo[k]) {
        return false;
      }
    }
    return true;
  }

  int build(int first, int last, int depth);
  // Reorders order_[first, last) into two non-empty halves and returns the
  // split point.
  int partition(int first, int last, bool by_cost);

  std::vector<AlignedBox<D>> boxes_;
  std::vector<int> order_;
  std::vector<Node> nodes_;
};

}  // namespace fsv
