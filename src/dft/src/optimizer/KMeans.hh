// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "ScanCell.hh"
#include "odb/geom.h"

namespace dft {

// Partition `cells` into k spatial clusters using Lloyd's algorithm with
// deterministic farthest-point seeding.  Returns an assignment vector where
// assignments[i] is the cluster index in [0, k) for cells[i].
//
// Capacity-aware: each cluster has an upper bound on the sum of cell bits.
// Pass cap_bits = std::numeric_limits<int64_t>::max() (the default) to run
// unconstrained vanilla Lloyd's.
//
// When a cap is in effect, every iteration uses a Bradley-Bennett-Demiriz
// style assignment: cells are processed in decreasing "confidence"
// (distance to 2nd-nearest minus distance to nearest), and each cell is
// placed in the closest cluster that still has capacity.  Cells in the
// core of a cluster (high confidence) are placed first; ambiguous
// boundary cells are pushed to a less-loaded cluster only when the
// preferred one is already full.  This keeps quality close to the
// unconstrained optimum while strictly honouring the bit cap.
//
// Fallback: if every cluster is at capacity (only possible with multi-bit
// cells that don't pack evenly), the cell goes to the least-loaded
// cluster — a soft cap rather than a hard error.
//
// Precondition: all cells must be placed (isPlaced() == true).
// Uses Manhattan distance throughout, consistent with the scan optimizer.
//
// Seeding is deterministic (farthest-point) rather than random so that
// OpenROAD runs remain reproducible across invocations.
//
// Degenerate cases:
//   k <= 1 or n <= k: all cells are assigned to cluster 0.
inline std::vector<int> KMeansClusters(
    const std::vector<ScanCell*>& cells,
    int k,
    int64_t cap_bits = std::numeric_limits<int64_t>::max(),
    int max_iters = 100)
{
  const int n = static_cast<int>(cells.size());

  if (k <= 1 || n <= k) {
    return std::vector<int>(n, 0);
  }

  // Cache cell origins and bit counts once.
  std::vector<odb::Point> pts(n);
  std::vector<int64_t> bits(n);
  for (int i = 0; i < n; i++) {
    pts[i] = cells[i]->getOrigin();
    bits[i] = static_cast<int64_t>(cells[i]->getBits());
  }

  // ---------------------------------------------------------------------------
  // Farthest-point seeding: pick k initial centroids spread across the die.
  // The first centroid is cells[0] (arbitrary but deterministic).  Each
  // subsequent centroid is the cell whose Manhattan distance to the nearest
  // existing centroid is largest.
  // ---------------------------------------------------------------------------
  std::vector<odb::Point> centroids;
  centroids.reserve(k);
  centroids.push_back(pts[0]);

  // seed_dist[i] = Manhattan distance from pts[i] to its nearest centroid so
  // far.  Reused only during seeding.
  std::vector<int64_t> seed_dist(n, std::numeric_limits<int64_t>::max());

  for (int c = 1; c < k; c++) {
    const odb::Point& new_cen = centroids.back();
    for (int i = 0; i < n; i++) {
      const int64_t d
          = std::abs(static_cast<int64_t>(pts[i].x()) - new_cen.x())
            + std::abs(static_cast<int64_t>(pts[i].y()) - new_cen.y());
      seed_dist[i] = std::min(seed_dist[i], d);
    }
    const int farthest = static_cast<int>(
        std::max_element(seed_dist.begin(), seed_dist.end())
        - seed_dist.begin());
    centroids.push_back(pts[farthest]);
  }

  // ---------------------------------------------------------------------------
  // Lloyd's algorithm with capacity-aware assignment.
  // ---------------------------------------------------------------------------
  std::vector<int> assignments(n, -1);

  // Per-cell sorted (distance, cluster) pairs, recomputed every iteration.
  std::vector<std::vector<std::pair<int64_t, int>>> ranked(n);
  for (int i = 0; i < n; i++) {
    ranked[i].reserve(k);
  }

  for (int iter = 0; iter < max_iters; iter++) {
    // For each cell, compute distances to every centroid and sort
    // ascending so we can try clusters in nearest-first order during
    // capacity-aware assignment.
    std::vector<int64_t> confidence(n, 0);
    for (int i = 0; i < n; i++) {
      ranked[i].clear();
      for (int c = 0; c < k; c++) {
        const int64_t d
            = std::abs(static_cast<int64_t>(pts[i].x()) - centroids[c].x())
              + std::abs(static_cast<int64_t>(pts[i].y()) - centroids[c].y());
        ranked[i].emplace_back(d, c);
      }
      std::sort(ranked[i].begin(), ranked[i].end());
      confidence[i] = ranked[i][1].first - ranked[i][0].first;
    }

    // Sort cell indices by descending confidence.  Ties broken by cell
    // index for determinism.  High-confidence cells claim their preferred
    // cluster first; low-confidence (boundary) cells get whatever
    // capacity remains.
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
      if (confidence[a] != confidence[b]) {
        return confidence[a] > confidence[b];
      }
      return a < b;
    });

    // Greedy capacity-aware assignment.
    std::vector<int64_t> cluster_bits(k, 0);
    std::vector<int> new_assignments(n, -1);
    bool changed = false;

    for (int i : order) {
      int chosen = -1;
      // Try clusters in nearest-first order; take the first one that
      // still has room for this cell's bits.
      for (const auto& [d, c] : ranked[i]) {
        (void) d;
        if (cluster_bits[c] + bits[i] <= cap_bits) {
          chosen = c;
          break;
        }
      }
      // Soft fallback: every cluster is full (possible when cells are
      // multi-bit and don't pack evenly).  Send the cell to whichever
      // cluster is currently least-loaded.
      if (chosen == -1) {
        chosen = static_cast<int>(
            std::min_element(cluster_bits.begin(), cluster_bits.end())
            - cluster_bits.begin());
      }
      new_assignments[i] = chosen;
      cluster_bits[chosen] += bits[i];
      if (new_assignments[i] != assignments[i]) {
        changed = true;
      }
    }
    assignments = std::move(new_assignments);

    if (!changed) {
      break;  // Converged.
    }

    // Update step: recompute each centroid as the mean of its assigned cells.
    std::vector<int64_t> sum_x(k, 0), sum_y(k, 0);
    std::vector<int> count(k, 0);
    for (int i = 0; i < n; i++) {
      sum_x[assignments[i]] += pts[i].x();
      sum_y[assignments[i]] += pts[i].y();
      count[assignments[i]]++;
    }
    for (int c = 0; c < k; c++) {
      if (count[c] > 0) {
        centroids[c] = odb::Point(static_cast<int>(sum_x[c] / count[c]),
                                   static_cast<int>(sum_y[c] / count[c]));
      }
      // Empty cluster: keep previous centroid to avoid degenerate collapse.
    }
  }

  return assignments;
}

}  // namespace dft
