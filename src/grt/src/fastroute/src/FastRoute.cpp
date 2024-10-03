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

#include "FastRoute.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "AbstractFastRouteRenderer.h"
#include "DataType.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

FastRouteCore::FastRouteCore(odb::dbDatabase* db,
                             utl::Logger* log,
                             stt::SteinerTreeBuilder* stt_builder)
    : max_degree_(0),
      db_(db),
      overflow_iterations_(0),
      congestion_report_iter_step_(0),
      x_range_(0),
      y_range_(0),
      num_adjust_(0),
      v_capacity_(0),
      h_capacity_(0),
      x_grid_(0),
      y_grid_(0),
      x_grid_max_(0),
      y_grid_max_(0),
      x_corner_(0),
      y_corner_(0),
      tile_size_(0),
      enlarge_(0),
      costheight_(0),
      ahth_(0),
      num_layers_(0),
      total_overflow_(0),
      has_2D_overflow_(false),
      grid_hv_(0),
      verbose_(false),
      critical_nets_percentage_(10),
      via_cost_(0),
      mazeedge_threshold_(0),
      v_capacity_lb_(0),
      h_capacity_lb_(0),
      regular_x_(false),
      regular_y_(false),
      logger_(log),
      stt_builder_(stt_builder),
      debug_(new DebugSetting())
{
  parasitics_builder_ = nullptr;
}

FastRouteCore::~FastRouteCore()
{
  clearNets();
}

void FastRouteCore::clear()
{
  clearNets();

  num_adjust_ = 0;
  v_capacity_ = 0;
  h_capacity_ = 0;
  total_overflow_ = 0;
  has_2D_overflow_ = false;

  h_edges_.resize(boost::extents[0][0]);
  v_edges_.resize(boost::extents[0][0]);
  seglist_.clear();

  gxs_.clear();
  gys_.clear();
  gs_.clear();

  tree_order_pv_.clear();
  tree_order_cong_.clear();

  h_edges_3D_.resize(boost::extents[0][0][0]);
  v_edges_3D_.resize(boost::extents[0][0][0]);

  parent_x1_.resize(boost::extents[0][0]);
  parent_y1_.resize(boost::extents[0][0]);
  parent_x3_.resize(boost::extents[0][0]);
  parent_y3_.resize(boost::extents[0][0]);

  net_eo_.clear();

  xcor_.clear();
  ycor_.clear();
  dcor_.clear();

  hv_.resize(boost::extents[0][0]);
  hyper_v_.resize(boost::extents[0][0]);
  hyper_h_.resize(boost::extents[0][0]);
  corr_edge_.resize(boost::extents[0][0]);

  in_region_.resize(boost::extents[0][0]);

  v_capacity_3D_.clear();
  h_capacity_3D_.clear();

  cost_hvh_.clear();
  cost_vhv_.clear();
  cost_h_.clear();
  cost_v_.clear();
  cost_lr_.clear();
  cost_tb_.clear();
  cost_hvh_test_.clear();
  cost_v_test_.clear();
  cost_tb_test_.clear();

  vertical_blocked_intervals_.clear();
  horizontal_blocked_intervals_.clear();
}

void FastRouteCore::clearNets()
{
  if (!sttrees_.empty()) {
    sttrees_.clear();
  }

  for (FrNet* net : nets_) {
    delete net;
  }
  nets_.clear();
  net_ids_.clear();
  seglist_.clear();
  db_net_id_map_.clear();
}

void FastRouteCore::setGridsAndLayers(int x, int y, int nLayers)
{
  x_grid_ = x;
  y_grid_ = y;
  num_layers_ = nLayers;
  layer_directions_.resize(num_layers_);
  if (std::max(x_grid_, y_grid_) >= 1000) {
    x_range_ = std::max(x_grid_, y_grid_);
    y_range_ = std::max(x_grid_, y_grid_);
  } else {
    x_range_ = 1000;
    y_range_ = 1000;
  }

  v_capacity_3D_.resize(num_layers_);
  h_capacity_3D_.resize(num_layers_);
  last_col_v_capacity_3D_.resize(num_layers_);
  last_row_h_capacity_3D_.resize(num_layers_);

  for (int i = 0; i < num_layers_; i++) {
    v_capacity_3D_[i] = 0;
    h_capacity_3D_[i] = 0;
    last_col_v_capacity_3D_[i] = 0;
    last_row_h_capacity_3D_[i] = 0;
  }

  hv_.resize(boost::extents[y_range_][x_range_]);
  hyper_v_.resize(boost::extents[y_range_][x_range_]);
  hyper_h_.resize(boost::extents[y_range_][x_range_]);
  corr_edge_.resize(boost::extents[y_range_][x_range_]);

  in_region_.resize(boost::extents[y_range_][x_range_]);

  cost_hvh_.resize(x_range_);  // Horizontal first Z
  cost_vhv_.resize(y_range_);  // Vertical first Z
  cost_h_.resize(y_range_);    // Horizontal segment cost
  cost_v_.resize(x_range_);    // Vertical segment cost
  cost_lr_.resize(y_range_);   // Left and right boundary cost
  cost_tb_.resize(x_range_);   // Top and bottom boundary cost

  cost_hvh_test_.resize(y_range_);  // Vertical first Z
  cost_v_test_.resize(x_range_);    // Vertical segment cost
  cost_tb_test_.resize(x_range_);   // Top and bottom boundary cost

  // maze3D variables
  directions_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  corr_edge_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  pr_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);

  int64 total_size = static_cast<int64>(num_layers_) * y_range_ * x_range_;
  pop_heap2_3D_.resize(total_size, false);

  // allocate memory for priority queue
  total_size = static_cast<int64>(y_grid_) * x_grid_ * num_layers_;
  src_heap_3D_.resize(total_size);
  dest_heap_3D_.resize(total_size);

  d1_3D_.resize(boost::extents[num_layers_][y_range_][x_range_]);
  d2_3D_.resize(boost::extents[num_layers_][y_range_][x_range_]);
}

