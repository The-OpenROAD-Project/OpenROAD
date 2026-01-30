// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "frBaseTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

namespace drt {

struct KDTreeNode
{
  int id;
  odb::Point point;
  std::unique_ptr<KDTreeNode> left;
  std::unique_ptr<KDTreeNode> right;

  KDTreeNode(const int idIn, const odb::Point& pt) : id(idIn), point(pt) {}
};

class KDTree
{
 public:
  KDTree(const std::vector<odb::Point>& points);

  ~KDTree() = default;

  std::vector<int> radiusSearch(const odb::Point& target, int radius) const;

 private:
  std::unique_ptr<KDTreeNode> buildTree(
      const std::vector<std::pair<int, odb::Point>>& points,
      const odb::Orientation2D& orient);

  void radiusSearchHelper(KDTreeNode* node,
                          const odb::Point& target,
                          frSquaredDistance radius_square,
                          const odb::Orientation2D& orient,
                          std::vector<int>& result) const;

  std::unique_ptr<KDTreeNode> root_;
};
}  // namespace drt
