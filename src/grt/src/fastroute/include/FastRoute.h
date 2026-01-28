// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataType.h"
#include "Graph2D.h"
#include "boost/functional/hash.hpp"
#include "boost/icl/interval.hpp"
#include "boost/icl/interval_set.hpp"
#include "boost/multi_array.hpp"
#include "grt/GRoute.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"

namespace utl {
class CallBackHandler;
class Logger;
}  // namespace utl

namespace odb {
class dbDatabase;
class dbTechLayerDir;
class dbTechLayer;
}  // namespace odb

namespace stt {
class SteinerTreeBuilder;
}

namespace gui {
class Gui;
}

namespace sta {
class dbSta;
}

namespace grt {

using boost::multi_array;
using boost::icl::interval;
using boost::icl::interval_set;

class AbstractFastRouteRenderer;
// Debug mode settings
struct DebugSetting
{
  const odb::dbNet* net = nullptr;
  bool steinerTree = false;
  bool rectilinearSTree = false;
  bool tree2D = false;
  bool tree3D = false;
  std::unique_ptr<AbstractFastRouteRenderer> renderer;
  std::string sttInputFileName;

  bool isOn() const { return renderer != nullptr; }
};

struct parent3D
{
  int16_t layer;
  int x, y;
};

struct CostParams
{
  const float logistic_coef;
  const float cost_height;
  const int slope;

  CostParams(float logistic_coef, float cost_height, int slope)
      : logistic_coef(logistic_coef), cost_height(cost_height), slope(slope)
  {
  }
};

class FastRouteCore
{
 public:
  FastRouteCore(odb::dbDatabase* db,
                utl::Logger* log,
                utl::CallBackHandler* callback_handler,
                stt::SteinerTreeBuilder* stt_builder,
                sta::dbSta* sta);
  ~FastRouteCore();

  void clear();
  void saveCongestion(int iter = -1);
  void setGridsAndLayers(int x, int y, int nLayers);
  void addVCapacity(int16_t verticalCapacity, int layer);
  void addHCapacity(int16_t horizontalCapacity, int layer);
  void setLowerLeft(int x, int y);
  void setTileSize(int size);
  void setResistanceAware(bool resistance_aware);
  void addLayerDirection(int layer_idx, const odb::dbTechLayerDir& direction);
  FrNet* addNet(odb::dbNet* db_net,
                bool is_clock,
                bool is_local,
                int driver_idx,
                int8_t cost,
                int min_layer,
                int max_layer,
                float slack,
                std::vector<int8_t>* edge_cost_per_layer);
  void deleteNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);
  void mergeNet(odb::dbNet* removed_net, odb::dbNet* preserved_net);
  void clearNetRoute(odb::dbNet* db_net);
  void clearNetsToRoute() { net_ids_.clear(); }
  void initEdges();
  void init3DEdges();
  void initLowerBoundCapacities();
  void setEdgeCapacity(int x1, int y1, int x2, int y2, int layer, int capacity);
  int getDbNetLayerEdgeCost(odb::dbNet* db_net, int layer);
  void initEdgesCapacityPerLayer();
  void setNumAdjustments(int nAdjustments);
  void addAdjustment(int x1,
                     int y1,
                     int x2,
                     int y2,
                     int layer,
                     uint16_t reducedCap,
                     bool isReduce);
  void releaseResourcesOnInterval(
      int x,
      int y,
      int layer,
      bool is_horizontal,
      const interval<int>::type& tile_reduce_interval,
      const std::vector<int>& track_space);
  void addVerticalAdjustments(
      const odb::Point& first_tile,
      const odb::Point& last_tile,
      int layer,
      const interval<int>::type& first_tile_reduce_interval,
      const interval<int>::type& last_tile_reduce_interval,
      const std::vector<int>& track_space,
      bool release = false);
  void addHorizontalAdjustments(
      const odb::Point& first_tile,
      const odb::Point& last_tile,
      int layer,
      const interval<int>::type& first_tile_reduce_interval,
      const interval<int>::type& last_tile_reduce_interval,
      const std::vector<int>& track_space,
      bool release = false);
  void saveResourcesBeforeAdjustments();
  bool computeSuggestedAdjustment(int& suggested_adjustment);
  void getPrecisionAdjustment(int x,
                              int y,
                              bool is_horizontal,
                              int& adjustment);
  void initBlockedIntervals(std::vector<int>& track_space);
  void initAuxVar();
  NetRouteMap run();
  int totalOverflow() const { return total_overflow_; }
  bool has2Doverflow() const { return has_2D_overflow_; }
  void getBlockage(odb::dbTechLayer* layer,
                   int x,
                   int y,
                   uint8_t& blockage_h,
                   uint8_t& blockage_v);
  void updateDbCongestion(int min_routing_layer, int max_routing_layer);
  void getCapacityReductionData(CapacityReductionData& cap_red_data);
  void findCongestedEdgesNets(NetsPerCongestedArea& nets_in_congested_edges,
                              bool vertical);
  void getCongestionGrid(std::vector<CongestionInformation>& congestionGridV,
                         std::vector<CongestionInformation>& congestionGridH);

