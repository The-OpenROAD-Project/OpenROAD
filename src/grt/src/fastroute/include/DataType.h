// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
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

std::ostream& operator<<(std::ostream& os, const RouteType& type);

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
  Segment(int netID,
          int16_t x1,
          int16_t y1,
          int16_t x2,
          int16_t y2,
          int8_t cost)
      : netID(netID), x1(x1), y1(y1), x2(x2), y2(y2), cost(cost)
  {
  }
  const int netID;
  const int16_t x1, y1, x2, y2;  // coordinates of two endpoints (x1 <= x2)
  int16_t Zpoint{-1};  // The coordinates of Z point (x for HVH and y for VHV)
  const int8_t cost;   // the netID of the net this segment belonging to
  bool xFirst : 1 {false};  // route x-direction first (only for L route)
  bool HVH : 1 {false};     // TRUE = HVH or false = VHV (only for Z route)
};

struct FrNet  // A Net is a set of connected MazePoints
{
  bool isClock() const { return is_clock_; }
  bool isCritical() { return is_critical_; }
  float getSlack() const { return slack_; }
  odb::dbNet* getDbNet() const { return db_net_; }
  int getDriverIdx() const { return driver_idx_; }
  int8_t getEdgeCost() const { return edge_cost_; }
  void setEdgeCost(int cost) { edge_cost_ = cost; }
  const char* getName() const;
  int getMaxLayer() const { return max_layer_; }
  int getMinLayer() const { return min_layer_; }
  int getNumPins() const { return pin_x_.size(); }
  int8_t getLayerEdgeCost(int layer) const;

  int getPinX(int idx) const { return pin_x_[idx]; }
  int getPinY(int idx) const { return pin_y_[idx]; }
  int getPinL(int idx) const { return pin_l_[idx]; }
  const std::vector<int>& getPinX() const { return pin_x_; }
  const std::vector<int>& getPinY() const { return pin_y_; }
  const std::vector<int>& getPinL() const { return pin_l_; }
  int getPinIdxFromPosition(int x, int y, int count);

  void addPin(int x, int y, int layer);
  void reset(odb::dbNet* db_net,
             bool is_clock,
             int driver_idx,
             int8_t edge_cost,
             int min_layer,
             int max_layer,
             float slack,
             std::vector<int8_t>* edge_cost_per_layer);
  void setMaxLayer(int max_layer) { max_layer_ = max_layer; }
  void setMinLayer(int min_layer) { min_layer_ = min_layer; }
  void setSlack(float slack) { slack_ = slack; }
  void setIsCritical(bool is_critical) { is_critical_ = is_critical; }
  void setIsSoftNDR(bool is_soft) { is_soft_ndr_ = is_soft; }
  bool isSoftNDR() { return is_soft_ndr_; }
  void setIsResAware(bool res_aware) { is_res_aware_ = res_aware; }
  bool isResAware() { return is_res_aware_; }

 private:
  odb::dbNet* db_net_;
  std::vector<int> pin_x_;  // x coordinates of pins
  std::vector<int> pin_y_;  // y coordinates of pins
  std::vector<int> pin_l_;  // l coordinates of pins
  bool is_clock_;           // flag that indicates if net is a clock net
  bool is_critical_;
  int driver_idx_;
  int8_t edge_cost_;
  int min_layer_;
  int max_layer_;
  float slack_;
  bool is_soft_ndr_ = false;
  bool is_res_aware_ = false;
  // Non-null when an NDR has been applied to the net.
  std::unique_ptr<std::vector<int8_t>> edge_cost_per_layer_;
};

struct Edge  // An Edge is the routing track holder between two adjacent
             // MazePoints
{
  int16_t congCNT;
  uint16_t cap;    // the capacity of the edge
  uint16_t usage;  // the usage of the edge
  uint16_t red;
  uint16_t real_cap;  // the real capacity without user adjustment
  int16_t last_usage;
  uint16_t ndr_overflow;  // number of NDR nets in congestion
  double est_usage;       // the estimated usage of the edge

  uint16_t usage_red() const { return usage + red; }
  double est_usage_red() const { return est_usage + red; }
};

struct Edge3D
{
  uint16_t cap;       // the capacity of the edge
  uint16_t usage;     // the usage of the edge
  uint16_t red;       // the reduction of capacity of the edge
  uint16_t real_cap;  // the real capacity without user adjustment
};

struct TreeNode
{
  bool assigned;

  int16_t status = 0;
  int16_t conCNT = 0;
  int16_t botL = -1;
  int16_t topL = -1;
  // heights and eID arrays size were increased after using PD
  // to create the tree topologies.
  static constexpr int max_connections = 10;
  int16_t heights[max_connections] = {0};
  int eID[max_connections] = {0};

  int16_t x, y;  // position in the grid graph
  int nbr_count = 0;
  int nbr[3];   // three neighbors
  int edge[3];  // three adjacent edges
  int hID = -1;
  int lID = -1;
  // If two nodes are at the same x & y then the duplicate will have
  // stackAlias set to the index of the first node.  This does not
  // apply to pins nodes, only Steiner nodes.
  int stackAlias;
};

struct GPoint3D
{
  int16_t x = 0;
  int16_t y = 0;
  int16_t layer = 0;
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
  bool HVH = false;

  // valid for ZRoute: the position of turn point for Z-shape
  int16_t Zpoint = -1;

  // valid for MazeRoute: a list of grids (n=routelen+1) the route
  // passes, (x1, y1) is the first one, but (x2, y2) is the lastone
  std::vector<GPoint3D> grids;

  // valid for MazeRoute: the number of edges in the route
  int routelen;

  int last_routelen = 0;  // the last routelen before overflow itter
};

struct TreeEdge
{
  bool assigned;

  int len = 0;  // the Manhanttan Distance for two end nodes
  int n1, n1a;
  int n2, n2a;
  Route route;
  void convertToMazerouteNet(const TreeNode& p1, const TreeNode& p2);
};

struct StTree
{
  int num_terminals = 0;
  // The nodes (pin and Steiner nodes) in the tree.
  std::vector<TreeNode> nodes;
  std::vector<TreeEdge> edges;
  std::map<int, int> node_to_pin_idx;

  int num_edges() const { return edges.size(); }
  int num_nodes() const { return nodes.size(); }
};

struct OrderNetPin
{
  int treeIndex;
  int minX;
  float length_per_pin;  // net length over pin count
  int ndr_priority;      // NDR nets are assigned first
  int res_aware;
  float slack;
  int clock;
};

struct OrderTree
{
  int length;
  int treeIndex;
  int16_t xmin;
};

struct OrderNetEdge
{
  int length;
  int edgeID;
};

using utl::format_as;

}  // namespace grt
