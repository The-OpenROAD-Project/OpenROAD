/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.

// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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
///////////////////////////////////////////////////////////////////////////////

// CORE and varients except SPACER
// CORE-SP = CORE, CORE FEEDTHRU, CORE TIEHIGH, CORE TIELOW, CORE ANTENNACELL, CORE WELLTAP

// COVER *, RING, PAD * - ignored
// CORE-SP to CORE-SP - padded footprints must not overlap
// CORE-SP to BLOCK * - no overlap (padding ignored)
// CORE-SP to ENDCAP *, CORE SPACER - no overlap (padding ignored)
// BLOCK * to BLOCK * - no checking
// The rules above apply to both FIXED or PLACED instances

#include <iostream>
#include <limits>
#include <iomanip>
#include <cmath>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::ofstream;
using std::to_string;
using std::vector;

using odb::Rect;
using odb::dbPlacementStatus;

using ord::warn;

bool Opendp::checkPlacement(bool verbose) {
  importDb();

  vector<Cell*> placed_failures;
  vector<Cell*> in_core_failures;
  vector<Cell*> overlap_failures;
  vector<Cell*> overlap_padded_failures;
  vector<Cell*> site_failures;
  vector<Cell*> power_line_failures;

  Grid *grid = makeGrid();
  bool have_padding = havePadding();
  for(Cell& cell : cells_) {
    if(isStdCell(&cell)) {
      // Site check
      if(cell.x_ % site_width_ != 0
	 || cell.y_ % row_height_ != 0)
	site_failures.push_back(&cell);
      if(checkPowerLine(cell)) {
	checkPowerLine(cell);
	power_line_failures.push_back(&cell);
      }
    }
    // Placed check
    if(!isPlaced(&cell))
      placed_failures.push_back(&cell);
    if(checkInCore(cell))
      in_core_failures.push_back(&cell);
    if(checkOverlap(cell, grid, false))
      overlap_failures.push_back(&cell);
    if(have_padding && checkOverlap(cell, grid, true))
      overlap_padded_failures.push_back(&cell);
  }

  reportFailures(placed_failures, "Placed", verbose);
  reportFailures(in_core_failures, "Placed in core", verbose);
  reportFailures(overlap_failures, "Overlap", verbose,
		 [&] (Cell *cell) -> void {reportOverlapFailure(cell, grid, false); });
  if (have_padding)
    reportFailures(overlap_padded_failures, "Overlap padded", verbose,
		   [&] (Cell *cell) -> void {reportOverlapFailure(cell, grid, true); });
  reportFailures(site_failures, "Site", verbose);
  reportFailures(power_line_failures, "Power line", verbose);

  deleteGrid(grid);

  return power_line_failures.size()
    || placed_failures.size()
    || in_core_failures.size()
    || overlap_failures.size()
    || site_failures.size();
}

void Opendp::reportFailures(vector<Cell*> failures,
			    const char *msg,
			    bool verbose) {
  reportFailures(failures, msg, verbose,
		 [] (Cell *cell) -> void { printf(" %s\n", cell->name()); });
}

void Opendp::reportFailures(vector<Cell*> failures,
			    const char *msg,
			    bool verbose,
			    std::function<void(Cell *cell)> report_failure) {
  if (failures.size()) {
    warn("%s check failed (%d).", msg, failures.size());
    if (verbose) {
      for(Cell *cell : failures) {
	report_failure(cell);
      }
    }
  }
}

void Opendp::reportOverlapFailure(Cell *cell, Grid *grid, bool padded) {
  Cell *overlap = checkOverlap(*cell, grid, padded);
  printf(" %s overlaps %s\n",
	 cell->name(),
	 overlap->name());
}

bool Opendp::isPlaced(Cell *cell) {
  switch (cell->db_inst_->getPlacementStatus()) {
  case dbPlacementStatus::PLACED:
  case dbPlacementStatus::FIRM:
  case dbPlacementStatus::LOCKED:
  case dbPlacementStatus::COVER:
    return true;
  case dbPlacementStatus::NONE:
  case dbPlacementStatus::UNPLACED:
  case dbPlacementStatus::SUGGESTED:
    return false;
  }
}

bool Opendp::checkPowerLine(Cell &cell) {
  int height = gridHeight(&cell);
  dbOrientType orient = cell.db_inst_->getOrient();
  int grid_y = gridY(&cell);
  Power top_power = topPower(&cell);
  return !(height == 1 || height == 3)
    // Everything below here is probably wrong but never exercised.
    && ((height % 2 == 0
	 // Even height
	 && top_power == rowTopPower(grid_y))
	|| (height % 2 == 1
	    // Odd height
	    && ((top_power == rowTopPower(grid_y)
		 && orient != dbOrientType::R0)
		|| (top_power != rowTopPower(grid_y)
		    && orient != dbOrientType::MX))));
}

bool Opendp::checkInCore(Cell &cell) {
  return gridPaddedX(&cell) < 0
    || gridY(&cell) < 0
    || gridPaddedEndX(&cell) > row_site_count_
    || gridEndY(&cell) > row_count_;
}


// Return the cell this cell overlaps.
Cell *Opendp::checkOverlap(Cell &cell,
			   Grid *grid,
			   bool padded) {
  int x_ll = gridPaddedX(&cell);
  int x_ur = gridPaddedEndX(&cell);
  int y_ll = gridY(&cell);
  int y_ur = gridEndY(&cell);
  x_ll = max(0, x_ll);
  y_ll = max(0, y_ll);
  x_ur = min(x_ur, row_site_count_);
  y_ur = min(y_ur, row_count_);
  
  for(int j = y_ll; j < y_ur; j++) {
    for(int k = x_ll; k < x_ur; k++) {
      Pixel &pixel = grid[j][k];
      Cell *pixel_cell = pixel.cell;
      if(pixel_cell) {
	if (pixel_cell != &cell
	    && overlap(&cell, pixel_cell, padded)
	    // BLOCK/BLOCK overlaps allowed
	    && !(isBlock(&cell)
		 && isBlock(pixel_cell)))
	  return pixel_cell;
      }
      else {
	pixel.cell = &cell;
      }
    }
  }
  return nullptr;
}

bool Opendp::overlap(Cell *cell1, Cell *cell2, bool padded) {
  int x_ll1, x_ur1, y_ll1, y_ur1;
  int x_ll2, x_ur2, y_ll2, y_ur2;
  if (padded) {
    initialPaddedLocation(cell1, x_ll1, y_ll1);
    initialPaddedLocation(cell2, x_ll2, y_ll2);
    x_ur1 = x_ll1 + paddedWidth(cell1);
    x_ur2 = x_ll2 + paddedWidth(cell2);
  }
  else {
    initialLocation(cell1, x_ll1, y_ll1);
    initialLocation(cell2, x_ll2, y_ll2);
    x_ur1 = x_ll1 + cell1->width_;
    x_ur2 = x_ll2 + cell2->width_;
  }
  y_ur1 = y_ll1 + cell1->height_;
  y_ur2 = y_ll2 + cell2->height_;
  return x_ll1 < x_ur2
    && x_ur1 > x_ll2
    && y_ll1 < y_ur2
    && y_ur1 > y_ll2;
}

}  // namespace opendp
