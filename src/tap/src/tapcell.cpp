/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

#include "tap/tapcell.h"

#include <map>
#include <string>
#include <utility>

#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace tap {

using std::max;
using std::min;
using std::string;
using std::vector;

Tapcell::Tapcell() : 
  db_(nullptr),
  logger_(nullptr),
  phy_idx_(0),
  tap_prefix_(""),
  endcap_prefix_("")
{
}

Tapcell::~Tapcell()
{
}

void Tapcell::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void Tapcell::reset()
{
  Tapcell::phy_idx_ = 0;
  Tapcell::filled_sites_.clear();
  tap_prefix_.clear();
  endcap_prefix_.clear();
}

//---------------------------------------------------------------

void Tapcell::setTapPrefix(const string& tap_prefix) {
  tap_prefix_ = tap_prefix;
}

void Tapcell::setEndcapPrefix(const string& endcap_prefix) {
  endcap_prefix_ = endcap_prefix;
}

void Tapcell::clear()
{
  int taps_removed = removeCells(tap_prefix_);
  int endcaps_removed = removeCells(endcap_prefix_);

  // Reset global parameters
  reset();
}

void Tapcell::run(odb::dbMaster* endcap_master,
                  int halo_x,
                  int halo_y,
                  const string& cnrcap_nwin_master,
                  const string& cnrcap_nwout_master,
                  int add_boundary_cell,
                  const string& tap_nwintie_master,
                  const string& tap_nwin2_master,
                  const string& tap_nwin3_master,
                  const string& tap_nwouttie_master,
                  const string& tap_nwout2_master,
                  const string& tap_nwout3_master,
                  const string& incnrcap_nwin_master,
                  const string& incnrcap_nwout_master,
                  const string& tapcell_master,
                  int dist)
{
  vector<odb::dbBox*> blockages = findBlockages();
  cutRows(endcap_master, blockages, halo_x, halo_y);
  vector<vector<odb::dbRow*>> rows = organizeRows();
  cnrcap_masters masters;
  masters.nwin_master = cnrcap_nwin_master;
  masters.nwout_master=cnrcap_nwout_master;

  if (endcap_master != nullptr) {
    insertEndcaps(rows, endcap_master, masters);
  }
  if (add_boundary_cell) {
    vector<string> tap_nw_masters;
    tap_nw_masters.push_back(tap_nwintie_master);
    tap_nw_masters.push_back(tap_nwin2_master);
    tap_nw_masters.push_back(tap_nwin3_master);
    tap_nw_masters.push_back(tap_nwouttie_master);
    tap_nw_masters.push_back(tap_nwout2_master);
    tap_nw_masters.push_back(tap_nwout3_master);

    vector<string> tap_macro_masters;
    tap_macro_masters.push_back(incnrcap_nwin_master);
    tap_macro_masters.push_back(tap_nwin2_master);
    tap_macro_masters.push_back(tap_nwin3_master);
    tap_macro_masters.push_back(tap_nwintie_master);
    tap_macro_masters.push_back(incnrcap_nwout_master);
    tap_macro_masters.push_back(tap_nwout2_master);
    tap_macro_masters.push_back(tap_nwout3_master);
    tap_macro_masters.push_back(tap_nwouttie_master);

    insertAtTopBottom(rows,
                      tap_nw_masters,
                      db_->findMaster(cnrcap_nwin_master.c_str()),
                      endcap_prefix_);
    insertAroundMacros(rows,
                       tap_macro_masters,
                       db_->findMaster(cnrcap_nwin_master.c_str()),
                       endcap_prefix_);
  }
  string tapcell_master_string(tapcell_master);
  insertTapcells(rows, tapcell_master_string, dist);
  filled_sites_.clear();
}

void Tapcell::cutRows(odb::dbMaster* endcap_master,
                      const vector<odb::dbBox*>& blockages,
                      int halo_x,
                      int halo_y)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  const int rows_count = (block->getRows()).size();
  const int block_count = blockages.size();

  const int min_row_width = (endcap_master!=nullptr) ? 2*endcap_master->getWidth(): 0;

  // Gather rows needing to be cut up front
  vector<odb::dbRow*> blocked_rows;
  std::map<odb::dbRow*, vector<odb::dbBox*>> row_blockages;
  for (odb::dbBox* blockage : blockages) {
    for (odb::dbRow* row : block->getRows()) {
      if (overlaps(blockage, row, halo_x, halo_y)) {
        if (row_blockages.find(row) == row_blockages.end()) {
           blocked_rows.push_back(row);
        }
        row_blockages[row].push_back(blockage);
      }
    }
  }

  //Cut rows around macros
  for(auto&[k,ignored]:row_blockages) {
    cutRow(block, k, row_blockages[k], min_row_width, halo_x, halo_y);
  }
  const int cut_rows_count = block->getRows().size() - rows_count;
  logger_->info(utl::TAP, 1, "Found {} macro blocks.", block_count);
  logger_->info(utl::TAP, 2, "Original rows: {}", rows_count);
  logger_->info(utl::TAP, 3, "Created {} rows for a total of {} rows.", cut_rows_count, cut_rows_count+rows_count);
}

