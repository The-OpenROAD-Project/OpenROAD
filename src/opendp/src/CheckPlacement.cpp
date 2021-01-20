/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "opendp/Opendp.h"
#include "utility/Logger.h"

namespace dpl {

using std::max;
using std::min;
using std::vector;

using odb::dbPlacementStatus;

using utl::DPL;

bool
Opendp::checkPlacement(bool verbose)
{
  importDb();

  vector<Cell *> placed_failures;
  vector<Cell *> in_rows_failures;
  vector<Cell *> overlap_failures;
  vector<Cell *> site_failures;
  vector<Cell *> power_line_failures;

  Grid *grid = makeGrid();
  for (Cell &cell : cells_) {
    if (isStdCell(&cell)) {
      // Site check
      if (cell.x_ % site_width_ != 0 || cell.y_ % row_height_ != 0)
        site_failures.push_back(&cell);
      if (checkPowerLine(cell)) {
        checkPowerLine(cell);
        power_line_failures.push_back(&cell);
      }
      if (!checkInRows(cell, grid))
        in_rows_failures.push_back(&cell);
    }
    // Placed check
    if (!isPlaced(&cell))
      placed_failures.push_back(&cell);
    // Overlap check
    if (checkOverlap(cell, grid))
      overlap_failures.push_back(&cell);
  }

  reportFailures(placed_failures, 3, "Placed", verbose);
  reportFailures(in_rows_failures, 4, "Placed in rows", verbose);
  reportFailures(overlap_failures, 5, "Overlap", verbose, [&](Cell *cell) -> void {
    reportOverlapFailure(cell, grid);
  });
  reportFailures(site_failures, 6, "Site", verbose);
  reportFailures(power_line_failures, 7, "Power line", verbose);

  deleteGrid(grid);

  return !power_line_failures.empty()
    || !placed_failures.empty()
    || !in_rows_failures.empty()
    || !overlap_failures.empty()
    || !site_failures.empty();
}

void
Opendp::reportFailures(const vector<Cell *> &failures,
                       int msg_id,
                       const char *msg,
                       bool verbose) const
{
  reportFailures(failures, msg_id, msg, verbose,
                 [&](Cell *cell) -> void {
                   logger_->report(" {}", cell->name());
                 });
}

void
Opendp::reportFailures(const vector<Cell *> &failures,
                       int msg_id,
                       const char *msg,
                       bool verbose,
                       const std::function<void(Cell *cell)> &report_failure) const
{
  if (!failures.empty()) {
    logger_->warn(DPL, msg_id, "{} check failed ({}).", msg, failures.size());
    if (verbose) {
      for (Cell *cell : failures) {
        report_failure(cell);
      }
    }
  }
}

void
Opendp::reportOverlapFailure(const Cell *cell, const Grid *grid) const
{
  const Cell *overlap = checkOverlap(*cell, grid);
  logger_->report(" {} overlaps {}", cell->name(), overlap->name());
}

bool
Opendp::isPlaced(const Cell *cell)
{
  return cell->db_inst_->isPlaced();
}

bool
Opendp::checkPowerLine(const Cell &cell) const
{
  int height = gridHeight(&cell);
  dbOrientType orient = cell.db_inst_->getOrient();
  int grid_y = gridY(&cell);
  Power top_power = topPower(&cell);
  return !(height == 1 || height == 3)
      // Everything below here is probably wrong but never exercised.
      && ((height % 2 == 0
           // Even height
           && top_power == rowTopPower(grid_y)) ||
          (height % 2 == 1
           // Odd height
           && ((top_power == rowTopPower(grid_y) && orient != dbOrientType::R0)
               || (top_power != rowTopPower(grid_y) && orient != dbOrientType::MX))));
}

bool
Opendp::checkInRows(const Cell &cell,
                    const Grid *grid) const
{
  int x_ll = gridX(&cell);
  int x_ur = gridEndX(&cell);
  int y_ll = gridY(&cell);
  int y_ur = gridEndY(&cell);
  if (!(x_ll >= 0
        && x_ur <= row_site_count_
        && y_ll >= 0
        && y_ur <= row_count_))
    return false;
  for (int j = y_ll; j < y_ur; j++) {
    for (int k = x_ll; k < x_ur; k++) {
      Pixel &pixel = grid[j][k];
      if (!pixel.is_valid)
        return false;
    }
  }
  return true;
}

// COVER *, RING, PAD * - ignored

// CLASSes are grouped as follows
// CR = {CORE, CORE FEEDTHRU, CORE TIEHIGH, CORE TIELOW, CORE ANTENNACELL}
// WT = CORE WELLTAP
// SP = CORE SPACER, ENDCAP *
// BL = BLOCK *

//    CR WT BL SP
// CR  P  P  P  O
// WT  P  O  P  O
// BL  P  P  -  O
// SP  O  O  O  O
//
// P = no padded overlap
// O = no overlap (padding ignored)
// - = no overlap check (overlap allowed)
// The rules apply to both FIXED or PLACED instances

// Return the cell this cell overlaps.

const Cell *
Opendp::checkOverlap(const Cell &cell, const Grid *grid) const
{
  int x_ll = gridPaddedX(&cell);
  int x_ur = gridPaddedEndX(&cell);
  int y_ll = gridY(&cell);
  int y_ur = gridEndY(&cell);
  x_ll = max(0, x_ll);
  y_ll = max(0, y_ll);
  x_ur = min(x_ur, row_site_count_);
  y_ur = min(y_ur, row_count_);

  for (int j = y_ll; j < y_ur; j++) {
    for (int k = x_ll; k < x_ur; k++) {
      Pixel &pixel = grid[j][k];
      const Cell *pixel_cell = pixel.cell;
      if (pixel_cell) {
        if (pixel_cell != &cell && overlap(&cell, pixel_cell)) {
          return pixel_cell;
        }
      }
      else {
        pixel.cell = &cell;
      }
    }
  }
  return nullptr;
}

bool
Opendp::overlap(const Cell *cell1, const Cell *cell2) const
{
  // BLOCK/BLOCK overlaps allowed
  if (isBlock(cell1) && isBlock(cell2)) {
    return false;
  }

  bool padded = havePadding() && isOverlapPadded(cell1, cell2);
  int x_ll1, x_ur1, y_ll1, y_ur1;
  int x_ll2, x_ur2, y_ll2, y_ur2;
  initialLocation(cell1, padded, &x_ll1, &y_ll1);
  initialLocation(cell2, padded, &x_ll2, &y_ll2);
  if (padded) {
    x_ur1 = x_ll1 + paddedWidth(cell1);
    x_ur2 = x_ll2 + paddedWidth(cell2);
  }
  else {
    x_ur1 = x_ll1 + cell1->width_;
    x_ur2 = x_ll2 + cell2->width_;
  }
  y_ur1 = y_ll1 + cell1->height_;
  y_ur2 = y_ll2 + cell2->height_;
  return x_ll1 < x_ur2 && x_ur1 > x_ll2 && y_ll1 < y_ur2 && y_ur1 > y_ll2;
}

bool
Opendp::isOverlapPadded(const Cell *cell1, const Cell *cell2) const
{
  return isCrWtBlClass(cell1)
    && isCrWtBlClass(cell2)
    && !(isWtClass(cell1) && isWtClass(cell2));
}

bool
Opendp::isCrWtBlClass(const Cell *cell) const
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
      return true;
    case dbMasterType::CORE_SPACER:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
    case dbMasterType::NONE:
      return false;
  }
  // gcc warniing
  return false;
}

bool
Opendp::isWtClass(const Cell *cell) const
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  return type == dbMasterType::CORE_WELLTAP;
}

}  // namespace