void FastRouteCore::addVCapacity(short verticalCapacity, int layer)
{
  v_capacity_3D_[layer - 1] = verticalCapacity;
  v_capacity_ += v_capacity_3D_[layer - 1];
}

void FastRouteCore::addHCapacity(short horizontalCapacity, int layer)
{
  h_capacity_3D_[layer - 1] = horizontalCapacity;
  h_capacity_ += h_capacity_3D_[layer - 1];
}

void FastRouteCore::setLowerLeft(int x, int y)
{
  x_corner_ = x;
  y_corner_ = y;
}

void FastRouteCore::setTileSize(int size)
{
  tile_size_ = size;
}

void FastRouteCore::addLayerDirection(int layer_idx,
                                      const odb::dbTechLayerDir& direction)
{
  layer_directions_[layer_idx] = direction;
}

FrNet* FastRouteCore::addNet(odb::dbNet* db_net,
                             bool is_clock,
                             int driver_idx,
                             int cost,
                             int min_layer,
                             int max_layer,
                             float slack,
                             std::vector<int>* edge_cost_per_layer)
{
  int netID;
  bool exists;
  FrNet* net;
  getNetId(db_net, netID, exists);
  if (exists) {
    net = nets_[netID];
    clearNetRoute(netID);
    seglist_[netID].clear();
  } else {
    net = new FrNet;
    nets_.push_back(net);
    netID = nets_.size() - 1;
    db_net_id_map_[db_net] = netID;
    // at most (2*num_pins-2) nodes -> (2*num_pins-3) segs_ for a net
    seglist_.emplace_back();
    sttrees_.emplace_back();
    gxs_.emplace_back();
    gys_.emplace_back();
    gs_.emplace_back();
  }
  net->reset(db_net,
             is_clock,
             driver_idx,
             cost,
             min_layer,
             max_layer,
             slack,
             edge_cost_per_layer);
  net_ids_.push_back(netID);

  return net;
}

void FastRouteCore::deleteNet(odb::dbNet* db_net)
{
  const int net_id = db_net_id_map_[db_net];
  FrNet* delete_net = nets_[net_id];
  nets_[net_id] = nullptr;
  delete delete_net;
  db_net_id_map_.erase(db_net);
}

void FastRouteCore::removeNet(odb::dbNet* db_net)
{
  if (db_net_id_map_.find(db_net) != db_net_id_map_.end()) {
    const int net_id = db_net_id_map_[db_net];
    clearNetRoute(net_id);
    deleteNet(db_net);
  }
}

void FastRouteCore::mergeNet(odb::dbNet* db_net)
{
  if (db_net_id_map_.find(db_net) != db_net_id_map_.end()) {
    const int net_id = db_net_id_map_[db_net];
    sttrees_[net_id].nodes.clear();
    sttrees_[net_id].edges.clear();
    deleteNet(db_net);
  }
}

void FastRouteCore::clearNetRoute(odb::dbNet* db_net)
{
  if (db_net_id_map_.find(db_net) != db_net_id_map_.end()) {
    const int net_id = db_net_id_map_[db_net];
    clearNetRoute(net_id);
  }
}

void FastRouteCore::getNetId(odb::dbNet* db_net, int& net_id, bool& exists)
{
  auto itr = db_net_id_map_.find(db_net);
  exists = itr != db_net_id_map_.end();
  net_id = exists ? itr->second : 0;
}

void FastRouteCore::clearNetRoute(const int netID)
{
  // clear used resources for the net route
  releaseNetResources(netID);

  // clear stree
  sttrees_[netID].nodes.clear();
  sttrees_[netID].edges.clear();
}

void FastRouteCore::initEdges()
{
  const float LB = 0.9;
  v_capacity_lb_ = LB * v_capacity_;
  h_capacity_lb_ = LB * h_capacity_;

  // allocate memory and initialize for edges

  h_edges_.resize(boost::extents[y_grid_][x_grid_ - 1]);
  v_edges_.resize(boost::extents[y_grid_ - 1][x_grid_]);

  v_edges_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  h_edges_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // 2D edge initialization
      if (j < x_grid_ - 1) {
        h_edges_[i][j].cap = h_capacity_;
        h_edges_[i][j].usage = 0;
        h_edges_[i][j].est_usage = 0;
        h_edges_[i][j].red = 0;
        h_edges_[i][j].last_usage = 0;
      }

      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        h_edges_3D_[k][i][j].cap = h_capacity_3D_[k];
        h_edges_3D_[k][i][j].usage = 0;
        h_edges_3D_[k][i][j].red = 0;
      }
    }
  }
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // 2D edge initialization
      if (i < y_grid_ - 1) {
        v_edges_[i][j].cap = v_capacity_;
        v_edges_[i][j].usage = 0;
        v_edges_[i][j].est_usage = 0;
        v_edges_[i][j].red = 0;
        v_edges_[i][j].last_usage = 0;
      }

      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        v_edges_3D_[k][i][j].cap = v_capacity_3D_[k];
        v_edges_3D_[k][i][j].usage = 0;
        v_edges_3D_[k][i][j].red = 0;
      }
    }
  }
}

void FastRouteCore::setNumAdjustments(int nAdjustments)
{
  num_adjust_ = nAdjustments;
}

void FastRouteCore::setMaxNetDegree(int deg)
{
  max_degree_ = deg;
}

