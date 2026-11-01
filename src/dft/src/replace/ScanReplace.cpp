// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanReplace.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Utils.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace dft {

namespace {
// Checks the ports
sta::LibertyPort* FindEquivalentPortInScanCell(
    const sta::LibertyPort* non_scan_cell_port,
    const sta::LibertyCell* scan_cell)
{
  sta::LibertyCellPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* scan_cell_port = scan_cell_ports_iter.next();
    if (scan_cell_port->isPwrGnd()) {
      continue;
    }
    bool port_equiv
        = non_scan_cell_port->direction() == scan_cell_port->direction();
    if (non_scan_cell_port->function() == nullptr
        && scan_cell_port->function() == nullptr) {
      // input ports do not have a function
      port_equiv
          = port_equiv && non_scan_cell_port->name() == scan_cell_port->name();
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
sta::LibertyPort* FindEquivalentPgPortInScanCell(
    const sta::LibertyPort* non_scan_cell_port,
    const sta::LibertyCell* scan_cell)
{
  sta::LibertyCellPortIterator scan_cell_ports_iter(scan_cell);
  while (scan_cell_ports_iter.hasNext()) {
    sta::LibertyPort* scan_cell_port = scan_cell_ports_iter.next();
    if (!scan_cell_port->isPwrGnd()) {
      continue;
    }
    const bool port_equiv
        = sta::LibertyPort::equiv(non_scan_cell_port, scan_cell_port);
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
    if (non_scan_cell_port->isPwrGnd()) {
      continue;
    }
    sta::LibertyPort* scan_equiv_port
        = FindEquivalentPortInScanCell(non_scan_cell_port, scan_cell);
    if (!scan_equiv_port) {
      return false;
    }

    port_mapping.insert({non_scan_cell_port->name(), scan_equiv_port->name()});
    seen_on_scan_cell.insert(scan_equiv_port);
  }

  sta::LibertyCellPortIterator non_scan_cell_pg_ports_iter(non_scan_cell);
  while (non_scan_cell_pg_ports_iter.hasNext()) {
    sta::LibertyPort* non_scan_cell_pg_port
        = non_scan_cell_pg_ports_iter.next();
    if (!non_scan_cell_pg_port->isPwrGnd()) {
      continue;
    }
    sta::LibertyPort* scan_equiv_port
        = FindEquivalentPgPortInScanCell(non_scan_cell_pg_port, scan_cell);
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
    if (scan_cell_port->isPwrGnd()) {
      continue;
    }
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

// 1% relative tolerance on output drive resistance so sub-percent SPICE
// characterization differences defer to smaller cell area.
constexpr double kDriveResistanceRelTolerance = 0.01;

// 0.01% relative tolerance on cell area when drive resistance is tied.
constexpr float kAreaRelTolerance = 1e-4f;

// Finds the primary sequential output port, preferring the non-inverted state
// output (seq.output()) before inverted state outputs or general output ports.
const sta::LibertyPort* FindPrimaryOutputPort(const sta::LibertyCell* cell)
{
  if (!cell) {
    return nullptr;
  }
  const sta::LibertyPort* seq_inv_port = nullptr;
  const sta::LibertyPort* func_port = nullptr;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    if (port->isPwrGnd() || !port->direction()
        || !port->direction()->isAnyOutput()) {
      continue;
    }
    const sta::FuncExpr* func = port->function();
    if (func && !func_port) {
      func_port = port;
    }
    for (const sta::Sequential& seq : cell->sequentials()) {
      if (port == seq.output()
          || (func && func->port() && func->port() == seq.output())) {
        return port;
      }
      if (!seq_inv_port
          && (port == seq.outputInv()
              || (func
                  && ((seq.output() && func->hasPort(seq.output()))
                      || (seq.outputInv()
                          && func->hasPort(seq.outputInv())))))) {
        seq_inv_port = port;
      }
    }
  }
  if (seq_inv_port) {
    return seq_inv_port;
  }
  if (func_port) {
    return func_port;
  }
  return cell->findLibertyPort("Q");
}

// Computes `|R_non_scan - R_scan| / max(|R_non_scan|, |R_scan|)` so comparisons
// are invariant to library resistance units (ohms vs. kohms).
double RelativeDriveResistanceDiff(const sta::LibertyPort* non_scan_out,
                                   const ScanCandidate& candidate)
{
  if (!non_scan_out) {
    return std::numeric_limits<double>::max();
  }
  const sta::LibertyCell* scan_cell = candidate.getScanCell();
  const sta::LibertyPort* scan_out = nullptr;
  const auto& port_mapping = candidate.getPortMapping();
  if (auto it = port_mapping.find(non_scan_out->name());
      it != port_mapping.end()) {
    scan_out = scan_cell->findLibertyPort(it->second.c_str());
  }
  if (!scan_out) {
    scan_out = FindEquivalentPortInScanCell(non_scan_out, scan_cell);
  }
  if (!scan_out) {
    scan_out = scan_cell->findLibertyPort(non_scan_out->name());
  }
  if (!scan_out) {
    return std::numeric_limits<double>::max();
  }
  const double r_non_scan = non_scan_out->driveResistance();
  const double r_scan = scan_out->driveResistance();
  const double max_r = std::max(std::abs(r_non_scan), std::abs(r_scan));
  if (max_r == 0.0) {
    return 0.0;
  }
  return std::abs(r_non_scan - r_scan) / max_r;
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

bool IsSingleCharScanInsertion(std::string_view non_scan_str,
                               std::string_view scan_str)
{
  if (scan_str.size() != non_scan_str.size() + 1) {
    return false;
  }
  size_t prefix_len = 0;
  while (prefix_len < non_scan_str.size()
         && non_scan_str[prefix_len] == scan_str[prefix_len]) {
    ++prefix_len;
  }
  const char inserted = scan_str[prefix_len];
  if (inserted != 's' && inserted != 'S') {
    return false;
  }
  return scan_str.substr(prefix_len + 1) == non_scan_str.substr(prefix_len);
}

bool IsMatchingFootprint(std::string_view ns, std::string_view s)
{
  if (ns.empty() || s.empty()) {
    return false;
  }
  return s == ns || IsSingleCharScanInsertion(ns, s);
}

size_t SelectBestCandidateRankIndex(const std::vector<CandidateRank>& ranks)
{
  if (ranks.empty()) {
    return 0;
  }

  std::vector<size_t> survivors(ranks.size());
  std::iota(survivors.begin(), survivors.end(), 0);

  auto filter_by_flag = [&](auto predicate) {
    if (std::ranges::any_of(survivors, predicate)) {
      std::erase_if(survivors, [&](size_t i) { return !predicate(i); });
    }
  };

  // 1. Prefer matching Liberty cell_footprint (e.g. "dfxtp" -> "sdfxtp").
  filter_by_flag([&](size_t i) { return ranks[i].footprint_match; });

  // 2. Prefer scan cells from the same LibertyLibrary (VT family).
  filter_by_flag([&](size_t i) { return ranks[i].same_library; });

  // 3. Keep candidates within 1% relative output drive resistance of the best.
  const double min_rel_drive_diff
      = std::ranges::min(survivors | std::views::transform([&](size_t i) {
                           return ranks[i].rel_drive_diff;
                         }));
  std::erase_if(survivors, [&](size_t i) {
    return (ranks[i].rel_drive_diff - min_rel_drive_diff)
           > kDriveResistanceRelTolerance;
  });

  // 4. Within the drive-strength band, keep candidates within 0.01% of the
  // smallest cell area.
  const float min_area
      = std::ranges::min(survivors | std::views::transform([&](size_t i) {
                           return ranks[i].area;
                         }));
  std::erase_if(survivors, [&](size_t i) {
    const float max_area
        = std::max(std::abs(ranks[i].area), std::abs(min_area));
    return (ranks[i].area - min_area) > kAreaRelTolerance * max_area;
  });

  // 5. Tie-break by single-char ('s'/'S') scan insertion match, then cell name.
  filter_by_flag([&](size_t i) { return ranks[i].direct_name_match; });

  return *std::ranges::min_element(survivors, [&](size_t a, size_t b) {
    return ranks[a].name < ranks[b].name;
  });
}

namespace {

CandidateRank ComputeCandidateRank(const sta::LibertyCell* non_scan_cell,
                                   const sta::LibertyPort* non_scan_out,
                                   const ScanCandidate& candidate)
{
  const sta::LibertyCell* scan_cell = candidate.getScanCell();
  return CandidateRank{
      .footprint_match
      = IsMatchingFootprint(non_scan_cell->footprint(), scan_cell->footprint()),
      .same_library
      = (scan_cell->libertyLibrary() == non_scan_cell->libertyLibrary()),
      .rel_drive_diff = RelativeDriveResistanceDiff(non_scan_out, candidate),
      .area = scan_cell->area(),
      .direct_name_match
      = IsSingleCharScanInsertion(non_scan_cell->name(), scan_cell->name()),
      .name = scan_cell->name(),
  };
}

}  // namespace

std::unique_ptr<ScanCandidate> SelectBestScanCell(
    const sta::LibertyCell* non_scan_cell,
    std::vector<std::unique_ptr<ScanCandidate>>& scan_candidates)
{
  if (!non_scan_cell || scan_candidates.empty()) {
    return nullptr;
  }
  const sta::LibertyPort* non_scan_out = FindPrimaryOutputPort(non_scan_cell);
  std::vector<CandidateRank> ranks;
  ranks.reserve(scan_candidates.size());
  for (const auto& candidate : scan_candidates) {
    ranks.push_back(
        ComputeCandidateRank(non_scan_cell, non_scan_out, *candidate));
  }
  const size_t best_idx = SelectBestCandidateRankIndex(ranks);
  return std::move(scan_candidates.at(best_idx));
}

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
      if (!liberty_cell->isSequential()) {
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

      if (utils::IsScanCell(db_network, liberty_cell)) {
        if (resizer_ && resizer_->dontUse(liberty_cell)) {
          continue;
        }
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
                         rsz::Resizer* resizer,
                         utl::Logger* logger)
    : db_(db), sta_(sta), resizer_(resizer), logger_(logger)
{
  db_network_ = sta->getDbNetwork();
}

void ScanReplace::scanReplace()
{
  odb::dbChip* chip = db_->getChip();
  scanReplace(chip->getBlock());
  sta_->networkChangedNonSdc();
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

    if (!from_liberty_cell->isSequential()
        || from_liberty_cell->isClockGate()) {
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
  sta_->networkChangedNonSdc();
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
