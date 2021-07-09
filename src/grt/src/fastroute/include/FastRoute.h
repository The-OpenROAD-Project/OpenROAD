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

#include "grt/GRoute.h"
#include "DataType.h"

namespace utl {
class Logger;
}

namespace odb{
class dbDatabase;
}

namespace grt {

class FastRouteCore
{
 public:
  FastRouteCore(odb::dbDatabase* db, utl::Logger* log);
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
             float alpha,
             bool is_clock,
             int driver_idx,
             int cost,
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
  std::vector<int> getTotalCapacityPerLayer() { return cap_per_layer; }
  std::vector<int> getTotalUsagePerLayer() { return usage_per_layer; }
  std::vector<int> getTotalOverflowPerLayer() { return overflow_per_layer; }
  std::vector<int> getMaxHorizontalOverflows() { return max_h_overflow; }
  std::vector<int> getMaxVerticalOverflows() { return max_v_overflow; }

 private:
  NetRouteMap getRoutes();
  void init_usage();

  // maze functions
  // Maze-routing in different orders
  void mazeRouteMSMD(int iter,
                            int expand,
                            float height,
                            int ripup_threshold,
                            int mazeedge_Threshold,
                            bool ordering,
                            int cost_type,
                            float LOGIS_COF,
                            int VIA,
                            int slope,
                            int L);
  void convertToMazeroute();
  void updateCongestionHistory(int round, int upType, bool stopDEC, int &max_adj);
  int getOverflow2D(int* maxOverflow);
  int getOverflow2Dmaze(int* maxOverflow, int* tUsage);
  int getOverflow3D(void);
  void str_accu(int rnd);
  void InitLastUsage(int upType);
  void InitEstUsage();
  void convertToMazerouteNet(int netID);
  void setupHeap(int netID,
                 int edgeID,
                 int* heapLen1,
                 int* heapLen2,
                 int regionX1,
                 int regionX2,
                 int regionY1,
                 int regionY2);
  int copyGrids(TreeNode* treenodes,
                int n1,
                int n2,
                TreeEdge* treeedges,
                int edge_n1n2,
                int gridsX_n1n2[],
                int gridsY_n1n2[]);
  void updateRouteType1(TreeNode* treenodes,
                        int n1,
                        int A1,
                        int A2,
                        int E1x,
                        int E1y,
                        TreeEdge* treeedges,
                        int edge_n1A1,
                        int edge_n1A2);
  void updateRouteType2(TreeNode* treenodes,
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
  void reInitTree(int netID);

  // maze3D functions
  void mazeRouteMSMDOrder3D(int expand, int ripupTHlb, int ripupTHub, int layerOrientation);
  void setupHeap3D(int netID,
                   int edgeID,
                   int* heapLen1,
                   int* heapLen2,
                   int regionX1,
                   int regionX2,
                   int regionY1,
                   int regionY2);
  void newUpdateNodeLayers(TreeNode* treenodes, int edgeID, int n1, int lastL);
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
  void copyStTree(int ind, Tree rsmt);
  void gen_brk_RSMT(bool congestionDriven,
                    bool reRoute,
                    bool genTree,
                    bool newType,
                    bool noADJ);
  void fluteNormal(int netID,
                   int d,
                   DTYPE x[],
                   DTYPE y[],
                   int acc,
                   float coeffV,
                   Tree* t);
  void fluteCongest(int netID,
                    int d,
                    DTYPE x[],
                    DTYPE y[],
                    int acc,
                    float coeffV,
                    Tree* t);
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
  void routeLVEnew(int netID, int edgeID, int threshold, int enlarge);

  int maxNetDegree;
  std::vector<int> cap_per_layer;
  std::vector<int> usage_per_layer;
  std::vector<int> overflow_per_layer;
  std::vector<int> max_h_overflow;
  std::vector<int> max_v_overflow;
  odb::dbDatabase* db_;
  bool allow_overflow_;
  int overflow_iterations_;
  int num_nets_;
  int layer_orientation_;
  std::vector<short> vCapacity3D;
  std::vector<short> hCapacity3D;

  float* costHVH;  // Horizontal first Z
  float* costVHV;  // Vertical first Z
  float* costH;    // Horizontal segment cost
  float* costV;    // Vertical segment cost
  float* costLR;   // Left and right boundary cost
  float* costTB;   // Top and bottom boundary cost
  float* costHVHtest;  // Vertical first Z
  float* costVtest;    // Vertical segment cost
  float* costTBtest;   // Top and bottom boundary cost
};

}  // namespace grt
