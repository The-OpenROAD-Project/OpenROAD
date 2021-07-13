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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "DataType.h"
#include "flute.h"
#include "utl/Logger.h"
#include "opendb/db.h"

namespace grt {

using utl::GRT;

FastRouteCore::FastRouteCore(odb::dbDatabase* db, utl::Logger* log)
{
  new_net_id_ = 0;
  seg_count_ = 0;
  pin_ind_ = 0;
  num_adjust_ = 0;
  v_capacity_ = 0;
  h_capacity_ = 0;
  num_nets_ = 0;
  max_degree_ = 0;
  logger_ = log;
  db_ = db;
  allow_overflow_ = false;
  overflow_iterations_ = 0;
  layer_orientation_ = 0;
  sttrees_ = nullptr;
  heap1_ = nullptr;
  heap2_ = nullptr;
}

FastRouteCore::~FastRouteCore()
{
  deleteComponents();
}

void FastRouteCore::clear()
{
  deleteComponents();
}

void FastRouteCore::deleteComponents()
{
  if (!nets_.empty()) {
    for (FrNet* net : nets_) {
      if (net != nullptr)
        delete net;
      net = nullptr;
    }
  
    nets_.clear();
  }

  h_edges_.clear();
  v_edges_.clear();
  seglist_.clear();
  seglist_index_.clear();
  seglist_cnt_.clear();

  gxs_.clear();
  gys_.clear();
  gs_.clear();

  tree_order_pv_.clear();
  tree_order_cong_.clear();

  h_edges_3D_.clear();
  v_edges_3D_.clear();

  if (sttrees_ != nullptr) {
    for (int i = 0; i < num_valid_nets_; i++) {
      int deg = sttrees_[i].deg;
      int numEdges = 2 * deg - 3;
      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        TreeEdge* treeedge = &(sttrees_[i].edges[edgeID]);
        if (treeedge->len > 0) {
          if (treeedge->route.gridsX != nullptr)
            free(treeedge->route.gridsX);
          if (treeedge->route.gridsY != nullptr)
            free(treeedge->route.gridsY);
          if (treeedge->route.gridsL != nullptr)
            free(treeedge->route.gridsL);
          treeedge->route.gridsX = nullptr;
          treeedge->route.gridsY = nullptr;
          treeedge->route.gridsL = nullptr;
        }
      }

      if (sttrees_[i].nodes != nullptr)
        delete[] sttrees_[i].nodes;
      sttrees_[i].nodes = nullptr;

      if (sttrees_[i].edges != nullptr)
        delete[] sttrees_[i].edges;
      sttrees_[i].edges = nullptr;
    }
    delete[] sttrees_;
    sttrees_ = nullptr;
  }
  
  parent_x1_.resize(boost::extents[0][0]);
  parent_y1_.resize(boost::extents[0][0]);
  parent_x3_.resize(boost::extents[0][0]);
  parent_y3_.resize(boost::extents[0][0]);

  if (heap1_ != nullptr)
    delete[] heap1_;
  if (heap2_ != nullptr)
    delete[] heap2_;
  heap1_ = nullptr;
  heap2_ = nullptr;

  pop_heap2_.clear();

  net_eo_.clear();

  xcor_.clear();
  ycor_.clear();
  dcor_.clear();

  hv_.resize(boost::extents[0][0]);
  hyper_v_.resize(boost::extents[0][0]);
  hyper_h_.resize(boost::extents[0][0]);
  corr_edge_.resize(boost::extents[0][0]);
  
  in_region_.resize(boost::extents[0][0]);

  d1_3D_.resize(boost::extents[0][0][0]);
  d2_3D_.resize(boost::extents[0][0][0]);
  d1_.resize(boost::extents[0][0]);
  d2_.resize(boost::extents[0][0]);

  v_capacity_3D_.clear();
  h_capacity_3D_.clear();

  grid_hs_.clear();
  grid_vs_.clear();

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

