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

#include <iostream>
#include <limits>

#include "Utils.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/EquivCells.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sequential.hh"

namespace dft {

namespace {

// Checks if the given LibertyCell is really a Scan Cell with a Scan In and a
// Scan Enable
bool IsScanCell(const sta::LibertyCell* libertyCell)
{
  const sta::TestCell* test_cell = libertyCell->testCell();
  if (test_cell) {
    return getLibertyScanIn(test_cell) != nullptr
           && getLibertyScanEnable(test_cell) != nullptr;
  }
  return false;
}

// Checks the ports
sta::LibertyPort* FindEquivalentPortInScanCell(
    const sta::LibertyPort* non_scan_cell_port,
    const sta::LibertyCell* scan_cell)
{
  sta::LibertyCellPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* scan_cell_port = scan_cell_ports_iter.next();

    bool port_equiv
        = non_scan_cell_port->direction() == scan_cell_port->direction();
    if (non_scan_cell_port->function() == nullptr
        && scan_cell_port->function() == nullptr) {
      // input ports do not have a function
      port_equiv
          = port_equiv
            && strcmp(non_scan_cell_port->name(), scan_cell_port->name()) == 0;
    } else {
      port_equiv = port_equiv
                   && sta::FuncExpr::equiv(non_scan_cell_port->function(),
                                           scan_cell_port->function());
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
sta::LibertyPgPort* FindEquivalentPortInScanCell(
    const sta::LibertyPgPort* non_scan_cell_port,
    const sta::LibertyCell* scan_cell)
{
  sta::LibertyCellPgPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPgPort* scan_cell_port = scan_cell_ports_iter.next();
    const bool port_equiv
        = sta::LibertyPgPort::equiv(non_scan_cell_port, scan_cell_port);
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
bool IsScanEquivalent(
    const sta::LibertyCell* non_scan_cell,
    const sta::LibertyCell* scan_cell,
    std::unordered_map<std::string, std::string>& port_mapping)
{
  std::set<sta::LibertyPort*> seen_on_scan_cell;
  sta::LibertyCellPortIterator non_scan_cell_ports_iter(non_scan_cell);
  while (non_scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* non_scan_cell_port = non_scan_cell_ports_iter.next();
    sta::LibertyPort* scan_equiv_port
        = FindEquivalentPortInScanCell(non_scan_cell_port, scan_cell);
    if (!scan_equiv_port) {
      return false;
    }

    port_mapping.insert({non_scan_cell_port->name(), scan_equiv_port->name()});
    seen_on_scan_cell.insert(scan_equiv_port);
  }

  sta::LibertyCellPgPortIterator non_scan_cell_pg_ports_iter(non_scan_cell);
  while (non_scan_cell_pg_ports_iter.hasNext()) {
    sta::LibertyPgPort* non_scan_cell_pg_port
        = non_scan_cell_pg_ports_iter.next();
    sta::LibertyPgPort* scan_equiv_port
        = FindEquivalentPortInScanCell(non_scan_cell_pg_port, scan_cell);
    if (!scan_equiv_port) {
      return false;
    }

    port_mapping.insert(
        {non_scan_cell_pg_port->name(), scan_equiv_port->name()});
  }

  // Check there are no extra signals on the scan flop not present
  // on the non-scan flop (e.g. preset, clear, or enable)
  sta::TestCell* test_cell = scan_cell->testCell();
  sta::LibertyCellPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* scan_cell_port = scan_cell_ports_iter.next();
    if (seen_on_scan_cell.find(scan_cell_port) != seen_on_scan_cell.end()) {
      continue;
    }
    // Extra scan related pins are ok
    if (test_cell) {
      sta::LibertyPort* test_port
          = test_cell->findLibertyPort(scan_cell_port->name());
      if (!test_port) {
        return false;
      }

      if (test_port->scanSignalType() != sta::ScanSignalType::none) {
        continue;
      }
    }
    return false;
  }
  return true;
}

// We calculate the difference between drive resistance of the Q pins to find
// the most similar scan cell to the non-scan one
double DifferencePerformanceCells(const sta::LibertyCell* non_scan_cell,
                                  const sta::LibertyCell* scan_cell)
{
  sta::LibertyPort* non_scan_q = non_scan_cell->findLibertyPort("Q");
  sta::LibertyPort* scan_q = scan_cell->findLibertyPort("Q");

  if (!non_scan_q || !scan_q) {
    // Use double's max so this is moved to the end of the vector when sorting
    return std::numeric_limits<double>::max();
  }

  return std::abs(non_scan_q->driveResistance() - scan_q->driveResistance());
}

// We select the scan_cell that is most similar to the non_scan_cell in
// performance
std::unique_ptr<ScanCandidate> SelectBestScanCell(
    const sta::LibertyCell* non_scan_cell,
    std::vector<std::unique_ptr<ScanCandidate>>& scan_candidates)
{
  std::sort(scan_candidates.begin(),
            scan_candidates.end(),
            [&non_scan_cell](const auto& lhs, const auto& rhs) {
              // We want to keep the difference as close as possible to the
              // non_scan_cell
              const double difference_lhs = DifferencePerformanceCells(
                  non_scan_cell, lhs->getScanCell());
              const double difference_rhs = DifferencePerformanceCells(
                  non_scan_cell, rhs->getScanCell());
              return difference_lhs < difference_rhs;
            });

  return std::move(scan_candidates.at(0));
}

// Returns true if the instance is connected to don't touch nets, false
// otherwise
bool HaveDontTouchNets(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net && net->isDoNotTouch()) {
      return true;
    }
  }
  return false;
}

}  // namespace

ScanCandidate::ScanCandidate(
    sta::LibertyCell* scan_cell,
    const std::unordered_map<std::string, std::string>& port_mapping)
    : scan_cell_(scan_cell), port_mapping_(port_mapping)
{
}

sta::LibertyCell* ScanCandidate::getScanCell() const
{
  return scan_cell_;
}

const std::unordered_map<std::string, std::string>&
ScanCandidate::getPortMapping() const
{
  return port_mapping_;
}

void ScanCandidate::debugPrintPortMapping(utl::Logger* logger) const
{
  debugPrint(logger,
             utl::DFT,
             "print_port_mapping_scan_candidate",
             2,
             "Port mapping for cell: {}",
             scan_cell_->name());
  for (const auto& [from_port, to_port] : port_mapping_) {
    debugPrint(logger,
               utl::DFT,
               "print_port_mapping_scan_candidate",
               2,
               "    {} -> {}",
               from_port,
               to_port);
  }
}

RollbackCandidate::RollbackCandidate(
    odb::dbMaster* master,
    const std::unordered_map<std::string, std::string>& port_mapping)
    : master_(master), port_mapping_(port_mapping)
{
}

odb::dbMaster* RollbackCandidate::getMaster() const
{
  return master_;
}

const std::unordered_map<std::string, std::string>&
RollbackCandidate::getPortMapping() const
{
  return port_mapping_;
}

void ScanReplace::collectScanCellAvailable()
{
  const sta::dbNetwork* db_network = sta_->getDbNetwork();

  std::vector<sta::LibertyCell*> non_scan_cells;

  // Let's collect scan lib cells and non-scan lib cells availables
  for (odb::dbLib* lib : db_->getLibs()) {
    for (odb::dbMaster* master : lib->getMasters()) {
      sta::Cell* master_cell = db_network->dbToSta(master);
      sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);

      if (!liberty_cell) {
        // Without info about the cell we can't do any replacement
        continue;
      }

      // We only care about sequential cells in DFT
      if (!liberty_cell->hasSequentials()) {
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
      } else {
        non_scan_cells.push_back(liberty_cell);
      }
    }
  }

