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

#include <iostream>
#include <limits>
#include <iomanip>
#include <cmath>
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

bool Opendp::checkLegality(bool verbose) {
  cout << "Check Legality" << endl;

  bool legal = true;
  legal &= row_check(verbose);
  legal &= site_check(verbose);
  legal &= power_line_check(verbose);
  legal &= edge_check(verbose);
  legal &= placed_check(verbose);
  legal &= overlap_check(verbose);
  return legal;
}

bool Opendp::row_check(bool verbose) {
  bool valid = true;
  int count = 0;
  for(int i = 0; i < cells_.size(); i++) {
    Cell* cell = &cells_[i];
    if(!isFixed(cell)) {
      if(cell->y_ % row_height_ != 0) {
	if (verbose)
	  cout << "row_check fail => " << cell->name()
	       << " y_ : " << cell->y_ << endl;
	valid = false;
	count++;
      }
    }
  }

  if(!valid)
    cout << "row check ==> FAIL (" << count << ")" << endl;
  else
    cout << "row check ==> PASS " << endl;

  return valid;
}

bool Opendp::site_check(bool verbose) {
  bool valid = true;
  int count = 0;
  for(int i = 0; i < cells_.size(); i++) {
    Cell* cell = &cells_[i];
    if(!isFixed(cell)) {
      if(cell->x_ % site_width_ != 0) {
	if (verbose)
	  cout << "site check fail ==> " << cell->name()
	       << " x_ : " << cell->x_ << endl;
	valid = false;
	count++;
      }
    }
  }
  if(!valid)
    cout << "site check ==> FAIL (" << count << ")" << endl;
  else
    cout << "site check ==> PASS " << endl;
  return valid;
}

bool Opendp::edge_check(bool verbose) {
  bool valid = true;
  int count = 0;

  for(int i = 0; i < rows_.size(); i++) {
    vector< Cell* > cells;
    for(int j = 0; j < row_site_count_; j++) {
      Cell* grid_cell = grid_[i][j].cell;
      if(grid_[i][j].is_valid) {
	if(grid_cell != nullptr && grid_cell != &dummy_cell_) {
#ifdef ODP_DEBUG
	  cout << "grid util : " << grid[i][j].util << endl;
	  cout << "cell name : "
	       << grid[i][j].cell->name() << endl;
#endif
	  if(cells.empty()) {
	    cells.push_back(grid_[i][j].cell);
	  }
	  else if(cells[cells.size() - 1] != grid_[i][j].cell) {
	    cells.push_back(grid_[i][j].cell);
	  }
	}
      }
    }
#ifdef ODP_DEBUG
    cout << " row search done " << endl;
    cout << " cell list size : " << cells.size() << endl;
#endif
  }

  if(!valid)
    cout << "edge check ==> FAIL (" << count << ")" << endl;
  else
    cout << "edge_check ==> PASS " << endl;
  return valid;
}

bool Opendp::power_line_check(bool verbose) {
  bool valid = true;
  int count = 0;
  for(Cell& cell : cells_) {
    if(!isFixed(&cell)
       // Magic number alert
       // Shouldn't this be odd test instead of 1 or 3? -cherry
       && !(cell.height_ == row_height_ || cell.height_ == row_height_ * 3)
       // should removed later
       && cell.inGroup()) {
      int y_size = gridHeight(&cell);
      int grid_y_ = gridY(&cell);
      power top_power = topPower(&cell);
      if(y_size % 2 == 0) {
	if(top_power == rows_[grid_y_].top_power) {
	  cout << "power check fail ( even height ) ==> "
	      << cell.name() << endl;
	  valid = false;
	  count++;
	}
      }
      else {
	if(top_power == rows_[grid_y_].top_power) {
	  if(cell.db_inst_->getOrient() != dbOrientType::R0) {
	    cout << "power check fail ( Should be N ) ==> "
		<< cell.name() << endl;
	    valid = false;
	    count++;
	  }
	}
	else {
	  if(cell.db_inst_->getOrient() != dbOrientType::MX) {
	    cout << "power_check fail ( Should be FS ) ==> "
		<< cell.name() << endl;
	    valid = false;
	    count++;
	  }
	}
      }
    }
  }

  if(!valid)
    cout << "power check ==> FAIL (" << count << ")" << endl;
  else
    cout << "power check ==> PASS " << endl;
  return valid;
}

bool Opendp::placed_check(bool verbose) {
  bool valid = true;
  int count = 0;
  for(Cell& cell : cells_) {
    if(!cell.is_placed_) {
      if (verbose)
	cout << "placed check fail ==> " << cell.name() << endl;
      valid = false;
      count++;
    }
  }

  if(!valid)
    cout << "placed_check ==> FAIL (" << count << ")" << endl;
  else
    cout << "placed_check ==>> PASS " << endl;

  return valid;
}

bool Opendp::overlap_check(bool verbose) {
  bool valid = true;
  int row_count = rows_.size();
  int col_count = row_site_count_;
  Pixel** grid2;
  grid2 = new Pixel*[row_count];
  for(int i = 0; i < row_count; i++) {
    grid2[i] = new Pixel[col_count];
  }

  for(int i = 0; i < row_count; i++) {
    for(int j = 0; j < col_count; j++) {
      grid2[i][j].grid_y_ = i;
      grid2[i][j].grid_x_ = j;
      grid2[i][j].cell = nullptr;
    }
  }

  for(Cell& cell : cells_) {
    int grid_x_ = gridX(&cell);
    int grid_y_ = gridY(&cell);

    int x_ur = gridEndX(&cell);
    int y_ur = gridEndY(&cell);

    // Fixed Cell can be out of Current DIEAREA settings.
    if(isFixed(&cell)) {
      grid_x_ = max(0, grid_x_);
      grid_y_ = max(0, grid_y_);
      x_ur = min(x_ur, col_count);
      y_ur = min(y_ur, row_count);
    }

    assert(grid_x_ >= 0);
    assert(grid_y_ >= 0);
    assert(x_ur <= coreGridMaxX());
    assert(y_ur <= coreGridMaxY());

    for(int j = grid_y_; j < y_ur; j++) {
      for(int k = grid_x_; k < x_ur; k++) {
        if(grid2[j][k].cell == nullptr) {
          grid2[j][k].cell = &cell;
          grid2[j][k].util = 1.0;
        }
        else {
	  if (verbose)
	    cout << "overlap_check ==> FAIL ( cell " << cell.name()
		 << " overlaps " << grid2[j][k].cell->name() << " ) "
		 << " ( " << (k * site_width_ + core_.xMin()) << ", "
		 << (j * row_height_ + core_.yMin()) << " )" << endl;
          valid = false;
        }
      }
    }
  }
  if(valid)
    cout << "overlap_check ==> PASS " << endl;
  else
    cout << "overlap_check ==> FAIL " << endl;
  return valid;
}

}  // namespace opendp
