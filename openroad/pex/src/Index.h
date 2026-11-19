// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The conductor boxes of a block, indexed per layer, for the queries
// extraction makes around each wire.

#pragma once

#include <utility>
#include <vector>

#include "boost/geometry/index/rtree.hpp"
#include "odb/geom.h"
#include "odb/geom_boost.h"  // IWYU pragma: keep

namespace pex {

// A box in database units on a layer (an index of the caller's layers).
struct IndexBox
{
  int layer;
  odb::Rect rect;
};

// One rtree per layer, bulk loaded, holding each box's rectangle and its id
// (its position in the boxes given). Built once and read-only afterwards, so
// queries may run in parallel.
class Index
{
 public:
  // Throws std::invalid_argument for a negative layer count, a box on a layer
  // outside [0, layers), or a box without area. Layers are built on `threads`
  // threads.
  Index(const std::vector<IndexBox>& boxes, int layers, int threads = 1);

  // Appends to `out` the ids of the boxes on `layer` that overlap `rect`,
  // edges touching included, each once, in an unspecified order.
  void query(int layer, const odb::Rect& rect, std::vector<int>& out) const;
  // The ids of the boxes on every layer that overlap `rect`, ascending.
  std::vector<int> query(const odb::Rect& rect) const;

 private:
  using Value = std::pair<odb::Rect, int>;  // a box and its id
  using Tree
      = boost::geometry::index::rtree<Value,
                                      boost::geometry::index::quadratic<16>>;

  std::vector<Tree> layers_;
};

}  // namespace pex
