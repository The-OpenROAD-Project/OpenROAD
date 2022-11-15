/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include "odb/util.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace tap {

using std::max;
using std::min;
using std::string;
using std::vector;

Tapcell::Tapcell() : db_(nullptr), logger_(nullptr)
{
  reset();
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
  phy_idx_ = 0;
  filled_sites_.clear();
  tap_prefix_ = "TAP_";
  endcap_prefix_ = "PHY_";
}

//---------------------------------------------------------------

void Tapcell::setTapPrefix(const string& tap_prefix)
{
  tap_prefix_ = tap_prefix;
}

void Tapcell::setEndcapPrefix(const string& endcap_prefix)
{
  endcap_prefix_ = endcap_prefix;
}

void Tapcell::clear()
{
  removeCells(tap_prefix_);
  removeCells(endcap_prefix_);

  // Reset global parameters
  reset();
}

int Tapcell::defaultDistance() const
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  return 2 * block->getDbUnitsPerMicron();
}

void Tapcell::cutRows(const Options& options)
{
  vector<odb::dbBox*> blockages = findBlockages();
  odb::dbBlock* block = db_->getChip()->getBlock();
  const int halo_x = options.halo_x >= 0 ? options.halo_x : defaultDistance();
  const int halo_y = options.halo_y >= 0 ? options.halo_y : defaultDistance();
  const int min_row_width
      = options.endcap_master ? 2 * options.endcap_master->getWidth() : 0;
  odb::cutRows(block, min_row_width, blockages, halo_x, halo_y, logger_);
}

void Tapcell::run(const Options& options)
{
  cutRows(options);

  const int dist = options.dist >= 0 ? options.dist : defaultDistance();

  vector<vector<odb::dbRow*>> rows = organizeRows();
  CornercapMasters masters;
  masters.nwin_master = options.cnrcap_nwin_master;
  masters.nwout_master = options.cnrcap_nwout_master;

  if (options.endcap_master != nullptr) {
    insertEndcaps(rows, options.endcap_master, masters);
  }
  if (options.addBoundaryCells()) {
    vector<string> tap_nw_masters;
    tap_nw_masters.push_back(options.tap_nwintie_master);
    tap_nw_masters.push_back(options.tap_nwin2_master);
    tap_nw_masters.push_back(options.tap_nwin3_master);
    tap_nw_masters.push_back(options.tap_nwouttie_master);
    tap_nw_masters.push_back(options.tap_nwout2_master);
    tap_nw_masters.push_back(options.tap_nwout3_master);

    vector<string> tap_macro_masters;
    tap_macro_masters.push_back(options.incnrcap_nwin_master);
    tap_macro_masters.push_back(options.tap_nwin2_master);
    tap_macro_masters.push_back(options.tap_nwin3_master);
    tap_macro_masters.push_back(options.tap_nwintie_master);
    tap_macro_masters.push_back(options.incnrcap_nwout_master);
    tap_macro_masters.push_back(options.tap_nwout2_master);
    tap_macro_masters.push_back(options.tap_nwout3_master);
    tap_macro_masters.push_back(options.tap_nwouttie_master);

    insertAtTopBottom(rows,
                      tap_nw_masters,
                      db_->findMaster(options.cnrcap_nwin_master.c_str()),
                      endcap_prefix_);
    insertAroundMacros(rows,
                       tap_macro_masters,
                       db_->findMaster(options.cnrcap_nwin_master.c_str()),
                       endcap_prefix_);
  }

  if (options.tapcell_master != nullptr) {
    insertTapcells(rows, options.tapcell_master, dist);
  }
  filled_sites_.clear();
}

int Tapcell::insertEndcaps(const vector<vector<odb::dbRow*>>& rows,
                           odb::dbMaster* endcap_master,
                           const CornercapMasters& masters)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbMaster* cnrcap_nwin_master;
  odb::dbMaster* cnrcap_nwout_master;

  bool do_corners;
  if (!masters.nwin_master.empty() && !masters.nwout_master.empty()) {
    if (masters.nwin_master.empty() || masters.nwout_master.empty()) {
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
      odb::Rect row_bb = subrow->getBBox();
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
          left_master
              = pickCornerMaster(getLocationType(llx, rows_above, rows_below),
                                 row_ori,
                                 cnrcap_nwin_master,
                                 cnrcap_nwout_master,
                                 endcap_master);
          right_master
              = pickCornerMaster(getLocationType(urx, rows_above, rows_below),
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
      makeInstance(
          block, right_master, right_ori, loc_2_x, loc_2_y, endcap_prefix_);
    }
  }

  int endcap_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 4, "Inserted {} endcaps.", endcap_count);
  return endcap_count;
}

