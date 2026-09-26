// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ScanArchitect.hh"
#include "ScanCell.hh"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {

// Order scan cells to reduce wirelength
int64_t OptimizeScanWirelengthNN(std::vector<std::unique_ptr<ScanCell>>& cells,
                                 utl::Logger* logger);

int64_t OptimizeScanWirelength2Opt(
    odb::Point source,
    odb::Point sink,
    std::vector<std::unique_ptr<ScanCell>>& cells,
    utl::Logger* logger,
    size_t max_iters = 30);

}  // namespace dft
