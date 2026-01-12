// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanArchitectHeuristic.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <string_view>
#include <utility>
#include <vector>

#include "Opt.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {

namespace {

struct PlacedScanCell
{
  std::unique_ptr<ScanCell> cell;
  odb::Point origin;
  uint64_t bits = 0;
  std::string_view name;
};

int64_t manhattanDist(const odb::Point& a, const odb::Point& b)
{
  const int64_t dx = static_cast<int64_t>(a.x()) - static_cast<int64_t>(b.x());
  const int64_t dy = static_cast<int64_t>(a.y()) - static_cast<int64_t>(b.y());
  return std::abs(dx) + std::abs(dy);
}

std::vector<std::vector<std::unique_ptr<ScanCell>>> clusterPlacedScanCells(
    std::vector<std::unique_ptr<ScanCell>> domain_cells,
    std::size_t chain_count,
    uint64_t max_length,
    utl::Logger* logger)
{
  std::vector<std::vector<std::unique_ptr<ScanCell>>> clustered(chain_count);
  if (chain_count == 0 || domain_cells.empty()) {
    return clustered;
  }

  std::vector<PlacedScanCell> cells;
  cells.reserve(domain_cells.size());

  uint64_t total_bits = 0;
  uint64_t max_cell_bits = 0;
  for (auto& uptr : domain_cells) {
    ScanCell* raw = uptr.get();
    const uint64_t bits = raw->getBits();
    cells.push_back(
        {.cell = std::move(uptr),
         .origin = raw->getOrigin(),
         .bits = bits,
         .name = raw->getName()});
    total_bits += bits;
    max_cell_bits = std::max(max_cell_bits, bits);
  }
  domain_cells.clear();

  if (max_length == 0) {
    // Defensive: fall back to a single chain assignment.
    for (auto& c : cells) {
      clustered.front().push_back(std::move(c.cell));
    }
    return clustered;
  }

  if (max_cell_bits > max_length) {
    logger->warn(utl::DFT,
                 69,
                 "Scan architect max_length={} is smaller than the largest "
                 "scan cell ({} bits); packing will overflow.",
                 max_length,
                 max_cell_bits);
  }
  if (total_bits > chain_count * max_length) {
    logger->warn(utl::DFT,
                 70,
                 "Scan architect chain budget is infeasible ({} bits over "
                 "{} chains with max_length={} -> capacity {}); packing will "
                 "overflow.",
                 total_bits,
                 chain_count,
                 max_length,
                 chain_count * max_length);
  }

  const std::size_t n = cells.size();
  const std::size_t k = std::min(chain_count, n);

  // Deterministic centroid seeds: pick lower-leftmost, then farthest-from-nearest.
  std::vector<std::size_t> seed_indices;
  seed_indices.reserve(k);

  std::size_t first = 0;
  int64_t lowest = std::numeric_limits<int64_t>::max();
  for (std::size_t i = 0; i < n; ++i) {
    const auto& p = cells[i].origin;
    const int64_t score
        = static_cast<int64_t>(p.x()) + static_cast<int64_t>(p.y());
    if (score < lowest || (score == lowest && cells[i].name < cells[first].name)) {
      first = i;
      lowest = score;
    }
  }
  seed_indices.push_back(first);

  while (seed_indices.size() < k) {
    std::size_t best = 0;
    bool found = false;
    int64_t best_score = -1;
    for (std::size_t i = 0; i < n; ++i) {
      const bool is_seed = std::ranges::find(seed_indices, i) != seed_indices.end();
      if (is_seed) {
        continue;
      }
      int64_t nearest = std::numeric_limits<int64_t>::max();
      for (const std::size_t s : seed_indices) {
        nearest = std::min(nearest, manhattanDist(cells[i].origin, cells[s].origin));
      }
      if (!found || nearest > best_score
          || (nearest == best_score && cells[i].name < cells[best].name)) {
        best = i;
        best_score = nearest;
        found = true;
      }
    }
    if (!found) {
      break;
    }
    seed_indices.push_back(best);
  }

  std::vector<odb::Point> centroids(chain_count, cells[first].origin);
  for (std::size_t i = 0; i < k; ++i) {
    centroids[i] = cells[seed_indices[i]].origin;
  }

  std::vector<std::size_t> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::ranges::stable_sort(order, [&](std::size_t a, std::size_t b) {
    if (cells[a].bits != cells[b].bits) {
      return cells[a].bits > cells[b].bits;  // pack large cells first
    }
    return cells[a].name < cells[b].name;
  });

  std::vector<std::size_t> assignment(n, 0);
  std::vector<std::size_t> prev_assignment(n, 0);

  constexpr int kMaxIters = 5;
  for (int iter = 0; iter < kMaxIters; ++iter) {
    prev_assignment = assignment;
    std::vector<uint64_t> used_bits(chain_count, 0);
    std::vector<bool> is_assigned(n, false);
    bool overflowed = false;

    // Pin the centroid seed cells to ensure we use all chains when possible.
    for (std::size_t c = 0; c < k; ++c) {
      const std::size_t idx = seed_indices[c];
      assignment[idx] = c;
      is_assigned[idx] = true;
      used_bits[c] += cells[idx].bits;
      if (used_bits[c] > max_length) {
        overflowed = true;
      }
    }

    for (const std::size_t idx : order) {
      if (is_assigned[idx]) {
        continue;
      }

      bool found = false;
      std::size_t best_chain = 0;
      int64_t best_dist = std::numeric_limits<int64_t>::max();
      uint64_t best_used = std::numeric_limits<uint64_t>::max();

      for (std::size_t c = 0; c < chain_count; ++c) {
        if (used_bits[c] + cells[idx].bits > max_length) {
          continue;
        }
        const int64_t dist = manhattanDist(cells[idx].origin, centroids[c]);
        if (!found || dist < best_dist
            || (dist == best_dist
                && (used_bits[c] < best_used
                    || (used_bits[c] == best_used && c < best_chain)))) {
          found = true;
          best_chain = c;
          best_dist = dist;
          best_used = used_bits[c];
        }
      }

      if (!found) {
        // No chain has remaining capacity; pick the least-overflowing chain.
        overflowed = true;
        best_chain = 0;
        uint64_t best_overflow = std::numeric_limits<uint64_t>::max();
        for (std::size_t c = 0; c < chain_count; ++c) {
          const uint64_t new_bits = used_bits[c] + cells[idx].bits;
          const uint64_t overflow = (new_bits > max_length) ? (new_bits - max_length) : 0;
          if (overflow < best_overflow || (overflow == best_overflow && c < best_chain)) {
            best_overflow = overflow;
            best_chain = c;
          }
        }
      }

      assignment[idx] = best_chain;
      is_assigned[idx] = true;
      used_bits[best_chain] += cells[idx].bits;
    }

    // Recompute centroids (weighted by bits). Keep centroid for empty chains.
    std::vector<int64_t> sum_x(chain_count, 0);
    std::vector<int64_t> sum_y(chain_count, 0);
    std::vector<uint64_t> sum_bits(chain_count, 0);
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t c = assignment[i];
      sum_x[c] += static_cast<int64_t>(cells[i].origin.x())
                  * static_cast<int64_t>(cells[i].bits);
      sum_y[c] += static_cast<int64_t>(cells[i].origin.y())
                  * static_cast<int64_t>(cells[i].bits);
      sum_bits[c] += cells[i].bits;
    }
    for (std::size_t c = 0; c < chain_count; ++c) {
      if (sum_bits[c] == 0) {
        continue;
      }
      const int64_t cx = sum_x[c] / static_cast<int64_t>(sum_bits[c]);
      const int64_t cy = sum_y[c] / static_cast<int64_t>(sum_bits[c]);
      centroids[c] = odb::Point(static_cast<int>(cx), static_cast<int>(cy));
    }

    const bool changed = (assignment != prev_assignment);
    if (!changed) {
      if (overflowed) {
        logger->warn(utl::DFT,
                     71,
                     "Scan architect chain packing overflowed max_length={} "
                     "during clustering.",
                     max_length);
      }
      break;
    }
  }