void FastRouteCore::addAdjustment(int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  int layer,
                                  uint16_t reducedCap,
                                  bool isReduce)
{
  const int k = layer - 1;

  if (y1 == y2) {
    // horizontal edge
    const int cap = h_edges_3D_[k][y1][x1].cap;
    int reduce;

    if (cap - reducedCap < 0) {
      if (isReduce) {
        if (verbose_)
          logger_->warn(GRT,
                        113,
                        "Underflow in reduce: cap, reducedCap: {}, {}",
                        cap,
                        reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    h_edges_3D_[k][y1][x1].cap = reducedCap;

    if (!isReduce) {
      const int increase = reducedCap - cap;
      if (x1 < x_grid_ - 1) {
        h_edges_[y1][x1].cap += increase;
        int new_red = h_edges_[y1][x1].red - increase;
        h_edges_[y1][x1].red = std::max(new_red, 0);
      }
      int new_red_3D = h_edges_3D_[k][y1][x1].red - increase;
      h_edges_3D_[k][y1][x1].red = std::max(new_red_3D, 0);
    } else {
      h_edges_3D_[k][y1][x1].red += reduce;
    }

    if (x1 < x_grid_ - 1) {
      h_edges_[y1][x1].cap -= reduce;
      h_edges_[y1][x1].red += reduce;
    }

  } else if (x1 == x2) {  // vertical edge
    const int cap = v_edges_3D_[k][y1][x1].cap;
    int reduce;

    if (cap - reducedCap < 0) {
      if (isReduce) {
        if (verbose_)
          logger_->warn(GRT,
                        114,
                        "Underflow in reduce: cap, reducedCap: {}, {}",
                        cap,
                        reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    v_edges_3D_[k][y1][x1].cap = reducedCap;

    if (!isReduce) {
      int increase = reducedCap - cap;
      if (y1 < y_grid_ - 1) {
        v_edges_[y1][x1].cap += increase;
        int new_red = v_edges_[y1][x1].red - increase;
        v_edges_[y1][x1].red = std::max(new_red, 0);
      }
      int new_red_3D = v_edges_3D_[k][y1][x1].red - increase;
      v_edges_3D_[k][y1][x1].red = std::max(new_red_3D, 0);
    } else {
      v_edges_3D_[k][y1][x1].red += reduce;
    }

    if (y1 < y_grid_ - 1) {
      v_edges_[y1][x1].cap -= reduce;
      v_edges_[y1][x1].red += reduce;
    }
  }
}

void FastRouteCore::addVerticalAdjustments(
    const odb::Point& first_tile,
    const odb::Point& last_tile,
    const int layer,
    const interval<int>::type& first_tile_reduce_interval,
    const interval<int>::type& last_tile_reduce_interval)
{
  // add intervals to set for each tile
  for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y < last_tile.getY(); y++) {
      if (x == first_tile.getX()) {
        vertical_blocked_intervals_[std::make_tuple(x, y, layer)]
            += first_tile_reduce_interval;
      } else if (x == last_tile.getX()) {
        vertical_blocked_intervals_[std::make_tuple(x, y, layer)]
            += last_tile_reduce_interval;
      } else {
        addAdjustment(x, y, x, y + 1, layer, 0, true);
      }
    }
  }
}

void FastRouteCore::addHorizontalAdjustments(
    const odb::Point& first_tile,
    const odb::Point& last_tile,
    const int layer,
    const interval<int>::type& first_tile_reduce_interval,
    const interval<int>::type& last_tile_reduce_interval)
{
  // add intervals to each tiles
  for (int x = first_tile.getX(); x < last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
      if (y == first_tile.getY()) {
        horizontal_blocked_intervals_[std::make_tuple(x, y, layer)]
            += first_tile_reduce_interval;
      } else if (y == last_tile.getY()) {
        horizontal_blocked_intervals_[std::make_tuple(x, y, layer)]
            += last_tile_reduce_interval;
      } else {
        addAdjustment(x, y, x + 1, y, layer, 0, true);
      }
    }
  }
}

void FastRouteCore::initBlockedIntervals(std::vector<int>& track_space)
{
  // Calculate reduce for vertical tiles
  for (const auto& [tile, intervals] : vertical_blocked_intervals_) {
    int x = std::get<0>(tile);
    int y = std::get<1>(tile);
    int layer = std::get<2>(tile);
    int edge_cap = getEdgeCapacity(x, y, x, y + 1, layer);
    if (edge_cap > 0) {
      int reduce = 0;
      if (layer > 0 && layer <= track_space.size()) {
        for (const auto& interval_it : intervals) {
          reduce += std::ceil(static_cast<float>(std::abs(
                                  interval_it.upper() - interval_it.lower()))
                              / track_space[layer - 1]);
        }
      }
      edge_cap -= reduce;
      if (edge_cap < 0)
        edge_cap = 0;
      addAdjustment(x, y, x, y + 1, layer, edge_cap, true);
    }
  }

  // Calculate reduce for horizontal tiles
  for (const auto& [tile, intervals] : horizontal_blocked_intervals_) {
    int x = std::get<0>(tile);
    int y = std::get<1>(tile);
    int layer = std::get<2>(tile);
    int edge_cap = getEdgeCapacity(x, y, x + 1, y, layer);
    if (edge_cap > 0) {
      int reduce = 0;
      if (layer > 0 && layer <= track_space.size()) {
        for (const auto& interval_it : intervals) {
          reduce += std::ceil(static_cast<float>(std::abs(
                                  interval_it.upper() - interval_it.lower()))
                              / track_space[layer - 1]);
        }
      }
      edge_cap -= reduce;
      if (edge_cap < 0)
        edge_cap = 0;
      addAdjustment(x, y, x + 1, y, layer, edge_cap, true);
    }
  }
}

int FastRouteCore::getEdgeCapacity(int x1, int y1, int x2, int y2, int layer)
{
  const int k = layer - 1;

  if (y1 == y2) {  // horizontal edge
    return h_edges_3D_[k][y1][x1].cap;
  } else if (x1 == x2) {  // vertical edge
    return v_edges_3D_[k][y1][x1].cap;
  } else {
    logger_->error(
        GRT,
        214,
        "Cannot get edge capacity: edge is not vertical or horizontal.");
    return 0;
  }
}

int FastRouteCore::getEdgeCapacity(FrNet* net,
                                   int x1,
                                   int y1,
                                   EdgeDirection direction)
{
  int cap = 0;

  // get 2D edge capacity respecting layer restrictions
  for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
    if (direction == EdgeDirection::Horizontal) {
      cap += h_edges_3D_[l][y1][x1].cap;
    } else {
      cap += v_edges_3D_[l][y1][x1].cap;
    }
  }

  return cap;
}

void FastRouteCore::incrementEdge3DUsage(int x1,
                                         int y1,
                                         int x2,
                                         int y2,
                                         int layer)
{
  const int k = layer - 1;

  if (y1 == y2) {  // horizontal edge
    for (int x = x1; x < x2; x++) {
      h_edges_3D_[k][y1][x].usage++;
    }
  } else if (x1 == x2) {  // vertical edge
    for (int y = y1; y < y2; y++) {
      v_edges_3D_[k][y][x1].usage++;
    }
  }
}

void FastRouteCore::initAuxVar()
{
  tree_order_cong_.clear();

  grid_hv_ = x_range_ * y_range_;

  parent_x1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_x3_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y3_.resize(boost::extents[y_grid_][x_grid_]);
}

NetRouteMap FastRouteCore::getRoutes()
{
  NetRouteMap routes;
  for (const int& netID : net_ids_) {
    odb::dbNet* db_net = nets_[netID]->getDbNet();
    GRoute& route = routes[db_net];
    std::unordered_set<GSegment, GSegmentHash> net_segs;

    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0 || treeedge->route.routelen > 0) {
        int routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;
        int lastX = tile_size_ * (gridsX[0] + 0.5) + x_corner_;
        int lastY = tile_size_ * (gridsY[0] + 0.5) + y_corner_;
        int lastL = gridsL[0];
        for (int i = 1; i <= routeLen; i++) {
          const int xreal = tile_size_ * (gridsX[i] + 0.5) + x_corner_;
          const int yreal = tile_size_ * (gridsY[i] + 0.5) + y_corner_;

          GSegment segment
              = GSegment(lastX, lastY, lastL + 1, xreal, yreal, gridsL[i] + 1);

          lastX = xreal;
          lastY = yreal;
          lastL = gridsL[i];
          if (net_segs.find(segment) == net_segs.end()) {
            if (segment.init_layer != segment.final_layer) {
              GSegment invet_via = GSegment(segment.final_x,
                                            segment.final_y,
                                            segment.final_layer,
                                            segment.init_x,
                                            segment.init_y,
                                            segment.init_layer);
              if (net_segs.find(invet_via) != net_segs.end()) {
                continue;
              }
            }

            net_segs.insert(segment);
            route.push_back(segment);
          }
        }
      }
    }
  }

  return routes;
}

