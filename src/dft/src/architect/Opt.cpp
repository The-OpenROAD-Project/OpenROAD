// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Opt.hh"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "ClockDomain.hh"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace dft {

namespace {
// Max number of cells we assume could be placed at the same point (hopefully,
// after DP, this is never more than one)
constexpr int kMaxCellsToSearch = 10;
}  // namespace

void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger)
{
  // Nothing to order
  if (cells.empty()) {
    return;
  }
  // No point running this if the cells aren't placed yet
  for (const auto& cell : cells) {
    if (!cell->isPlaced()) {
      return;
    }
  }
  // Define the starting node as the lower leftmost, so we don't accidentally
  // start somewhere in the middle
  size_t start_index = 0;
  int64_t lowest_dist = std::numeric_limits<int64_t>::max();
  // Get points in a form ready to insert into index
  using bg_point = bg::model::point<int, 2, bg::cs::cartesian>;
  std::vector<std::pair<bg_point, size_t>> transformed;

  for (size_t i = 0; i < cells.size(); i++) {
    // Find the lower leftmost cell by looking for the cell with the lowest
    // manhattan distance to the origin
    auto origin = cells[i]->getOrigin();
    const int64_t dist = origin.x() + origin.y();
    if (dist < lowest_dist) {
      start_index = i;
      lowest_dist = dist;
    }
    // Add the point to the set of points
    transformed.emplace_back(bg_point(origin.x(), origin.y()), i);
  }
  // Update the index
  bgi::rtree<std::pair<bg_point, size_t>, bgi::rstar<4>> rtree(transformed);
  auto cursor = transformed[start_index];

  // Search nearest neighbours
  std::vector<std::unique_ptr<ScanCell>> result;
  result.emplace_back(std::move(cells[cursor.second]));
  while (result.size() < cells.size()) {
    // Search for next nearest
    auto next = cursor;
    for (auto it = rtree.qbegin(bgi::nearest(cursor.first, kMaxCellsToSearch));
         it != rtree.qend();
         ++it) {
      next = *it;
      if (next.second != cursor.second) {
        break;
      }
    }
    if (next.second == cursor.second) {
      logger->error(
          utl::DFT,
          10,
          "Couldn't find nearest neighbor, too many overlapping cells");
    }
    // Make sure we only visit things once
    rtree.remove(cursor);
    cursor = std::move(next);
    result.emplace_back(std::move(cells[cursor.second]));
  }
  // Replace with sorted vector
  std::swap(cells, result);
}

}  // namespace dft
