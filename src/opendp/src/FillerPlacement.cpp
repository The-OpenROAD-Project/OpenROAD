/////////////////////////////////////////////////////////////////////////////
// James Cherry, Parallax Software, Inc.
//
// BSD 3-Clause License
//
// Copyright (c) 2020, James Cherry, Parallax Software, Inc.
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

#include <algorithm>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::vector;
using std::to_string;
using std::max;
using std::min;
using std::cout;
using std::endl;

using ord::error;

using odb::dbMaster;
using odb::dbLib;
using odb::dbPlacementStatus;

void
Opendp::fillerPlacement(StringSeq *filler_master_names)
{
  if (cells_.empty())
    importDb();

  findFillerMasters(filler_master_names);
  gap_fillers_.clear();
  filler_count_ = 0;
  Grid *grid = makeCellGrid();
  for(int row = 0; row < row_count_; row++)
    placeRowFillers(grid, row);
  cout << "Placed " << to_string(filler_count_) << " filler instances." << endl;
}

void
Opendp::findFillerMasters(StringSeq *filler_master_names)
{
  filler_masters_.clear();
  for(string &master_name : *filler_master_names) {
    for (dbLib *lib : db_->getLibs()) {
      dbMaster *master = lib->findMaster(master_name.c_str());
      if (master) {
	filler_masters_.push_back(master);
	break;
      }
    }
  }
  std::sort(filler_masters_.begin(), filler_masters_.end(),
       [](dbMaster *master1,
	  dbMaster *master2) {
	 return master1->getWidth() > master2->getWidth();
       });
}

Grid *
Opendp::makeCellGrid()
{
  Grid *grid = makeGrid();

  for(Cell& cell : cells_) {
    int grid_x = gridX(&cell);
    int grid_y = gridY(&cell);

    int x_ur = gridEndX(&cell);
    int y_ur = gridEndY(&cell);

    // Don't barf if cell is outside the core.
    grid_x = max(0, grid_x);
    grid_y = max(0, grid_y);
    x_ur = min(x_ur, row_site_count_);
    y_ur = min(y_ur, row_count_);

    for(int j = grid_y; j < y_ur; j++) {
      for(int k = grid_x; k < x_ur; k++) {
	grid[j][k].cell = &cell;
      }
    }
  }
  return grid;
}

void
Opendp::placeRowFillers(Grid *grid,
			int row)
{
  dbOrientType orient = rowOrient(row);
  int j = 0;
  while (j < row_site_count_) {
    if (grid[row][j].cell == nullptr) {
      int k = j;
      while (grid[row][k].cell == nullptr && k < row_site_count_)
	k++;
      int gap = k - j;
      //printf("filling row %d gap %d %d:%d\n", row, gap, j, k - 1);
      dbMasterSeq &fillers = gapFillers(gap);
      k = j;
      for (dbMaster *master : fillers) {
	string inst_name = "FILLER_" + to_string(row) + "_" + to_string(k);
	//printf(" filler %s %d\n", inst_name.c_str(), master->getWidth() / site_width_);
	dbInst *inst = dbInst::create(block_, master, inst_name.c_str());
	int x = core_.xMin() + k * site_width_;
	int y = core_.yMin() + row * row_height_;
	inst->setOrient(orient);
	inst->setLocation(x, y);
	inst->setPlacementStatus(dbPlacementStatus::PLACED);
	filler_count_++;
	k += master->getWidth() / site_width_;
      }
      j += gap;
    }
    else
      j++;
  }
}

// return list of masters to fill gap (in site width units)
dbMasterSeq &
Opendp::gapFillers(int gap)
{
  if (gap_fillers_.size() < gap + 1)
    gap_fillers_.resize(gap + 1);
  dbMasterSeq &fillers = gap_fillers_[gap];
  if (fillers.empty()) {
    int width = 0;
    dbMaster *smallest_filler = filler_masters_[filler_masters_.size() - 1];
    bool have_filler1 = smallest_filler->getWidth() == site_width_;
    for (dbMaster *filler_master : filler_masters_) {
      int filler_width = filler_master->getWidth() / site_width_;
      while ((width + filler_width) <= gap
	     && (have_filler1
		 || (width + filler_width) != gap - 1)) {
	fillers.push_back(filler_master);
	width += filler_width;
	if (width == gap)
	  return fillers;
      }
    }
    error("could not fill gap of size %d", gap);
    return fillers;
  }
  else
    return fillers;
}

}  // namespace opendp
