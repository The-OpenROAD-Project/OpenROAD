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
#include "sta/StaMain.hh"

#include "opendb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"
#include <map>
#include <utility>
#include <string>


namespace tap {


using std::max;
using std::min;
using std::vector;


Tapcell::Tapcell()
  : db_(nullptr)
{
}

Tapcell::~Tapcell()
{
}

void Tapcell::init(odb::dbDatabase *db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;

  // Define swig TCL commands.
  //Tap_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  //sta::evalTclInit(tcl_interp, sta::tap_tcl_inits);
}


//}

//---------------------------------------------------------------

//namespace evaltap{
// int phy_idx;
// std::vector<std::vector<int>> filled_sites;
//utl::Logger *logger_;
// std::vector<odb::dbSite> filled_sites;    
// std::string default_tapcell_prefix = "TAP_";
// std::string default_endcap_prefix = "PHY_";

// void Tapcell::clear (std::string tap_prefix, std::string endcap_prefix ) {
//   int taps_removed = remove_cells(tap_prefix);
//   int endcaps_removed = remove_cells(endcap_prefix);

//   // Reset global parameters 
//   phy_idx = 0;
//   filled_sites.clear();
// }

int phy_idx;
std::vector<std::vector<int>> filled_sites;
//variable filled_sites

void Tapcell::run(odb::dbMaster* endcap_master, int halo_x, int halo_y, const char* cnrcap_nwin_master, const char* cnrcap_nwout_master, const char* endcap_prefix, int add_boundary_cell, const char* tap_nwintie_master, const char* tap_nwin2_master, const char* tap_nwin3_master, const char* tap_nwouttie_master, const char* tap_nwout2_master, const char* tap_nwout3_master, const char* incnrcap_nwin_master, const char* incnrcap_nwout_master, const char* tapcell_master, int dist, const char* tap_prefix)
{
  cut_rows(endcap_master, find_blockages(), halo_x, halo_y);

  std::vector<std::vector<odb::dbRow*>> rows = organize_rows();
  std::vector<std::string> cnrcap_masters;
  std::string cnrcap_nwin_master_string(cnrcap_nwin_master);
  std::string cnrcap_nwout_master_string(cnrcap_nwout_master);
  cnrcap_masters.push_back(cnrcap_nwin_master_string);
  //logger_->info(utl::TAP, 0, "Name is: {} ", tap_nwin3_master);
  cnrcap_masters.push_back(cnrcap_nwout_master_string);

  phy_idx = 0;
  //set tap::filled_sites []
  if (endcap_master != nullptr ) {
    insert_endcaps(rows, endcap_master, cnrcap_masters, endcap_prefix);
  }
  if (add_boundary_cell) {
    //logger_->info(utl::TAP, 40, "boundary is: {} ", add_boundary_cell);
    std::vector<std::string> tap_nw_masters;
    tap_nw_masters.push_back(tap_nwintie_master);
    tap_nw_masters.push_back(tap_nwin2_master);
    tap_nw_masters.push_back(tap_nwin3_master);
    tap_nw_masters.push_back(tap_nwouttie_master);
    tap_nw_masters.push_back(tap_nwout2_master);
    tap_nw_masters.push_back(tap_nwout3_master);

    std::vector<std::string> tap_macro_masters;
    tap_macro_masters.push_back(incnrcap_nwin_master);
    tap_macro_masters.push_back(tap_nwin2_master);
    tap_macro_masters.push_back(tap_nwin3_master);
    tap_macro_masters.push_back(tap_nwintie_master);
    tap_macro_masters.push_back(incnrcap_nwout_master);
    tap_macro_masters.push_back(tap_nwout2_master);
    tap_macro_masters.push_back(tap_nwout3_master);
    tap_macro_masters.push_back(tap_nwouttie_master);

    insert_at_top_bottom(rows, tap_nw_masters, db_->findMaster(cnrcap_nwin_master), endcap_prefix);
    insert_around_macros(rows, tap_macro_masters, db_->findMaster(cnrcap_nwin_master), endcap_prefix);
  }
  insert_tapcells(rows, tapcell_master, dist, tap_prefix);
}

void Tapcell::cut_rows(odb::dbMaster* endcap_master, std::vector<odb::dbInst*> blockages,int halo_x, int halo_y)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  int rows_count = (block->getRows()).size();
  int block_count = blockages.size();
  int min_row_width;

  if (endcap_master != nullptr) {
    int end_width = endcap_master->getWidth();
    min_row_width = 2*end_width;
  } else {
    min_row_width = 0;
  }
  
  //Gather rows needing to be cut up front
  std::vector<odb::dbRow*> blocked_rows;   
  std::map<std::string, std::vector<odb::dbBox*>> row_blockages;
  std::vector<odb::dbBox*> row_blockages_bbox;
  for (odb::dbInst* blockage : blockages) {
    for(odb::dbRow* row : block->getRows()) {
      if (overlaps(blockage, row, halo_x, halo_y)) {
        std::string  row_name = row->getName();
        if (row_blockages.find(row_name) == row_blockages.end()) {
          blocked_rows.push_back(row);
        } 
        row_blockages_bbox.push_back(blockage->getBBox());
        row_blockages.insert(std::pair<std::string, vector<odb::dbBox*>>(row_name, row_blockages_bbox));
      }
    }
  }
  
  // cut rows around macros
  for(odb::dbRow* row : blocked_rows) {
    cut_row(block, row, row_blockages, min_row_width, halo_x, halo_y);
  }
  int cut_rows_count = block->getRows().size()-rows_count;
  logger_->info(utl::TAP, 1, "Macro blocks found: {}", block_count);
  logger_->info(utl::TAP, 2, "Original rows: {}", rows_count);
  logger_->info(utl::TAP, 3, "Created rows: {}", cut_rows_count);
}

