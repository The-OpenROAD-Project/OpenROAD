// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
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

#include "boost/geometry/core/cs.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "utl/Logger.h"
// NOLINTNEXTLINE
#include "boost/geometry/strategies/strategies.hpp"  // Required implictly by rtree
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace dpl {

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

using dbMasterSeq = std::vector<odb::dbMaster*>;

using IRDropByPoint = std::map<odb::Point, double>;
struct GapInfo;
struct DecapCell;
struct IRDrop;

struct GlobalSwapParams
{
  int passes = 2;
  double tolerance = 0.01;
  double tradeoff = 0.4;
  double profiling_excess = 1.10;
  std::vector<double> budget_multipliers{1.50, 1.25, 1.10, 1.04};
  double area_weight = 0.4;
  double pin_weight = 0.6;
  double user_congestion_weight = 35.0;
  int sampling_moves = 150;
  int normalization_interval = 1000;
};
////////////////////////////////////////////////////////////////

class Opendp
{
 public:
  Opendp(odb::dbDatabase* db, utl::Logger* logger);
  ~Opendp();

  Opendp(const Opendp&) = delete;
  Opendp& operator=(const Opendp&) = delete;

  void legalCellPos(odb::dbInst* db_inst);  // call from rsz
  void initMacrosAndGrid();                 // call from rsz

  // legalize/report
  // max_displacment is in sites. use zero for defaults.
  void detailedPlacement(int max_displacement_x,
                         int max_displacement_y,
                         const std::string& report_file_name = std::string(""),
                         bool incremental = false);
  void reportLegalizationStats() const;

  void setPaddingGlobal(int left, int right);
  void setPadding(odb::dbMaster* master, int left, int right);
  void setPadding(odb::dbInst* inst, int left, int right);
  void setDebug(std::unique_ptr<dpl::DplObserver>& observer);
  void setJumpMoves(int jump_moves);
  void setIterativePlacement(bool iterative);
  void setDeepIterativePlacement(bool deep_iterative);

  // Global padding.
  int padGlobalLeft() const;
  int padGlobalRight() const;
  // Find instance/master/global padding value for an instance.
  int padLeft(odb::dbInst* inst) const;
  int padRight(odb::dbInst* inst) const;

  void checkPlacement(bool verbose, const std::string& report_file_name = "");
  void fillerPlacement(const dbMasterSeq& filler_masters,
                       const char* prefix,
                       bool verbose);
  void removeFillers();
  void optimizeMirroring();
  void resetGlobalSwapParams();
  void configureGlobalSwapParams(int passes,
                                 double tolerance,
                                 double tradeoff,
                                 double area_weight,
                                 double pin_weight,
                                 double user_weight,
                                 int sampling_moves,
                                 int normalization_interval,
                                 double profiling_excess,
                                 const std::vector<double>& budget_multipliers);
  const GlobalSwapParams& getGlobalSwapParams() const
  {
    return global_swap_params_;
  }
  void setExtraDplEnabled(bool enabled) { extra_dpl_enabled_ = enabled; }
  bool isExtraDplEnabled() const { return extra_dpl_enabled_; }

  // Place decap cells
  void addDecapMaster(odb::dbMaster* decap_master, double decap_cap);
  void insertDecapCells(double target, IRDropByPoint& psm_ir_drops);

  // Get the instance adjacent to the left or right of a given instance
  odb::dbInst* getAdjacentInstance(odb::dbInst* inst, bool left) const;

  // Find a cluster of instances that are touching each other
  std::vector<odb::dbInst*> getAdjacentInstancesCluster(
      odb::dbInst* inst) const;
  Padding* getPadding() { return padding_.get(); }
  void improvePlacement(int seed,
                        int max_displacement_x,
                        int max_displacement_y);
  // Journalling
  Journal* getJournal() const;
  void setJournal(Journal* journal);

  odb::Point getOdbLocation(const Node* cell) const;
  odb::Point getDplLocation(const Node* cell) const;

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

  using MasterByImplant = std::map<odb::dbTechLayer*, dbMasterSeq>;

  using YCoordToGap = std::map<DbuY, std::vector<std::unique_ptr<GapInfo>>>;

  friend class OpendpTest_IsPlaced_Test;
  friend class Graphics;
  void findDisplacementStats();
  DbuPt pointOffMacro(const Node& cell);
  void convertDbToCell(odb::dbInst* db_inst, Node& cell);
  // Return error count.
  void saveViolations(const std::vector<Node*>& failures,
                      odb::dbMarkerCategory* category,
                      const std::string& violation_type = "") const;
  void importDb();
  void importClear();
  odb::Rect getBbox(odb::dbInst* inst);
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
  int distToRect(const Node* cell, const odb::Rect& rect) const;
  static bool checkOverlap(const odb::Rect& cell, const odb::Rect& box);
  bool checkOverlap(const Node* cell, const DbuRect& rect) const;
  static bool isInside(const odb::Rect& cell, const odb::Rect& box);
  bool isInside(const Node* cell, const odb::Rect& rect) const;
  PixelPt diamondSearch(const Node* cell, GridX x, GridY y) const;
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
  bool checkMasterSym(unsigned masterSym, unsigned cellOri) const;
  bool shiftMove(Node* cell);
  bool mapMove(Node* cell);
  bool mapMove(Node* cell, const GridPt& grid_pt);
  int distChange(const Node* cell, DbuX x, DbuY y) const;
  bool swapCells(Node* cell1, Node* cell2);
  bool refineMove(Node* cell);
  void deepIterativePause(const std::string& message, bool only_print = false);

  DbuPt legalPt(const Node* cell, const DbuPt& pt) const;
  GridPt legalGridPt(const Node* cell, const DbuPt& pt) const;
  DbuPt legalPt(const Node* cell, bool padded) const;
  GridPt legalGridPt(const Node* cell, bool padded) const;
  DbuPt nearestBlockEdge(const Node* cell,
                         const DbuPt& legal_pt,
                         const odb::Rect& block_bbox) const;

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
                    const std::vector<Node*>& padding_failures,
                    const std::vector<Node*>& one_site_gap_failures,
                    const std::vector<Node*>& site_align_failures,
                    const std::vector<Node*>& region_placement_failures,
                    const std::vector<Node*>& placement_failures,
                    const std::vector<Node*>& edge_spacing_failures,
                    const std::vector<Node*>& blocked_layers_failures);
  void writeJsonReport(const std::string& filename);

  void rectDist(const Node* cell,
                const odb::Rect& rect,
                // Return values.
                int* x,
                int* y) const;
  int rectDist(const Node* cell, const odb::Rect& rect) const;
  void deleteGrid();
  // Cell initial location wrt core origin.

  // Lower left corner in core coordinates.
  DbuPt initialLocation(const Node* cell, bool padded) const;
  int disp(const Node* cell) const;
  // Place fillers
  dbMasterSeq filterFillerMasters(const dbMasterSeq& filler_masters) const;
  MasterByImplant splitByImplant(const dbMasterSeq& filler_masters);
  void setInitialGridCells();
  void setGridCells();
  dbMasterSeq& gapFillers(odb::dbTechLayer* implant,
                          GridX gap,
                          const MasterByImplant& filler_masters_by_implant);
  void placeRowFillers(GridY row,
                       const std::string& prefix,
                       const MasterByImplant& filler_masters);
  static bool isFiller(odb::dbInst* db_inst);
  bool isOneSiteCell(odb::dbMaster* db_master) const;
  const char* gridInstName(GridY row, GridX col);

  // Place decaps
  std::vector<int> findDecapCellIndices(const DbuX& gap_width,
                                        const double& current,
                                        const double& target);
  void insertDecapInPos(odb::dbMaster* master,
                        const DbuX& pos_x,
                        const DbuY& pos_y);
  void insertDecapInRow(const std::vector<std::unique_ptr<GapInfo>>& gaps,
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
  void setGridLoc(Node* cell, GridX x, GridY y);

  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
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
  std::map<odb::dbTechLayer*, GapFillers> gap_fillers_;
  std::map<odb::dbMaster*, int> filler_count_;
  bool have_fillers_ = false;

  // Decap placement.
  std::vector<std::unique_ptr<DecapCell>> decap_masters_;
  int decap_count_ = 0;
  YCoordToGap gaps_;

  // Results saved for optional reporting.
  int64_t hpwl_before_ = 0;
  int64_t displacement_avg_ = 0;
  int64_t displacement_sum_ = 0;
  int64_t displacement_max_ = 0;

  std::unique_ptr<DplObserver> debug_observer_;
  std::unique_ptr<Node> dummy_cell_;
  int jump_moves_ = 0;
  int move_count_ = 1;
  bool iterative_placement_ = false;
  bool deep_iterative_placement_ = false;
  bool incremental_ = false;

  // Magic numbers
  static constexpr double group_refine_percent_ = .05;
  static constexpr double refine_percent_ = .02;
  static constexpr int rand_seed_ = 777;
  GlobalSwapParams global_swap_params_;
  bool extra_dpl_enabled_ = false;
};

int divRound(int dividend, int divisor);
int divCeil(int dividend, int divisor);
int divFloor(int dividend, int divisor);

}  // namespace dpl
