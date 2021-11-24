/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "dpl/Opendp.h"

#include <algorithm>
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;
using std::to_string;

using utl::DPL;

using odb::dbLib;
using odb::dbMaster;
using odb::dbPlacementStatus;

void
Opendp::fillerPlacement(dbMasterSeq *filler_masters,
                        const char* prefix)
{
  if (cells_.empty())
    importDb();

  std::sort(filler_masters->begin(),
            filler_masters->end(),
            [](dbMaster *master1, dbMaster *master2) {
              return master1->getWidth() > master2->getWidth();
            });

  gap_fillers_.clear();
  filler_count_ = 0;
  initGrid();
  setGridCells();

  for (int row = 0; row < row_count_; row++)
    placeRowFillers(row, prefix, filler_masters);

  logger_->info(DPL, 1, "Placed {} filler instances.", filler_count_);
}

void
Opendp::setGridCells()
{
  for (Cell &cell : cells_)
    visitCellPixels(cell, false,
                    [&] (Pixel *pixel) { setGridCell(cell, pixel); } );
}

void
Opendp::placeRowFillers(int row,
                        const char* prefix,
                        dbMasterSeq *filler_masters)
{
  dbOrientType orient = rowOrient(row);
  int j = 0;
  while (j < row_site_count_) {
    Pixel *pixel = gridPixel(j, row);
    if (pixel->cell == nullptr
        && pixel->is_valid) {
      int k = j;
      while (k < row_site_count_
             && gridPixel(k, row)->cell == nullptr
             && gridPixel(k, row)->is_valid) {
        k++;
      }
      int gap = k - j;
      // printf("filling row %d gap %d %d:%d\n", row, gap, j, k - 1);
      dbMasterSeq &fillers = gapFillers(gap, filler_masters);
      if (fillers.empty()) {
        int x = core_.xMin() + j * site_width_;
        int y = core_.yMin() + row * row_height_;
        logger_->error(DPL, 2,
                       "could not fill gap of size {} at {},{} dbu between {} and {}",
                       gap, x, y,
                       gridInstName(row, j - 1),
                       gridInstName(row, k + 1));
      }
      else {
        k = j;
        for (dbMaster *master : fillers) {
          string inst_name = prefix + to_string(row) + "_" + to_string(k);
          // printf(" filler %s %d\n", inst_name.c_str(), master->getWidth() /
          // site_width_);
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
    }
    else {
      j++;
    }
  }
}

const char *
Opendp::gridInstName(int row,
                     int col)
{
  if (col < 0)
    return "core_left";
  else if (col > row_site_count_)
    return "core_right";
  else {
    const Cell *cell = gridPixel(col, row)->cell;
    if (cell)
      return cell->db_inst_->getConstName();
  }
  return "?";
}

// Return list of masters to fill gap (in site width units).
dbMasterSeq &
Opendp::gapFillers(int gap,
                   dbMasterSeq *filler_masters)
{
  if (gap_fillers_.size() < gap + 1) {
    gap_fillers_.resize(gap + 1);
  }
  dbMasterSeq &fillers = gap_fillers_[gap];
  if (fillers.empty()) {
    int width = 0;
    dbMaster *smallest_filler = (*filler_masters)[filler_masters->size() - 1];
    bool have_filler1 = smallest_filler->getWidth() == site_width_;
    for (dbMaster *filler_master : *filler_masters) {
      int filler_width = filler_master->getWidth() / site_width_;
      while ((width + filler_width) <= gap
             && (have_filler1
                 || (width + filler_width) != gap - 1)) {
        fillers.push_back(filler_master);
        width += filler_width;
        if (width == gap) {
          return fillers;
        }
      }
    }
    // Fail. Return empty fillers.
    fillers.clear();
  }
  return fillers;
}

}  // namespace opendp