  const std::vector<int16_t>& getVerticalCapacities() { return v_capacity_3D_; }
  const std::vector<int16_t>& getHorizontalCapacities()
  {
    return h_capacity_3D_;
  }
  int getAvailableResources(int x1, int y1, int x2, int y2, int layer);
  int getEdgeCapacity(int x1, int y1, int x2, int y2, int layer);
  const multi_array<Edge3D, 3>& getHorizontalEdges3D() { return h_edges_3D_; }
  const multi_array<Edge3D, 3>& getVerticalEdges3D() { return v_edges_3D_; }
  void setLastColVCapacity(int16_t cap, int layer)
  {
    last_col_v_capacity_3D_[layer] = cap;
  }
  void setLastRowHCapacity(int16_t cap, int layer)
  {
    last_row_h_capacity_3D_[layer] = cap;
  }
  const std::vector<int16_t>& getLastColumnVerticalCapacities()
  {
    return last_col_v_capacity_3D_;
  }
  const std::vector<int16_t>& getLastRowHorizontalCapacities()
  {
    return last_row_h_capacity_3D_;
  }
  void setRegularX(bool regular_x) { regular_x_ = regular_x; }
  void setRegularY(bool regular_y) { regular_y_ = regular_y; }
  void incrementEdge3DUsage(int x1, int y1, int x2, int y2, int layer);
  void updateEdge2DAnd3DUsage(int x1,
                              int y1,
                              int x2,
                              int y2,
                              int layer,
                              int used,
                              odb::dbNet* db_net);
  void setMaxNetDegree(int);
  void updateRouteGridsLayer(int x1,
                             int y1,
                             int x2,
                             int y2,
                             int layer,
                             int new_layer,
                             odb::dbNet* db_net);
  void setVerbose(bool v);
  void setCriticalNetsPercentage(float u);
  float getCriticalNetsPercentage() { return critical_nets_percentage_; };
  void setOverflowIterations(int iterations);
  void setCongestionReportIterStep(int congestion_report_iter_step);
  void setCongestionReportFile(const char* congestion_file_name);
  void setGridMax(int x_max, int y_max);
  void setDetourPenalty(int penalty);
  void getCongestionNets(std::set<odb::dbNet*>& congestion_nets);
  void computeCongestionInformation();
  std::vector<int> getOriginalResources();
  const std::vector<int>& getTotalCapacityPerLayer() { return cap_per_layer_; }
  const std::vector<int>& getTotalUsagePerLayer() { return usage_per_layer_; }
  const std::vector<int>& getTotalOverflowPerLayer()
  {
    return overflow_per_layer_;
  }
  const std::vector<int>& getMaxHorizontalOverflows()
  {
    return max_h_overflow_;
  }
  const std::vector<int>& getMaxVerticalOverflows() { return max_v_overflow_; }

  // debug mode functions
  void setDebugOn(std::unique_ptr<AbstractFastRouteRenderer> renderer);
  void setDebugNet(const odb::dbNet* net);
  void setDebugSteinerTree(bool steinerTree);
  void setDebugRectilinearSTree(bool rectiliniarSTree);
  void setDebugTree2D(bool tree2D);
  void setDebugTree3D(bool tree3D);
  void setSttInputFilename(const char* file_name);
  std::string getSttInputFileName();
  const odb::dbNet* getDebugNet();
  bool hasSaveSttInput();

