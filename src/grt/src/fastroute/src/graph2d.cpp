// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Graph2D.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "DataType.h"
#include "FastRoute.h"

namespace grt {

// Initializes the 2D graph with grid dimensions, capacities, and layers.
void Graph2D::init(const int x_grid,
                   const int y_grid,
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
      h_edges_[x][y].cap = 0;
      h_edges_[x][y].usage = 0;
      h_edges_[x][y].est_usage = 0;
      h_edges_[x][y].red = 0;
      h_edges_[x][y].last_usage = 0;
      h_edges_[x][y].ndr_overflow = 0;
    }
  }
  for (int x = 0; x < x_grid; x++) {
    for (int y = 0; y < y_grid - 1; y++) {
      // Edge initialization
      v_edges_[x][y].cap = 0;
      v_edges_[x][y].usage = 0;
      v_edges_[x][y].est_usage = 0;
      v_edges_[x][y].red = 0;
      v_edges_[x][y].last_usage = 0;
      v_edges_[x][y].ndr_overflow = 0;
    }
  }
}

// Initializes the estimated usage of all edges to 0.
void Graph2D::InitEstUsage()
{
  foreachEdge([](Edge& edge) { edge.est_usage = 0; });
}

// Initializes the last usage of all edges based on the update type.
void Graph2D::InitLastUsage(const int upType)
{
  foreachEdge([](Edge& edge) { edge.last_usage = 0; });

  if (upType == 1) {
    foreachEdge([](Edge& edge) { edge.congCNT = 0; });
  } else if (upType == 2) {
    foreachEdge([](Edge& edge) { edge.last_usage = edge.last_usage * 0.2; });
  }
}

// Clears all horizontal and vertical edges from the graph.
void Graph2D::clear()
{
  h_edges_.resize(boost::extents[0][0]);
  v_edges_.resize(boost::extents[0][0]);
}

// Clears the sets of used horizontal and vertical grid cells.
void Graph2D::clearUsed()
{
  v_used_ggrid_.clear();
  h_used_ggrid_.clear();
}

// Clears the NDR lists
void Graph2D::clearNDRnets()
{
  for (auto row : v_ndr_nets_) {
    for (auto& ndr_set : row) {
      ndr_set.clear();
    }
  }
  for (auto row : h_ndr_nets_) {
    for (auto& ndr_set : row) {
      ndr_set.clear();
    }
  }
}

// Checks if the graph has any horizontal or vertical edges.
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

// Adds capacity to a horizontal edge.
void Graph2D::addCapH(const int x, const int y, const int cap)
{
  h_edges_[x][y].cap += cap;
}

// Adds capacity to a vertical edge.
void Graph2D::addCapV(const int x, const int y, const int cap)
{
  v_edges_[x][y].cap += cap;
}

// Updates estimated usage for a horizontal edge segment, considering NDRs.
void Graph2D::updateEstUsageH(const Interval& xi,
                              const int y,
                              FrNet* net,
                              const double usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateEstUsageH(x, y, net, usage);
  }
}

// Updates estimated usage for a horizontal edge, considering NDRs.
void Graph2D::updateEstUsageH(const int x,
                              const int y,
                              FrNet* net,
                              const double usage)
{
  h_edges_[x][y].est_usage
      += getCostNDRAware(net, x, y, usage, EdgeDirection::Horizontal);

  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

// Adds the estimated usage to the actual usage for all edges.
void Graph2D::addEstUsageToUsage()
{
  foreachEdge([](Edge& edge) { edge.usage += edge.est_usage; });
}

// Updates estimated usage for a vertical edge segment, considering NDRs.
void Graph2D::updateEstUsageV(const int x,
                              const Interval& yi,
                              FrNet* net,
                              const double usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateEstUsageV(x, y, net, usage);
  }
}