void Tapcell::cutRow(odb::dbBlock* block,
                     odb::dbRow* row,
                     vector<odb::dbBox*>& row_blockages,
                     int min_row_width,
                     int halo_x,
                     int halo_y)
{
  string row_name = row->getName();
  odb::Rect row_bb;
  row->getBBox(row_bb);

  odb::dbSite* row_site = row->getSite();
  const int site_width = row_site->getWidth();
  odb::dbOrientType orient = row->getOrient();
  odb::dbRowDir direction = row->getDirection();

  const int curr_min_row_width = min_row_width + 2 * site_width;

  vector<odb::dbBox*> row_blockage_bboxs = row_blockages;
  vector<std::pair<int, int>> row_blockage_xs;
  for (odb::dbBox* row_blockage_bbox : row_blockages) {
    row_blockage_xs.push_back(
        std::make_pair(row_blockage_bbox->xMin(), row_blockage_bbox->xMax()));
  }

  std::sort(row_blockage_xs.begin(), row_blockage_xs.end());

  int start_origin_x = row_bb.xMin();
  int start_origin_y = row_bb.yMin();
  int row_sub_idx = 1;
  for (std::pair<int, int> blockage : row_blockage_xs) {
    const int blockage_x0 = blockage.first;
    const int new_row_end_x
        = makeSiteLoc(blockage_x0 - halo_x, site_width, 1, start_origin_x);
    buildRow(block,
             row_name + "_" + std::to_string(row_sub_idx),
             row_site,
             start_origin_x,
             new_row_end_x,
             start_origin_y,
             orient,
             direction,
             curr_min_row_width);
    row_sub_idx++;
    const int blockage_x1 = blockage.second;
    start_origin_x
        = makeSiteLoc(blockage_x1 + halo_x, site_width, 0, start_origin_x);
  }
  // Make last row
  buildRow(block,
           row_name + "_" + std::to_string(row_sub_idx),
           row_site,
           start_origin_x,
           row_bb.xMax(),
           start_origin_y,
           orient,
           direction,
           curr_min_row_width);
  // Remove current row
  odb::dbRow::destroy(row);
}

