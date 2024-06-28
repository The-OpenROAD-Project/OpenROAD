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
#include "odb/dbShape.h"

namespace dpl {

using std::to_string;

using utl::DPL;

using odb::dbMaster;
using odb::dbPlacementStatus;

using IRDropByPoint = std::map<odb::Point, double>;
using IRDropByLayer = std::map<odb::dbTechLayer*, IRDropByPoint>;

void Opendp::setDecapMaster(dbMaster* decap_master, double decap_cap)
{
  decap_masters_.push_back({decap_master, decap_cap});
}

vector<int> Opendp::getDecapCell(const int &gap_width, const double &current, const double &target)
{
  vector<int> id_masters;
  double acum = 0.0;
  int width_acum = 0;
  for (int i = 0; i < decap_masters_.size(); i++) {
    if ((gap_width - width_acum) >= decap_masters_[i].first->getWidth() && (acum + decap_masters_[i].second) <= (target - current)) {
      id_masters.push_back(i);
      acum += decap_masters_[i].second;
      width_acum += decap_masters_[i].first->getWidth();
    }
  }
  return id_masters;
}

odb::dbTechLayer* Opendp::getLowestLayer(odb::dbNet * db_net)
{
  int min_layer_level = std::numeric_limits<int>::max();
  std::vector<odb::dbShape> via_boxes;
  for (odb::dbSWire* swire : db_net->getSWires()) {
    for (odb::dbSBox* s : swire->getWires()) {
      if (s->isVia()) {
        s->getViaBoxes(via_boxes);
        for (const odb::dbShape& box : via_boxes) {
          odb::dbTechLayer* tech_layer = box.getTechLayer();
          if (tech_layer->getRoutingLevel() == 0) {
            continue;
          }
          if (min_layer_level == -1 || min_layer_level > tech_layer->getRoutingLevel()) {
            min_layer_level = tech_layer->getRoutingLevel();
          }
        }
      } else {
        odb::dbTechLayer* tech_layer = s->getTechLayer();
        if (min_layer_level == -1 || min_layer_level > tech_layer->getRoutingLevel()) {
          min_layer_level = tech_layer->getRoutingLevel();
        }
      }
    }
  }
  return db_->getTech()->findRoutingLayer(min_layer_level);
}

void Opendp::insertDecapCells(const double target)
{

  // init dpl variables
  if (cells_.empty()) {
    importDb();
  }

  decap_count_ = 0;
  initGrid();
  setGridCells();

  if (grid_->infoMapEmpty()) {
    logger_->error(DPL, 52, "Info map wasnt load to place DECAP cells");
  } 

  // Sort decaps cells in decrease order
  std::sort(decap_masters_.begin(),
            decap_masters_.end(),
            [](std::pair<dbMaster*,double>& decap_master1, std::pair<dbMaster*,double>& decap_master2) {
            return decap_master1.second > decap_master2.second;
            });

  // Get IR DROP of net VDD on layer met1
  IRDropByPoint ir_drop;
  odb::dbNet* db_net = block_->findNet("VDD");
  // Get lowest layer
  odb::dbTechLayer* tech_layer = getLowestLayer(db_net);

  psm_->getIRDropForLayer2(db_net, tech_layer, ir_drop);

  if (ir_drop.empty()) {
    logger_->error(DPL, 53, "Any IR DROP point found, run analyse_power_grid before of insert DECAP cells");
  }

  // Sort the IR DROP point ins decrease order
  std::vector<std::pair<odb::Point, double>> irdrop_points;
  for (auto & it: ir_drop) {
    irdrop_points.push_back({it.first, it.second});
  }

  std::sort(irdrop_points.begin(),
            irdrop_points.end(),
            [](std::pair<odb::Point, double>& point1, std::pair<odb::Point, double>& point2) {
              return point1.second > point2.second;
            });

  // If fillers are placed
  if (have_fillers_) {
    logger_->error(DPL, 54, "Filler cells found when running insert decap cells");
  }

  // Find gaps availables 
  findGaps();
  int gaps_count = 0;
  // Sort each gap vector for X position
  for (auto &it: gaps_){
    std::sort(it.second.begin(),
              it.second.end(),
              [](GapX& gap1, GapX& gap2) {
                return gap1.x < gap2.x;
              });
    gaps_count += it.second.size();
  }

  if (gaps_count == 0) {
    logger_->error(DPL, 55, "Gaps dont found during insert decap cells");
  }

  std::cerr << "Gaps count " << gaps_count << std::endl;

  double total_cap = 0.0;
  for (auto &irdrop_it: irdrop_points) {

    // Find gaps in same row
    auto it_gapY = gaps_.find(irdrop_it.first.getY());
    if (it_gapY != gaps_.end()) {
      // Find and insert decap in this row
        insertDecapInRow(it_gapY->second, it_gapY->first, irdrop_it.first.getX(), irdrop_it.first.getY(), total_cap, target);
    } 
    
    // if row is not first, then get lower row
    if (it_gapY != gaps_.begin()) {
      it_gapY--;
      // verify if row + height >= ypoint
      if (it_gapY->first + (it_gapY->second).begin()->height <= irdrop_it.first.getY()) {
        insertDecapInRow(it_gapY->second, it_gapY->first, irdrop_it.first.getX(), irdrop_it.first.getY(), total_cap, target);
      }
    }
  }
  logger_->info(DPL, 56, "Placed {} decap cells, cap total {:6.4f}", decap_count_, total_cap);
}

void Opendp::insertDecapInRow(const vector<GapX> &gaps, const int gap_y, const int irdrop_x, const int irdrop_y, double &total, const double &target)
{
  // Find gap in same row and x near the ir drop
  auto find_gap = std::lower_bound(gaps.begin(), gaps.end(),
                                   irdrop_x,
                                   [](const GapX &elem, const int &value) {
                                     return elem.x < value;
                                   }
                                  );
  // if this row has free gap with x <= xpoint <= x + width
  if (find_gap != gaps.end() && (find_gap->x + find_gap->width) >= irdrop_x && !find_gap->is_filled) { 
    auto ids = getDecapCell(find_gap->width, total, target);
    if (!ids.empty()) {
      gaps_[gap_y][find_gap - gaps.begin()].is_filled = true;
    }
    int gap_x = find_gap->x;
    for (const int &it_decap : ids) {
      // insert decap inst in this pos
      insertDecapInPos(decap_masters_[it_decap].first, find_gap->orient, gap_x, gap_y);

      gap_x += decap_masters_[it_decap].first->getWidth();
      total += decap_masters_[it_decap].second;
      decap_count_++;
    }
  }
}

void Opendp::insertDecapInPos(dbMaster* master, const odb::dbOrientType &orient, const int &pos_x, const int &pos_y)
{
  // insert decap inst
  string inst_name = "DECAP_" + to_string(decap_count_);
  dbInst* inst = dbInst::create(block_,
                                master,
                                inst_name.c_str(),
                                /* physical_only */ true);
  inst->setOrient(orient);
  inst->setLocation(pos_x, pos_y);
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  inst->setSourceType(odb::dbSourceType::DIST);      
}

void Opendp::findGaps()
{
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
      findGapsInRow(row,
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
      findGapsInRow(row,
                    row_height,
                    chosen_grid_info);
    }
  }
}

void Opendp::findGapsInRow(GridY row, DbuY row_height, const GridInfo& grid_info)
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
      const Rect core = grid_->getCore();
      // Save gap information (pos in dbu)
      DbuX gap_x{core.xMin() + gridToDbu(j, site_width)};
      DbuY gap_y{core.yMin() + gridToDbu(row, DbuY{row_height})};
      DbuX gap_width {gridToDbu(k, site_width) - gridToDbu(j, site_width)};
      gaps_[gap_y.v].push_back(GapX(gap_x.v, orient, gap_width.v, row_height.v));

      j += (k - j);
    }
    else {
      j++;
    }
  }
}

}  // namespace dpl