void  Tapcell::cut_row(odb::dbBlock* block, odb::dbRow* row, std::map<std::string, std::vector<odb::dbBox*>> row_blockages, int min_row_width, int halo_x, int halo_y) {
  std::string row_name = row->getName();
  //odb::Rect &row_bb;
  //ASK
  odb::Rect row_bb;
  row->getBBox(row_bb);

  odb::dbSite* row_site = row->getSite();
  int site_width = row_site->getWidth();
  odb::dbOrientType orient = row->getOrient();
  odb::dbRowDir direction = row->getDirection();

  int start_origin_x = row_bb.xMin();
  int start_origin_y = row_bb.yMin();

  int curr_min_row_width = min_row_width + 2*site_width;

  //ASK: set row_blockage_bboxs [dict get $row_blockages $row_name] ?????
  std::vector<odb::dbBox*> row_blockage_bboxs = row_blockages[row_name];
  //set row_blockage_xs []
  std::vector<std::pair<int,int>> row_blockage_xs;
  //WRONG WHAT'S RIGHT?????
  for(odb::dbBox* row_blockage_bbox : row_blockages[row_name]) {
    row_blockage_xs.push_back(std::make_pair(row_blockage_bbox->xMin(), row_blockage_bbox->xMax()));
  }
  std::vector<int> Data;
  // CONVERT TO INTEGERS????
  //set row_blockage_xs [lsort -integer -index 0 $row_blockage_xs]
  std::sort(row_blockage_xs.begin(), row_blockage_xs.end());

  int row_sub_idx = 1;
  for( std::pair<int,int> blockage : row_blockage_xs) {
    int blockage_x0 = blockage.first;
    int blockage_x1 = blockage.second;
    //blockage = {blockage_x0, blockage_x1};
    // ensure rows are an integer length of sitewidth
    int new_row_end_x = make_site_loc(blockage_x0 - halo_x, site_width, -1, start_origin_x);
    build_row(block, row_name + "_" + std::to_string(row_sub_idx), row_site, start_origin_x, new_row_end_x, start_origin_y, orient, direction, curr_min_row_width);
    row_sub_idx++;
    
    start_origin_x = make_site_loc(blockage_x1 + halo_x, site_width, 1, start_origin_x);
  }

  // Make last row
  build_row(block, row_name + "_" + std::to_string(row_sub_idx), row_site, start_origin_x, row_bb.xMax(), start_origin_y, orient, direction, curr_min_row_width);
  
  // Remove current row
  odb::dbRow::destroy(row);
  //odb::inDbRowDestroy(row);
}

int Tapcell::insert_endcaps(std::vector<std::vector<odb::dbRow*>> rows, odb::dbMaster* endcap_master, std::vector<std::string> cnrcap_masters, const char* prefix) {
  int start_phy_idx = phy_idx;
  odb::dbBlock *block = db_->getChip()->getBlock();
  int do_corners;
  odb::dbMaster* cnrcap_nwin_master;
  odb::dbMaster* cnrcap_nwout_master;
  odb::dbMaster* masterl;
  odb::dbMaster* masterr;

  int endcapwidth = endcap_master->getWidth();

  if (cnrcap_masters.size() == 2) {
    //lassign $cnrcap_masters cnrcap_nwin_master_name cnrcap_nwout_master_name
    std::string cnrcap_nwin_master_name = cnrcap_masters[0];
    std::string cnrcap_nwout_master_name = cnrcap_masters[1];
    std::string invalid = "INVALID";
    if (cnrcap_nwin_master_name.compare("INVALID") == 0 || cnrcap_nwout_master_name.compare("INVALID") == 0) {
      do_corners = 0;
    } else {
      do_corners = 1;

      cnrcap_nwin_master = db_->findMaster(cnrcap_nwin_master_name.c_str());
      if ( cnrcap_nwin_master == nullptr ) {
        logger_->error(utl::TAP, 12, "Master {} not found.", cnrcap_nwin_master_name);
      }
      cnrcap_nwout_master = db_->findMaster(cnrcap_nwout_master_name.c_str());

      if ( cnrcap_nwout_master == nullptr ) {
        logger_->error(utl::TAP, 13, "Master {} not found.", cnrcap_nwout_master_name);
      }
      //get_min_max_x?????
      std::pair<int, int> min_max_x;
      min_max_x = get_min_max_x(rows); 
    }
  } else {
     do_corners = 0;
  }

  int bottom_row = 0;
  int top_row = rows.size()-1;

  for (int cur_row = bottom_row;  cur_row <= top_row; cur_row++) {
    //foreach subrow [lindex $rows $cur_row] {
    for(odb::dbRow* subrow : rows[cur_row]) {
      if (!(check_symmetry(endcap_master, subrow->getOrient()))) {
        continue;
      }
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      auto row_ori = subrow->getOrient();

      int llx = row_bb.xMin();
      int lly = row_bb.yMin();
      int urx = row_bb.xMax();
      int ury = row_bb.yMax();

      if (do_corners) {
        if (cur_row == top_row) {
          //pick_corner_master????
          masterl = pick_corner_master(1, row_ori, cnrcap_nwin_master, cnrcap_nwout_master, endcap_master);
          masterr = masterl;
        } else if(cur_row == bottom_row) {
          masterl = pick_corner_master(-1, row_ori, cnrcap_nwin_master, cnrcap_nwout_master, endcap_master);
          masterr = masterl;
        } else {
          auto rows_above = rows[cur_row + 1];
          auto rows_below = rows[cur_row - 1];
          masterl = pick_corner_master(is_x_corner(llx, rows_above, rows_below), row_ori, cnrcap_nwin_master, cnrcap_nwout_master, endcap_master);
          masterr = pick_corner_master(is_x_corner(urx, rows_above, rows_below), row_ori, cnrcap_nwin_master, cnrcap_nwout_master, endcap_master);
        }
      } else {
        masterl = endcap_master;
        masterr = masterl;
      }

      if ((masterl->getWidth()) > (urx - llx)) {
        continue;
      }

      build_cell(block, masterl, row_ori, llx, lly, prefix);

      int master_x = masterr->getWidth();
      int master_y = masterr->getHeight();

      int loc_2_x = urx - master_x;
      int loc_2_y = ury - master_y;
      if (llx == loc_2_x && lly == loc_2_y) {
        logger_->warn(utl::TAP, 9, "row {} have enough space for only one endcap.", subrow->getName());
        continue;
      }

      odb::dbOrientType right_ori = row_ori;
      if (row_ori == odb::dbOrientType::MX ) {
        right_ori = odb::dbOrientType::R180;
      } else if ( row_ori == odb::dbOrientType::R0) {
        right_ori = odb::dbOrientType::MY;
      }
      build_cell(block, masterr, right_ori, loc_2_x, loc_2_y, prefix);
    }
  }

  int endcap_count = phy_idx - start_phy_idx;
  logger_->info(utl::TAP, 4, "Endcaps inserted: {}", endcap_count);
  return endcap_count;
}