  // NDR related functions
  void clearNDRnets();
  void computeCongestedNDRnets();
  void updateSoftNDRNetUsage(int net_id, int edge_cost);
  void setSoftNDR(int net_id);
  void applySoftNDR(const std::vector<int>& net_ids);

  int x_corner() const { return x_corner_; }
  int y_corner() const { return y_corner_; }
  int tile_size() const { return tile_size_; }

  AbstractFastRouteRenderer* fastrouteRender()
  {
    return debug_->renderer.get();
  }
  void getOverflowPositions(
      std::vector<std::pair<odb::Point, bool>>& overflow_pos);

  NetRouteMap getPlanarRoutes();
  void getPlanarRoute(odb::dbNet* db_net, GRoute& route);
  void get3DRoute(odb::dbNet* db_net, GRoute& route);
  void setIncrementalGrt(bool is_incremental);

 private:
  int getEdgeCapacity(FrNet* net, int x1, int y1, EdgeDirection direction);
  void getNetId(odb::dbNet* db_net, int& net_id, bool& exists);
  void clearNetRoute(int netID);
  void clearNets();
  double dbuToMicrons(int dbu);
  odb::Rect globalRoutingToBox(const GSegment& route);
  NetRouteMap getRoutes();
  void updateSlacks(float percentage = 0.15);
  void preProcessTechLayers();
  odb::dbTechLayer* getTechLayer(int layer, bool is_via);

  // maze functions
  // Maze-routing in different orders
  double getCost(int index, bool is_horizontal, const CostParams& cost_params);
  void mazeRouteMSMD(int iter,
                     int expand,
                     int ripup_threshold,
                     int maze_edge_threshold,
                     bool ordering,
                     int via,
                     int L,
                     const CostParams& cost_params,
                     float& slack_th);
  void convertToMazeroute();
  int getOverflow2D(int* maxOverflow);
  int getOverflow2Dmaze(int* maxOverflow, int* tUsage);
  int getOverflow3D();
  void findNetsNearPosition(std::set<odb::dbNet*>& congestion_nets,
                            const odb::Point& position,
                            bool is_horizontal,
                            int& radius);
  void SaveLastRouteLen();
  void checkAndFixEmbeddedTree(int net_id);
  bool areEdgesOverlapping(int net_id,
                           int edge_id,
                           const std::vector<int>& edges);
  void fixOverlappingEdge(
      int net_id,
      int edge,
      std::vector<std::pair<int16_t, int16_t>>& blocked_positions);
  void routeLShape(const TreeNode& startpoint,
                   const TreeNode& endpoint,
                   std::vector<std::pair<int16_t, int16_t>>& blocked_positions,
                   std::vector<GPoint3D>& new_route);
  void convertToMazerouteNet(int netID);
  void setupHeap(int netID,
                 int edgeID,
                 std::vector<double*>& src_heap,
                 std::vector<double*>& dest_heap,
                 multi_array<double, 2>& d1,
                 multi_array<double, 2>& d2,
                 int regionX1,
                 int regionX2,
                 int regionY1,
                 int regionY2);
  int copyGrids(const std::vector<TreeNode>& treenodes,
                int n1,
                int n2,
                const std::vector<TreeEdge>& treeedges,
                int edge_n1n2,
                std::vector<int>& gridsX_n1n2,
                std::vector<int>& gridsY_n1n2);
  bool updateRouteType1(int net_id,
                        const std::vector<TreeNode>& treenodes,
                        int n1,
                        int A1,
                        int A2,
                        int E1x,
                        int E1y,
                        std::vector<TreeEdge>& treeedges,
                        int edge_n1A1,
                        int edge_n1A2);
  bool updateRouteType2(int net_id,
                        const std::vector<TreeNode>& treenodes,
                        int n1,
                        int A1,
                        int A2,
                        int C1,
                        int C2,
                        int E1x,
                        int E1y,
                        std::vector<TreeEdge>& treeedges,
                        int edge_n1A1,
                        int edge_n1A2,
                        int edge_C1C2);
  void reInitTree(int netID);