int Tapcell::insertEndcaps(const vector<vector<odb::dbRow*>>& rows,
                           odb::dbMaster* endcap_master,
                           const cnrcap_masters& masters)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbMaster* cnrcap_nwin_master;
  odb::dbMaster* cnrcap_nwout_master;

  bool do_corners;
  if (!masters.nwin_master.empty() && !masters.nwout_master.empty())
  {
    if (masters.nwin_master.compare("INVALID") == 0
        || masters.nwout_master.compare("INVALID") == 0) {
      do_corners = 0;
    } else {
      do_corners = 1;

      cnrcap_nwin_master = db_->findMaster(masters.nwin_master.c_str());
      if (cnrcap_nwin_master == nullptr) {
        logger_->error(
            utl::TAP, 12, "Master {} not found.", masters.nwin_master);
      }
      cnrcap_nwout_master = db_->findMaster(masters.nwout_master.c_str());

      if (cnrcap_nwout_master == nullptr) {
        logger_->error(
            utl::TAP, 13, "Master {} not found.", masters.nwout_master);
      }
    }
  } else {
    do_corners = 0;
  }

  const int bottom_row = 0;
  const int top_row = rows.size() - 1;

  for (int cur_row = bottom_row; cur_row <= top_row; cur_row++) {
    for (odb::dbRow* subrow : rows[cur_row]) {
      if (!(checkSymmetry(endcap_master, subrow->getOrient()))) {
        continue;
      }
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      auto row_ori = subrow->getOrient();

      const int llx = row_bb.xMin();
      const int urx = row_bb.xMax();

      odb::dbMaster* left_master;
      odb::dbMaster* right_master;

      if (do_corners) {
        if (cur_row == top_row) {
          left_master = pickCornerMaster(BelowMacro,
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          right_master = left_master;
        } else if (cur_row == bottom_row) {
          left_master = pickCornerMaster(AboveMacro,
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          right_master = left_master;
        } else {
          auto rows_above = rows[cur_row + 1];
          auto rows_below = rows[cur_row - 1];
          left_master = pickCornerMaster(getLocationType(llx, rows_above, rows_below),
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          right_master = pickCornerMaster(getLocationType(urx, rows_above, rows_below),
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
        }
      } else {
        left_master = endcap_master;
        right_master = left_master;
      }

      if ((left_master->getWidth()) > (urx - llx)) {
        continue;
      }

      const int lly = row_bb.yMin();
      makeInstance(block, left_master, row_ori, llx, lly, endcap_prefix_);

      const int master_x = right_master->getWidth();
      const int master_y = right_master->getHeight();

      const int loc_2_x = urx - master_x;
      const int ury = row_bb.yMax();
      const int loc_2_y = ury - master_y;
      if (llx == loc_2_x && lly == loc_2_y) {
        logger_->warn(utl::TAP,
                      9,
                      "Row {} has enough space for only one endcap.",
                      subrow->getName());
        continue;
      }

      odb::dbOrientType right_ori = row_ori;
      if (row_ori == odb::dbOrientType::MX) {
        right_ori = odb::dbOrientType::R180;
      } else if (row_ori == odb::dbOrientType::R0) {
        right_ori = odb::dbOrientType::MY;
      }
      makeInstance(block, right_master, right_ori, loc_2_x, loc_2_y, endcap_prefix_);
    }
  }

  int endcap_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 4, "Inserted {} endcaps.", endcap_count);
  return endcap_count;
}

bool Tapcell::isXInRow(const int x, const vector<odb::dbRow*>& subrow)
{
  for (odb::dbRow* row : subrow) {
    odb::Rect row_bb;
    row->getBBox(row_bb);
    if (x >= row_bb.xMin() && x <= row_bb.xMax()) {
      return true;
    }
  }
  return false;
}

// Get location type of x if above macro, below macro or not a macro
LocationType Tapcell::getLocationType(const int x,
                       const vector<odb::dbRow*>& rows_above,
                       const vector<odb::dbRow*>& rows_below)
{
  const int in_above = isXInRow(x, rows_above);
  const int in_below = isXInRow(x, rows_below);

  if (in_above && !in_below) {
    return AboveMacro;
  }
  if (!in_above && in_below) {
    return BelowMacro;
  }
  return None;
}

odb::dbMaster* Tapcell::pickCornerMaster(LocationType top_bottom,
                                         odb::dbOrientType ori,
                                         odb::dbMaster* cnrcap_nwin_master,
                                         odb::dbMaster* cnrcap_nwout_master,
                                         odb::dbMaster* endcap_master)
{
  if (top_bottom == BelowMacro) {
    if (ori == odb::dbOrientType::MX) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else if (top_bottom == AboveMacro) {
    if (ori == odb::dbOrientType::R0) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else {
    return endcap_master;
  }
}

int Tapcell::insertTapcells(const vector<vector<odb::dbRow*>>& rows,
                            const string& tapcell_master,
                            int dist)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();

  odb::dbMaster* master = db_->findMaster(tapcell_master.c_str());
  if (master == nullptr) {
    logger_->error(utl::TAP, 11, "Master {} not found.", tapcell_master);
  }

  int y;
  std::map<int, vector<vector<int>>> row_fills;
  for (filled_sites& placement : filled_sites_) {
    y = placement.yMin;
    int x_start = placement.xMin;
    int x_end = placement.xMax;
    vector<int> x_start_end;
    x_start_end.push_back(x_start);
    x_start_end.push_back(x_end);
    row_fills[y].push_back(x_start_end);
  }

  for(const auto&[y,ignored]:row_fills) {
    vector<int> prev_x;
    vector<int> xs;
    vector<vector<int>> merged_placements;
    std::sort(row_fills[y].begin(), row_fills[y].end());
    for (vector<int> xs : row_fills[y]) {
      std::sort(xs.begin(), xs.end());
      if (prev_x.size() == 0) {
        prev_x = xs;
      } else {
        if (xs[0] == prev_x[1]) {
          prev_x[1] = xs[1];
        } else {
          merged_placements.push_back(prev_x);
          prev_x = xs;
        }
      }
    }
    if (prev_x.size() != 0) {
      merged_placements.push_back(prev_x);
    }
    if (prev_x.size() == 0) {
      merged_placements.push_back(xs);
    }
    row_fills.insert({y, merged_placements});
  }

  std::map<int, int> rows_with_macros;
  std::map<std::pair<int, int>, vector<int>> macro_outlines
      = getMacroOutlines(rows);
  for(auto&[ignored,bot_top_row]:macro_outlines) {
    for (int i = 0; i < bot_top_row.size(); i += 2) {
      // Traverse all rows cut by a macro
      bot_top_row[i]--;
      bot_top_row[i + 1]++;

      if (bot_top_row[i] >= 0) {
        rows_with_macros.insert({bot_top_row[i], 0});
      }
      if (bot_top_row[i + 1] <= rows.size()) {
        rows_with_macros.insert({bot_top_row[i + 1], 0});
      }
    }
  }

  for (int row_idx = 0; row_idx < rows.size(); row_idx++) {
    vector<odb::dbRow*> subrows = rows[row_idx];
    odb::Rect rowbb;
    subrows[0]->getBBox(rowbb);
    const int row_y = rowbb.yMin();
    vector<vector<int>> row_fill_check;
    if (row_fills.find(row_y) != row_fills.end()) {
      row_fill_check = row_fills[row_y];
    } else {
      row_fill_check.clear();
    }

    int gaps_above_below = 0;
    if (rows_with_macros.begin()->first == row_idx) {
      gaps_above_below = 1;
      rows_with_macros.erase(rows_with_macros.begin()->first);
    }

    for (odb::dbRow* row : subrows) {
      if (!checkSymmetry(master, row->getOrient())) {
        continue;
      }

      int offset = 0;
      int pitch = -1;
      const int dist1 = dist * (db_->getTech()->getLefUnits());
      const int dist2 = 2 * dist * (db_->getTech()->getLefUnits());

      if ((row_idx % 2) == 0) {
        offset = dist1;
      } else {
        offset = dist2;
      }

      if ((row_idx == 0) || (row_idx == rows.size() - 1) || gaps_above_below) {
        pitch = dist1;
        offset = dist1;
      } else {
        pitch = dist2;
      }

      int x;
      odb::Rect row_bb;
      row->getBBox(row_bb);
      const int llx = row_bb.xMin();
      const int urx = row_bb.xMax();

      for (x = llx + offset; x < urx; x += pitch) {
        const int site_x = row->getSite()->getWidth();
        x = makeSiteLoc(x, site_x, 1, llx);
        // Check if site is filled
        const int tap_width = master->getWidth();
        odb::dbOrientType ori = row->getOrient();
        int overlap = checkIfFilled(x, tap_width, ori, row_fill_check);
        if (overlap == 0) {
          const int lly = row_bb.yMin();
          makeInstance(block, master, ori, x, lly, tap_prefix_);
        }
      }
    }
  }
  int tapcell_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 5, "Inserted {} tapcells.", tapcell_count);
  return tapcell_count;
}

int Tapcell::checkIfFilled(int x,
                            int width,
                            odb::dbOrientType& orient,
                            const vector<vector<int>>& row_insts)
{
  int x_start;
  int x_end;
  if (orient == odb::dbOrientType::MY || orient == odb::dbOrientType::R180) {
    x_start = x - width;
    x_end = x;
  } else {
    x_start = x;
    x_end = x + width;
  }

  for (auto placement : row_insts) {
    if (x_end > placement[0] && x_start < placement[1]) {
      int left_x = placement[0] - width;
      int right_x = placement[1];
      vector<std::pair<int, int>> value;
      return left_x + right_x;
    }
  }
  return 0;
}

int Tapcell::insertAtTopBottom(const vector<vector<odb::dbRow*>>& rows,
                               const vector<string>& masters,
                               odb::dbMaster* endcap_master,
                               const string& prefix)
{
  int start_phy_idx = phy_idx_;

  odb::dbBlock* block = db_->getChip()->getBlock();
  string tap_nwintie_master_name;
  string tap_nwin2_master_name;
  string tap_nwin3_master_name;
  string tap_nwouttie_master_name;
  string tap_nwout2_master_name;
  string tap_nwout3_master_name;

  if (masters.size() == 6) {
    tap_nwintie_master_name = masters[0];
    tap_nwin2_master_name = masters[1];
    tap_nwin3_master_name = masters[2];
    tap_nwouttie_master_name = masters[3];
    tap_nwout2_master_name = masters[4];
    tap_nwout3_master_name = masters[5];
  }

  odb::dbMaster* tap_nwintie_master
      = db_->findMaster(tap_nwintie_master_name.c_str());
  if (tap_nwintie_master == nullptr) {
    logger_->error(
        utl::TAP, 18, "Master {} not found.", tap_nwintie_master_name);
  }
  odb::dbMaster* tap_nwin2_master
      = db_->findMaster(tap_nwin2_master_name.c_str());
  if (tap_nwin2_master == nullptr) {
    logger_->error(utl::TAP, 19, "Master {} not found.", tap_nwin2_master_name);
  }
  odb::dbMaster* tap_nwin3_master
      = db_->findMaster(tap_nwin3_master_name.c_str());
  if (tap_nwin3_master == nullptr) {
    logger_->error(utl::TAP, 20, "Master {} not found.", tap_nwin3_master_name);
  }
  odb::dbMaster* tap_nwouttie_master
      = db_->findMaster(tap_nwouttie_master_name.c_str());
  if (tap_nwouttie_master == nullptr) {
    logger_->error(utl::TAP, 21, "Master $tap_nwouttie_master_name not found.");
  }
  odb::dbMaster* tap_nwout2_master
      = db_->findMaster(tap_nwout2_master_name.c_str());
  if (tap_nwout2_master == nullptr) {
    logger_->error(
        utl::TAP, 22, "Master {} not found.", tap_nwout2_master_name);
  }
  odb::dbMaster* tap_nwout3_master
      = db_->findMaster(tap_nwout3_master_name.c_str());
  if (tap_nwout3_master == nullptr) {
    logger_->error(
        utl::TAP, 23, "Master {} not found.", tap_nwout3_master_name);
  }

  vector<vector<odb::dbRow*>> new_rows;
  // Grab bottom
  new_rows.push_back(rows[0]);
  // Grab top
  new_rows.push_back(rows.back());

  for (int cur_row = 0; cur_row < new_rows.size(); cur_row++) {
    for (odb::dbRow* subrow : new_rows[cur_row]) {
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      const int endcapwidth = endcap_master->getWidth();
      const int llx = row_bb.xMin();
      const int x_start = llx + endcapwidth;
      const int urx = row_bb.xMax();
      const int x_end = urx - endcapwidth;
      const int lly = row_bb.yMin();
      odb::dbOrientType ori = subrow->getOrient();
      insertAtTopBottomHelper(block,
                              cur_row,
                              0,
                              ori,
                              x_start,
                              x_end,
                              lly,
                              tap_nwintie_master,
                              tap_nwin2_master,
                              tap_nwin3_master,
                              tap_nwouttie_master,
                              tap_nwout2_master,
                              tap_nwout3_master,
                              prefix);
    }
  }
  int topbottom_cnt = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 6, "Inserted {} top/bottom cells.", topbottom_cnt);
  return topbottom_cnt;
}

void Tapcell::insertAtTopBottomHelper(odb::dbBlock* block,
                                      int top_bottom,
                                      bool is_macro,
                                      odb::dbOrientType ori,
                                      int x_start,
                                      int x_end,
                                      int lly,
                                      odb::dbMaster* tap_nwintie_master,
                                      odb::dbMaster* tap_nwin2_master,
                                      odb::dbMaster* tap_nwin3_master,
                                      odb::dbMaster* tap_nwouttie_master,
                                      odb::dbMaster* tap_nwout2_master,
                                      odb::dbMaster* tap_nwout3_master,
                                      const string& prefix)
{
  odb::dbMaster* master;
  odb::dbMaster* tb2_master;
  odb::dbMaster* tb3_master;
  if (top_bottom == 1) {
    // Top
    if ((ori == odb::dbOrientType::R0 && is_macro)
        || (ori == odb::dbOrientType::MX && !is_macro)) {
      master = tap_nwintie_master;
      tb2_master = tap_nwin2_master;
      tb3_master = tap_nwin3_master;
    } else {
      master = tap_nwouttie_master;
      tb2_master = tap_nwout2_master;
      tb3_master = tap_nwout3_master;
    }
  } else {
    // Bottom
    if ((ori == odb::dbOrientType::MX && is_macro)
        || (ori == odb::dbOrientType::R0 && !is_macro)) {
      master = tap_nwintie_master;
      tb2_master = tap_nwin2_master;
      tb3_master = tap_nwin3_master;
    } else {
      master = tap_nwouttie_master;
      tb2_master = tap_nwout2_master;
      tb3_master = tap_nwout3_master;
    }
  }

  const int tbtiewidth = master->getWidth();
  const int tap2_master_width = tb2_master->getWidth();
  int tbtiecount = std::floor((x_end - x_start) / tbtiewidth);
  // Ensure there is room at the end of the row for atleast one tb2, if needed
  int remaining_distance = x_end - x_start - (tbtiecount * tbtiewidth);
  if ((remaining_distance != 0) && (remaining_distance < tap2_master_width)) {
    tbtiecount--;
  }
  // Insert tb tie
  int x = x_start;
  for (int n = 0; n < tbtiecount; n++) {
    if (checkSymmetry(master, ori)) {
      makeInstance(block, master, ori, x, lly, prefix);
    }
    x += tbtiewidth;
  }

  // Fill remaining sites
  const int tap3_master_width = tb3_master->getWidth();
  int tb3tiecount = std::floor((x_end - x) / tap3_master_width);
  remaining_distance = x_end - x - (tb3tiecount * tap3_master_width);
  while ((remaining_distance % tap2_master_width) != 0
         && remaining_distance >= 0) {
    tb3tiecount--;
    remaining_distance = x_end - x - (tb3tiecount * tap3_master_width);
  }

  // Fill with 3s
  for (int n = 0; n < tb3tiecount; n++) {
    if (checkSymmetry(tb3_master, ori)) {
      makeInstance(block, tb3_master, ori, x, lly, prefix);
    }
    x += tap3_master_width;
  }

  // Fill with 2s
  for (; x < x_end; x += tap2_master_width) {
    if (checkSymmetry(tb2_master, ori)) {
      makeInstance(block, tb2_master, ori, x, lly, prefix);
    }
  }
}

int Tapcell::insertAroundMacros(const vector<vector<odb::dbRow*>>& rows,
                                const vector<string>& masters,
                                odb::dbMaster* corner_master,
                                const string& prefix)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();
  string incnrcap_nwin_master_name;
  string tap_nwin2_master_name;
  string tap_nwin3_master_name;
  string tap_nwintie_master_name;
  string incnrcap_nwout_master_name;
  string tap_nwout2_master_name;
  string tap_nwout3_master_name;
  string tap_nwouttie_master_name;

  if (masters.size() == 8) {
    incnrcap_nwin_master_name.assign(masters[0]);
    tap_nwin2_master_name.assign(masters[1]);
    tap_nwin3_master_name.assign(masters[2]);
    tap_nwintie_master_name.assign(masters[3]);
    incnrcap_nwout_master_name.assign(masters[4]);
    tap_nwout2_master_name.assign(masters[5]);
    tap_nwout3_master_name.assign(masters[6]);
    tap_nwouttie_master_name.assign(masters[7]);
  }

  odb::dbMaster* tap_nwintie_master
      = db_->findMaster(tap_nwintie_master_name.c_str());
  if (tap_nwintie_master == nullptr) {
    logger_->error(
        utl::TAP, 24, "Master {} not found.", tap_nwintie_master_name);
  }
  odb::dbMaster* tap_nwin2_master
      = db_->findMaster(tap_nwin2_master_name.c_str());
  if (tap_nwin2_master == nullptr) {
    logger_->error(utl::TAP, 25, "Master {} not found.", tap_nwin2_master_name);
  }
  odb::dbMaster* tap_nwin3_master
      = db_->findMaster(tap_nwin3_master_name.c_str());
  if (tap_nwin3_master == nullptr) {
    logger_->error(utl::TAP, 26, "Master {} not found.", tap_nwin3_master_name);
  }
  odb::dbMaster* tap_nwouttie_master
      = db_->findMaster(tap_nwouttie_master_name.c_str());
  if (tap_nwouttie_master == nullptr) {
    logger_->error(
        utl::TAP, 27, "Master {} not found.", tap_nwouttie_master_name);
  }
  odb::dbMaster* tap_nwout2_master
      = db_->findMaster(tap_nwout2_master_name.c_str());
  if (tap_nwout2_master == nullptr) {
    logger_->error(
        utl::TAP, 28, "Master {} not found.", tap_nwout2_master_name);
  }
  odb::dbMaster* tap_nwout3_master
      = db_->findMaster(tap_nwout3_master_name.c_str());
  if (tap_nwout3_master == nullptr) {
    logger_->error(
        utl::TAP, 29, "Master {} not found.", tap_nwout3_master_name);
  }
  odb::dbMaster* incnrcap_nwin_master
      = db_->findMaster(incnrcap_nwin_master_name.c_str());
  if (incnrcap_nwin_master == nullptr) {
    logger_->error(
        utl::TAP, 30, "Master {} not found.", incnrcap_nwin_master_name);
  }
  odb::dbMaster* incnrcap_nwout_master
      = db_->findMaster(incnrcap_nwout_master_name.c_str());
  if (incnrcap_nwout_master == nullptr) {
    logger_->error(
        utl::TAP, 31, "Master {} not found.", incnrcap_nwout_master_name);
  }

  std::map<std::pair<int, int>, vector<int>> macro_outlines
      = getMacroOutlines(rows);

  for(auto&[x_start_end, outline]:macro_outlines) {
    for (int i = 0; i < outline.size(); i += 2) {
      const int x_start = x_start_end.first;
      const int x_end = x_start_end.second;
      int bot_row = outline[i];
      int top_row = outline[i + 1];
      bot_row--;
      top_row++;
      int row_start;
      int row_end;
      const int corner_cell_width = corner_master->getWidth();
      const int total_rows = rows.size();
      odb::dbMaster* incnr_master;
      odb::dbOrientType west_ori;
      if (top_row < total_rows - 1) {
        odb::dbRow* top_row_inst = rows[top_row].front();
        odb::dbOrientType top_row_ori = top_row_inst->getOrient();
        odb::Rect row_bb;
        top_row_inst->getBBox(row_bb);
        const int top_row_y = row_bb.yMin();

        row_start = x_start;
        row_end = x_end;
        if (row_start == -1) {
          row_start = row_bb.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          odb::Rect rowbb_;
          rows[top_row].back()->getBBox(rowbb_);
          row_end = rowbb_.xMax();
          row_end = row_end - corner_cell_width;
        }
        // Do top row
        insertAtTopBottomHelper(block,
                                1,
                                1,
                                top_row_ori,
                                row_start,
                                row_end,
                                top_row_y,
                                tap_nwintie_master,
                                tap_nwin2_master,
                                tap_nwin3_master,
                                tap_nwouttie_master,
                                tap_nwout2_master,
                                tap_nwout3_master,
                                prefix);
        // Do corners
        if (top_row_ori == odb::dbOrientType::R0) {
          incnr_master = incnrcap_nwin_master;
          west_ori = odb::dbOrientType::MY;

        } else {
          incnr_master = incnrcap_nwout_master;
          west_ori = odb::dbOrientType::R180;
        }

        // NE corner
        if (checkSymmetry(incnr_master, top_row_ori)) {
          makeInstance(block, incnr_master, top_row_ori, x_end, top_row_y, prefix);
        }
        // NW corner
        if (checkSymmetry(incnr_master, west_ori)) {
          makeInstance(block,
                    incnr_master,
                    west_ori,
                    (x_start - incnr_master->getWidth()),
                    top_row_y,
                    prefix);
        }
      }

      if (bot_row >= 1) {
        odb::dbRow* bot_row_inst = rows[bot_row].front();
        odb::dbOrientType bot_row_ori = bot_row_inst->getOrient();
        odb::Rect rowbb1;
        bot_row_inst->getBBox(rowbb1);
        const int bot_row_y = rowbb1.yMin();

        row_start = x_start;
        row_end = x_end;
        if (row_start == -1) {
          row_start = rowbb1.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          odb::Rect rowbb3;
          rows[bot_row].back()->getBBox(rowbb3);
          row_end = rowbb3.xMax();
          row_end = row_end - corner_cell_width;
        }

        // Do bottom row
        insertAtTopBottomHelper(block,
                                0,
                                1,
                                bot_row_ori,
                                row_start,
                                row_end,
                                bot_row_y,
                                tap_nwintie_master,
                                tap_nwin2_master,
                                tap_nwin3_master,
                                tap_nwouttie_master,
                                tap_nwout2_master,
                                tap_nwout3_master,
                                prefix);

        // Do corners
        if (bot_row_ori == odb::dbOrientType::MX) {
          incnr_master = incnrcap_nwin_master;
          west_ori = odb::dbOrientType::R180;
        } else {
          incnr_master = incnrcap_nwout_master;
          west_ori = odb::dbOrientType::MY;
        }

        // SE corner
        if (checkSymmetry(incnr_master, bot_row_ori)) {
          makeInstance(block, incnr_master, bot_row_ori, x_end, bot_row_y, prefix);
        }
        // SW corner
        if (checkSymmetry(incnr_master, west_ori)) {
          makeInstance(block,
                    incnr_master,
                    west_ori,
                    (x_start - incnr_master->getWidth()),
                    bot_row_y,
                    prefix);
        }
      }
    }
  }
  int blkgs_cnt = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 7, "Inserted {} cells near blockages.", blkgs_cnt);
  return blkgs_cnt;
}

