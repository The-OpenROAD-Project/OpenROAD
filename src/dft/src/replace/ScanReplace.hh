// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace dft {

// A scan cell that can be use to replace a non-scan cell
// It contains the LibertyCell that we can use to instantiate a new cell and a
// port map to store the equivalence non-scan and scan ports.
//
// A scan candidate is decided by checking some performance data (see
// ScanCandidate.cpp's DifferencePerformanceCells) to select the most similar
// one to the original cell
//
class ScanCandidate
{
 public:
  ScanCandidate(
      sta::LibertyCell* scan_cell,
      const std::unordered_map<std::string, std::string>& port_mapping);

  // Returns the LibertyCell of the scan cell
  sta::LibertyCell* getScanCell() const;

  // How to connect the old non scan cell's ports to the new scan cell
  const std::unordered_map<std::string, std::string>& getPortMapping() const;

  // Prints to DEBUG the port mapping to debug how we want to connect the
  // old's cell ports to the new one
  void debugPrintPortMapping(utl::Logger* logger) const;

 private:
  sta::LibertyCell* scan_cell_;
  std::unordered_map<std::string, std::string> port_mapping_;
};

class RollbackCandidate
{
 public:
  RollbackCandidate(
      odb::dbMaster* master,
      const std::unordered_map<std::string, std::string>& port_mapping);

  odb::dbMaster* getMaster() const;

  const std::unordered_map<std::string, std::string>& getPortMapping() const;

 private:
  odb::dbMaster* master_;
  std::unordered_map<std::string, std::string> port_mapping_;
};

// Performs scan replacement on a OpenROAD's database
class ScanReplace
{
 public:
  ScanReplace(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  // Populates the internal state with available scan cells (if there is any).
  // This method doesn't change the design
  void collectScanCellAvailable();

  // Perform the scan replacement of the non-scan cells into scan cells.
  // This method changes the design
  void scanReplace();

  void rollbackScanReplace();

  // Debug how we are going to replace non-scan lib cells into the available
  // scan lib cells
  void debugPrintScanEquivalents() const;

 private:
  std::unordered_map<sta::LibertyCell*, std::unique_ptr<ScanCandidate>>
      non_scan_to_scan_lib_cells_;
  std::unordered_set<sta::LibertyCell*> available_scan_lib_cells_;

  // Map from master->original master
  std::unordered_map<odb::dbMaster*, std::unique_ptr<RollbackCandidate>>
      rollback_candidates_;

  // Performs the scan replacement on the given block iterating over the
  // internal blocks (if there is any)
  void scanReplace(odb::dbBlock* block);

  // Rollsback the scan replace, replacing the cells with their old master.
  void rollbackScanReplace(odb::dbBlock* block);

  // Stores the master and scan cell's master so we can perform a rollback later
  // if we are running in preview_dft
  void addCellForRollback(odb::dbMaster* master,
                          odb::dbMaster* master_scan_cell,
                          const std::unique_ptr<ScanCandidate>& scan_candidate);

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  sta::dbNetwork* db_network_;
};

}  // namespace dft
