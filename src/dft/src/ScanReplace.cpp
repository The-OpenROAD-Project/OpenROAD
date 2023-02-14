///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "ScanReplace.hh"
#include "Utils.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/Liberty.hh"
#include "sta/EquivCells.hh"
#include "sta/FuncExpr.hh"
#include "sta/Sequential.hh"
#include <iostream>

namespace dft {
namespace replace {

namespace {

// Checks if the given LibertyCell is really a Scan Cell with a Scan In and a
// Scan Enable
bool IsScanCell(const sta::LibertyCell* libertyCell) {
  if (libertyCell->isBuffer() || libertyCell->isInverter()) {
    return false;
  }

  const sta::TestCell* test_cell = libertyCell->testCell();
  if (test_cell) {
    return test_cell->scanIn() != nullptr && test_cell->scanEnable() != nullptr;
  }
  return false;
}

// Buffers can't be replaced
bool CanBeScanReplace(odb::dbMaster *master, const sta::LibertyCell* libertyCell) {
  if (libertyCell->isBuffer() || libertyCell->isInverter()) {
    return false;
  }

  return true;
}


// Checks the ports 
sta::LibertyPort* FindEquivalentPortInScanCell(const sta::LibertyPort* non_scan_cell_port, const sta::LibertyCell* scan_cell) {
  sta::LibertyCellPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* scan_cell_port = scan_cell_ports_iter.next();

    bool port_equiv = non_scan_cell_port->direction() == scan_cell_port->direction();
    if (non_scan_cell_port->function() == nullptr && scan_cell_port->function() == nullptr) {
      // input ports do not have a function
      port_equiv = 
          port_equiv
          && strcmp(non_scan_cell_port->name(), scan_cell_port->name()) == 0;
    } else {
      port_equiv =
          port_equiv
          && sta::FuncExpr::equiv(non_scan_cell_port->function(), scan_cell_port->function());
    }

    if (port_equiv) {
      return scan_cell_port;
    }
  }
  return nullptr;
}

// Checks the power ports between non scan cell and scan cell and checks if they
// are equivalent. Returns the equivalent port found, otherwise nullptr if there
// is none
sta::LibertyPgPort* FindEquivalentPortInScanCell(const sta::LibertyPgPort* non_scan_cell_port, const sta::LibertyCell* scan_cell) {
  sta::LibertyCellPgPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPgPort* scan_cell_port = scan_cell_ports_iter.next();
    const bool port_equiv = sta::LibertyPgPort::equiv(non_scan_cell_port, scan_cell_port);
    if (port_equiv) {
      return scan_cell_port;
    }
  }
  return nullptr;
}

// We look for each of the non_scan_cell's ports for a functional equivalent one
// in the scan_cell. If there is a port that we are unable to find an scan
// equivalent, then we can say that the given scan cell is not a replacement for
// the given non-scan one
bool IsScanEquivalent(const sta::LibertyCell* non_scan_cell, const sta::LibertyCell* scan_cell, std::unordered_map<std::string, std::string> &port_mapping) {
  sta::LibertyCellPortIterator non_scan_cell_ports_iter(non_scan_cell);
  while (non_scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* non_scan_cell_port = non_scan_cell_ports_iter.next();
    sta::LibertyPort* scan_equiv_port = FindEquivalentPortInScanCell(non_scan_cell_port, scan_cell);
    if (!scan_equiv_port) {
      return false;
    } else {
      port_mapping.insert({non_scan_cell_port->name(), scan_equiv_port->name()});
    }
  }

  sta::LibertyCellPgPortIterator non_scan_cell_pg_ports_iter(non_scan_cell);
  while (non_scan_cell_pg_ports_iter.hasNext()) {
    sta::LibertyPgPort* non_scan_cell_pg_port = non_scan_cell_pg_ports_iter.next();
    sta::LibertyPgPort* scan_equiv_port = FindEquivalentPortInScanCell(non_scan_cell_pg_port, scan_cell);
    if (!scan_equiv_port) {
      return false;
    } else {
      port_mapping.insert({non_scan_cell_pg_port->name(), scan_equiv_port->name()});
    }
  }

  return true;
}

// We calculate the difference in performance of the ports given
double DifferencePerformancePorts(const sta::LibertyPort* port1, const sta::LibertyPort* port2) {
  double diff = 0;

  diff += std::abs(port1->capacitance() - port2->capacitance());
  diff += std::abs(port1->driveResistance() - port2->driveResistance());

  bool exist_fanout_load1 = false, exist_fanout_load2 = false;
  float fanout_load1 = 0, fanout_load2 = 0;

  port1->fanoutLoad(fanout_load1, exist_fanout_load1);
  port2->fanoutLoad(fanout_load2, exist_fanout_load2);

  if (exist_fanout_load1 && exist_fanout_load2) {
    diff += std::abs(fanout_load1 - fanout_load2);
  }

  return diff;
}

// We calculate the difference in performance between the cells iterating
// through the ports
double DifferencePerformanceCells(const sta::LibertyCell* non_scan_cell, const sta::LibertyCell* scan_cell) {
  double diff = 0;

  // Area
  diff += std::abs(non_scan_cell->area() - scan_cell->area());

  // leakage
  if (non_scan_cell->leakagePowerExists() && scan_cell->leakagePowerExists()) {
    bool exists;
    float leak1 = 0, leak2 = 0;
    non_scan_cell->leakagePower(leak1, exists);
    scan_cell->leakagePower(leak2, exists);
    diff += std::abs(leak1 - leak2);
  }

  // Now let's check each of the ports
  sta::LibertyCellPortIterator non_scan_cell_ports_iter(non_scan_cell);
  while (non_scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* non_scan_cell_port = non_scan_cell_ports_iter.next();
    sta::LibertyPort* scan_equiv = FindEquivalentPortInScanCell(non_scan_cell_port, scan_cell);
    
    diff += DifferencePerformancePorts(non_scan_cell_port, scan_equiv);
  }

  return diff;
}

// We select the scan_cell that is most similar to the non_scan_cell in
// performance
std::unique_ptr<ScanCandidate> SelectBestScanCell(const sta::LibertyCell* non_scan_cell, std::vector<std::unique_ptr<ScanCandidate>> &scan_candidates) {
  std::sort(scan_candidates.begin(), scan_candidates.end(), [&non_scan_cell](const auto &lhs, const auto &rhs) {
    // We want to keep the difference as close as possible to the non_scan_cell
    const double difference_lhs = DifferencePerformanceCells(non_scan_cell, lhs->getScanCell());
    const double difference_rhs = DifferencePerformanceCells(non_scan_cell, rhs->getScanCell());
    return difference_lhs < difference_rhs;
  });

  return std::move(scan_candidates.at(0));
}


} // namespace
  

