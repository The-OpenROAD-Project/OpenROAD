// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanArchitectHeuristic.hh"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "Opt.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
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
    for (auto& current_chain : scan_chains) {
      const uint64_t max_length
          = hash_domain_to_limits_.find(hash_domain)->second.max_length;
      while (current_chain->getBits() < max_length
             && scan_cells_bucket_->numberOfCells(hash_domain)) {
        std::unique_ptr<ScanCell> scan_cell
            = scan_cells_bucket_->pop(hash_domain);
        current_chain->add(std::move(scan_cell));
      }

      debugPrint(logger_,
                 utl::DFT,
                 "scan_architect",
                 1,
                 "Starting nearest-neighbor sort for chain {}",
                 current_chain->getName());

      auto total_wire_length = current_chain->sortScanCells(
          [&](std::vector<std::unique_ptr<ScanCell>>& falling,
              std::vector<std::unique_ptr<ScanCell>>& rising,
              std::vector<std::unique_ptr<ScanCell>>& sorted) {
            sorted.reserve(falling.size() + rising.size());
            // Sort to reduce wire length
            auto falling_wire_length
                = OptimizeScanWirelengthNN(falling, logger_);
            auto rising_wire_length = OptimizeScanWirelengthNN(rising, logger_);
            int64_t distance_between_falling_and_rising = 0;
            if (rising.size() && falling.size()) {
              distance_between_falling_and_rising
                  = odb::Point::manhattanDistance(
                      falling[falling.size() - 1]->getOrigin(),
                      rising[0]->getOrigin());
            }
            // Falling edges first
            std::ranges::move(falling, std::back_inserter(sorted));
            std::ranges::move(rising, std::back_inserter(sorted));
            return falling_wire_length + distance_between_falling_and_rising
                   + rising_wire_length;
          });

      debugPrint(logger_,
                 utl::DFT,
                 "scan_architect",
                 1,
                 "Concluded nearest-neighbor sort for chain {} with TWL {}",
                 current_chain->getName(),
                 total_wire_length);
    }
  }
}

}  // namespace dft
