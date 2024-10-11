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
using std::vector;

using utl::Logger;

using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbTechLayer;
using odb::Point;
using odb::Rect;

struct Cell;
struct Group;
struct Master;
struct Pixel;

class DplObserver;
class Grid;
class GridInfo;
class Padding;
class PixelPt;

template <typename T>
struct TypedCoordinate;

// These have to be defined here even though they are only used
// in the implementation section.  C++ doesn't allow you to forward
// declare types of this sort.
struct GridXType;
using GridX = TypedCoordinate<GridXType>;

struct GridYType;
using GridY = TypedCoordinate<GridYType>;

struct DbuXType;
using DbuX = TypedCoordinate<DbuXType>;

struct DbuYType;
using DbuY = TypedCoordinate<DbuYType>;

struct GridPt;
struct DbuPt;
struct DbuRect;

using dbMasterSeq = vector<dbMaster*>;

using IRDropByPoint = std::map<odb::Point, double>;
struct GapInfo;
struct DecapCell;
struct IRDrop;
////////////////////////////////////////////////////////////////

class Opendp
{
 public:
  Opendp();
  ~Opendp();

  Opendp(const Opendp&) = delete;
  Opendp& operator=(const Opendp&) = delete;

  void legalCellPos(dbInst* db_inst);  // call from rsz
  void initMacrosAndGrid();            // call from rsz

  void init(dbDatabase* db, Logger* logger);
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
  void fillerPlacement(dbMasterSeq* filler_masters, const char* prefix);
  void removeFillers();
  void optimizeMirroring();

  // Place decap cells
  void addDecapMaster(dbMaster* decap_master, double decap_cap);
  void insertDecapCells(double target, IRDropByPoint& psm_ir_drops);

 private:
  using bgPoint
      = boost::geometry::model::d2::point_xy<int,
                                             boost::geometry::cs::cartesian>;
  using bgBox = boost::geometry::model::box<bgPoint>;

  using RtreeBox
      = boost::geometry::index::rtree<bgBox,
                                      boost::geometry::index::quadratic<16>>;

  // gap -> sequence of masters to fill the gap
  using GapFillers = vector<dbMasterSeq>;

  using MasterByImplant = std::map<dbTechLayer*, dbMasterSeq>;

  using YCoordToGap = std::map<DbuY, vector<GapInfo*>>;

  friend class OpendpTest_IsPlaced_Test;
  friend class Graphics;
  void findDisplacementStats();
  DbuPt pointOffMacro(const Cell& cell);
  void convertDbToCell(dbInst* db_inst, Cell& cell);
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
  bool isMultiRow(const Cell* cell) const;
  void updateDbInstLocations();

  void makeMaster(Master* master, dbMaster* db_master);

  void initGrid();
  std::string printBgBox(const boost::geometry::model::box<bgPoint>& queryBox);
  void detailedPlacement();
  DbuPt nearestPt(const Cell* cell, const DbuRect& rect) const;
  int distToRect(const Cell* cell, const Rect& rect) const;
  static bool checkOverlap(const Rect& cell, const Rect& box);
  bool checkOverlap(const Cell* cell, const DbuRect& rect) const;
  static bool isInside(const Rect& cell, const Rect& box);
  bool isInside(const Cell* cell, const Rect& rect) const;
  PixelPt diamondSearch(const Cell* cell, GridX x, GridY y) const;
  void diamondSearchSide(const Cell* cell,
                         GridX x,
                         GridY y,
                         GridX x_min,
                         GridY y_min,
                         GridX x_max,
                         GridY y_max,
                         int x_offset,
                         int y_offset,
                         // Return values
                         PixelPt& best_pt,
                         int& best_dist) const;
  PixelPt binSearch(GridX x, const Cell* cell, GridX bin_x, GridY bin_y) const;
  bool checkRegionOverlap(const Cell* cell,
                          GridX x,
                          GridY y,
                          GridX x_end,
                          GridY y_end) const;
  bool checkPixels(const Cell* cell,
                   GridX x,
                   GridY y,
                   GridX x_end,
                   GridY y_end) const;
  void shiftMove(Cell* cell);
  bool mapMove(Cell* cell);
  bool mapMove(Cell* cell, const GridPt& grid_pt);
  int distChange(const Cell* cell, DbuX x, DbuY y) const;
  bool swapCells(Cell* cell1, Cell* cell2);
  bool refineMove(Cell* cell);

  DbuPt legalPt(const Cell* cell, const DbuPt& pt) const;
  GridPt legalGridPt(const Cell* cell, const DbuPt& pt) const;
  DbuPt legalPt(const Cell* cell, bool padded) const;
  GridPt legalGridPt(const Cell* cell, bool padded) const;
  DbuPt nearestBlockEdge(const Cell* cell,
                         const DbuPt& legal_pt,
                         const Rect& block_bbox) const;