bool Tapcell::isXInRow(const int x, const vector<odb::dbRow*>& subrow)
{
  for (odb::dbRow* row : subrow) {
    odb::Rect row_bb = row->getBBox();
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
  return NoMacro;
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
                            odb::dbMaster* tapcell_master,
                            int dist)
{
  int start_phy_idx = phy_idx_;
  odb::dbBlock* block = db_->getChip()->getBlock();

  RowFills row_fills = findRowFills();

  std::set<int> rows_with_macros;
  std::map<std::pair<int, int>, vector<int>> macro_outlines
      = getMacroOutlines(rows);
  for (auto& [ignored, bot_top_row] : macro_outlines) {
    for (int i = 0; i < bot_top_row.size(); i += 2) {
      // Traverse all rows cut by a macro
      bot_top_row[i]--;
      bot_top_row[i + 1]++;

      if (bot_top_row[i] >= 0) {
        rows_with_macros.insert(bot_top_row[i]);
      }
      if (bot_top_row[i + 1] <= rows.size()) {
        rows_with_macros.insert(bot_top_row[i + 1]);
      }
    }
  }

  for (int row_idx = 0; row_idx < rows.size(); row_idx++) {
    vector<odb::dbRow*> subrows = rows[row_idx];
    odb::Rect rowbb = subrows[0]->getBBox();
    const int row_y = rowbb.yMin();
    vector<vector<int>> row_fill_check;
    if (row_fills.find(row_y) != row_fills.end()) {
      row_fill_check = row_fills[row_y];
    } else {
      row_fill_check.clear();
    }

    int gaps_above_below = 0;
    if (rows_with_macros.find(row_idx) != rows_with_macros.end()) {
      gaps_above_below = 1;
      rows_with_macros.erase(row_idx);
    }

    for (odb::dbRow* row : subrows) {
      if (!checkSymmetry(tapcell_master, row->getOrient())) {
        continue;
      }

      int offset = 0;
      int pitch = -1;
      const int dist1 = dist;
      const int dist2 = 2 * dist;

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

      odb::Rect row_bb = row->getBBox();
      const int llx = row_bb.xMin();
      const int urx = row_bb.xMax();

      const int site_width = row->getSite()->getWidth();
      for (int x = llx + offset; x < urx; x += pitch) {
        x = odb::makeSiteLoc(x, site_width, 1, llx);
        // Check if site is filled
        const int tap_width = tapcell_master->getWidth();
        odb::dbOrientType ori = row->getOrient();
        bool overlap = checkIfFilled(x, tap_width, ori, row_fill_check);
        if (!overlap) {
          const int lly = row_bb.yMin();
          makeInstance(block, tapcell_master, ori, x, lly, tap_prefix_);
        }
      }
    }
  }
  int tapcell_count = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 5, "Inserted {} tapcells.", tapcell_count);
  return tapcell_count;
}

bool Tapcell::checkIfFilled(int x,
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

  for (const auto& placement : row_insts) {
    if (x_end > placement[0] && x_start < placement[1]) {
      int left_x = placement[0] - width;
      int right_x = placement[1];
      return left_x + right_x > 0;
    }
  }
  return false;
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

  RowFills row_fills = findRowFills();

  for (int cur_row = 0; cur_row < new_rows.size(); cur_row++) {
    for (odb::dbRow* subrow : new_rows[cur_row]) {
      odb::Rect rowbb = subrow->getBBox();
      const int row_y = rowbb.yMin();
      vector<vector<int>> row_fill_check;
      if (row_fills.find(row_y) != row_fills.end()) {
        row_fill_check = row_fills[row_y];
      } else {
        row_fill_check.clear();
      }

      odb::Rect row_bb = subrow->getBBox();
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
                              row_fill_check,
                              prefix);
    }
  }
  int topbottom_cnt = phy_idx_ - start_phy_idx;
  logger_->info(utl::TAP, 6, "Inserted {} top/bottom cells.", topbottom_cnt);
  return topbottom_cnt;
}

