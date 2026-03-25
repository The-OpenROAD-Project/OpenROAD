// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/Dft.hh"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ClockDomain.hh"
#include "DftConfig.hh"
#include "OdbScanCellAdapter.hh"
#include "Opt.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "ScanCellFactory.hh"
#include "ScanPin.hh"
#include "ScanReplace.hh"
#include "ScanStitch.hh"
#include "boost/property_tree/json_parser.hpp"
#include "boost/property_tree/ptree.hpp"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace {
constexpr char kDefaultPartition[] = "default";
}  // namespace

namespace dft {

Dft::Dft(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db),
      sta_(sta),
      logger_(logger),
      dft_config_(std::make_unique<DftConfig>())
{
}

Dft::~Dft() = default;

void Dft::reset()
{
  scan_replace_.reset();
  need_to_run_pre_dft_ = true;
}

void Dft::pre_dft()
{
  scan_replace_ = std::make_unique<ScanReplace>(db_, sta_, logger_);
  scan_replace_->collectScanCellAvailable();

  // This should always be at the end
  need_to_run_pre_dft_ = false;
}

void Dft::reportDftPlan(bool verbose)
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }

  std::vector<std::unique_ptr<ScanChain>> scan_chains = scanArchitect();

  logger_->report("***************************");
  logger_->report("Report DFT Plan");
  logger_->report("Number of chains: {:d}", scan_chains.size());
  logger_->report("Clock domain: {:s}",
                  ScanArchitectConfig::ClockMixingName(
                      dft_config_->getScanArchitectConfig().getClockMixing()));
  logger_->report("***************************\n");
  for (const auto& scan_chain : scan_chains) {
    scan_chain->report(logger_, verbose);
  }
  logger_->report("");
}

void Dft::scanReplace()
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }
  scan_replace_->scanReplace();
}

void Dft::executeDftPlan()
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }
  std::vector<std::unique_ptr<ScanChain>> scan_chains = scanArchitect();

  ScanStitch stitch(db_, logger_, dft_config_->getScanStitchConfig());
  stitch.Stitch(scan_chains);

  // Write scan chains to odb
  odb::dbBlock* db_block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = db_block->getDft();

  for (const auto& chain : scan_chains) {
    odb::dbScanChain* db_sc = odb::dbScanChain::create(db_dft);
    db_sc->setName(chain->getName());
    odb::dbScanPartition* db_part = odb::dbScanPartition::create(db_sc);
    db_part->setName(kDefaultPartition);
    odb::dbScanList* db_scanlist = odb::dbScanList::create(db_part);

    for (const auto& scan_cell : chain->getScanCells()) {
      std::string inst_name(scan_cell->getName());
      odb::dbInst* db_inst = db_block->findInst(inst_name.c_str());
      odb::dbScanInst* db_scaninst = db_scanlist->add(db_inst);
      db_scaninst->setBits(scan_cell->getBits());
      ScanLoad scan_enable = scan_cell->getScanEnable();
      std::visit([&](auto&& pin) { db_scaninst->setScanEnable(pin); },
                 scan_enable.getValue());
      auto scan_in_term = scan_cell->getScanIn().getValue();
      auto scan_out_term = scan_cell->getScanOut().getValue();
      db_scaninst->setAccessPins(
          {.scan_in = scan_in_term, .scan_out = scan_out_term});

      const ClockDomain& clock_domain = scan_cell->getClockDomain();
      db_scaninst->setScanClock(clock_domain.getClockName());
      switch (clock_domain.getClockEdge()) {
        case ClockEdge::Rising:
          db_scaninst->setClockEdge(odb::dbScanInst::ClockEdge::Rising);
          break;
        case ClockEdge::Falling:
          db_scaninst->setClockEdge(odb::dbScanInst::ClockEdge::Falling);
          break;
      }
    }

    std::optional<ScanDriver> sc_enable_driver = chain->getScanEnable();
    std::optional<ScanDriver> sc_in_driver = chain->getScanIn();
    std::optional<ScanLoad> sc_out_load = chain->getScanOut();

    if (sc_enable_driver.has_value()) {
      std::visit(
          [&](auto&& sc_enable_term) { db_sc->setScanEnable(sc_enable_term); },
          sc_enable_driver.value().getValue());
    }
    if (sc_in_driver.has_value()) {
      std::visit([&](auto&& sc_in_term) { db_sc->setScanIn(sc_in_term); },
                 sc_in_driver.value().getValue());
    }
    if (sc_out_load.has_value()) {
      std::visit([&](auto&& sc_out_term) { db_sc->setScanOut(sc_out_term); },
                 sc_out_load.value().getValue());
    }
  }
}

DftConfig* Dft::getMutableDftConfig()
{
  return dft_config_.get();
}

