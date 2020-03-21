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
  if (cells_.empty())
    importDb();

  vector<Cell*> placed_failures;
  vector<Cell*> in_core_failures;
  vector<Cell*> overlap_failures;
  vector<Cell*> row_failures;
  vector<Cell*> site_failures;
  vector<Cell*> power_line_failures;

  Grid *grid = makeGrid();
  for(Cell& cell : cells_) {
    if(isStdCell(&cell)) {
      // Row check
      if (cell.y_ % row_height_ != 0)
	row_failures.push_back(&cell);
      // Site check
      if(cell.x_ % site_width_ != 0)
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
    if(checkOverlap(cell, grid))
      overlap_failures.push_back(&cell);
  }

  reportFailures(placed_failures, "Placed", verbose);
  reportFailures(in_core_failures, "Placed in core", verbose);
  reportOverlapFailures(overlap_failures,
			"Overlap check failed.", verbose, grid);
  reportFailures(row_failures, "Row", verbose);
  reportFailures(site_failures, "Site", verbose);
  reportFailures(power_line_failures, "Power line", verbose);

  deleteGrid(grid);

  return power_line_failures.size()
    || placed_failures.size()
    || in_core_failures.size()
    || overlap_failures.size()
    || row_failures.size()
    || site_failures.size();
}

void Opendp::reportFailures(vector<Cell*> failures,
			    const char *msg,
			    bool verbose) {
  if (failures.size()) {
    warn("%s check failed (%d).", msg, failures.size());
    if (verbose) {
      for(Cell *cell : failures) {
	printf(" %s\n", cell->name());
      }
    }
  }
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

void Opendp::reportOverlapFailures(vector<Cell*> failures,
				   const char *msg,
				   bool verbose,
				   Grid *grid) {
  if (failures.size()) {
    warn("%s check failed (%d).", msg, failures.size());
    if (verbose) {
      for(Cell *cell : failures) {
	Cell *overlap = checkOverlap(*cell, grid);
	printf(" %s%s overlaps %s%s\n",
	       cell->name(),
	       isPadded(cell) ? " padded" : "",
	       overlap->name(),
	       isPadded(overlap) ? " padded" : "");
      }
    }
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


Cell *Opendp::checkOverlap(Cell &cell,
			   Grid *grid) {
  int grid_x = gridPaddedX(&cell);
  int x_ur = gridPaddedEndX(&cell);
  int grid_y = gridY(&cell);
  int y_ur = gridEndY(&cell);
  grid_x = max(0, grid_x);
  grid_y = max(0, grid_y);
  x_ur = min(x_ur, row_site_count_);
  y_ur = min(y_ur, row_count_);
  
  for(int j = grid_y; j < y_ur; j++) {
    for(int k = grid_x; k < x_ur; k++) {
      Pixel &pixel = grid[j][k];
      if(pixel.cell) {
	// BLOCK/BLOCK overlaps allowed
	if (!(isBlock(&cell)
	      && isBlock(pixel.cell)))
	  return pixel.cell;
      }
      else {
	pixel.cell = &cell;
      }
    }
  }
  return nullptr;
}

}  // namespace opendp