const std::pair<int, int> Tapcell::getMinMaxX(const vector<vector<odb::dbRow*>>& rows)
{
  int min_x = std::numeric_limits<int>::min();
  int max_x = std::numeric_limits<int>::max();
  for (vector<odb::dbRow*> subrow : rows) {
    int new_min_x;
    int new_max_x;
    for (odb::dbRow* row : subrow) {
      odb::Rect row_bb;
      row->getBBox(row_bb);
      new_min_x = row_bb.xMin();
      new_max_x = row_bb.xMax();
      if (min_x == std::numeric_limits<int>::min()) {
        min_x = new_min_x;
      } else {
        min_x = std::min(min_x, new_min_x);
      }

      if (max_x == std::numeric_limits<int>::max()) {
        max_x = new_max_x;
      } else {
        max_x = std::max(max_x, new_max_x);
      }
    }
  }
  return {min_x, max_x};
}

// Return map of x-positions where rows were cut because of a macro 
// to their corresponding vector of rows 
std::map<std::pair<int, int>, vector<int>> Tapcell::getMacroOutlines(
    const vector<vector<odb::dbRow*>>& rows)
{
  std::map<std::pair<int, int>, vector<int>> macro_outlines;
  std::pair<int, int> min_max_x = getMinMaxX(rows);

  for (int cur_row = 0; cur_row < rows.size(); cur_row++) {
    vector<odb::dbRow*> subrows = rows[cur_row];
    const int subrow_count = subrows.size();

    // Positions where rows were cut because of a macro
    int xMax_pos = -1;
    int xMin_pos = -1;

    for (int cur_subrow = 0; cur_subrow < subrow_count; cur_subrow++) {
      odb::Rect currow_bb;
      subrows[cur_subrow]->getBBox(currow_bb);

      xMin_pos = currow_bb.xMin();
      if (cur_subrow == 0 && xMin_pos == min_max_x.first) {
        // Still on the first position
        xMin_pos = -1;
      }

      if (xMax_pos != xMin_pos) {
        macro_outlines[{xMax_pos,xMin_pos}].push_back(cur_row);
      }

      xMax_pos = currow_bb.xMax();
    }
    if (xMax_pos != min_max_x.second) {
      // Row is cut at the end
      macro_outlines[{xMax_pos,-1}].push_back(cur_row);
    }
  }

  std::map<std::pair<int, int>, vector<int>> macro_outlines_array;
  for(const auto&[k,all_rows]:macro_outlines) {
    vector<int> new_rows;
    new_rows.push_back(all_rows.front());
    for (int i = 1; i < all_rows.size() - 1; i++) {
      if (all_rows[i] + 1 == all_rows[i + 1]) {
        continue;
      }
      new_rows.push_back(all_rows[i]);
      new_rows.push_back(all_rows[i + 1]);
    }
    new_rows.push_back(all_rows.back());
    macro_outlines_array[k].insert(
           macro_outlines_array[k].end(), new_rows.begin(), new_rows.end());
  }
  return macro_outlines_array;
}

