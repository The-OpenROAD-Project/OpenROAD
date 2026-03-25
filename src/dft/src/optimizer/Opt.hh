// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "ScanCell.hh"
#include "utl/Logger.h"

namespace dft {

// Order scan cells to reduce wirelength.
// vertical_weight scales all vertical (y) distances relative to horizontal (x).
//   1.0 = standard Manhattan distance (default)
//   >1.0 = penalise vertical wiring (prefer horizontal connections)
//   <1.0 = penalise horizontal wiring (prefer vertical connections)
void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger,
                            double vertical_weight = 1.0);

}  // namespace dft
