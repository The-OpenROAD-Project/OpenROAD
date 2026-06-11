// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Opt.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
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
// Max number of cells we assume could be placed at the same point (hopefully,
// after DP, this is never more than one)
constexpr int kMaxCellsToSearch = 10;

// Number of nearest neighbours kept per cell for candidate-edge filtering.
// k = 50 is the conservative value recommended by Reinelt (Section 3, page 5).
constexpr int kMaxNeighbors = 50;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Weighted Manhattan distance between two points (int64 to avoid overflow).
int64_t Dist(const odb::Point& a,
                     const odb::Point& b)
{
  const int64_t dx = std::abs(static_cast<int64_t>(a.x()) - b.x());
  const int64_t dy = std::abs(static_cast<int64_t>(a.y()) - b.y());
  return dx + dy;
}

// ---------------------------------------------------------------------------
// k-Nearest-Neighbor candidate edges
//
// For each cell i we precompute the k nearest cells measured by the weighted
// distance from scan-out(i) to scan-in(neighbour).  The 2-Opt and 3-Opt inner
// loops iterate only over these candidates, reducing per-pass cost from O(n²)
// to O(n·k).
//
// The structure stores *cell identities* (indices into the original vector),
// not chain positions, so it survives local swaps without rebuild.  A separate
// pos[] array translates identity → current position.
// ---------------------------------------------------------------------------
struct CandidateEdges
{
  // neighbors[i] = vector of cell indices that are the k nearest neighbours
  // of cell i, measured by Dist(so[i], si[neighbour]).
  std::vector<std::vector<int>> neighbors;

  void build(const std::vector<odb::Point>& scan_in,
             const std::vector<odb::Point>& scan_out)
  {
    const int n = static_cast<int>(scan_in.size());
    neighbors.resize(n);

    // Build an R-tree of scan-in pin locations.
    using Point = bg::model::point<int, 2, bg::cs::cartesian>;
    std::vector<std::pair<Point, int>> indexed;
    indexed.reserve(n);
    for (int i = 0; i < n; i++) {
      indexed.emplace_back(Point(scan_in[i].x(), scan_in[i].y()), i);
    }
    bgi::rtree<std::pair<Point, int>, bgi::rstar<4>> rtree(indexed);

    // Over-query factor: R-tree returns by Euclidean proximity which may not
    // match weighted Manhattan.  Retrieve extra candidates and re-rank.
    const int query_k = std::min(n, kMaxNeighbors * 2);
    const int keep_k = std::min(n - 1, kMaxNeighbors);

    for (int i = 0; i < n; i++) {
      Point so_pt(scan_out[i].x(), scan_out[i].y());
      std::vector<std::pair<int64_t, int>> ranked;
      ranked.reserve(query_k);

      for (auto it = rtree.qbegin(bgi::nearest(so_pt, query_k));
           it != rtree.qend();
           ++it) {
        const int j = it->second;
        if (j == i) {
          continue;  // skip self
        }
        ranked.emplace_back(Dist(scan_out[i], scan_in[j]), j);
      }
      std::sort(ranked.begin(), ranked.end());
      const int limit
          = std::min(keep_k, static_cast<int>(ranked.size()));
      neighbors[i].reserve(limit);
      for (int r = 0; r < limit; r++) {
        neighbors[i].push_back(ranked[r].second);
      }
    }
  }
};

