////////////////////////////////////////////////////////////////////////////////
// Authors: Vitor Bandeira, Eder Matheus Monteiro e Isadora Oliveira
//          (Advisor: Ricardo Reis)
//
// BSD 3-Clause License
//
// Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <boost/functional/hash.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/multi_array.hpp>
#include <set>
#include <unordered_map>
#include <vector>

#include "AbstractMakeWireParasitics.h"
#include "DataType.h"
#include "grt/GRoute.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"

namespace utl {
class Logger;
}

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

using boost::multi_array;
using boost::icl::interval;
using boost::icl::interval_set;

namespace grt {

class AbstractFastRouteRenderer;
class MakeWireParasitics;

// Debug mode settings
struct DebugSetting
{
  const odb::dbNet* net_ = nullptr;
  bool steinerTree_ = false;
  bool rectilinearSTree_ = false;
  bool tree2D_ = false;
  bool tree3D_ = false;
  std::unique_ptr<AbstractFastRouteRenderer> renderer_;
  std::string sttInputFileName_;

  bool isOn() const { return renderer_ != nullptr; }
};

using stt::Tree;

struct parent3D
{
  int16_t layer;
  int x, y;
};

class FastRouteCore
{
 public:
  FastRouteCore(odb::dbDatabase* db,
                utl::Logger* log,
                stt::SteinerTreeBuilder* stt_builder);
  ~FastRouteCore();

  void clear();
  void saveCongestion(int iter = -1);
  void setGridsAndLayers(int x, int y, int nLayers);
  void addVCapacity(short verticalCapacity, int layer);
  void addHCapacity(short horizontalCapacity, int layer);
  void setLowerLeft(int x, int y);
  void setTileSize(int size);
  void addLayerDirection(int layer_idx, const odb::dbTechLayerDir& direction);
  FrNet* addNet(odb::dbNet* db_net,
                bool is_clock,
                int driver_idx,
                int cost,
                int min_layer,
                int max_layer,
                float slack,
                std::vector<int>* edge_cost_per_layer);
  void deleteNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);
  void mergeNet(odb::dbNet* db_net);
  void clearNetRoute(odb::dbNet* db_net);
  void initEdges();
  void setNumAdjustments(int nAdjustements);
  void addAdjustment(int x1,
                     int y1,
                     int x2,
                     int y2,
                     int layer,
                     uint16_t reducedCap,
                     bool isReduce);
  void addVerticalAdjustments(
      const odb::Point& first_tile,
      const odb::Point& last_tile,
      const int layer,
      const interval<int>::type& first_tile_reduce_interval,
      const interval<int>::type& last_tile_reduce_interval);
  void addHorizontalAdjustments(
      const odb::Point& first_tile,
      const odb::Point& last_tile,
      const int layer,
      const interval<int>::type& first_tile_reduce_interval,
      const interval<int>::type& last_tile_reduce_interval);
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

  const std::vector<short>& getVerticalCapacities() { return v_capacity_3D_; }
  const std::vector<short>& getHorizontalCapacities() { return h_capacity_3D_; }
  int getEdgeCapacity(int x1, int y1, int x2, int y2, int layer);
  const multi_array<Edge3D, 3>& getHorizontalEdges3D() { return h_edges_3D_; }
  const multi_array<Edge3D, 3>& getVerticalEdges3D() { return v_edges_3D_; }
  void setLastColVCapacity(short cap, int layer)
  {
    last_col_v_capacity_3D_[layer] = cap;
  }
  void setLastRowHCapacity(short cap, int layer)
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
  void setMaxNetDegree(int);
  void setVerbose(bool v);
  void setCriticalNetsPercentage(float u);
  float getCriticalNetsPercentage() { return critical_nets_percentage_; };
  void setMakeWireParasiticsBuilder(AbstractMakeWireParasitics* builder);
  void setOverflowIterations(int iterations);
  void setCongestionReportIterStep(int congestion_report_iter_step);
  void setCongestionReportFile(const char* congestion_file_name);
  void setGridMax(int x_max, int y_max);
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

  int x_corner() const { return x_corner_; }
  int y_corner() const { return y_corner_; }
  int tile_size() const { return tile_size_; }

  AbstractFastRouteRenderer* fastrouteRender()
  {
    return debug_->renderer_.get();
  }

 private:
  int getEdgeCapacity(FrNet* net, int x1, int y1, EdgeDirection direction);
  void getNetId(odb::dbNet* db_net, int& net_id, bool& exists);
  void clearNetRoute(int netID);
  void clearNets();
  double dbuToMicrons(int dbu);
  odb::Rect globalRoutingToBox(const GSegment& route);
  NetRouteMap getRoutes();
  NetRouteMap getPlanarRoutes();

