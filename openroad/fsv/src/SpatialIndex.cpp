// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SpatialIndex.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <vector>

#include "fsv/Geometry.h"

namespace fsv {

namespace {

constexpr int kLeafSize = 4;
// Candidate split positions per axis are the boundaries between bins of box
// centers.
constexpr int kBins = 16;

constexpr double kMax = std::numeric_limits<double>::max();
constexpr double kLowest = std::numeric_limits<double>::lowest();

template <int D>
AlignedBox<D> emptyBox()
{
  AlignedBox<D> b;
  b.lo.fill(kMax);
  b.hi.fill(kLowest);
  return b;
}

// Grows a box to cover another.
template <int D>
void grow(AlignedBox<D>& bounds, const AlignedBox<D>& b)
{
  for (int k = 0; k < D; k++) {
    bounds.lo[k] = std::min(bounds.lo[k], b.lo[k]);
    bounds.hi[k] = std::max(bounds.hi[k], b.hi[k]);
  }
}

// A node's volume (area, in 2D), the split cost's measure of how often a
// nearest-distance query has to open it. (Ray tracers use surface area; on
// thin routed wires volume measured 5-15% faster.)
template <int D>
double volume(const AlignedBox<D>& b)
{
  double v = 1;
  for (int k = 0; k < D; k++) {
    v *= b.hi[k] - b.lo[k];
  }
  return v;
}

}  // namespace

template <int D>
SpatialIndex<D>::SpatialIndex(const std::vector<AlignedBox<D>>& boxes)
    : boxes_(boxes), order_(boxes.size())
{
  std::iota(order_.begin(), order_.end(), 0);
  if (!boxes_.empty()) {
    nodes_.reserve(2 * boxes_.size());
    build(0, static_cast<int>(boxes_.size()), 0);
  }
}

template <int D>
int SpatialIndex<D>::build(const int first, const int last, const int depth)
{
  const int id = static_cast<int>(nodes_.size());
  nodes_.emplace_back();
  AlignedBox<D> bounds = emptyBox<D>();
  for (int i = first; i < last; i++) {
    grow(bounds, boxes_[order_[i]]);
  }
  nodes_[id].bounds = bounds;
  if (last - first <= kLeafSize) {
    nodes_[id].first = first;
    nodes_[id].count = last - first;
    return id;
  }
  const int mid = partition(first, last, depth < kMaxCostDepth);
  const int left = build(first, mid, depth + 1);
  const int right = build(mid, last, depth + 1);
  nodes_[id].left = left;
  nodes_[id].right = right;
  return id;
}

template <int D>
int SpatialIndex<D>::partition(const int first,
                               const int last,
                               const bool by_cost)
{
  // Box centers are compared as lo + hi (twice the center) throughout.
  auto center2 = [&](const int box, const int k) {
    return boxes_[box].lo[k] + boxes_[box].hi[k];
  };
  Vec<D> cmin;
  Vec<D> cmax;
  cmin.fill(kMax);
  cmax.fill(kLowest);
  for (int i = first; i < last; i++) {
    for (int k = 0; k < D; k++) {
      cmin[k] = std::min(cmin[k], center2(order_[i], k));
      cmax[k] = std::max(cmax[k], center2(order_[i], k));
    }
  }
  auto binOf = [&](const int box, const int k) {
    const int b = static_cast<int>((center2(box, k) - cmin[k])
                                   / (cmax[k] - cmin[k]) * kBins);
    return std::clamp(b, 0, kBins - 1);
  };

  // Binned split cost: on every axis, score each boundary between center
  // bins by volume(left) * count(left) + volume(right) * count(right) and
  // keep the cheapest. On routed layers this separates the
  // layers first (their z ranges are disjoint), then the wires across their
  // routing direction, and it finds a layer boundary even when the layers
  // hold different numbers of boxes.
  double best_cost = kMax;
  int best_axis = -1;
  int best_bin = 0;
  for (int k = 0; by_cost && k < D; k++) {
    if (!(cmax[k] > cmin[k])) {
      continue;
    }
    std::array<AlignedBox<D>, kBins> bin_bounds;
    bin_bounds.fill(emptyBox<D>());
    std::array<int, kBins> bin_count{};
    for (int i = first; i < last; i++) {
      const int b = binOf(order_[i], k);
      grow(bin_bounds[b], boxes_[order_[i]]);
      bin_count[b]++;
    }
    // Sweep from the right for each suffix's volume and count.
    std::array<double, kBins> right_volume{};
    std::array<int, kBins> right_count{};
    AlignedBox<D> acc = emptyBox<D>();
    int count = 0;
    for (int b = kBins - 1; b > 0; b--) {
      grow(acc, bin_bounds[b]);
      count += bin_count[b];
      right_volume[b] = count > 0 ? volume(acc) : 0.0;
      right_count[b] = count;
    }
    // Sweep from the left; splitting before bin b puts bins < b on the left.
    acc = emptyBox<D>();
    count = 0;
    for (int b = 1; b < kBins; b++) {
      grow(acc, bin_bounds[b - 1]);
      count += bin_count[b - 1];
      if (count == 0 || right_count[b] == 0) {
        continue;
      }
      const double cost
          = volume(acc) * count + right_volume[b] * right_count[b];
      if (cost < best_cost) {
        best_cost = cost;
        best_axis = k;
        best_bin = b;
      }
    }
  }
  if (best_axis >= 0) {
    const auto mid = std::partition(
        order_.begin() + first, order_.begin() + last, [&](const int box) {
          return binOf(box, best_axis) < best_bin;
        });
    return static_cast<int>(mid - order_.begin());
  }

  // Median split on the axis with the widest spread of centers, used below
  // kMaxCostDepth and when every center coincides.
  int axis = 0;
  for (int k = 1; k < D; k++) {
    if (cmax[k] - cmin[k] > cmax[axis] - cmin[axis]) {
      axis = k;
    }
  }
  const int mid = (first + last) / 2;
  // Partition the box indices around the median center on that axis. Only
  // the halves matter, so the O(n) nth_element suffices instead of a sort.
  std::nth_element(order_.begin() + first,
                   order_.begin() + mid,
                   order_.begin() + last,
                   [&](const int a, const int b) {
                     return center2(a, axis) < center2(b, axis);
                   });
  return mid;
}

template <int D>
typename SpatialIndex<D>::Nearest SpatialIndex<D>::nearest(
    const Vec<D>& p) const
{
  // A point at or below the ground plane is touching it.
  Nearest best{std::max(0.0, p[D - 1]), kGround};
  if (nodes_.empty()) {
    return best;
  }
  // Branch and bound, visiting the closer child first. Each stack entry
  // carries its node's bounds distance, computed once when it is pushed.
  struct Entry
  {
    int node;
    double dist;
  };
  std::array<Entry, kStackSize> stack;
  int top = 0;
  stack[top++] = {0, linfDistance(p, nodes_[0].bounds)};
  while (top > 0) {
    const Entry entry = stack[--top];
    if (entry.dist >= best.dist) {
      continue;
    }
    const Node& node = nodes_[entry.node];
    if (node.count > 0) {
      for (int i = node.first; i < node.first + node.count; i++) {
        const double d = linfDistance(p, boxes_[order_[i]]);
        if (d < best.dist) {
          best = {d, order_[i]};
        }
      }
      continue;
    }
    const double dl = linfDistance(p, nodes_[node.left].bounds);
    const double dr = linfDistance(p, nodes_[node.right].bounds);
    if (dl < dr) {
      stack[top++] = {node.right, dr};
      stack[top++] = {node.left, dl};
    } else {
      stack[top++] = {node.left, dl};
      stack[top++] = {node.right, dr};
    }
  }
  return best;
}

template class SpatialIndex<2>;
template class SpatialIndex<3>;

}  // namespace fsv
