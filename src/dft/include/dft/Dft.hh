// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dft {
class ScanReplace;
class DftConfig;
class ScanChain;

// The main DFT implementation.
//
// We can split the DFT process in 3 main steps:
//
// 1) Scan Replace: Where non scan sequential cells are replaced by the scan
// equivalent.
//
// 2) Scan Architect: We create the scan chains and decide what scan cells are
// going to be in them. This is an instance of the Bin-Packing problem where we
// have elements of different size (cells of different bits) and bins (scan
// chains) where to put them.
//
// 3 Scan Stitching: Where we take the scan chains from Scan Architect and
// connect the cells together to form the scan chains in the design. This is
// done by connecting the output of each scan scell to the scan input of the
// next scan cell, based on the order defined in Scan Architect.
//
// See:
//  VLSI Test Principles and Architectures, 2006, Chapter 2.7: Scan Design Flow
//
class Dft
{
 public:
  Dft(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~Dft();

  // Pre-work for insert_dft. We collect the cells that need to be
  // scan replaced. This function doesn't mutate the design.
  void pre_dft();

  // Calls pre_dft performing scan replace and scan architect.
  // A report is going to be generated and printed followed by a rollback to
  // undo the work of scan replace. Use this command/function to iterate
  // different options like max_length
  //
  // Here we do:
  //  - Scan Replace
  //  - Scan Architect
  //  - Rollback
  //  - Shows Scan Architect report
  //
  // If verbose is true, then we show all the cells that are inside the scan
  // chains
  void reportDftPlan(bool verbose);

  // Inserts the scan chains into the design. For now this just replace the
  // cells in the design with scan equivalent. This functions mutates the
  // design.
  //
  // Here we do:
  //  - Scan Replace
  //
  void scanReplace();

  // Runs the complete flow for scan insertion based on the user's settings
  //
  // Here we do:
  //  - Scan Replace
  //  - Scan Architect
  //  - Scan Insertion
  //  - Store the inserted DFT (scan chains) into odb for later optimization
  void executeDftPlan();

  // Returns a mutable version of DftConfig
  DftConfig* getMutableDftConfig();

  // Returns a const version of DftConfig
  const DftConfig& getDftConfig() const;

  // Prints to stdout
  void reportDftConfig() const;

  // Performs scan optimizations on the netlist
  void scanOpt();

 private:
  // If we need to run pre_dft to create the internal state
  bool need_to_run_pre_dft_{true};

  // Resets the internal state
  void reset();

  // Common function to perform scan replace and scan architect. Shared between
  // report_dft_plan and execute_dft_plan
  std::vector<std::unique_ptr<ScanChain>> scanArchitect();

  // Global state
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;

  // Internal state
  std::unique_ptr<ScanReplace> scan_replace_;
  std::unique_ptr<DftConfig> dft_config_;
};

}  // namespace dft
