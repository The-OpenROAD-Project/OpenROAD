// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Opt.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
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

// Number of nearest neighbors kept per cell for candidate-edge filtering.
// k = 50 is the conservative value recommended by Reinelt (Section 3, page 5).
constexpr int kMaxNeighbors = 50;

// Max number of cells kept in each reverse-neighbor list.  It bounds the
// positions a move marks when many cells list the same cell, as when cells
// overlap.
constexpr int kMaxReverseNeighbors = 2 * kMaxNeighbors;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Manhattan distance between two points (int64 to avoid overflow).
int64_t Dist(const odb::Point& a, const odb::Point& b)
{
  const int64_t dx = std::abs(static_cast<int64_t>(a.x()) - b.x());
  const int64_t dy = std::abs(static_cast<int64_t>(a.y()) - b.y());
  return dx + dy;
}

// ---------------------------------------------------------------------------
// k-Nearest-Neighbor candidate edges
//
// For each cell i we precompute the k nearest cells measured by the
// distance from scan-out(i) to scan-in(neighbor).  The 2-Opt and 3-Opt inner
// loops iterate only over these candidates, reducing per-pass cost from O(n²)
// to O(n·k).
//
// The structure stores *cell identities* (indices into the original vector),
// not chain positions, so it survives local swaps without rebuild.  A separate
// pos[] array translates identity → current position.
// ---------------------------------------------------------------------------
struct CandidateEdges
{
  // neighbors[i] = vector of cell indices that are the k nearest neighbors
  // of cell i, measured by Dist(so[i], si[neighbor]).
  std::vector<std::vector<int>> neighbors;
  // reverse_neighbors[j] = cells that list j among their neighbors, at most
  // kMaxReverseNeighbors of them, preferring those that rank j nearest.
  std::vector<std::vector<int>> reverse_neighbors;

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
    const bgi::rtree<std::pair<Point, int>, bgi::rstar<4>> rtree(indexed);

    // Over-query factor: R-tree returns by Euclidean proximity which may not
    // match Manhattan.  Retrieve extra candidates and re-rank.
    const int query_k = std::min(n, kMaxNeighbors * 2);
    const int keep_k = std::min(n - 1, kMaxNeighbors);

    for (int i = 0; i < n; i++) {
      const Point so_pt(scan_out[i].x(), scan_out[i].y());
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
      const int limit = std::min(keep_k, static_cast<int>(ranked.size()));
      neighbors[i].reserve(limit);
      for (int r = 0; r < limit; r++) {
        neighbors[i].push_back(ranked[r].second);
      }
    }

    // (rank of j in neighbors[i], i) for each cell j.
    std::vector<std::vector<std::pair<int, int>>> listed_by(n);
    for (int i = 0; i < n; i++) {
      for (int rank = 0; rank < static_cast<int>(neighbors[i].size()); rank++) {
        listed_by[neighbors[i][rank]].emplace_back(rank, i);
      }
    }
    reverse_neighbors.resize(n);
    for (int j = 0; j < n; j++) {
      auto& listers = listed_by[j];
      if (static_cast<int>(listers.size()) > kMaxReverseNeighbors) {
        std::nth_element(listers.begin(),
                         listers.begin() + kMaxReverseNeighbors,
                         listers.end());
        listers.resize(kMaxReverseNeighbors);
      }
      reverse_neighbors[j].reserve(listers.size());
      for (const auto& [rank, i] : listers) {
        reverse_neighbors[j].push_back(i);
      }
    }
  }
};

// Sums over a range of per-edge values with point updates (a Fenwick tree),
// so a 2-Opt move updates only the edges it changes.
class EdgeSums
{
 public:
  // Starts from values, in O(n).
  explicit EdgeSums(const std::vector<int64_t>& values)
      : tree_(values.size() + 1, 0)
  {
    const int size = static_cast<int>(tree_.size());
    for (int i = 1; i < size; i++) {
      tree_[i] += values[i - 1];
      const int parent = i + (i & -i);
      if (parent < size) {
        tree_[parent] += tree_[i];
      }
    }
  }

  void add(int index, const int64_t delta)
  {
    for (index++; index < static_cast<int>(tree_.size());
         index += index & -index) {
      tree_[index] += delta;
    }
  }

  // Sum of the values at [first, last]; 0 when the range is empty.
  int64_t range(const int first, const int last) const
  {
    return first > last ? 0 : prefix(last) - prefix(first - 1);
  }

 private:
  // Sum of the values at [0, index].
  int64_t prefix(int index) const
  {
    int64_t sum = 0;
    for (index++; index > 0; index -= index & -index) {
      sum += tree_[index];
    }
    return sum;
  }

  std::vector<int64_t> tree_;
};