const DftConfig& Dft::getDftConfig() const
{
  return *dft_config_;
}

void Dft::reportDftConfig() const
{
  logger_->report("DFT Config:");
  dft_config_->report(logger_);
}

std::vector<std::unique_ptr<ScanChain>> Dft::scanArchitect()
{
  std::vector<std::unique_ptr<ScanCell>> scan_cells
      = CollectScanCells(db_, sta_, logger_);

  // Scan Architect
  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger_);
  scan_cells_bucket->init(dft_config_->getScanArchitectConfig(), scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(
          dft_config_->getScanArchitectConfig(),
          std::move(scan_cells_bucket),
          logger_);
  scan_architect->init();
  scan_architect->architect();

  return scan_architect->getScanChains();
}

void Dft::scanOpt()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = block->getDft();

  int chains_optimized = 0;
  for (odb::dbScanChain* chain : db_dft->getScanChains()) {
    // Collect all scan instances from this chain.
    std::vector<std::unique_ptr<ScanCell>> cells;
    for (odb::dbScanPartition* part : chain->getScanPartitions()) {
      for (odb::dbScanList* list : part->getScanLists()) {
        for (odb::dbScanInst* si : list->getScanInsts()) {
          cells.push_back(
              std::make_unique<OdbScanCellAdapter>(si, logger_));
        }
      }
    }
    // dbScanList::add() uses insertAtFront, so iteration order from odb is
    // reversed relative to how executeDftPlan inserted them.  Reverse to
    // recover the original chain order before optimising.
    std::reverse(cells.begin(), cells.end());

    if (cells.size() < 2) {
      continue;
    }

    // Record the original order to detect if anything changed.
    std::vector<std::string> original_order;
    original_order.reserve(cells.size());
    for (const auto& c : cells) {
      original_order.push_back(std::string(c->getName()));
    }

    // Run the wirelength optimizer.
    OptimizeScanWirelength(cells, logger_);

    // Check if the order actually changed.
    bool order_changed = false;
    for (size_t i = 0; i < cells.size(); i++) {
      if (std::string(cells[i]->getName()) != original_order[i]) {
        order_changed = true;
        break;
      }
    }

    if (!order_changed) {
      continue;
    }

    // Restitch: reconnect scan-in / scan-out nets in the optimised order.
    //
    // Chain-level scan_in: connect to the first cell's SI.
    // Inter-cell: for each pair (i, i+1), connect SO[i] → SI[i+1].
    // Chain-level scan_out: connect the last cell's SO to the scan_out port.
    //
    // We reuse the scan-out net on each cell as the driver net.

    // First cell: connect chain scan_in to the first cell's SI.
    auto* first = static_cast<OdbScanCellAdapter*>(cells.front().get());
    odb::dbITerm* first_si = first->getScanInITerm();
    if (first_si != nullptr) {
      auto chain_si = chain->getScanIn();
      std::visit(
          [&](auto&& pin) {
            if (pin != nullptr) {
              odb::dbNet* net = pin->getNet();
              if (net != nullptr) {
                first_si->connect(net);
              }
            }
          },
          chain_si);
    }

    // Interior cells: connect SO[i] → SI[i+1].
    for (size_t i = 0; i + 1 < cells.size(); i++) {
      auto* curr = static_cast<OdbScanCellAdapter*>(cells[i].get());
      auto* next = static_cast<OdbScanCellAdapter*>(cells[i + 1].get());

      odb::dbITerm* so_iterm = curr->getScanOutITerm();
      odb::dbITerm* si_iterm = next->getScanInITerm();
      if (so_iterm == nullptr || si_iterm == nullptr) {
        continue;
      }

      // Reuse the net on the scan-out driver, or create a new one.
      odb::dbNet* net = so_iterm->getNet();
      if (net == nullptr) {
        net = odb::dbNet::create(block, so_iterm->getName().c_str());
        if (net == nullptr) {
          logger_->error(
              utl::DFT, 15, "Failed to create net for scan_opt restitching.");
        }
        net->setSigType(odb::dbSigType::SCAN);
        so_iterm->connect(net);
      }
      si_iterm->connect(net);
    }

    // Update the chain-level scan_out metadata to point to the new last
    // cell's scan-out ITerm.  We do NOT physically reconnect nets because
    // the scan_out BTerm/ITerm is typically a functional output (Q pin)
    // that must stay on its original net — moving it would create a
    // spurious assign in the Verilog netlist.
    auto* last = static_cast<OdbScanCellAdapter*>(cells.back().get());
    odb::dbITerm* last_so = last->getScanOutITerm();
    if (last_so != nullptr) {
      chain->setScanOut(last_so);
    }

    chains_optimized++;
  }

  logger_->info(
      utl::DFT, 16, "Optimized {} scan chain(s).", chains_optimized);
}

}  // namespace dft
