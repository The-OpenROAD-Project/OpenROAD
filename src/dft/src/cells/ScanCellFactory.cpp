// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanCellFactory.hh"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "OneBitScanCell.hh"
#include "ScanCell.hh"
#include "Utils.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace dft {

namespace {

enum class TypeOfCell
{
  kOneBitCell,
  kNotSupported
};

sta::LibertyCell* GetLibertyCell(odb::dbMaster* master,
                                 sta::dbNetwork* db_network)
{
  sta::Cell* master_cell = db_network->dbToSta(master);
  return db_network->libertyCell(master_cell);
}

sta::TestCell* GetTestCell(odb::dbMaster* master,
                           sta::dbNetwork* db_network,
                           utl::Logger* logger)
{
  sta::LibertyCell* liberty_cell = GetLibertyCell(master, db_network);
  if (liberty_cell == nullptr) {
    logger->warn(utl::DFT,
                 11,
                 "Cell master '{:s}' has no lib info. Can't find scan cell",
                 master->getName());
    return nullptr;
  }
  sta::TestCell* test_cell = liberty_cell->testCell();
  if (test_cell && getLibertyScanIn(test_cell) != nullptr
      && getLibertyScanEnable(test_cell) != nullptr) {
    return test_cell;
  }
  return nullptr;
}

TypeOfCell IdentifyCell(odb::dbInst* inst, sta::dbSta* sta)
{
  sta::dbNetwork* db_network = sta->getDbNetwork();
  sta::LibertyCell* liberty_cell
      = GetLibertyCell(inst->getMaster(), db_network);
  if (liberty_cell != nullptr && liberty_cell->hasSequentials()
      && !inst->getMaster()->isBlock()) {
    // we assume that we are only dealing with one bit cells, but in the future
    // we could deal with multibit cells too
    return TypeOfCell::kOneBitCell;
  }
  return TypeOfCell::kNotSupported;
}

std::unique_ptr<ClockDomain> GetClockDomainFromClock(
    sta::LibertyCell* liberty_cell,
    sta::Clock* clock,
    odb::dbITerm* clock_pin)
{
  ClockEdge edge = ClockEdge::Rising;
  const sta::SequentialSeq& sequentials = liberty_cell->sequentials();
  // TODO: Other cells may have more than one sequential
  for (const sta::Sequential& sequential : sequentials) {
    // If the clock has a left FuncExpr, then
    sta::FuncExpr* left = sequential.clock()->left();
    sta::FuncExpr* right = sequential.clock()->right();
    if (left && !right) {
      //  When operator is NOT, left is the only operand
      edge = ClockEdge::Falling;
    } else {
      edge = ClockEdge::Rising;
    }
  }

  // TODO: Create the clock domain based on the timing instead of the name to
  // better control equivalent clocks
  return std::make_unique<ClockDomain>(clock->name(), edge);
}

std::unique_ptr<ClockDomain> FindOneBitCellClockDomain(odb::dbInst* inst,
                                                       sta::dbSta* sta,
                                                       utl::Logger* logger)
{
  std::vector<odb::dbITerm*> clock_pins = utils::GetClockPin(inst);
  if (clock_pins.empty()) {
    logger->warn(utl::DFT,
                 49,
                 "Can't find a clock pin for cell '{:s}'",
                 inst->getName());
    return nullptr;
  }
  sta::dbNetwork* db_network = sta->getDbNetwork();
  // A one bit cell should only have one clock pin

  odb::dbITerm* clock_pin = clock_pins.at(0);
  std::optional<sta::Clock*> clock = utils::GetClock(sta, clock_pin);

  if (clock.has_value()) {
    return GetClockDomainFromClock(
        GetLibertyCell(inst->getMaster(), db_network), *clock, clock_pin);
  }

  return nullptr;
}

std::unique_ptr<OneBitScanCell> CreateOneBitCell(odb::dbInst* inst,
                                                 sta::dbSta* sta,
                                                 utl::Logger* logger)
{
  sta::dbNetwork* db_network = sta->getDbNetwork();
  std::unique_ptr<ClockDomain> clock_domain
      = FindOneBitCellClockDomain(inst, sta, logger);
  sta::TestCell* test_cell = GetTestCell(inst->getMaster(), db_network, logger);

  if (!clock_domain) {
    logger->warn(utl::DFT,
                 5,
                 "Cell '{:s}' doesn't have a valid clock connected. Can't "
                 "create a scan cell",
                 inst->getName());
    return nullptr;
  }

  if (!test_cell) {
    logger->warn(
        utl::DFT,
        7,
        "Cell '{:s}' is not a scan cell. Can't use it for scan architect",
        inst->getName());
    return nullptr;
  }

  return std::make_unique<OneBitScanCell>(inst->getName(),
                                          std::move(clock_domain),
                                          inst,
                                          test_cell,
                                          db_network,
                                          logger);
}

}  // namespace

std::unique_ptr<ScanCell> ScanCellFactory(odb::dbInst* inst,
                                          sta::dbSta* sta,
                                          utl::Logger* logger)
{
  TypeOfCell type_of_cell = IdentifyCell(inst, sta);

  switch (type_of_cell) {
    case TypeOfCell::kOneBitCell:
      return CreateOneBitCell(inst, sta, logger);
    default:
      return nullptr;
      break;
  }
}

void CollectScanCells(odb::dbBlock* block,
                      sta::dbSta* sta,
                      utl::Logger* logger,
                      std::vector<std::unique_ptr<ScanCell>>& scan_cells)
{
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isDoNotTouch()) {
      // Do not scan
      continue;
    }
    // TODO: Support doNotScan later

    std::unique_ptr<ScanCell> scan_cell = ScanCellFactory(inst, sta, logger);
    if (scan_cell != nullptr) {
      scan_cells.push_back(std::move(scan_cell));
    }
  }

  // Go inside the next blocks
  for (odb::dbBlock* next_block : block->getChildren()) {
    CollectScanCells(next_block, sta, logger, scan_cells);
  }
}

std::vector<std::unique_ptr<ScanCell>> CollectScanCells(odb::dbDatabase* db,
                                                        sta::dbSta* sta,
                                                        utl::Logger* logger)
{
  std::vector<std::unique_ptr<ScanCell>> scan_cells;

  odb::dbChip* chip = db->getChip();
  CollectScanCells(chip->getBlock(), sta, logger, scan_cells);

  // To keep preview_dft consistent between calls and rollbacks
  std::ranges::sort(scan_cells, [](const auto& lhs, const auto& rhs) {
    return lhs->getName() < rhs->getName();
  });

  return scan_cells;
}

}  // namespace dft
