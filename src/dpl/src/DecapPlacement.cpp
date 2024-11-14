/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Precision Innovations Inc.
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

#include "DecapObjects.h"
#include "Objects.h"
#include "Padding.h"
#include "dpl/Opendp.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace dpl {

using std::to_string;

using utl::DPL;

using odb::dbMaster;
using odb::dbPlacementStatus;

using IRDropByLayer = std::map<odb::dbTechLayer*, IRDropByPoint>;

void Opendp::addDecapMaster(dbMaster* decap_master, double decap_cap)
{
  decap_masters_.emplace_back(new DecapCell(decap_master, decap_cap));
}

// Return list of decap indices to fill gap
vector<int> Opendp::findDecapCellIndices(const DbuX& gap_width,
                                         const double& current,
                                         const double& target)
{
  vector<int> id_masters;
  double cap_acum = 0.0;
  int width_acum = 0;
  const DbuX site_width = grid_->getSiteWidth();
  const DbuX min_space = gridToDbu(
      padding_->padGlobalRight() + padding_->padGlobalLeft(), site_width);
  for (int i = 0; i < decap_masters_.size(); i++) {
    const int master_width = decap_masters_[i]->master->getWidth();
    const double master_cap = decap_masters_[i]->capacitance;
    while ((width_acum + master_width) <= (gap_width - min_space.v)
           && (cap_acum + master_cap) <= (target - current)) {
      id_masters.push_back(i);
      cap_acum += master_cap;
      width_acum += master_width;
      if (width_acum == gap_width) {
        return id_masters;
      }
    }
  }
  return id_masters;
}

// Return IR Drops for the net name in the lowest layer
void Opendp::mapToVectorIRDrops(IRDropByPoint& psm_ir_drops,
                                std::vector<IRDrop>& ir_drops)
{
  // Sort the IR DROP point in descending order
  for (auto& it : psm_ir_drops) {
    const odb::Point pt = it.first;
    DbuPt dbu_pt{DbuX{pt.getX()}, DbuY{pt.getY()}};
    ir_drops.emplace_back(dbu_pt, it.second);
  }

  std::sort(ir_drops.begin(),
            ir_drops.end(),
            [](const IRDrop& ir_drop1, const IRDrop& ir_drop2) {
              return ir_drop1.value > ir_drop2.value;
            });
}

void Opendp::prepareDecapAndGaps()
{
  // Sort decaps cells in descending order
  std::sort(decap_masters_.begin(),
            decap_masters_.end(),
            [](const DecapCell* decap1, const DecapCell* decap2) {
              return decap1->capacitance > decap2->capacitance;
            });

  // Find gaps availables
  findGaps();
  int gaps_count = 0;
  // Sort each gap vector for X position
  for (auto& it : gaps_) {
    std::sort(it.second.begin(),
              it.second.end(),
              [](const GapInfo* gap1, const GapInfo* gap2) {
                return gap1->x < gap2->x;
              });
    gaps_count += it.second.size();
  }

  if (gaps_count == 0) {
    logger_->error(DPL, 55, "Gaps not found when inserting decap cells.");
  }
}

void Opendp::insertDecapCells(const double target, IRDropByPoint& psm_ir_drops)
{
  // init dpl variables
  if (cells_.empty()) {
    importDb();
  }

  double total_cap = 0.0;
  decap_count_ = 0;
  initGrid();
  setGridCells();

  // If fillers are placed
  if (have_fillers_) {
    logger_->error(DPL, 54, "Run remove_fillers before inserting decap cells");
  }

  // Sort Decap cells and Gaps
  prepareDecapAndGaps();

  // Get IR DROP of net VDD on the lowest layer
  std::vector<IRDrop> ir_drops;
  mapToVectorIRDrops(psm_ir_drops, ir_drops);

  for (auto& irdrop_it : ir_drops) {
    // Find gaps in same row
    auto it_gapY = gaps_.find(irdrop_it.position.y);
    if (it_gapY != gaps_.end()) {
      // Find and insert decap in this row
      insertDecapInRow(it_gapY->second,
                       it_gapY->first,
                       irdrop_it.position.x,
                       irdrop_it.position.y,
                       total_cap,
                       target);
    }

    // if row is not first, then get lower row
    if (it_gapY != gaps_.begin()) {
      it_gapY--;
      // verify if row + height >= ypoint
      if (it_gapY->first + (*(it_gapY->second).begin())->height
          <= irdrop_it.position.y) {
        insertDecapInRow(it_gapY->second,
                         it_gapY->first,
                         irdrop_it.position.x,
                         irdrop_it.position.y,
                         total_cap,
                         target);
      }
    }
  }

  logger_->info(DPL,
                56,
                "Placed {} decap cells. Total capacitance: {:6.6f}",
                decap_count_,
                total_cap);
}

