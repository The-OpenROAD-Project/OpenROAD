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
using odb::Point;
using odb::Rect;

struct Pixel;
struct Group;
class DplObserver;

using bgPoint
    = boost::geometry::model::d2::point_xy<int, boost::geometry::cs::cartesian>;
using bgBox = boost::geometry::model::box<bgPoint>;

using RtreeBox
    = boost::geometry::index::rtree<bgBox,
                                    boost::geometry::index::quadratic<16>>;
// The "Grid" is now an array of 2D grids. The new dimension is to support
// multi-height cells. Each unique row height creates a new grid that is used in
// legalization. The first index is the grid index (corresponding to row
// height), second index is the row index, and third index is the site index.
using Grid = std::vector<std::vector<std::vector<Pixel>>>;
using dbMasterSeq = vector<dbMaster*>;
// gap -> sequence of masters to fill the gap
using GapFillers = vector<dbMasterSeq>;

using InstPaddingMap = map<dbInst*, pair<int, int>>;
using MasterPaddingMap = map<dbMaster*, pair<int, int>>;

struct Master
{
  bool is_multi_row = false;
};

struct Grid_map_key
{
  int grid_index;
  // TODO: consider removing the two operator overloading
  bool operator<(const Grid_map_key& other) const
  {
    return grid_index < other.grid_index;
  }
  bool operator==(const Grid_map_key& other) const
  {
    return grid_index == other.grid_index;
  }
};

class HybridSiteInfo
{
 public:
  HybridSiteInfo(int index, dbSite* site) : index(index), site(site) {}
  int getIndex() { return index; }
  dbSite* getSite() { return site; }

 private:
  int index;
  dbSite* site;
};

struct Cell
{
  const char* name() const;
  bool inGroup() const { return group_ != nullptr; }
  int64_t area() const;

  dbInst* db_inst_ = nullptr;
  int x_ = 0, y_ = 0;  // lower left wrt core DBU
  dbOrientType orient_;
  int width_ = 0, height_ = 0;  // DBU
  bool is_placed_ = false;
  bool hold_ = false;
  Group* group_ = nullptr;
  Rect* region_ = nullptr;  // group rect

  bool isHybrid() const
  {
    if (!getSite()) {
      return false;
    }
    return getSite()->isHybrid();
  }