NetRouteMap FastRouteCore::getPlanarRoutes()
{
  NetRouteMap routes;

  // Get routes before layer assignment

  for (const int& netID : net_ids_) {
    auto fr_net = nets_[netID];
    odb::dbNet* db_net = fr_net->getDbNet();
    GRoute& route = routes[db_net];
    std::unordered_set<GSegment, GSegmentHash> net_segs;

    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        int lastX = tile_size_ * (gridsX[0] + 0.5) + x_corner_;
        int lastY = tile_size_ * (gridsY[0] + 0.5) + y_corner_;

        // defines the layer used for vertical edges are still 2D
        int layer_h = 0;

        // defines the layer used for horizontal edges are still 2D
        int layer_v = 0;

        if (layer_directions_[nets_[netID]->getMinLayer()]
            == odb::dbTechLayerDir::VERTICAL) {
          layer_h = nets_[netID]->getMinLayer() + 1;
          layer_v = nets_[netID]->getMinLayer();
        } else {
          layer_h = nets_[netID]->getMinLayer();
          layer_v = nets_[netID]->getMinLayer() + 1;
        }
        int second_x = tile_size_ * (gridsX[1] + 0.5) + x_corner_;
        int lastL = (lastX == second_x) ? layer_v : layer_h;

        for (int i = 1; i <= routeLen; i++) {
          const int xreal = tile_size_ * (gridsX[i] + 0.5) + x_corner_;
          const int yreal = tile_size_ * (gridsY[i] + 0.5) + y_corner_;
          GSegment segment;
          if (lastX == xreal) {
            // if change direction add a via to change the layer
            if (lastL == layer_h) {
              segment = GSegment(
                  lastX, lastY, lastL + 1, lastX, lastY, layer_v + 1);
              if (net_segs.find(segment) == net_segs.end()) {
                net_segs.insert(segment);
                route.push_back(segment);
              }
            }
            lastL = layer_v;
            segment
                = GSegment(lastX, lastY, lastL + 1, xreal, yreal, lastL + 1);
          } else {
            // if change direction add a via to change the layer
            if (lastL == layer_v) {
              segment = GSegment(
                  lastX, lastY, lastL + 1, lastX, lastY, layer_h + 1);
              if (net_segs.find(segment) == net_segs.end()) {
                net_segs.insert(segment);
                route.push_back(segment);
              }
            }
            lastL = layer_h;
            segment
                = GSegment(lastX, lastY, lastL + 1, xreal, yreal, lastL + 1);
          }
          lastX = xreal;
          lastY = yreal;
          if (net_segs.find(segment) == net_segs.end()) {
            net_segs.insert(segment);
            route.push_back(segment);
          }
        }
      }
    }
  }

  return routes;
}

void FastRouteCore::getBlockage(odb::dbTechLayer* layer,
                                int x,
                                int y,
                                uint8_t& blockage_h,
                                uint8_t& blockage_v)
{
  int l = layer->getRoutingLevel() - 1;
  if (x == x_grid_ - 1 && y == y_grid_ - 1 && x_grid_ > 1 && y_grid_ > 1) {
    blockage_h = h_edges_3D_[l][y][x - 1].red;
    blockage_v = v_edges_3D_[l][y - 1][x].red;
  } else {
    blockage_h = h_edges_3D_[l][y][x].red;
    blockage_v = v_edges_3D_[l][y][x].red;
  }
}