void Opendp::insertDecapInRow(const vector<GapInfo*>& gaps,
                              const DbuY gap_y,
                              const DbuX irdrop_x,
                              const DbuY irdrop_y,
                              double& total,
                              const double& target)
{
  // Find gap in same row and x near the ir drop
  auto find_gap = std::lower_bound(
      gaps.begin(),
      gaps.end(),
      irdrop_x,
      [](const GapInfo* elem, const DbuX& value) { return elem->x < value; });
  // if this row has free gap with x <= xpoint <= x + width
  if (find_gap != gaps.end()
      && ((*find_gap)->x + (*find_gap)->width) >= irdrop_x
      && !(*find_gap)->is_filled) {
    auto ids = findDecapCellIndices((*find_gap)->width, total, target);
    if (!ids.empty()) {
      gaps_[gap_y][find_gap - gaps.begin()]->is_filled = true;
    }
    DbuX gap_x = (*find_gap)->x;
    for (const int& it_decap : ids) {
      // insert decap inst in this pos
      insertDecapInPos(decap_masters_[it_decap]->master, gap_x, gap_y);

      gap_x += decap_masters_[it_decap]->master->getWidth();
      total += decap_masters_[it_decap]->capacitance;
      decap_count_++;
    }
  }
}

void Opendp::insertDecapInPos(dbMaster* master,
                              const DbuX& pos_x,
                              const DbuY& pos_y)
{
  // insert decap inst
  string inst_name = "DECAP_" + to_string(decap_count_);
  dbInst* inst = dbInst::create(block_,
                                master,
                                inst_name.c_str(),
                                /* physical_only */ true);
  const Rect core = grid_->getCore();
  const GridX grid_x = grid_->gridX(pos_x - core.xMin());
  const GridY grid_y = grid_->gridSnapDownY(pos_y - core.yMin());
  const Pixel* pixel = grid_->gridPixel(grid_x, grid_y);
  const dbOrientType orient = pixel->sites.at(master->getSite());
  inst->setOrient(orient);
  inst->setLocation(pos_x.v, pos_y.v);
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  inst->setSourceType(odb::dbSourceType::DIST);
}

void Opendp::findGaps()
{
  GridY chosen_row_count = grid_->getRowCount();
  for (GridY row{0}; row < chosen_row_count; row++) {
    DbuY site_height = grid_->rowHeight(row);
    findGapsInRow(row, site_height);
  }
}

void Opendp::findGapsInRow(GridY row, DbuY row_height)
{
  const DbuX site_width = grid_->getSiteWidth();
  const GridX row_site_count = grid_->getRowSiteCount();
  GridX j{0};
  while (j < row_site_count) {
    Pixel* pixel = grid_->gridPixel(j, row);
    if (pixel->cell == nullptr && pixel->is_valid) {
      GridX k = j;
      while (k < row_site_count && grid_->gridPixel(k, row)->cell == nullptr
             && grid_->gridPixel(k, row)->is_valid) {
        k++;
      }
      const Rect core = grid_->getCore();
      // Save gap information (pos in dbu)
      DbuX gap_x{core.xMin() + gridToDbu(j, site_width)};
      DbuY gap_y{core.yMin() + gridToDbu(row, row_height)};
      DbuX gap_width{gridToDbu(k, site_width) - gridToDbu(j, site_width)};
      gaps_[gap_y].emplace_back(new GapInfo(gap_x, gap_width, row_height));

      j += (k - j);
    } else {
      j++;
    }
  }
}

}  // namespace dpl
