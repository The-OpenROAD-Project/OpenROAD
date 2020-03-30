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

#pragma once

#include <map>
#include <set>
#include <vector>
#include <functional>

#include "opendb/db.h"

// Remove leading underscore to enable debug printing.
#define _ODP_DEBUG

namespace opendp {

using std::string;
using std::vector;
using std::set;
using std::map;

using odb::Rect;
using odb::Point;
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
using odb::dbMasterType;

class Pixel;
struct Group;

typedef Pixel* Grid;
typedef vector<string> StringSeq;
typedef vector<dbMaster*> dbMasterSeq;
// gap -> sequence of masters to fill the gap
typedef vector<dbMasterSeq> GapFillers;

enum Power { undefined, VDD, VSS };

struct Macro {
  bool is_multi_row_;
  Power top_power_;    // VDD/VSS
};

struct Cell {
  Cell();
  const char *name();
  bool inGroup() { return group_ != nullptr; }
  int64_t area();

  dbInst* db_inst_;
  int x_, y_;		       // lower left wrt core DBU
  dbOrientType orient_;
  int width_, height_;		// DBU
  bool is_placed_;
  bool hold_;
  Group* group_;
  Rect *region_;  // group rect
};

struct Group {
  Group();

  string name;
  vector< Rect > regions;
  vector< Cell* > cells_;
  Rect boundary;
  double util;
};

struct Pixel {
  int grid_x_;
  int grid_y_;
  Group* group_;
  Cell* cell;
  double util;
  bool is_valid;  // false for dummy cells
};

////////////////////////////////////////////////////////////////

typedef set<dbMaster*> dbMasterSet;

class Opendp {
 public:
  Opendp();
  ~Opendp();
  void clear();
  void init(dbDatabase* db);
  // legalize/report
  // max_displacment is in rows, 0 for unconstrained
  void detailedPlacement(int max_displacment);
  void setPaddingGlobal(int left,
			int right);
  // Return true if illegal.
  bool checkPlacement(bool verbose);
  void fillerPlacement(StringSeq *filler_master_names);
  void reportLegalizationStats();
  void reportDesignStats();
  int64_t hpwl(bool initial);
  void displacementStats(// Return values.
			 int64_t &avg_displacement,
			 int64_t &sum_displacement,
			 int64_t &max_displacement);
  void setPowerNetName(const char *power_name);
  void setGroundNetName(const char *ground_name);
  void reportGrid();

 private:
  void importDb();
  void importClear();
  void reportImportWarnings();
  void makeMacros();
  void examineRows();
  void makeCells();
  bool isPlacedType(dbMasterType type);
  void makeGroups();
  void findRowPower();
  double dbuToMicrons(int64_t dbu);
  double dbuAreaToMicrons(int64_t dbu_area);
  bool isFixed(Cell* cell);  // fixed cell or not
  bool isMultiRow(Cell* cell);
  Power topPower(Cell* cell);
  void updateDbInstLocations();

  void defineTopPower(Macro &macro,
		      dbMaster *master);
  int find_ymax(dbMTerm* term);

  void initGrid();
  void findDesignStats();

  void detailedPlacement();
  Point nearestPt(Cell* cell, Rect* rect);
  int dist_for_rect(Cell* cell, Rect* rect);
  bool check_overlap(Rect cell, Rect box);
  bool check_overlap(Cell* cell, Rect* rect);
  bool check_inside(Rect cell, Rect box);
  bool check_inside(Cell* cell, Rect* rect);
  bool binSearch(int grid_x, Cell* cell,
		 int x, int y,
		 // Return values
		 int &avail_x,
		 int &avail_y);
  Pixel *diamondSearch(Cell* cell, int x, int y);
  bool shift_move(Cell* cell);
  bool map_move(Cell* cell);
  bool map_move(Cell* cell, int x, int y);
  set< Cell* > gridCellsInBoundary(Rect* rect);
  int distChange(Cell* cell, int x, int y);
  bool swap_cell(Cell* cell1, Cell* cell2);
  bool refine_move(Cell* cell);

  void placeGroups();
  void prePlace();
  void prePlaceGroups();
  void place();
  void placeGroups2();
  void brickPlace1(Group* group);
  void brickPlace2(Group* group);
  int groupRefine(Group* group);
  int anneal(Group* group);
  int anneal();
  int refine();
  bool cellFitsInCore(Cell *cell);