void FastRouteCore::updateDbCongestion(int min_routing_layer,
                                       int max_routing_layer)
{
  if (h_edges_3D_.num_elements() == 0) {  // no information
    return;
  }
  auto block = db_->getChip()->getBlock();
  auto db_gcell = block->getGCellGrid();
  if (db_gcell)
    db_gcell->resetGrid();
  else
    db_gcell = odb::dbGCellGrid::create(block);

  db_gcell->addGridPatternX(x_corner_, x_grid_, tile_size_);
  db_gcell->addGridPatternY(y_corner_, y_grid_, tile_size_);
  auto db_tech = db_->getTech();
  for (int k = min_routing_layer - 1; k <= max_routing_layer - 1; k++) {
    auto layer = db_tech->findRoutingLayer(k + 1);
    if (layer == nullptr) {
      continue;
    }

    const uint8_t capH = h_capacity_3D_[k];
    const uint8_t capV = v_capacity_3D_[k];
    const uint8_t last_row_capH = last_row_h_capacity_3D_[k];
    const uint8_t last_col_capV = last_col_v_capacity_3D_[k];
    bool is_horizontal
        = layer_directions_[k] == odb::dbTechLayerDir::HORIZONTAL;
    for (int y = 0; y < y_grid_; y++) {
      for (int x = 0; x < x_grid_; x++) {
        if (is_horizontal) {
          if (!regular_y_ && y == y_grid_ - 1) {
            db_gcell->setCapacity(layer, x, y, last_row_capH);
          } else {
            db_gcell->setCapacity(layer, x, y, capH);
          }
        } else {
          if (!regular_x_ && x == x_grid_ - 1) {
            db_gcell->setCapacity(layer, x, y, last_col_capV);
          } else {
            db_gcell->setCapacity(layer, x, y, capV);
          }
        }
        if (x == x_grid_ - 1 && y == y_grid_ - 1 && x_grid_ > 1
            && y_grid_ > 1) {
          uint8_t blockageH = h_edges_3D_[k][y][x - 1].red;
          uint8_t blockageV = v_edges_3D_[k][y - 1][x].red;
          uint8_t usageH = h_edges_3D_[k][y - 1][x - 1].usage + blockageH;
          uint8_t usageV = v_edges_3D_[k][y - 1][x - 1].usage + blockageV;
          db_gcell->setUsage(layer, x, y, usageH + usageV);
        } else {
          uint8_t blockageH = h_edges_3D_[k][y][x].red;
          uint8_t blockageV = v_edges_3D_[k][y][x].red;
          uint8_t usageH = h_edges_3D_[k][y][x].usage + blockageH;
          uint8_t usageV = v_edges_3D_[k][y][x].usage + blockageV;
          db_gcell->setUsage(layer, x, y, usageH + usageV);
        }
      }
    }
  }
}

void FastRouteCore::getCapacityReductionData(
    CapacityReductionData& cap_red_data)
{
  cap_red_data.resize(x_grid_);
  for (int x = 0; x < x_grid_; x++) {
    cap_red_data[x].resize(y_grid_);
  }

  for (int k = 0; k < num_layers_; k++) {
    const uint8_t capH = h_capacity_3D_[k];
    const uint8_t capV = v_capacity_3D_[k];
    const uint8_t last_row_capH = last_row_h_capacity_3D_[k];
    const uint8_t last_col_capV = last_col_v_capacity_3D_[k];
    bool is_horizontal
        = layer_directions_[k] == odb::dbTechLayerDir::HORIZONTAL;
    for (int x = 0; x < x_grid_; x++) {
      for (int y = 0; y < y_grid_; y++) {
        if (is_horizontal) {
          if (!regular_y_ && y == y_grid_ - 1) {
            cap_red_data[x][y].capacity += last_row_capH;
          } else if (x != x_grid_ - 1 || y == y_grid_ - 1) {
            // don't add horizontal cap in the last col because there is no
            // usage there
            cap_red_data[x][y].capacity += capH;
          }
        } else {
          if (!regular_x_ && x == x_grid_ - 1) {
            cap_red_data[x][y].capacity += last_col_capV;
          } else if (y != y_grid_ - 1 || x == x_grid_ - 1) {
            // don't add vertical cap in the last row because there is no usage
            // there
            cap_red_data[x][y].capacity += capV;
          }
        }
        if (x == x_grid_ - 1 && y == y_grid_ - 1 && x_grid_ > 1
            && y_grid_ > 1) {
          uint8_t blockageH = h_edges_3D_[k][y][x - 1].red;
          uint8_t blockageV = v_edges_3D_[k][y - 1][x].red;
          cap_red_data[x][y].reduction += blockageH + blockageV;
        } else {
          uint8_t blockageH = h_edges_3D_[k][y][x].red;
          uint8_t blockageV = v_edges_3D_[k][y][x].red;
          cap_red_data[x][y].reduction += blockageH + blockageV;
        }
      }
    }
  }
}

