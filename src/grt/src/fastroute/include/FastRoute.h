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
#include <unordered_map>
#include <vector>

#include "DataType.h"
#include "grt/GRoute.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}

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

class FastRouteRenderer;

// Debug mode settings
struct DebugSetting
{
  const odb::dbNet* net_;
  bool steinerTree_;
  bool rectilinearSTree_;
  bool tree2D_;
  bool tree3D_;
  bool isOn_;
  std::string sttInputFileName_;
  DebugSetting()
      : net_(nullptr),
        steinerTree_(false),
        rectilinearSTree_(false),
        tree2D_(false),
        tree3D_(false),
        isOn_(false),
        sttInputFileName_("")
  {
  }
};

using stt::Tree;

typedef std::pair<int, int> TileCongestion;

class FastRouteCore
{
 public:
  FastRouteCore(odb::dbDatabase* db,
                utl::Logger* log,
                stt::SteinerTreeBuilder* stt_builder,
                gui::Gui* gui);
  ~FastRouteCore();

  void clear();
  void setGridsAndLayers(int x, int y, int nLayers);
  void addVCapacity(short verticalCapacity, int layer);
  void addHCapacity(short horizontalCapacity, int layer);
  void setLowerLeft(int x, int y);
  void setTileSize(int size);
  void setLayerOrientation(int x);
  FrNet* addNet(odb::dbNet* db_net,
                bool is_clock,
                int driver_idx,
                int cost,
                int min_layer,
                int max_layer,
                float slack,
                std::vector<int>* edge_cost_per_layer);
  void initEdges();
  void setNumAdjustments(int nAdjustements);
  void addAdjustment(int x1,
                     int y1,
                     int x2,
                     int y2,
                     int layer,
                     int reducedCap,
                     bool isReduce);
  void applyVerticalAdjustments(const odb::Point& first_tile,
                                const odb::Point& last_tile,
                                int layer,
                                int first_tile_reduce,
                                int last_tile_reduce);
  void applyHorizontalAdjustments(const odb::Point& first_tile,
                                  const odb::Point& last_tile,
                                  int layer,
                                  int first_tile_reduce,
                                  int last_tile_reduce);
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
  void updateDbCongestion();
  void getCongestionGrid(
      std::vector<std::pair<GSegment, TileCongestion>>& congestionGridV,
      std::vector<std::pair<GSegment, TileCongestion>>& congestionGridH);

  const std::vector<short>& getVerticalCapacities() { return v_capacity_3D_; }
  const std::vector<short>& getHorizontalCapacities() { return h_capacity_3D_; }
  int getEdgeCapacity(int x1, int y1, int x2, int y2, int layer);
  const multi_array<Edge3D, 3>& getHorizontalEdges3D() { return h_edges_3D_; }
  const multi_array<Edge3D, 3>& getVerticalEdges3D() { return v_edges_3D_; }
  void incrementEdge3DUsage(int x1, int y1, int x2, int y2, int layer);
  void setMaxNetDegree(int);
  void setVerbose(bool v);
  void setOverflowIterations(int iterations);
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
  void setDebugOn(bool isOn);
  void setDebugNet(const odb::dbNet* net);
  void setDebugSteinerTree(bool steinerTree);
  void setDebugRectilinearSTree(bool rectiliniarSTree);
  void setDebugTree2D(bool tree2D);
  void setDebugTree3D(bool tree3D);
  void setSttInputFilename(const char* file_name);
  std::string getSttInputFileName();
  const odb::dbNet* getDebugNet();
  bool hasSaveSttInput();