  void fixed_cell_assign();
  void group_cell_region_assign();
  void group_pixel_assign();
  void group_pixel_assign2();
  void erase_pixel(Cell* cell);
  void paint_pixel(Cell* cell, int grid_x, int grid_y);

  // checkPlacement
  bool isPlaced(Cell *cell);
  bool checkPowerLine(Cell &cell);
  bool checkInCore(Cell &cell);
  Cell *checkOverlap(Cell &cell,
		     Grid *grid,
		     bool padded);
  bool overlap(Cell *cell1, Cell *cell2, bool padded);
  void reportFailures(vector<Cell*> failures,
		      const char *msg,
		      bool verbose);
  void reportFailures(vector<Cell*> failures,
		      const char *msg,
		      bool verbose,
		      std::function<void(Cell *cell)> report_failure);
  void reportOverlapFailure(Cell *cell, Grid *grid, bool padded);

  void rectDist(Cell *cell,
		Rect *rect,
		// Return values.
		int x,
		int y);
  int rectDist(Cell *cell,
	       Rect *rect);
  Power rowTopPower(int row);
  dbOrientType rowOrient(int row);
  bool havePadding();

  Grid *makeGrid();
  void deleteGrid(Grid *grid);
  // Cell initial location wrt core origin.
  int gridX(int x);
  int gridY(int y);
  int gridEndX();
  int gridEndY();
  int gridPaddedWidth(Cell* cell);
  int64_t paddedArea(Cell *cell);
  int gridNearestHeight(Cell* cell);
  int gridNearestWidth(Cell* cell);
  int gridHeight(Cell* cell);
  int gridX(Cell* cell);
  int gridPaddedX(Cell* cell);
  int gridY(Cell* cell);
  int gridPaddedEndX(Cell *cell);
  int gridEndX(Cell *cell);
  int gridEndY(Cell *cell);
  void setGridPaddedLoc(Cell *cell,
			int x,
			int y);
  void initialLocation(dbInst* inst,
		       // Return values.
		       int &x,
		       int &y);
  void initialLocation(Cell *cell,
		       // Return values.
		       int &x,
		       int &y);
  void initialPaddedLocation(Cell *cell,
			     // Return values.
			     int &x,
			     int &y);
  bool isStdCell(Cell *cell);
  bool isBlock(Cell *cell);
  int paddedWidth(Cell *cell);
  bool isPaddedType(Cell *cell);
  bool isPadded(Cell *cell);
  int disp(Cell *cell);
  int coreGridMaxX();
  int coreGridMaxY();
  // Place fillers
  void findFillerMasters(StringSeq *filler_master_names);
  dbMasterSeq &gapFillers(int gap);
  Grid *makeCellGrid();  
  void placeRowFillers(Grid *grid,
		       int row);
  void reportGrid(Grid *grid);

  dbDatabase* db_;
  dbBlock* block_;
  int pad_left_;
  int pad_right_;
  const char *power_net_name_;
  const char *ground_net_name_;

  vector< Cell > cells_;
  vector< Group > groups_;

  map< dbMaster*, Macro > db_master_map_;
  map< dbInst*, Cell* > db_inst_map_;

  Rect core_;
  Power initial_power_;
  bool row0_orient_is_r0_;
  bool row0_top_power_is_vdd_;
  Power macro_top_power_;
  int row_height_; // dbu
  int site_width_;
  int row_count_;
  int row_site_count_;
  int max_cell_height_;
  int max_displacement_constraint_;  // rows

  // 2D pixel grid
  Grid *grid_;
  Cell dummy_cell_;

  // Design stats.
  int fixed_inst_count_;
  int multi_row_inst_count_;
  // total placeable area (excluding row blockages) dbu^2
  int64_t design_area_;
  // total movable cell area dbu^2
  int64_t movable_area_;
  int64_t movable_padded_area_;
  // total fixed cell area dbu^2
  int64_t fixed_area_;
  int64_t fixed_padded_area_;
  double design_util_;
  double design_padded_util_;

  dbMasterSeq filler_masters_;
  // gap (in sites) -> seq of masters
  GapFillers gap_fillers_;
  int filler_count_;

  // Magic numbers
  int diamond_search_height_;  // grid units
  int diamond_search_width_;   // grid units
  static constexpr int bin_search_width_ = 10;
  static constexpr double group_refine_percent_ = .05;
  static constexpr double refine_percent_ = .02;
  static constexpr int rand_seed_ = 777;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace opendp
