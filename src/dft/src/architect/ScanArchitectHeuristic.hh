// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "utl/Logger.h"

namespace dft {

// An heuristic algorithm to solve the bin packing problem for the creation of
// scan chains. The idea is to sort the scan cells from the biggest (bits) to
// the smallest and start adding the biggest cells to each scan chain.
class ScanArchitectHeuristic : public ScanArchitect
{
 public:
  ScanArchitectHeuristic(const ScanArchitectConfig& config,
                         std::unique_ptr<ScanCellsBucket> scan_cells_bucket,
                         utl::Logger* logger);
  // Not copyable or movable
  ScanArchitectHeuristic(const ScanArchitectHeuristic&) = delete;
  ScanArchitectHeuristic& operator=(const ScanArchitectHeuristic&) = delete;
  ~ScanArchitectHeuristic() override = default;

  void architect() override;

  utl::Logger* logger_;

 private:
};

}  // namespace dft
