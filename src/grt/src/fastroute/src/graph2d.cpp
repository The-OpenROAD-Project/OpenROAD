// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FastRoute.h"

namespace grt {

void Graph2D::init(const int x_grid,
                   const int y_grid,
                   const int h_capacity,
                   const int v_capacity,
                   const int num_layers,
                   utl::Logger* logger)
{
  x_grid_ = x_grid;
  y_grid_ = y_grid;
  num_layers_ = num_layers;
  logger_ = logger;

  h_edges_.resize(boost::extents[x_grid - 1][y_grid]);
  v_edges_.resize(boost::extents[x_grid][y_grid - 1]);

  for (int x = 0; x < x_grid - 1; x++) {
    for (int y = 0; y < y_grid; y++) {
      // Edge initialization
      h_edges_[x][y].cap = h_capacity;
      h_edges_[x][y].usage = 0;
      h_edges_[x][y].est_usage = 0;
      h_edges_[x][y].red = 0;
      h_edges_[x][y].last_usage = 0;
    }
  }
  for (int x = 0; x < x_grid; x++) {
    for (int y = 0; y < y_grid - 1; y++) {
      // Edge initialization
      v_edges_[x][y].cap = v_capacity;
      v_edges_[x][y].usage = 0;
      v_edges_[x][y].est_usage = 0;
      v_edges_[x][y].red = 0;
      v_edges_[x][y].last_usage = 0;
    }
  }
}

void Graph2D::InitEstUsage()
{
  foreachEdge([](Edge& edge) { edge.est_usage = 0; });
}

void Graph2D::InitLastUsage(const int upType)
{
  foreachEdge([](Edge& edge) { edge.last_usage = 0; });

  if (upType == 1) {
    foreachEdge([](Edge& edge) { edge.congCNT = 0; });
  } else if (upType == 2) {
    foreachEdge([](Edge& edge) { edge.last_usage = edge.last_usage * 0.2; });
  }
}

void Graph2D::clear()
{
  h_edges_.resize(boost::extents[0][0]);
  v_edges_.resize(boost::extents[0][0]);
}

void Graph2D::clearUsed()
{
  v_used_ggrid_.clear();
  h_used_ggrid_.clear();
}

bool Graph2D::hasEdges() const
{
  return !h_edges_.empty() && !v_edges_.empty();
}

uint16_t Graph2D::getUsageH(const int x, const int y) const
{
  return h_edges_[x][y].usage;
}

uint16_t Graph2D::getUsageV(const int x, const int y) const
{
  return v_edges_[x][y].usage;
}

int16_t Graph2D::getLastUsageH(int x, int y) const
{
  return h_edges_[x][y].last_usage;
}

int16_t Graph2D::getLastUsageV(int x, int y) const
{
  return v_edges_[x][y].last_usage;
}

double Graph2D::getEstUsageH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage;
}

double Graph2D::getEstUsageV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage;
}

uint16_t Graph2D::getUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].usage_red();
}

uint16_t Graph2D::getUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].usage_red();
}

double Graph2D::getEstUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage_red();
}

double Graph2D::getEstUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage_red();
}

int Graph2D::getOverflowH(const int x, const int y) const
{
  const auto& edge = h_edges_[x][y];
  return edge.usage - edge.cap;
}

int Graph2D::getOverflowV(const int x, const int y) const
{
  const auto& edge = v_edges_[x][y];
  return edge.usage - edge.cap;
}

uint16_t Graph2D::getCapH(int x, int y) const
{
  return h_edges_[x][y].cap;
}

uint16_t Graph2D::getCapV(int x, int y) const
{
  return v_edges_[x][y].cap;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsH() const
{
  return h_used_ggrid_;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsV() const
{
  return v_used_ggrid_;
}

void Graph2D::addCapH(const int x, const int y, const int cap)
{
  h_edges_[x][y].cap += cap;
}

void Graph2D::addCapV(const int x, const int y, const int cap)
{
  v_edges_[x][y].cap += cap;
}

void Graph2D::addEstUsageH(const Interval& xi, const int y, const double usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].est_usage += usage;
    if (usage != 0) { // TODO: check if the correct is ">" or "!="
      h_used_ggrid_.insert({x, y});
    }
  }
}