  // Let's find what are the scan equivalent cells for each non scan
  std::unordered_map<sta::LibertyCell*,
                     std::vector<std::unique_ptr<ScanCandidate>>>
      scan_candidates;
  for (sta::LibertyCell* non_scan_cell : non_scan_cells) {
    for (sta::LibertyCell* scan_cell : available_scan_lib_cells_) {
      std::unordered_map<std::string, std::string> port_mapping;
      if (IsScanEquivalent(non_scan_cell, scan_cell, port_mapping)) {
        scan_candidates[non_scan_cell].push_back(
            std::make_unique<ScanCandidate>(scan_cell, port_mapping));
      }
    }
  }

  // Populate non_scan_to_scan_cells to map non scan to scan cells with just the
  // best equivalent scan cell
  for (auto& candidates : scan_candidates) {
    sta::LibertyCell* non_scan_cell = candidates.first;
    std::unique_ptr<ScanCandidate> scan_candidate
        = SelectBestScanCell(non_scan_cell, candidates.second);
    scan_candidate->debugPrintPortMapping(logger_);
    non_scan_to_scan_lib_cells_.insert(
        {non_scan_cell, std::move(scan_candidate)});
  }

  debugPrintScanEquivalents();
}

ScanReplace::ScanReplace(odb::dbDatabase* db,
                         sta::dbSta* sta,
                         utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
  db_network_ = sta->getDbNetwork();
}

void ScanReplace::scanReplace()
{
  odb::dbChip* chip = db_->getChip();
  scanReplace(chip->getBlock());
  sta_->networkChanged();
}