  for (std::size_t i = 0; i < n; ++i) {
    clustered[assignment[i]].push_back(std::move(cells[i].cell));
  }

  return clustered;
}

}  // namespace

ScanArchitectHeuristic::ScanArchitectHeuristic(
    const ScanArchitectConfig& config,
    std::unique_ptr<ScanCellsBucket> scan_cells_bucket,
    utl::Logger* logger)
    : ScanArchitect(config, std::move(scan_cells_bucket)), logger_(logger)
{
}

void ScanArchitectHeuristic::architect()
{
  // For each hash_domain, lets distribute the scan cells over the scan chains
  for (auto& [hash_domain, scan_chains] : hash_domain_scan_chains_) {
    const uint64_t max_length
        = hash_domain_to_limits_.find(hash_domain)->second.max_length;

    // Collect all cells for this hash domain so we can do placement-aware
    // partitioning when multiple chains are requested.
    std::vector<std::unique_ptr<ScanCell>> domain_cells;
    domain_cells.reserve(scan_cells_bucket_->numberOfCells(hash_domain));
    while (scan_cells_bucket_->numberOfCells(hash_domain)) {
      domain_cells.push_back(scan_cells_bucket_->pop(hash_domain));
    }

    const bool do_spatial_partition
        = (scan_chains.size() > 1 && !domain_cells.empty()
           && std::ranges::all_of(domain_cells,
                                  [](const std::unique_ptr<ScanCell>& c) {
                                    return c->isPlaced();
                                  }));

    std::vector<std::vector<std::unique_ptr<ScanCell>>> chain_cells;
    if (do_spatial_partition) {
      // Placement-aware clustering with iterative reassignment (swap/move)
      // before running the intra-chain TSP heuristic.
      chain_cells = clusterPlacedScanCells(
          std::move(domain_cells), scan_chains.size(), max_length, logger_);
    } else {
      // Fallback: deterministic first-fit packing.
      chain_cells.resize(scan_chains.size());
      std::size_t cursor = 0;
      for (std::size_t c = 0; c < scan_chains.size(); ++c) {
        uint64_t used = 0;
        while (cursor < domain_cells.size()
               && used + domain_cells[cursor]->getBits() <= max_length) {
          used += domain_cells[cursor]->getBits();
          chain_cells[c].push_back(std::move(domain_cells[cursor]));
          ++cursor;
        }
      }

      if (cursor < domain_cells.size()) {
        logger_->warn(utl::DFT,
                      68,
                      "Scan architect could not pack all scan cells into the "
                      "requested chain budget ({} remaining).",
                      domain_cells.size() - cursor);
        for (; cursor < domain_cells.size(); ++cursor) {
          chain_cells.back().push_back(std::move(domain_cells[cursor]));
        }
      }
    }

    for (std::size_t i = 0; i < scan_chains.size(); ++i) {
      for (auto& cell : chain_cells[i]) {
        scan_chains[i]->add(std::move(cell));
      }
    }

    for (auto& current_chain : scan_chains) {
      current_chain->sortScanCells(
          [this](std::vector<std::unique_ptr<ScanCell>>& falling,
                 std::vector<std::unique_ptr<ScanCell>>& rising,
                 std::vector<std::unique_ptr<ScanCell>>& sorted) {
            sorted.reserve(falling.size() + rising.size());
            // Sort to reduce wire length
            OptimizeScanWirelength(falling, logger_);
            OptimizeScanWirelength(rising, logger_);
            // Falling edge first
            std::ranges::move(falling, std::back_inserter(sorted));
            std::ranges::move(rising, std::back_inserter(sorted));
          });
    }
  }
}

}  // namespace dft