NetRouteMap FastRouteCore::run()
{
  if (netCount() == 0) {
    return getRoutes();
  }

  v_used_ggrid_.clear();
  h_used_ggrid_.clear();

  int tUsage;
  int cost_step;
  int maxOverflow = 0;
  int minoflrnd = 0;
  int bwcnt = 0;

  // Init grid variables when debug mode is actived
  if (debug_->isOn()) {
    fastrouteRender()->setGridVariables(tile_size_, x_corner_, y_corner_);
  }

  // TODO: check this size
  int max_degree2 = 2 * max_degree_;
  xcor_.resize(max_degree2);
  ycor_.resize(max_degree2);
  dcor_.resize(max_degree2);
  net_eo_.reserve(max_degree2);

  int THRESH_M = 20;
  const int ENLARGE = 15;  // 5
  const int ESTEP1 = 10;   // 10
  const int ESTEP2 = 5;    // 5
  const int ESTEP3 = 5;    // 5
  int CSTEP1 = 2;          // 5
  const int CSTEP2 = 2;    // 3
  const int CSTEP3 = 5;    // 15
  const int COSHEIGHT = 4;
  int L = 0;
  int VIA = 2;
  const int Ripvalue = -1;
  const bool goingLV = true;
  const bool noADJ = false;
  const int thStep1 = 10;
  const int thStep2 = 4;
  const int LVIter = 3;
  const int mazeRound = 500;
  int bmfl = BIG_INT;
  int minofl = BIG_INT;
  float LOGIS_COF = 0;
  int slope;
  int max_adj;

  // call FLUTE to generate RSMT and break the nets into segments (2-pin nets)

  via_cost_ = 0;
  gen_brk_RSMT(false, false, false, false, noADJ);
  routeLAll(true);
  gen_brk_RSMT(true, true, true, false, noADJ);

  getOverflow2D(&maxOverflow);
  newrouteLAll(false, true);
  getOverflow2D(&maxOverflow);
  spiralRouteAll();
  newrouteZAll(10);
  int past_cong = getOverflow2D(&maxOverflow);

  convertToMazeroute();

  int enlarge_ = 10;
  int newTH = 10;
  bool stopDEC = false;
  int upType = 1;

  costheight_ = COSHEIGHT;
  if (maxOverflow > 700) {
    costheight_ = 8;
    LOGIS_COF = 1.33;
    VIA = 0;
    THRESH_M = 0;
    CSTEP1 = 30;
  }

  for (int i = 0; i < LVIter; i++) {
    LOGIS_COF = std::max<float>(2.0 / (1 + log(maxOverflow)), LOGIS_COF);
    LOGIS_COF = 2.0 / (1 + log(maxOverflow));
    debugPrint(logger_,
               GRT,
               "patternRouting",
               1,
               "LV routing round {}, enlarge {}.",
               i,
               enlarge_);
    routeMonotonicAll(newTH, enlarge_, LOGIS_COF);

    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    enlarge_ += 5;
    newTH -= 5;
    if (newTH < 1) {
      newTH = 1;
    }
  }

  //  past_cong = getOverflow2Dmaze( &maxOverflow);

  InitEstUsage();

  int i = 1;
  costheight_ = COSHEIGHT;
  enlarge_ = ENLARGE;
  int ripup_threshold = Ripvalue;

  minofl = total_overflow_;
  stopDEC = false;

  slope = 20;
  L = 1;
  int cost_type = 1;

  InitLastUsage(upType);
  if (total_overflow_ > 0 && overflow_iterations_ > 0 && verbose_) {
    logger_->info(GRT, 101, "Running extra iterations to remove overflow.");
  }

  // debug mode Rectilinear Steiner Tree before overflow iterations
  if (debug_->isOn() && debug_->rectilinearSTree_) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net_) {
        StTreeVisualization(sttrees_[netID], nets_[netID], false);
      }
    }
  }

  SaveLastRouteLen();

  const int max_overflow_increases = 25;

  float slack_th = std::numeric_limits<float>::lowest();

  // set overflow_increases as -1 since the first iteration always sum 1
  int overflow_increases = -1;
  int last_total_overflow = 0;
  float overflow_reduction_percent = -1;
  while (total_overflow_ > 0 && i <= overflow_iterations_
         && overflow_increases <= max_overflow_increases) {
    if (THRESH_M > 15) {
      THRESH_M -= thStep1;
    } else if (THRESH_M >= 2) {
      THRESH_M -= thStep2;
    } else {
      THRESH_M = 0;
    }
    if (THRESH_M <= 0) {
      THRESH_M = 0;
    }

    if (total_overflow_ > 2000) {
      enlarge_ += ESTEP1;  // ENLARGE+(i-1)*ESTEP;
      cost_step = CSTEP1;
    } else if (total_overflow_ < 500) {
      cost_step = CSTEP3;
      enlarge_ += ESTEP3;
      ripup_threshold = -1;
    } else {
      cost_step = CSTEP2;
      enlarge_ += ESTEP2;
    }
    updateCongestionHistory(upType, stopDEC, max_adj);

    if (total_overflow_ > 15000 && maxOverflow > 400) {
      enlarge_ = std::max(x_grid_, y_grid_) / 30;
      slope = BIG_INT;
      if (i == 5) {
        VIA = 0;
        LOGIS_COF = 1.33;
        ripup_threshold = -1;
        //  cost_type = 3;

      } else if (i > 6) {
        if (i % 2 == 0) {
          LOGIS_COF += 0.5;
        }
        if (i > 40) {
          break;
        }
      }
      if (i > 10) {
        cost_type = 1;
        ripup_threshold = 0;
      }
    }

    enlarge_ = std::min(enlarge_, x_grid_ / 2);
    costheight_ += cost_step;
    mazeedge_threshold_ = THRESH_M;

    if (upType == 3) {
      LOGIS_COF
          = std::max<float>(2.0 / (1 + log(maxOverflow + max_adj)), LOGIS_COF);
    } else {
      LOGIS_COF = std::max<float>(2.0 / (1 + log(maxOverflow)), LOGIS_COF);
    }

    if (i == 8) {
      L = 0;
      upType = 2;
      InitLastUsage(upType);
    }

    if (maxOverflow == 1) {
      // L = 0;
      ripup_threshold = -1;
      slope = 5;
    }

    if (maxOverflow > 300 && past_cong > 15000) {
      L = 0;
    }

    mazeRouteMSMD(i,
                  enlarge_,
                  costheight_,
                  ripup_threshold,
                  mazeedge_threshold_,
                  !(i % 3),
                  cost_type,
                  LOGIS_COF,
                  VIA,
                  slope,
                  L,
                  slack_th);
    int last_cong = past_cong;
    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    if (minofl > past_cong) {
      minofl = past_cong;
      minoflrnd = i;
    }

    if (i == 8) {
      L = 1;
    }

    i++;

    if (past_cong < 200 && i > 30 && upType == 2 && max_adj <= 20) {
      upType = 4;
      stopDEC = true;
    }

    if (maxOverflow < 150) {
      if (i == 20 && past_cong > 200) {
        if (verbose_) {
          logger_->info(GRT, 103, "Extra Run for hard benchmark.");
        }
        L = 0;
        upType = 3;
        stopDEC = true;
        slope = 5;
        mazeRouteMSMD(i,
                      enlarge_,
                      costheight_,
                      ripup_threshold,
                      mazeedge_threshold_,
                      !(i % 3),
                      cost_type,
                      LOGIS_COF,
                      VIA,
                      slope,
                      L,
                      slack_th);
        last_cong = past_cong;
        past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

        str_accu(12);
        L = 1;
        stopDEC = false;
        slope = 3;
        upType = 2;
      }
      if (i == 35 && tUsage > 800000) {
        str_accu(25);
      }
      if (i == 50 && tUsage > 800000) {
        str_accu(40);
      }
    }

    if (i > 50) {
      upType = 4;
      if (i > 70) {
        stopDEC = true;
      }
    }

    if (past_cong > 0.7 * last_cong) {
      costheight_ += CSTEP3;
    }

    if (past_cong >= last_cong) {
      VIA = 0;
    }

    if (past_cong < bmfl) {
      bwcnt = 0;
      if (i > 140 || (i > 80 && past_cong < 20)) {
        copyRS();
        bmfl = past_cong;

        L = 0;
        mazeRouteMSMD(i,
                      enlarge_,
                      costheight_,
                      ripup_threshold,
                      mazeedge_threshold_,
                      !(i % 3),
                      cost_type,
                      LOGIS_COF,
                      VIA,
                      slope,
                      L,
                      slack_th);
        last_cong = past_cong;
        past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);
        if (past_cong < last_cong) {
          copyRS();
          bmfl = past_cong;
        }
        L = 1;
        if (minofl > past_cong) {
          minofl = past_cong;
          minoflrnd = i;
        }
      }
    } else {
      bwcnt++;
    }

    if (bmfl > 10) {
      if (bmfl > 30 && bmfl < 72 && bwcnt > 50) {
        break;
      }
      if (bmfl < 30 && bwcnt > 50) {
        break;
      }
      if (i >= mazeRound) {
        getOverflow2Dmaze(&maxOverflow, &tUsage);
        break;
      }
    }

    if (i >= mazeRound) {
      getOverflow2Dmaze(&maxOverflow, &tUsage);
      break;
    }

    if (total_overflow_ > last_total_overflow) {
      overflow_increases++;
    }
    if (last_total_overflow > 0) {
      overflow_reduction_percent
          = std::max(overflow_reduction_percent,
                     1 - ((float) total_overflow_ / last_total_overflow));
    }

    last_total_overflow = total_overflow_;

    // generate DRC report each interval
    if (congestion_report_iter_step_ && i % congestion_report_iter_step_ == 0) {
      saveCongestion(i);
    }
  }  // end overflow iterations

  // Debug mode Tree 2D after overflow iterations
  if (debug_->isOn() && debug_->tree2D_) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net_) {
        StTreeVisualization(sttrees_[netID], nets_[netID], false);
      }
    }
  }

  has_2D_overflow_ = total_overflow_ > 0;

  if (minofl > 0) {
    debugPrint(logger_,
               GRT,
               "congestionIterations",
               1,
               "Minimal overflow {} occurring at round {}.",
               minofl,
               minoflrnd);
    copyBR();
  }

  if (overflow_increases > max_overflow_increases) {
    if (verbose_)
      logger_->warn(
          GRT,
          230,
          "Congestion iterations cannot increase overflow, reached the "
          "maximum number of times the total overflow can be increased.");
  }

  freeRR();

  removeLoops();

  getOverflow2Dmaze(&maxOverflow, &tUsage);

  layerAssignment();

  costheight_ = 3;
  via_cost_ = 1;

  if (goingLV && past_cong == 0) {
    mazeRouteMSMDOrder3D(enlarge_, 0, 20);
    mazeRouteMSMDOrder3D(enlarge_, 0, 12);
  }

  fillVIA();
  const int finallength = getOverflow3D();
  const int numVia = threeDVIA();
  checkRoute3D();

  if (verbose_) {
    logger_->info(GRT, 111, "Final number of vias: {}", numVia);
    logger_->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));
  }

  // Debug mode Tree 3D after layer assignament
  if (debug_->isOn() && debug_->tree3D_) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net_) {
        StTreeVisualization(sttrees_[netID], nets_[netID], true);
      }
    }
  }

  NetRouteMap routes = getRoutes();
  net_eo_.clear();
  net_ids_.clear();
  return routes;
}

