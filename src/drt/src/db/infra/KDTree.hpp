// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <memory>
#include <vector>

#include "frBaseTypes.h"
#include "odb/geom.h"

using odb::Orientation2D;
using odb::Point;
namespace drt {

struct KDTreeNode
{
  int id;
  Point point;
  std::unique_ptr<KDTreeNode> left;
  std::unique_ptr<KDTreeNode> right;

  KDTreeNode(const int idIn, const Point& pt) : id(idIn), point(pt) {}
};

class KDTree
{
 public:
  KDTree(const std::vector<Point>& points);

  ~KDTree() = default;

  std::vector<int> radiusSearch(const Point& target, int radius) const;

 private:
  std::unique_ptr<KDTreeNode> buildTree(
      const std::vector<std::pair<int, Point>>& points,
      const Orientation2D& orient);

  void radiusSearchHelper(KDTreeNode* node,
                          const Point& target,
                          const frSquaredDistance radius_square,
                          const Orientation2D& orient,
                          std::vector<int>& result) const;

  std::unique_ptr<KDTreeNode> root_;
};
}  // namespace drt
