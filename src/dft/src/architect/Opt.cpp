// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Opt.hh"

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometry.hpp"
#include "odb/geom.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace dft {

namespace {
// Max number of cells we assume could be placed at the same point (hopefully,
// after DP, this is never more than one)
constexpr int kMaxCellsToSearch = 10;
}  // namespace

int64_t OptimizeScanWirelengthNN(std::vector<std::unique_ptr<ScanCell>>& cells,
                                 utl::Logger* logger)
{
  // Nothing to order
  if (cells.empty()) {
    return 0;
  }
  // No point running this if the cells aren't placed yet
  for (const auto& cell : cells) {
    if (!cell->isPlaced()) {
      return 0;
    }
  }
  // Define the starting node as the lower leftmost, so we don't accidentally
  // start somewhere in the middle
  size_t start_index = 0;
  int64_t lowest_dist = std::numeric_limits<int64_t>::max();
  // Get points in a form ready to insert into index
  using Point = bg::model::point<int, 2, bg::cs::cartesian>;
  std::vector<std::pair<Point, size_t>> transformed;

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
    transformed.emplace_back(Point(origin.x(), origin.y()), i);
  }
  // Update the index
  bgi::rtree<std::pair<Point, size_t>, bgi::rstar<4>> rtree(transformed);
  auto cursor = transformed[start_index];

  // Search nearest neighbours
  std::vector<std::unique_ptr<ScanCell>> result;
  result.emplace_back(std::move(cells[cursor.second]));
  int64_t twl = 0;
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
    twl += std::llabs(boost::geometry::get<0>(cursor.first)
                      - boost::geometry::get<0>(next.first))
           + std::llabs(boost::geometry::get<1>(cursor.first)
                        - boost::geometry::get<1>(next.first));
    // Make sure we only visit things once
    rtree.remove(cursor);
    cursor = std::move(next);
    result.emplace_back(std::move(cells[cursor.second]));
  }
  // Replace with sorted vector
  std::swap(cells, result);

  return twl;
}

int64_t OptimizeScanWirelength2Opt(
    const odb::Point source,
    const odb::Point sink,
    std::vector<std::unique_ptr<ScanCell>>& cells,
    utl::Logger* logger,
    size_t max_iters)
{
  // Nothing to order
  if (cells.empty()) {
    return 0;
  }

  size_t N = cells.size() + 2;

  std::vector<odb::Point> points(N);
  size_t i = -1;
  points[++i] = source;

  int64_t score = 0;
  odb::Point last = points[i];
  for (const auto& cell : cells) {
    if (!cell->isPlaced()) {
      // No point running this if the cells aren't placed yet
      return 0;
    }

    points[++i] = cell->getOrigin();
    score += odb::Point::manhattanDistance(last, points[i]);
    last = points[i];
  }
  points[++i] = sink;
  score += odb::Point::manhattanDistance(last, sink);
  assert(++i == N);

  int64_t best_score = score;

  // Already "optimal" (unplaced)
  if (best_score == 0) {
    return 0;
  }

  size_t iters = 0;
  bool improved_last_iter = true;
  while (improved_last_iter && iters < max_iters) {
    iters += 1;
    debugPrint(logger,
               utl::DFT,
               "2opt",
               1,
               "Starting iteration {} (TWL: {})...",
               iters,
               best_score);
    improved_last_iter = false;
    for (size_t i = 0; i < N - 2; i += 1) {
      for (size_t j = i + 2; j < N - 1; j += 1) {
        auto&& x = points[i];
        auto&& y = points[j];
        auto&& u = points[i + 1];
        auto&& v = points[j + 1];
        auto score_d = -odb::Point::manhattanDistance(x, u)
                       - odb::Point::manhattanDistance(y, v)
                       + odb::Point::manhattanDistance(x, y)
                       + odb::Point::manhattanDistance(u, v);
        if (score_d < 0) {
          improved_last_iter = true;
          best_score += score_d;
          for (size_t k = 0; k < (j - i) / 2; k += 1) {
            std::swap(points[i + 1 + k], points[j - k]);
            std::swap(cells[i + k], cells[j - k - 1]);
          }
        }
      }
    }
  }

  if (improved_last_iter) {
    debugPrint(logger,
               utl::DFT,
               "2opt",
               1,
               "Stopping because the max iteration count ({}) was reached.",
               iters);
  } else {
    debugPrint(logger,
               utl::DFT,
               "2opt",
               1,
               "Stopping after {} iterations because of a lack of improvement.",
               iters);
  }

  return best_score;
}

}  // namespace dft