// ---------------------------------------------------------------------------
// 2-Opt scan chain ordering with asymmetric-distance correction.
//
// The scan chain is a directed path: each edge runs from cell[i].scanOut to
// cell[i+1].scanIn. Reversing a segment [i+1..j] changes the direction of
// every internal edge, incurring a "reversal cost" that the standard symmetric
// 2-Opt ignores.  We correct for this using a prefix-sum array RC[], where
//   revCost(i+1, j) = RC[j] - RC[i+1]
// is the net extra wire introduced by reversing those edges.
//
// The inner loop iterates only over k-NN candidate edges rather than all
// pairs.  Because the reversal cost can be negative, the g1 > 0 gain
// pruning used in symmetric TSP does NOT apply here — we must check every
// candidate (Section 3.1, page 8-9 of Boese et al. 1994).
//
// Algorithm: Boese, Kahng, Tsay, "Scan Chain Optimization: Heuristic and
// Optimal Solutions", 1994, Section 3.1.
// ---------------------------------------------------------------------------
void TwoOptScanChain(std::vector<std::unique_ptr<ScanCell>>& cells,
                     const CandidateEdges& candidates)
{
  const int n = static_cast<int>(cells.size());
  if (n < 4) {
    return;
  }

  // Cache scan-in / scan-out pin locations.
  std::vector<odb::Point> si(n), so(n);
  for (int i = 0; i < n; i++) {
    si[i] = cells[i]->getScanInLocation();
    so[i] = cells[i]->getScanOutLocation();
  }

  // D(i, j) = weighted distance from scan-out of cell i to scan-in of cell j.
  auto D = [&](int i, int j) -> int64_t {
    return Dist(so[i], si[j]);
  };

  // Identity ↔ position mapping.  identity[pos] = cell id, pos_of[id] = pos.
  std::vector<int> identity(n), pos_of(n);
  for (int i = 0; i < n; i++) {
    identity[i] = i;
    pos_of[i] = i;
  }

  // Build reversal-cost prefix array.
  std::vector<int64_t> RC(n, 0);
  auto rebuildRC = [&](int from) {
    for (int k = std::max(1, from); k < n; k++) {
      const int64_t rev = Dist(so[k], si[k - 1]);
      RC[k] = RC[k - 1] + (rev - D(k - 1, k));
    }
  };
  rebuildRC(1);

  // Main 2-Opt loop: restart from the beginning after each improvement.
  bool improved = true;
  while (improved) {
    improved = false;
    for (int i = 0; i < n - 3 && !improved; i++) {
      const int id_i1 = identity[i + 1];  // cell at position i+1

      // Iterate over k-NN of cell id_i1 (measured by so → si distance).
      for (int neighbor_id : candidates.neighbors[id_i1]) {
        const int j = pos_of[neighbor_id];
        // j must index the second removed edge (j, j+1), so j ∈ [i+2, n-2].
        if (j < i + 2 || j > n - 2) {
          continue;
        }
        // Remove edges (i→i+1) and (j→j+1).
        // Add    edges (i→j)   and (i+1→j+1).
        // Reverse segment [i+1..j].
        const int64_t gain = D(i, i + 1) + D(j, j + 1) - D(i, j)
                             - D(i + 1, j + 1) - (RC[j] - RC[i + 1]);
        if (gain > 0) {
          std::reverse(cells.begin() + i + 1, cells.begin() + j + 1);
          // Refresh pin-location cache and identity/pos arrays.
          for (int k = i + 1; k <= j; k++) {
            si[k] = cells[k]->getScanInLocation();
            so[k] = cells[k]->getScanOutLocation();
          }
          // The reversed segment swaps identities.
          for (int lo = i + 1, hi = j; lo < hi; lo++, hi--) {
            std::swap(identity[lo], identity[hi]);
          }
          for (int k = i + 1; k <= j; k++) {
            pos_of[identity[k]] = k;
          }
          rebuildRC(i + 1);
          improved = true;
          break;
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------
// 3-Opt scan chain ordering: direction-preserving subtour swap.
//
// Given positions i, j, k in the chain (i < j < k < n-1):
//   Remove edges (i, i+1), (j, j+1), (k, k+1).
//   Reconnect: ...i → j+1...k → i+1...j → k+1...
// This swaps subtours [i+1..j] and [j+1..k] without reversing either one
// (Figure 5b from Boese et al. 1994), so no reversal-cost correction is
// needed.
//
// Gain pruning IS valid here (no reversal cost):
//   g1 = D(i, i+1) - D(i, j+1)     must be > 0
//   g1 + g2 > 0  where g2 = D(j, j+1) - D(j, k+1)
//   total gain = g1 + g2 + g3  where g3 = D(k, k+1) - D(k, i+1)
// ---------------------------------------------------------------------------
void ThreeOptScanChain(std::vector<std::unique_ptr<ScanCell>>& cells,
                       const CandidateEdges& candidates)
{
  const int n = static_cast<int>(cells.size());
  if (n < 6) {
    return;
  }

  std::vector<odb::Point> si(n), so(n);
  for (int i = 0; i < n; i++) {
    si[i] = cells[i]->getScanInLocation();
    so[i] = cells[i]->getScanOutLocation();
  }

  auto D = [&](int a, int b) -> int64_t {
    return Dist(so[a], si[b]);
  };

  // Build a map from ScanCell pointer → original candidate index so we can
  // recover identity after rotates.
  std::unordered_map<const ScanCell*, int> ptr_to_id;
  for (int i = 0; i < n; i++) {
    ptr_to_id[cells[i].get()] = i;
  }

  // identity[pos] = original candidate index of cell at this position.
  // pos_of[id]    = current position of the cell with original index id.
  std::vector<int> identity(n), pos_of(n);
  auto rebuildIdentity = [&]() {
    for (int i = 0; i < n; i++) {
      identity[i] = ptr_to_id[cells[i].get()];
      pos_of[identity[i]] = i;
    }
  };
  rebuildIdentity();

  bool improved = true;
  while (improved) {
    improved = false;
    for (int i = 0; i < n - 5 && !improved; i++) {
      const int64_t d_i = D(i, i + 1);
      const int id_i = identity[i];

      // Middle loop: after the swap, edge i → j+1 is created, so j+1 should
      // be a neighbour of cell at position i.
      for (int nb_id : candidates.neighbors[id_i]) {
        const int j1_pos = pos_of[nb_id];
        const int j = j1_pos - 1;
        if (j < i + 2 || j >= n - 3) {
          continue;
        }
        const int64_t g1 = d_i - D(i, j + 1);
        if (g1 <= 0) {
          continue;
        }

        const int64_t d_j = D(j, j + 1);
        const int id_j = identity[j];

        // Inner loop: after the swap, edge j → k+1 is created.
        for (int nb2_id : candidates.neighbors[id_j]) {
          const int k1_pos = pos_of[nb2_id];
          const int k = k1_pos - 1;
          if (k < j + 2 || k >= n - 1) {
            continue;
          }
          const int64_t g2 = d_j - D(j, k + 1);
          if (g1 + g2 <= 0) {
            continue;
          }
          const int64_t g3 = D(k, k + 1) - D(k, i + 1);
          const int64_t gain = g1 + g2 + g3;
          if (gain > 0) {
            // Swap subtours [i+1..j] and [j+1..k] via rotate.
            std::rotate(cells.begin() + i + 1,
                        cells.begin() + j + 1,
                        cells.begin() + k + 1);
            // Refresh caches for the affected range.
            for (int m = i + 1; m <= k; m++) {
              si[m] = cells[m]->getScanInLocation();
              so[m] = cells[m]->getScanOutLocation();
            }
            // Rebuild identity/pos for affected range.
            for (int m = i + 1; m <= k; m++) {
              identity[m] = ptr_to_id[cells[m].get()];
              pos_of[identity[m]] = m;
            }
            improved = true;
            break;
          }
        }
        if (improved) {
          break;
        }
      }
    }
  }
}

}  // namespace (anonymous)

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
          14,
          "Couldn't find nearest neighbor, too many overlapping cells");
    }
    // Make sure we only visit things once
    rtree.remove(cursor);
    cursor = std::move(next);
    result.emplace_back(std::move(cells[cursor.second]));
  }
  // Replace with sorted vector
  std::swap(cells, result);

  // Build k-NN candidate edges for the local-search passes.
  const int n = static_cast<int>(cells.size());
  std::vector<odb::Point> si(n), so(n);
  for (int i = 0; i < n; i++) {
    si[i] = cells[i]->getScanInLocation();
    so[i] = cells[i]->getScanOutLocation();
  }
  CandidateEdges candidates;
  candidates.build(si, so);

  // Improve the nearest-neighbor ordering with local search.
  // 2-Opt corrects for asymmetric scan-in/scan-out pin offsets.
  // 3-Opt (subtour swap) catches direction-preserving improvements.
  TwoOptScanChain(cells, candidates);
  ThreeOptScanChain(cells, candidates);
}

}  // namespace dft
