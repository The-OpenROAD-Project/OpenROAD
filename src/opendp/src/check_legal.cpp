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
      if(cell->y_coord % row_height_ != 0) {
	if (verbose)
	  cout << "row_check fail => " << cell->name()
	       << " y_coord : " << cell->y_coord << endl;
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
      if(cell->x_coord % site_width_ != 0) {
	if (verbose)
	  cout << "site check fail ==> " << cell->name()
	       << " x_coord : " << cell->x_coord << endl;
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
      Cell* grid_cell = grid_[i][j].linked_cell;
      if(grid_[i][j].is_valid) {
	if(grid_cell != NULL && grid_cell != &dummy_cell_) {
#ifdef ODP_DEBUG
	  cout << "grid util : " << grid[i][j].util << endl;
	  cout << "cell name : "
	       << grid[i][j].linked_cell->name() << endl;
#endif
	  if(cells.empty()) {
	    cells.push_back(grid_[i][j].linked_cell);
	  }
	  else if(cells[cells.size() - 1] != grid_[i][j].linked_cell) {
	    cells.push_back(grid_[i][j].linked_cell);
	  }
	}
      }
    }
#ifdef ODP_DEBUG
    cout << " row search done " << endl;
    cout << " cell list size : " << cells.size() << endl;
#endif
    if(!cells.empty()) {

      for(int k = 0; k < cells.size() - 1; k++) {
#ifdef ODP_DEBUG
	cout << " left cell : " << cells[k]->name() << endl;
	cout << " Right cell : " << cells[k + 1]->name() << endl;
#endif
	if(cells.size() >= 2) {
	  Macro* left_macro = cells[k]->cell_macro;
	  Macro* right_macro = cells[k + 1]->cell_macro;
	  if(left_macro->edgetypeRight != 0 && right_macro->edgetypeLeft != 0) {
	    // The following statement cannot be executed anymore because the
	    // edge_spacing map was hard coded in the obsolete LEF reader that
	    // predated the Si2 LEF reader.
	    int space = divRound(edge_spacing_[make_pair(left_macro->edgetypeRight,     
							 right_macro->edgetypeLeft)],
				 site_width_);
	    int cell_dist = cells[k + 1]->x_coord - cells[k]->x_coord -
	      cells[k]->width;
	    if(cell_dist < space) {
	      if (verbose)
		cout << " edge_check fail ==> " << cells[k]->name()
		     << " >> " << cell_dist << "(" << space << ") << "
		     << cells[k + 1]->name() << endl;
	      count++;
	    }
	  }
	}
      }
    }
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
       && !(cell.height == row_height_ || cell.height == row_height_ * 3)
       // should removed later
       && cell.inGroup()) {
      Macro* macro = cell.cell_macro;
      // This should probably be using gridHeight(cell).
      int y_size = divRound(cell.height, row_height_);
      int y_pos = gridNearestY(&cell);
      if(y_size % 2 == 0) {
	if(macro->top_power == rows_[y_pos].top_power) {
	  cout << "power check fail ( even height ) ==> "
	      << cell.name() << endl;
	  valid = false;
	  count++;
	}
      }
      else {
	if(macro->top_power == rows_[y_pos].top_power) {
	  if(cell.db_inst->getOrient() != dbOrientType::R0) {
	    cout << "power check fail ( Should be N ) ==> "
		<< cell.name() << endl;
	    valid = false;
	    count++;
	  }
	}
	else {
	  if(cell.db_inst->getOrient() != dbOrientType::MX) {
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
    if(!cell.is_placed) {
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
      grid2[i][j].y_pos = i;
      grid2[i][j].x_pos = j;
      grid2[i][j].linked_cell = NULL;
    }
  }

  for(Cell& cell : cells_) {
    int x_pos = gridNearestX(&cell);
    int y_pos = gridNearestY(&cell);
    int x_step = gridWidth(&cell);
    int y_step = gridHeight(&cell);

    int x_ur = x_pos + x_step;
    int y_ur = y_pos + y_step;

    // Fixed Cell can be out of Current DIEAREA settings.
    if(isFixed(&cell)) {
      x_pos = max(0, x_pos);
      y_pos = max(0, y_pos);
      x_ur = min(x_ur, col_count);
      y_ur = min(y_ur, row_count);
    }

    assert(x_pos > -1);
    assert(y_pos > -1);
    assert(x_step > 0);
    assert(y_step > 0);
    assert(x_ur <= coreGridMaxX());
    assert(y_ur <= coreGridMaxY());

    for(int j = y_pos; j < y_ur; j++) {
      for(int k = x_pos; k < x_ur; k++) {
        if(grid2[j][k].linked_cell == NULL) {
          grid2[j][k].linked_cell = &cell;
          grid2[j][k].util = 1.0;
        }
        else {
	  if (verbose)
	    cout << "overlap_check ==> FAIL ( cell " << cell.name()
		 << " overlaps " << grid2[j][k].linked_cell->name() << " ) "
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