// The chain order shared by the 2-Opt and 3-Opt passes.  identity[pos] is
// the node id at each position and pos_of[id] its position; si[pos] and
// so[pos] are the scan-in and scan-out pins of the node at pos.  The first
// and last positions never move.
struct Chain
{
  Chain(std::vector<int> order,
        const std::vector<odb::Point>& si_of_id,
        const std::vector<odb::Point>& so_of_id)
      : identity(std::move(order)),
        pos_of(si_of_id.size()),
        si(identity.size()),
        so(identity.size())
  {
    for (int pos = 0; pos < size(); pos++) {
      const int id = identity[pos];
      pos_of[id] = pos;
      si[pos] = si_of_id[id];
      so[pos] = so_of_id[id];
    }
  }

  int size() const { return static_cast<int>(identity.size()); }

  std::vector<int> identity;
  std::vector<int> pos_of;
  std::vector<odb::Point> si;
  std::vector<odb::Point> so;
};

// Runs a first-improvement local search over positions [0, last].
// try_move(i, mark) applies an improving move anchored at position i, if
// any, and calls mark(p) for positions p the move most likely made
// improvable.  Marked positions are tried lowest first.  The marks can miss
// a position a move made improvable, so full passes over every position
// repeat until one makes no move, and the result is a local optimum.
template <typename TryMove>
bool RunLocalSearch(const int last, const TryMove& try_move)
{
  if (last < 0) {
    return false;
  }
  std::set<int> dirty;
  const auto mark = [last, &dirty](const int p) {
    if (p >= 0 && p <= last) {
      dirty.insert(p);
    }
  };
  bool any_move = false;
  bool improved = true;
  while (improved) {
    improved = false;
    for (int i = 0; i <= last; i++) {
      dirty.insert(dirty.end(), i);
    }
    while (!dirty.empty()) {
      const int i = *dirty.begin();
      dirty.erase(dirty.begin());
      if (try_move(i, mark)) {
        improved = true;
        any_move = true;
      }
    }
  }
  return any_move;
}

// ---------------------------------------------------------------------------
// 2-Opt scan chain ordering with asymmetric-distance correction.
//
// The scan chain is a directed path: each edge runs from cell[i].scanOut to
// cell[i+1].scanIn. Reversing a segment [i+1..j] changes the direction of
// every internal edge, incurring a "reversal cost" that the standard symmetric
// 2-Opt ignores.  rev[k] is the extra wire when edge (k-1, k) runs backward,
// and the reversal cost of [i+1..j] is the sum of rev[i+2..j].
//
// The inner loop iterates only over k-NN candidate edges rather than all
// pairs.  Because the reversal cost can be negative, the g1 > 0 gain
// pruning used in symmetric TSP does NOT apply here — we must check every
// candidate (Section 3.1, page 8-9 of Boese et al. 1994).
//
// RunLocalSearch chooses the positions to try.  A move updates only the
// edges of the reversed segment, so its cost is the segment length.
//
// Algorithm: Boese, Kahng, Tsay, "Scan Chain Optimization: Heuristic and
// Optimal Solutions", 1994, Section 3.1.
// ---------------------------------------------------------------------------
//
bool TwoOptScanChain(Chain& chain, const CandidateEdges& candidates)
{
  const int n = chain.size();
  if (n < 4) {
    return false;
  }
  std::vector<int>& identity = chain.identity;
  std::vector<int>& pos_of = chain.pos_of;
  std::vector<odb::Point>& si = chain.si;
  std::vector<odb::Point>& so = chain.so;

  // D(i, j) = Manhattan distance from scan-out of cell i to scan-in of cell j.
  const auto D = [&](int i, int j) -> int64_t { return Dist(so[i], si[j]); };

  // rev[k] is the extra wire when edge (k-1, k) runs backward.
  std::vector<int64_t> rev(n, 0);
  for (int k = 1; k < n; k++) {
    rev[k] = Dist(so[k], si[k - 1]) - D(k - 1, k);
  }
  EdgeSums rev_sums(rev);
  const auto update_rev = [&](const int k) {
    const int64_t value = Dist(so[k], si[k - 1]) - D(k - 1, k);
    rev_sums.add(k, value - rev[k]);
    rev[k] = value;
  };

  // Applies the first improving move that reconnects position i, if any.
  const auto try_move = [&](const int i, const auto& mark) {
    const int id_i1 = identity[i + 1];  // cell at position i+1

    // Iterate over k-NN of cell id_i1 (measured by so → si distance).
    for (const int neighbor_id : candidates.neighbors[id_i1]) {
      const int j = pos_of[neighbor_id];
      // j must index the second removed edge (j, j+1), so j ∈ [i+2, n-2].
      if (j < i + 2 || j > n - 2) {
        continue;
      }
      // Remove edges (i→i+1) and (j→j+1).
      // Add    edges (i→j)   and (i+1→j+1).
      // Reverse segment [i+1..j].
      const int64_t gain = D(i, i + 1) + D(j, j + 1) - D(i, j) - D(i + 1, j + 1)
                           - rev_sums.range(i + 2, j);
      if (gain > 0) {
        std::reverse(si.begin() + i + 1, si.begin() + j + 1);
        std::reverse(so.begin() + i + 1, so.begin() + j + 1);
        std::reverse(identity.begin() + i + 1, identity.begin() + j + 1);
        for (int k = i + 1; k <= j; k++) {
          pos_of[identity[k]] = k;
        }
        for (int k = i + 1; k <= j + 1; k++) {
          update_rev(k);
        }
        // Moves anchored at a changed edge, or at a cell that lists an end
        // of a changed edge as a neighbor, may now improve.
        for (const int p : {i, i + 1, j, j + 1}) {
          mark(p);
          mark(p - 1);
          for (const int r : candidates.reverse_neighbors[identity[p]]) {
            mark(pos_of[r] - 1);
          }
        }
        return true;
      }
    }
    return false;
  };

  return RunLocalSearch(n - 4, try_move);
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
bool ThreeOptScanChain(Chain& chain, const CandidateEdges& candidates)
{
  const int n = chain.size();
  if (n < 6) {
    return false;
  }
  std::vector<int>& identity = chain.identity;
  std::vector<int>& pos_of = chain.pos_of;
  std::vector<odb::Point>& si = chain.si;
  std::vector<odb::Point>& so = chain.so;

  const auto D = [&](int a, int b) -> int64_t { return Dist(so[a], si[b]); };

  // Applies the first improving move that reconnects position i, if any.
  const auto try_move = [&](const int i, const auto& mark) {
    const int64_t d_i = D(i, i + 1);
    const int id_i = identity[i];

    // Middle loop: after the swap, edge i → j+1 is created, so j+1 should
    // be a neighbor of cell at position i.
    for (const int nb_id : candidates.neighbors[id_i]) {
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
      for (const int nb2_id : candidates.neighbors[id_j]) {
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
          std::rotate(
              si.begin() + i + 1, si.begin() + j + 1, si.begin() + k + 1);
          std::rotate(
              so.begin() + i + 1, so.begin() + j + 1, so.begin() + k + 1);
          std::rotate(identity.begin() + i + 1,
                      identity.begin() + j + 1,
                      identity.begin() + k + 1);
          for (int m = i + 1; m <= k; m++) {
            pos_of[identity[m]] = m;
          }
          // Moves anchored at a changed edge, or at a cell that lists an
          // end of a changed edge as a neighbor, may now improve.
          for (const int p : {i, i + 1, i + k - j, i + k - j + 1, k, k + 1}) {
            mark(p);
            mark(p - 1);
            for (const int r : candidates.reverse_neighbors[identity[p]]) {
              mark(pos_of[r]);
            }
          }
          return true;
        }
      }
    }
    return false;
  };

  return RunLocalSearch(n - 6, try_move);
}

}  // namespace

