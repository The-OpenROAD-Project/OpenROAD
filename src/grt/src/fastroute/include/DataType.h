////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "utl/Logger.h"

namespace odb {
class dbNet;
}

using int64 = std::int64_t;

namespace grt {

enum class RouteType
{
  NoRoute,
  LRoute,
  ZRoute,
  MazeRoute
};

std::ostream& operator<<(std::ostream& os, RouteType type);

enum class Direction
{
  North,
  East,
  South,
  West,
  Origin,
  Up,
  Down
};

enum class EdgeDirection
{
  Horizontal,
  Vertical
};

struct Segment  // A Segment is a 2-pin connection
{
  bool xFirst;  // route x-direction first (only for L route)
  bool HVH;     // TRUE = HVH or false = VHV (only for Z route)

  int16_t x1, y1, x2, y2;  // coordinates of two endpoints
  int netID;               // the netID of the net this segment belonging to
  int16_t Zpoint;  // The coordinates of Z point (x for HVH and y for VHV)
};

struct FrNet  // A Net is a set of connected MazePoints
{
  bool isClock() const { return is_clock_; }
  bool isCritical() { return is_critical_; }
  float getSlack() const { return slack_; }
  odb::dbNet* getDbNet() const { return db_net_; }
  int getDriverIdx() const { return driver_idx_; }
  int getEdgeCost() const { return edge_cost_; }
  const char* getName() const;
  int getMaxLayer() const { return max_layer_; }
  int getMinLayer() const { return min_layer_; }
  int getNumPins() const { return pin_x_.size(); }
  int getLayerEdgeCost(int layer) const;

  int getPinX(int idx) const { return pin_x_[idx]; }
  int getPinY(int idx) const { return pin_y_[idx]; }
  int getPinL(int idx) const { return pin_l_[idx]; }
  const std::vector<int>& getPinX() const { return pin_x_; }
  const std::vector<int>& getPinY() const { return pin_y_; }
  const std::vector<int>& getPinL() const { return pin_l_; }

  void addPin(int x, int y, int layer);
  void reset(odb::dbNet* db_net,
             bool is_clock,
             int driver_idx,
             int edge_cost,
             int min_layer,
             int max_layer,
             float slack,
             std::vector<int>* edge_cost_per_layer);
  void setMaxLayer(int max_layer) { max_layer_ = max_layer; }
  void setMinLayer(int min_layer) { min_layer_ = min_layer; }
  void setSlack(float slack) { slack_ = slack; }
  void setIsCritical(bool is_critical) { is_critical_ = is_critical; }

 private:
  odb::dbNet* db_net_;
  std::vector<int> pin_x_;  // x coordinates of pins
  std::vector<int> pin_y_;  // y coordinates of pins
  std::vector<int> pin_l_;  // l coordinates of pins
  bool is_clock_;           // flag that indicates if net is a clock net
  bool is_critical_;
  int driver_idx_;
  int edge_cost_;
  int min_layer_;
  int max_layer_;
  float slack_;
  // Non-null when an NDR has been applied to the net.
  std::unique_ptr<std::vector<int>> edge_cost_per_layer_;
};

struct Edge  // An Edge is the routing track holder between two adjacent
             // MazePoints
{
  int16_t congCNT;
  uint16_t cap;    // the capacity of the edge
  uint16_t usage;  // the usage of the edge
  uint16_t red;
  int16_t last_usage;
  float est_usage;  // the estimated usage of the edge

  uint16_t usage_red() const { return usage + red; }
  float est_usage_red() const { return est_usage + red; }
};

struct Edge3D
{
  uint16_t cap;    // the capacity of the edge
  uint16_t usage;  // the usage of the edge
  uint16_t red;    // the reduction of capacity of the edge
};

struct TreeNode
{
  bool assigned;

  int16_t status = 0;
  int16_t conCNT;
  int16_t botL, topL;
  // heights and eID arrays size were increased after using PD
  // to create the tree topologies.
  static constexpr int max_connections = 10;
  int16_t heights[max_connections];
  int eID[max_connections];

  int16_t x, y;  // position in the grid graph
  int nbr_count = 0;
  int nbr[3];   // three neighbors
  int edge[3];  // three adjacent edges
  int hID;
  int lID;
  // If two nodes are at the same x & y then the duplicate will have
  // stackAlias set to the index of the first node.  This does not
  // apply to pins nodes, only Steiner nodes.
  int stackAlias;
};

struct Route
{
  RouteType type;  // type of route: LRoute, ZRoute, MazeRoute

  // valid for LRoute:
  // true - the route is horizontal first (x1, y1) - (x2, y1) - (x2, y2),
  // false (x1, y1) - (x1, y2) - (x2, y2)
  bool xFirst = false;

  // valid for ZRoute:
  // true - the route is HVH shape, false - VHV shape
  bool HVH;

  // valid for ZRoute: the position of turn point for Z-shape
  int16_t Zpoint;

  // valid for MazeRoute: a list of grids (n=routelen+1) the route
  // passes, (x1, y1) is the first one, but (x2, y2) is the lastone
  std::vector<int16_t> gridsX;
  std::vector<int16_t> gridsY;
  std::vector<int16_t> gridsL;

  // valid for MazeRoute: the number of edges in the route
  int routelen;

  int last_routelen = 0;  // the last routelen before overflow itter
};

struct TreeEdge
{
  bool assigned;

  int len;  // the Manhanttan Distance for two end nodes
  int n1, n1a;
  int n2, n2a;
  Route route;
};

struct StTree
{
  int num_terminals = 0;
  // The nodes (pin and Steiner nodes) in the tree.
  std::vector<TreeNode> nodes;
  std::vector<TreeEdge> edges;

  int num_edges() const { return edges.size(); }
  int num_nodes() const { return nodes.size(); }
};

struct OrderNetPin
{
  int treeIndex;
  int minX;
  float npv;  // net length over pin
};

struct OrderTree
{
  int length;
  int treeIndex;
  int xmin;
};

struct OrderNetEdge
{
  int length;
  int edgeID;
};

using utl::format_as;

}  // namespace grt
