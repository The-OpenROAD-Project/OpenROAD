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

#include <algorithm>

#include "dpl/Opendp.h"
#include "utl/Logger.h"

namespace dpl {

using std::to_string;

using utl::DPL;

using odb::dbMaster;
using odb::dbPlacementStatus;

void Opendp::fillerPlacement(dbMasterSeq* filler_masters, const char* prefix)
{
  if (cells_.empty()) {
    importDb();
  }

  std::sort(filler_masters->begin(),
            filler_masters->end(),
            [](dbMaster* master1, dbMaster* master2) {
              return master1->getWidth() > master2->getWidth();
            });

  gap_fillers_.clear();
  filler_count_ = 0;
  initGrid();
  setGridCells();

  if (!grid_info_map_.empty()) {
    int min_height = INT_MAX;
    Grid_map_key chosen_grid_key = {0};
    // we will first try to find the grid with min height that is non hybrid, if
    // that doesn't exist, we will pick the first hybrid grid.
    for (auto [grid_idx, itr_grid_info] : grid_info_map_) {
      int site_height = itr_grid_info.getSites()[0].first->getHeight();
      if (!itr_grid_info.isHybrid() && site_height < min_height) {
        min_height = site_height;
        chosen_grid_key = grid_idx;
      }
    }
    auto chosen_grid_info = grid_info_map_.at(chosen_grid_key);
    int chosen_row_count = chosen_grid_info.getRowCount();
    if (!chosen_grid_info.isHybrid()) {
      int site_height = min_height;
      for (int row = 0; row < chosen_row_count; row++) {
        placeRowFillers(
            row, prefix, filler_masters, site_height, chosen_grid_info);
      }
    } else {
      auto hybrid_sites_vec = chosen_grid_info.getSites();
      const int hybrid_sites_num = hybrid_sites_vec.size();
      for (int row = 0; row < chosen_row_count; row++) {
        placeRowFillers(
            row,
            prefix,
            filler_masters,
            hybrid_sites_vec[row % hybrid_sites_num].first->getHeight(),
            chosen_grid_info);
      }
    }
  }

  logger_->info(DPL, 1, "Placed {} filler instances.", filler_count_);
}

void Opendp::setGridCells()
{
  for (Cell& cell : cells_) {
    visitCellPixels(
        cell, false, [&](Pixel* pixel) { setGridCell(cell, pixel); });
  }
}

void Opendp::placeRowFillers(int row,
                             const char* prefix,
                             dbMasterSeq* filler_masters,
                             int row_height,
                             GridInfo grid_info)
{
  int j = 0;

  int row_site_count = divFloor(core_.dx(), site_width_);
  while (j < row_site_count) {
    Pixel* pixel = gridPixel(grid_info.getGridIndex(), j, row);
    const dbOrientType orient = pixel->orient_;
    if (pixel->cell == nullptr && pixel->is_valid) {
      int k = j;
      while (k < row_site_count
             && gridPixel(grid_info.getGridIndex(), k, row)->cell == nullptr
             && gridPixel(grid_info.getGridIndex(), k, row)->is_valid) {
        k++;
      }

      int gap = k - j;
      // printf("filling row %d gap %d %d:%d\n", row, gap, j, k - 1);
      dbMasterSeq& fillers = gapFillers(gap, filler_masters);
      if (fillers.empty()) {
        int x = core_.xMin() + j * site_width_;
        int y = core_.yMin() + row * row_height;
        logger_->error(
            DPL,
            2,
            "could not fill gap of size {} at {},{} dbu between {} and {}",
            gap,
            x,
            y,
            gridInstName(row, j - 1, row_height, grid_info),
            gridInstName(row, k + 1, row_height, grid_info));
      } else {
        k = j;
        debugPrint(
            logger_, DPL, "filler", 2, "fillers size is {}.", fillers.size());
        for (dbMaster* master : fillers) {
          string inst_name = prefix + to_string(grid_info.getGridIndex()) + "_"
                             + to_string(row) + "_" + to_string(k);
          // printf(" filler %s %d\n", inst_name.c_str(), master->getWidth() /
          // site_width_);
          dbInst* inst = dbInst::create(block_,
                                        master,
                                        inst_name.c_str(),
                                        /* physical_only */ true);
          int x = core_.xMin() + k * site_width_;
          int y = core_.yMin() + row * row_height;
          inst->setOrient(orient);
          inst->setLocation(x, y);
          inst->setPlacementStatus(dbPlacementStatus::PLACED);
          inst->setSourceType(odb::dbSourceType::DIST);
          filler_count_++;
          k += master->getWidth() / site_width_;
        }
        j += gap;
      }
    } else {
      j++;
    }
  }
}

const char* Opendp::gridInstName(int row,
                                 int col,
                                 int row_height,
                                 GridInfo grid_info)
{
  if (col < 0) {
    return "core_left";
  }
  if (col > grid_info.getSiteCount()) {
    return "core_right";
  }

  const Cell* cell = gridPixel(grid_info.getGridIndex(), col, row)->cell;
  if (cell) {
    return cell->db_inst_->getConstName();
  }
  return "?";
}

// Return list of masters to fill gap (in site width units).
dbMasterSeq& Opendp::gapFillers(int gap, dbMasterSeq* filler_masters)
{
  if (gap_fillers_.size() < gap + 1) {
    gap_fillers_.resize(gap + 1);
  }
  dbMasterSeq& fillers = gap_fillers_[gap];
  if (fillers.empty()) {
    int width = 0;
    dbMaster* smallest_filler = (*filler_masters)[filler_masters->size() - 1];
    bool have_filler1 = smallest_filler->getWidth() == site_width_;
    for (dbMaster* filler_master : *filler_masters) {
      int filler_width = filler_master->getWidth() / site_width_;
      while ((width + filler_width) <= gap
             && (have_filler1 || (width + filler_width) != gap - 1)) {
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

void Opendp::removeFillers()
{
  block_ = db_->getChip()->getBlock();
  for (odb::dbInst* db_inst : block_->getInsts()) {
    if (isFiller(db_inst)) {
      odb::dbInst::destroy(db_inst);
    }
  }
}

bool Opendp::isFiller(odb::dbInst* db_inst)
{
  dbMaster* db_master = db_inst->getMaster();
  return db_master->getType() == odb::dbMasterType::CORE_SPACER
         // Filter spacer cells used as tapcells.
         && db_inst->getPlacementStatus() != odb::dbPlacementStatus::LOCKED;
}

// Return true if cell is a single site Core Spacer.
bool Opendp::isOneSiteCell(odb::dbMaster* db_master) const
{
  return db_master->getType() == odb::dbMasterType::CORE_SPACER
         && db_master->getWidth() == site_width_;
}

}  // namespace dpl