  // maze3D functions
  float getMazeRouteCost3D(int net_id,
                           int from_layer,
                           int to_layer,
                           int from_x,
                           int from_y,
                           int to_x,
                           int to_y,
                           bool is_via);
  void mazeRouteMSMDOrder3D(int expand, int ripupTHlb, int ripupTHub);
  void addNeighborPoints(int netID,
                         int n1,
                         int n2,
                         std::vector<int*>& points_heap_3D,
                         multi_array<int, 3>& dist_3D,
                         multi_array<Direction, 3>& directions_3D,
                         multi_array<int, 3>& corr_edge_3D,
                         multi_array<int, 3>& path_len_3D);
  void setupHeap3D(int netID,
                   int edgeID,
                   std::vector<int*>& src_heap_3D,
                   std::vector<int*>& dest_heap_3D,
                   multi_array<Direction, 3>& directions_3D,
                   multi_array<int, 3>& corr_edge_3D,
                   multi_array<int, 3>& d1_3D,
                   multi_array<int, 3>& d2_3D,
                   multi_array<int, 3>& path_len_3D,
                   int regionX1,
                   int regionX2,
                   int regionY1,
                   int regionY2);
  void newUpdateNodeLayers(std::vector<TreeNode>& treenodes,
                           int edgeID,
                           int n1,
                           int lastL);
  int copyGrids3D(std::vector<TreeNode>& treenodes,
                  int n1,
                  int n2,
                  std::vector<TreeEdge>& treeedges,
                  int edge_n1n2,
                  std::vector<GPoint3D>& grids_n1n2);
  void updateRouteType13D(int netID,
                          std::vector<TreeNode>& treenodes,
                          int n1,
                          int A1,
                          int A2,
                          int E1x,
                          int E1y,
                          std::vector<TreeEdge>& treeedges,
                          int edge_n1A1,
                          int edge_n1A2);
  void updateRouteType23D(int netID,
                          std::vector<TreeNode>& treenodes,
                          int n1,
                          int A1,
                          int A2,
                          int C1,
                          int C2,
                          int E1x,
                          int E1y,
                          std::vector<TreeEdge>& treeedges,
                          int edge_n1A1,
                          int edge_n1A2,
                          int edge_C1C2);

  // rsmt functions
  void copyStTree(int ind, const stt::Tree& rsmt);
  void gen_brk_RSMT(bool congestionDriven,
                    bool reRoute,
                    bool genTree,
                    bool newType,
                    bool noADJ);
  void fluteNormal(int netID,
                   const std::vector<int>& x,
                   const std::vector<int>& y,
                   int acc,
                   float coeffV,
                   stt::Tree& t);
  void fluteCongest(int netID,
                    const std::vector<int>& x,
                    const std::vector<int>& y,
                    int acc,
                    float coeffV,
                    stt::Tree& t);
  float coeffADJ(int netID);
  bool HTreeSuite(int netID);
  bool VTreeSuite(int netID);
  bool netCongestion(int netID);

  // route functions
  // old functions for segment list data structure
  void routeSegL(Segment* seg);
  void routeLAll(bool firstTime);
  // new functions for tree data structure
  void newrouteL(int netID, RouteType ripuptype, bool viaGuided);
  void newrouteZ(int netID, int threshold);
  void newrouteZ_edge(int netID, int edgeID);
  void newrouteLAll(bool firstTime, bool viaGuided);
  void newrouteZAll(int threshold);
  void routeMonotonicAll(int threshold, int expand, float logis_cof);
  void spiralRouteAll();
  void newrouteLInMaze(int netID);
  void estimateOneSeg(const Segment* seg);
  void routeSegV(const Segment* seg);
  void routeSegH(const Segment* seg);
  void routeSegLFirstTime(Segment* seg);
  void spiralRoute(int netID, int edgeID);
  void routeMonotonic(int netID,
                      int edgeID,
                      multi_array<double, 2>& d1,
                      multi_array<double, 2>& d2,
                      int threshold,
                      int enlarge);

  // ripup functions
  void ripupSegL(const Segment* seg);
  void newRipup(const TreeEdge* treeedge,
                int x1,
                int y1,
                int x2,
                int y2,
                int netID);

  bool newRipupCheck(const TreeEdge* treeedge,
                     int x1,
                     int y1,
                     int x2,
                     int y2,
                     int ripup_threshold,
                     float critical_slack,
                     int netID,
                     int edgeID);

