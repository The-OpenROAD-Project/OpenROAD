// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Opt.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "utl/Logger.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace dft {

namespace {
// Manhattan nearest-neighbor scan (O(n^2)) is exact but quadratic; keep it for
// moderately sized chains.
constexpr std::size_t kQuadraticHeuristicMaxCells = 10000;

// Euclidean rtree nearest query candidate count (used as a fallback for large
// chains where quadratic heuristics are too expensive).
constexpr std::size_t kNearestCandidateCount = 256;

// Bound the runtime of 2-opt (O(n^2)). The quadratic local-search is only used
// on smaller chains.
constexpr std::size_t kTwoOptMaxCellsFor1Pass = 12000;
constexpr std::size_t kTwoOptMaxCellsFor2Passes = 8000;
constexpr std::size_t kTwoOptMaxCellsFor3Passes = 4000;

constexpr std::size_t kFarthestInsertionMaxCells = kQuadraticHeuristicMaxCells;
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

  const auto manhattan
      = [](const odb::Point& a, const odb::Point& b) -> int64_t {
    const int64_t dx
        = static_cast<int64_t>(a.x()) - static_cast<int64_t>(b.x());
    const int64_t dy
        = static_cast<int64_t>(a.y()) - static_cast<int64_t>(b.y());
    return std::abs(dx) + std::abs(dy);
  };

  const auto path_len = [&](const std::vector<std::size_t>& order,
                            const std::vector<odb::Point>& origins) -> int64_t {
    int64_t total = 0;
    for (std::size_t i = 1; i < order.size(); ++i) {
      total += manhattan(origins[order[i - 1]], origins[order[i]]);
    }
    return total;
  };

  const auto no_next_scan_cell = [&]() -> void {
    logger->error(utl::DFT, 16, "Couldn't find next scan cell to order");
  };

  const std::size_t n = cells.size();
  std::vector<odb::Point> origins;
  origins.reserve(n);
  std::vector<std::string_view> names;
  names.reserve(n);
  for (const auto& cell : cells) {
    origins.emplace_back(cell->getOrigin());
    names.emplace_back(cell->getName());
  }

  // Define the starting node as the lower leftmost, so we don't accidentally
  // start somewhere in the middle
  size_t start_index = 0;
  int64_t lowest_dist = std::numeric_limits<int64_t>::max();

  for (size_t i = 0; i < n; i++) {
    // Find the lower leftmost cell by looking for the cell with the lowest
    // manhattan distance to the origin.
    const auto& origin = origins[i];
    const int64_t dist
        = static_cast<int64_t>(origin.x()) + static_cast<int64_t>(origin.y());
    if (dist < lowest_dist
        || (dist == lowest_dist && names[i] < names[start_index])) {
      start_index = i;
      lowest_dist = dist;
    }
  }

  const auto greedy_nn_order
      = [&](std::size_t start) -> std::vector<std::size_t> {
    std::vector<std::size_t> order;
    order.reserve(n);
    std::vector<bool> used(n, false);

    std::size_t cur = start;
    used[cur] = true;
    order.push_back(cur);

    while (order.size() < n) {
      bool found = false;
      std::size_t best = cur;
      int64_t best_dist = std::numeric_limits<int64_t>::max();
      for (std::size_t i = 0; i < n; ++i) {
        if (used[i]) {
          continue;
        }
        const int64_t dist = manhattan(origins[cur], origins[i]);
        if (!found || dist < best_dist
            || (dist == best_dist && names[i] < names[best])) {
          best = i;
          best_dist = dist;
          found = true;
        }
      }
      if (!found) {
        no_next_scan_cell();
      }
      used[best] = true;
      cur = best;
      order.push_back(cur);
    }
    return order;
  };