  // maze functions
  // Maze-routing in different orders
  void mazeRouteMSMD(const int iter,
                     const int expand,
                     const float cost_height,
                     const int ripup_threshold,
                     const int maze_edge_threshold,
                     const bool ordering,
                     const int cost_type,
                     const float logis_cof,
                     const int via,
                     const int slope,
                     const int L,
                     float& slack_th);
  void convertToMazeroute();
  void updateCongestionHistory(int up_type, bool stop_decreasing, int& max_adj);
  int getOverflow2D(int* maxOverflow);
  int getOverflow2Dmaze(int* maxOverflow, int* tUsage);
  int getOverflow3D();
  void setCongestionNets(std::set<odb::dbNet*>& congestion_nets,
                         int& posX,
                         int& posY,
                         int dir,
                         int& radius);
  void str_accu(int rnd);
  void InitLastUsage(int upType);
  void InitEstUsage();
  void SaveLastRouteLen();
  void checkAndFixEmbeddedTree(const int net_id);
  bool areEdgesOverlapping(const int net_id,
                           const int edge_id,
                           const std::vector<int>& edges);
  void fixOverlappingEdge(
      const int net_id,
      int edge,
      std::vector<std::pair<int16_t, int16_t>>& blocked_positions);
  void routeLShape(const TreeNode& startpoint,
                   const TreeNode& endpoint,
                   std::vector<std::pair<short, short>>& blocked_positions,
                   std::vector<short>& new_route_x,
                   std::vector<short>& new_route_y);
  void convertToMazerouteNet(const int netID);
  void setupHeap(const int netID,
                 const int edgeID,
                 std::vector<double*>& src_heap,
                 std::vector<double*>& dest_heap,
                 multi_array<double, 2>& d1,
                 multi_array<double, 2>& d2,
                 const int regionX1,
                 const int regionX2,
                 const int regionY1,
                 const int regionY2);
  int copyGrids(const std::vector<TreeNode>& treenodes,
                const int n1,
                const int n2,
                const std::vector<TreeEdge>& treeedges,
                const int edge_n1n2,
                std::vector<int>& gridsX_n1n2,
                std::vector<int>& gridsY_n1n2);
  bool updateRouteType1(const int net_id,
                        const std::vector<TreeNode>& treenodes,
                        const int n1,
                        const int A1,
                        const int A2,
                        const int E1x,
                        const int E1y,
                        std::vector<TreeEdge>& treeedges,
                        const int edge_n1A1,
                        const int edge_n1A2);
  bool updateRouteType2(const int net_id,
                        const std::vector<TreeNode>& treenodes,
                        const int n1,
                        const int A1,
                        const int A2,
                        const int C1,
                        const int C2,
                        const int E1x,
                        const int E1y,
                        std::vector<TreeEdge>& treeedges,
                        const int edge_n1A1,
                        const int edge_n1A2,
                        const int edge_C1C2);
  void reInitTree(const int netID);