  bool newRipupCongestedL(const TreeEdge* treeedge,
                          std::vector<TreeNode>& treenodes,
                          int x1,
                          int y1,
                          int x2,
                          int y2,
                          int deg,
                          int netID);
  bool newRipup3DType3(int netID, int edgeID);
  void newRipupNet(int netID);
  void releaseNetResources(int netID);

  // utility functions
  void setTreeNodesVariables(int netID);
  int splitEdge(std::vector<TreeEdge>& treeedges,
                std::vector<TreeNode>& treenodes,
                int n1,
                int n2,
                int edge_n1n2);
  void printEdge(int netID, int edgeID);
  void ConvertToFull3DType2();
  void fillVIA();
  void ensurePinCoverage();
  void getViaStackRange(int netID,
                        int nodeID,
                        int16_t& bot_pin_l,
                        int16_t& top_pin_l);
  int threeDVIA();
  void fixEdgeAssignment(int& net_layer,
                         const multi_array<Edge3D, 3>& edges_3D,
                         int x,
                         int y,
                         int k,
                         int l,
                         bool horizontal,
                         int& best_cost,
                         multi_array<int, 2>& layer_grid,
                         int net_cost);
  void assignEdge(int netID, int edgeID, bool processDIR);
  void recoverEdge(int netID, int edgeID);
  void layerAssignmentV4();
  void netpinOrderInc();
  void checkRoute3D();
  void StNetOrder();
  float CalculatePartialSlack();
  float getNetSlack(odb::dbNet* net);

  // Resistance-aware related functions
  float getWireResistance(int layer, int length, FrNet* net);
  float getViaResistance(int from_layer, int to_layer);
  int getWireCost(int layer, int length, FrNet* net);
  int getViaCost(int from_layer, int to_layer);
  float getNetResistance(FrNet* net, bool assume_layer = false);
  float getResAwareScore(FrNet* net);
  void updateWorstMetrics(FrNet* net);
  void resetWorstMetrics();

  /**
   * @brief Validates the routing of edges for a specified net.
   *
   * Checks various aspects of the routing, including edge lengths,
   * grid positions, and the distances between routing points. Warnings are
   * logged if verbose_ is set.
   *
   * @param netID The ID of the net to be validated.
   *
   * @returns true if any issues are detected in the routing. Else, false.
   */
  bool checkRoute2DTree(int netID);
  void removeLoops();
  void netedgeOrderDec(int netID, std::vector<OrderNetEdge>& net_eo);
  void printTree2D(int netID);
  void printEdge2D(int netID, int edgeID);
  void printEdge3D(int netID, int edgeID);
  void printTree3D(int netID);
  void check2DEdgesUsage();
  /**
   * @brief Compares edges used by routing and usage in h/v edges of the routing
   * graph.
   *
   * Performs a full comparison between edges used by routing and
   * usage in the h/v edges of the routing graph.
   * Somewhat expensive but helpful for finding usage errors.
   */
  void verify2DEdgesUsage();
  /**
   * @brief Compares edges used by routing and usage in h/v edges of the routing
   * graph.
   *
   * Perfmorms a full comparison between edges used by routing and
   * usage in the h/v edges of the routing graph.
   * Somewhat expensive but helpful for finding usage errors.
   */
  void verify3DEdgesUsage();
  void layerAssignment();
  void copyBR();
  void copyRS();
  void freeRR();
  int edgeShift(stt::Tree& t, int net);
  int edgeShiftNew(stt::Tree& t, int net);

  void steinerTreeVisualization(const stt::Tree& stree, FrNet* net);
  void StTreeVisualization(const StTree& stree,
                           FrNet* net,
                           bool is3DVisualization);
  int netCount() const { return nets_.size(); }

  using Tile = std::tuple<int, int, int>;

  static const int BIG_INT = 1e9;  // big integer used as infinity
  static const int HCOST = 5000;

  int max_degree_;
  std::vector<int> cap_per_layer_;
  std::vector<int> usage_per_layer_;
  std::vector<int> overflow_per_layer_;
  std::vector<int> max_h_overflow_;
  std::vector<int> max_v_overflow_;
  odb::dbDatabase* db_;
  int overflow_iterations_;
  int congestion_report_iter_step_;
  std::string congestion_file_name_;
  std::vector<odb::dbTechLayerDir> layer_directions_;
  std::vector<odb::dbTechLayer*> db_layers_;
  int x_range_;
  int y_range_;