  new_net_id_ = 0;
  seg_count_ = 0;
  pin_ind_ = 0;
  num_adjust_ = 0;
  v_capacity_ = 0;
  h_capacity_ = 0;
  num_nets_ = 0;
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

  grid_hs_.resize(num_layers_);
  grid_vs_.resize(num_layers_);

  layer_grid_.resize(boost::extents[num_layers_][MAXLEN]);
  via_link_.resize(boost::extents[num_layers_][MAXLEN]);

  d1_3D_.resize(boost::extents[num_layers_][y_range_][x_range_]);
  d2_3D_.resize(boost::extents[num_layers_][y_range_][x_range_]);

  d1_.resize(boost::extents[y_range_][x_range_]);
  d2_.resize(boost::extents[y_range_][x_range_]);

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

void FastRouteCore::setNumberNets(int nNets)
{
  num_nets_ = nNets;
  nets_.resize(num_nets_);
  for (int i = 0; i < num_nets_; i++)
    nets_[i] = new FrNet;
  seglist_index_.resize(num_nets_ + 1);
}

void FastRouteCore::setLowerLeft(int x, int y)
{
  x_corner_ = x;
  y_corner_ = y;
}

void FastRouteCore::setTileSize(int width, int height)
{
  w_tile_ = width;
  h_tile_ = height;
}

void FastRouteCore::setLayerOrientation(int x)
{
  layer_orientation_ = x;
}

void FastRouteCore::addPin(int netID, int x, int y, int layer)
{
  FrNet* net = nets_[netID];
  net->pinX.push_back(x);
  net->pinY.push_back(y);
  net->pinL.push_back(layer);
}

int FastRouteCore::addNet(odb::dbNet* db_net,
                          int num_pins,
                          float alpha,
                          bool is_clock,
                          int driver_idx,
                          int cost,
                          int min_layer,
                          int max_layer,
                          std::vector<int> edge_cost_per_layer)
{
  int netID = new_net_id_;
  FrNet* net = nets_[new_net_id_];
  pin_ind_ = num_pins;
  net->db_net = db_net;
  net->numPins = num_pins;
  net->deg = pin_ind_;
  net->alpha = alpha;
  net->is_clock = is_clock;
  net->driver_idx = driver_idx;
  net->edgeCost = cost;
  net->minLayer = min_layer;
  net->maxLayer = max_layer;
  net->edge_cost_per_layer = edge_cost_per_layer;

  seglist_index_[new_net_id_] = seg_count_;
  new_net_id_++;
  // at most (2*num_pins-2) nodes -> (2*num_pins-3) segs_ for a net
  seg_count_ += 2 * pin_ind_ - 3;
  return netID;
}

void FastRouteCore::init_usage()
{
  for (int i = 0; i < y_grid_ * (x_grid_ - 1); i++)
    h_edges_[i].usage = 0;
  for (int i = 0; i < (y_grid_ - 1) * x_grid_; i++)
    v_edges_[i].usage = 0;
}

void FastRouteCore::initEdges()
{
  const float LB = 0.9;
  const float UB = 1.3;
  v_capacity_lb_ = LB * v_capacity_;
  h_capacity_lb_ = LB * h_capacity_;

  // TODO: check this, there was an if pin_ind_ > 1 && pin_ind_ < 2000
  if (pin_ind_ > 1) {
    seglist_index_[new_net_id_] = seg_count_;  // the end pointer of the seglist
  }
  num_valid_nets_ = new_net_id_;

  // allocate memory and initialize for edges

  h_edges_.resize((x_grid_ - 1) * y_grid_);
  v_edges_.resize(x_grid_ * (y_grid_ - 1));

  init_usage();

  v_edges_3D_.resize(num_layers_ * x_grid_ * y_grid_);
  h_edges_3D_.resize(num_layers_ * x_grid_ * y_grid_);

  // 2D edge initialization
  int TC = 0;
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      int grid = i * (x_grid_ - 1) + j;
      h_edges_[grid].cap = h_capacity_;
      TC += h_capacity_;
      h_edges_[grid].usage = 0;
      h_edges_[grid].est_usage = 0;
      h_edges_[grid].red = 0;
      h_edges_[grid].last_usage = 0;
    }
  }
  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      int grid = i * x_grid_ + j;
      v_edges_[grid].cap = v_capacity_;
      TC += v_capacity_;
      v_edges_[grid].usage = 0;
      v_edges_[grid].est_usage = 0;
      v_edges_[grid].red = 0;
      v_edges_[grid].last_usage = 0;
    }
  }

  // 3D edge initialization
  for (int k = 0; k < num_layers_; k++) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_; j++) {
        int grid = i * (x_grid_ - 1) + j + k * (x_grid_ - 1) * y_grid_;
        h_edges_3D_[grid].cap = h_capacity_3D_[k];
        h_edges_3D_[grid].usage = 0;
        h_edges_3D_[grid].red = 0;
      }
    }
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_; j++) {
        int grid = i * x_grid_ + j + k * x_grid_ * (y_grid_ - 1);
        v_edges_3D_[grid].cap = v_capacity_3D_[k];
        v_edges_3D_[grid].usage = 0;
        v_edges_3D_[grid].red = 0;
      }
    }
  }
}

