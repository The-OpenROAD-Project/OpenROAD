/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.

// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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

#ifndef OPENDP_H
#define OPENDP_H

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "opendb/db.h"

#define _ODP_DEBUG

namespace opendp {

using std::string;

using odb::adsRect;
using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbLib;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbOrientType;
using odb::dbRow;
using odb::dbSite;

enum power { undefined, VDD, VSS };

struct Macro {
  bool is_multi_row_;
  power top_power_;    // VDD/VSS
};

struct Group;

struct Cell {
  Cell();
  const char *name();
  bool inGroup() { return group_ != nullptr; }
  int64_t area();

  dbInst* db_inst_;
  int x_, y_;		       // lower left wrt core with padding DBU
  dbOrientType orient_;
  int width_, height_;		// DBU
  bool is_placed_;
  bool hold_;
  Group* group_;
};

// Rows covering core.
// These do NOT correspond to database rows, which may be fragmented.
struct Row {
  Row();

  int origX; // DBU
  int origY; // DBU
  dbOrientType orient_;
  power top_power;
};

struct Group {
  Group();

  std::string name;
  std::vector< adsRect > regions;
  std::vector< Cell* > siblings;
  adsRect boundary;
  double util;
};

struct Pixel {
  int grid_x_;
  int grid_y_;
  Group* group_;
  Cell* cell;
  double util;
  bool is_valid;  // false for dummy cells

  Pixel();
};

////////////////////////////////////////////////////////////////

class Opendp {
 public:
  Opendp();
  ~Opendp();
  void clear();
  void init(dbDatabase* db);
  bool readConstraints(string constraint_file);
  // legalize/check/report
  bool legalizePlacement(bool verbose);
  void setPaddingGlobal(int left,
			int right);
  bool checkLegality(bool verbose);

  void reportLegalizationStats();
  void reportDesignStats();
  double hpwl(bool initial);
  void displacementStats(// Return values.
			 int &avg_displacement,
			 int &sum_displacement,
			 int &max_displacement);

 private:
  void dbToOpendp();
  void make_macros(dbLib* db_lib);
  void make_core_rows();
  void make_cells();
  void makeGroups();
  void findInitialPower();
  double dbuToMicrons(int64_t dbu);
  bool isFixed(Cell* cell);  // fixed cell or not
  bool isMultiRow(Cell* cell);
  power topPower(Cell* cell);
  void updateDbInstLocations();

  void defineTopPower(Macro &macro,
		      dbMaster *master);
  int find_ymax(dbMTerm* term);

  // read files for legalizer - parser.cpp
  void initAfterImport();
  void findDesignStats();
  void copy_init_to_final();

  // utility.cpp
  void power_mapping();
  void simplePlacement(bool verbose);
  std::pair< int, int > nearest_coord_to_rect_boundary(Cell* cell,
                                                       adsRect* rect);
  int dist_for_rect(Cell* cell, adsRect* rect);
  bool check_overlap(adsRect cell, adsRect box);
  bool check_overlap(Cell* cell, adsRect* rect);
  bool check_inside(adsRect cell, adsRect box);
  bool check_inside(Cell* cell, adsRect* rect);
  bool binSearch(int grid_x, Cell* cell,
		 int x, int y,
		 // Return values
		 int &avail_x,
		 int &avail_y);
  bool diamondSearch(Cell* cell, int x, int y,
		     // Return value
		     Pixel *&pixel);
  bool shift_move(Cell* cell);
  bool map_move(Cell* cell);
  bool map_move(Cell* cell, int x, int y);
  std::vector< Cell* > overlap_cells(Cell* cell);
  std::vector< Cell* > get_cells_from_boundary(adsRect* rect);
  int dist_benefit(Cell* cell, int x, int y);
  bool swap_cell(Cell* cell1, Cell* cell2);
  bool refine_move(Cell* cell);

  void non_group_cell_pre_placement();
  void group_cell_pre_placement();
  void non_group_cell_placement();
  void group_cell_placement();
  void brick_placement1(Group* group);
  void brick_placement2(Group* group);
  int group_refine(Group* group);
  int group_annealing(Group* group);
  int non_group_annealing();
  int non_group_refine();

  // assign.cpp
  void fixed_cell_assign();
  void group_pixel_assign();
  void group_pixel_assign2();
  void erase_pixel(Cell* cell);
  void paint_pixel(Cell* cell, int grid_x, int grid_y);

  bool row_check(bool verbose);
  bool site_check(bool verbose);
  bool edge_check(bool verbose);
  bool power_line_check(bool verbose);
  bool placed_check(bool verbose);
  bool overlap_check(bool verbose);
  void rectDist(Cell *cell,
		adsRect *rect,
		// Return values.
		int x_tar,
		int y_tar);
  int rectDist(Cell *cell,
	       adsRect *rect);

  // Cell initial location wrt core origin.
  int gridX(int x);
  int gridY(int y);
  int gridWidth();
  int gridHeight();
  int gridEndX();
  int gridEndY();
  int gridWidth(Cell* cell);
  int gridNearestHeight(Cell* cell);
  int gridNearestWidth(Cell* cell);
  int gridHeight(Cell* cell);
  int gridX(Cell* cell);
  int gridY(Cell* cell);
  int gridEndX(Cell *cell);
  int gridEndY(Cell *cell);
  void initLocation(Cell *cell,
		    int &x,
		    int &y);
  int paddedWidth(Cell *cell);
  adsRect region(Cell *cell);
  int disp(Cell *cell);
  int coreGridWidth();
  int coreGridHeight();
  int coreGridMaxX();
  int coreGridMaxY();
  void error(const char *what);

  dbDatabase* db_;
  dbBlock* block_;
  int pad_left_;
  int pad_right_;

  std::vector< Cell > cells_;
  std::vector< Row > rows_;
  std::vector< Group > groups_;

  std::map< dbMaster*, Macro > db_master_map_;
  std::map< dbInst*, Cell* > db_inst_map_;

  adsRect core_;
  power initial_power_;
  int row_height_; // dbu
  int site_width_;
  int row_site_count_;
  int max_cell_height_;
  int diamond_search_height_;  // grid units
  int max_displacement_constraint_; // from constraints file

  // 2D pixel grid
  Pixel** grid_;
  Cell dummy_cell_;

  // Design stats.
  int fixed_inst_count_;
  int multi_height_inst_count_;
  // total placeable area (excluding row blockages) dbu^2
  int64_t design_area_;
  // total movable cell area dbu^2
  int64_t movable_area_;
  // total fixed cell area (excluding terminal NIs) dbu^2
  int64_t fixed_area_;
  double design_util_;

  // Magic numbers
  static constexpr double group_refine_percent_ = .05;
  static constexpr double non_group_refine_percent_ = .02;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace opendp

#endif