  bool en_estimate_parasitics_ = false;
  bool resistance_aware_ = false;
  bool enable_resistance_aware_ = false;
  bool is_3d_step_ = false;
  bool is_incremental_grt_ = false;
  float worst_slack_;
  float worst_net_resistance_;
  int worst_net_length_;
  int worst_fanout_;
  int num_adjust_;
  int v_capacity_;
  int h_capacity_;
  int x_grid_;
  int y_grid_;
  int x_grid_max_;
  int y_grid_max_;
  int x_corner_;
  int y_corner_;
  int tile_size_;
  int enlarge_;
  int costheight_;
  int ahth_;
  int num_layers_;
  int total_overflow_;  // total # overflow
  bool has_2D_overflow_;
  int grid_hv_;
  bool verbose_;
  float critical_nets_percentage_;
  int via_cost_;
  int mazeedge_threshold_;
  float v_capacity_lb_;
  float h_capacity_lb_;
  bool regular_x_;
  bool regular_y_;

  std::vector<int16_t> v_capacity_3D_;
  std::vector<int16_t> h_capacity_3D_;
  std::vector<int16_t> last_col_v_capacity_3D_;
  std::vector<int16_t> last_row_h_capacity_3D_;
  std::vector<double> cost_hvh_;       // Horizontal first Z
  std::vector<double> cost_vhv_;       // Vertical first Z
  std::vector<double> cost_h_;         // Horizontal segment cost
  std::vector<double> cost_v_;         // Vertical segment cost
  std::vector<double> cost_lr_;        // Left and right boundary cost
  std::vector<double> cost_tb_;        // Top and bottom boundary cost
  std::vector<double> cost_hvh_test_;  // Vertical first Z
  std::vector<double> cost_v_test_;    // Vertical segment cost
  std::vector<double> cost_tb_test_;   // Top and bottom boundary cost
  std::vector<double> h_cost_table_;
  std::vector<double> v_cost_table_;
  std::vector<int> xcor_;
  std::vector<int> ycor_;
  std::vector<int> dcor_;

  std::vector<FrNet*> nets_;
  std::unordered_map<odb::dbNet*, int> db_net_id_map_;  // db net -> net id
  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>> gxs_;
  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>> gys_;
  // the copy of vertical sequence for nets, used for second FLUTE
  std::vector<std::vector<int>> gs_;
  std::vector<std::vector<Segment>> seglist_;  // indexed by netID, segID
  std::vector<OrderNetPin> tree_order_pv_;
  std::vector<OrderTree> tree_order_cong_;

  Graph2D graph2d_;
  multi_array<Edge3D, 3> h_edges_3D_;  // The way it is indexed is (Layer, Y, X)
  multi_array<Edge3D, 3> v_edges_3D_;  // The way it is indexed is (Layer, Y, X)
  multi_array<int, 2> corr_edge_;
  multi_array<int16_t, 2> parent_x1_;
  multi_array<int16_t, 2> parent_y1_;
  multi_array<int16_t, 2> parent_x3_;
  multi_array<int16_t, 2> parent_y3_;
  multi_array<bool, 2> hv_;
  multi_array<bool, 2> hyper_v_;
  multi_array<bool, 2> hyper_h_;
  multi_array<bool, 2> in_region_;

  std::vector<StTree> sttrees_;  // the Steiner trees
  std::vector<StTree> sttrees_bk_;

  utl::CallBackHandler* callback_handler_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  sta::dbSta* sta_;

  std::unique_ptr<DebugSetting> debug_;

  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      vertical_blocked_intervals_;
  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      horizontal_blocked_intervals_;

  std::vector<int> net_ids_;

  // Maze 3D variables
  multi_array<Direction, 3> directions_3D_;
  multi_array<int, 3> corr_edge_3D_;
  multi_array<parent3D, 3> pr_3D_;
  std::vector<bool> pop_heap2_3D_;
  std::vector<int*> src_heap_3D_;
  std::vector<int*> dest_heap_3D_;
  multi_array<int, 3> d1_3D_;
  multi_array<int, 3> d2_3D_;
  multi_array<int, 3> path_len_3D_;
  int detour_penalty_;
};

extern const char* getNetName(odb::dbNet* db_net);

}  // namespace grt
