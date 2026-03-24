// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "KDTree.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "frBaseTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
using odb::horizontal;
namespace drt {
KDTree::KDTree(const std::vector<odb::Point>& points)
{
  std::vector<std::pair<int, odb::Point>> points_with_ids;
  points_with_ids.reserve(points.size());
  for (int i = 0; i < points.size(); ++i) {
    points_with_ids.emplace_back(i, points[i]);
  }
  root_ = buildTree(points_with_ids, horizontal);
}

std::unique_ptr<KDTreeNode> KDTree::buildTree(
    const std::vector<std::pair<int, odb::Point>>& points,
    const odb::Orientation2D& orient)
{
  if (points.empty()) {
    return nullptr;
  }

  auto sorted_points = points;
  std::ranges::sort(sorted_points,
                    [orient](const std::pair<int, odb::Point>& a,
                             const std::pair<int, odb::Point>& b) {
                      return orient == horizontal ? a.second.x() < b.second.x()
                                                  : a.second.y() < b.second.y();
                    });

  const size_t median_index = sorted_points.size() / 2;
  std::unique_ptr<KDTreeNode> node = std::make_unique<KDTreeNode>(
      sorted_points[median_index].first, sorted_points[median_index].second);

  std::vector<std::pair<int, odb::Point>> left_points(
      sorted_points.begin(), sorted_points.begin() + median_index);
  std::vector<std::pair<int, odb::Point>> right_points(
      sorted_points.begin() + median_index + 1, sorted_points.end());
  const auto next_orient = orient.turn_90();
  node->left = buildTree(left_points, next_orient);
  node->right = buildTree(right_points, next_orient);

  return node;
}

std::vector<int> KDTree::radiusSearch(const odb::Point& target,
                                      int radius) const
{
  std::vector<int> result;
  const frSquaredDistance radius_square
      = radius * static_cast<frSquaredDistance>(radius);
  radiusSearchHelper(root_.get(), target, radius_square, horizontal, result);
  return result;
}

void KDTree::radiusSearchHelper(KDTreeNode* node,
                                const odb::Point& target,
                                const frSquaredDistance radius_square,
                                const odb::Orientation2D& orient,
                                std::vector<int>& result) const
{
  if (!node) {
    return;
  }
  const frCoord dx = target.x() - node->point.x();
  const frCoord dy = target.y() - node->point.y();
  const frSquaredDistance distance_square
      = dx * static_cast<frSquaredDistance>(dx)
        + dy * static_cast<frSquaredDistance>(dy);
  if (distance_square <= radius_square) {
    result.push_back(node->id);
  }

  const frCoord diff = (orient == horizontal) ? dx : dy;
  const frSquaredDistance diff_square
      = diff * static_cast<frSquaredDistance>(diff);

  const auto next_orient = orient.turn_90();

  if (diff < 0) {
    radiusSearchHelper(
        node->left.get(), target, radius_square, next_orient, result);
    if (diff_square <= radius_square) {
      radiusSearchHelper(
          node->right.get(), target, radius_square, next_orient, result);
    }
  } else {
    radiusSearchHelper(
        node->right.get(), target, radius_square, next_orient, result);
    if (diff_square <= radius_square) {
      radiusSearchHelper(
          node->left.get(), target, radius_square, next_orient, result);
    }
  }
}
}  // namespace drt
