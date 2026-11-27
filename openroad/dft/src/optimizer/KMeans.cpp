// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "KMeans.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "ScanCell.hh"
#include "odb/geom.h"

namespace dft {

namespace {

// Farthest-point seeding: picks k initial centroids spread across the die.
// The first centroid is pts[0] (arbitrary but deterministic).  Each
// subsequent centroid is the point whose Manhattan distance to the nearest
// existing centroid is largest.
std::vector<odb::Point> SeedCentroids(const std::vector<odb::Point>& pts,
                                      const int k)
{
  std::vector<odb::Point> centroids;
  centroids.reserve(k);
  centroids.push_back(pts[0]);

  // seed_dist[i] = Manhattan distance from pts[i] to its nearest centroid so
  // far.
  std::vector<int64_t> seed_dist(pts.size(),
                                 std::numeric_limits<int64_t>::max());

  for (int c = 1; c < k; c++) {
    const odb::Point& new_cen = centroids.back();
    for (size_t i = 0; i < pts.size(); i++) {
      seed_dist[i] = std::min(seed_dist[i],
                              odb::Point::manhattanDistance(pts[i], new_cen));
    }
    const auto farthest
        = std::ranges::max_element(seed_dist) - seed_dist.begin();
    centroids.push_back(pts[farthest]);
  }
  return centroids;
}

// Assigns each point to the closest centroid that still has room for its
// bits, most confident points first.  ranked is per-point scratch space
// reused across calls.  Returns true if any assignment changed.
bool AssignWithCapacity(
    const std::vector<odb::Point>& pts,
    const std::vector<int64_t>& bits,
    const std::vector<odb::Point>& centroids,
    const int64_t cap_bits,
    std::vector<std::vector<std::pair<int64_t, int>>>& ranked,
    std::vector<int>& assignments)
{
  const int n = static_cast<int>(pts.size());
  const int k = static_cast<int>(centroids.size());

  // For each point, compute distances to every centroid and sort ascending
  // so we can try clusters in nearest-first order.
  std::vector<int64_t> confidence(n, 0);
  for (int i = 0; i < n; i++) {
    ranked[i].clear();
    for (int c = 0; c < k; c++) {
      ranked[i].emplace_back(
          odb::Point::manhattanDistance(pts[i], centroids[c]), c);
    }
    std::ranges::sort(ranked[i]);
    confidence[i] = ranked[i][1].first - ranked[i][0].first;
  }

  // Sort point indices by descending confidence.  Ties broken by index for
  // determinism.  High-confidence points claim their preferred cluster
  // first; low-confidence (boundary) points get whatever capacity remains.
  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::ranges::sort(order, [&](const int a, const int b) {
    if (confidence[a] != confidence[b]) {
      return confidence[a] > confidence[b];
    }
    return a < b;
  });

  std::vector<int64_t> cluster_bits(k, 0);
  std::vector<int> new_assignments(n, -1);
  bool changed = false;

  for (const int i : order) {
    int chosen = -1;
    // Try clusters in nearest-first order; take the first one that still
    // has room for this point's bits.
    for (const auto& [d, c] : ranked[i]) {
      if (cluster_bits[c] + bits[i] <= cap_bits) {
        chosen = c;
        break;
      }
    }
    // Soft fallback: every cluster is full (possible when cells are
    // multi-bit and don't pack evenly).  Send the point to whichever
    // cluster is currently least-loaded.
    if (chosen == -1) {
      chosen = static_cast<int>(std::ranges::min_element(cluster_bits)
                                - cluster_bits.begin());
    }
    new_assignments[i] = chosen;
    cluster_bits[chosen] += bits[i];
    if (new_assignments[i] != assignments[i]) {
      changed = true;
    }
  }
  assignments = std::move(new_assignments);
  return changed;
}

// Moves each centroid to the mean of its assigned points.  An empty
// cluster keeps its previous centroid to avoid degenerate collapse.
void UpdateCentroids(const std::vector<odb::Point>& pts,
                     const std::vector<int>& assignments,
                     std::vector<odb::Point>& centroids)
{
  const int k = static_cast<int>(centroids.size());
  std::vector<int64_t> sum_x(k, 0);
  std::vector<int64_t> sum_y(k, 0);
  std::vector<int> count(k, 0);
  for (size_t i = 0; i < pts.size(); i++) {
    sum_x[assignments[i]] += pts[i].x();
    sum_y[assignments[i]] += pts[i].y();
    count[assignments[i]]++;
  }
  for (int c = 0; c < k; c++) {
    if (count[c] > 0) {
      centroids[c] = odb::Point(static_cast<int>(sum_x[c] / count[c]),
                                static_cast<int>(sum_y[c] / count[c]));
    }
  }
}

}  // namespace

std::vector<int> KMeansClusters(const std::vector<ScanCell*>& cells,
                                const int k,
                                const int64_t cap_bits,
                                const int max_iters)
{
  const int n = static_cast<int>(cells.size());

  if (k <= 1) {
    return std::vector<int>(n, 0);
  }
  if (n <= k) {
    std::vector<int> assignments(n);
    std::iota(assignments.begin(), assignments.end(), 0);
    return assignments;
  }

  // Cache cell origins and bit counts once.
  std::vector<odb::Point> pts(n);
  std::vector<int64_t> bits(n);
  for (int i = 0; i < n; i++) {
    pts[i] = cells[i]->getOrigin();
    bits[i] = static_cast<int64_t>(cells[i]->getBits());
  }

  std::vector<odb::Point> centroids = SeedCentroids(pts, k);

  // Lloyd's algorithm with capacity-aware assignment.
  std::vector<int> assignments(n, -1);
  std::vector<std::vector<std::pair<int64_t, int>>> ranked(n);
  for (auto& r : ranked) {
    r.reserve(k);
  }
  for (int iter = 0; iter < max_iters; iter++) {
    if (!AssignWithCapacity(
            pts, bits, centroids, cap_bits, ranked, assignments)) {
      break;  // Converged.
    }
    UpdateCentroids(pts, assignments, centroids);
  }

  return assignments;
}

}  // namespace dft
