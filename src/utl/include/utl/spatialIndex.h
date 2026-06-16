// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <iterator>
#include <utility>
#include <vector>

#include "boost/geometry/index/rtree.hpp"

namespace utl {

// Generic (geometry, payload) spatial index backed by a boost rtree.
// GeometryT can be anything boost::geometry recognizes as an indexable
// (point, box, segment, ...). The caller is responsible for building the
// Value vector; the packing constructor bulk-loads a better-balanced tree
// than repeated insert.
//
// Supports box queries and removal of matched values. Iteration yields
// whatever values are still in the index.
template <class GeometryT, class PayloadT>
class SpatialIndex
{
 public:
  using Value = std::pair<GeometryT, PayloadT>;
  using Tree
      = boost::geometry::index::rtree<Value,
                                      boost::geometry::index::quadratic<16>>;

  explicit SpatialIndex(const std::vector<Value>& values)
      : tree_(values.begin(), values.end())
  {
  }

  template <class BoxT>
  std::vector<Value> query(const BoxT& box) const
  {
    std::vector<Value> hits;
    tree_.query(boost::geometry::index::intersects(box),
                std::back_inserter(hits));
    return hits;
  }

  void remove(const Value& v) { tree_.remove(v); }

  typename Tree::const_iterator begin() const { return tree_.begin(); }
  typename Tree::const_iterator end() const { return tree_.end(); }

 private:
  Tree tree_;
};

}  // namespace utl
