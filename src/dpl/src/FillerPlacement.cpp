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

#include "Grid.h"
#include "Objects.h"
#include "dpl/Opendp.h"
#include "utl/Logger.h"

namespace dpl {

using std::to_string;

using utl::DPL;

using odb::dbMaster;
using odb::dbPlacementStatus;

using utl::format_as;

static dbTechLayer* getImplant(dbMaster* master)
{
  if (!master) {
    return nullptr;
  }

  for (auto obs : master->getObstructions()) {
    auto layer = obs->getTechLayer();
    if (layer->getType() == odb::dbTechLayerType::IMPLANT) {
      return layer;
    }
  }
  return nullptr;
}

Opendp::MasterByImplant Opendp::splitByImplant(dbMasterSeq* filler_masters)
{
  MasterByImplant mapping;
  for (auto master : *filler_masters) {
    mapping[getImplant(master)].emplace_back(master);
  }

  return mapping;
}

void Opendp::fillerPlacement(dbMasterSeq* filler_masters, const char* prefix)
{
  if (cells_.empty()) {
    importDb();
  }

  auto filler_masters_by_implant = splitByImplant(filler_masters);

  for (auto& [layer, masters] : filler_masters_by_implant) {
    std::sort(masters.begin(),
              masters.end(),
              [](dbMaster* master1, dbMaster* master2) {
                return master1->getWidth() > master2->getWidth();
              });
  }

  gap_fillers_.clear();
  filler_count_ = 0;
  initGrid();
  setGridCells();

  if (!grid_->infoMapEmpty()) {
    DbuY min_height{std::numeric_limits<int>::max()};
    GridMapKey chosen_grid_key = {0};
    // we will first try to find the grid with min height that is non hybrid, if
    // that doesn't exist, we will pick the first hybrid grid.
    for (auto [grid_idx, itr_grid_info] : grid_->getInfoMap()) {
      dbSite* site = itr_grid_info.getSites()[0].site;
      DbuY site_height{static_cast<int>(site->getHeight())};
      if (!itr_grid_info.isHybrid() && site_height < min_height) {
        min_height = site_height;
        chosen_grid_key = grid_idx;
      }
    }
    const auto& chosen_grid_info = grid_->getInfoMap().at(chosen_grid_key);
    GridY chosen_row_count = chosen_grid_info.getRowCount();
    if (!chosen_grid_info.isHybrid()) {
      DbuY site_height = min_height;
      for (GridY row{0}; row < chosen_row_count; row++) {
        placeRowFillers(row,
                        prefix,
                        filler_masters_by_implant,
                        site_height,
                        chosen_grid_info);
      }
    } else {
      const auto& hybrid_sites_vec = chosen_grid_info.getSites();
      const int hybrid_sites_num = hybrid_sites_vec.size();
      for (GridY row{0}; row < chosen_row_count; row++) {
        const int index = row.v % hybrid_sites_num;
        dbSite* site = hybrid_sites_vec[index].site;
        DbuY row_height{static_cast<int>(site->getHeight())};
        placeRowFillers(row,
                        prefix,
                        filler_masters_by_implant,
                        row_height,
                        chosen_grid_info);
      }
    }
  }

  logger_->info(DPL, 1, "Placed {} filler instances.", filler_count_);
}

void Opendp::setGridCells()
{
  for (Cell& cell : cells_) {
    grid_->visitCellPixels(
        cell, false, [&](Pixel* pixel) { setGridCell(cell, pixel); });
  }
}

void Opendp::placeRowFillers(GridY row,
                             const char* prefix,
                             const MasterByImplant& filler_masters_by_implant,
                             DbuY row_height,
                             const GridInfo& grid_info)
{
  GridX j{0};

  const DbuX site_width = grid_->getSiteWidth();
  GridX row_site_count{divFloor(grid_->getCore().dx(), site_width.v)};
  while (j < row_site_count) {
    Pixel* pixel = grid_->gridPixel(grid_info.getGridIndex(), j, row);
    const dbOrientType orient = pixel->orient_;
    if (pixel->cell == nullptr && pixel->is_valid) {
      GridX k = j;
      while (k < row_site_count
             && grid_->gridPixel(grid_info.getGridIndex(), k, row)->cell
                    == nullptr
             && grid_->gridPixel(grid_info.getGridIndex(), k, row)->is_valid) {
        k++;
      }

      dbTechLayer* implant = nullptr;
      if (j > 0) {
        auto pixel = grid_->gridPixel(grid_info.getGridIndex(), j - 1, row);
        if (pixel->cell && pixel->cell->db_inst_) {
          implant = getImplant(pixel->cell->db_inst_->getMaster());
        }
      } else if (k < row_site_count) {
        auto pixel = grid_->gridPixel(grid_info.getGridIndex(), k, row);
        if (pixel->cell && pixel->cell->db_inst_) {
          implant = getImplant(pixel->cell->db_inst_->getMaster());
        }
      } else {  // totally empty row - use anything
        implant = filler_masters_by_implant.begin()->first;
      }

      GridX gap = k - j;
      dbMasterSeq& fillers
          = gapFillers(implant, gap, filler_masters_by_implant);
      const Rect core = grid_->getCore();
      if (fillers.empty()) {
        DbuX x{core.xMin() + gridToDbu(j, site_width)};
        DbuY y{core.yMin() + gridToDbu(row, DbuY{row_height})};
        logger_->error(
            DPL,
            2,
            "could not fill gap of size {} at {},{} dbu between {} and {}",
            gap,
            x,
            y,
            gridInstName(row, j - 1, grid_info),
            gridInstName(row, k + 1, grid_info));
      } else {
        k = j;
        debugPrint(
            logger_, DPL, "filler", 2, "fillers size is {}.", fillers.size());
        for (dbMaster* master : fillers) {
          string inst_name = prefix + to_string(grid_info.getGridIndex()) + "_"
                             + to_string(row.v) + "_" + to_string(k.v);
          dbInst* inst = dbInst::create(block_,
                                        master,
                                        inst_name.c_str(),
                                        /* physical_only */ true);
          DbuX x{core.xMin() + gridToDbu(k, site_width)};
          DbuY y{core.yMin() + gridToDbu(row, DbuY{row_height})};
          inst->setOrient(orient);
          inst->setLocation(x.v, y.v);
          inst->setPlacementStatus(dbPlacementStatus::PLACED);
          inst->setSourceType(odb::dbSourceType::DIST);
          filler_count_++;
          k += master->getWidth() / site_width.v;
        }
        j += gap;
      }
    } else {
      j++;
    }
  }
}

const char* Opendp::gridInstName(GridY row,
                                 GridX col,
                                 const GridInfo& grid_info)
{
  if (col < 0) {
    return "core_left";
  }
  if (col > grid_info.getSiteCount()) {
    return "core_right";
  }

  const Cell* cell = grid_->gridPixel(grid_info.getGridIndex(), col, row)->cell;
  if (cell) {
    return cell->db_inst_->getConstName();
  }
  return "?";
}

// Return list of masters to fill gap (in site width units).
dbMasterSeq& Opendp::gapFillers(
    dbTechLayer* implant,
    GridX gap,
    const MasterByImplant& filler_masters_by_implant)
{
  auto iter = filler_masters_by_implant.find(implant);
  if (iter == filler_masters_by_implant.end()) {
    logger_->error(DPL, 50, "No fillers found for {}.", implant->getName());
  }
  const dbMasterSeq& filler_masters = iter->second;

  GapFillers& gap_fillers = gap_fillers_[implant];
  if (gap_fillers.size() < gap + 1) {
    gap_fillers.resize(gap.v + 1);
  }
  dbMasterSeq& fillers = gap_fillers[gap.v];
  if (fillers.empty()) {
    int width = 0;
    dbMaster* smallest_filler = filler_masters[filler_masters.size() - 1];
    const DbuX site_width = grid_->getSiteWidth();
    bool have_filler1 = smallest_filler->getWidth() == site_width;
    for (dbMaster* filler_master : filler_masters) {
      int filler_width = filler_master->getWidth() / site_width.v;
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
  for (dbInst* db_inst : block_->getInsts()) {
    if (isFiller(db_inst)) {
      odb::dbInst::destroy(db_inst);
    }
  }
}

/* static */
bool Opendp::isFiller(dbInst* db_inst)
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
         && db_master->getWidth() == grid_->getSiteWidth();
}

}  // namespace dpl