// function to detect if blockage overlaps with row
bool Tapcell::overlaps(odb::dbBox* blockage,
                       odb::dbRow* row,
                       int halo_x,
                       int halo_y)
{
  odb::Rect rowBB;
  row->getBBox(rowBB);

  // Check if Y has overlap first since rows are long and skinny
  const int blockage_lly = blockage->yMin() - halo_y;
  const int blockage_ury = blockage->yMax() + halo_y;
  const int row_lly = rowBB.yMin();
  const int row_ury = rowBB.yMax();

  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return false;
  }

  const int blockage_llx = blockage->xMin() - halo_x;
  const int blockage_urx = blockage->xMax() + halo_x;
  const int row_llx = rowBB.xMin();
  const int row_urx = rowBB.xMax();

  if (blockage_llx >= row_urx || row_llx >= blockage_urx) {
    return false;
  }

  return true;
}

vector<odb::dbBox*> Tapcell::findBlockages()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  vector<odb::dbBox*> blockages;
  for (auto&& inst : db_->getChip()->getBlock()->getInsts()) {
    if (inst->isBlock()) {
      if (!inst->isPlaced()) {
        logger_->warn(utl::TAP, 32, "Macro {} is not placed.", inst->getName());
        continue;
      }
      blockages.push_back(inst->getBBox());
    }
  }

  return blockages;

}