// Updates estimated usage for a vertical edge, considering NDRs.
void Graph2D::updateEstUsageV(const int x,
                              const int y,
                              FrNet* net,
                              const double usage)
{
  v_edges_[x][y].est_usage
      += getCostNDRAware(net, x, y, usage, EdgeDirection::Vertical);

  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

// Adds reduction to a horizontal edge.
void Graph2D::addRedH(const int x, const int y, const int red)
{
  auto& val = h_edges_[x][y].red;
  val = std::max(val + red, 0);
}

// Adds reduction to a vertical edge.
void Graph2D::addRedV(const int x, const int y, const int red)
{
  auto& val = v_edges_[x][y].red;
  val = std::max(val + red, 0);
}

// Adds usage to a horizontal edge segment.
void Graph2D::addUsageH(const Interval& xi, const int y, const int used)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    addUsageH(x, y, used);
  }
}

// Adds usage to a horizontal edge.
void Graph2D::addUsageH(const int x, const int y, const int used)
{
  h_edges_[x][y].usage += used;
  if (used > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

// Adds usage to a vertical edge segment.
void Graph2D::addUsageV(const int x, const Interval& yi, const int used)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    addUsageV(x, y, used);
  }
}

// Adds usage to a vertical edge.
void Graph2D::addUsageV(const int x, const int y, const int used)
{
  v_edges_[x][y].usage += used;
  if (used > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

// Updates the list of congested nets.
void Graph2D::updateCongList(const std::string& net_name,
                             const double edge_cost)
{
  if (edge_cost > 0) {
    congestion_nets_.insert(net_name);
  } else {
    congestion_nets_.erase(net_name);
  }
}

// Prints all congested nets (debug).
void Graph2D::printAllElements()
{
  if (congestion_nets_.empty()) {
    logger_->report("No congestion nets.");
    return;
  }

  logger_->reportLiteral(
      fmt::format("Congestion nets ({}): ", congestion_nets_.size()));
  for (auto it = congestion_nets_.begin(); it != congestion_nets_.end(); ++it) {
    if (it != congestion_nets_.begin()) {
      logger_->reportLiteral(", ");
    }
    logger_->reportLiteral(fmt::format("\"{}\"", *it));
  }
  logger_->reportLiteral("\n");
}

// Updates usage for a horizontal edge, considering NDRs.
void Graph2D::updateUsageH(const int x,
                           const int y,
                           FrNet* net,
                           const int usage)
{
  h_edges_[x][y].usage
      += getCostNDRAware(net, x, y, usage, EdgeDirection::Horizontal);

  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

// Updates usage for a horizontal edge segment.
void Graph2D::updateUsageH(const Interval& xi,
                           const int y,
                           FrNet* net,
                           const int usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateUsageH(x, y, net, usage);
  }
}

// Updates usage for a vertical edge, considering NDRs.
void Graph2D::updateUsageV(const int x,
                           const int y,
                           FrNet* net,
                           const int usage)
{
  v_edges_[x][y].usage
      += getCostNDRAware(net, x, y, usage, EdgeDirection::Vertical);

  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

// Updates usage for a vertical edge segment.
void Graph2D::updateUsageV(const int x,
                           const Interval& yi,
                           FrNet* net,
                           const int usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateUsageV(x, y, net, usage);
  }
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

// Updates the congestion history of edges.
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

// Accumulates stress on edges based on congestion.
void Graph2D::str_accu(const int rnd)
{
  foreachEdge([rnd](Edge& edge) {
    const int overflow = edge.usage - edge.cap;
    if (overflow > 0 || edge.congCNT > rnd) {
      edge.last_usage += edge.congCNT * overflow / 2;
    }
  });
}

// Applies a function to each edge in the graph.
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

void Graph2D::saveResources(const int x, const int y, bool is_horizontal)
{
  if (is_horizontal) {
    h_edges_[x][y].real_cap = h_edges_[x][y].cap;
  } else {
    v_edges_[x][y].real_cap = v_edges_[x][y].cap;
  }
}

bool Graph2D::computeSuggestedAdjustment(const int x,
                                         const int y,
                                         bool is_horizontal,
                                         int& adjustment)
{
  float real_capacity, usage;
  if (is_horizontal) {
    real_capacity = h_edges_[x][y].real_cap;
    usage = h_edges_[x][y].usage;
  } else {
    real_capacity = v_edges_[x][y].real_cap;
    usage = v_edges_[x][y].usage;
  }
  if (real_capacity >= usage) {
    adjustment = (1.0 - (usage / real_capacity)) * 100;
    return true;
  }
  return false;
}

// Initializes the NDR nets for each grid cell.
void Graph2D::initNDRnets()
{
  v_ndr_nets_.resize(boost::extents[x_grid_][y_grid_]);
  h_ndr_nets_.resize(boost::extents[x_grid_][y_grid_]);
}

void Graph2D::addCongestedNDRnet(const int net_id, const uint16_t num_edges)
{
  congested_ndrs_.emplace_back(net_id, num_edges);
}

void Graph2D::sortCongestedNDRnets()
{
  std::ranges::sort(congested_ndrs_, NDRCongestionComparator());
}

int Graph2D::getOneCongestedNDRnet()
{
  if (!congested_ndrs_.empty()) {
    return congested_ndrs_[0].net_id;
  }
  return -1;
}

// Get 10% of the NDR nets more involved in congestion
std::vector<int> Graph2D::getMultipleCongestedNDRnet()
{
  std::vector<int> net_ids;
  if (!congested_ndrs_.empty()) {
    for (int i = 0; i < ceil((double) congested_ndrs_.size() / 10); i++) {
      net_ids.push_back(congested_ndrs_[i].net_id);
    }
  }
  return net_ids;
}

// Initializes the 3D capacity of the graph.
void Graph2D::initCap3D()
{
  v_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
  h_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
  initNDRnets();
}

// Updates the 3D capacity of a specific edge.
void Graph2D::updateCap3D(int x,
                          int y,
                          int layer,
                          EdgeDirection direction,
                          const double cap)
{
  auto& cap3D = (EdgeDirection::Horizontal == direction)
                    ? h_cap_3D_[layer][x][y]
                    : v_cap_3D_[layer][x][y];
  cap3D.cap = cap;
  cap3D.cap_ndr = cap;
}

// Prints the capacity of each edge per layer.
void Graph2D::printEdgeCapPerLayer()
{
  logger_->report("=== printEdgeCapPerLayer ===");
  for (int y = 0; y < y_grid_; y++) {
    for (int x = 0; x < x_grid_; x++) {
      for (int l = 0; l < num_layers_; l++) {
        if (x < x_grid_ - 1) {
          logger_->report(
              "\tH x{} y{} l{}: {}", x, y, l, h_cap_3D_[l][x][y].cap);
        }
        if (y < y_grid_ - 1) {
          logger_->report(
              "\tV x{} y{} l{}: {}", x, y, l, v_cap_3D_[l][x][y].cap);
        }
      }
    }
  }
}

// Checks if there is enough NDR capacity for a given net.
bool Graph2D::hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction)
{
  const int8_t edgeCost = net->getEdgeCost();

  if (edgeCost == 1) {
    return true;
  }

  // For nets with high cost, we need at least one layer with sufficient
  // capacity
  for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
    double layer_cap = 0;
    int8_t layer_edge_cost = net->getLayerEdgeCost(l);

    layer_cap = (direction == EdgeDirection::Horizontal)
                    ? h_cap_3D_[l][x][y].cap_ndr
                    : v_cap_3D_[l][x][y].cap_ndr;

    if (layer_cap >= layer_edge_cost) {
      return true;
    }
  }

  // No single layer can accommodate this NDR net
  return false;
}

// Calculates the cost of an edge, considering NDRs.
double Graph2D::getCostNDRAware(FrNet* net,
                                int x,
                                int y,
                                const double edge_cost,
                                EdgeDirection direction)
{
  const int8_t edgeCost = net->getEdgeCost();

  // No processing needed for nets with 1 edge cost
  if (edgeCost == 1) {
    return edge_cost;
  }

  constexpr double OVERFLOW_COST_MULTIPLIER = 100.0;

  // Get references to the appropriate edge data based on direction
  auto& edge = (direction == EdgeDirection::Horizontal) ? h_edges_[x][y]
                                                        : v_edges_[x][y];
  auto& ndr_nets = (direction == EdgeDirection::Horizontal) ? h_ndr_nets_[x][y]
                                                            : v_ndr_nets_[x][y];

  const std::string& net_name = net->getName();
  bool is_net_present = ndr_nets.find(net) != ndr_nets.end();
  double final_edge_cost = 0;

  if (edge_cost < 0) {  // Rip-up: remove resource
    // If the net is in the list, remove it and compute the edge cost.
    // If the net is not in the list, it probably means that we are removing
    // half the edge cost a second time in the initial routing steps. But we
    // only need to count once to avoid problems when managing 3D capacity
    if (is_net_present) {
      ndr_nets.erase(net);
      // If the edge already has an overflow caused by NDR net we need to remove
      // the big edge cost value
      if (edge.ndr_overflow > 0) {
        edge.ndr_overflow--;
        final_edge_cost = -OVERFLOW_COST_MULTIPLIER * edgeCost;
      } else {
        final_edge_cost = -edgeCost;
      }
      updateNDRCapLayer(x, y, net, direction, edge_cost);
    }
  } else {  // Routing: add resource
    // If the net is not in the list, add it and compute the edge cost.
    // If the net is in the list, it probably means that we are in the
    // initial routing steps adding two times half the edge cost. But we
    // only need to count once to avoid problems when managing 3D capacity
    if (!is_net_present) {
      // If the edge already has an overflow caused by NDR net or it will have
      // an overflow due to lack of capacity in a single layer, we need to add
      // the big edge cost value
      if (edge.ndr_overflow > 0 || !hasNDRCapacity(net, x, y, direction)) {
        edge.ndr_overflow++;
        final_edge_cost = OVERFLOW_COST_MULTIPLIER * edgeCost;
      } else {
        final_edge_cost = edgeCost;
      }
      ndr_nets.insert(net);
      updateNDRCapLayer(x, y, net, direction, edge_cost);
    }
  }

  return final_edge_cost;
}

// Prints the NDR capacity of a specific grid cell.
void Graph2D::printNDRCap(const int x, const int y)
{
  logger_->report("=== PrintNDRCap (x{} y{}) ===", x, y);
  for (int l = 0; l < num_layers_; l++) {
    logger_->report("\tL{} - H Cap: {} NDR Cap: {} - V Cap: {} NDR Cap: {}",
                    l,
                    h_cap_3D_[l][x][y].cap,
                    h_cap_3D_[l][x][y].cap_ndr,
                    v_cap_3D_[l][x][y].cap,
                    v_cap_3D_[l][x][y].cap_ndr);
  }
}

// Updates the NDR capacity of a layer for a given net.
void Graph2D::updateNDRCapLayer(const int x,
                                const int y,
                                FrNet* net,
                                EdgeDirection dir,
                                const double edge_cost)
{
  const int8_t edgeCost = net->getEdgeCost();
  if (edgeCost == 1) {
    return;
  }

  auto& cap_3D = (dir == EdgeDirection::Horizontal) ? h_cap_3D_ : v_cap_3D_;
  int8_t layer_edge_cost = 0;

  for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
    auto& layer_cap = cap_3D[l][x][y];
    layer_edge_cost = net->getLayerEdgeCost(l);
    if (edge_cost < 0) {  // Reducing edge usage
      // If we already have a NDR net in this layer, increase the NDR capacity
      // available again
      if (layer_cap.cap - layer_cap.cap_ndr >= layer_edge_cost) {
        layer_cap.cap_ndr += layer_edge_cost;
        return;
      }
    } else {  // Increasing edge usage
      // If there is NDR capacity available, reduce the capacity value
      if (layer_cap.cap_ndr >= layer_edge_cost) {
        layer_cap.cap_ndr -= layer_edge_cost;
        return;
      }
    }
  }

  // If the edge is already with congestion and there is no capacity available
  // in any layer, reduce the capacity available of the first layer.
  // When rippin-up, it will be the first to be released
  if (edge_cost > 0) {
    layer_edge_cost = net->getLayerEdgeCost(net->getMinLayer());
    cap_3D[net->getMinLayer()][x][y].cap_ndr -= layer_edge_cost;
  }
}

}  // namespace grt