void Tapcell::insertAtTopBottomHelper(
    odb::dbBlock* block,
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
    const std::vector<std::vector<int>>& row_fill_check,
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
    if (checkSymmetry(master, ori)
        && !checkIfFilled(x, master->getWidth(), ori, row_fill_check)) {
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
    if (checkSymmetry(tb3_master, ori)
        && !checkIfFilled(x, tb3_master->getWidth(), ori, row_fill_check)) {
      makeInstance(block, tb3_master, ori, x, lly, prefix);
    }
    x += tap3_master_width;
  }

  // Fill with 2s
  for (; x < x_end; x += tap2_master_width) {
    if (checkSymmetry(tb2_master, ori)
        && !checkIfFilled(x, tb2_master->getWidth(), ori, row_fill_check)) {
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

  for (auto& [x_start_end, outline] : macro_outlines) {
    for (int i = 0; i < outline.size(); i += 2) {
      RowFills row_fills = findRowFills();
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
        odb::Rect row_bb = top_row_inst->getBBox();
        const int top_row_y = row_bb.yMin();

        vector<vector<int>> row_fill_check;
        if (row_fills.find(top_row_y) != row_fills.end()) {
          row_fill_check = row_fills[top_row_y];
        } else {
          row_fill_check.clear();
        }

        row_start = x_start;
        row_end = x_end;
        if (row_start == -1) {
          row_start = row_bb.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          odb::Rect rowbb_ = rows[top_row].back()->getBBox();
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
                                row_fill_check,
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
        if (checkSymmetry(incnr_master, top_row_ori)
            && !checkIfFilled(
                x_end, incnr_master->getWidth(), top_row_ori, row_fill_check)) {
          makeInstance(
              block, incnr_master, top_row_ori, x_end, top_row_y, prefix);
        }
        // NW corner
        if (checkSymmetry(incnr_master, west_ori)
            && !checkIfFilled((x_start - incnr_master->getWidth()),
                              incnr_master->getWidth(),
                              west_ori,
                              row_fill_check)) {
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
        odb::Rect rowbb1 = bot_row_inst->getBBox();
        const int bot_row_y = rowbb1.yMin();

        vector<vector<int>> row_fill_check;
        if (row_fills.find(bot_row_y) != row_fills.end()) {
          row_fill_check = row_fills[bot_row_y];
        } else {
          row_fill_check.clear();
        }

        row_start = x_start;
        row_end = x_end;
        if (row_start == -1) {
          row_start = rowbb1.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          odb::Rect rowbb3 = rows[bot_row].back()->getBBox();
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
                                row_fill_check,
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
        if (checkSymmetry(incnr_master, bot_row_ori)
            && !checkIfFilled(
                x_end, incnr_master->getWidth(), bot_row_ori, row_fill_check)) {
          makeInstance(
              block, incnr_master, bot_row_ori, x_end, bot_row_y, prefix);
        }
        // SW corner
        if (checkSymmetry(incnr_master, west_ori)
            && !checkIfFilled((x_start - incnr_master->getWidth()),
                              incnr_master->getWidth(),
                              west_ori,
                              row_fill_check)) {
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

const std::pair<int, int> Tapcell::getMinMaxX(
    const vector<vector<odb::dbRow*>>& rows)
{
  int min_x = std::numeric_limits<int>::min();
  int max_x = std::numeric_limits<int>::max();
  for (vector<odb::dbRow*> subrow : rows) {
    int new_min_x;
    int new_max_x;
    for (odb::dbRow* row : subrow) {
      odb::Rect row_bb = row->getBBox();
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

Tapcell::RowFills Tapcell::findRowFills()
{
  int y;
  std::map<int, vector<vector<int>>> row_fills;
  for (FilledSites& placement : filled_sites_) {
    y = placement.yMin;
    int x_start = placement.xMin;
    int x_end = placement.xMax;
    vector<int> x_start_end;
    x_start_end.push_back(x_start);
    x_start_end.push_back(x_end);
    row_fills[y].push_back(x_start_end);
  }

  for (const auto& [y, ignored] : row_fills) {
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

  return row_fills;
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
      odb::Rect currow_bb = subrows[cur_subrow]->getBBox();

      xMin_pos = currow_bb.xMin();
      if (cur_subrow == 0 && xMin_pos == min_max_x.first) {
        // Still on the first position
        xMin_pos = -1;
      }

      if (xMax_pos != xMin_pos) {
        macro_outlines[{xMax_pos, xMin_pos}].push_back(cur_row);
      }

      xMax_pos = currow_bb.xMax();
    }
    if (xMax_pos != min_max_x.second) {
      // Row is cut at the end
      macro_outlines[{xMax_pos, -1}].push_back(cur_row);
    }
  }

  std::map<std::pair<int, int>, vector<int>> macro_outlines_array;
  for (const auto& [k, all_rows] : macro_outlines) {
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

vector<odb::dbBox*> Tapcell::findBlockages()
{
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
  odb::dbInst* inst = odb::dbInst::create(block,
                                          master,
                                          name.c_str(),
                                          /* physical_only */ true);
  if (inst == nullptr) {
    logger_->error(utl::TAP,
                   33,
                   "Not able to build instance {} with master {}.",
                   name,
                   master->getName());
  }
  inst->setOrient(orientation);
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  inst->setSourceType(odb::dbSourceType::DIST);

  odb::dbBox* inst_bb = inst->getBBox();
  FilledSites filled;
  filled.yMin = inst_bb->yMin();
  filled.xMin = inst_bb->xMin();
  filled.xMax = inst_bb->xMax();
  filled_sites_.push_back(filled);

  phy_idx_++;
}

// Return rows vector organized according to yMin value,
// each subvector organized according to xMin value
vector<vector<odb::dbRow*>> Tapcell::organizeRows()
{
  std::map<int, vector<odb::dbRow*>> rows_dict;
  // Gather rows according to yMin values
  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    odb::Rect rowBB = row->getBBox();
    rows_dict[rowBB.yMin()].push_back(row);
  }

  // Organize rows bottom to top
  vector<vector<odb::dbRow*>> rows;
  for (const auto& [k, ignored] : rows_dict) {
    std::map<int, odb::dbRow*> in_row_dict;
    for (odb::dbRow* in_row : rows_dict[k]) {
      odb::Rect row_BB = in_row->getBBox();
      in_row_dict.insert({row_BB.xMin(), in_row});
    }
    // Organize sub rows left to right
    vector<odb::dbRow*> in_row;
    for (const auto& [in_k, ignored] : in_row_dict) {
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
    if (inst->getName().find(prefix_string) == 0) {
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
