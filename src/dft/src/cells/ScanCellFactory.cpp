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

#include "ScanCellFactory.hh"

#include <iostream>

#include "ClockDomain.hh"
#include "Utils.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/Clock.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sequential.hh"

namespace dft {

namespace {

enum class TypeOfCell
{
  OneBitCell,
  NotSupported
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

TypeOfCell IdentifyCell(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();
  if (master->isSequential() && !master->isBlock()) {
    // we assume that we are only dealing with one bit cells, but in the future
    // we could deal with multibit cells too
    return TypeOfCell::OneBitCell;
  }
  return TypeOfCell::NotSupported;
}

std::unique_ptr<ClockDomain> GetClockDomainFromClock(
    sta::LibertyCell* liberty_cell,
    sta::Clock* clock,
    odb::dbITerm* clock_pin)
{
  ClockEdge edge = ClockEdge::Rising;
  const sta::SequentialSeq& sequentials = liberty_cell->sequentials();
  // TODO: Other cells may have more than one sequential
  for (const sta::Sequential* sequential : sequentials) {
    // If the clock has a left FuncExpr, then
    sta::FuncExpr* left = sequential->clock()->left();
    sta::FuncExpr* right = sequential->clock()->right();
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
                                                       sta::dbSta* sta)
{
  std::vector<odb::dbITerm*> clock_pins = utils::GetClockPin(inst);
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
      = FindOneBitCellClockDomain(inst, sta);
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
  TypeOfCell type_of_cell = IdentifyCell(inst);

  switch (type_of_cell) {
    case TypeOfCell::OneBitCell:
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
  std::sort(scan_cells.begin(),
            scan_cells.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs->getName() < rhs->getName();
            });

  return scan_cells;
}

}  // namespace dft