 private:
  int getEdgeCapacity(FrNet* net, int x1, int y1, EdgeDirection direction);
  void getNetId(odb::dbNet* db_net, int& net_id, bool& exists);
  void clearNetRoute(const int netID);
  void initNetAuxVars();
  void clearNets();
  NetRouteMap getRoutes();

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
                     const int L);
  void convertToMazeroute();
  void updateCongestionHistory(const int upType, bool stopDEC, int& max_adj);
  int getOverflow2D(int* maxOverflow);
  int getOverflow2Dmaze(int* maxOverflow, int* tUsage);
  int getOverflow3D();
  void str_accu(const int rnd);
  void InitLastUsage(const int upType);
  void InitEstUsage();
  void fixEmbeddedTrees();
  void checkAndFixEmbeddedTree(const int net_id);
  bool areEdgesOverlapping(const int net_id,
                           const int edge_id,
                           const std::vector<int>& edges);
  void fixOverlappingEdge(
      const int net_id,
      const int edge,
      std::vector<std::pair<short, short>>& blocked_positions);
  void bendEdge(TreeEdge* treeedge,
                TreeNode* treenodes,
                std::vector<short>& new_route_x,
                std::vector<short>& new_route_y,
                std::vector<std::pair<short, short>>& blocked_positions);
  void routeLShape(const TreeNode& startpoint,
                   const TreeNode& endpoint,
                   std::vector<std::pair<short, short>>& blocked_positions,
                   std::vector<short>& new_route_x,
                   std::vector<short>& new_route_y);
  void convertToMazerouteNet(const int netID);
  void setupHeap(const int netID,
                 const int edgeID,
                 std::vector<float*>& src_heap,
                 std::vector<float*>& dest_heap,
                 multi_array<float, 2>& d1,
                 multi_array<float, 2>& d2,
                 const int regionX1,
                 const int regionX2,
                 const int regionY1,
                 const int regionY2);
  int copyGrids(const TreeNode* treenodes,
                const int n1,
                const int n2,
                const TreeEdge* treeedges,
                const int edge_n1n2,
                std::vector<int>& gridsX_n1n2,
                std::vector<int>& gridsY_n1n2);
  bool updateRouteType1(const int net_id,
                        const TreeNode* treenodes,
                        const int n1,
                        const int A1,
                        const int A2,
                        const int E1x,
                        const int E1y,
                        TreeEdge* treeedges,
                        const int edge_n1A1,
                        const int edge_n1A2);
  bool updateRouteType2(const int net_id,
                        const TreeNode* treenodes,
                        const int n1,
                        const int A1,
                        const int A2,
                        const int C1,
                        const int C2,
                        const int E1x,
                        const int E1y,
                        TreeEdge* treeedges,
                        const int edge_n1A1,
                        const int edge_n1A2,
                        const int edge_C1C2);
  void reInitTree(const int netID);

  // maze3D functions
  void mazeRouteMSMDOrder3D(int expand,
                            int ripupTHlb,
                            int ripupTHub,
                            int layerOrientation);
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
  void newUpdateNodeLayers(TreeNode* treenodes,
                           const int edgeID,
                           const int n1,
                           const int lastL);
  int copyGrids3D(TreeNode* treenodes,
                  int n1,
                  int n2,
                  TreeEdge* treeedges,
                  int edge_n1n2,
                  int gridsX_n1n2[],
                  int gridsY_n1n2[],
                  int gridsL_n1n2[]);
  void updateRouteType13D(int netID,
                          TreeNode* treenodes,
                          int n1,
                          int A1,
                          int A2,
                          int E1x,
                          int E1y,
                          TreeEdge* treeedges,
                          int edge_n1A1,
                          int edge_n1A2);
  void updateRouteType23D(int netID,
                          TreeNode* treenodes,
                          int n1,
                          int A1,
                          int A2,
                          int C1,
                          int C2,
                          int E1x,
                          int E1y,
                          TreeEdge* treeedges,
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
  void routeMonotonicAll(int threshold);
  void routeMonotonic(int netID, int edgeID, int threshold);
  void routeLVAll(int threshold, int expand, float logis_cof);
  void spiralRouteAll();
  void newrouteLInMaze(int netID);
  void estimateOneSeg(Segment* seg);
  void routeSegV(Segment* seg);
  void routeSegH(Segment* seg);
  void routeSegLFirstTime(Segment* seg);
  void spiralRoute(int netID, int edgeID);
  void routeLVEnew(int netID,
                   int edgeID,
                   multi_array<float, 2>& d1,
                   multi_array<float, 2>& d2,
                   int threshold,
                   int enlarge);

  // ripup functions
  void ripupSegL(const Segment* seg);
  void ripupSegZ(const Segment* seg);
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
                     const int netID,
                     const int edgeID);

  bool newRipupType2(const TreeEdge* treeedge,
                     TreeNode* treenodes,
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
  void printEdge(const int netID, const int edgeID);
  void ConvertToFull3DType2();
  void fillVIA();
  int threeDVIA();
  void fixEdgeAssignment(int& net_layer,
                         multi_array<Edge3D, 3>& edges_3D,
                         int x,
                         int y,
                         int k,
                         int l,
                         bool horizontal,
                         int& best_cost);
  void assignEdge(int netID, int edgeID, bool processDIR);
  void recoverEdge(int netID, int edgeID);
  void layerAssignmentV4();
  void netpinOrderInc();
  void checkRoute3D();
  void StNetOrder();
  bool checkRoute2DTree(int netID);
  void removeLoops();
  void netedgeOrderDec(int netID);
  void printTree2D(int netID);
  void printEdge2D(int netID, int edgeID);
  void printEdge3D(int netID, int edgeID);
  void printTree3D(int netID);
  void check2DEdgesUsage();
  void verify2DEdgesUsage();
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

  static const int MAXLEN = 20000;
  static const int BIG_INT = 1e9;  // big integer used as infinity
  static const int HCOST = 5000;

  int max_degree_;
  std::vector<int> cap_per_layer_;
  std::vector<int> usage_per_layer_;
  std::vector<int> overflow_per_layer_;
  std::vector<int> max_h_overflow_;
  std::vector<int> max_v_overflow_;
  odb::dbDatabase* db_;
  gui::Gui* gui_;
  int overflow_iterations_;
  int layer_orientation_;
  int x_range_;
  int y_range_;

  int num_adjust_;
  int v_capacity_;
  int h_capacity_;
  int x_grid_;
  int y_grid_;
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
  int via_cost_;
  int mazeedge_threshold_;
  float v_capacity_lb_;
  float h_capacity_lb_;

  std::vector<short> v_capacity_3D_;
  std::vector<short> h_capacity_3D_;
  std::vector<float> cost_hvh_;       // Horizontal first Z
  std::vector<float> cost_vhv_;       // Vertical first Z
  std::vector<float> cost_h_;         // Horizontal segment cost
  std::vector<float> cost_v_;         // Vertical segment cost
  std::vector<float> cost_lr_;        // Left and right boundary cost
  std::vector<float> cost_tb_;        // Top and bottom boundary cost
  std::vector<float> cost_hvh_test_;  // Vertical first Z
  std::vector<float> cost_v_test_;    // Vertical segment cost
  std::vector<float> cost_tb_test_;   // Top and bottom boundary cost
  std::vector<float> h_cost_table_;
  std::vector<float> v_cost_table_;
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
  multi_array<int, 2> layer_grid_;
  multi_array<int, 2> via_link_;
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

  FastRouteRenderer* fastrouteRender_;
  std::unique_ptr<DebugSetting> debug_;

  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      vertical_blocked_intervals_;
  std::unordered_map<Tile, interval_set<int>, boost::hash<Tile>>
      horizontal_blocked_intervals_;
};

}  // namespace grt