bool Tapcell::is_x_in_row(int x, std::vector<odb::dbRow*> subrow) {
  for(odb::dbRow* row : subrow) {
    odb::Rect row_bb;
    row->getBBox(row_bb);
    if (x >= row_bb.xMin() && x <= row_bb.xMax()) {
      return 1;
    }
  }
  return 0;
}

bool Tapcell::is_x_corner(int x, std::vector<odb::dbRow*> rows_above, std::vector<odb::dbRow*>  rows_below) {
  int in_above = is_x_in_row(x, rows_above);
  int in_below = is_x_in_row(x, rows_below);

  if (in_above && !in_below) {
    return -1;
  }
  if (!in_above && in_below) {
    return 1;
  }
  return 0;
}

odb::dbMaster* Tapcell::pick_corner_master(int top_bottom, odb::dbOrientType ori, odb::dbMaster* cnrcap_nwin_master, odb::dbMaster* cnrcap_nwout_master, odb::dbMaster* endcap_master) {
  if (top_bottom == 1 ) {
    if (ori == odb::dbOrientType::MX ) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else if ( top_bottom == -1 ) {
    if ( ori == odb::dbOrientType::R0 ) {
      return cnrcap_nwin_master;
    } else {
      return cnrcap_nwout_master;
    }
  } else {
    return endcap_master;
  }
}

int Tapcell::insert_tapcells(std::vector<vector<odb::dbRow*>> rows, std::string tapcell_master, int dist, std::string prefix) {
  int start_phy_idx = phy_idx;
  std::vector<int> xs;
  std::vector<int> prev_x;
  std::vector<vector<int>> merged_placements;
  odb::dbBlock* block = db_->getChip()->getBlock();

  odb::dbMaster* master = db_->findMaster(tapcell_master.c_str());
  if (master == nullptr) {
    logger_->error(utl::TAP, 11, "Master {} not found.", tapcell_master);
  }
  int tap_width = master->getWidth();

  //int dist1 = odb::Rect::microns_to_dbu(dist);
  int dist1 = dist*(db_->getTech()->getLefUnits());
  int dist2 = 2*dist*(db_->getTech()->getLefUnits());
  int y;
  std::map<int, std::vector<vector<int>>> row_fills;
  for(std::vector<int> placement : filled_sites) {
    y = placement[0];
    int x_start = placement[1];
    int x_end = placement[2];
    std::vector<int> x_start_end;
    x_start_end.push_back(x_start);
    x_start_end.push_back(x_end);
    std::vector<vector<int>> vector2;
    vector2.push_back(x_start_end);
    row_fills.insert(std::pair<int, std::vector<vector<int>>> (y, vector2));
    //dict lappend row_fills $y "$x_start $x_end"
  }

  //for(int i=0; i<row_fills.size(); i++) {
  for(std::map<int, std::vector<vector<int>>>::iterator iter = row_fills.begin(); iter != row_fills.end(); ++iter) {
    int x = iter->first;
    for(std::vector<int> xs : row_fills[x]) {
    //xs = row_fills[x];
      if (prev_x.size() == 0) {
        prev_x = xs;
      } else {
        if (xs[0] == prev_x[1]) {
          prev_x[1] = xs[0];
        } else {
            merged_placements.push_back(prev_x);
          prev_x = xs;
        }
      }
    }
    if (prev_x.size() != 0) {
            merged_placements.push_back(prev_x);
      //merged_placements.push_back(prev_x);
    }
    if (prev_x.size() == 0) {
      //merged_placements.push_back(xs);
            merged_placements.push_back(xs);
    }
    //std::map<int, std::vector<int>>::iterator itr;
    //row_fills.find(y);
    //if(itr != row_fills.end()) {
    row_fills[y] = merged_placements;
    //}
  }

  std::map<int, int> rows_with_macros;
  //ASK
  //for(auto macro_rows : get_macro_outlines(rows)) {
  std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> macro_outlines = get_macro_outlines(rows);
  for(std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>::iterator iter = macro_outlines.begin(); iter != macro_outlines.end(); ++iter) {
    //foreach {bot_row top_row} $macro_rows {
    for(auto bot_top_row : iter->second) {
      bot_top_row.first--;
      bot_top_row.second++;

      if (bot_top_row.first >= 0) {
        rows_with_macros.insert(std::pair<int, int>(bot_top_row.first,0));
      }
      if (bot_top_row.second <= rows.size()) {
        rows_with_macros.insert(std::pair<int, int>(bot_top_row.second,0));
      }
    }
  }

  for(int row_idx = 0; row_idx < rows.size(); row_idx++) {
    std::vector<odb::dbRow*> subrows = rows[row_idx];
    odb::Rect rowbb;
    subrows[0]->getBBox(rowbb);
    std::vector<vector<int>> row_fill_check;
    int row_y = rowbb.yMin();
    if (row_fills.find(row_y)!=row_fills.end()) {
      row_fill_check = row_fills[row_y];
    } else {
      row_fill_check.clear();
    }

    int gaps_above_below = 0;
    if (rows_with_macros[0] == row_idx) {
      gaps_above_below = 1;
      rows_with_macros.erase(0);
    }

    for(odb::dbRow* row:subrows) {
      if (!check_symmetry(master, row->getOrient())) {
        continue;
      }
      int site_x = row->getSite()->getWidth();
      odb::Rect row_bb;
      row->getBBox(row_bb);
      int llx = row_bb.xMin();
      int lly = row_bb.yMin();
      int urx = row_bb.xMax();
      int ury = row_bb.yMax();
      auto ori = row->getOrient();

      int offset = 0;
      int pitch = -1;

      if ((row_idx % 2) == 0) {
        offset = dist1;
      } else {
        offset = dist2;
      }

      if (row_idx == 0 || row_idx == rows.size()-1 || gaps_above_below) {
        int pitch = dist1;
        int offset = dist1;
      } else {
        int pitch = dist2;
      }

      for (int x = llx+offset; x < urx; x = x+pitch) {
        x = make_site_loc(x, site_x, -1, llx);

        //check if site is filled
        bool overlap = check_if_filled(x, tap_width, ori, row_fill_check);
        if (overlap == 0) {
          build_cell(block, master, ori, x, lly, prefix);
        }
      }
    }
  }
  int tapcell_count = phy_idx - start_phy_idx;
  logger_->info(utl::TAP, 5, "Tapcells inserted: {}", tapcell_count);
  return tapcell_count;
}

// //ASK
// //TYPE OF RETURN??
bool Tapcell::check_if_filled(int x, int width, odb::dbOrientType orient, std::vector<std::vector<int>> row_insts) {
  int x_start;
  int x_end;
  if (orient == odb::dbOrientType::MY || orient == odb::dbOrientType::R180) {
    x_start = x-width;
    x_end = x;
  } else {
    x_start = x;
    x_end = x+width;
  }
  
  for(auto placement : row_insts) {
    if (x_end > placement[0] && x_start < placement[1]) {
      int left_x = placement[0] - width;
      int right_x = placement[1];
      std::vector<std::pair<int, int>> value;
      //ASK: RETURN CORRECT?
      return left_x+right_x;
    }
  }
  return 0;
}

int Tapcell::insert_at_top_bottom(std::vector<std::vector<odb::dbRow*>> rows, std::vector<std::string> masters, odb::dbMaster* endcap_master, std::string prefix) {
  int row_info;
  int start_phy_idx = phy_idx;
  odb::dbBlock* block = db_->getChip()->getBlock();
  std::string tap_nwintie_master_name;
  std::string tap_nwin2_master_name;
  std::string tap_nwin3_master_name;
  std::string tap_nwouttie_master_name;
  std::string tap_nwout2_master_name;
  std::string tap_nwout3_master_name;

  if (masters.size() == 6) {
    tap_nwintie_master_name = masters[0];
    tap_nwin2_master_name = masters[1];
    tap_nwin3_master_name = masters[2];
    tap_nwouttie_master_name = masters[3];
    tap_nwout2_master_name = masters[4];
    tap_nwout3_master_name = masters[5];

    // lassign $masters tap_nwintie_master_name tap_nwin2_master_name tap_nwin3_master_name \
    // tap_nwouttie_master_name tap_nwout2_master_name tap_nwout3_master_name
  }

  odb::dbMaster* tap_nwintie_master = db_->findMaster(tap_nwintie_master_name.c_str());
  if (tap_nwintie_master == nullptr) {
    logger_->error(utl::TAP, 18, "Master {} not found.", tap_nwintie_master_name);
  }
  odb::dbMaster* tap_nwin2_master = db_->findMaster(tap_nwin2_master_name.c_str());
  if (tap_nwin2_master == nullptr) {
    logger_->error(utl::TAP, 19, "Master {} not found.", tap_nwin2_master_name);
  }
  odb::dbMaster* tap_nwin3_master = db_->findMaster(tap_nwin3_master_name.c_str());
  if (tap_nwin3_master == nullptr) {
    logger_->error(utl::TAP, 20, "Master {} not found.", tap_nwin3_master_name);
  }
  odb::dbMaster* tap_nwouttie_master = db_->findMaster(tap_nwouttie_master_name.c_str());
  if (tap_nwouttie_master == nullptr) {
    logger_->error(utl::TAP, 21, "Master $tap_nwouttie_master_name not found.");
  }
  odb::dbMaster* tap_nwout2_master = db_->findMaster(tap_nwout2_master_name.c_str());
  if (tap_nwout2_master == nullptr) {
    logger_->error(utl::TAP, 22, "Master {} not found.", tap_nwout2_master_name);
  }
  odb::dbMaster* tap_nwout3_master = db_->findMaster(tap_nwout3_master_name.c_str());
  if (tap_nwout3_master == nullptr) {
    logger_->error(utl::TAP, 23, "Master {} not found.", tap_nwout3_master_name);
  }

  int tbtiewidth = tap_nwintie_master->getWidth();
  int endcapwidth = endcap_master->getWidth();

  std::vector<std::vector<odb::dbRow*>> new_rows;
  // grab bottom
  new_rows.push_back(rows[0]);
  // grab top
  new_rows.push_back(rows.back());
  
  for (int cur_row = 0; cur_row < new_rows.size(); cur_row++) {
    for(odb::dbRow* subrow : new_rows[cur_row]) {
      int site_x = subrow->getSite()->getWidth();
      odb::Rect row_bb;
      subrow->getBBox(row_bb);
      int llx = row_bb.xMin();
      int lly = row_bb.yMin();
      int urx = row_bb.xMax();
      int ury = row_bb.yMax();

      odb::dbOrientType ori = subrow->getOrient();

      int x_start = llx+endcapwidth;
      int x_end = urx-endcapwidth;

      insert_at_top_bottom_helper(block, cur_row, 0, ori, x_start, x_end, lly, tap_nwintie_master, tap_nwin2_master, tap_nwin3_master, tap_nwouttie_master, tap_nwout2_master, tap_nwout3_master, prefix);
    }
  }
  int topbottom_cnt = phy_idx - start_phy_idx;
  logger_->info(utl::TAP, 6, "Top/bottom cells inserted: {}", topbottom_cnt);
  return topbottom_cnt;
}

void Tapcell::insert_at_top_bottom_helper(odb::dbBlock* block, int top_bottom, bool is_macro, odb::dbOrientType ori, int x_start, int x_end, int lly, odb::dbMaster* tap_nwintie_master, odb::dbMaster* tap_nwin2_master, odb::dbMaster* tap_nwin3_master, odb::dbMaster* tap_nwouttie_master, odb::dbMaster* tap_nwout2_master, odb::dbMaster* tap_nwout3_master, std::string prefix) {
  odb::dbMaster* master;
  odb::dbMaster* tb2_master;
  odb::dbMaster* tb3_master;
  if (top_bottom == 1) {
    // top
    if ((ori == odb::dbOrientType::R0 && is_macro) || (ori == odb::dbOrientType::MX && !is_macro)) {
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
    if ((ori == odb::dbOrientType::MX && is_macro) || (ori == odb::dbOrientType::R0 && !is_macro)) {
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
  
  int tbtiecount = std::floor((x_end-x_start) / tbtiewidth);
  // ensure there is room at the end of the row for atleast one tb2, if needed
  int remaining_distance = x_end - x_start - (tbtiecount*tbtiewidth);
  if (remaining_distance != 0 && remaining_distance < tap2_master_width) {
    tbtiecount--;
  }
  //insert tb tie
  int x = x_start;
  for (int n = 0; n < tbtiecount ; n++) {
    if (check_symmetry(master, ori)) {
      build_cell(block, master, ori, x, lly, prefix);
    }
    int x = x+tbtiewidth;
  }

  // fill remaining sites
  int tb3tiecount = std::floor((x_end-x) / tap3_master_width);
  remaining_distance = x_end - x - (tb3tiecount*tap3_master_width);
  while ((remaining_distance % tap2_master_width) != 0 && remaining_distance >= 0) {
    tb3tiecount--;
    remaining_distance = x_end - x - (tb3tiecount*tap3_master_width);
  }
  
  // fill with 3s
  for (int n=0; n < tb3tiecount; n++) {
    if (check_symmetry(tb3_master, ori)) {
      build_cell(block,tb3_master, ori, x, lly, prefix);
    }
    int x = x+tap3_master_width;
  }
  
  // fill with 2s
  for (x=x; x < x_end; x = x+tap2_master_width) {
    if (check_symmetry(tb2_master, ori)) {
      build_cell(block, tb2_master, ori, x, lly, prefix);
    }
  }
}

int Tapcell::insert_around_macros(std::vector<std::vector<odb::dbRow*>> rows, std::vector<std::string> masters, odb::dbMaster* corner_master, std::string prefix) {
  int phy_idx;
  int start_phy_idx = phy_idx;
  int x_start;
  int x_end;
  int row_end;
  int row_start;
  odb::dbMaster* incnr_master;
  odb::dbOrientType west_ori;
  odb::dbBlock* block = db_->getChip()->getBlock();
  std::string incnrcap_nwin_master_name;
  std::string tap_nwin2_master_name;
  std::string tap_nwin3_master_name;
  std::string tap_nwintie_master_name;
  std::string incnrcap_nwout_master_name;
  std::string tap_nwout2_master_name;
  std::string tap_nwout3_master_name;
  std::string tap_nwouttie_master_name;

  if (masters.size() == 8) {
    incnrcap_nwin_master_name = masters[0];
    tap_nwin2_master_name = masters[1];
    tap_nwin3_master_name = masters[2];
    tap_nwintie_master_name = masters[3];
    incnrcap_nwout_master_name = masters[4];
    tap_nwout2_master_name = masters[5];
    tap_nwout3_master_name = masters[6];
    tap_nwouttie_master_name = masters[7];
      // lassign $masters incnrcap_nwin_master_name tap_nwin2_master_name tap_nwin3_master_name tap_nwintie_master_name \
    // incnrcap_nwout_master_name tap_nwout2_master_name tap_nwout3_master_name tap_nwouttie_master_name
  }
  
  odb::dbMaster* tap_nwintie_master = db_->findMaster(tap_nwintie_master_name.c_str());
  if (tap_nwintie_master == nullptr) {
    logger_->error(utl::TAP, 24, "Master {} not found.", tap_nwintie_master_name);
  }
  odb::dbMaster* tap_nwin2_master = db_->findMaster(tap_nwin2_master_name.c_str());
  if (tap_nwin2_master == nullptr) {
    logger_->error(utl::TAP, 25, "Master {} not found.", tap_nwin2_master_name);
  }
  odb::dbMaster* tap_nwin3_master = db_->findMaster(tap_nwin3_master_name.c_str());
  if (tap_nwin3_master == nullptr) {
    logger_->error(utl::TAP, 26, "Master {} not found.", tap_nwin3_master_name);
  }
  odb::dbMaster* tap_nwouttie_master = db_->findMaster(tap_nwouttie_master_name.c_str());
  if (tap_nwouttie_master == nullptr) {
    logger_->error(utl::TAP, 27, "Master {} not found.", tap_nwouttie_master_name);
  }
  odb::dbMaster* tap_nwout2_master = db_->findMaster(tap_nwout2_master_name.c_str());
  if (tap_nwout2_master == nullptr) {
    logger_->error(utl::TAP, 28, "Master {} not found.", tap_nwout2_master_name);
  }
  odb::dbMaster* tap_nwout3_master = db_->findMaster(tap_nwout3_master_name.c_str());
  if (tap_nwout3_master == nullptr) {
    logger_->error(utl::TAP, 29, "Master {} not found.", tap_nwout3_master_name);
  }
  odb::dbMaster* incnrcap_nwin_master = db_->findMaster(incnrcap_nwin_master_name.c_str());
  if (incnrcap_nwin_master == nullptr) {
    logger_->error(utl::TAP, 30, "Master {} not found.", incnrcap_nwin_master_name);
  }
  odb::dbMaster* incnrcap_nwout_master = db_->findMaster(incnrcap_nwout_master_name.c_str());
  if (incnrcap_nwout_master == nullptr) {
    logger_->error(utl::TAP, 31, "Master {} not found.", incnrcap_nwout_master_name);
  }
  
  // find macro outlines
  //RIGHT TYPE FOR MACRO OUTLINES??
  std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> macro_outlines = get_macro_outlines(rows);
  
  int corner_cell_width = corner_master->getWidth();
  
  int total_rows = rows.size();

  // std::map<auto, std::vector<odb::dbRow*>>iterator it;

  //for(it=macro_outlines.begin(); it!=macro_outlines.end(); it++)
  //{
  for(std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>::iterator iter = macro_outlines.begin(); iter != macro_outlines.end(); ++iter){
    //lassign $key x_start x_end
    x_start = iter->first.first;
    x_end = iter->first.second;
    //foreach {bot_row top_row} $outlines {
    for(auto outline : iter->second) {
      // move to actual rows
      outline.first--;
      outline.second++;
      // incr bot_row -1
      // incr top_row
      
      if (outline.second < total_rows-1) {
        auto top_row_inst = rows[outline.second][0];
        auto top_row_ori = top_row_inst->getOrient();
        odb::Rect row_bb;
        top_row_inst->getBBox(row_bb);
        int top_row_y = row_bb.yMin();
        
        int row_start = x_start;
        int row_end = x_end;
        if (row_start == -1) {
          odb::Rect rowbb;
          top_row_inst->getBBox(rowbb);
          int row_start = rowbb.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          //set row_end [[[lindex $rows $top_row end] getBBox] xMax]
          odb::Rect rowbb;
          rows[outline.second].back()->getBBox(rowbb);
          int row_end = rowbb.xMax();
          row_end = row_end - corner_cell_width;
        }
        // do top row
        insert_at_top_bottom_helper(block, 1, 1, top_row_ori, row_start, row_end, top_row_y, tap_nwintie_master, tap_nwin2_master, tap_nwin3_master, tap_nwouttie_master, tap_nwout2_master, tap_nwout3_master, prefix);
        // do corners
        if (top_row_ori == odb::dbOrientType::R0 ) {
          incnr_master = incnrcap_nwin_master;
          west_ori = odb::dbOrientType::MY;
        } else {
          incnr_master = incnrcap_nwout_master;
          west_ori = odb::dbOrientType::R180;
        }
        
        // NE corner
        if (check_symmetry(incnr_master, top_row_ori)) {
          build_cell(block, incnr_master, top_row_ori, x_end, top_row_y, prefix);
        }
        // NW corner
        if (check_symmetry(incnr_master, west_ori)) {
          build_cell(block, incnr_master, west_ori, x_start - incnr_master->getWidth(), top_row_y, prefix);
        }
      }
      if(outline.first >= 1) {
        //set bot_row_inst [lindex $rows $bot_row 0]
        auto bot_row_inst = rows[outline.first][0];
        auto bot_row_ori = bot_row_inst->getOrient();
        odb::Rect rowbb1;
        bot_row_inst->getBBox(rowbb1);
        int bot_row_y = rowbb1.yMin();
        
        row_start = x_start;
        row_end = x_end;
        if (row_start == -1) {
          odb::Rect rowbb2;
          bot_row_inst->getBBox(rowbb2);
          row_start = rowbb2.xMin();
          row_start = row_start + corner_cell_width;
        }
        if (row_end == -1) {
          //set row_end [[[lindex $rows $bot_row end] getBBox] xMax]
          odb::Rect rowbb3;
          rows[outline.first].back()->getBBox(rowbb3);
          row_end = rowbb3.xMax();
          row_end = row_end - corner_cell_width;
        }
        
        // do bottom row
        insert_at_top_bottom_helper(block, 0, 1, bot_row_ori, row_start, row_end, bot_row_y, tap_nwintie_master, tap_nwin2_master, tap_nwin3_master, tap_nwouttie_master, tap_nwout2_master, tap_nwout3_master, prefix);
        
        // do corners
        if (bot_row_ori == odb::dbOrientType::MX ) {
            incnr_master = incnrcap_nwin_master;
            west_ori = odb::dbOrientType::R180;
        } else {
            incnr_master = incnrcap_nwout_master;
            west_ori = odb::dbOrientType::MY;
        }
        
        // SE corner
        if (check_symmetry(incnr_master, bot_row_ori)) {
          build_cell(block, incnr_master, bot_row_ori, x_end, bot_row_y, prefix);
        }
        // SW corner
        if (check_symmetry(incnr_master, west_ori)) {
          build_cell(block, incnr_master, west_ori, x_start - incnr_master->getWidth(), bot_row_y, prefix);
        }
      }
    }
  }
  int blkgs_cnt = phy_idx - start_phy_idx;
  logger_->info(utl::TAP, 7, "Cells inserted near blkgs: {}", blkgs_cnt);
  return blkgs_cnt;
}

std::pair<int, int> Tapcell::get_min_max_x(std::vector<std::vector<odb::dbRow*>> rows) {
  int min_x = -1;
  int max_x = -1;
  for ( std::vector<odb::dbRow*> subrow : rows) {
    for(odb::dbRow* row : subrow) {
      odb::Rect row_bb;
      row->getBBox(row_bb);
      int new_min_x = row_bb.xMin();
      int new_max_x = row_bb.xMax();
      if (min_x == -1) {
        int min_x = new_min_x;
      } else if (min_x > new_min_x) {
        int min_x = new_min_x;
      }
      if (max_x == -1) {
        int max_x = new_max_x;
      } else if (max_x < new_max_x) {
        int max_x = new_max_x;
      }
    }
  }
  std::pair<int, int> min_max_x(min_x, max_x);
  return min_max_x;
}

std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> Tapcell::get_macro_outlines(std::vector<std::vector<odb::dbRow*>> rows) {
  std::map<std::pair<int, int>, int> macro_outlines;
  std::pair<int, int> min_max_x;
  std::vector<int> all_rows;
  std::vector<std::pair<int, int>> new_rows;
  std::pair<int, int> new_rows_pair;
  min_max_x = get_min_max_x(rows); 

  for(int cur_row = 0; cur_row < rows.size(); cur_row++) {
    std::vector<odb::dbRow*> subrows = rows[cur_row];
    int subrow_count = subrows.size();

    int macro0 = -1;
    int macro1 = -1;

    for (int cur_subrow = 0; cur_subrow < subrow_count; cur_subrow++) {
      odb::Rect currow_bb;
      subrows[cur_subrow]->getBBox(currow_bb);

      int macro1 = currow_bb.xMin();
      if (cur_subrow == 0 && macro1 == min_max_x.first) {
        // still on the first position
        int macro1 = -1;
      }

      if (macro0 != macro1) {
        std::pair<int, int> macro0_macro1 (macro0, macro1);
        macro_outlines.insert(std::pair<std::pair<int, int>, int>(macro0_macro1, cur_row));
        //dict lappend macro_outlines "$macro0 $macro1" $cur_row
      }

      int macro0 = currow_bb.xMax();
    }
    if (macro0 != min_max_x.second) {
      // Row is cut at the end
      //dict lappend macro_outlines "$macro0 -1" $cur_row
      std::pair<int, int> macro0_macro1 (macro0, -1);
      macro_outlines.insert(std::pair<std::pair<int, int>, int>(macro0_macro1, cur_row));
    }
  }

  std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> macro_outlines_array;
  for(std::map<std::pair<int, int>, int>::iterator iter = macro_outlines.begin(); iter != macro_outlines.end(); ++iter)
  {
    std::pair<int, int> k =  iter->first;
  //for key [dict keys $macro_outlines] {
    all_rows.push_back(macro_outlines[k]);
    new_rows_pair;
    new_rows_pair.first=all_rows[0];
    new_rows_pair.second=all_rows[0];
    //new_rows.push_back(all_rows[0]);
    new_rows.push_back(new_rows_pair);
    for (int i=1 ; i < all_rows.size()-1; i++) {
      if (all_rows[i]+1 == all_rows[i+1]) {
        continue;
      }
      new_rows_pair;
      new_rows_pair.first=all_rows[i];
      new_rows_pair.second=all_rows[i+1];
      new_rows.push_back(new_rows_pair);
      // new_rows.push_back(all_rows[i]);
      // new_rows.push_back(all_rows[i+1]);
    }
    new_rows_pair;
    new_rows_pair.first=all_rows.back();
    new_rows_pair.second=all_rows.back();
    new_rows.push_back(new_rows_pair);
    //new_rows.push_back(all_rows.back());

    macro_outlines_array.insert(std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>(k, new_rows));
  }

  return macro_outlines_array;
}

// // proc to detect if blockage overlaps with row
bool Tapcell::overlaps(odb::dbInst* blockage, odb::dbRow* row, int halo_x, int halo_y) {
  odb::dbBox* blockageBB = blockage->getBBox();
  odb::Rect rowBB;
  row->getBBox(rowBB);

  // check if Y has overlap first since rows are long and skinny 
  int blockage_lly = blockageBB->yMin() - halo_y;
  int blockage_ury = blockageBB->yMax() + halo_y;
  int row_lly = rowBB.yMin();
  int row_ury = rowBB.yMax();
  
  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return 0;
  }

  int blockage_llx = blockageBB->xMin() - halo_x;
  int blockage_urx = blockageBB->xMax() + halo_x;
  int row_llx = rowBB.xMin();
  int row_urx = rowBB.xMax();

  if(blockage_llx >= row_urx || row_llx >= blockage_urx) {
    return 0;
  }
  
  return 1;
}

std::vector<odb::dbInst*> Tapcell::find_blockages () {
  odb::dbBlock* block = db_->getChip()->getBlock();
  std::vector<odb::dbInst*> blockages;

  for( odb::dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    if (inst->isBlock()) {
      if (!inst->isPlaced()) {
        logger_->warn(utl::TAP, 32, "Macro {} is not placed", inst->getName());
        continue;
      }
      blockages.push_back(inst);
    }
  }

  return blockages;
}

int Tapcell::make_site_loc( int x, double site_x, int dirc=1, int offset=0) {
  double site_int = (x - offset) / site_x;
  if (dirc == 1) {
    site_int = ceil(site_int);
  } else {
    site_int = floor(site_int);
  }
  return site_int * site_x + offset;
}

void Tapcell::build_cell( odb::dbBlock* block, odb::dbMaster* master, odb::dbOrientType orientation, int x, int y, std::string prefix) {

  if (x < 0 || y < 0) {
    return;
  }
  std::string name = prefix + std::to_string(phy_idx);
  odb::dbInst* inst = odb::dbInst::create(block, master, name.c_str());
  if (inst == nullptr) {
    return;
  }
  odb::dbSourceType DIST;
  inst->setOrient(orientation);
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  inst->setSourceType(DIST);

  odb::dbBox* inst_bb = inst->getBBox();
  std::vector<int> filled_sites_vector;
  filled_sites_vector.push_back(inst_bb->yMin());
  filled_sites_vector.push_back(inst_bb->xMin());
  filled_sites_vector.push_back(inst_bb->xMax());
  filled_sites.push_back(filled_sites_vector);
  //map filled_sites "[$inst_bb yMin] [$inst_bb xMin] [$inst_bb xMax]"

  phy_idx++;
}

void Tapcell::build_row(odb::dbBlock* block, std::string name, odb::dbSite* site, int start_x, int end_x, int y, odb::dbOrientType orient, odb::dbRowDir direction, int min_row_width) {
  int site_width = site->getWidth();

  double new_row_num_sites = (end_x - start_x)/site_width;
  double new_row_width = new_row_num_sites*site_width;

  if (new_row_num_sites > 0 && new_row_width >= min_row_width) {
    odb::dbRow::create(block, name.c_str(), site, start_x, y, orient, direction, new_row_num_sites, site_width);
  }
}

std::vector<std::vector<odb::dbRow*>> Tapcell::organize_rows() {
  std::map<int, std::vector<odb::dbRow*>> rows_dict;
  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    odb::Rect rowBB;
    row->getBBox(rowBB);
    rows_dict[rowBB.yMin()].push_back(row);
  }

  // organize rows bottom to top
  std::vector<std::vector<odb::dbRow*>> rows;
  for(std::map<int, std::vector<odb::dbRow*>>::iterator iter = rows_dict.begin(); iter != rows_dict.end(); ++iter)
  {
    int k =  iter->first;
  //foreach key [lsort -integer [dict keys $rows_dict]] {
    std::map<int, odb::dbRow*> in_row_dict;
    for(odb::dbRow* in_row : rows_dict[k]) {
      odb::Rect row_BB;
      in_row->getBBox(row_BB);
      in_row_dict.insert(std::pair<int, odb::dbRow*>(row_BB.xMin(), in_row));
    }
    // organize sub rows left to right
    std::vector<odb::dbRow*> in_row;
    for(std::map<int, odb::dbRow*>::iterator iter = in_row_dict.begin(); iter != in_row_dict.end(); ++iter)
    { 
      int in_k =  iter->first;
    //foreach in_key [lsort -integer [dict keys $in_row_dict]] {
      in_row.push_back(in_row_dict[in_k]);
    }
    rows.push_back(in_row);
    in_row.clear();
  }
  rows_dict.clear();

  return rows;
}

int Tapcell::remove_cells(const char* prefix) {
  odb::dbBlock* block = db_->getChip()->getBlock();
  std::string prefix_string(prefix);
  //odb::dbBlock* block = getOpenRoad()->getDb()->getChip()->getBlock();
  int removed = 0;

  if (prefix_string.size() == 0) {
  //if (prefix.length() == 0) {
    // no prefix is given, this will result in all cells being removed
    return 0;
  }

  for(odb::dbInst* inst : block->getInsts()) {
    if (prefix_string.compare(inst->getName()) == 0) {
      odb::dbInst::destroy(inst);
      removed++;
    }
  }

  return removed;
}

bool Tapcell::check_symmetry( odb::dbMaster* master, odb::dbOrientType ori) {
  bool symmetry_x = master->getSymmetryX();
  bool symmetry_y = master->getSymmetryY();

  switch(ori) {
    case odb::dbOrientType::R0:
      return 1;
    case odb::dbOrientType::MX:
      return symmetry_x;
    case odb::dbOrientType::MY:
      return symmetry_y;
    case odb::dbOrientType::R180:
      return (symmetry_x && symmetry_y);
    default:
      return 0;
  }
}

// namespace end
}
