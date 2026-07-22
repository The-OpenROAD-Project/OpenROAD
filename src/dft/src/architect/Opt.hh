// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "ScanArchitect.hh"
#include "ScanCell.hh"
#include "utl/Logger.h"

namespace dft {

// Order scan cells to reduce wirelength
void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger);

}  // namespace dft
