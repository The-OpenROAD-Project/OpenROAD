/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>  // pair
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace dpl {

using std::map;
using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

using utl::Logger;

using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbLib;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbOrientType;
using odb::dbRow;
using odb::dbSite;
using odb::Point;
using odb::Rect;

struct Pixel;
struct Group;
class Graphics;

using Grid = Pixel**;
using dbMasterSeq = vector<dbMaster*>;
// gap -> sequence of masters to fill the gap
using GapFillers = vector<dbMasterSeq>;

typedef map<dbInst*, pair<int, int>> InstPaddingMap;
typedef map<dbMaster*, pair<int, int>> MasterPaddingMap;

struct Master
{
  bool is_multi_row = false;
};

struct Cell
{
  Cell();
  const char* name() const;
  bool inGroup() const { return group_ != nullptr; }
  int64_t area() const;

  dbInst* db_inst_;
  int x_, y_;  // lower left wrt core DBU
  dbOrientType orient_;
  int width_, height_;  // DBU
  bool is_placed_;
  bool hold_;
  Group* group_;
  Rect* region_;  // group rect
};

struct Group
{
  Group();

  string name;
  vector<Rect> regions;
  vector<Cell*> cells_;
  Rect boundary;
  double util;
};

struct Pixel
{
  Cell* cell;
  Group* group_;
  double util;
  dbOrientType orient_;
  bool is_valid;     // false for dummy cells
  bool is_hopeless;  // too far from sites for diamond search
};

// For optimize mirroring.
class NetBox
{
 public:
  NetBox();
  NetBox(dbNet* net, Rect box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();

  dbNet* net_;
  Rect box_;
  Rect box_saved_;
  bool ignore_;
};

typedef unordered_map<dbNet*, NetBox> NetBoxMap;
typedef vector<NetBox*> NetBoxes;

////////////////////////////////////////////////////////////////

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt();
  PixelPt(Pixel* pixel, int grid_x, int grid_y);
  Pixel* pixel;
  Point pt;  // grid locataion
};

class Opendp
{
 public:
  Opendp();
  ~Opendp();

  Opendp(const Opendp&) = delete;
  Opendp& operator=(const Opendp&) = delete;
  Opendp(const Opendp&&) = delete;
  Opendp& operator=(const Opendp&&) = delete;

  void init(dbDatabase* db, Logger* logger);
  void initBlock();
  // legalize/report
  // max_displacment is in sites. use zero for defaults.
  void detailedPlacement(int max_displacement_x, int max_displacement_y);
  void reportLegalizationStats() const;
  void setPaddingGlobal(int left, int right);
  void setPadding(dbMaster* inst, int left, int right);
  void setPadding(dbInst* inst, int left, int right);
  void setDebug(bool displacement,
                float min_displacement,
                const dbInst* debug_instance);

  // Global padding.
  int padGlobalLeft() const { return pad_left_; }
  int padGlobalRight() const { return pad_right_; }
  // Find instance/master/global padding value for an instance.
  int padRight(dbInst* inst) const;
  int padLeft(dbInst* inst) const;
  // Return error count.
  void checkPlacement(bool verbose);
  void fillerPlacement(dbMasterSeq* filler_masters, const char* prefix);
  void removeFillers();
  int64_t hpwl() const;
  int64_t hpwl(dbNet* net) const;
  void findDisplacementStats();
  void optimizeMirroring();

  const vector<Cell>& getCells() const { return cells_; }
  Rect getCore() const { return core_; }
  int getRowHeight() const { return row_height_; }
  int getSiteWidth() const { return site_width_; }
  int getRowCount() const { return row_count_; }
  int getRowSiteCount() const { return row_site_count_; }

 private:
  void importDb();
  void importClear();
  Rect getBbox(dbInst* inst);
  void makeMacros();
  void examineRows();
  void makeCells();
  static bool isPlacedType(dbMasterType type);
  void makeGroups();
  double dbuToMicrons(int64_t dbu) const;
  double dbuAreaToMicrons(int64_t dbu_area) const;
  bool isFixed(const Cell* cell) const;  // fixed cell or not
  bool isMultiRow(const Cell* cell) const;
  void updateDbInstLocations();

  void makeMaster(Master* master, dbMaster* db_master);

  void initGrid();
  void detailedPlacement();
  Point nearestPt(const Cell* cell, const Rect* rect) const;
  int distToRect(const Cell* cell, const Rect* rect) const;
  static bool checkOverlap(const Rect& cell, const Rect& box);
  bool checkOverlap(const Cell* cell, const Rect* rect) const;
  static bool isInside(const Rect& cell, const Rect& box);
  bool isInside(const Cell* cell, const Rect* rect) const;
  PixelPt diamondSearch(const Cell* cell,
                        // grid indices
                        int x,
                        int y) const;
  void diamondSearchSide(const Cell* cell,
                         int x,
                         int y,
                         int x_min,
                         int y_min,
                         int x_max,
                         int y_max,
                         int x_offset,
                         int y_offset,
                         // Return values
                         PixelPt& best_pt,
                         int& best_dist) const;
  PixelPt binSearch(int x, const Cell* cell, int bin_x, int bin_y) const;
  bool checkPixels(const Cell* cell, int x, int y, int x_end, int y_end) const;
  void shiftMove(Cell* cell);
  bool mapMove(Cell* cell);
  bool mapMove(Cell* cell, Point grid_pt);
  int distChange(const Cell* cell, int x, int y) const;
  bool swapCells(Cell* cell1, Cell* cell2);
  bool refineMove(Cell* cell);

  Point legalPt(const Cell* cell, Point pt) const;
  Point legalGridPt(const Cell* cell, Point pt) const;
  Point legalPt(const Cell* cell, bool padded) const;
  Point legalGridPt(const Cell* cell, bool padded) const;
  Point nearestBlockEdge(const Cell* cell,
                         const Point& legal_pt,
                         const Rect& block_bbox) const;
  bool moveHopeless(const Cell* cell, int& grid_x, int& grid_y) const;
  void placeGroups();
  void prePlace();
  void prePlaceGroups();
  void place();
  void placeGroups2();
  void brickPlace1(const Group* group);
  void brickPlace2(const Group* group);
  int groupRefine(const Group* group);
  int anneal(Group* group);
  int anneal();
  int refine();
  bool cellFitsInCore(Cell* cell);
  void setFixedGridCells();
  void visitCellPixels(Cell& cell,
                       bool padded,
                       const std::function<void(Pixel* pixel)>& visitor) const;
  void setGridCell(Cell& cell, Pixel* pixel);
  void groupAssignCellRegions();
  void groupInitPixels();
  void groupInitPixels2();
  void erasePixel(Cell* cell);
  void paintPixel(Cell* cell, int grid_x, int grid_y);

  // checkPlacement
  static bool isPlaced(const Cell* cell);
  bool checkInRows(const Cell& cell) const;
  Cell* checkOverlap(Cell& cell) const;
  bool overlap(const Cell* cell1, const Cell* cell2) const;
  bool isOverlapPadded(const Cell* cell1, const Cell* cell2) const;
  bool isCrWtBlClass(const Cell* cell) const;
  bool isWtClass(const Cell* cell) const;
  void reportFailures(const vector<Cell*>& failures,
                      int msg_id,
                      const char* msg,
                      bool verbose) const;
  void reportFailures(
      const vector<Cell*>& failures,
      int msg_id,
      const char* msg,
      bool verbose,
      const std::function<void(Cell* cell)>& report_failure) const;
  void reportOverlapFailure(Cell* cell) const;

  void rectDist(const Cell* cell,
                const Rect* rect,
                // Return values.
                int* x,
                int* y) const;
  int rectDist(const Cell* cell, const Rect* rect) const;
  bool havePadding() const;

  void deleteGrid();
  Pixel* gridPixel(int x, int y) const;
  // Cell initial location wrt core origin.
  int gridX(int x) const;
  int gridY(int y) const;
  int gridEndX(int x) const;
  int gridEndY(int y) const;
  int gridEndX() const;
  int gridEndY() const;
  int gridPaddedWidth(const Cell* cell) const;
  int64_t paddedArea(const Cell* cell) const;
  int gridNearestHeight(const Cell* cell) const;
  int gridNearestWidth(const Cell* cell) const;
  int gridHeight(const Cell* cell) const;
  int gridX(const Cell* cell) const;
  int gridPaddedX(const Cell* cell) const;
  int gridY(const Cell* cell) const;
  int gridPaddedEndX(const Cell* cell) const;
  int gridEndX(const Cell* cell) const;
  int gridEndY(const Cell* cell) const;
  void setGridPaddedLoc(Cell* cell, int x, int y) const;
  // Lower left corner in core coordinates.
  Point initialLocation(const Cell* cell, bool padded) const;
  bool isStdCell(const Cell* cell) const;
  static bool isBlock(const Cell* cell);
  int paddedWidth(const Cell* cell) const;
  bool isPaddedType(dbInst* inst) const;
  int padLeft(const Cell* cell) const;
  int padRight(const Cell* cell) const;
  int disp(const Cell* cell) const;
  // Place fillers
  void setGridCells();
  dbMasterSeq& gapFillers(int gap, dbMasterSeq* filler_masters);
  void placeRowFillers(int row,
                       const char* prefix,
                       dbMasterSeq* filler_masters);
  bool isFiller(odb::dbInst* db_inst);

  const char* gridInstName(int row, int col);

  // Optimizing mirroring
  void findNetBoxes();
  vector<dbInst*> findMirrorCandidates(NetBoxes& net_boxes);
  int mirrorCandidates(vector<dbInst*>& mirror_candidates);
  // Sum of ITerm hpwl's.
  int64_t hpwl(dbInst* inst);
  void updateNetBoxes(dbInst* inst);
  void saveNetBoxes(dbInst* inst);
  void restoreNetBoxes(dbInst* inst);

  Logger* logger_;
  dbDatabase* db_;
  dbBlock* block_;
  int pad_left_;
  int pad_right_;
  InstPaddingMap inst_padding_map_;
  MasterPaddingMap master_padding_map_;

  vector<Cell> cells_;
  vector<Group> groups_;

  map<const dbMaster*, Master> db_master_map_;
  map<dbInst*, Cell*> db_inst_map_;

  Rect core_;
  int row_height_;  // dbu
  int site_width_;  // dbu
  int row_count_;
  int row_site_count_;
  int have_multi_row_cells_;
  int max_displacement_x_;  // sites
  int max_displacement_y_;  // sites
  vector<dbInst*> placement_failures_;

  // 2D pixel grid
  Grid grid_;
  Cell dummy_cell_;

  // Filler placement.
  // gap (in sites) -> seq of masters
  GapFillers gap_fillers_;
  int filler_count_;
  bool have_fillers_;

  // Results saved for optional reporting.
  int64_t hpwl_before_;
  int64_t displacement_avg_;
  int64_t displacement_sum_;
  int64_t displacement_max_;

  // Optimiize mirroring.
  NetBoxMap net_box_map_;

  std::unique_ptr<Graphics> graphics_;

  // Magic numbers
  static constexpr int bin_search_width_ = 10;
  static constexpr double group_refine_percent_ = .05;
  static constexpr double refine_percent_ = .02;
  static constexpr int rand_seed_ = 777;
  // Net bounding box siaz on nets with more instance terminals
  // than this are ignored.
  static constexpr int mirror_max_iterm_count_ = 100;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace dpl