ScanCandidate::ScanCandidate(sta::LibertyCell* scan_cell, std::unordered_map<std::string, std::string> port_mapping):
  scan_cell_(scan_cell),
  port_mapping_(port_mapping)
{}

sta::LibertyCell* ScanCandidate::getScanCell() const {
  return scan_cell_;
}

const std::unordered_map<std::string, std::string>& ScanCandidate::getPortMapping() const {
  return port_mapping_;
}

void ScanCandidate::debugPrintPortMapping() const {
  std::cout << "Port mapping for cell: " << scan_cell_->name() << std::endl;
  for (const auto& [from_port, to_port]: port_mapping_) {
    std::cout << "    " << from_port << " -> " << to_port << std::endl;
  }
}

void ScanReplace::collectScanCellAvailable() {
  const sta::dbNetwork* db_network = sta_->getDbNetwork();

  std::vector<sta::LibertyCell*> non_scan_cells;


  // Let's collect scan lib cells and non-scan lib cells availables
  for (odb::dbLib* lib: db_->getLibs()) {
    for (odb::dbMaster* master: lib->getMasters()) {
      // We only care about sequential cells in DFT
      if (!master->isSequential()) {
        continue;
      }

      sta::Cell* master_cell = db_network->dbToSta(master);
      sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);

      if (!liberty_cell) {
        // Without info about the cell we can't do any replacement
        continue;
      }

      // If the lib cell is marked as dontUse, then skip it
      if (liberty_cell->dontUse()) {
        continue;
      }

      // We skip clock gating lib cells
      if (liberty_cell->isClockGate()) {
        continue;
      }
      
      if (IsScanCell(liberty_cell)) {
        available_scan_lib_cells_.insert(liberty_cell);
      } else if (CanBeScanReplace(master, liberty_cell)) {
        non_scan_cells.push_back(liberty_cell);
      }
    }
  }

  // Let's find what are the scan equivalent cells for each non scan
  std::unordered_map<sta::LibertyCell*, std::vector<std::unique_ptr<ScanCandidate>>> scan_candidates;
  for (sta::LibertyCell* non_scan_cell: non_scan_cells) {
    for (sta::LibertyCell* scan_cell: available_scan_lib_cells_) {
      std::unordered_map<std::string, std::string> port_mapping;
      if (IsScanEquivalent(non_scan_cell, scan_cell, port_mapping)) {
        scan_candidates[non_scan_cell].push_back(std::make_unique<ScanCandidate>(scan_cell, port_mapping));
      }
    }

    // Check if this non_scan_cell have scan equivalents
    auto found = scan_candidates.find(non_scan_cell);
    if (found == scan_candidates.end()) {
      logger_->warn(utl::DFT, 1, "Cell '{:s}' doesn't have an scan equivalent", non_scan_cell->name());
    }
  }

  // Populate non_scan_to_scan_cells to map non scan to scan cells with just the
  // best equivalent scan cell 
  for (auto& candidates: scan_candidates) {
    sta::LibertyCell* non_scan_cell = candidates.first;
    non_scan_to_scan_lib_cells_.insert({non_scan_cell, SelectBestScanCell(non_scan_cell, candidates.second)});
  }
}