void FastRouteCore::setVerbose(bool v)
{
  verbose_ = v;
}

void FastRouteCore::setCriticalNetsPercentage(float u)
{
  critical_nets_percentage_ = u;
}

void FastRouteCore::setMakeWireParasiticsBuilder(
    AbstractMakeWireParasitics* builder)
{
  parasitics_builder_ = builder;
}

void FastRouteCore::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
}

void FastRouteCore::setCongestionReportIterStep(int congestion_report_iter_step)
{
  congestion_report_iter_step_ = congestion_report_iter_step;
}

void FastRouteCore::setCongestionReportFile(const char* congestion_file_name)
{
  congestion_file_name_ = congestion_file_name;
}

void FastRouteCore::setGridMax(int x_max, int y_max)
{
  x_grid_max_ = x_max;
  y_grid_max_ = y_max;
}

std::vector<int> FastRouteCore::getOriginalResources()
{
  std::vector<int> original_resources(num_layers_);
  for (int l = 0; l < num_layers_; l++) {
    bool is_horizontal
        = layer_directions_[l] == odb::dbTechLayerDir::HORIZONTAL;
    if (is_horizontal) {
      if (!regular_y_) {
        original_resources[l] += (v_capacity_3D_[l] + h_capacity_3D_[l])
                                 * (y_grid_) * (x_grid_ - 1);
        original_resources[l] += (last_col_v_capacity_3D_[l] * y_grid_)
                                 + (last_row_h_capacity_3D_[l] * x_grid_);
      } else {
        original_resources[l]
            += (v_capacity_3D_[l] + h_capacity_3D_[l]) * (y_grid_) * (x_grid_);
      }
    } else {
      if (!regular_x_) {
        original_resources[l] += (v_capacity_3D_[l] + h_capacity_3D_[l])
                                 * (y_grid_ - 1) * (x_grid_);
        original_resources[l] += (last_col_v_capacity_3D_[l] * y_grid_)
                                 + (last_row_h_capacity_3D_[l] * x_grid_);
      } else {
        original_resources[l]
            += (v_capacity_3D_[l] + h_capacity_3D_[l]) * (y_grid_) * (x_grid_);
      }
    }
  }

  return original_resources;
}