  bool isHybridParent() const
  {
    if (!getSite()) {
      return false;
    }
    return getSite()->isHybridParent();
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

class GridInfo
{
 public:
  GridInfo(int row_count,
           int site_count,
           int grid_index,
           const std::vector<std::pair<dbSite*, dbOrientType>>& sites)
      : row_count(row_count),
        site_count(site_count),
        grid_index(grid_index),
        sites(sites)
  {
  }

  int getRowCount() const { return row_count; }

  int getSiteCount() const { return site_count; }

  int getGridIndex() const { return grid_index; }

  const std::vector<std::pair<dbSite*, dbOrientType>>& getSites() const
  {
    return sites;
  }

  const bool isHybrid()
  {
    // TODO: consider removing the second part
    return sites.size() > 1 || sites[0].first->isHybridParent();
  }
  const int getSitesTotalHeight()
  {
    return std::accumulate(
        sites.begin(),
        sites.end(),
        0,
        [](int sum, const std::pair<dbSite*, dbOrientType>& entry) {
          return sum + entry.first->getHeight();
        });
  }

 private:
  const int row_count;
  const int site_count;
  const int grid_index;
  const std::vector<std::pair<dbSite*, dbOrientType>>
      sites;  // will have one site only for non-hybrid and hybrid parent cells.
              // For hybrid children, this will have all the sites
};

// For optimize mirroring.
class NetBox
{
 public:
  NetBox() = default;
  NetBox(dbNet* net, Rect box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();

  dbNet* net_ = nullptr;
  Rect box_;
  Rect box_saved_;
  bool ignore_ = false;
};

using NetBoxMap = unordered_map<dbNet*, NetBox>;
using NetBoxes = vector<NetBox*>;

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

  Point pointOffMacro(const Cell& cell);
  void convertDbToCell(dbInst* db_inst, Cell& cell);
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
  int padGlobalLeft() const { return pad_left_; }
  int padGlobalRight() const { return pad_right_; }
  // Find instance/master/global padding value for an instance.
  int padRight(dbInst* inst) const;
  int padLeft(dbInst* inst) const;
  // Return error count.
  void processViolationsPtree(boost::property_tree::ptree& entry,
                              const std::vector<Cell*>& failures,
                              const std::string& violation_type = "") const;
  void checkPlacement(bool verbose,
                      bool disallow_one_site_gaps = false,
                      string report_file_name = "");
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
  int64_t hpwl() const;
  int64_t hpwl(dbNet* net) const;
  void findDisplacementStats();
  void optimizeMirroring();

  const vector<Cell>& getCells() const { return cells_; }
  Rect getCore() const { return core_; }
  int getRowHeight() const { return row_height_; }
  int getRowHeight(const Cell* cell) const;
  int getSiteWidth() const { return site_width_; }
  int getRowCount() const { return row_count_; }
  int getRowSiteCount() const { return row_site_count_; }

 private:
  friend class OpendpTest_IsPlaced_Test;
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
  Grid_map_key getGridMapKey(const Cell* cell) const;
  Grid_map_key getGridMapKey(const dbSite* site) const;

  void makeMaster(Master* master, dbMaster* db_master);

  void initGrid();
  void initGridLayersMap();
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
  int getHybridSiteIndex(dbSite* site);
  int calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const;

  Point legalPt(const Cell* cell,
                const Point& pt,
                int row_height = -1,
                int site_width = -1) const;
  Point legalGridPt(const Cell* cell,
                    const Point& pt,
                    int row_height = -1,
                    int site_width = -1) const;
  Point legalPt(const Cell* cell,
                bool padded,
                int row_height = -1,
                int site_width = -1) const;
  Point legalGridPt(const Cell* cell,
                    bool padded,
                    int row_height = -1,
                    int site_width = -1) const;
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
  bool cellFitsInCore(Cell* cell);
  void setFixedGridCells();
  void visitCellPixels(Cell& cell,
                       bool padded,
                       const std::function<void(Pixel* pixel)>& visitor) const;
  void visitCellBoundaryPixels(
      Cell& cell,
      bool padded,
      const std::function<
          void(Pixel* pixel, odb::Direction2D edge, int x, int y)>& visitor)
      const;
  void setGridCell(Cell& cell, Pixel* pixel);
  void groupAssignCellRegions();
  void groupInitPixels();
  void groupInitPixels2();
  void erasePixel(Cell* cell);
  void paintPixel(Cell* cell, int grid_x, int grid_y);
  int map_ycoordinates(int source_grid_coordinate,
                       const Grid_map_key& source_grid_key,
                       const Grid_map_key& target_grid_key) const;

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
  bool havePadding() const;
  void checkOneSiteDbMaster();
  void deleteGrid();
  Pixel* gridPixel(int grid_idx, int x, int y) const;
  // Cell initial location wrt core origin.

  int getSiteWidth(const Cell* cell) const;
  int getRowCount(const Cell* cell) const;
  int getRowCount(int row_height) const;
  int gridPaddedWidth(const Cell* cell, int site_width) const;
  int gridPaddedWidth(const Cell* cell) const;
  int64_t paddedArea(const Cell* cell) const;
  int gridNearestHeight(const Cell* cell) const;
  int gridNearestHeight(const Cell* cell, int row_height) const;
  int gridNearestWidth(const Cell* cell) const;
  int gridNearestWidth(const Cell* cell, int site_width) const;
  int gridHeight(const Cell* cell, int row_height) const;
  int gridHeight(const Cell* cell) const;
  GridInfo getGridInfo(const Cell* cell) const;
  int gridX(int x, int site_width) const;
  int gridX(const Cell* cell) const;
  int gridX(const Cell* cell, int site_width) const;
  int gridPaddedX(const Cell* cell) const;
  int gridPaddedX(const Cell* cell, int site_width) const;
  int gridY(int y, int row_height) const;
  int gridY(const Cell* cell) const;
  pair<int, int> gridY(
      int y,
      const std::vector<std::pair<dbSite*, dbOrientType>>& grid_sites) const;
  pair<int, int> gridEndY(
      int y,
      const std::vector<std::pair<dbSite*, dbOrientType>>& grid_sites) const;
  int gridPaddedEndX(const Cell* cell) const;
  int gridPaddedEndX(const Cell* cell, int site_width) const;
  int gridEndX(int x, int site_width) const;
  int gridEndX(const Cell* cell) const;
  int gridEndX(const Cell* cell, int site_width) const;
  int gridEndY(int y, int row_height) const;
  int gridEndY(const Cell* cell) const;
  void setGridPaddedLoc(Cell* cell, int x, int y, int site_width) const;
  std::pair<int, GridInfo> getRowInfo(const Cell* cell) const;
  // Lower left corner in core coordinates.
  Point initialLocation(const Cell* cell, bool padded) const;
  bool isStdCell(const Cell* cell) const;
  static bool isBlock(const Cell* cell);
  int paddedWidth(const Cell* cell, int site_width) const;
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
                       dbMasterSeq* filler_masters,
                       int row_height,
                       GridInfo grid_info);
  bool isFiller(odb::dbInst* db_inst);
  bool isOneSiteCell(odb::dbMaster* db_master) const;
  const char* gridInstName(int row,
                           int col,
                           int row_height,
                           GridInfo grid_info);

  // Optimizing mirroring
  void findNetBoxes();
  vector<dbInst*> findMirrorCandidates(NetBoxes& net_boxes);
  int mirrorCandidates(vector<dbInst*>& mirror_candidates);
  // Sum of ITerm hpwl's.
  int64_t hpwl(dbInst* inst);
  void updateNetBoxes(dbInst* inst);
  void saveNetBoxes(dbInst* inst);
  void restoreNetBoxes(dbInst* inst);

  Logger* logger_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;
  int pad_left_ = 0;
  int pad_right_ = 0;
  InstPaddingMap inst_padding_map_;
  MasterPaddingMap master_padding_map_;

  vector<Cell> cells_;
  vector<Group> groups_;

  map<const dbMaster*, Master> db_master_map_;
  map<Grid_map_key, GridInfo> grid_info_map_;
  unordered_map<int, int>
      site_idx_to_grid_idx;  // this map is used to map each unqie site to a
                             // grid. the key is always unique, but the value is
                             // not unique in the case of hybrid sites
                             // (alternating rows)
  Grid_map_key smallest_non_hybrid_grid_key;
  std::vector<GridInfo*> grid_info_vector_;
  map<int, int> siteIdToGridId;
  map<dbInst*, Cell*> db_inst_map_;

  Rect core_;
  int row_height_ = 0;  // dbu
  int site_width_ = 0;  // dbu
  int row_count_ = 0;
  int row_site_count_ = 0;
  int have_multi_row_cells_ = 0;
  int max_displacement_x_ = 0;  // sites
  int max_displacement_y_ = 0;  // sites
  bool disallow_one_site_gaps_ = false;
  vector<Cell*> placement_failures_;

  // 3D pixel grid
  Grid grid_;
  Cell dummy_cell_;
  RtreeBox regions_rtree;

  // Filler placement.
  // gap (in sites) -> seq of masters
  GapFillers gap_fillers_;
  int filler_count_ = 0;
  bool have_fillers_ = false;
  bool have_one_site_cells_ = false;

  // Results saved for optional reporting.
  int64_t hpwl_before_ = 0;
  int64_t displacement_avg_ = 0;
  int64_t displacement_sum_ = 0;
  int64_t displacement_max_ = 0;

  // Optimiize mirroring.
  NetBoxMap net_box_map_;

  std::unique_ptr<DplObserver> debug_observer_;

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
