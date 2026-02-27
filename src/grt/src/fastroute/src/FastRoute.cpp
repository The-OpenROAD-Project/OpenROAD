// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FastRoute.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "AbstractFastRouteRenderer.h"
#include "DataType.h"
#include "grt/GRoute.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

FastRouteCore::FastRouteCore(odb::dbDatabase* db,
                             utl::Logger* log,
                             utl::CallBackHandler* callback_handler,
                             stt::SteinerTreeBuilder* stt_builder,
                             sta::dbSta* sta)
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
      callback_handler_(callback_handler),
      logger_(log),
      stt_builder_(stt_builder),
      sta_(sta),
      debug_(new DebugSetting()),
      detour_penalty_(0)
{
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

  graph2d_.clear();
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

  detour_penalty_ = 0;
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
  } else {
    x_range_ = 1000;
  }
  y_range_ = x_range_;

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
  path_len_3D_.resize(boost::extents[num_layers_][y_range_][x_range_]);
}

void FastRouteCore::addVCapacity(int16_t verticalCapacity, int layer)
{
  v_capacity_3D_[layer - 1] = verticalCapacity;
  v_capacity_ += v_capacity_3D_[layer - 1];
}

void FastRouteCore::addHCapacity(int16_t horizontalCapacity, int layer)
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
                             bool is_local,
                             int driver_idx,
                             int8_t cost,
                             int min_layer,
                             int max_layer,
                             float slack,
                             std::vector<int8_t>* edge_cost_per_layer)
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
  // Don't add local nets to the list of ids that will be routed. It is only
  // necessary to add them to make mergeNet work with local nets.
  if (!is_local) {
    net_ids_.push_back(netID);
  }

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

void FastRouteCore::mergeNet(odb::dbNet* removed_net, odb::dbNet* preserved_net)
{
  if (db_net_id_map_.find(removed_net) != db_net_id_map_.end()) {
    const int removed_net_id = db_net_id_map_[removed_net];
    auto& removed_nodes = sttrees_[removed_net_id].nodes;
    auto& removed_edges = sttrees_[removed_net_id].edges;

    if (db_net_id_map_.find(preserved_net) != db_net_id_map_.end()) {
      const int preserved_net_id = db_net_id_map_[preserved_net];
      sttrees_[preserved_net_id].num_terminals
          += sttrees_[removed_net_id].num_terminals;
      auto& preserved_nodes = sttrees_[preserved_net_id].nodes;
      auto& preserved_edges = sttrees_[preserved_net_id].edges;

      preserved_nodes.insert(
          preserved_nodes.end(), removed_nodes.begin(), removed_nodes.end());
      preserved_edges.insert(
          preserved_edges.end(), removed_edges.begin(), removed_edges.end());
    } else {
      logger_->error(
          utl::GRT,
          13,
          "Net {} is not present in FastRouteCore structures when trying to "
          "merge with net {}",
          preserved_net->getName(),
          removed_net->getName());
    }

    removed_nodes.clear();
    removed_edges.clear();
    deleteNet(removed_net);
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

void FastRouteCore::clearNDRnets()
{
  graph2d_.clearNDRnets();
}

void FastRouteCore::initEdges()
{
  // allocate memory and initialize for edges
  graph2d_.init(x_grid_, y_grid_, num_layers_, logger_);

  init3DEdges();
}

void FastRouteCore::init3DEdges()
{
  v_edges_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  h_edges_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        h_edges_3D_[k][i][j].cap = 0;
        h_edges_3D_[k][i][j].usage = 0;
        h_edges_3D_[k][i][j].red = 0;
      }
    }
  }
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        v_edges_3D_[k][i][j].cap = 0;
        v_edges_3D_[k][i][j].usage = 0;
        v_edges_3D_[k][i][j].red = 0;
      }
    }
  }
}

void FastRouteCore::initLowerBoundCapacities()
{
  const float LB = 0.9;
  v_capacity_lb_ = LB * v_capacity_;
  h_capacity_lb_ = LB * h_capacity_;
}

void FastRouteCore::setEdgeCapacity(int x1,
                                    int y1,
                                    int x2,
                                    int y2,
                                    int layer,
                                    int capacity)
{
  const int k = layer - 1;

  if (y1 == y2) {
    graph2d_.addCapH(x1, y1, capacity);
    h_edges_3D_[k][y1][x1].cap = capacity;
  } else if (x1 == x2) {
    graph2d_.addCapV(x1, y1, capacity);
    v_edges_3D_[k][y1][x1].cap = capacity;
  }
}