// Recursive function that iterates over a block (and the blocks inside this
// one) replacing the cells with scan equivalent
void ScanReplace::scanReplace(odb::dbBlock* block)
{
  std::unordered_set<odb::dbInst*> already_replaced;
  std::unordered_set<odb::dbMaster*> no_lib_warned;

  for (odb::dbInst* inst : block->getInsts()) {
    // The instances already scan replaced are skipped
    if (already_replaced.find(inst) != already_replaced.end()) {
      continue;
    }

    if (inst->isDoNotTouch()) {
      // Do not scan replace dont_touch
      continue;
    }

    if (HaveDontTouchNets(inst)) {
      // The cell is connected to don't touch nets
      continue;
    }

    if (inst->isHierarchical()) {
      // can't replace a hier
      continue;
    }

    odb::dbMaster* master = inst->getMaster();
    sta::Cell* master_cell = db_network_->dbToSta(master);
    sta::LibertyCell* from_liberty_cell = db_network_->libertyCell(master_cell);

    if (from_liberty_cell == nullptr) {
      // cell doesn't exist in lib, no timing info
      if (master->getType() == odb::dbMasterType::CORE
          && no_lib_warned.find(master) == no_lib_warned.end()) {
        logger_->warn(
            utl::DFT,
            12,
            "Cell master '{:s}' has no lib info. Can't create scan cell(s)",
            master->getName());
        no_lib_warned.insert(master);
      }
      continue;
    }

    if (!from_liberty_cell->hasSequentials()) {
      // If the cell is not sequential, then there is nothing to replace
      continue;
    }

    if (available_scan_lib_cells_.find(from_liberty_cell)
        != available_scan_lib_cells_.end()) {
      logger_->info(
          utl::DFT,
          3,
          "Cell '{:s}' is already an scan cell, we will not replace it",
          inst->getName());
      continue;
    }

    auto found = non_scan_to_scan_lib_cells_.find(from_liberty_cell);
    if (found == non_scan_to_scan_lib_cells_.end()) {
      logger_->warn(utl::DFT,
                    2,
                    "Can't scan replace cell '{:s}', that has lib cell '{:s}'. "
                    "No scan equivalent lib cell found",
                    inst->getName(),
                    from_liberty_cell->name());
      continue;
    }

    const std::unique_ptr<ScanCandidate>& scan_candidate = found->second;
    sta::LibertyCell* scan_cell = scan_candidate->getScanCell();
    odb::dbMaster* master_scan_cell = db_network_->staToDb(scan_cell);

    odb::dbInst* new_cell = utils::ReplaceCell(
        block, inst, master_scan_cell, scan_candidate->getPortMapping());

    already_replaced.insert(new_cell);
    addCellForRollback(master, master_scan_cell, scan_candidate);
  }

  // Recursive iterate inside the block to look for inside hiers
  for (odb::dbBlock* next_block : block->getChildren()) {
    scanReplace(next_block);
  }
}

void ScanReplace::debugPrintScanEquivalents() const
{
  for (const auto& [liberty_cell, scan_candidate] :
       non_scan_to_scan_lib_cells_) {
    debugPrint(logger_,
               utl::DFT,
               "print_scan_equivalent",
               2,
               "{:s} -> {:s}",
               liberty_cell->name(),
               scan_candidate->getScanCell()->name());
  }
}

void ScanReplace::rollbackScanReplace()
{
  odb::dbChip* chip = db_->getChip();
  rollbackScanReplace(chip->getBlock());
  sta_->networkChanged();
}

void ScanReplace::rollbackScanReplace(odb::dbBlock* block)
{
  for (odb::dbInst* inst : block->getInsts()) {
    auto found = rollback_candidates_.find(inst->getMaster());
    if (found == rollback_candidates_.end()) {
      // Cell was not scan replaced, nothing to do
      continue;
    }

    RollbackCandidate& rollback_candidate = *found->second;
    utils::ReplaceCell(block,
                       inst,
                       rollback_candidate.getMaster(),
                       rollback_candidate.getPortMapping());
  }

  // Recursive iterate inside the block to look for inside hiers
  for (odb::dbBlock* next_block : block->getChildren()) {
    rollbackScanReplace(next_block);
  }
}

void ScanReplace::addCellForRollback(
    odb::dbMaster* master,
    odb::dbMaster* master_scan_cell,
    const std::unique_ptr<ScanCandidate>& scan_candidate)
{
  auto found = rollback_candidates_.find(master_scan_cell);
  if (found != rollback_candidates_.end()) {
    // This master already has a rollback, nothing to do
    return;
  }

  // Flip the port mapping to be able to rollback
  std::unordered_map<std::string, std::string> rollback_port_mapping;
  for (const auto& [from, to] : scan_candidate->getPortMapping()) {
    rollback_port_mapping.insert({to, from});
  }

  std::unique_ptr<RollbackCandidate> rollback_candidate
      = std::make_unique<RollbackCandidate>(master, rollback_port_mapping);
  rollback_candidates_.insert(
      {master_scan_cell, std::move(rollback_candidate)});
}

}  // namespace dft
