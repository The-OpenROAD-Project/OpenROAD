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

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <functional>
#include <map>
#include <memory>
#include <numeric>  // accumulate
#include <set>
#include <string>
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
using odb::dbTechLayer;
using odb::Point;
using odb::Rect;

struct Cell;
struct Group;
struct Pixel;

class DplObserver;
class Grid;
class GridInfo;
class Padding;

using bgPoint
    = boost::geometry::model::d2::point_xy<int, boost::geometry::cs::cartesian>;
using bgBox = boost::geometry::model::box<bgPoint>;

using RtreeBox
    = boost::geometry::index::rtree<bgBox,
                                    boost::geometry::index::quadratic<16>>;

using dbMasterSeq = vector<dbMaster*>;
// gap -> sequence of masters to fill the gap
using GapFillers = vector<dbMasterSeq>;

struct Master
{
  bool is_multi_row = false;
};

class HybridSiteInfo
{
 public:
  HybridSiteInfo(int index, dbSite* site) : index_(index), site_(site) {}
  int getIndex() const { return index_; }
  const dbSite* getSite() const { return site_; }

 private:
  const int index_;
  const dbSite* site_;
};

struct Cell
{
  const char* name() const;
  bool inGroup() const { return group_ != nullptr; }
  int64_t area() const;
  bool isStdCell() const;
  int siteWidth() const;
  bool isFixed() const;

  dbInst* db_inst_ = nullptr;
  int x_ = 0;  // lower left wrt core DBU
  int y_ = 0;
  dbOrientType orient_;
  int width_ = 0;  // DBU
  int height_ = 0;
  bool is_placed_ = false;
  bool hold_ = false;
  Group* group_ = nullptr;
  Rect* region_ = nullptr;  // group rect

  bool isHybrid() const
  {
    dbSite* site = getSite();
    return site ? site->isHybrid() : false;
  }

  bool isHybridParent() const
  {
    dbSite* site = getSite();
    return site ? site->hasRowPattern() : false;
  }

  dbSite* getSite() const
  {
    if (!db_inst_ || !db_inst_->getMaster()) {
      return nullptr;
    }
    return db_inst_->getMaster()->getSite();
  }
};

struct Group
{
  string name;
  vector<Rect> regions;
  vector<Cell*> cells_;
  Rect boundary;
  double util = 0.0;
};

struct Pixel
{
  Cell* cell;
  Group* group_;
  double util;
  dbOrientType orient_;
  bool is_valid;     // false for dummy cells
  bool is_hopeless;  // too far from sites for diamond search
  dbSite* site;      // site that this pixel is
};