void FastRouteCore::setNumAdjustments(int nAdjustments)
{
  num_adjust_ = nAdjustments;
}

int FastRouteCore::getEdgeCurrentResource(long x1,
                                          long y1,
                                          int l1,
                                          long x2,
                                          long y2,
                                          int l2)
{
  int grid, k;
  int resource = 0;

  k = l1 - 1;
  if (y1 == y2) {
    grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    resource = h_edges_3D_[grid].cap - h_edges_3D_[grid].usage;
  } else if (x1 == x2) {
    grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    resource = v_edges_3D_[grid].cap - v_edges_3D_[grid].usage;
  } else
  {
    logger_->error(GRT, 212, "Cannot get edge resource: edge is not vertical or horizontal.");
  }

  return resource;
}

int FastRouteCore::getEdgeCurrentUsage(long x1,
                                       long y1,
                                       int l1,
                                       long x2,
                                       long y2,
                                       int l2)
{
  int grid, k;
  int usage = 0;

  k = l1 - 1;
  if (y1 == y2) {
    grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    usage = h_edges_3D_[grid].usage;
  } else if (x1 == x2) {
    grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    usage = v_edges_3D_[grid].usage;
  } else
  {
    logger_->error(GRT, 213, "Cannot get edge usage: edge is not vertical or horizontal.");
  }

  return usage;
}

void FastRouteCore::setMaxNetDegree(int deg)
{
  max_degree_ = deg;
}