// Useful to prevent NDR nets to be assigned to 3D edges with insufficient
// capacity. Need to be initialized after all the adjustments
void FastRouteCore::initEdgesCapacityPerLayer()
{
  graph2d_.initCap3D();

  for (int y = 0; y < y_grid_; y++) {
    for (int x = 0; x < x_grid_; x++) {
      for (int l = 0; l < num_layers_; l++) {
        if (x < x_grid_ - 1) {
          graph2d_.updateCap3D(
              x, y, l, EdgeDirection::Horizontal, h_edges_3D_[l][y][x].cap);
        }
        if (y < y_grid_ - 1) {
          graph2d_.updateCap3D(
              x, y, l, EdgeDirection::Vertical, v_edges_3D_[l][y][x].cap);
        }
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
        if (verbose_) {
          logger_->warn(GRT,
                        113,
                        "Underflow in reduce: cap, reducedCap: {}, {}",
                        cap,
                        reducedCap);
        }
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    h_edges_3D_[k][y1][x1].cap = reducedCap;

    if (!isReduce) {
      const int increase = reducedCap - cap;
      if (x1 < x_grid_ - 1) {
        graph2d_.addCapH(x1, y1, increase);
        graph2d_.addRedH(x1, y1, -increase);
      }
      int new_red_3D = h_edges_3D_[k][y1][x1].red - increase;
      h_edges_3D_[k][y1][x1].red = std::max(new_red_3D, 0);
    } else {
      h_edges_3D_[k][y1][x1].red += reduce;
    }

    if (x1 < x_grid_ - 1) {
      graph2d_.addCapH(x1, y1, -reduce);
      graph2d_.addRedH(x1, y1, reduce);
    }

  } else if (x1 == x2) {  // vertical edge
    const int cap = v_edges_3D_[k][y1][x1].cap;
    int reduce;

    if (cap - reducedCap < 0) {
      if (isReduce) {
        if (verbose_) {
          logger_->warn(GRT,
                        114,
                        "Underflow in reduce: cap, reducedCap: {}, {}",
                        cap,
                        reducedCap);
        }
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    v_edges_3D_[k][y1][x1].cap = reducedCap;

    if (!isReduce) {
      int increase = reducedCap - cap;
      if (y1 < y_grid_ - 1) {
        graph2d_.addCapV(x1, y1, increase);
        graph2d_.addRedV(x1, y1, -increase);
      }
      int new_red_3D = v_edges_3D_[k][y1][x1].red - increase;
      v_edges_3D_[k][y1][x1].red = std::max(new_red_3D, 0);
    } else {
      v_edges_3D_[k][y1][x1].red += reduce;
    }

    if (y1 < y_grid_ - 1) {
      graph2d_.addCapV(x1, y1, -reduce);
      graph2d_.addRedV(x1, y1, reduce);
    }
  }
}

void FastRouteCore::saveResourcesBeforeAdjustments()
{
  // Save real horizontal resources
  for (int x = 0; x < x_grid_ - 1; x++) {
    for (int y = 0; y < y_grid_; y++) {
      graph2d_.saveResources(x, y, true);
      for (int l = 0; l < num_layers_; l++) {
        h_edges_3D_[l][y][x].real_cap = h_edges_3D_[l][y][x].cap;
      }
    }
  }
  // Save real vertical resources
  for (int x = 0; x < x_grid_; x++) {
    for (int y = 0; y < y_grid_ - 1; y++) {
      graph2d_.saveResources(x, y, false);
      for (int l = 0; l < num_layers_; l++) {
        v_edges_3D_[l][y][x].real_cap = v_edges_3D_[l][y][x].cap;
      }
    }
  }
}

void FastRouteCore::releaseResourcesOnInterval(
    int x,
    int y,
    int layer,
    bool is_horizontal,
    const interval<int>::type& tile_reduce_interval,
    const std::vector<int>& track_space)
{
  int edge_cap;
  // Get capacity on the position
  if (is_horizontal) {
    edge_cap = getEdgeCapacity(x, y, x + 1, y, layer);
  } else {
    edge_cap = getEdgeCapacity(x, y, x, y + 1, layer);
  }
  // Get total of resources to release
  int increase = 0;
  if (layer > 0 && layer <= track_space.size()) {
    increase
        = std::ceil(static_cast<float>(std::abs(tile_reduce_interval.upper()
                                                - tile_reduce_interval.lower()))
                    / track_space[layer - 1]);
  }
  // increase resource
  edge_cap += increase;
  if (is_horizontal) {
    addAdjustment(x, y, x + 1, y, layer, edge_cap, false);
  } else {
    addAdjustment(x, y, x, y + 1, layer, edge_cap, false);
  }
}

void FastRouteCore::addVerticalAdjustments(
    const odb::Point& first_tile,
    const odb::Point& last_tile,
    const int layer,
    const interval<int>::type& first_tile_reduce_interval,
    const interval<int>::type& last_tile_reduce_interval,
    const std::vector<int>& track_space,
    bool release)
{
  // Add intervals to set or release resources for each tile
  for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y < last_tile.getY(); y++) {
      if (x == first_tile.getX()) {
        if (release) {
          releaseResourcesOnInterval(
              x, y, layer, false, first_tile_reduce_interval, track_space);
        } else {
          vertical_blocked_intervals_[std::make_tuple(x, y, layer)]
              += first_tile_reduce_interval;
        }
      } else if (x == last_tile.getX()) {
        if (release) {
          releaseResourcesOnInterval(
              x, y, layer, false, last_tile_reduce_interval, track_space);
        } else {
          vertical_blocked_intervals_[std::make_tuple(x, y, layer)]
              += last_tile_reduce_interval;
        }
      } else {
        // Restore capacity removed by blockage
        if (release) {
          int reduced = v_edges_3D_[layer - 1][y][x].red;
          addAdjustment(x, y, x, y + 1, layer, reduced, false);
        } else {
          addAdjustment(x, y, x, y + 1, layer, 0, true);
        }
      }
    }
  }
}

void FastRouteCore::addHorizontalAdjustments(
    const odb::Point& first_tile,
    const odb::Point& last_tile,
    const int layer,
    const interval<int>::type& first_tile_reduce_interval,
    const interval<int>::type& last_tile_reduce_interval,
    const std::vector<int>& track_space,
    bool release)
{
  // Add intervals to set or release resources for each tile
  for (int x = first_tile.getX(); x < last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
      if (y == first_tile.getY()) {
        if (release) {
          releaseResourcesOnInterval(
              x, y, layer, true, first_tile_reduce_interval, track_space);
        } else {
          horizontal_blocked_intervals_[std::make_tuple(x, y, layer)]
              += first_tile_reduce_interval;
        }
      } else if (y == last_tile.getY()) {
        if (release) {
          releaseResourcesOnInterval(
              x, y, layer, true, last_tile_reduce_interval, track_space);
        } else {
          horizontal_blocked_intervals_[std::make_tuple(x, y, layer)]
              += last_tile_reduce_interval;
        }
      } else {
        // Restore capacity removed by blockage
        if (release) {
          int reduced = h_edges_3D_[layer - 1][y][x].red;
          addAdjustment(x, y, x + 1, y, layer, reduced, false);
        } else {
          addAdjustment(x, y, x + 1, y, layer, 0, true);
        }
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
      edge_cap = std::max(edge_cap, 0);
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
      edge_cap = std::max(edge_cap, 0);
      addAdjustment(x, y, x + 1, y, layer, edge_cap, true);
    }
  }
}

int FastRouteCore::getAvailableResources(int x1,
                                         int y1,
                                         int x2,
                                         int y2,
                                         int layer)
{
  const int k = layer - 1;
  int available_cap = 0;
  if (y1 == y2) {  // horizontal edge
    available_cap = h_edges_3D_[k][y1][x1].cap - h_edges_3D_[k][y1][x1].usage;
  } else if (x1 == x2) {  // vertical edge
    available_cap = v_edges_3D_[k][y1][x1].cap - v_edges_3D_[k][y1][x1].usage;
  } else {
    logger_->error(
        GRT,
        213,
        "Cannot get available resources: edge is not vertical or horizontal.");
  }
  return available_cap;
}

int FastRouteCore::getEdgeCapacity(int x1, int y1, int x2, int y2, int layer)
{
  const int k = layer - 1;

  if (y1 == y2) {  // horizontal edge
    return h_edges_3D_[k][y1][x1].cap;
  }
  if (x1 == x2) {  // vertical edge
    return v_edges_3D_[k][y1][x1].cap;
  }
  logger_->error(
      GRT,
      214,
      "Cannot get edge capacity: edge is not vertical or horizontal.");
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

void FastRouteCore::updateEdge2DAnd3DUsage(int x1,
                                           int y1,
                                           int x2,
                                           int y2,
                                           int layer,
                                           int used,
                                           odb::dbNet* db_net)
{
  const int k = layer - 1;
  FrNet* net = nullptr;
  int net_id;
  bool exists;
  getNetId(db_net, net_id, exists);

  net = nets_[net_id];

  int8_t layer_edge_cost = net->getLayerEdgeCost(k);
  int8_t edge_cost = net->getEdgeCost();

  if (y1 == y2) {  // horizontal edge
    graph2d_.updateUsageH({x1, x2}, y1, net, used * edge_cost);
    for (int x = x1; x < x2; x++) {
      h_edges_3D_[k][y1][x].usage += used * layer_edge_cost;
    }
  } else if (x1 == x2) {  // vertical edge
    graph2d_.updateUsageV(x1, {y1, y2}, net, used * edge_cost);
    for (int y = y1; y < y2; y++) {
      v_edges_3D_[k][y][x1].usage += used * layer_edge_cost;
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
        const std::vector<GPoint3D>& grids = treeedge->route.grids;
        int lastX = tile_size_ * (grids[0].x + 0.5) + x_corner_;
        int lastY = tile_size_ * (grids[0].y + 0.5) + y_corner_;
        int lastL = grids[0].layer;
        for (int i = 1; i <= routeLen; i++) {
          const int xreal = tile_size_ * (grids[i].x + 0.5) + x_corner_;
          const int yreal = tile_size_ * (grids[i].y + 0.5) + y_corner_;

          GSegment segment = GSegment(
              lastX, lastY, lastL + 1, xreal, yreal, grids[i].layer + 1);

          lastX = xreal;
          lastY = yreal;
          lastL = grids[i].layer;
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

// Updates the layer assignment for specific route segments after repair
// antennas. This function is called after jumper insertion during antenna
// violation repair. When a jumper is inserted to fix an antenna violation,
// certain route segments need to be moved to a different layer. This function
// searches through all edges of the specified net and updates the layer
// assignment for any route points that fall within the specified region.
void FastRouteCore::updateRouteGridsLayer(int x1,
                                          int y1,
                                          int x2,
                                          int y2,
                                          int layer,
                                          int new_layer,
                                          odb::dbNet* db_net)
{
  // Get the internal net ID from the database net object
  int net_id;
  bool exists;
  getNetId(db_net, net_id, exists);

  // Access the routing tree edges for this net
  std::vector<TreeEdge>& treeedges = sttrees_[net_id].edges;
  const int num_edges = sttrees_[net_id].num_edges();

  // Iterate through all edges in the net's routing tree
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    TreeEdge* treeedge = &(treeedges[edgeID]);
    // Only process edges that have actual routing
    if (treeedge->len > 0 || treeedge->route.routelen > 0) {
      int routeLen = treeedge->route.routelen;
      std::vector<GPoint3D>& grids = treeedge->route.grids;

      // If the point is within the specified rectangular region AND on the
      // original layer
      for (int i = 0; i <= routeLen; i++) {
        if (grids[i].x >= x1 && grids[i].x <= x2 && grids[i].y >= y1
            && grids[i].y <= y2 && grids[i].layer == layer) {
          // Update to the new layer
          grids[i].layer = new_layer;
        }
      }
    }
  }
}

int FastRouteCore::getDbNetLayerEdgeCost(odb::dbNet* db_net, int layer)
{
  int net_id;
  bool exists;
  getNetId(db_net, net_id, exists);

  return nets_[net_id]->getLayerEdgeCost(layer - 1);
}

void FastRouteCore::getPlanarRoute(odb::dbNet* db_net, GRoute& route)
{
  int netID;
  bool exists;
  getNetId(db_net, netID, exists);

  std::unordered_set<GSegment, GSegmentHash> net_segs;

  const auto& treeedges = sttrees_[netID].edges;
  const int num_edges = sttrees_[netID].num_edges();

  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      int routeLen = treeedge->route.routelen;
      const std::vector<GPoint3D>& grids = treeedge->route.grids;
      int lastX = (tile_size_ * (grids[0].x + 0.5)) + x_corner_;
      int lastY = (tile_size_ * (grids[0].y + 0.5)) + y_corner_;

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
      int second_x = (tile_size_ * (grids[1].x + 0.5)) + x_corner_;
      int lastL = (lastX == second_x) ? layer_v : layer_h;

      for (int i = 1; i <= routeLen; i++) {
        const int xreal = (tile_size_ * (grids[i].x + 0.5)) + x_corner_;
        const int yreal = (tile_size_ * (grids[i].y + 0.5)) + y_corner_;
        GSegment segment;
        if (lastX == xreal) {
          // if change direction add a via to change the layer
          if (lastL == layer_h) {
            segment
                = GSegment(lastX, lastY, lastL + 1, lastX, lastY, layer_v + 1);
            if (net_segs.find(segment) == net_segs.end()) {
              net_segs.insert(segment);
              route.push_back(segment);
            }
          }
          lastL = layer_v;
          segment = GSegment(lastX, lastY, lastL + 1, xreal, yreal, lastL + 1);
        } else {
          // if change direction add a via to change the layer
          if (lastL == layer_v) {
            segment
                = GSegment(lastX, lastY, lastL + 1, lastX, lastY, layer_h + 1);
            if (net_segs.find(segment) == net_segs.end()) {
              net_segs.insert(segment);
              route.push_back(segment);
            }
          }
          lastL = layer_h;
          segment = GSegment(lastX, lastY, lastL + 1, xreal, yreal, lastL + 1);
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

void FastRouteCore::get3DRoute(odb::dbNet* db_net, GRoute& route)
{
  int netID;
  bool exists;
  getNetId(db_net, netID, exists);

  std::unordered_set<GSegment, GSegmentHash> net_segs;

  const auto& treeedges = sttrees_[netID].edges;
  const int num_edges = sttrees_[netID].num_edges();

  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      int routeLen = treeedge->route.routelen;
      const std::vector<GPoint3D>& grids = treeedge->route.grids;
      const int num_terminals = sttrees_[netID].num_terminals;
      const auto& treenodes = sttrees_[netID].nodes;
      int node1_alias = treeedge->n1a;
      int node2_alias = treeedge->n2a;

      std::vector<GPoint3D> filled_grids;

      // Handle vias for node1_alias (start node)
      if (node1_alias < num_terminals || treenodes[node1_alias].hID == edgeID
          || (edgeID == treenodes[node1_alias].lID
              && treenodes[node1_alias].hID == BIG_INT)) {
        int16_t bottom_layer = treenodes[node1_alias].botL;
        int16_t top_layer = treenodes[node1_alias].topL;
        int16_t edge_init_layer = grids[0].layer;

        if (node1_alias < num_terminals) {
          int16_t pin_botL, pin_topL;
          getViaStackRange(netID, node1_alias, pin_botL, pin_topL);
          bottom_layer = std::min(pin_botL, bottom_layer);
          top_layer = std::max(pin_topL, top_layer);

          for (int16_t l = bottom_layer; l < top_layer; l++) {
            filled_grids.push_back({grids[0].x, grids[0].y, l});
          }

          for (int16_t l = top_layer; l > edge_init_layer; l--) {
            filled_grids.push_back({grids[0].x, grids[0].y, l});
          }
        } else {
          for (int16_t l = bottom_layer; l < edge_init_layer; l++) {
            filled_grids.push_back({grids[0].x, grids[0].y, l});
          }
        }
      }

      for (int j = 0; j <= routeLen; j++) {
        filled_grids.emplace_back(grids[j]);
      }

      // Handle vias for node2_alias (end node)
      if (node2_alias < num_terminals || treenodes[node2_alias].hID == edgeID
          || (edgeID == treenodes[node2_alias].lID
              && treenodes[node2_alias].hID == BIG_INT)) {
        int16_t bottom_layer = treenodes[node2_alias].botL;
        int16_t top_layer = treenodes[node2_alias].topL;
        if (node2_alias < num_terminals) {
          int16_t pin_botL, pin_topL;
          getViaStackRange(netID, node2_alias, pin_botL, pin_topL);
          bottom_layer = std::min(pin_botL, bottom_layer);
          top_layer = std::max(pin_topL, top_layer);

          // Adjust bottom_layer if it's the same as the last filled grid layer
          if (bottom_layer == filled_grids.back().layer) {
            bottom_layer++;
          }

          // Ensure the loop for descending vias is correct
          for (int16_t l = filled_grids.back().layer - 1; l > bottom_layer;
               l--) {
            filled_grids.push_back(
                {filled_grids.back().x, filled_grids.back().y, l});
          }

          for (int16_t l = bottom_layer; l <= top_layer; l++) {
            filled_grids.push_back(
                {filled_grids.back().x, filled_grids.back().y, l});
          }
        } else {
          for (int16_t l = top_layer - 1; l >= bottom_layer; l--) {
            filled_grids.push_back(
                {filled_grids.back().x, filled_grids.back().y, l});
          }
        }
      }

      int lastX = (tile_size_ * (filled_grids[0].x + 0.5)) + x_corner_;
      int lastY = (tile_size_ * (filled_grids[0].y + 0.5)) + y_corner_;
      int lastL = filled_grids[0].layer;

      for (int i = 1; i < filled_grids.size(); i++) {
        const int xreal = (tile_size_ * (filled_grids[i].x + 0.5)) + x_corner_;
        const int yreal = (tile_size_ * (filled_grids[i].y + 0.5)) + y_corner_;
        const int currentL = filled_grids[i].layer;

        // Prevent adding segments that are effectively zero-length vias on the
        // same layer
        if (lastX == xreal && lastY == yreal && lastL == currentL) {
          // Skip this segment as it's a redundant via on the same layer
          lastX = xreal;
          lastY = yreal;
          lastL = currentL;
          continue;
        }

        GSegment segment
            = GSegment(lastX, lastY, lastL + 1, xreal, yreal, currentL + 1);
        segment.setIs3DRoute(true);

        // Only add segment if it's not a duplicate
        if (net_segs.find(segment) == net_segs.end()) {
          net_segs.insert(segment);
          route.push_back(segment);
        }

        lastX = xreal;
        lastY = yreal;
        lastL = currentL;
      }
    }
  }
}

NetRouteMap FastRouteCore::getPlanarRoutes()
{
  NetRouteMap routes;

  // Get routes before layer assignment
  if (!is_3d_step_) {
    for (const int& netID : net_ids_) {
      auto fr_net = nets_[netID];
      odb::dbNet* db_net = fr_net->getDbNet();
      GRoute& route = routes[db_net];
      getPlanarRoute(db_net, route);
    }
  } else {
    for (const int& netID : net_ids_) {
      auto fr_net = nets_[netID];
      odb::dbNet* db_net = fr_net->getDbNet();
      GRoute& route = routes[db_net];
      get3DRoute(db_net, route);
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
  if (db_gcell) {
    db_gcell->resetGrid();
  } else {
    db_gcell = odb::dbGCellGrid::create(block);
  }

  db_gcell->addGridPatternX(x_corner_, x_grid_, tile_size_);
  db_gcell->addGridPatternY(y_corner_, y_grid_, tile_size_);
  auto db_tech = db_->getTech();
  for (int k = min_routing_layer - 1; k <= max_routing_layer - 1; k++) {
    auto layer = db_tech->findRoutingLayer(k + 1);
    if (layer == nullptr) {
      continue;
    }

    bool is_horizontal
        = layer_directions_[k] == odb::dbTechLayerDir::HORIZONTAL;
    if (is_horizontal) {
      int last_cell_cap_h = 0;
      for (int y = 0; y < y_grid_; y++) {
        for (int x = 0; x < x_grid_; x++) {
          const uint8_t capH
              = x == x_grid_ - 1
                    ? last_cell_cap_h
                    : h_edges_3D_[k][y][x].cap + h_edges_3D_[k][y][x].red;
          db_gcell->setCapacity(layer, x, y, capH);
          last_cell_cap_h = capH;
        }
      }
    } else {
      int last_cell_cap_v = 0;
      for (int x = 0; x < x_grid_; x++) {
        for (int y = 0; y < y_grid_; y++) {
          const uint8_t capV
              = y == y_grid_ - 1
                    ? last_cell_cap_v
                    : v_edges_3D_[k][y][x].cap + v_edges_3D_[k][y][x].red;
          db_gcell->setCapacity(layer, x, y, capV);
          last_cell_cap_v = capV;
        }
      }
    }

    for (int y = 0; y < y_grid_; y++) {
      for (int x = 0; x < x_grid_; x++) {
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
    bool is_horizontal
        = layer_directions_[k] == odb::dbTechLayerDir::HORIZONTAL;
    if (is_horizontal) {
      int last_cell_cap_h = 0;
      for (int y = 0; y < y_grid_; y++) {
        for (int x = 0; x < x_grid_; x++) {
          const uint8_t cap_h
              = x == x_grid_ - 1
                    ? last_cell_cap_h
                    : h_edges_3D_[k][y][x].cap + h_edges_3D_[k][y][x].red;
          cap_red_data[x][y].capacity += cap_h;
          last_cell_cap_h = cap_h;
        }
      }
    } else {
      int last_cell_cap_v = 0;
      for (int x = 0; x < x_grid_; x++) {
        for (int y = 0; y < y_grid_; y++) {
          const uint8_t cap_v
              = y == y_grid_ - 1
                    ? last_cell_cap_v
                    : v_edges_3D_[k][y][x].cap + v_edges_3D_[k][y][x].red;
          cap_red_data[x][y].capacity += cap_v;
          last_cell_cap_v = cap_v;
        }
      }
    }

    for (int x = 0; x < x_grid_; x++) {
      for (int y = 0; y < y_grid_; y++) {
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

  graph2d_.clearUsed();
  preProcessTechLayers();

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
  const bool noADJ = false;
  const int thStep1 = 10;
  const int thStep2 = 4;
  const int LVIter = 3;
  const int mazeRound = 500;
  int bmfl = BIG_INT;
  int minofl = BIG_INT;
  float logistic_coef = 0;
  int slope;
  int max_adj;
  int long_edge_len = 40;
  int short_edge_len = 12;
  const int soft_ndr_overflow_th = 10000;

  // call FLUTE to generate RSMT and break the nets into segments (2-pin nets)
  via_cost_ = 0;
  gen_brk_RSMT(false, false, false, false, noADJ);
  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After RSMT");
  }

  // First time L routing
  routeLAll(true);
  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After routeLAll");
  }

  // Congestion-driven rip-up and reroute L
  gen_brk_RSMT(true, true, true, false, noADJ);
  getOverflow2D(&maxOverflow);
  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After congestion-driven RSMT");
  }

  // New rip-up and reroute L via-guided
  newrouteLAll(false, true);
  getOverflow2D(&maxOverflow);
  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After newRouteLAll");
  }

  // Rip-up and reroute using spiral route
  spiralRouteAll();
  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After spiralRouteAll");
  }

  // Rip-up a tree edge according to its ripup type and Z-route it
  newrouteZAll(10);
  int past_cong = getOverflow2D(&maxOverflow);

  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    logger_->report("After newRouteZAll");
  }

  convertToMazeroute();

  int enlarge_ = 10;
  int newTH = 10;
  bool stopDEC = false;
  int upType = 1;

  costheight_ = COSHEIGHT;
  if (maxOverflow > 700) {
    costheight_ = 8;
    logistic_coef = 1.33;
    VIA = 0;
    THRESH_M = 0;
    CSTEP1 = 30;
  }

  for (int i = 0; i < LVIter; i++) {
    logistic_coef = 2.0 / (1 + log(maxOverflow));
    debugPrint(logger_,
               GRT,
               "patternRouting",
               1,
               "LV routing round {}, enlarge {}.",
               i,
               enlarge_);
    routeMonotonicAll(newTH, enlarge_, logistic_coef);

    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    enlarge_ += 5;
    newTH -= 5;
    newTH = std::max(newTH, 1);
  }

  graph2d_.InitEstUsage();

  int i = 1;
  costheight_ = COSHEIGHT;
  enlarge_ = ENLARGE;
  int ripup_threshold = Ripvalue;

  minofl = total_overflow_;
  stopDEC = false;

  slope = 20;
  L = 1;

  graph2d_.InitLastUsage(upType);
  if (total_overflow_ > 0 && overflow_iterations_ > 0 && verbose_) {
    logger_->info(GRT, 101, "Running extra iterations to remove overflow.");
  }

  // debug mode Rectilinear Steiner Tree before overflow iterations
  if (debug_->isOn() && debug_->rectilinearSTree) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net) {
        logger_->report("RST Tree before overflow iterations");
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
    if (verbose_) {
      logger_->info(
          GRT, 102, "Start extra iteration {}/{}", i, overflow_iterations_);
    }

    if (THRESH_M > 15) {
      THRESH_M -= thStep1;
    } else if (THRESH_M >= 2) {
      THRESH_M -= thStep2;
    } else {
      THRESH_M = 0;
    }
    THRESH_M = std::max(THRESH_M, 0);

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
    graph2d_.updateCongestionHistory(upType, ahth_, stopDEC, max_adj);

    if (total_overflow_ > 15000 && maxOverflow > 400) {
      enlarge_ = std::max(x_grid_, y_grid_) / 30;
      slope = BIG_INT;
      if (i == 5) {
        VIA = 0;
        logistic_coef = 1.33;
        ripup_threshold = -1;
      } else if (i > 6) {
        if (i % 2 == 0) {
          logistic_coef += 0.5;
        }
      }
      if (i > 10) {
        ripup_threshold = 0;
      }
    }

    enlarge_ = std::min(enlarge_, x_grid_ / 2);
    costheight_ += cost_step;
    mazeedge_threshold_ = THRESH_M;

    if (upType == 3) {
      logistic_coef = std::max<float>(2.0 / (1 + log(maxOverflow + max_adj)),
                                      logistic_coef);
    } else {
      logistic_coef
          = std::max<float>(2.0 / (1 + log(maxOverflow)), logistic_coef);
    }

    if (i == 8) {
      L = 0;
      upType = 2;
      graph2d_.InitLastUsage(upType);
    }

    if (maxOverflow == 1) {
      ripup_threshold = -1;
      slope = 5;
    }

    if (maxOverflow > 300 && past_cong > 15000) {
      L = 0;
    }

    auto cost_params = CostParams(logistic_coef, costheight_, slope);
    mazeRouteMSMD(i,
                  enlarge_,
                  ripup_threshold,
                  mazeedge_threshold_,
                  !(i % 3),
                  VIA,
                  L,
                  cost_params,
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
        L = 0;
        upType = 3;
        stopDEC = true;
        slope = 5;
        auto cost_params = CostParams(logistic_coef, costheight_, slope);
        mazeRouteMSMD(i,
                      enlarge_,
                      ripup_threshold,
                      mazeedge_threshold_,
                      !(i % 3),
                      VIA,
                      L,
                      cost_params,
                      slack_th);
        last_cong = past_cong;
        past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

        graph2d_.str_accu(12);
        L = 1;
        stopDEC = false;
        slope = 3;
        upType = 2;
      }
      if (i == 35 && tUsage > 800000) {
        graph2d_.str_accu(25);
      }
      if (i == 50 && tUsage > 800000) {
        graph2d_.str_accu(40);
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
        auto cost_params = CostParams(logistic_coef, costheight_, slope);
        mazeRouteMSMD(i,
                      enlarge_,
                      ripup_threshold,
                      mazeedge_threshold_,
                      !(i % 3),
                      VIA,
                      L,
                      cost_params,
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

    if (logger_->debugCheck(GRT, "congestionIterations", 1)) {
      logger_->report(
          "=== Overflow Iteration {} - TotalOverflow {} - OverflowIter {} - "
          "OverflowIncreases {} - MaxOverIncr {} ===",
          i,
          total_overflow_,
          overflow_iterations_,
          overflow_increases,
          max_overflow_increases);
    }

    // Try disabling NDR nets to fix congestion
    if (total_overflow_ > 0
        && (i == overflow_iterations_
            || overflow_increases == max_overflow_increases)) {
      // Compute all the NDR nets involved in congestion
      computeCongestedNDRnets();

      std::vector<int> net_ids;

      // If the congestion is not that high (note that the overflow is inflated
      // by 100x when there is no capacity available for a NDR net in a specific
      // edge)
      if (total_overflow_ < soft_ndr_overflow_th) {
        // Select one NDR net to be disabled
        int net_id = graph2d_.getOneCongestedNDRnet();
        if (net_id != -1) {
          net_ids.push_back(net_id);
        }
      } else {  // Select multiple NDR nets
        net_ids = graph2d_.getMultipleCongestedNDRnet();
      }

      // Only apply soft NDR if there is NDR nets involved in congestion
      if (!net_ids.empty()) {
        // Apply the soft NDR to the selected list of nets
        applySoftNDR(net_ids);

        // Reset loop parameters
        overflow_increases = 0;
        i = 1;
        costheight_ = COSHEIGHT;
        enlarge_ = ENLARGE;
        ripup_threshold = Ripvalue;
        minofl = total_overflow_;
        bmfl = minofl;
        stopDEC = false;

        slope = 20;
        L = 1;

        // Increase maze route 3D threshold to fix bad routes
        long_edge_len = BIG_INT;
      }
    }

    // generate DRC report each interval
    if (congestion_report_iter_step_ && i % congestion_report_iter_step_ == 0) {
      saveCongestion(i);
    }
  }  // end overflow iterations

  // Debug mode Tree 2D after overflow iterations
  if (debug_->isOn() && debug_->tree2D) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net) {
        logger_->report("Tree 2D after overflow iterations");
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
    if (verbose_) {
      logger_->warn(
          GRT,
          230,
          "Congestion iterations cannot increase overflow, reached the "
          "maximum number of times the total overflow can be increased.");
    }
  }

  freeRR();

  removeLoops();

  getOverflow2Dmaze(&maxOverflow, &tUsage);

  layerAssignment();

  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    getOverflow3D();
    logger_->report("After LayerAssignment - 2D/3D cong: {}/{}",
                    past_cong,
                    total_overflow_);
  }

  costheight_ = 3;
  via_cost_ = 1;

  if (past_cong == 0) {
    // Increase ripup threshold if res-aware is enabled
    if (enable_resistance_aware_) {
      long_edge_len = BIG_INT;
      short_edge_len = BIG_INT;
    }

    mazeRouteMSMDOrder3D(enlarge_, 0, long_edge_len);
    mazeRouteMSMDOrder3D(enlarge_, 0, short_edge_len);
  }

  // Disable estimate parasitics for grt incremental steps with resistance-aware
  // strategy to prevent issues during repair design and repair timing
  en_estimate_parasitics_ = false;

  if (logger_->debugCheck(GRT, "grtSteps", 1)) {
    getOverflow3D();
    logger_->report("After MazeRoute3D - 3Dcong: {}", total_overflow_);
  }

  fillVIA();
  const int finallength = getOverflow3D();
  const int numVia = threeDVIA();
  checkRoute3D();
  ensurePinCoverage();

  logger_->metric("global_route__vias", numVia);
  if (verbose_) {
    logger_->info(GRT, 111, "Final number of vias: {}", numVia);
    logger_->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));
  }

  // Debug mode Tree 3D after layer assignament
  if (debug_->isOn() && debug_->tree3D) {
    for (const int& netID : net_ids_) {
      if (nets_[netID]->getDbNet() == debug_->net) {
        logger_->report("Tree 3D after maze route 3D");
        StTreeVisualization(sttrees_[netID], nets_[netID], true);
      }
    }
  }

  NetRouteMap routes = getRoutes();
  net_ids_.clear();
  return routes;
}

void FastRouteCore::applySoftNDR(const std::vector<int>& net_ids)
{
  for (auto net_id : net_ids) {
    logger_->warn(GRT,
                  273,
                  "Disabled NDR (to reduce congestion) for net: {}",
                  nets_[net_id]->getName());

    // Remove the usage of all the edges involved with this net
    updateSoftNDRNetUsage(net_id, -nets_[net_id]->getEdgeCost());

    // Reset the edge cost and layer edge cost to 1
    setSoftNDR(net_id);

    // Update the usage of all the edges involved with this net considering
    // the new edge cost
    updateSoftNDRNetUsage(net_id, nets_[net_id]->getEdgeCost());
  }
}

void FastRouteCore::setSoftNDR(const int net_id)
{
  nets_[net_id]->setIsSoftNDR(true);
  nets_[net_id]->setEdgeCost(1);
}

void FastRouteCore::computeCongestedNDRnets()
{
  // Clear the old list first
  graph2d_.clearCongestedNDRnets();

  // Compute all NDR nets to identify those in congestion
  for (auto net_id : net_ids_) {
    FrNet* net = nets_[net_id];

    // Ignore non-NDR and soft-NDR nets
    if (net->getDbNet()->getNonDefaultRule() == nullptr || net->isSoftNDR()) {
      continue;
    }

    // Access the routing tree edges for this net
    std::vector<TreeEdge>& treeedges = sttrees_[net_id].edges;
    const int num_edges = sttrees_[net_id].num_edges();

    uint16_t num_congested_edges = 0;

    // Iterate through all edges in the net's routing tree
    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      // Only process edges that have actual routing
      if (treeedge->len > 0 || treeedge->route.routelen > 0) {
        int routeLen = treeedge->route.routelen;
        std::vector<GPoint3D>& grids = treeedge->route.grids;

        // Check route
        for (int i = 0; i < routeLen; i++) {
          if (grids[i].x == grids[i + 1].x) {  // vertical
            const int min_y = std::min(grids[i].y, grids[i + 1].y);
            // Increment congested edges if have overflow
            if (graph2d_.getOverflowV(grids[i].x, min_y) > 0) {
              num_congested_edges++;
            }
          } else {  // horizontal
            const int min_x = std::min(grids[i].x, grids[i + 1].x);
            if (graph2d_.getOverflowH(min_x, grids[i].y) > 0) {
              num_congested_edges++;
            }
          }
        }
      }
    }
    if (num_congested_edges > 0) {
      // Include the NDR net in the list
      graph2d_.addCongestedNDRnet(net_id, num_congested_edges);
      if (logger_->debugCheck(GRT, "softNDR", 1)) {
        logger_->report("Congested NDR net: {} Edges: {}",
                        net->getName(),
                        num_congested_edges);
      }
    }
  }

  // Sort the congested NDR nets according to the priorities
  graph2d_.sortCongestedNDRnets();
}

void FastRouteCore::updateSoftNDRNetUsage(const int net_id, const int edge_cost)
{
  FrNet* net = nets_[net_id];
  // Access the routing tree edges for this net
  std::vector<TreeEdge>& treeedges = sttrees_[net_id].edges;
  const int num_edges = sttrees_[net_id].num_edges();

  // Iterate through all edges in the net's routing tree
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    TreeEdge* treeedge = &(treeedges[edgeID]);
    // Only process edges that have actual routing
    if (treeedge->len > 0 || treeedge->route.routelen > 0) {
      int routeLen = treeedge->route.routelen;
      std::vector<GPoint3D>& grids = treeedge->route.grids;

      // Update route usage
      for (int i = 0; i < routeLen; i++) {
        if (grids[i].x == grids[i + 1].x) {  // vertical
          const int min_y = std::min(grids[i].y, grids[i + 1].y);
          graph2d_.updateUsageV(grids[i].x, min_y, net, edge_cost);
        } else {  // horizontal
          const int min_x = std::min(grids[i].x, grids[i + 1].x);
          graph2d_.updateUsageH(min_x, grids[i].y, net, edge_cost);
        }
      }
    }
  }
}

void FastRouteCore::setVerbose(bool v)
{
  verbose_ = v;
}

void FastRouteCore::setCriticalNetsPercentage(float u)
{
  critical_nets_percentage_ = u;
}

void FastRouteCore::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
}

void FastRouteCore::setCongestionReportIterStep(int congestion_report_iter_step)
{
  congestion_report_iter_step_ = congestion_report_iter_step;
}

void FastRouteCore::setResistanceAware(bool resistance_aware)
{
  enable_resistance_aware_ = resistance_aware;
  en_estimate_parasitics_ = true;
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

void FastRouteCore::setDetourPenalty(int penalty)
{
  detour_penalty_ = penalty;
}

std::vector<int> FastRouteCore::getOriginalResources()
{
  std::vector<int> original_resources(num_layers_);
  for (int l = 0; l < num_layers_; l++) {
    bool is_horizontal
        = layer_directions_[l] == odb::dbTechLayerDir::HORIZONTAL;
    if (is_horizontal) {
      for (int i = 0; i < y_grid_; i++) {
        for (int j = 0; j < x_grid_ - 1; j++) {
          original_resources[l] += h_edges_3D_[l][i][j].real_cap;
        }
      }
    } else {
      for (int i = 0; i < y_grid_ - 1; i++) {
        for (int j = 0; j < x_grid_; j++) {
          original_resources[l] += v_edges_3D_[l][i][j].real_cap;
        }
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

const char* FrNet::getName() const
{
  return getNetName(getDbNet());
}

////////////////////////////////////////////////////////////////
void FastRouteCore::setDebugOn(
    std::unique_ptr<AbstractFastRouteRenderer> renderer)
{
  debug_->renderer = std::move(renderer);
}
void FastRouteCore::setDebugSteinerTree(bool steinerTree)
{
  debug_->steinerTree = steinerTree;
}
void FastRouteCore::setDebugTree2D(bool tree2D)
{
  debug_->tree2D = tree2D;
}
void FastRouteCore::setDebugTree3D(bool tree3D)
{
  debug_->tree3D = tree3D;
}
void FastRouteCore::setDebugNet(const odb::dbNet* net)
{
  debug_->net = net;
}
void FastRouteCore::setDebugRectilinearSTree(bool rectiliniarSTree)
{
  debug_->rectilinearSTree = rectiliniarSTree;
}
void FastRouteCore::setSttInputFilename(const char* file_name)
{
  debug_->sttInputFileName = std::string(file_name);
}
bool FastRouteCore::hasSaveSttInput()
{
  return !debug_->sttInputFileName.empty();
}
std::string FastRouteCore::getSttInputFileName()
{
  return debug_->sttInputFileName;
}
const odb::dbNet* FastRouteCore::getDebugNet()
{
  return debug_->net;
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

int8_t FrNet::getLayerEdgeCost(int layer) const
{
  if (edge_cost_per_layer_ && !is_soft_ndr_) {
    return (*edge_cost_per_layer_)[layer];
  }

  return 1;
}

int FrNet::getPinIdxFromPosition(int x, int y, int count)
{
  int cnt = 1;
  for (int idx = 0; idx < pin_x_.size(); idx++) {
    const int pin_x = pin_x_[idx];
    const int pin_y = pin_y_[idx];

    if (x == pin_x && y == pin_y) {
      if (cnt == count) {
        return idx;
      }
      cnt++;
    }
  }

  return -1;
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
                  int8_t edge_cost,
                  int min_layer,
                  int max_layer,
                  float slack,
                  std::vector<int8_t>* edge_cost_per_layer)
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