int Tapcell::makeSiteLoc(int x, double site_x, bool at_left_from_macro, int offset)
{
  auto site_int = double(x - offset) / site_x;
  if (at_left_from_macro == 0) {
    site_int = ceil(site_int);
  } else {
    site_int = floor(site_int);
  }
  site_int = int(site_int * site_x + offset);
  return site_int;
}

void Tapcell::makeInstance(odb::dbBlock* block,
                        odb::dbMaster* master,
                        odb::dbOrientType orientation,
                        int x,
                        int y,
                        const string& prefix)
{
  if (x < 0 || y < 0) {
    return;
  }
  string name = prefix + std::to_string(phy_idx_);
  odb::dbInst* inst = odb::dbInst::create(block, master, name.c_str());
  if (inst == nullptr) {
    logger_->error(utl::TAP, 33, "Not able to build instance {} with master {}.", name, master->getName());
  }
  inst->setOrient(orientation);
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  inst->setSourceType(odb::dbSourceType::DIST);

  odb::dbBox* inst_bb = inst->getBBox();
  filled_sites filled;
  filled.yMin = inst_bb->yMin();
  filled.xMin = inst_bb->xMin();
  filled.xMax = inst_bb->xMax();
  filled_sites_.push_back(filled);

  phy_idx_++;
}