  const auto farthest_insertion_order
      = [&](std::size_t start) -> std::vector<std::size_t> {
    std::vector<std::size_t> order;
    order.reserve(n);
    std::vector<bool> in_path(n, false);

    order.push_back(start);
    in_path[start] = true;

    if (n == 1) {
      return order;
    }

    // Seed the path with the cell farthest from the start so the initial
    // segment spans the placement.
    std::size_t farthest = start;
    int64_t farthest_dist = -1;
    for (std::size_t i = 0; i < n; ++i) {
      if (i == start) {
        continue;
      }
      const int64_t dist = manhattan(origins[start], origins[i]);
      if (dist > farthest_dist
          || (dist == farthest_dist && names[i] < names[farthest])) {
        farthest = i;
        farthest_dist = dist;
      }
    }

    order.push_back(farthest);
    in_path[farthest] = true;

    std::vector<int64_t> nearest_dist(n, std::numeric_limits<int64_t>::max());
    for (std::size_t i = 0; i < n; ++i) {
      if (in_path[i]) {
        nearest_dist[i] = 0;
        continue;
      }
      nearest_dist[i] = std::min(manhattan(origins[i], origins[start]),
                                 manhattan(origins[i], origins[farthest]));
    }

    while (order.size() < n) {
      // Pick the cell farthest from the current path (farthest insertion).
      std::size_t next = start;
      bool found = false;
      int64_t best_score = -1;
      for (std::size_t i = 0; i < n; ++i) {
        if (in_path[i]) {
          continue;
        }
        const int64_t score = nearest_dist[i];
        if (!found || score > best_score
            || (score == best_score && names[i] < names[next])) {
          next = i;
          best_score = score;
          found = true;
        }
      }
      if (!found) {
        no_next_scan_cell();
      }

      // Insert it at the position that minimises the path length increase.
      std::size_t best_pos = order.size();  // append by default
      int64_t best_delta = manhattan(origins[order.back()], origins[next]);

      for (std::size_t pos = 0; pos + 1 < order.size(); ++pos) {
        const auto a = order[pos];
        const auto b = order[pos + 1];
        const int64_t delta = manhattan(origins[a], origins[next])
                              + manhattan(origins[next], origins[b])
                              - manhattan(origins[a], origins[b]);
        if (delta < best_delta || (delta == best_delta && pos + 1 < best_pos)) {
          best_delta = delta;
          best_pos = pos + 1;
        }
      }

      order.insert(order.begin() + static_cast<std::ptrdiff_t>(best_pos), next);
      in_path[next] = true;

      // Update the nearest distance cache for remaining nodes.
      for (std::size_t i = 0; i < n; ++i) {
        if (in_path[i]) {
          continue;
        }
        const int64_t dist = manhattan(origins[i], origins[next]);
        if (dist < nearest_dist[i]) {
          nearest_dist[i] = dist;
        }
      }
    }

    // Keep the start fixed as the first element.
    if (!order.empty() && order.front() != start) {
      const auto it = std::find(order.begin(), order.end(), start);
      if (it != order.end()) {
        std::rotate(order.begin(), it, order.end());
      }
    }

    return order;
  };

  const auto two_opt_improve
      = [&](std::vector<std::size_t>& order, int max_passes) -> void {
    if (max_passes <= 0 || order.size() < 4) {
      return;
    }

    const int64_t before = path_len(order, origins);
    bool improved_any = false;

    for (int pass = 0; pass < max_passes; ++pass) {
      bool pass_improved = false;
      for (std::size_t i = 0; i + 3 < order.size(); ++i) {
        for (std::size_t j = i + 2; j + 1 < order.size(); ++j) {
          const auto a = order[i];
          const auto b = order[i + 1];
          const auto c = order[j];
          const auto d = order[j + 1];
          const int64_t old_cost = manhattan(origins[a], origins[b])
                                   + manhattan(origins[c], origins[d]);
          const int64_t new_cost = manhattan(origins[a], origins[c])
                                   + manhattan(origins[b], origins[d]);
          if (new_cost < old_cost) {
            std::reverse(order.begin() + static_cast<std::ptrdiff_t>(i + 1),
                         order.begin() + static_cast<std::ptrdiff_t>(j + 1));
            pass_improved = true;
            improved_any = true;
          }
        }
      }
      if (!pass_improved) {
        break;
      }
    }

    if (improved_any) {
      const int64_t after = path_len(order, origins);
      debugPrint(logger,
                 utl::DFT,
                 "scan_chain_opt",
                 1,
                 "OptimizeScanWirelength: 2-opt improved path length {} -> {} "
                 "({} cells)",
                 before,
                 after,
                 order.size());
    }
  };

