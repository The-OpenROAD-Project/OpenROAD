// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Index.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/index/predicates.hpp"
#include "boost/iterator/function_output_iterator.hpp"
#include "odb/geom.h"
#include "utl/ThreadPool.h"

namespace pex {

Index::Index(const std::vector<IndexBox>& boxes,
             const int layers,
             const int threads)
{
  if (layers < 0) {
    throw std::invalid_argument("pex: a negative layer count");
  }
  layers_.resize(layers);
  std::vector<std::vector<Value>> values(layers_.size());
  for (int id = 0; std::cmp_less(id, boxes.size()); id++) {
    const IndexBox& b = boxes[id];
    if (b.layer < 0 || std::cmp_greater_equal(b.layer, layers_.size())) {
      throw std::invalid_argument("pex: box " + std::to_string(id)
                                  + " is on no indexed layer");
    }
    if (b.rect.dx() <= 0 || b.rect.dy() <= 0) {
      throw std::invalid_argument("pex: box " + std::to_string(id)
                                  + " has no area");
    }
    values[b.layer].emplace_back(b.rect, id);
  }
  std::vector<int> work(layers_.size());
  for (size_t l = 0; l < layers_.size(); l++) {
    work[l] = static_cast<int>(l);
  }
  utl::ThreadPool pool(std::max(1, threads));
  // The range constructor packs the tree, which is deterministic for a given
  // order of values.
  pool.parallelFor(work, [&](const int l) {
    layers_[l] = Tree(values[l].begin(), values[l].end());
    // A layer's copy is freed as soon as its tree is built.
    std::vector<Value>().swap(values[l]);
  });
}

void Index::query(const int layer,
                  const odb::Rect& rect,
                  std::vector<int>& out) const
{
  layers_[layer].query(boost::geometry::index::intersects(rect),
                       boost::make_function_output_iterator(
                           [&](const Value& v) { out.push_back(v.second); }));
}

std::vector<int> Index::query(const odb::Rect& rect) const
{
  std::vector<int> out;
  for (int layer = 0; std::cmp_less(layer, layers_.size()); layer++) {
    query(layer, rect, out);
  }
  std::ranges::sort(out);
  return out;
}

}  // namespace pex
