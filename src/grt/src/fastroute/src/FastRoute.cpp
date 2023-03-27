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
#include <unordered_set>

#include "DataType.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

FastRouteCore::FastRouteCore(odb::dbDatabase* db,
                             utl::Logger* log,
                             stt::SteinerTreeBuilder* stt_builder,
                             gui::Gui* gui)
    : max_degree_(0),
      db_(db),
      gui_(gui),
      overflow_iterations_(0),
      layer_orientation_(0),
      x_range_(0),
      y_range_(0),
      num_adjust_(0),
      v_capacity_(0),
      h_capacity_(0),
      x_grid_(0),
      y_grid_(0),
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
      via_cost_(0),
      mazeedge_threshold_(0),
      v_capacity_lb_(0),
      h_capacity_lb_(0),
      logger_(log),
      stt_builder_(stt_builder),
      fastrouteRender_(nullptr),
      debug_(new DebugSetting())
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

  layer_grid_.resize(boost::extents[0][0]);
  via_link_.resize(boost::extents[0][0]);

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
  seglist_.clear();
  db_net_id_map_.clear();
}

void FastRouteCore::setGridsAndLayers(int x, int y, int nLayers)
{
  x_grid_ = x;
  y_grid_ = y;
  num_layers_ = nLayers;
  if (std::max(x_grid_, y_grid_) >= 1000) {
    x_range_ = std::max(x_grid_, y_grid_);
    y_range_ = std::max(x_grid_, y_grid_);
  } else {
    x_range_ = 1000;
    y_range_ = 1000;
  }

  v_capacity_3D_.resize(num_layers_);
  h_capacity_3D_.resize(num_layers_);

  for (int i = 0; i < num_layers_; i++) {
    v_capacity_3D_[i] = 0;
    h_capacity_3D_[i] = 0;
  }

  layer_grid_.resize(boost::extents[num_layers_][MAXLEN]);
  via_link_.resize(boost::extents[num_layers_][MAXLEN]);

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

void FastRouteCore::setLayerOrientation(int x)
{
  layer_orientation_ = x;
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
  }
  net->reset(db_net,
             is_clock,
             driver_idx,
             cost,
             min_layer,
             max_layer,
             slack,
             edge_cost_per_layer);

  return net;
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
  sttrees_[netID].nodes.reset();
  sttrees_[netID].edges.reset();
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
    for (int j = 0; j < x_grid_ - 1; j++) {
      // 2D edge initialization
      h_edges_[i][j].cap = h_capacity_;
      h_edges_[i][j].usage = 0;
      h_edges_[i][j].est_usage = 0;
      h_edges_[i][j].red = 0;
      h_edges_[i][j].last_usage = 0;

      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        h_edges_3D_[k][i][j].cap = h_capacity_3D_[k];
        h_edges_3D_[k][i][j].usage = 0;
      }
    }
  }
  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // 2D edge initialization
      v_edges_[i][j].cap = v_capacity_;
      v_edges_[i][j].usage = 0;
      v_edges_[i][j].est_usage = 0;
      v_edges_[i][j].red = 0;
      v_edges_[i][j].last_usage = 0;

      // 3D edge initialization
      for (int k = 0; k < num_layers_; k++) {
        v_edges_3D_[k][i][j].cap = v_capacity_3D_[k];
        v_edges_3D_[k][i][j].usage = 0;
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
                                  int reducedCap,
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
      h_edges_[y1][x1].cap += increase;
    }

    h_edges_[y1][x1].cap -= reduce;
    h_edges_[y1][x1].red += reduce;

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
      v_edges_[y1][x1].cap += increase;
    }

    v_edges_[y1][x1].cap -= reduce;
    v_edges_[y1][x1].red += reduce;
  }
}

void FastRouteCore::applyVerticalAdjustments(const odb::Point& first_tile,
                                             const odb::Point& last_tile,
                                             int layer,
                                             int first_tile_reduce,
                                             int last_tile_reduce)
{
  for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y < last_tile.getY(); y++) {
      if (x == first_tile.getX()) {
        int edge_cap = getEdgeCapacity(x, y, x, y + 1, layer);
        edge_cap -= first_tile_reduce;
        if (edge_cap < 0)
          edge_cap = 0;
        addAdjustment(x, y, x, y + 1, layer, edge_cap, true);
      } else if (x == last_tile.getX()) {
        int edge_cap = getEdgeCapacity(x, y, x, y + 1, layer);
        edge_cap -= last_tile_reduce;
        if (edge_cap < 0)
          edge_cap = 0;
        addAdjustment(x, y, x, y + 1, layer, edge_cap, true);
      } else {
        addAdjustment(x, y, x, y + 1, layer, 0, true);
      }
    }
  }
}

