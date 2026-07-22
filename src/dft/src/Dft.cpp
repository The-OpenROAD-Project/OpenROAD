// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/Dft.hh"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "ClockDomain.hh"
#include "DftConfig.hh"
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
  logger_->warn(utl::DFT, 14, "Scan Opt is not currently implemented");
}

}  // namespace dft