void FastRouteCore::addAdjustment(long x1,
                                  long y1,
                                  int l1,
                                  long x2,
                                  long y2,
                                  int l2,
                                  int reducedCap,
                                  bool isReduce)
{
  const int k = l1 - 1;

  if (y1 == y2)  // horizontal edge
  {
    int grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    int cap = h_edges_3D_[grid].cap;
    int reduce;

    if (((int) cap - reducedCap) < 0) {
      if (isReduce) {
        logger_->warn(GRT, 113, "Underflow in reduce: cap, reducedCap: {}, {}", cap, reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    h_edges_3D_[grid].cap = reducedCap;
    h_edges_3D_[grid].red = reduce;

    grid = y1 * (x_grid_ - 1) + x1;
    if (!isReduce) {
      int increase = reducedCap - cap;
      h_edges_[grid].cap += increase;
    }

    h_edges_[grid].cap -= reduce;
    h_edges_[grid].red += reduce;

  } else if (x1 == x2)  // vertical edge
  {
    int grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    int cap = v_edges_3D_[grid].cap;
    int reduce;

    if (((int) cap - reducedCap) < 0) {
      if (isReduce) {
        logger_->warn(GRT, 114, "Underflow in reduce: cap, reducedCap: {}, {}", cap, reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    v_edges_3D_[grid].cap = reducedCap;
    v_edges_3D_[grid].red = reduce;

    grid = y1 * x_grid_ + x1;
    if (!isReduce) {
      int increase = reducedCap - cap;
      v_edges_[grid].cap += increase;
    }

    v_edges_[grid].cap -= reduce;
    v_edges_[grid].red += reduce;
  }
}

int FastRouteCore::getEdgeCapacity(long x1,
                                   long y1,
                                   int l1,
                                   long x2,
                                   long y2,
                                   int l2)
{
  int cap = 0;

  const int k = l1 - 1;

  if (y1 == y2)  // horizontal edge
  {
    int grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    cap = h_edges_3D_[grid].cap;
  } else if (x1 == x2)  // vertical edge
  {
    int grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    cap = v_edges_3D_[grid].cap;
  } else
  {
    logger_->error(GRT, 214, "Cannot get edge capacity: edge is not vertical or horizontal.");
  }

  return cap;
}

void FastRouteCore::setEdgeCapacity(long x1,
                                    long y1,
                                    int l1,
                                    long x2,
                                    long y2,
                                    int l2,
                                    int newCap)
{
  const int k = l1 - 1;
  int grid;
  int reduce;

  if (y1 == y2)  // horizontal edge
  {
    grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    int currCap = h_edges_3D_[grid].cap;
    h_edges_3D_[grid].cap = newCap;

    grid = y1 * (x_grid_ - 1) + x1;
    reduce = currCap - newCap;
    h_edges_[grid].cap -= reduce;
  } else if (x1 == x2)  // vertical edge
  {
    grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    int currCap = v_edges_3D_[grid].cap;
    v_edges_3D_[grid].cap = newCap;

    grid = y1 * x_grid_ + x1;
    reduce = currCap - newCap;
    v_edges_[grid].cap -= reduce;
  }
}

void FastRouteCore::setEdgeUsage(long x1,
                                 long y1,
                                 int l1,
                                 long x2,
                                 long y2,
                                 int l2,
                                 int newUsage)
{
  const int k = l1 - 1;
  int grid;

  if (y1 == y2)  // horizontal edge
  {
    grid = y1 * (x_grid_ - 1) + x1 + k * (x_grid_ - 1) * y_grid_;
    h_edges_3D_[grid].usage = newUsage;

    grid = y1 * (x_grid_ - 1) + x1;
    h_edges_[grid].usage += newUsage;
  } else if (x1 == x2)  // vertical edge
  {
    grid = y1 * x_grid_ + x1 + k * x_grid_ * (y_grid_ - 1);
    v_edges_3D_[grid].usage = newUsage;

    grid = y1 * x_grid_ + x1;
    v_edges_[grid].usage += newUsage;
  }
}

void FastRouteCore::initAuxVar()
{
  tree_order_cong_.clear();

  seglist_cnt_.resize(num_valid_nets_);
  seglist_.resize(seg_count_);
  sttrees_ = new StTree[num_valid_nets_];
  gxs_.resize(num_valid_nets_);
  gys_.resize(num_valid_nets_);
  gs_.resize(num_valid_nets_);

  grid_hv_ = x_range_ * y_range_;
  grid_h_ = (x_grid_ - 1) * y_grid_;
  grid_v_ = x_grid_ * (y_grid_ - 1);
  for (int k = 0; k < num_layers_; k++) {
    grid_hs_[k] = k * grid_h_;
    grid_vs_[k] = k * grid_v_;
  }

  parent_x1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y1_.resize(boost::extents[y_grid_][x_grid_]);
  parent_x3_.resize(boost::extents[y_grid_][x_grid_]);
  parent_y3_.resize(boost::extents[y_grid_][x_grid_]);

  pop_heap2_.resize(y_grid_ * x_range_);

  // allocate memory for priority queue
  heap1_ = new float*[y_grid_ * x_grid_];
  heap2_ = new float*[y_grid_ * x_grid_];

  sttrees_bk_ = NULL;
}

NetRouteMap FastRouteCore::getRoutes()
{
  NetRouteMap routes;
  for (int netID = 0; netID < num_valid_nets_; netID++) {
    odb::dbNet* db_net = nets_[netID]->db_net;
    GRoute& route = routes[db_net];

    TreeEdge* treeedges = sttrees_[netID].edges;
    int deg = sttrees_[netID].deg;

    for (int edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int routeLen = treeedge->route.routelen;
        short* gridsX = treeedge->route.gridsX;
        short* gridsY = treeedge->route.gridsY;
        short* gridsL = treeedge->route.gridsL;
        int lastX = w_tile_ * (gridsX[0] + 0.5) + x_corner_;
        int lastY = h_tile_ * (gridsY[0] + 0.5) + y_corner_;
        int lastL = gridsL[0];
        for (int i = 1; i <= routeLen; i++) {
          int xreal = w_tile_ * (gridsX[i] + 0.5) + x_corner_;
          int yreal = h_tile_ * (gridsY[i] + 0.5) + y_corner_;

          GSegment segment
              = GSegment(lastX, lastY, lastL + 1, xreal, yreal, gridsL[i] + 1);
          lastX = xreal;
          lastY = yreal;
          lastL = gridsL[i];
          route.push_back(segment);
        }
      }
    }
  }

  return routes;
}

void FastRouteCore::updateDbCongestion()
{
  auto block = db_->getChip()->getBlock();
  auto db_gcell = odb::dbGCellGrid::create(block);
  if(db_gcell == nullptr)
  {
    db_gcell = block->getGCellGrid();
    logger_->warn(utl::GRT, 211, "dbGcellGrid already exists in db. Clearing existing dbGCellGrid");
    db_gcell->resetGrid();
  }
  db_gcell->addGridPatternX(x_corner_, x_grid_, w_tile_);
  db_gcell->addGridPatternY(y_corner_, y_grid_ + 1, h_tile_);
  for (int k = 0; k < num_layers_; k++) {
    auto layer = db_->getTech()->findRoutingLayer(k+1);
    if(layer == nullptr)
    {
      logger_->warn(utl::GRT, 215, "skipping layer {} not found in db", k+1);
      continue;
    }

    for (int y = 0; y < y_grid_; y++) {
      for (int x = 0; x < x_grid_ - 1; x++) {
        int grid_h_ = y * (x_grid_ - 1) + x + k * (x_grid_ - 1) * y_grid_;

        unsigned short capH = h_capacity_3D_[k];
        unsigned short blockageH = (h_capacity_3D_[k] - h_edges_3D_[grid_h_].cap);
        unsigned short usageH = h_edges_3D_[grid_h_].usage + blockageH;

        db_gcell->setHorizontalCapacity(layer, x, y, (uint) capH);
        db_gcell->setHorizontalUsage(layer, x, y, (uint) usageH);
        db_gcell->setHorizontalBlockage(layer, x, y, (uint) blockageH);
      }
    }

    for (int y = 0; y < y_grid_ - 1; y++) {
      for (int x = 0; x < x_grid_; x++) {
        int grid_v_ = y * x_grid_ + x + k * x_grid_ * (y_grid_ - 1);

        unsigned short capV = v_capacity_3D_[k];
        unsigned short blockageV = (v_capacity_3D_[k] - v_edges_3D_[grid_v_].cap);
        unsigned short usageV = v_edges_3D_[grid_v_].usage + blockageV;

        db_gcell->setVerticalCapacity(layer, x, y, (uint) capV);
        db_gcell->setVerticalUsage(layer, x, y, (uint) usageV);
        db_gcell->setVerticalBlockage(layer, x, y, (uint) blockageV);
      }
    }
  }
}

NetRouteMap FastRouteCore::run()
{
  int tUsage;
  int cost_step;
  int maxOverflow;
  int minoflrnd = 0;
  int bwcnt = 0;

  // TODO: check this size
  max_degree_ = 2 * max_degree_;
  xcor_.resize(max_degree_);
  ycor_.resize(max_degree_);
  dcor_.resize(max_degree_);
  net_eo_.reserve(max_degree_);

  int SLOPE = 5;
  int THRESH_M = 20;
  int ENLARGE = 15;     // 5
  int ESTEP1 = 10;  // 10
  int ESTEP2 = 5;   // 5
  int ESTEP3 = 5;   // 5
  int CSTEP1 = 2;   // 5
  int CSTEP2 = 2;   // 3
  int CSTEP3 = 5;   // 15
  int COSHEIGHT = 4;
  int L = 0;
  int VIA = 2;
  int Ripvalue = -1;
  int ripupTH3D = 10;
  bool goingLV = true;
  bool noADJ = false;
  int thStep1 = 10;
  int thStep2 = 4;
  int LVIter = 3;
  int mazeRound = 500;
  int bmfl = BIG_INT;
  int minofl = BIG_INT;
  float LOGIS_COF;
  int slope;
  int max_adj;

  // call FLUTE to generate RSMT and break the nets into segments (2-pin nets)

  clock_t t1 = clock();

  VIA = 2;

  via_cost_ = 0;
  gen_brk_RSMT(false, false, false, false, noADJ);
  if (verbose_ > 1)
    logger_->info(GRT, 97, "First L Route.");
  routeLAll(true);
  gen_brk_RSMT(true, true, true, false, noADJ);
  getOverflow2D(&maxOverflow);
  if (verbose_ > 1)
    logger_->info(GRT, 98, "Second L Route.");
  newrouteLAll(false, true);
  getOverflow2D(&maxOverflow);
  spiralRouteAll();
  newrouteZAll(10);
  if (verbose_ > 1)
    logger_->info(GRT, 99, "First Z Route.");
  int past_cong = getOverflow2D(&maxOverflow);

  convertToMazeroute();

  int enlarge_ = 10;
  int newTH = 10;
  int healingTrigger = 0;
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
    if (verbose_ > 1)
      logger_->info(GRT, 100, "LV routing round {}, enlarge {}.", i, enlarge_);
    routeLVAll(newTH, enlarge_, LOGIS_COF);

    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    enlarge_ += 5;
    newTH -= 5;
    if (newTH < 1) {
      newTH = 1;
    }
  }

  //  past_cong = getOverflow2Dmaze( &maxOverflow);

  clock_t t3 = clock();
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
  if (total_overflow_ > 0 && overflow_iterations_ > 0) {
    logger_->info(GRT, 101, "Running extra iterations to remove overflow.");
  }

  static const int max_overflow_increases_ = 15;

  // set overflow_increases as -1 since the first iteration always sum 1
  int overflow_increases = -1;
  int last_total_overflow = 0;
  while (total_overflow_ > 0 &&
         i <= overflow_iterations_ &&
         overflow_increases <= max_overflow_increases_) {
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
      updateCongestionHistory(i, upType, stopDEC, max_adj);
    } else if (total_overflow_ < 500) {
      cost_step = CSTEP3;
      enlarge_ += ESTEP3;
      ripup_threshold = -1;
      updateCongestionHistory(i, upType, stopDEC, max_adj);
    } else {
      cost_step = CSTEP2;
      enlarge_ += ESTEP2;
      updateCongestionHistory(i, upType, stopDEC, max_adj);
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
      stopDEC =true;
    }

    if (maxOverflow < 150) {
      if (i == 20 && past_cong > 200) {
        logger_->info(GRT, 103, "Extra Run for hard benchmark.");
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
      healingTrigger++;
    }

    if (past_cong < bmfl) {
      bwcnt = 0;
      if (i > 140 || (i > 80 && past_cong < 20)) {
        copyRS();
        bmfl = past_cong;

        L = 0;
        SLOPE = BIG_INT;
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
        SLOPE = 5;
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
    last_total_overflow = total_overflow_;
  } // end overflow iterations

  bool has_2D_overflow = total_overflow_ > 0;

  if (minofl > 0) {
    logger_->info(GRT, 104, "Minimal overflow {} occurring at round {}.", minofl, minoflrnd);
    copyBR();
  }

  if (overflow_increases > max_overflow_increases_) {
    logger_->warn(GRT,
                 230,
                 "Congestion iterations reached the maximum number of total overflow increases.");
  }

  freeRR();

  checkUsage();

  if (verbose_ > 1)
    logger_->info(GRT, 105, "Maze routing finished.");

  if (verbose_ > 1) {
    logger_->report("Final 2D results:");
  }
  getOverflow2Dmaze(&maxOverflow, &tUsage);

  if (verbose_ > 1)
    logger_->info(GRT, 106, "Layer assignment begins.");
  newLA();
  if (verbose_ > 1)
    logger_->info(GRT, 107, "Layer assignment finished.");

  clock_t t2 = clock();
  float gen_brk_Time = (float) (t2 - t1) / CLOCKS_PER_SEC;

  costheight_ = 3;
  via_cost_ = 1;

  if (gen_brk_Time < 60) {
    ripupTH3D = 15;
  } else if (gen_brk_Time < 120) {
    ripupTH3D = 18;
  } else {
    ripupTH3D = 20;
  }

  if (goingLV && past_cong == 0) {
    if (verbose_ > 1)
      logger_->info(GRT, 108, "Post-processing begins.");
    mazeRouteMSMDOrder3D(enlarge_, 0, ripupTH3D, layer_orientation_);

    if (gen_brk_Time > 120) {
      mazeRouteMSMDOrder3D(enlarge_, 0, 12, layer_orientation_);
    }
    if (verbose_ > 1)
      logger_->info(GRT, 109, "Post-processing finished.\n Starting via filling.");
  }

  fillVIA();
  int finallength = getOverflow3D();
  int numVia = threeDVIA();
  checkRoute3D();

  logger_->info(GRT, 111, "Final number of vias: {}", numVia);
  logger_->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));

  NetRouteMap routes = getRoutes();

  net_eo_.clear();

  updateDbCongestion();

  if (has_2D_overflow && !allow_overflow_) {
    logger_->error(GRT, 118, "Routing congestion too high.");
  }

  if (total_overflow_ > 0) {
    logger_->warn(GRT, 115, "Global routing finished with overflow.");
  }

  return routes;
}

void FastRouteCore::setVerbose(int v)
{
  verbose_ = v;
}

void FastRouteCore::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
}

void FastRouteCore::setAllowOverflow(bool allow)
{
  allow_overflow_ = allow;
}

std::vector<int> FastRouteCore::getOriginalResources()
{
  std::vector<int> original_resources;
  original_resources.resize(num_layers_);
  for (int l = 0; l < num_layers_; l++) {
    original_resources[l] += (v_capacity_3D_[l]+h_capacity_3D_[l])*y_grid_*x_grid_;
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
        int grid = i * (x_grid_ - 1) + j + l * (x_grid_ - 1) * y_grid_;
        cap_per_layer_[l] += h_edges_3D_[grid].cap;
        usage_per_layer_[l] += h_edges_3D_[grid].usage;

        int overflow = h_edges_3D_[grid].usage - h_edges_3D_[grid].cap;
        if (overflow > 0) {
          overflow_per_layer_[l] += overflow;
          max_h_overflow_[l] = std::max(max_h_overflow_[l], overflow);
        }
      }
    }
    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
        int grid = i * x_grid_ + j + l * x_grid_ * (y_grid_ - 1);
        cap_per_layer_[l] += v_edges_3D_[grid].cap;
        usage_per_layer_[l] += v_edges_3D_[grid].usage;

        int overflow = v_edges_3D_[grid].usage - v_edges_3D_[grid].cap;
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

const char* netName(FrNet* net)
{
  return getNetName(net->db_net);
}

}  // namespace grt
