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

#include <memory>
#include <string>
#include <vector>
#include <spdlog/fmt/fmt.h>

namespace odb {
class dbNet;
}

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

  short x1, y1, x2, y2;  // coordinates of two endpoints
  int netID;             // the netID of the net this segment belonging to
  short Zpoint;          // The coordinates of Z point (x for HVH and y for VHV)
};

struct FrNet  // A Net is a set of connected MazePoints
{
  bool isClock() const { return is_clock_; }
  bool isRouted() const { return is_routed_; }
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
  void setIsRouted(bool is_routed) { is_routed_ = is_routed; }
  void setMaxLayer(int max_layer) { max_layer_ = max_layer; }
  void setMinLayer(int min_layer) { min_layer_ = min_layer; }

 private:
  odb::dbNet* db_net_;
  std::vector<int> pin_x_;  // x coordinates of pins
  std::vector<int> pin_y_;  // y coordinates of pins
  std::vector<int> pin_l_;  // l coordinates of pins
  bool is_clock_;           // flag that indicates if net is a clock net
  int driver_idx_;
  int edge_cost_;
  int min_layer_;
  int max_layer_;
  float slack_;
  // Non-null when an NDR has been applied to the net.
  std::unique_ptr<std::vector<int>> edge_cost_per_layer_;
  bool is_routed_ = false;
};

struct Edge  // An Edge is the routing track holder between two adjacent
             // MazePoints
{
  short congCNT;
  unsigned short cap;    // the capacity of the edge
  unsigned short usage;  // the usage of the edge
  unsigned short red;
  short last_usage;
  float est_usage;  // the estimated usage of the edge

  unsigned short usage_red() const { return usage + red; }
  float est_usage_red() const { return est_usage + red; }
};

struct Edge3D
{
  unsigned short cap;    // the capacity of the edge
  unsigned short usage;  // the usage of the edge
  unsigned short red;
};

struct TreeNode
{
  bool assigned;

  short status;
  short conCNT;
  short botL, topL;
  // heights and eID arrays size were increased after using PD
  // to create the tree topologies.
  static constexpr int max_connections = 10;
  short heights[max_connections];
  int eID[max_connections];

  short x, y;   // position in the grid graph
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
  bool xFirst;

  // valid for ZRoute:
  // true - the route is HVH shape, false - VHV shape
  bool HVH;

  // valid for ZRoute: the position of turn point for Z-shape
  short Zpoint;

  // valid for MazeRoute: a list of grids (n=routelen+1) the route
  // passes, (x1, y1) is the first one, but (x2, y2) is the lastone
  std::vector<short> gridsX;
  std::vector<short> gridsY;
  std::vector<short> gridsL;

  // valid for MazeRoute: the number of edges in the route
  int routelen;
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
  int num_nodes = 0;
  int num_terminals = 0;
  // The nodes (pin and Steiner nodes) in the tree.
  std::unique_ptr<TreeNode[]> nodes;
  std::unique_ptr<TreeEdge[]> edges;

  int num_edges() const { return num_nodes - 1; }
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

}  // namespace grt

#if defined(FMT_VERSION) && FMT_VERSION >= 90000
#include <fmt/ostream.h>
template <> struct fmt::formatter<grt::RouteType> : fmt::ostream_formatter {};
#endif // FMT_VERSION >= 90000
