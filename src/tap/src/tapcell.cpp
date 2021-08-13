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

#include "opendb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace tap {

using std::max;
using std::min;
using std::string;
using std::vector;

Tapcell::Tapcell() : db_(nullptr)
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
}

//---------------------------------------------------------------

void Tapcell::setTapPrefix(const char* tap_prefix) {
  tap_prefix_ = tap_prefix;
}

void Tapcell::setEndcapPrefix(const char* endcap_prefix) {
  endcap_prefix_ = endcap_prefix;
}

void Tapcell::clear()
{
  int taps_removed = removeCells(tap_prefix_);
  int endcaps_removed = removeCells(endcap_prefix_);

  // Reset global parameters
  phy_idx_ = 0;
  filled_sites_.clear();
}

void Tapcell::run(odb::dbMaster* endcap_master,
                  int& halo_x,
                  int& halo_y,
                  const char* cnrcap_nwin_master,
                  const char* cnrcap_nwout_master,
                  int& add_boundary_cell,
                  const char* tap_nwintie_master,
                  const char* tap_nwin2_master,
                  const char* tap_nwin3_master,
                  const char* tap_nwouttie_master,
                  const char* tap_nwout2_master,
                  const char* tap_nwout3_master,
                  const char* incnrcap_nwin_master,
                  const char* incnrcap_nwout_master,
                  const char* tapcell_master,
                  int& dist)
{
  cutRows(endcap_master, findBlockages(), halo_x, halo_y);
  vector<vector<odb::dbRow*>> rows = organizeRows();
  vector<string> cnrcap_masters;
  string cnrcap_nwin_master_string(cnrcap_nwin_master);
  string cnrcap_nwout_master_string(cnrcap_nwout_master);
  cnrcap_masters.push_back(cnrcap_nwin_master_string);
  cnrcap_masters.push_back(cnrcap_nwout_master_string);

  phy_idx_ = 0;
  filled_sites_.clear();
  if (endcap_master != nullptr) {
    insertEndcaps(rows, endcap_master, cnrcap_masters);
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
                      db_->findMaster(cnrcap_nwin_master),
                      endcap_prefix_);
    insertAroundMacros(rows,
                       tap_macro_masters,
                       db_->findMaster(cnrcap_nwin_master),
                       endcap_prefix_);
  }
  insertTapcells(rows, tapcell_master, dist);
  filled_sites_.clear();
}

void Tapcell::cutRows(odb::dbMaster* endcap_master,
                      vector<odb::dbBox*> blockages,
                      int& halo_x,
                      int& halo_y)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  int rows_count = (block->getRows()).size();
  int block_count = blockages.size();
  int min_row_width;
  int end_width;

  if (endcap_master != nullptr) {
    end_width = endcap_master->getWidth();
    min_row_width = 2 * end_width;
  } else {
    min_row_width = 0;
  }

  // Gather rows needing to be cut up front
  vector<odb::dbRow*> blocked_rows;
  std::map<string, vector<odb::dbBox*>> row_blockages;
  string row_name;
  for (odb::dbBox* blockage : blockages) {
    for (odb::dbRow* row : block->getRows()) {
      if (overlaps(blockage, row, halo_x, halo_y)) {
        row_name = row->getName();
        if (row_blockages.find(row_name) == row_blockages.end()) {
          blocked_rows.push_back(row);
          vector<odb::dbBox*> row_blockages_bbox;
          row_blockages_bbox.push_back(blockage);
          row_blockages.insert(std::pair<string, vector<odb::dbBox*>>(
              row_name, row_blockages_bbox));
        } else {
          row_blockages[row_name].push_back(blockage);
        }
      }
    }
  }

  // cut rows around macros
  for (odb::dbRow* row : blocked_rows) {
    cutRow(block, row, row_blockages, min_row_width, halo_x, halo_y);
  }
  int cut_rows_count = ((block->getRows()).size()) - rows_count;
  logger_->info(utl::TAP, 1, "Found {} macro blocks.", block_count);
  logger_->info(utl::TAP, 2, "Original rows: {}", rows_count);
  logger_->info(utl::TAP, 3, "Created {} rows for a total of {} rows.", cut_rows_count, cut_rows_count+rows_count);
}

