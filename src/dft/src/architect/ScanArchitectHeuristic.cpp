// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanArchitectHeuristic.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "Opt.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {

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

    if (do_spatial_partition) {
      // Sort by placement location to cluster scan cells into spatially-local
      // chains before running the intra-chain TSP heuristic. This reduces long
      // scan hops when multiple chains are enabled.
      std::ranges::stable_sort(domain_cells,
                               [](const std::unique_ptr<ScanCell>& a,
                                  const std::unique_ptr<ScanCell>& b) {
                                 const odb::Point oa = a->getOrigin();
                                 const odb::Point ob = b->getOrigin();
                                 if (oa.x() != ob.x()) {
                                   return oa.x() < ob.x();
                                 }
                                 if (oa.y() != ob.y()) {
                                   return oa.y() < ob.y();
                                 }
                                 return a->getName() < b->getName();
                               });
    }

    std::size_t cursor = 0;
    for (auto& current_chain : scan_chains) {
      while (cursor < domain_cells.size()
             && current_chain->getBits() + domain_cells[cursor]->getBits()
                    <= max_length) {
        current_chain->add(std::move(domain_cells[cursor]));
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
        scan_chains.back()->add(std::move(domain_cells[cursor]));
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