////////////////////////////////////////////////////////////////

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt() = default;
  PixelPt(Pixel* pixel, int grid_x, int grid_y);
  Pixel* pixel = nullptr;
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

  void legalCellPos(dbInst* db_inst);
  void initMacrosAndGrid();

  void init(dbDatabase* db, Logger* logger);
  void initBlock();
  // legalize/report
  // max_displacment is in sites. use zero for defaults.
  void detailedPlacement(int max_displacement_x,
                         int max_displacement_y,
                         const std::string& report_file_name = std::string(""),
                         bool disallow_one_site_gaps = false);
  void reportLegalizationStats() const;

  void setPaddingGlobal(int left, int right);
  void setPadding(dbMaster* master, int left, int right);
  void setPadding(dbInst* inst, int left, int right);
  void setDebug(std::unique_ptr<dpl::DplObserver>& observer);

  // Global padding.
  int padGlobalLeft() const;
  int padGlobalRight() const;
  // Find instance/master/global padding value for an instance.
  int padLeft(dbInst* inst) const;
  int padRight(dbInst* inst) const;

  void checkPlacement(bool verbose,
                      bool disallow_one_site_gaps = false,
                      const string& report_file_name = "");
  void writeJsonReport(const string& filename,
                       const vector<Cell*>& placed_failures,
                       const vector<Cell*>& in_rows_failures,
                       const vector<Cell*>& overlap_failures,
                       const vector<Cell*>& one_site_gap_failures,
                       const vector<Cell*>& site_align_failures,
                       const vector<Cell*>& region_placement_failures,
                       const vector<Cell*>& placement_failures);
  void fillerPlacement(dbMasterSeq* filler_masters, const char* prefix);
  void removeFillers();
  void optimizeMirroring();

 private:
  using MasterByImplant = std::map<dbTechLayer*, dbMasterSeq>;
  friend class OpendpTest_IsPlaced_Test;
  friend class Graphics;
  void findDisplacementStats();
  Point pointOffMacro(const Cell& cell);
  void convertDbToCell(dbInst* db_inst, Cell& cell);
  const vector<Cell>& getCells() const { return cells_; }
  Rect getCore() const;
  int getRowHeight() const;
  int getSiteWidth() const;
  // Return error count.
  void processViolationsPtree(boost::property_tree::ptree& entry,
                              const std::vector<Cell*>& failures,
                              const std::string& violation_type = "") const;
  void importDb();
  void importClear();
  Rect getBbox(dbInst* inst);
  void makeMacros();
  void makeCells();
  static bool isPlacedType(dbMasterType type);
  void makeGroups();
  double dbuToMicrons(int64_t dbu) const;
  double dbuAreaToMicrons(int64_t dbu_area) const;
  bool isMultiRow(const Cell* cell) const;
  void updateDbInstLocations();

  void makeMaster(Master* master, dbMaster* db_master);

  void initGrid();
  std::string printBgBox(const boost::geometry::model::box<bgPoint>& queryBox);
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
  bool checkRegionOverlap(const Cell* cell,
                          int x,
                          int y,
                          int x_end,
                          int y_end) const;
  bool checkPixels(const Cell* cell, int x, int y, int x_end, int y_end) const;
  void shiftMove(Cell* cell);
  bool mapMove(Cell* cell);
  bool mapMove(Cell* cell, const Point& grid_pt);
  int distChange(const Cell* cell, int x, int y) const;
  bool swapCells(Cell* cell1, Cell* cell2);
  bool refineMove(Cell* cell);

  Point legalPt(const Cell* cell, const Point& pt, int row_height = -1) const;
  Point legalGridPt(const Cell* cell,
                    const Point& pt,
                    int row_height = -1) const;
  Point legalPt(const Cell* cell, bool padded, int row_height = -1) const;
  Point legalGridPt(const Cell* cell, bool padded, int row_height = -1) const;
  Point nearestBlockEdge(const Cell* cell,
                         const Point& legal_pt,
                         const Rect& block_bbox) const;

  void findOverlapInRtree(bgBox& queryBox, vector<bgBox>& overlaps) const;
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
  void setFixedGridCells();
  void setGridCell(Cell& cell, Pixel* pixel);
  void groupAssignCellRegions();
  void groupInitPixels();
  void groupInitPixels2();

  // checkPlacement
  static bool isPlaced(const Cell* cell);
  bool checkInRows(const Cell& cell) const;
  Cell* checkOverlap(Cell& cell) const;
  Cell* checkOneSiteGaps(Cell& cell) const;
  bool overlap(const Cell* cell1, const Cell* cell2) const;
  bool checkRegionPlacement(const Cell* cell) const;
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
  void checkOneSiteDbMaster();
  void deleteGrid();
  // Cell initial location wrt core origin.

  int getRowCount(const Cell* cell) const;
  int64_t paddedArea(const Cell* cell) const;
  int gridNearestHeight(const Cell* cell) const;
  int gridNearestHeight(const Cell* cell, int row_height) const;
  int gridNearestWidth(const Cell* cell) const;
  std::pair<int, GridInfo> getRowInfo(const Cell* cell) const;
  // Lower left corner in core coordinates.
  Point initialLocation(const Cell* cell, bool padded) const;
  static bool isBlock(const Cell* cell);
  int disp(const Cell* cell) const;
  // Place fillers
  MasterByImplant splitByImplant(dbMasterSeq* filler_masters);
  void setGridCells();
  dbMasterSeq& gapFillers(dbTechLayer* implant,
                          int gap,
                          const MasterByImplant& filler_masters_by_implant);
  void placeRowFillers(int row,
                       const char* prefix,
                       const MasterByImplant& filler_masters,
                       int row_height,
                       GridInfo grid_info);
  bool isFiller(odb::dbInst* db_inst);
  bool isOneSiteCell(odb::dbMaster* db_master) const;
  const char* gridInstName(int row,
                           int col,
                           int row_height,
                           GridInfo grid_info);

  Logger* logger_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;
  std::unique_ptr<Padding> padding_;

  vector<Cell> cells_;
  vector<Group> groups_;

  map<const dbMaster*, Master> db_master_map_;
  map<dbInst*, Cell*> db_inst_map_;

  int have_multi_row_cells_ = 0;
  int max_displacement_x_ = 0;  // sites
  int max_displacement_y_ = 0;  // sites
  bool disallow_one_site_gaps_ = false;
  vector<Cell*> placement_failures_;

  // 3D pixel grid
  std::unique_ptr<Grid> grid_;
  Cell dummy_cell_;
  RtreeBox regions_rtree;

  // Filler placement.
  // gap (in sites) -> seq of masters by implant
  map<dbTechLayer*, GapFillers> gap_fillers_;
  int filler_count_ = 0;
  bool have_fillers_ = false;
  bool have_one_site_cells_ = false;

  // Results saved for optional reporting.
  int64_t hpwl_before_ = 0;
  int64_t displacement_avg_ = 0;
  int64_t displacement_sum_ = 0;
  int64_t displacement_max_ = 0;

  std::unique_ptr<DplObserver> debug_observer_;

  // Magic numbers
  static constexpr int bin_search_width_ = 10;
  static constexpr double group_refine_percent_ = .05;
  static constexpr double refine_percent_ = .02;
  static constexpr int rand_seed_ = 777;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace dpl