void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger,
                            const std::optional<odb::Point>& source,
                            const std::optional<odb::Point>& sink)
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

  // Start from the cell nearest the source. Without a source, start from the
  // lower leftmost cell so we don't accidentally start in the middle.
  size_t start_index = 0;
  int64_t lowest_dist = std::numeric_limits<int64_t>::max();
  // Get points in a form ready to insert into index
  using Point = bg::model::point<int, 2, bg::cs::cartesian>;
  std::vector<std::pair<Point, size_t>> transformed;

  for (size_t i = 0; i < cells.size(); i++) {
    const auto origin = cells[i]->getOrigin();
    const int64_t dist = source ? Dist(*source, cells[i]->getScanInLocation())
                                : int64_t{origin.x()} + origin.y();
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

  // Search nearest neighbors
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

  // Node ids 0..n-1 are the cells. The source and sink, when given, are extra
  // nodes pinned to the ends of the order, so the local search accounts for
  // the wire into the first cell and out of the last one.
  const int n = static_cast<int>(cells.size());
  std::vector<odb::Point> si(n), so(n);
  for (int i = 0; i < n; i++) {
    si[i] = cells[i]->getScanInLocation();
    so[i] = cells[i]->getScanOutLocation();
  }
  std::vector<int> order;
  order.reserve(n + 2);
  if (source) {
    order.push_back(static_cast<int>(si.size()));
    si.push_back(*source);
    so.push_back(*source);
  }
  for (int i = 0; i < n; i++) {
    order.push_back(i);
  }
  if (sink) {
    order.push_back(static_cast<int>(si.size()));
    si.push_back(*sink);
    so.push_back(*sink);
  }

  // Build k-NN candidate edges for the local-search passes.
  CandidateEdges candidates;
  candidates.build(si, so);

  // Improve the nearest-neighbor ordering with local search.
  // 2-Opt corrects for asymmetric scan-in/scan-out pin offsets.
  // 3-Opt (subtour swap) catches direction-preserving improvements.
  // Alternate the two until one finds no move, as each can open moves for
  // the other.
  Chain chain(std::move(order), si, so);
  TwoOptScanChain(chain, candidates);
  while (ThreeOptScanChain(chain, candidates)) {
    if (!TwoOptScanChain(chain, candidates)) {
      break;
    }
  }

  std::vector<std::unique_ptr<ScanCell>> ordered;
  ordered.reserve(n);
  for (const int id : chain.identity) {
    if (id < n) {
      ordered.push_back(std::move(cells[id]));
    }
  }
  std::swap(cells, ordered);
}

}  // namespace dft