void Tapcell::buildRow(odb::dbBlock* block,
                       const string& name,
                       odb::dbSite* site,
                       int start_x,
                       int end_x,
                       int y,
                       odb::dbOrientType& orient,
                       odb::dbRowDir& direction,
                       int min_row_width)
{
  const int site_width = site->getWidth();
  const int new_row_num_sites = (end_x - start_x) / site_width;
  const int new_row_width = new_row_num_sites * site_width;

  if (new_row_num_sites > 0 && new_row_width >= min_row_width) {
    odb::dbRow::create(block,
                       name.c_str(),
                       site,
                       start_x,
                       y,
                       orient,
                       direction,
                       new_row_num_sites,
                       site_width);
  }
}

// Return rows vector organized according to yMin value,
// each subvector organized according to xMin value 
vector<vector<odb::dbRow*>> Tapcell::organizeRows()
{
  std::map<int, vector<odb::dbRow*>> rows_dict;
  // Gather rows according to yMin values
  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    odb::Rect rowBB;
    row->getBBox(rowBB);
    rows_dict[rowBB.yMin()].push_back(row);
  }


  // Organize rows bottom to top
  vector<vector<odb::dbRow*>> rows;
  for(const auto&[k,ignored]:rows_dict) {
    std::map<int, odb::dbRow*> in_row_dict;
    for (odb::dbRow* in_row : rows_dict[k]) {
      odb::Rect row_BB;
      in_row->getBBox(row_BB);
      in_row_dict.insert({row_BB.xMin(), in_row});
    }
    // Organize sub rows left to right
    vector<odb::dbRow*> in_row;
    for(const auto&[in_k,ignored]:in_row_dict) {
      in_row.push_back(in_row_dict[in_k]);
    }
    rows.push_back(in_row);
    in_row.clear();
  }
  return rows;
}

int Tapcell::removeCells(const std::string& prefix)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  string prefix_string(prefix);
  int removed = 0;

  if (prefix_string.length() == 0) {
    // If no prefix is given, return 0 instead of having all cells removed
    return 0;
  }

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getName().find(prefix_string) != string::npos) {
      odb::dbInst::destroy(inst);
      removed++;
    }
  }

  return removed;
}

bool Tapcell::checkSymmetry(odb::dbMaster* master, odb::dbOrientType ori)
{
  bool symmetry_x = master->getSymmetryX();
  bool symmetry_y = master->getSymmetryY();

  switch (ori) {
    case odb::dbOrientType::R0:
      return true;
    case odb::dbOrientType::MX:
      return symmetry_x;
    case odb::dbOrientType::MY:
      return symmetry_y;
    case odb::dbOrientType::R180:
      return (symmetry_x && symmetry_y);
    default:
      return false;
  }
}

// namespace end
}  // namespace tap
