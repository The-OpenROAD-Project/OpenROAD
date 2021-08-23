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

#include <map>
#include <string>
#include <vector>

#include "DataType.h"
#include "boost/multi_array.hpp"
#include "grt/GRoute.h"
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

using boost::multi_array;

namespace grt {

using stt::Tree;

class FastRouteCore
{
 public:
  FastRouteCore(odb::dbDatabase* db,
                utl::Logger* log,
                stt::SteinerTreeBuilder* stt_builder);
  ~FastRouteCore();

  void deleteComponents();
  void clear();
  void setGridsAndLayers(int x, int y, int nLayers);
  void addVCapacity(short verticalCapacity, int layer);
  void addHCapacity(short horizontalCapacity, int layer);
  void addMinWidth(int width, int layer);
  void addMinSpacing(int spacing, int layer);
  void addViaSpacing(int spacing, int layer);
  void setNumberNets(int nNets);
  void setLowerLeft(int x, int y);
  void setTileSize(int width, int height);
  void setLayerOrientation(int x);
  void addPin(int netID, int x, int y, int layer);
  int addNet(odb::dbNet* db_net,
             int num_pins,
             bool is_clock,
             int driver_idx,
             int cost,
             int min_layer,
             int max_layer,
             std::vector<int> edge_cost_per_layer);
  void initEdges();
  void setNumAdjustments(int nAdjustements);
  void addAdjustment(long x1,
                     long y1,
                     int l1,
                     long x2,
                     long y2,
                     int l2,
                     int reducedCap,
                     bool isReduce);
  void initAuxVar();
  NetRouteMap run();
  void updateDbCongestion();

  int getEdgeCapacity(long x1, long y1, int l1, long x2, long y2, int l2);
  int getEdgeCapacity(FrNet* net,
                      int x1,
                      int y1,
                      EdgeDirection direction);
  int getEdgeCurrentResource(long x1,
                             long y1,
                             int l1,
                             long x2,
                             long y2,
                             int l2);
  int getEdgeCurrentUsage(long x1, long y1, int l1, long x2, long y2, int l2);
  void setEdgeUsage(long x1,
                    long y1,
                    int l1,
                    long x2,
                    long y2,
                    int l2,
                    int newUsage);
  void setEdgeCapacity(long x1,
                       long y1,
                       int l1,
                       long x2,
                       long y2,
                       int l2,
                       int newCap);
  void setMaxNetDegree(int);
  void setVerbose(int v);
  void setOverflowIterations(int iterations);
  void setAllowOverflow(bool allow);
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

 private:
  NetRouteMap getRoutes();
  void init_usage();

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
  void updateRouteType1(const TreeNode* treenodes,
                        const int n1,
                        const int A1,
                        const int A2,
                        const int E1x,
                        const int E1y,
                        TreeEdge* treeedges,
                        const int edge_n1A1,
                        const int edge_n1A2);
  void updateRouteType2(const TreeNode* treenodes,
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
                const TreeNode* treenodes,
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

  // utility functions
  void printEdge(const int netID, const int edgeID);
  void ConvertToFull3DType2();
  void fillVIA();
  int threeDVIA();
  void assignEdge(int netID, int edgeID, bool processDIR);
  void recoverEdge(int netID, int edgeID);
  void newLayerAssignmentV4();
  void netpinOrderInc();
  void checkRoute3D();
  void StNetOrder();
  bool checkRoute2DTree(int netID);
  void checkUsage();
  void netedgeOrderDec(int netID);
  void printTree2D(int netID);
  void printEdge2D(int netID, int edgeID);
  void printEdge3D(int netID, int edgeID);
  void printTree3D(int netID);
  void check2DEdgesUsage();
  void newLA();
  void copyBR(void);
  void copyRS(void);
  void freeRR(void);
  int edgeShift(Tree& t, int net);
  int edgeShiftNew(Tree& t, int net);

  static const int MAXLEN = 20000;
  static const int BIG_INT = 1e7;  // big integer used as infinity
  static const int HCOST = 5000;

  int max_degree_;
  std::vector<int> cap_per_layer_;
  std::vector<int> usage_per_layer_;
  std::vector<int> overflow_per_layer_;
  std::vector<int> max_h_overflow_;
  std::vector<int> max_v_overflow_;
  odb::dbDatabase* db_;
  bool allow_overflow_;
  int overflow_iterations_;
  int num_nets_;
  int layer_orientation_;
  int x_range_;
  int y_range_;

  int new_net_id_;
  int seg_count_;
  int pin_ind_;
  int num_adjust_;
  int v_capacity_;
  int h_capacity_;
  int x_grid_;
  int y_grid_;
  int x_corner_;
  int y_corner_;
  int w_tile_;
  int h_tile_;
  int enlarge_;
  int costheight_;
  int ahth_;
  int num_valid_nets_;  // # nets need to be routed (having pins in different
                        // grids)
  int num_layers_;
  int total_overflow_;  // total # overflow
  int grid_hv_;
  int verbose_;
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
  std::vector<int> seglist_index_;  // the index for the segments for each net
  std::vector<int> seglist_cnt_;    // the number of segements for each net

  std::vector<FrNet*> nets_;
  std::vector<OrderNetEdge> net_eo_;
  std::vector<std::vector<int>>
      gxs_;  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>>
      gys_;  // the copy of xs for nets, used for second FLUTE
  std::vector<std::vector<int>>
      gs_;  // the copy of vertical sequence for nets, used for second FLUTE
  std::vector<Segment> seglist_;
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
};

}  // namespace grt