void Tapcell::cutRow(odb::dbBlock* block,
                     odb::dbRow* row,
                     std::map<string, vector<odb::dbBox*>>& row_blockages,
                     int& min_row_width,
                     int& halo_x,
                     int& halo_y)
{
  string row_name = row->getName();
  odb::Rect row_bb;
  row->getBBox(row_bb);

  odb::dbSite* row_site = row->getSite();
  const int site_width = row_site->getWidth();
  odb::dbOrientType orient = row->getOrient();
  odb::dbRowDir direction = row->getDirection();

  int start_origin_x = row_bb.xMin();
  int start_origin_y = row_bb.yMin();

  int curr_min_row_width = min_row_width + 2 * site_width;

  vector<odb::dbBox*> row_blockage_bboxs = row_blockages[row_name];
  vector<std::pair<int, int>> row_blockage_xs;
  for (odb::dbBox* row_blockage_bbox : row_blockages[row_name]) {
    row_blockage_xs.push_back(
        std::make_pair(row_blockage_bbox->xMin(), row_blockage_bbox->xMax()));
  }

  std::sort(row_blockage_xs.begin(), row_blockage_xs.end());

  int row_sub_idx = 1;
  for (std::pair<int, int> blockage : row_blockage_xs) {
    int blockage_x0 = blockage.first;
    int blockage_x1 = blockage.second;
    int new_row_end_x
        = makeSiteLoc(blockage_x0 - halo_x, site_width, -1, start_origin_x);
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
    start_origin_x
        = makeSiteLoc(blockage_x1 + halo_x, site_width, 1, start_origin_x);
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

int Tapcell::insertEndcaps(vector<vector<odb::dbRow*>>& rows,
                           odb::dbMaster* endcap_master,
                           vector<string>& cnrcap_masters)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbMaster* cnrcap_nwin_master;
  odb::dbMaster* cnrcap_nwout_master;

  int endcapwidth = endcap_master->getWidth();
  int do_corners;
  if (cnrcap_masters.size() == 2) {
    string cnrcap_nwin_master_name = cnrcap_masters[0];
    string cnrcap_nwout_master_name = cnrcap_masters[1];
    string invalid = "INVALID";
    if (cnrcap_nwin_master_name.compare("INVALID") == 0
        || cnrcap_nwout_master_name.compare("INVALID") == 0) {
      do_corners = 0;
    } else {
      do_corners = 1;

      cnrcap_nwin_master = db_->findMaster(cnrcap_nwin_master_name.c_str());
      if (cnrcap_nwin_master == nullptr) {
        logger_->error(
            utl::TAP, 12, "Master {} not found.", cnrcap_nwin_master_name);
      }
      cnrcap_nwout_master = db_->findMaster(cnrcap_nwout_master_name.c_str());

      if (cnrcap_nwout_master == nullptr) {
        logger_->error(
            utl::TAP, 13, "Master {} not found.", cnrcap_nwout_master_name);
      }
      std::pair<int, int> min_max_x;
      min_max_x = getMinMaxX(rows);
    }
  } else {
    do_corners = 0;
  }

  int bottom_row = 0;
  int top_row = rows.size() - 1;

  for (int cur_row = bottom_row; cur_row <= top_row; cur_row++) {
    for (odb::dbRow* subrow : rows[cur_row]) {
      if (!(checkSymmetry(endcap_master, subrow->getOrient()))) {
        continue;
      }
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      auto row_ori = subrow->getOrient();

      const int llx = row_bb.xMin();
      const int lly = row_bb.yMin();
      const int urx = row_bb.xMax();
      const int ury = row_bb.yMax();

      odb::dbMaster* masterl;
      odb::dbMaster* masterr;

      if (do_corners) {
        if (cur_row == top_row) {
          masterl = pickCornerMaster(1,
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          masterr = masterl;
        } else if (cur_row == bottom_row) {
          masterl = pickCornerMaster(-1,
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          masterr = masterl;
        } else {
          auto rows_above = rows[cur_row + 1];
          auto rows_below = rows[cur_row - 1];
          masterl = pickCornerMaster(isXCorner(llx, rows_above, rows_below),
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
          masterr = pickCornerMaster(isXCorner(urx, rows_above, rows_below),
                                     row_ori,
                                     cnrcap_nwin_master,
                                     cnrcap_nwout_master,
                                     endcap_master);
        }
      } else {
        masterl = endcap_master;
        masterr = masterl;
      }

      if ((masterl->getWidth()) > (urx - llx)) {
        continue;
      }

      buildCell(block, masterl, row_ori, llx, lly, endcap_prefix_);

      int master_x = masterr->getWidth();
      int master_y = masterr->getHeight();

      int loc_2_x = urx - master_x;
      int loc_2_y = ury - master_y;
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
      buildCell(block, masterr, right_ori, loc_2_x, loc_2_y, endcap_prefix_);
    }
  }

  int endcap_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 4, "Inserted {} endcaps.", endcap_count);
  return endcap_count;
}

bool Tapcell::isXInRow(const int x, vector<odb::dbRow*>& subrow)
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

int Tapcell::isXCorner(const int x,
                       vector<odb::dbRow*>& rows_above,
                       vector<odb::dbRow*>& rows_below)
{
  int in_above = isXInRow(x, rows_above);
  int in_below = isXInRow(x, rows_below);

  if (in_above && !in_below) {
    return -1;
  }
  if (!in_above && in_below) {
    return 1;
  }
  return 0;
}

odb::dbMaster* Tapcell::pickCornerMaster(int top_bottom,
                                         odb::dbOrientType ori,
                                         odb::dbMaster* cnrcap_nwin_master,
                                         odb::dbMaster* cnrcap_nwout_master,
                                         odb::dbMaster* endcap_master)
{
  if (top_bottom == 1) {
    if (ori == odb::dbOrientType::MX) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else if (top_bottom == -1) {
    if (ori == odb::dbOrientType::R0) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else {
    return endcap_master;
  }
}

int Tapcell::insertTapcells(vector<vector<odb::dbRow*>>& rows,
                            string tapcell_master,
                            int& dist)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();

  odb::dbMaster* master = db_->findMaster(tapcell_master.c_str());
  if (master == nullptr) {
    logger_->error(utl::TAP, 11, "Master {} not found.", tapcell_master);
  }
  int tap_width = master->getWidth();
  int dist1 = dist * (db_->getTech()->getLefUnits());
  int dist2 = 2 * dist * (db_->getTech()->getLefUnits());

  int y;
  std::map<int, vector<vector<int>>> row_fills;
  for (vector<int>& placement : filled_sites_) {
    y = placement[0];
    int x_start = placement[1];
    int x_end = placement[2];
    vector<int> x_start_end;
    x_start_end.push_back(x_start);
    x_start_end.push_back(x_end);
    if (row_fills.find(y) == row_fills.end()) {
      vector<vector<int>> vector2;
      vector2.push_back(x_start_end);
      row_fills.insert(std::pair<int, vector<vector<int>>>(y, vector2));
    } else {
      row_fills[y].push_back(x_start_end);
    }
  }

  for (std::map<int, vector<vector<int>>>::iterator iter = row_fills.begin();
       iter != row_fills.end();
       ++iter) {
    y = iter->first;
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
    row_fills.insert(std::pair<int, vector<vector<int>>>(y, merged_placements));
  }

  std::map<int, int> rows_with_macros;
  std::map<std::pair<int, int>, vector<int>> macro_outlines
      = getMacroOutlines(rows);
  for (std::map<std::pair<int, int>, vector<int>>::iterator iter
       = macro_outlines.begin();
       iter != macro_outlines.end();
       ++iter) {
    vector<int> bot_top_row = iter->second;
    for (int i = 0; i < bot_top_row.size(); i += 2) {
      bot_top_row[i]--;
      bot_top_row[i + 1]++;

      if (bot_top_row[i] >= 0) {
        rows_with_macros.insert(std::pair<int, int>(bot_top_row[i], 0));
      }
      if (bot_top_row[i + 1] <= rows.size()) {
        rows_with_macros.insert(std::pair<int, int>(bot_top_row[i + 1], 0));
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
      const int site_x = row->getSite()->getWidth();
      odb::Rect row_bb;
      row->getBBox(row_bb);
      int llx = row_bb.xMin();
      const int lly = row_bb.yMin();
      const int urx = row_bb.xMax();
      const int ury = row_bb.yMax();
      odb::dbOrientType ori = row->getOrient();
      int offset = 0;
      int pitch = -1;

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
      for (x = llx + offset; x < urx; x = x + pitch) {
        x = makeSiteLoc(x, site_x, -1, llx);
        // check if site is filled
        int overlap = checkIfFilled(x, tap_width, ori, row_fill_check);
        if (overlap == 0) {
          buildCell(block, master, ori, x, lly, tap_prefix_);
        }
      }
    }
  }
  int tapcell_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 5, "Inserted {} tapcells.", tapcell_count);
  return tapcell_count;
}

int Tapcell::checkIfFilled(int& x,
                            int& width,
                            odb::dbOrientType& orient,
                            vector<vector<int>>& row_insts)
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

int Tapcell::insertAtTopBottom(vector<vector<odb::dbRow*>>& rows,
                               vector<string> masters,
                               odb::dbMaster* endcap_master,
                               string prefix)
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

  const int endcapwidth = endcap_master->getWidth();

  vector<vector<odb::dbRow*>> new_rows;
  // grab bottom
  new_rows.push_back(rows[0]);
  // grab top
  new_rows.push_back(rows.back());

  for (int cur_row = 0; cur_row < new_rows.size(); cur_row++) {
    for (odb::dbRow* subrow : new_rows[cur_row]) {
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      int llx = row_bb.xMin();
      int lly = row_bb.yMin();
      int urx = row_bb.xMax();
      int ury = row_bb.yMax();

      odb::dbOrientType ori = subrow->getOrient();

      int x_start = llx + endcapwidth;
      int x_end = urx - endcapwidth;

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
                                      int& x_start,
                                      int& x_end,
                                      int& lly,
                                      odb::dbMaster* tap_nwintie_master,
                                      odb::dbMaster* tap_nwin2_master,
                                      odb::dbMaster* tap_nwin3_master,
                                      odb::dbMaster* tap_nwouttie_master,
                                      odb::dbMaster* tap_nwout2_master,
                                      odb::dbMaster* tap_nwout3_master,
                                      string prefix)
{
  odb::dbMaster* master;
  odb::dbMaster* tb2_master;
  odb::dbMaster* tb3_master;
  if (top_bottom == 1) {
    // top
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
    // bottom
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

  int tbtiewidth = master->getWidth();
  int tap3_master_width = tb3_master->getWidth();
  int tap2_master_width = tb2_master->getWidth();

  int tbtiecount = std::floor((x_end - x_start) / tbtiewidth);
  // ensure there is room at the end of the row for atleast one tb2, if needed
  int remaining_distance = x_end - x_start - (tbtiecount * tbtiewidth);
  if ((remaining_distance != 0) && (remaining_distance < tap2_master_width)) {
    tbtiecount--;
  }
  // insert tb tie
  int x = x_start;
  for (int n = 0; n < tbtiecount; n++) {
    if (checkSymmetry(master, ori)) {
      buildCell(block, master, ori, x, lly, prefix);
    }
    x = x + tbtiewidth;
  }

  // fill remaining sites
  int tb3tiecount = std::floor((x_end - x) / tap3_master_width);
  remaining_distance = x_end - x - (tb3tiecount * tap3_master_width);
  while ((remaining_distance % tap2_master_width) != 0
         && remaining_distance >= 0) {
    tb3tiecount--;
    remaining_distance = x_end - x - (tb3tiecount * tap3_master_width);
  }

  // fill with 3s
  for (int n = 0; n < tb3tiecount; n++) {
    if (checkSymmetry(tb3_master, ori)) {
      buildCell(block, tb3_master, ori, x, lly, prefix);
    }
    x = x + tap3_master_width;
  }

  // fill with 2s
  for (; x < x_end; x = x + tap2_master_width) {
    if (checkSymmetry(tb2_master, ori)) {
      buildCell(block, tb2_master, ori, x, lly, prefix);
    }
  }
}

int Tapcell::insertAroundMacros(vector<vector<odb::dbRow*>>& rows,
                                vector<string>& masters,
                                odb::dbMaster* corner_master,
                                string prefix)
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
  int corner_cell_width = corner_master->getWidth();

  int total_rows = rows.size();

  for (std::map<std::pair<int, int>, vector<int>>::iterator iter
       = macro_outlines.begin();
       iter != macro_outlines.end();
       ++iter) {
    int x_start = iter->first.first;
    int x_end = iter->first.second;
    vector<int> outline = iter->second;
    for (int i = 0; i < outline.size(); i += 2) {
      int bot_row = outline[i];
      int top_row = outline[i + 1];
      bot_row--;
      top_row++;
      int row_start;
      int row_end;
      odb::dbMaster* incnr_master;
      odb::dbOrientType west_ori;
      if (top_row < total_rows - 1) {
        odb::dbRow* top_row_inst = rows[top_row].front();
        odb::dbOrientType top_row_ori = top_row_inst->getOrient();
        odb::Rect row_bb;
        top_row_inst->getBBox(row_bb);
        int top_row_y = row_bb.yMin();

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
        // do top row
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
        // do corners
        if (top_row_ori == odb::dbOrientType::R0) {
          incnr_master = incnrcap_nwin_master;
          west_ori = odb::dbOrientType::MY;

        } else {
          incnr_master = incnrcap_nwout_master;
          west_ori = odb::dbOrientType::R180;
        }

        // NE corner
        if (checkSymmetry(incnr_master, top_row_ori)) {
          buildCell(block, incnr_master, top_row_ori, x_end, top_row_y, prefix);
        }
        // NW corner
        if (checkSymmetry(incnr_master, west_ori)) {
          buildCell(block,
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
        int bot_row_y = rowbb1.yMin();

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

        // do bottom row
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

        // do corners
        if (bot_row_ori == odb::dbOrientType::MX) {
          incnr_master = incnrcap_nwin_master;
          west_ori = odb::dbOrientType::R180;
        } else {
          incnr_master = incnrcap_nwout_master;
          west_ori = odb::dbOrientType::MY;
        }

        // SE corner
        if (checkSymmetry(incnr_master, bot_row_ori)) {
          buildCell(block, incnr_master, bot_row_ori, x_end, bot_row_y, prefix);
        }
        // SW corner
        if (checkSymmetry(incnr_master, west_ori)) {
          buildCell(block,
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

std::pair<int, int> Tapcell::getMinMaxX(vector<vector<odb::dbRow*>>& rows)
{
  int min_x = -1;
  int max_x = -1;
  int new_min_x;
  int new_max_x;
  for (vector<odb::dbRow*> subrow : rows) {
    for (odb::dbRow* row : subrow) {
      odb::Rect row_bb;
      row->getBBox(row_bb);
      new_min_x = row_bb.xMin();
      new_max_x = row_bb.xMax();
      if (min_x == -1) {
        min_x = new_min_x;
      } else if (min_x > new_min_x) {
        min_x = new_min_x;
      }
      if (max_x == -1) {
        max_x = new_max_x;
      } else if (max_x < new_max_x) {
        max_x = new_max_x;
      }
    }
  }
  return {min_x, max_x};
}

std::map<std::pair<int, int>, vector<int>> Tapcell::getMacroOutlines(
    vector<vector<odb::dbRow*>>& rows)
{
  std::map<std::pair<int, int>, vector<int>> macro_outlines;
  std::pair<int, int> min_max_x = getMinMaxX(rows);

  for (int cur_row = 0; cur_row < rows.size(); cur_row++) {
    vector<odb::dbRow*> subrows = rows[cur_row];
    int subrow_count = subrows.size();

    int macro0 = -1;
    int macro1 = -1;

    for (int cur_subrow = 0; cur_subrow < subrow_count; cur_subrow++) {
      odb::Rect currow_bb;
      subrows[cur_subrow]->getBBox(currow_bb);

      macro1 = currow_bb.xMin();
      if (cur_subrow == 0 && macro1 == min_max_x.first) {
        // still on the first position
        macro1 = -1;
      }

      if (macro0 != macro1) {
        std::pair<int, int> macro0_macro1(macro0, macro1);
        if (macro_outlines.find(macro0_macro1) == macro_outlines.end()) {
          vector<int> outlines;
          outlines.push_back(cur_row);
          macro_outlines.insert(std::pair<std::pair<int, int>, vector<int>>(
              macro0_macro1, outlines));
        } else {
          macro_outlines[macro0_macro1].push_back(cur_row);
        }
      }

      macro0 = currow_bb.xMax();
    }
    if (macro0 != min_max_x.second) {
      // Row is cut at the end
      std::pair<int, int> macro0_macro1(macro0, -1);
      if (macro_outlines.find(macro0_macro1) == macro_outlines.end()) {
        vector<int> outlines;
        outlines.push_back(cur_row);
        macro_outlines.insert(std::pair<std::pair<int, int>, vector<int>>(
            macro0_macro1, outlines));
      } else {
        macro_outlines[macro0_macro1].push_back(cur_row);
      }
    }
  }

  std::map<std::pair<int, int>, vector<int>> macro_outlines_array;
  for (std::map<std::pair<int, int>, vector<int>>::iterator iter
       = macro_outlines.begin();
       iter != macro_outlines.end();
       ++iter) {
    std::pair<int, int> k = iter->first;
    vector<int> all_rows = macro_outlines[k];
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
    if (macro_outlines_array.find(k) == macro_outlines_array.end()) {
      macro_outlines_array.insert(
          std::pair<std::pair<int, int>, vector<int>>(k, new_rows));
    } else {
      macro_outlines_array[k].insert(
          macro_outlines_array[k].end(), new_rows.begin(), new_rows.end());
    }
  }

  return macro_outlines_array;
}

// // proc to detect if blockage overlaps with row
bool Tapcell::overlaps(odb::dbBox* blockage,
                       odb::dbRow* row,
                       int& halo_x,
                       int& halo_y)
{
  odb::Rect rowBB;
  row->getBBox(rowBB);

  // check if Y has overlap first since rows are long and skinny
  int blockage_lly = blockage->yMin() - halo_y;
  int blockage_ury = blockage->yMax() + halo_y;
  int row_lly = rowBB.yMin();
  int row_ury = rowBB.yMax();

  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return false;
  }

  int blockage_llx = blockage->xMin() - halo_x;
  int blockage_urx = blockage->xMax() + halo_x;
  int row_llx = rowBB.xMin();
  int row_urx = rowBB.xMax();

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

int Tapcell::makeSiteLoc(int x, double site_x, int dirc, int& offset)
{
  auto site_int = double(x - offset) / site_x;
  if (dirc == 1) {
    site_int = ceil(site_int);
  } else {
    site_int = floor(site_int);
  }
  site_int = int(site_int * site_x + offset);
  return site_int;
}

void Tapcell::buildCell(odb::dbBlock* block,
                        odb::dbMaster* master,
                        odb::dbOrientType orientation,
                        int x,
                        int y,
                        string prefix)
{
  if (x < 0 || y < 0) {
    return;
  }
  string name = prefix + std::to_string(phy_idx_);
  odb::dbInst* inst = odb::dbInst::create(block, master, name.c_str());
  if (inst == nullptr) {
    return;
  }
  inst->setOrient(orientation);
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  inst->setSourceType(odb::dbSourceType::DIST);

  odb::dbBox* inst_bb = inst->getBBox();
  vector<int> filled_sites_vector;
  filled_sites_vector.push_back(inst_bb->yMin());
  filled_sites_vector.push_back(inst_bb->xMin());
  filled_sites_vector.push_back(inst_bb->xMax());
  filled_sites_.push_back(filled_sites_vector);

  phy_idx_++;
}

void Tapcell::buildRow(odb::dbBlock* block,
                       string name,
                       odb::dbSite* site,
                       int start_x,
                       int end_x,
                       int y,
                       odb::dbOrientType& orient,
                       odb::dbRowDir& direction,
                       int& min_row_width)
{
  int site_width = site->getWidth();

  int new_row_num_sites = (end_x - start_x) / site_width;
  int new_row_width = new_row_num_sites * site_width;

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

vector<vector<odb::dbRow*>> Tapcell::organizeRows()
{
  std::map<int, vector<odb::dbRow*>> rows_dict;
  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    odb::Rect rowBB;
    row->getBBox(rowBB);
    if (rows_dict.find(rowBB.yMin()) == rows_dict.end()) {
      vector<odb::dbRow*> rows_vector;
      rows_vector.push_back(row);
      rows_dict.insert(
          std::pair<int, vector<odb::dbRow*>>(rowBB.yMin(), rows_vector));
    } else {
      rows_dict[rowBB.yMin()].push_back(row);
    }
  }

  // organize rows bottom to top
  vector<vector<odb::dbRow*>> rows;
  for (std::map<int, vector<odb::dbRow*>>::iterator iter = rows_dict.begin();
       iter != rows_dict.end();
       ++iter) {
    int k = iter->first;
    std::map<int, odb::dbRow*> in_row_dict;
    for (odb::dbRow* in_row : rows_dict[k]) {
      odb::Rect row_BB;
      in_row->getBBox(row_BB);
      in_row_dict.insert(std::pair<int, odb::dbRow*>(row_BB.xMin(), in_row));
    }
    // organize sub rows left to right
    vector<odb::dbRow*> in_row;
    for (std::map<int, odb::dbRow*>::iterator iter = in_row_dict.begin();
         iter != in_row_dict.end();
         ++iter) {
      int in_k = iter->first;
      in_row.push_back(in_row_dict[in_k]);
    }
    rows.push_back(in_row);
    in_row.clear();
  }

  return rows;
}

int Tapcell::removeCells(const char* prefix)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  string prefix_string(prefix);
  int removed = 0;

  if (prefix_string.length() == 0) {
    // if no prefix is given, return 0 instead of having all cells removed
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