  int two_opt_passes = 0;
  if (n <= kTwoOptMaxCellsFor3Passes) {
    two_opt_passes = 3;
  } else if (n <= kTwoOptMaxCellsFor2Passes) {
    two_opt_passes = 2;
  } else if (n <= kTwoOptMaxCellsFor1Pass) {
    two_opt_passes = 1;
  }

  // Quadratic heuristics for small/medium chains.
  if (n <= kQuadraticHeuristicMaxCells) {
    std::vector<std::size_t> best_order = greedy_nn_order(start_index);
    int64_t best_cost = path_len(best_order, origins);
    two_opt_improve(best_order, two_opt_passes);
    best_cost = path_len(best_order, origins);

    if (n <= kFarthestInsertionMaxCells) {
      std::vector<std::size_t> fi_order = farthest_insertion_order(start_index);
      two_opt_improve(fi_order, two_opt_passes);
      const int64_t fi_cost = path_len(fi_order, origins);
      if (fi_cost < best_cost) {
        best_cost = fi_cost;
        best_order = std::move(fi_order);
      }
    }

    std::vector<std::unique_ptr<ScanCell>> ordered;
    ordered.reserve(n);
    for (const std::size_t idx : best_order) {
      ordered.emplace_back(std::move(cells[idx]));
    }
    std::swap(cells, ordered);
    return;
  }

  // Fallback for large chains: rtree greedy ordering with a Manhattan-aware
  // choice among the nearest Euclidean candidates.
  // Get points in a form ready to insert into index
  using Point = bg::model::point<int, 2, bg::cs::cartesian>;
  std::vector<std::pair<Point, size_t>> transformed;

  for (size_t i = 0; i < n; i++) {
    const auto& origin = origins[i];
    transformed.emplace_back(Point(origin.x(), origin.y()), i);
  }
  // Update the index
  bgi::rtree<std::pair<Point, size_t>, bgi::rstar<4>> rtree(transformed);
  auto cursor = transformed[start_index];

  // Search nearest neighbours. The rtree nearest search is Euclidean, so we
  // evaluate a handful of nearest Euclidean candidates and pick the best by
  // Manhattan distance (the scan-chain cost proxy we care about).
  std::vector<std::unique_ptr<ScanCell>> ordered;
  ordered.reserve(cells.size());

  ordered.emplace_back(std::move(cells[cursor.second]));
  rtree.remove(cursor);

  while (ordered.size() < cells.size()) {
    bool found = false;
    std::pair<Point, size_t> best = cursor;
    int64_t best_dist = std::numeric_limits<int64_t>::max();
    odb::Point cursor_pt(bg::get<0>(cursor.first), bg::get<1>(cursor.first));

    for (auto it
         = rtree.qbegin(bgi::nearest(cursor.first, kNearestCandidateCount));
         it != rtree.qend();
         ++it) {
      const auto cand = *it;
      odb::Point cand_pt(bg::get<0>(cand.first), bg::get<1>(cand.first));
      const int64_t dist = manhattan(cursor_pt, cand_pt);
      if (!found || dist < best_dist
          || (dist == best_dist && cand.second < best.second)) {
        best = cand;
        best_dist = dist;
        found = true;
      }
    }

    if (!found) {
      no_next_scan_cell();
    }

    cursor = best;
    ordered.emplace_back(std::move(cells[cursor.second]));
    rtree.remove(cursor);
  }

  // Replace with ordered vector.
  std::swap(cells, ordered);
}

}  // namespace dft
