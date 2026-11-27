// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "ScanCell.hh"

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
// unconstrained optimum while strictly honoring the bit cap.
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
//   k <= 1: all cells are assigned to cluster 0.
//   n <= k: each cell gets its own cluster, 0 to n - 1.
std::vector<int> KMeansClusters(const std::vector<ScanCell*>& cells,
                                int k,
                                int64_t cap_bits
                                = std::numeric_limits<int64_t>::max(),
                                int max_iters = 100);

}  // namespace dft
