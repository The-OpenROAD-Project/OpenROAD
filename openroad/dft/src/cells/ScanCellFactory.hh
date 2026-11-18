// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "OneBitScanCell.hh"
#include "ScanCell.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"

namespace dft {

// Creates a scan cell based on the given dbInst.
std::unique_ptr<ScanCell> ScanCellFactory(odb::dbInst* inst,
                                          utl::Logger* logger);

// Returns a vector of ScanCells after iterating throught all the design
// collecting the ScanCells
std::vector<std::unique_ptr<ScanCell>> CollectScanCells(odb::dbDatabase* db,
                                                        sta::dbSta* sta,
                                                        utl::Logger* logger);

}  // namespace dft