void FastRouteCore::applyHorizontalAdjustments(const odb::Point& first_tile,
                                               const odb::Point& last_tile,
                                               int layer,
                                               int first_tile_reduce,
                                               int last_tile_reduce)
{
  for (int x = first_tile.getX(); x < last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
      if (y == first_tile.getY()) {
        int edge_cap = getEdgeCapacity(x, y, x + 1, y, layer);
        edge_cap -= first_tile_reduce;
        if (edge_cap < 0)
          edge_cap = 0;
        addAdjustment(x, y, x + 1, y, layer, edge_cap, true);
      } else if (y == last_tile.getY()) {
        int edge_cap = getEdgeCapacity(x, y, x + 1, y, layer);
        edge_cap -= last_tile_reduce;
        if (edge_cap < 0)
          edge_cap = 0;
        addAdjustment(x, y, x + 1, y, layer, edge_cap, true);
      } else {
        addAdjustment(x, y, x + 1, y, layer, 0, true);
      }
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
      for (auto interval_it : intervals) {
        reduce += ceil(static_cast<float>(
                           std::abs(interval_it.upper() - interval_it.lower()))
                       / track_space[layer - 1]);
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
      for (const auto& interval_it : intervals) {
        reduce += ceil(static_cast<float>(
                           std::abs(interval_it.upper() - interval_it.lower()))
                       / track_space[layer - 1]);
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

  initNetAuxVars();

  grid_hv_ = x_range_ * y_range_;

  parent_x1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_x3_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y3_.resize(boost::extents[y_grid_][x_grid_]);
}

void FastRouteCore::initNetAuxVars()
{
  int node_count = netCount();
  seglist_.resize(node_count);
  sttrees_.resize(node_count);
  gxs_.resize(node_count);
  gys_.resize(node_count);
  gs_.resize(node_count);
}

NetRouteMap FastRouteCore::getRoutes()
{
  NetRouteMap routes;
  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    nets_[netID]->setIsRouted(true);
    odb::dbNet* db_net = nets_[netID]->getDbNet();
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
            net_segs.insert(segment);
            route.push_back(segment);
          }
        }
      }
    }
  }

  return routes;
}

void FastRouteCore::updateDbCongestion()
{
  auto block = db_->getChip()->getBlock();
  auto db_gcell = block->getGCellGrid();
  if (db_gcell)
    db_gcell->resetGrid();
  else
    db_gcell = odb::dbGCellGrid::create(block);

  db_gcell->addGridPatternX(x_corner_, x_grid_, tile_size_);
  db_gcell->addGridPatternY(y_corner_, y_grid_, tile_size_);
  auto db_tech = db_->getTech();
  for (int k = 0; k < num_layers_; k++) {
    auto layer = db_tech->findRoutingLayer(k + 1);
    if (layer == nullptr) {
      continue;
    }

    const unsigned short capH = h_capacity_3D_[k];
    const unsigned short capV = v_capacity_3D_[k];
    for (int y = 0; y < y_grid_; y++) {
      for (int x = 0; x < x_grid_ - 1; x++) {
        const unsigned short blockageH = capH - h_edges_3D_[k][y][x].cap;
        const unsigned short blockageV = capV - v_edges_3D_[k][y][x].cap;
        const unsigned short usageH = h_edges_3D_[k][y][x].usage + blockageH;
        const unsigned short usageV = v_edges_3D_[k][y][x].usage + blockageV;
        db_gcell->setCapacity(layer, x, y, capH, capV, 0);
        db_gcell->setUsage(layer, x, y, usageH, usageV, 0);
        db_gcell->setBlockage(layer, x, y, blockageH, blockageV, 0);
      }
    }
  }
}

NetRouteMap FastRouteCore::run()
{
  int tUsage;
  int cost_step;
  int maxOverflow = 0;
  int minoflrnd = 0;
  int bwcnt = 0;

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
    routeLVAll(newTH, enlarge_, LOGIS_COF);

    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    enlarge_ += 5;
    newTH -= 5;
    if (newTH < 1) {
      newTH = 1;
    }
  }

  // check and fix invalid embedded trees
  fixEmbeddedTrees();

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
  if (debug_->isOn_ && debug_->rectilinearSTree_) {
    for (int netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->getDbNet() == debug_->net_
          && !nets_[netID]->isRouted()) {
        StTreeVisualization(sttrees_[netID], nets_[netID], false);
      }
    }
  }

  const int max_overflow_increases = 25;

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
      updateCongestionHistory(upType, stopDEC, max_adj);
    } else if (total_overflow_ < 500) {
      cost_step = CSTEP3;
      enlarge_ += ESTEP3;
      ripup_threshold = -1;
      updateCongestionHistory(upType, stopDEC, max_adj);
    } else {
      cost_step = CSTEP2;
      enlarge_ += ESTEP2;
      updateCongestionHistory(upType, stopDEC, max_adj);
    }

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
                  L);
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
                      L);
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
                      L);
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
  }  // end overflow iterations

  // Debug mode Tree 2D after overflow iterations
  if (debug_->isOn_ && debug_->tree2D_) {
    for (int netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->getDbNet() == debug_->net_
          && !nets_[netID]->isRouted()) {
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

  // Debug mode Tree 3D after layer assignament
  if (debug_->isOn_ && debug_->tree3D_) {
    for (int netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->getDbNet() == debug_->net_
          && !nets_[netID]->isRouted()) {
        StTreeVisualization(sttrees_[netID], nets_[netID], true);
      }
    }
  }

  if (goingLV && past_cong == 0) {
    mazeRouteMSMDOrder3D(enlarge_, 0, 20, layer_orientation_);
    mazeRouteMSMDOrder3D(enlarge_, 0, 12, layer_orientation_);
  }

  fillVIA();
  const int finallength = getOverflow3D();
  const int numVia = threeDVIA();
  checkRoute3D();

  if (verbose_) {
    logger_->info(GRT, 111, "Final number of vias: {}", numVia);
    logger_->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));
  }

  NetRouteMap routes = getRoutes();
  net_eo_.clear();
  return routes;
}