  // maze3D functions
  void mazeRouteMSMDOrder3D(int expand, int ripupTHlb, int ripupTHub);
  void addNeighborPoints(int netID,
                         int n1,
                         int n2,
                         std::vector<int*>& points_heap_3D,
                         multi_array<int, 3>& dist_3D,
                         multi_array<Direction, 3>& directions_3D,
                         multi_array<int, 3>& corr_edge_3D);
  void setupHeap3D(int netID,
                   int edgeID,
                   std::vector<int*>& src_heap_3D,
                   std::vector<int*>& dest_heap_3D,
                   multi_array<Direction, 3>& directions_3D,
                   multi_array<int, 3>& corr_edge_3D,
                   multi_array<int, 3>& d1_3D,
                   multi_array<int, 3>& d2_3D,
                   int regionX1,
                   int regionX2,
                   int regionY1,
                   int regionY2);
  void newUpdateNodeLayers(std::vector<TreeNode>& treenodes,
                           const int edgeID,
                           const int n1,
                           const int lastL);
  int copyGrids3D(std::vector<TreeNode>& treenodes,
                  int n1,
                  int n2,
                  std::vector<TreeEdge>& treeedges,
                  int edge_n1n2,
                  std::vector<int>& gridsX_n1n2,
                  std::vector<int>& gridsY_n1n2,
                  std::vector<int>& gridsL_n1n2);
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
  void copyStTree(const int ind, const Tree& rsmt);
  void gen_brk_RSMT(const bool congestionDriven,
                    const bool reRoute,
                    const bool genTree,
                    const bool newType,
                    const bool noADJ);
  void fluteNormal(const int netID,
                   const std::vector<int>& x,
                   const std::vector<int>& y,
                   const int acc,
                   const float coeffV,
                   Tree& t);
  void fluteCongest(const int netID,
                    const std::vector<int>& x,
                    const std::vector<int>& y,
                    const int acc,
                    const float coeffV,
                    Tree& t);
  float coeffADJ(const int netID);
  bool HTreeSuite(const int netID);
  bool VTreeSuite(const int netID);
  bool netCongestion(const int netID);

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
  void estimateOneSeg(Segment* seg);
  void routeSegV(Segment* seg);
  void routeSegH(Segment* seg);
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
                const int x1,
                const int y1,
                const int x2,
                const int y2,
                const int netID);

  bool newRipupCheck(const TreeEdge* treeedge,
                     const int x1,
                     const int y1,
                     const int x2,
                     const int y2,
                     const int ripup_threshold,
                     const float critical_slack,
                     const int netID,
                     const int edgeID);

  bool newRipupType2(const TreeEdge* treeedge,
                     std::vector<TreeNode>& treenodes,
                     const int x1,
                     const int y1,
                     const int x2,
                     const int y2,
                     const int deg,
                     const int netID);
  bool newRipup3DType3(const int netID, const int edgeID);
  void newRipupNet(const int netID);
  void releaseNetResources(const int netID);

  // utility functions
  void setTreeNodesVariables(int netID);
  int splitEdge(std::vector<TreeEdge>& treeedges,
                std::vector<TreeNode>& treenodes,
                int n1,
                int n2,
                int edge_n1n2);
  void printEdge(const int netID, const int edgeID);
  void ConvertToFull3DType2();
  void fillVIA();
  void getViaStackRange(int netID,
                        int nodeID,
                        int16_t& bot_pin_l,
                        int16_t& top_pin_l);
  int threeDVIA();
  void fixEdgeAssignment(int& net_layer,
                         multi_array<Edge3D, 3>& edges_3D,
                         int x,
                         int y,
                         int k,
                         int l,
                         bool horizontal,
                         int& best_cost,
                         multi_array<int, 2>& layer_grid);
  void assignEdge(int netID, int edgeID, bool processDIR);
  void recoverEdge(int netID, int edgeID);
  void layerAssignmentV4();
  void netpinOrderInc();
  void checkRoute3D();
  void StNetOrder();
  float CalculatePartialSlack();
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
  void netedgeOrderDec(int netID);
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
  void copyBR(void);
  void copyRS(void);
  void freeRR(void);
  int edgeShift(Tree& t, int net);
  int edgeShiftNew(Tree& t, int net);

  void steinerTreeVisualization(const stt::Tree& stree, FrNet* net);
  void StTreeVisualization(const StTree& stree,
                           FrNet* net,
                           bool is3DVisualization);
  int netCount() const { return nets_.size(); }

  typedef std::tuple<int, int, int> Tile;

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
  int x_range_;
  int y_range_;

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
  std::vector<int> route_net_ids_;  // IDs of nets to route
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

  std::vector<short> v_capacity_3D_;
  std::vector<short> h_capacity_3D_;
  std::vector<short> last_col_v_capacity_3D_;
  std::vector<short> last_row_h_capacity_3D_;
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
  std::vector<OrderNetEdge> net_eo_;
  std::vector<std::vector<int>>
      gxs_;  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>>
      gys_;  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>>
      gs_;  // the copy of vertical sequence for nets, used for second FLUTE
  std::vector<std::vector<Segment>> seglist_;  // indexed by netID, segID
  std::vector<OrderNetPin> tree_order_pv_;
  std::vector<OrderTree> tree_order_cong_;

  multi_array<Edge, 2> v_edges_;       // The way it is indexed is (Y, X)
  multi_array<Edge, 2> h_edges_;       // The way it is indexed is (Y, X)
  multi_array<Edge3D, 3> h_edges_3D_;  // The way it is indexed is (Layer, Y, X)
  multi_array<Edge3D, 3> v_edges_3D_;  // The way it is indexed is (Layer, Y, X)
  multi_array<int, 2> corr_edge_;
  multi_array<short, 2> parent_x1_;
  multi_array<short, 2> parent_y1_;
  multi_array<short, 2> parent_x3_;
  multi_array<short, 2> parent_y3_;
  multi_array<bool, 2> hv_;
  multi_array<bool, 2> hyper_v_;
  multi_array<bool, 2> hyper_h_;
  multi_array<bool, 2> in_region_;

  std::vector<StTree> sttrees_;  // the Steiner trees
  std::vector<StTree> sttrees_bk_;

  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  AbstractMakeWireParasitics* parasitics_builder_;

  std::unique_ptr<DebugSetting> debug_;

  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      vertical_blocked_intervals_;
  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      horizontal_blocked_intervals_;

  std::set<std::pair<int, int>> h_used_ggrid_;
  std::set<std::pair<int, int>> v_used_ggrid_;
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
};

}  // namespace grt
