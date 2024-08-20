/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "KDTree.hpp"

#include <algorithm>
#include <cmath>
using odb::horizontal;
namespace drt {
KDTree::KDTree(const std::vector<Point>& points)
{
  std::vector<std::pair<int, Point>> points_with_ids;
  points_with_ids.reserve(points.size());
  for (int i = 0; i < points.size(); ++i) {
    points_with_ids.emplace_back(i, points[i]);
  }
  root_ = buildTree(points_with_ids, horizontal);
}

std::unique_ptr<KDTreeNode> KDTree::buildTree(
    const std::vector<std::pair<int, Point>>& points,
    const Orientation2D& orient)
{
  if (points.empty()) {
    return nullptr;
  }

  auto sorted_points = points;
  std::sort(
      sorted_points.begin(),
      sorted_points.end(),
      [orient](const std::pair<int, Point>& a, const std::pair<int, Point>& b) {
        return orient == horizontal ? a.second.x() < b.second.x()
                                    : a.second.y() < b.second.y();
      });

  const size_t median_index = sorted_points.size() / 2;
  std::unique_ptr<KDTreeNode> node = std::make_unique<KDTreeNode>(
      sorted_points[median_index].first, sorted_points[median_index].second);

  std::vector<std::pair<int, Point>> left_points(
      sorted_points.begin(), sorted_points.begin() + median_index);
  std::vector<std::pair<int, Point>> right_points(
      sorted_points.begin() + median_index + 1, sorted_points.end());
  const auto next_orient = orient.turn_90();
  node->left = buildTree(left_points, next_orient);
  node->right = buildTree(right_points, next_orient);

  return node;
}

std::vector<int> KDTree::radiusSearch(const Point& target, int radius) const
{
  std::vector<int> result;
  const frSquaredDistance radius_square
      = radius * static_cast<frSquaredDistance>(radius);
  radiusSearchHelper(root_.get(), target, radius_square, horizontal, result);
  return result;
}

void KDTree::radiusSearchHelper(KDTreeNode* node,
                                const Point& target,
                                const frSquaredDistance radius_square,
                                const Orientation2D& orient,
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