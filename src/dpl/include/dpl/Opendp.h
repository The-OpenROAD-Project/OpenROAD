// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <functional>
#include <map>
#include <memory>
#include <numeric>  // accumulate
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>  // pair
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace dpl {

using utl::Logger;

using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbTechLayer;
using odb::Point;
using odb::Rect;

class Node;
class Group;
class Master;
class Edge;

class Architecture;
class Network;
struct Pixel;

class DplObserver;
class Grid;
class GridInfo;
class Padding;
class PixelPt;
class PlacementDRC;
class Journal;

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

using dbMasterSeq = std::vector<dbMaster*>;

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
                         const std::string& report_file_name = std::string(""));
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

  void checkPlacement(bool verbose, const std::string& report_file_name = "");
  void fillerPlacement(const dbMasterSeq& filler_masters,
                       const char* prefix,
                       bool verbose);
  void removeFillers();
  void optimizeMirroring();

  // Place decap cells
  void addDecapMaster(dbMaster* decap_master, double decap_cap);
  void insertDecapCells(double target, IRDropByPoint& psm_ir_drops);

  // Get the instance adjacent to the left or right of a given instance
  dbInst* getAdjacentInstance(dbInst* inst, bool left) const;

  // Find a cluster of instances that are touching each other
  std::vector<dbInst*> getAdjacentInstancesCluster(dbInst* inst) const;
  Padding* getPadding() { return padding_.get(); }
  void improvePlacement(int seed,
                        int max_displacement_x,
                        int max_displacement_y);
  // Journalling
  Journal* getJournal() const;
  void setJournal(Journal* journal);

 private:
  using bgPoint
      = boost::geometry::model::d2::point_xy<int,
                                             boost::geometry::cs::cartesian>;
  using bgBox = boost::geometry::model::box<bgPoint>;

  using RtreeBox
      = boost::geometry::index::rtree<bgBox,
                                      boost::geometry::index::quadratic<16>>;

  // gap -> sequence of masters to fill the gap
  using GapFillers = std::vector<dbMasterSeq>;

  using MasterByImplant = std::map<dbTechLayer*, dbMasterSeq>;

  using YCoordToGap = std::map<DbuY, std::vector<GapInfo*>>;

  friend class OpendpTest_IsPlaced_Test;
  friend class Graphics;
  void findDisplacementStats();
  DbuPt pointOffMacro(const Node& cell);
  void convertDbToCell(dbInst* db_inst, Node& cell);
  // Return error count.
  void saveViolations(const std::vector<Node*>& failures,
                      odb::dbMarkerCategory* category,
                      const std::string& violation_type = "") const;
  void importDb();
  void importClear();
  Rect getBbox(dbInst* inst);
  void createNetwork();
  void createArchitecture();
  void setUpPlacementGroups();
  void adjustNodesOrient();
  bool isMultiRow(const Node* cell) const;
  void updateDbInstLocations();

  void initGrid();

  void initPlacementDRC();

  std::string printBgBox(const boost::geometry::model::box<bgPoint>& queryBox);
  void detailedPlacement();
  DbuPt nearestPt(const Node* cell, const DbuRect& rect) const;
  int distToRect(const Node* cell, const Rect& rect) const;
  static bool checkOverlap(const Rect& cell, const Rect& box);
  bool checkOverlap(const Node* cell, const DbuRect& rect) const;
  static bool isInside(const Rect& cell, const Rect& box);
  bool isInside(const Node* cell, const Rect& rect) const;
  PixelPt searchNearestSite(const Node* cell, GridX x, GridY y) const;
  int calcDist(GridPt p0, GridPt p1) const;
  bool canBePlaced(const Node* cell, GridX bin_x, GridY bin_y) const;
  bool checkRegionOverlap(const Node* cell,
                          GridX x,
                          GridY y,
                          GridX x_end,
                          GridY y_end) const;
  bool checkPixels(const Node* cell,
                   GridX x,
                   GridY y,
                   GridX x_end,
                   GridY y_end) const;
  bool shiftMove(Node* cell);
  bool mapMove(Node* cell);
  bool mapMove(Node* cell, const GridPt& grid_pt);
  int distChange(const Node* cell, DbuX x, DbuY y) const;
  bool swapCells(Node* cell1, Node* cell2);
  bool refineMove(Node* cell);

  DbuPt legalPt(const Node* cell, const DbuPt& pt) const;
  GridPt legalGridPt(const Node* cell, const DbuPt& pt) const;
  DbuPt legalPt(const Node* cell, bool padded) const;
  GridPt legalGridPt(const Node* cell, bool padded) const;
  DbuPt nearestBlockEdge(const Node* cell,
                         const DbuPt& legal_pt,
                         const Rect& block_bbox) const;

  void findOverlapInRtree(const bgBox& queryBox,
                          std::vector<bgBox>& overlaps) const;
  bool moveHopeless(const Node* cell, GridX& grid_x, GridY& grid_y) const;
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
  void setGridCell(Node& cell, Pixel* pixel);
  void groupAssignCellRegions();
  void groupInitPixels();
  void groupInitPixels2();

  // checkPlacement
  static bool isPlaced(const Node* cell);
  bool checkInRows(const Node& cell) const;
  const Node* checkOverlap(Node& cell) const;
  Node* checkOneSiteGaps(Node& cell) const;
  bool overlap(const Node* cell1, const Node* cell2) const;
  bool checkRegionPlacement(const Node* cell) const;
  static bool isOverlapPadded(const Node* cell1, const Node* cell2);
  static bool isCrWtBlClass(const Node* cell);
  static bool isWellTap(const Node* cell);
  void reportFailures(const std::vector<Node*>& failures,
                      int msg_id,
                      const char* msg,
                      bool verbose) const;
  void reportFailures(
      const std::vector<Node*>& failures,
      int msg_id,
      const char* msg,
      bool verbose,
      const std::function<void(Node* cell)>& report_failure) const;
  void reportOverlapFailure(Node* cell) const;
  void saveFailures(const std::vector<Node*>& placed_failures,
                    const std::vector<Node*>& in_rows_failures,
                    const std::vector<Node*>& overlap_failures,
                    const std::vector<Node*>& one_site_gap_failures,
                    const std::vector<Node*>& site_align_failures,
                    const std::vector<Node*>& region_placement_failures,
                    const std::vector<Node*>& placement_failures,
                    const std::vector<Node*>& edge_spacing_failures);
  void writeJsonReport(const std::string& filename);

  void rectDist(const Node* cell,
                const Rect& rect,
                // Return values.
                int* x,
                int* y) const;
  int rectDist(const Node* cell, const Rect& rect) const;
  void deleteGrid();
  // Cell initial location wrt core origin.

  // Lower left corner in core coordinates.
  DbuPt initialLocation(const Node* cell, bool padded) const;
  int disp(const Node* cell) const;
  // Place fillers
  MasterByImplant splitByImplant(const dbMasterSeq& filler_masters);
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
  std::vector<int> findDecapCellIndices(const DbuX& gap_width,
                                        const double& current,
                                        const double& target);
  void insertDecapInPos(dbMaster* master, const DbuX& pos_x, const DbuY& pos_y);
  void insertDecapInRow(const std::vector<GapInfo*>& gaps,
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
  void placeCell(Node* cell, GridX x, GridY y);
  void unplaceCell(Node* cell);
  void setGridPaddedLoc(Node* cell, GridX x, GridY y);

  Logger* logger_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;
  odb::Rect core_;

  std::unique_ptr<Architecture> arch_;  // Information about rows, etc.
  std::unique_ptr<Network> network_;    // The netlist, cells, etc.
  std::shared_ptr<Padding> padding_;
  std::unique_ptr<PlacementDRC> drc_engine_;
  Journal* journal_ = nullptr;

  bool have_multi_row_cells_ = false;
  int max_displacement_x_ = 0;  // sites
  int max_displacement_y_ = 0;  // sites
  bool disallow_one_site_gaps_ = false;
  std::vector<Node*> placement_failures_;

  // 2D pixel grid
  std::unique_ptr<Grid> grid_;
  RtreeBox regions_rtree_;

  // Filler placement.
  // gap (in sites) -> seq of masters by implant
  std::map<dbTechLayer*, GapFillers> gap_fillers_;
  std::map<dbMaster*, int> filler_count_;
  bool have_fillers_ = false;

  // Decap placement.
  std::vector<DecapCell*> decap_masters_;
  int decap_count_ = 0;
  YCoordToGap gaps_;

  // Results saved for optional reporting.
  int64_t hpwl_before_ = 0;
  int64_t displacement_avg_ = 0;
  int64_t displacement_sum_ = 0;
  int64_t displacement_max_ = 0;

  std::unique_ptr<DplObserver> debug_observer_;
  std::unique_ptr<Node> dummy_cell_;

  // Magic numbers
  static constexpr double group_refine_percent_ = .05;
  static constexpr double refine_percent_ = .02;
  static constexpr int rand_seed_ = 777;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace dpl
