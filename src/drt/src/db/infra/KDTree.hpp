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

#pragma once

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