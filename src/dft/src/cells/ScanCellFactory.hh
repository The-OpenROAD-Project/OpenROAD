// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "ScanCell.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dft {

// Returns a vector of ScanCells after iterating through the design and
// collecting the ScanCells.
std::vector<std::unique_ptr<ScanCell>> CollectScanCells(odb::dbDatabase* db,
                                                        sta::dbSta* sta,
                                                        utl::Logger* logger);

}  // namespace dft