void FastRouteCore::setVerbose(bool v)
{
  verbose_ = v;
}

void FastRouteCore::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
}

std::vector<int> FastRouteCore::getOriginalResources()
{
  std::vector<int> original_resources(num_layers_);
  for (int l = 0; l < num_layers_; l++) {
    original_resources[l]
        += (v_capacity_3D_[l] + h_capacity_3D_[l]) * y_grid_ * x_grid_;
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

////////////////////////////////////////////////////////////////////////////////////////////

enum class TreeStructure
{
  steinerTreeByStt,
  steinerTreeByFastroute
};

class FastRouteRenderer : public gui::Renderer
{
 public:
  FastRouteRenderer(odb::dbTech* tech,
                    int tile_size,
                    int x_corner,
                    int y_corner);
  void highlight(const FrNet* net);
  void setSteinerTree(const stt::Tree& stree);
  void setStTreeValues(const StTree& stree);
  void setIs3DVisualization(bool is3DVisualization);
  void setTreeStructure(TreeStructure treeStructure);

  virtual void drawObjects(gui::Painter& /* painter */) override;

 private:
  void drawTreeEdges(gui::Painter& painter);
  void drawCircleObjects(gui::Painter& painter);
  void drawLineObject(int x1,
                      int y1,
                      int l1,
                      int x2,
                      int y2,
                      int l2,
                      gui::Painter& painter);

  TreeStructure treeStructure_;

  // Steiner Tree by stt
  stt::Tree stree_;

  // Steiner tree by fastroute
  std::vector<TreeEdge> treeEdges_;
  bool is3DVisualization_;

  // net data of pins
  std::vector<int> pinX_;  // array of X coordinates of pins
  std::vector<int> pinY_;  // array of Y coordinates of pins
  std::vector<int> pinL_;  // array of L coordinates of pins

  odb::dbTech* tech_;
  int tile_size_, x_corner_, y_corner_;
};

FastRouteRenderer::FastRouteRenderer(odb::dbTech* tech,
                                     int tile_size,
                                     int x_corner,
                                     int y_corner)
    : treeStructure_(TreeStructure::steinerTreeByStt),
      is3DVisualization_(false),
      tech_(tech),
      tile_size_(tile_size),
      x_corner_(x_corner),
      y_corner_(y_corner)
{
}
void FastRouteRenderer::setTreeStructure(TreeStructure treeStructure)
{
  treeStructure_ = treeStructure;
}
void FastRouteRenderer::highlight(const FrNet* net)
{
  pinX_ = net->getPinX();
  pinY_ = net->getPinY();
  pinL_ = net->getPinL();
}
void FastRouteRenderer::setSteinerTree(const stt::Tree& stree)
{
  stree_ = stree;
}

void FastRouteRenderer::setStTreeValues(const StTree& stree)
{
  treeEdges_.clear();
  const int num_edges = stree.num_edges();
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    treeEdges_.push_back(stree.edges[edgeID]);
  }
}
void FastRouteRenderer::setIs3DVisualization(bool is3DVisualization)
{
  is3DVisualization_ = is3DVisualization;
}

void FastRouteRenderer::drawLineObject(int x1,
                                       int y1,
                                       int layer1,
                                       int x2,
                                       int y2,
                                       int layer2,
                                       gui::Painter& painter)
{
  if (layer1 == layer2) {
    if (is3DVisualization_) {
      odb::dbTechLayer* layer = tech_->findRoutingLayer(layer1);
      painter.setPen(layer);
      painter.setBrush(layer);
    } else {
      painter.setPen(painter.cyan);
      painter.setBrush(painter.cyan);
    }
    painter.setPenWidth(700);
    painter.drawLine(x1, y1, x2, y2);
  }
}
void FastRouteRenderer::drawTreeEdges(gui::Painter& painter)
{
  int lastL = 0;
  for (TreeEdge treeEdge : treeEdges_) {
    if (treeEdge.len == 0) {
      continue;
    }

    int routeLen = treeEdge.route.routelen;
    const std::vector<short>& gridsX = treeEdge.route.gridsX;
    const std::vector<short>& gridsY = treeEdge.route.gridsY;
    const std::vector<short>& gridsL = treeEdge.route.gridsL;
    int lastX = tile_size_ * (gridsX[0] + 0.5) + x_corner_;
    int lastY = tile_size_ * (gridsY[0] + 0.5) + y_corner_;

    if (is3DVisualization_)
      lastL = gridsL[0];

    for (int i = 1; i <= routeLen; i++) {
      const int xreal = tile_size_ * (gridsX[i] + 0.5) + x_corner_;
      const int yreal = tile_size_ * (gridsY[i] + 0.5) + y_corner_;

      if (is3DVisualization_) {
        drawLineObject(
            lastX, lastY, lastL + 1, xreal, yreal, gridsL[i] + 1, painter);
        lastL = gridsL[i];
      } else {
        drawLineObject(
            lastX, lastY, -1, xreal, yreal, -1, painter);  // -1 to 2D Trees
      }
      lastX = xreal;
      lastY = yreal;
    }
  }
}
void FastRouteRenderer::drawCircleObjects(gui::Painter& painter)
{
  painter.setPenWidth(700);
  for (auto i = 0; i < pinX_.size(); i++) {
    const int xreal = tile_size_ * (pinX_[i] + 0.5) + x_corner_;
    const int yreal = tile_size_ * (pinY_[i] + 0.5) + y_corner_;

    odb::dbTechLayer* layer = tech_->findRoutingLayer(pinL_[i] + 1);
    painter.setPen(layer);
    painter.setBrush(layer);
    painter.drawCircle(xreal, yreal, 1500);
  }
}

void FastRouteRenderer::drawObjects(gui::Painter& painter)
{
  if (treeStructure_ == TreeStructure::steinerTreeByStt) {
    painter.setPen(painter.white);
    painter.setBrush(painter.white);
    painter.setPenWidth(700);

    const int deg = stree_.deg;
    for (int i = 0; i < 2 * deg - 2; i++) {
      const int x1 = tile_size_ * (stree_.branch[i].x + 0.5) + x_corner_;
      const int y1 = tile_size_ * (stree_.branch[i].y + 0.5) + y_corner_;
      const int n = stree_.branch[i].n;
      const int x2 = tile_size_ * (stree_.branch[n].x + 0.5) + x_corner_;
      const int y2 = tile_size_ * (stree_.branch[n].y + 0.5) + y_corner_;
      const int len = abs(x1 - x2) + abs(y1 - y2);
      if (len > 0) {
        painter.drawLine(x1, y1, x2, y2);
      }
    }

    drawCircleObjects(painter);
  } else if (treeStructure_ == TreeStructure::steinerTreeByFastroute) {
    drawTreeEdges(painter);

    drawCircleObjects(painter);
  }
}

////////////////////////////////////////////////////////////////
void FastRouteCore::setDebugOn(bool isOn)
{
  debug_->isOn_ = isOn;
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
  // init FastRouteRender
  if (gui::Gui::enabled()) {
    if (fastrouteRender_ == nullptr) {
      fastrouteRender_ = new FastRouteRenderer(
          db_->getTech(), tile_size_, x_corner_, y_corner_);
      gui_->registerRenderer(fastrouteRender_);
    }
    fastrouteRender_->highlight(net);
    fastrouteRender_->setIs3DVisualization(
        false);  // isnt 3D because is steiner tree generated by stt
    fastrouteRender_->setSteinerTree(stree);
    fastrouteRender_->setTreeStructure(TreeStructure::steinerTreeByStt);
    gui_->redraw();
    gui_->pause();
  }
}

void FastRouteCore::StTreeVisualization(const StTree& stree,
                                        FrNet* net,
                                        bool is3DVisualization)
{
  // init FastRouteRender
  if (gui_) {
    if (fastrouteRender_ == nullptr) {
      fastrouteRender_ = new FastRouteRenderer(
          db_->getTech(), tile_size_, x_corner_, y_corner_);
      gui_->registerRenderer(fastrouteRender_);
    }
    fastrouteRender_->highlight(net);
    fastrouteRender_->setIs3DVisualization(is3DVisualization);
    fastrouteRender_->setStTreeValues(stree);
    fastrouteRender_->setTreeStructure(TreeStructure::steinerTreeByFastroute);
    gui_->redraw();
    gui_->pause();
  }
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
  is_routed_ = false;
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
