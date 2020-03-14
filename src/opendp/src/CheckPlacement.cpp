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

// CORE  to CORE  padded footprints must not overlap
// BLOCK to CORE  footprints must not overlap (padding ignored)
// BLOCK to BLOCK footprints must not overlap (padding ignored)
// The rules above apply to both FIXED or PLACED instances
// Instances of all other CLASSes are not checked (ignored)

#include <iostream>
#include <limits>
#include <iomanip>
#include <cmath>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::cout;
using std::endl;
using std::make_pair;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::to_string;
using std::vector;

using odb::adsRect;

bool Opendp::checkPlacement(bool verbose) {
  bool fail = false;
  fail |= row_check(verbose);
  fail |= site_check(verbose);
  fail |= power_line_check(verbose);
  fail |= placed_check(verbose);
  fail |= overlap_check(verbose);
  return fail;
}

bool Opendp::row_check(bool verbose) {
  bool fail = false;
  int count = 0;
  for(Cell& cell : cells_) {
    if(isClassCore(&cell)) {
      if(cell.y_ % row_height_ != 0) {
	if (verbose)
	  cout << "row_check fail => " << cell.name()
	       << " y : " << cell.y_ + core_.yMin() << endl;
	fail = true;
	count++;
      }
    }
  }

  if(fail)
    cout << "row check ==> FAIL (" << count << ")" << endl;
  else
    cout << "row check ==> PASS " << endl;

  return fail;
}

bool Opendp::site_check(bool verbose) {
  bool fail = false;
  int count = 0;
  for(Cell& cell : cells_) {
    if(isClassCore(&cell)) {
      if(cell.x_ % site_width_ != 0) {
	if (verbose)
	  cout << "site check fail ==> " << cell.name()
	       << " x : " << cell.x_ + core_.xMin() << endl;
	fail = true;
	count++;
      }
    }
  }
  if(fail)
    cout << "site check ==> FAIL (" << count << ")" << endl;
  else
    cout << "site check ==> PASS " << endl;
  return fail;
}

bool Opendp::power_line_check(bool verbose) {
  bool fail = false;
  int count = 0;
  for(Cell& cell : cells_) {
    if(isClassCore(&cell)
       // Magic number alert
       // Shouldn't this be odd test instead of 1 or 3? -cherry
       && !(cell.height_ == row_height_ || cell.height_ == row_height_ * 3)
       // should removed later
       && cell.inGroup()) {
      int y_size = gridHeight(&cell);
      int grid_y = gridY(&cell);
      power top_power = topPower(&cell);
      if(y_size % 2 == 0) {
	if(top_power == rowTopPower(grid_y)) {
	  cout << "power check fail ( even height ) ==> "
	      << cell.name() << endl;
	  fail = true;
	  count++;
	}
      }
      else {
	if(top_power == rowTopPower(grid_y)) {
	  if(cell.db_inst_->getOrient() != dbOrientType::R0) {
	    cout << "power check fail ( Should be N ) ==> "
		<< cell.name() << endl;
	    fail = true;
	    count++;
	  }
	}
	else {
	  if(cell.db_inst_->getOrient() != dbOrientType::MX) {
	    cout << "power_check fail ( Should be FS ) ==> "
		<< cell.name() << endl;
	    fail = true;
	    count++;
	  }
	}
      }
    }
  }

  if(fail)
    cout << "power check ==> FAIL (" << count << ")" << endl;
  else
    cout << "power check ==> PASS " << endl;
  return fail;
}

bool Opendp::placed_check(bool verbose) {
  bool fail = false;
  int count = 0;
  for(Cell& cell : cells_) {
    if(!cell.is_placed_) {
      if (verbose)
	cout << "placed check fail ==> " << cell.name() << endl;
      fail = true;
      count++;
    }
  }

  if(fail)
    cout << "placed_check ==> FAIL (" << count << ")" << endl;
  else
    cout << "placed_check ==>> PASS " << endl;

  return fail;
}

bool Opendp::overlap_check(bool verbose) {
  bool fail = false;
  Grid *grid = makeGrid();

  for(Cell& cell : cells_) {
    int grid_x = gridPaddedX(&cell);
    int x_ur = gridPaddedEndX(&cell);
    int grid_y = gridY(&cell);
    int y_ur = gridEndY(&cell);
    if (grid_x < 0
	|| grid_y < 0
	|| x_ur > row_site_count_
	|| y_ur > row_count_) {
      ord::warn("Cell %s %sis outside the core boundary.",
		cell.name(),
		isPadded(&cell) ? "with padding " : "");
      fail = true;
    }

    grid_x = max(0, grid_x);
    grid_y = max(0, grid_y);
    x_ur = min(x_ur, row_site_count_);
    y_ur = min(y_ur, row_count_);

    for(int j = grid_y; j < y_ur; j++) {
      for(int k = grid_x; k < x_ur; k++) {
        if(grid[j][k].cell == nullptr) {
          grid[j][k].cell = &cell;
          grid[j][k].util = 1.0;
        }
        else {
	  if (verbose)
	    cout << "overlap_check ==> FAIL ( cell " << cell.name()
		 << " overlaps " << grid[j][k].cell->name() << " ) "
		 << " ( " << (k * site_width_ + core_.xMin()) << ", "
		 << (j * row_height_ + core_.yMin()) << " )" << endl;
	  fail = true;
        }
      }
    }
  }
  deleteGrid(grid);

  if(fail)
    cout << "overlap_check ==> FAIL " << endl;
  else
    cout << "overlap_check ==> PASS " << endl;
  return fail;
}

}  // namespace opendp