ScanReplace::ScanReplace(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger):
  db_(db),
  sta_(sta),
  logger_(logger)
{
  db_network_ = sta->getDbNetwork();
}


void ScanReplace::scanReplace() {
  odb::dbChip* chip = db_->getChip();
  scanReplace(chip->getBlock());
}

// Recursive function that iterates over a block (and the blocks inside this
// one) replacing the cells with scan equivalent
void ScanReplace::scanReplace(odb::dbBlock* block) {
  odb::dbNet* ground_net = utils::FindGroundNet(db_network_, block);

  for (odb::dbInst* inst: block->getInsts()) {
    if (inst->isDoNotTouch()) {
      // Do not scan replace dont_touch
      continue;
    }

    if (inst->isHierarchical()) {
      // can't replace a hier
      continue;
    }

    if (!utils::IsSequentialCell(db_network_, inst)) {
      // If the cell is not sequential, then there is nothing to replace
      continue;
    }

    odb::dbMaster* master = inst->getMaster();
    sta::Cell* master_cell = db_network_->dbToSta(master);
    sta::LibertyCell* from_liberty_cell = db_network_->libertyCell(master_cell);

    auto found_scan_cell = available_scan_lib_cells_.find(from_liberty_cell);
    if (found_scan_cell != available_scan_lib_cells_.end()) {
      logger_->info(utl::DFT, 3, "Cell '{:s}' is already an scan cell, we will not replace it", inst->getName());
      continue;
    }

    auto found = non_scan_to_scan_lib_cells_.find(from_liberty_cell);
    if (found == non_scan_to_scan_lib_cells_.end()) {
      logger_->warn(utl::DFT, 2, "Can't scan replace cell '{:s}', that has lib cell '{:s}'. No scan equivalent lib cell found", inst->getName(), from_liberty_cell->name());
      continue;
    }

    const std::unique_ptr<ScanCandidate>& scan_candidate = found->second;
    sta::LibertyCell* scan_cell = scan_candidate->getScanCell();
    odb::dbMaster* master_scan_cell = db_network_->staToDb(scan_cell);

    odb::dbInst* new_instance = utils::ReplaceCell(block, inst, master_scan_cell, scan_candidate->getPortMapping());
    // Tie dangling scan pins to ground
    utils::TieScanPins(db_network_, new_instance, scan_candidate->getScanCell(), ground_net);
  }

  // Recursive iterate inside the block to look for inside hiers
  for (odb::dbBlock* next_block: block->getChildren()) {
    scanReplace(next_block);
  }
}

void ScanReplace::debugPrintScanEquivalents() const {
  for (const auto& [liberty_cell, scan_candidate]: non_scan_to_scan_lib_cells_) {
    std::cout << liberty_cell->name() << " -> " << scan_candidate->getScanCell()->name() << std::endl;
  }
}

} // namespace replace
} // namespace dft
