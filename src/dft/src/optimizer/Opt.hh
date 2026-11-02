// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "ScanCell.hh"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {

// Order scan cells to reduce wirelength. When given, source is where the
// chain enters the first cell and sink is where it leaves the last one; the
// order then accounts for those two wires.
void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger,
                            const std::optional<odb::Point>& source = {},
                            const std::optional<odb::Point>& sink = {});

}  // namespace dft