void FastRouteCore::computeCongestionInformation()
{
  cap_per_layer_.resize(num_layers_);
  usage_per_layer_.resize(num_layers_);
  overflow_per_layer_.resize(num_layers_);
  max_h_overflow_.resize(num_layers_);
  max_v_overflow_.resize(num_layers_);

  for (int l = 0; l < num_layers_; l++) {
    cap_per_layer_[l] = 0;
    usage_per_layer_[l] = 0;
    overflow_per_layer_[l] = 0;
    max_h_overflow_[l] = 0;
    max_v_overflow_[l] = 0;

    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
        cap_per_layer_[l] += h_edges_3D_[l][i][j].cap;
        usage_per_layer_[l] += h_edges_3D_[l][i][j].usage;

        const int overflow
            = h_edges_3D_[l][i][j].usage - h_edges_3D_[l][i][j].cap;
        if (overflow > 0) {
          overflow_per_layer_[l] += overflow;
          max_h_overflow_[l] = std::max(max_h_overflow_[l], overflow);
        }
      }
    }
    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
        cap_per_layer_[l] += v_edges_3D_[l][i][j].cap;
        usage_per_layer_[l] += v_edges_3D_[l][i][j].usage;

        const int overflow
            = v_edges_3D_[l][i][j].usage - v_edges_3D_[l][i][j].cap;
        if (overflow > 0) {
          overflow_per_layer_[l] += overflow;
          max_v_overflow_[l] = std::max(max_v_overflow_[l], overflow);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////

const char* getNetName(odb::dbNet* db_net);

const char* FrNet::getName() const
{
  return getNetName(getDbNet());
}

////////////////////////////////////////////////////////////////
void FastRouteCore::setDebugOn(
    std::unique_ptr<AbstractFastRouteRenderer> renderer)
{
  debug_->renderer_ = std::move(renderer);
}
void FastRouteCore::setDebugSteinerTree(bool steinerTree)
{
  debug_->steinerTree_ = steinerTree;
}
void FastRouteCore::setDebugTree2D(bool tree2D)
{
  debug_->tree2D_ = tree2D;
}
void FastRouteCore::setDebugTree3D(bool tree3D)
{
  debug_->tree3D_ = tree3D;
}
void FastRouteCore::setDebugNet(const odb::dbNet* net)
{
  debug_->net_ = net;
}
void FastRouteCore::setDebugRectilinearSTree(bool rectiliniarSTree)
{
  debug_->rectilinearSTree_ = rectiliniarSTree;
}
void FastRouteCore::setSttInputFilename(const char* file_name)
{
  debug_->sttInputFileName_ = std::string(file_name);
}
bool FastRouteCore::hasSaveSttInput()
{
  return (debug_->sttInputFileName_ != "");
}
std::string FastRouteCore::getSttInputFileName()
{
  return debug_->sttInputFileName_;
}
const odb::dbNet* FastRouteCore::getDebugNet()
{
  return debug_->net_;
}

void FastRouteCore::steinerTreeVisualization(const stt::Tree& stree, FrNet* net)
{
  if (!debug_->isOn()) {
    return;
  }
  fastrouteRender()->highlight(net);
  fastrouteRender()->setIs3DVisualization(
      false);  //()isnt 3D because is steiner tree generated by stt
  fastrouteRender()->setSteinerTree(stree);
  fastrouteRender()->setTreeStructure(TreeStructure::steinerTreeByStt);
  fastrouteRender()->redrawAndPause();
}

void FastRouteCore::StTreeVisualization(const StTree& stree,
                                        FrNet* net,
                                        bool is3DVisualization)
{
  if (!debug_->isOn()) {
    return;
  }
  fastrouteRender()->highlight(net);
  fastrouteRender()->setIs3DVisualization(is3DVisualization);
  fastrouteRender()->setStTreeValues(stree);
  fastrouteRender()->setTreeStructure(TreeStructure::steinerTreeByFastroute);
  fastrouteRender()->redrawAndPause();
}

////////////////////////////////////////////////////////////////

int FrNet::getLayerEdgeCost(int layer) const
{
  if (edge_cost_per_layer_)
    return (*edge_cost_per_layer_)[layer];
  else
    return 1;
}

void FrNet::addPin(int x, int y, int layer)
{
  pin_x_.push_back(x);
  pin_y_.push_back(y);
  pin_l_.push_back(layer);
}

void FrNet::reset(odb::dbNet* db_net,
                  bool is_clock,
                  int driver_idx,
                  int edge_cost,
                  int min_layer,
                  int max_layer,
                  float slack,
                  std::vector<int>* edge_cost_per_layer)
{
  db_net_ = db_net;
  is_critical_ = false;
  is_clock_ = is_clock;
  driver_idx_ = driver_idx;
  edge_cost_ = edge_cost;
  min_layer_ = min_layer;
  max_layer_ = max_layer;
  slack_ = slack;
  edge_cost_per_layer_.reset(edge_cost_per_layer);
  pin_x_.clear();
  pin_y_.clear();
  pin_l_.clear();
}

}  // namespace grt