  void findOverlapInRtree(const bgBox& queryBox, vector<bgBox>& overlaps) const;
  bool moveHopeless(const Cell* cell, GridX& grid_x, GridY& grid_y) const;
  void placeGroups();
  void prePlace();
  void prePlaceGroups();
  void place();
  void placeGroups2();
  void brickPlace1(const Group* group);
  void brickPlace2(const Group* group);
  int groupRefine(const Group* group);
  int anneal(Group* group);
  int refine();
  void setFixedGridCells();
  void setGridCell(Cell& cell, Pixel* pixel);
  void groupAssignCellRegions();
  void groupInitPixels();
  void groupInitPixels2();

  // checkPlacement
  static bool isPlaced(const Cell* cell);
  bool checkInRows(const Cell& cell) const;
  const Cell* checkOverlap(Cell& cell) const;
  Cell* checkOneSiteGaps(Cell& cell) const;
  bool overlap(const Cell* cell1, const Cell* cell2) const;
  bool checkRegionPlacement(const Cell* cell) const;
  static bool isOverlapPadded(const Cell* cell1, const Cell* cell2);
  static bool isCrWtBlClass(const Cell* cell);
  static bool isWellTap(const Cell* cell);
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
  void writeJsonReport(const string& filename,
                       const vector<Cell*>& placed_failures,
                       const vector<Cell*>& in_rows_failures,
                       const vector<Cell*>& overlap_failures,
                       const vector<Cell*>& one_site_gap_failures,
                       const vector<Cell*>& site_align_failures,
                       const vector<Cell*>& region_placement_failures,
                       const vector<Cell*>& placement_failures);

  void rectDist(const Cell* cell,
                const Rect& rect,
                // Return values.
                int* x,
                int* y) const;
  int rectDist(const Cell* cell, const Rect& rect) const;
  void checkOneSiteDbMaster();
  void deleteGrid();
  // Cell initial location wrt core origin.

  // Lower left corner in core coordinates.
  DbuPt initialLocation(const Cell* cell, bool padded) const;
  int disp(const Cell* cell) const;
  // Place fillers
  MasterByImplant splitByImplant(dbMasterSeq* filler_masters);
  void setGridCells();
  dbMasterSeq& gapFillers(dbTechLayer* implant,
                          GridX gap,
                          const MasterByImplant& filler_masters_by_implant);
  void placeRowFillers(GridY row,
                       const std::string& prefix,
                       const MasterByImplant& filler_masters);
  std::pair<odb::dbSite*, odb::dbOrientType> fillSite(Pixel* pixel);
  static bool isFiller(odb::dbInst* db_inst);
  bool isOneSiteCell(odb::dbMaster* db_master) const;
  const char* gridInstName(GridY row, GridX col);

  // Place decaps
  vector<int> findDecapCellIndices(const DbuX& gap_width,
                                   const double& current,
                                   const double& target);
  void insertDecapInPos(dbMaster* master, const DbuX& pos_x, const DbuY& pos_y);
  void insertDecapInRow(const vector<GapInfo*>& gaps,
                        DbuY gap_y,
                        DbuX irdrop_x,
                        DbuY irdrop_y,
                        double& total,
                        const double& target);
  void findGaps();
  void findGapsInRow(GridY row, DbuY row_height);
  void mapToVectorIRDrops(IRDropByPoint& psm_ir_drops,
                          std::vector<IRDrop>& ir_drops);
  void prepareDecapAndGaps();

  Logger* logger_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;
  std::shared_ptr<Padding> padding_;

  vector<Cell> cells_;
  vector<Group> groups_;

  map<const dbMaster*, Master> db_master_map_;
  map<dbInst*, Cell*> db_inst_map_;

  bool have_multi_row_cells_ = false;
  int max_displacement_x_ = 0;  // sites
  int max_displacement_y_ = 0;  // sites
  bool disallow_one_site_gaps_ = false;
  vector<Cell*> placement_failures_;

  // 2D pixel grid
  std::unique_ptr<Grid> grid_;
  RtreeBox regions_rtree_;

  // Filler placement.
  // gap (in sites) -> seq of masters by implant
  map<dbTechLayer*, GapFillers> gap_fillers_;
  int filler_count_ = 0;
  bool have_fillers_ = false;
  bool have_one_site_cells_ = false;

  // Decap placement.
  vector<DecapCell*> decap_masters_;
  int decap_count_ = 0;
  YCoordToGap gaps_;

  // Results saved for optional reporting.
  int64_t hpwl_before_ = 0;
  int64_t displacement_avg_ = 0;
  int64_t displacement_sum_ = 0;
  int64_t displacement_max_ = 0;

  std::unique_ptr<DplObserver> debug_observer_;
  std::unique_ptr<Cell> dummy_cell_;

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