void Graph2D::updateEstUsageH(const Interval& xi, const int y, FrNet* net, const double edge_cost)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateEstUsageH(x, y, net, edge_cost);

    // logger_->report("EstUsage: {} x{} y{} - {}", net->getName(), x, y, getEstUsageH(x,y));
  }
}

void Graph2D::updateEstUsageH(const int x, const int y, FrNet* net, const double edge_cost)
{
  h_edges_[x][y].est_usage += getCostNDRAware(net, x, y, edge_cost, EdgeDirection::Horizontal);
  if (edge_cost != 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageH(const int x, const int y, const double usage)
{
  h_edges_[x][y].est_usage += usage;
  if (usage != 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageToUsage()
{
  foreachEdge([](Edge& edge) { edge.usage += edge.est_usage; });
}

void Graph2D::addEstUsageV(const int x, const Interval& yi, const double usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].est_usage += usage;
    if (usage != 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::updateEstUsageV(const int x, const Interval& yi, FrNet* net, const double edge_cost)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateEstUsageV(x, y, net, edge_cost);
    // logger_->report("EstUsage: {} x{} y{} - {}", net->getName(), x, y, getEstUsageV(x,y));
  }
}

void Graph2D::updateEstUsageV(const int x, const int y, FrNet* net, const double edge_cost)
{
  double usage = getCostNDRAware(net, x, y, edge_cost, EdgeDirection::Vertical);
  v_edges_[x][y].est_usage += usage;
  if (usage != 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageV(const int x, const int y, const double usage)
{
  v_edges_[x][y].est_usage += usage;
  if (usage != 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addRedH(const int x, const int y, const int red)
{
  auto& val = h_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addRedV(const int x, const int y, const int red)
{
  auto& val = v_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addUsageH(const Interval& xi, const int y, const int used)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].usage += used;
    if (used != 0) {
      h_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageV(const int x, const Interval& yi, const int used)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].usage += used;
    if (used != 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageH(const int x, const int y, const int used)
{
  h_edges_[x][y].usage += used;
  if (used != 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageH(const int x, const int y, FrNet* net, const int used)
{
  int usage = getCostNDRAware(net, x, y, used, EdgeDirection::Horizontal);
  h_edges_[x][y].usage += usage;
  if (usage != 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageH(const Interval& xi, const int y, FrNet* net, const int used)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateUsageH(x, y, net, used);
  }
}

void Graph2D::addUsageV(const int x, const int y, const int used)
{
  v_edges_[x][y].usage += used;
  if (used != 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageV(const int x, const int y, FrNet* net, const int used)
{
  int usage = getCostNDRAware(net, x, y, used, EdgeDirection::Vertical);
  v_edges_[x][y].usage += usage;
  if (usage != 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageV(const int x, const Interval& yi, FrNet* net, const int used)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateUsageV(x, y, net, used);
  }
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

void Graph2D::updateCongestionHistory(const int up_type,
                                      const int ahth,
                                      bool stop_decreasing,
                                      int& max_adj)
{
  int maxlimit = 0;

  if (up_type == 2) {
    stop_decreasing = max_adj < ahth;
  }

  auto updateEdges = [&](const auto& grid, auto& edges) {
    for (const auto& [x, y] : grid) {
      const int overflow = edges[x][y].usage - edges[x][y].cap;
      if (overflow > 0) {
        edges[x][y].congCNT++;
        edges[x][y].last_usage += overflow;
      } else if (!stop_decreasing) {
        if (up_type != 1) {
          edges[x][y].congCNT = std::max<int>(0, edges[x][y].congCNT - 1);
        }
        if (up_type != 3) {
          edges[x][y].last_usage *= 0.9;
        } else {
          edges[x][y].last_usage
              = std::max<int>(edges[x][y].last_usage + overflow, 0);
        }
      }
      maxlimit = std::max<int>(maxlimit, edges[x][y].last_usage);
    }
  };

  updateEdges(h_used_ggrid_, h_edges_);
  updateEdges(v_used_ggrid_, v_edges_);

  max_adj = maxlimit;
}

void Graph2D::str_accu(const int rnd)
{
  foreachEdge([rnd](Edge& edge) {
    const int overflow = edge.usage - edge.cap;
    if (overflow > 0 || edge.congCNT > rnd) {
      edge.last_usage += edge.congCNT * overflow / 2;
    }
  });
}

void Graph2D::foreachEdge(const std::function<void(Edge&)>& func)
{
  auto inner = [&](auto& edges) {
    Edge* edges_data = edges.data();

    const size_t num_edges = edges.num_elements();

    for (size_t i = 0; i < num_edges; ++i) {
      func(edges_data[i]);
    }
  };
  inner(h_edges_);
  inner(v_edges_);
}


void Graph2D::initCap3D()
{
  v_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
  h_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
}

void Graph2D::updateCap3D(int x, int y, int layer, EdgeDirection direction, const uint16_t cap)
{
  if (direction == EdgeDirection::Horizontal){
    h_cap_3D_[layer][x][y] = cap;
  }else {
    v_cap_3D_[layer][x][y] = cap;
  }
}

void Graph2D::printEdgeCapPerLayer()
{
  for (int y = 0; y < y_grid_; y++) {
    for (int x = 0; x < x_grid_; x++) {
      for (int l = 0; l < num_layers_; l++) {
        if (x < x_grid_ - 1) {
          logger_->report("H x{} y{} l{}: {}",x,y,l,h_cap_3D_[l][x][y]);
        }
        if (y < y_grid_ - 1) {
          logger_->report("V x{} y{} l{}: {}",x,y,l,v_cap_3D_[l][x][y]);
        }
      }
    }
  }
}

bool Graph2D::hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction)
{
  // Check if this is an NDR net
  bool is_ndr = (net->getDbNet()->getNonDefaultRule() != nullptr);
  const int edgeCost = net->getEdgeCost();
  int max_single_layer_cap = 0;
  
  if (is_ndr) {
      // For NDR nets, we need at least one layer with sufficient capacity
      for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        // int edgeCost = net->getLayerEdgeCost(l); // TODO: check if using layer cost is better
        int layer_cap = 0;
        if (direction == EdgeDirection::Horizontal) {
            layer_cap = h_cap_3D_[l][x][y];
        } else {
            layer_cap = v_cap_3D_[l][x][y];
        }
        max_single_layer_cap = std::max(max_single_layer_cap,layer_cap);
        if (layer_cap >= edgeCost) {
          return true; 
        }
      }
      // logger_->report("=== Max Layer Cap: {} x{} y{} max {}",net->getName(), x, y, max_single_layer_cap);
      // No single layer can accommodate this NDR net
      return false;
  }
  
  return true;
}

double Graph2D::getCostNDRAware(FrNet* net, int x, int y, const double edge_cost, EdgeDirection direction)
{  
  // No single layer can accommodate this NDR net
  if (!hasNDRCapacity(net, x, y, direction)) {
    return 100*edge_cost; 
  }
  return edge_cost;
}

int Graph2D::getCostNDRAware(FrNet* net, int x, int y, const int edge_cost, EdgeDirection direction)
{  
  // No single layer can accommodate this NDR net
  if (!hasNDRCapacity(net, x, y, direction)) {
    return 100*edge_cost; 
  }
  return edge_cost;
}

}  // namespace grt
