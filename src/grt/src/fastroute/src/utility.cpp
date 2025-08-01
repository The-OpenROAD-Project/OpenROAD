// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <queue>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "DataType.h"
#include "FastRoute.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace grt {

using utl::GRT;

void FastRouteCore::printEdge(const int netID, const int edgeID)
{
  const TreeEdge edge = sttrees_[netID].edges[edgeID];
  const auto& nodes = sttrees_[netID].nodes;

  logger_->report("edge {}: ({}, {})->({}, {})",
                  edgeID,
                  nodes[edge.n1].x,
                  nodes[edge.n1].y,
                  nodes[edge.n2].x,
                  nodes[edge.n2].y);
  std::string routes_rpt;
  for (int i = 0; i <= edge.route.routelen; i++) {
    routes_rpt
        += fmt::format("({}, {}) ", edge.route.gridsX[i], edge.route.gridsY[i]);
  }
  logger_->report("{}", routes_rpt);
}

void FastRouteCore::ConvertToFull3DType2()
{
  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        std::vector<int16_t> tmpX;
        std::vector<int16_t> tmpY;
        std::vector<int16_t> tmpL;
        int newCNT = 0;
        const int routeLen = treeedge->route.routelen;
        tmpX.reserve(routeLen + num_layers_);
        tmpY.reserve(routeLen + num_layers_);
        tmpL.reserve(routeLen + num_layers_);

        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;
        // finish from n1->real route
        int j;
        for (j = 0; j < routeLen; j++) {
          tmpX.push_back(gridsX[j]);
          tmpY.push_back(gridsY[j]);
          tmpL.push_back(gridsL[j]);
          newCNT++;

          if (gridsL[j] > gridsL[j + 1]) {
            for (int k = gridsL[j]; k > gridsL[j + 1]; k--) {
              tmpX.push_back(gridsX[j + 1]);
              tmpY.push_back(gridsY[j + 1]);
              tmpL.push_back(k);
              newCNT++;
            }
          } else if (gridsL[j] < gridsL[j + 1]) {
            for (int k = gridsL[j]; k < gridsL[j + 1]; k++) {
              tmpX.push_back(gridsX[j + 1]);
              tmpY.push_back(gridsY[j + 1]);
              tmpL.push_back(k);
              newCNT++;
            }
          }
        }
        tmpX.push_back(gridsX[j]);
        tmpY.push_back(gridsY[j]);
        tmpL.push_back(gridsL[j]);
        newCNT++;
        // last grid -> node2 finished
        if (treeedges[edgeID].route.type == RouteType::MazeRoute) {
          treeedges[edgeID].route.gridsX.clear();
          treeedges[edgeID].route.gridsY.clear();
          treeedges[edgeID].route.gridsL.clear();
        }
        treeedge->route.gridsX.resize(newCNT, 0);
        treeedge->route.gridsY.resize(newCNT, 0);
        treeedge->route.gridsL.resize(newCNT, 0);
        treeedge->route.type = RouteType::MazeRoute;
        treeedge->route.routelen = newCNT - 1;

        for (int k = 0; k < newCNT; k++) {
          treeedge->route.gridsX[k] = tmpX[k];
          treeedge->route.gridsY[k] = tmpY[k];
          treeedge->route.gridsL[k] = tmpL[k];
        }
      }
    }
  }
}

static bool compareNetPins(const OrderNetPin& a, const OrderNetPin& b)
{
  return std::tie(a.length_per_pin, a.minX, a.treeIndex)
         < std::tie(b.length_per_pin, b.minX, b.treeIndex);
}

void FastRouteCore::netpinOrderInc()
{
  tree_order_pv_.clear();

  for (const int& netID : net_ids_) {
    int16_t xmin = std::numeric_limits<int16_t>::max();
    int totalLength = 0;
    const auto& treenodes = sttrees_[netID].nodes;
    const StTree* stree = &(sttrees_[netID]);
    const int num_edges = stree->num_edges();
    for (int ind = 0; ind < num_edges; ind++) {
      totalLength += stree->edges[ind].len;
      xmin = std::min(xmin, treenodes[stree->edges[ind].n1].x);
    }

    const float length_per_pin = (float) totalLength / stree->num_terminals;

    tree_order_pv_.push_back({netID, xmin, length_per_pin});
  }

  std::stable_sort(
      tree_order_pv_.begin(), tree_order_pv_.end(), compareNetPins);
}

void FastRouteCore::fillVIA()
{
  int numVIAT1 = 0;
  int numVIAT2 = 0;

  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    const int num_terminals = sttrees_[netID].num_terminals;
    const auto& treenodes = sttrees_[netID].nodes;

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      int node1_alias = treeedge->n1a;
      int node2_alias = treeedge->n2a;
      if (treeedge->len > 0) {
        std::vector<int16_t> tmpX;
        std::vector<int16_t> tmpY;
        std::vector<int16_t> tmpL;
        int newCNT = 0;
        int routeLen = treeedge->route.routelen;
        tmpX.reserve(routeLen + num_layers_);
        tmpY.reserve(routeLen + num_layers_);
        tmpL.reserve(routeLen + num_layers_);

        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        if (treenodes[node1_alias].hID == edgeID
            || (edgeID == treenodes[node1_alias].lID
                && treenodes[node1_alias].hID == BIG_INT
                && node1_alias < num_terminals)) {
          int bottom_layer = treenodes[node1_alias].botL;
          int top_layer = treenodes[node1_alias].topL;
          int edge_init_layer = gridsL[0];
          if (node1_alias < num_terminals) {
            int16_t pin_botL, pin_topL;
            getViaStackRange(netID, node1_alias, pin_botL, pin_topL);
            bottom_layer = std::min((int) pin_botL, bottom_layer);
            top_layer = std::max((int) pin_topL, top_layer);

            for (int l = bottom_layer; l < top_layer; l++) {
              tmpX.push_back(gridsX[0]);
              tmpY.push_back(gridsY[0]);
              tmpL.push_back(l);
              newCNT++;
              numVIAT1++;
            }

            for (int l = top_layer; l > edge_init_layer; l--) {
              tmpX.push_back(gridsX[0]);
              tmpY.push_back(gridsY[0]);
              tmpL.push_back(l);
              newCNT++;
            }
          } else {
            for (int l = bottom_layer; l < edge_init_layer; l++) {
              tmpX.push_back(gridsX[0]);
              tmpY.push_back(gridsY[0]);
              tmpL.push_back(l);
              newCNT++;
              if (node1_alias >= num_terminals) {
                numVIAT2++;
              }
            }
          }
        }

        for (int j = 0; j <= routeLen; j++) {
          tmpX.push_back(gridsX[j]);
          tmpY.push_back(gridsY[j]);
          tmpL.push_back(gridsL[j]);
          newCNT++;
        }

        if (routeLen <= 0) {
          logger_->error(GRT, 254, "Edge has no previous routing.");
        }

        if (treenodes[node2_alias].hID == edgeID
            || (edgeID == treenodes[node2_alias].lID
                && treenodes[node2_alias].hID == BIG_INT
                && node2_alias < num_terminals)) {
          int bottom_layer = treenodes[node2_alias].botL;
          int top_layer = treenodes[node2_alias].topL;
          if (node2_alias < num_terminals) {
            int16_t pin_botL, pin_topL;
            getViaStackRange(netID, node2_alias, pin_botL, pin_topL);
            bottom_layer = std::min((int) pin_botL, bottom_layer);
            top_layer = std::max((int) pin_topL, top_layer);
            if (bottom_layer == tmpL[newCNT - 1]) {
              bottom_layer++;
            }

            for (int16_t l = tmpL[newCNT - 1] - 1; l > bottom_layer; l--) {
              tmpX.push_back(tmpX[newCNT - 1]);
              tmpY.push_back(tmpY[newCNT - 1]);
              tmpL.push_back(l);
              newCNT++;
            }

            for (int l = bottom_layer; l <= top_layer; l++) {
              tmpX.push_back(tmpX[newCNT - 1]);
              tmpY.push_back(tmpY[newCNT - 1]);
              tmpL.push_back(l);
              newCNT++;
              numVIAT1++;
            }
          } else {
            for (int l = top_layer - 1; l >= bottom_layer; l--) {
              tmpX.push_back(tmpX[newCNT - 1]);
              tmpY.push_back(tmpY[newCNT - 1]);
              tmpL.push_back(l);
              newCNT++;
              if (node1_alias >= num_terminals) {
                numVIAT2++;
              }
            }
          }
        }

        // Update the edge's route only if there were VIAs added for this edge
        if (newCNT != routeLen) {
          if (treeedges[edgeID].route.type == RouteType::MazeRoute) {
            treeedges[edgeID].route.gridsX.clear();
            treeedges[edgeID].route.gridsY.clear();
            treeedges[edgeID].route.gridsL.clear();
          }
          treeedge->route.gridsX.resize(newCNT, 0);
          treeedge->route.gridsY.resize(newCNT, 0);
          treeedge->route.gridsL.resize(newCNT, 0);
          treeedge->route.type = RouteType::MazeRoute;
          treeedge->route.routelen = newCNT - 1;

          for (int k = 0; k < newCNT; k++) {
            treeedge->route.gridsX[k] = tmpX[k];
            treeedge->route.gridsY[k] = tmpY[k];
            treeedge->route.gridsL[k] = tmpL[k];
          }
        }
      } else if ((treenodes[treeedge->n1].hID == BIG_INT
                  && treenodes[treeedge->n1].lID == BIG_INT)
                 || (treenodes[treeedge->n2].hID == BIG_INT
                     && treenodes[treeedge->n2].lID == BIG_INT)) {
        int node1 = treeedge->n1;
        int node2 = treeedge->n2;
        if ((treenodes[node1].botL == num_layers_
             && treenodes[node1].topL == -1)
            || (treenodes[node2].botL == num_layers_
                && treenodes[node2].topL == -1)) {
          continue;
        }

        int l1 = treenodes[node1].botL;
        int l2 = treenodes[node2].botL;
        int bottom_layer = std::min(l1, l2);
        int top_layer = std::max(l1, l2);
        if (node1 < num_terminals) {
          int16_t pin_botL, pin_topL;
          getViaStackRange(netID, node1, pin_botL, pin_topL);
          bottom_layer = std::min((int) pin_botL, bottom_layer);
          top_layer = std::max((int) pin_topL, top_layer);
        }

        if (node2 < num_terminals) {
          int16_t pin_botL, pin_topL;
          getViaStackRange(netID, node2, pin_botL, pin_topL);
          bottom_layer = std::min((int) pin_botL, bottom_layer);
          top_layer = std::max((int) pin_topL, top_layer);
        }

        treeedge->route.gridsX.resize(top_layer - bottom_layer + 1, 0);
        treeedge->route.gridsY.resize(top_layer - bottom_layer + 1, 0);
        treeedge->route.gridsL.resize(top_layer - bottom_layer + 1, 0);
        treeedge->route.type = RouteType::MazeRoute;
        treeedge->route.routelen = top_layer - bottom_layer;

        int count = 0;
        for (int l = bottom_layer; l <= top_layer; l++) {
          treeedge->route.gridsX[count] = treenodes[node1].x;
          treeedge->route.gridsY[count] = treenodes[node1].y;
          treeedge->route.gridsL[count] = l;
          count++;
        }
      }
    }
  }

  if (verbose_) {
    logger_->info(GRT, 197, "Via related to pin nodes: {}", numVIAT1);
    logger_->info(GRT, 198, "Via related Steiner nodes: {}", numVIAT2);
    logger_->info(GRT, 199, "Via filling finished.");
  }
}

/*returns the start and end of the stack necessary to reach a node*/
void FastRouteCore::getViaStackRange(const int netID,
                                     const int nodeID,
                                     int16_t& bot_pin_l,
                                     int16_t& top_pin_l)
{
  FrNet* net = nets_[netID];
  const auto& treenodes = sttrees_[netID].nodes;
  int node_x = treenodes[nodeID].x;
  int node_y = treenodes[nodeID].y;
  bot_pin_l = SHRT_MAX;
  top_pin_l = -1;

  const auto& pin_X = net->getPinX();
  const auto& pin_Y = net->getPinY();
  const auto& pin_L = net->getPinL();

  for (int p = 0; p < pin_L.size(); p++) {
    if (pin_X[p] == node_x && pin_Y[p] == node_y) {
      bot_pin_l = std::min(bot_pin_l, (int16_t) pin_L[p]);
      top_pin_l = std::max(top_pin_l, (int16_t) pin_L[p]);
    }
  }
}

int FastRouteCore::threeDVIA()
{
  int numVIA = 0;

  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);

      if (treeedge->len > 0) {
        int routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        for (int j = 0; j < routeLen; j++) {
          if (gridsL[j] != gridsL[j + 1]) {
            numVIA++;
          }
        }
      }
    }
  }

  return (numVIA);
}

void FastRouteCore::fixEdgeAssignment(int& net_layer,
                                      const multi_array<Edge3D, 3>& edges_3D,
                                      const int x,
                                      const int y,
                                      const int k,
                                      const int l,
                                      const bool vertical,
                                      int& best_cost,
                                      multi_array<int, 2>& layer_grid)
{
  const bool is_vertical
      = layer_directions_[l] == odb::dbTechLayerDir::VERTICAL;
  // if layer direction doesn't match edge direction or
  // if already found a layer for the edge, ignores the remaining layers
  if (is_vertical != vertical || best_cost > 0) {
    layer_grid[l][k] = std::numeric_limits<int>::min();
  } else {
    layer_grid[l][k] = edges_3D[l][y][x].cap - edges_3D[l][y][x].usage;
    best_cost = std::max(best_cost, layer_grid[l][k]);
    if (best_cost > 0) {
      // set the new min/max routing layer for the net to avoid
      // errors during mazeRouteMSMDOrder3D
      net_layer = l;
    }
  }
}

void FastRouteCore::assignEdge(const int netID,
                               const int edgeID,
                               const bool processDIR)
{
  int k;
  int endLayer = 0;

  FrNet* net = nets_[netID];
  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;
  TreeEdge* treeedge = &(treeedges[edgeID]);

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  std::vector<short>& gridsL = treeedge->route.gridsL;

  const int routelen = treeedge->route.routelen;
  const int n1a = treeedge->n1a;
  const int n2a = treeedge->n2a;

  std::vector<std::vector<long>> gridD;
  gridD.resize(num_layers_);
  for (int i = 0; i < num_layers_; i++) {
    gridD[i].resize(treeedge->route.routelen + 1);
  }

  multi_array<int, 2> via_link;
  via_link.resize(boost::extents[num_layers_][routelen + 1]);
  for (int l = 0; l < num_layers_; l++) {
    for (k = 0; k <= routelen; k++) {
      gridD[l][k] = BIG_INT;
      via_link[l][k] = BIG_INT;
    }
  }

  multi_array<int, 2> layer_grid;
  layer_grid.resize(boost::extents[num_layers_][routelen + 1]);
  for (k = 0; k < routelen; k++) {
    int best_cost = std::numeric_limits<int>::min();
    if (gridsX[k] == gridsX[k + 1]) {
      const int min_y = std::min(gridsY[k], gridsY[k + 1]);
      for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        // check if the current layer is vertical to match the edge orientation
        bool is_vertical
            = layer_directions_[l] == odb::dbTechLayerDir::VERTICAL;
        if (is_vertical) {
          layer_grid[l][k] = v_edges_3D_[l][min_y][gridsX[k]].cap
                             - v_edges_3D_[l][min_y][gridsX[k]].usage;
          best_cost = std::max(best_cost, layer_grid[l][k]);
        } else {
          layer_grid[l][k] = std::numeric_limits<int>::min();
        }
      }

      // assigning the edge to the layer range would cause overflow try to
      // assign the edge to the closest layer below the min routing layer.
      // if design has 2D overflow, accept the congestion in layer assignment
      if (best_cost <= 0 && !has_2D_overflow_) {
        int min_layer = net->getMinLayer();
        for (int l = net->getMinLayer() - 1; l >= 0; l--) {
          fixEdgeAssignment(min_layer,
                            v_edges_3D_,
                            gridsX[k],
                            min_y,
                            k,
                            l,
                            true,
                            best_cost,
                            layer_grid);
        }
        net->setMinLayer(min_layer);
        // try to assign the edge to the closest layer above the max routing
        // layer
        int max_layer = net->getMaxLayer();
        for (int l = net->getMaxLayer() + 1; l < num_layers_; l++) {
          fixEdgeAssignment(max_layer,
                            v_edges_3D_,
                            gridsX[k],
                            min_y,
                            k,
                            l,
                            true,
                            best_cost,
                            layer_grid);
        }
        net->setMaxLayer(max_layer);
      } else {  // the edge was assigned to a layer without causing overflow
        for (int l = 0; l < num_layers_; l++) {
          if (l < net->getMinLayer() || l > net->getMaxLayer()) {
            layer_grid[l][k] = std::numeric_limits<int>::min();
          }
        }
      }
    } else {
      const int min_x = std::min(gridsX[k], gridsX[k + 1]);
      for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        // check if the current layer is horizontal to match the edge
        // orientation
        bool is_horizontal
            = layer_directions_[l] == odb::dbTechLayerDir::HORIZONTAL;
        if (is_horizontal) {
          layer_grid[l][k] = h_edges_3D_[l][gridsY[k]][min_x].cap
                             - h_edges_3D_[l][gridsY[k]][min_x].usage;
          best_cost = std::max(best_cost, layer_grid[l][k]);
        } else {
          layer_grid[l][k] = std::numeric_limits<int>::min();
        }
      }

      // assigning the edge to the layer range would cause overflow try to
      // assign the edge to the closest layer below the min routing layer.
      // if design has 2D overflow, accept the congestion in layer assignment
      if (best_cost <= 0 && !has_2D_overflow_) {
        int min_layer = net->getMinLayer();
        for (int l = net->getMinLayer() - 1; l >= 0; l--) {
          fixEdgeAssignment(min_layer,
                            h_edges_3D_,
                            min_x,
                            gridsY[k],
                            k,
                            l,
                            false,
                            best_cost,
                            layer_grid);
        }
        net->setMinLayer(min_layer);
        // try to assign the edge to the closest layer above the max routing
        // layer
        int max_layer = net->getMaxLayer();
        for (int l = net->getMaxLayer() + 1; l < num_layers_; l++) {
          fixEdgeAssignment(max_layer,
                            h_edges_3D_,
                            min_x,
                            gridsY[k],
                            k,
                            l,
                            false,
                            best_cost,
                            layer_grid);
        }
        net->setMaxLayer(max_layer);
      } else {  // the edge was assigned to a layer without causing overflow
        for (int l = 0; l < num_layers_; l++) {
          if (l < net->getMinLayer() || l > net->getMaxLayer()) {
            layer_grid[l][k] = std::numeric_limits<int>::min();
          }
        }
      }
    }
  }

  if (processDIR) {
    if (treenodes[n1a].assigned) {
      for (int l = treenodes[n1a].botL; l <= treenodes[n1a].topL; l++) {
        gridD[l][0] = 0;
      }
    } else {
      if (verbose_) {
        logger_->warn(GRT, 200, "Start point not assigned.");
      }
      fflush(stdout);
    }

    for (k = 0; k < routelen; k++) {
      for (int l = 0; l < num_layers_; l++) {
        for (int i = 0; i < num_layers_; i++) {
          if (k == 0) {
            if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
              gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
              via_link[i][k] = l;
            }
          } else {
            if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
              gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
              via_link[i][k] = l;
            }
          }
        }
      }
      for (int l = 0; l < num_layers_; l++) {
        if (layer_grid[l][k] > 0) {
          gridD[l][k + 1] = gridD[l][k] + 1;
        } else if (layer_grid[l][k] == std::numeric_limits<int>::min()
                   || l < net->getMinLayer() || l > net->getMaxLayer()) {
          // when the layer orientation doesn't match the edge orientation,
          // set a larger weight to avoid assigning to this layer when the
          // routing has 3D overflow
          gridD[l][k + 1] = gridD[l][k] + 2 * BIG_INT;
        } else {
          gridD[l][k + 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (int l = 0; l < num_layers_; l++) {
      for (int i = 0; i < num_layers_; i++) {
        if (gridD[i][k] > gridD[l][k] + abs(i - l) * 1) {
          gridD[i][k] = gridD[l][k] + abs(i - l) * 1;
          via_link[i][k] = l;
        }
      }
    }

    k = routelen;

    if (treenodes[n2a].assigned) {
      int min_result = BIG_INT;
      for (int i = treenodes[n2a].topL; i >= treenodes[n2a].botL; i--) {
        if (gridD[i][routelen] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    } else {
      int min_result = gridD[0][routelen];
      endLayer = 0;
      for (int i = 0; i < num_layers_; i++) {
        if (gridD[i][routelen] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    }

    int last_layer;
    if (via_link[endLayer][routelen] == BIG_INT) {
      last_layer = endLayer;
    } else {
      last_layer = via_link[endLayer][routelen];
    }

    for (k = routelen; k >= 0; k--) {
      gridsL[k] = last_layer;
      if (via_link[last_layer][k] != BIG_INT) {
        last_layer = via_link[last_layer][k];
      }
    }

    if (gridsL[0] < treenodes[n1a].botL) {
      treenodes[n1a].botL = gridsL[0];
      treenodes[n1a].lID = edgeID;
    }
    if (gridsL[0] > treenodes[n1a].topL) {
      treenodes[n1a].topL = gridsL[0];
      treenodes[n1a].hID = edgeID;
    }

    if (treenodes[n2a].assigned) {
      if (gridsL[routelen] < treenodes[n2a].botL) {
        treenodes[n2a].botL = gridsL[routelen];
        treenodes[n2a].lID = edgeID;
      }
      if (gridsL[routelen] > treenodes[n2a].topL) {
        treenodes[n2a].topL = gridsL[routelen];
        treenodes[n2a].hID = edgeID;
      }

    } else {
      treenodes[n2a].topL = gridsL[routelen];
      treenodes[n2a].botL = gridsL[routelen];
      treenodes[n2a].lID = treenodes[n2a].hID = edgeID;
    }

    if (treenodes[n2a].assigned) {
      if (gridsL[routelen] > treenodes[n2a].topL
          || gridsL[routelen] < treenodes[n2a].botL) {
        logger_->error(GRT,
                       202,
                       "Target ending layer ({}) out of range.",
                       gridsL[routelen]);
      }
    }

  } else {
    if (treenodes[n2a].assigned) {
      for (int l = treenodes[n2a].botL; l <= treenodes[n2a].topL; l++) {
        gridD[l][routelen] = 0;
      }
    }

    for (k = routelen; k > 0; k--) {
      for (int l = 0; l < num_layers_; l++) {
        for (int i = 0; i < num_layers_; i++) {
          if (k == routelen) {
            if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
              gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
              via_link[i][k] = l;
            }
          } else {
            if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
              gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
              via_link[i][k] = l;
            }
          }
        }
      }
      for (int l = 0; l < num_layers_; l++) {
        if (layer_grid[l][k - 1] > 0) {
          gridD[l][k - 1] = gridD[l][k] + 1;
        } else if (layer_grid[l][k] == std::numeric_limits<int>::min()
                   || l < net->getMinLayer() || l > net->getMaxLayer()) {
          // when the layer orientation doesn't match the edge orientation,
          // set a larger weight to avoid assigning to this layer when the
          // routing has 3D overflow
          gridD[l][k - 1] = gridD[l][k] + 2 * BIG_INT;
        } else {
          gridD[l][k - 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (int l = 0; l < num_layers_; l++) {
      for (int i = 0; i < num_layers_; i++) {
        if (gridD[i][0] > gridD[l][0] + abs(i - l) * 1) {
          gridD[i][0] = gridD[l][0] + abs(i - l) * 1;
          via_link[i][0] = l;
        }
      }
    }

    if (treenodes[n1a].assigned) {
      int min_result = BIG_INT;
      for (int i = treenodes[n1a].topL; i >= treenodes[n1a].botL; i--) {
        if (gridD[i][k] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][0];
          endLayer = i;
        }
      }

    } else {
      int min_result = gridD[0][k];
      endLayer = 0;
      for (int i = 0; i < num_layers_; i++) {
        if (gridD[i][k] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][k];
          endLayer = i;
        }
      }
    }

    int last_layer = endLayer;

    for (k = 0; k <= routelen; k++) {
      if (via_link[last_layer][k] != BIG_INT) {
        last_layer = via_link[last_layer][k];
      }
      gridsL[k] = last_layer;
    }

    gridsL[routelen] = gridsL[routelen - 1];

    if (gridsL[routelen] < treenodes[n2a].botL) {
      treenodes[n2a].botL = gridsL[routelen];
      treenodes[n2a].lID = edgeID;
    }
    if (gridsL[routelen] > treenodes[n2a].topL) {
      treenodes[n2a].topL = gridsL[routelen];
      treenodes[n2a].hID = edgeID;
    }

    if (treenodes[n1a].assigned) {
      if (gridsL[0] < treenodes[n1a].botL) {
        treenodes[n1a].botL = gridsL[0];
        treenodes[n1a].lID = edgeID;
      }
      if (gridsL[0] > treenodes[n1a].topL) {
        treenodes[n1a].topL = gridsL[0];
        treenodes[n1a].hID = edgeID;
      }

    } else {
      // treenodes[n1a].assigned = true;
      treenodes[n1a].topL = gridsL[0];  // std::max(endLayer, gridsL[0]);
      treenodes[n1a].botL = gridsL[0];  // std::min(endLayer, gridsL[0]);
      treenodes[n1a].lID = treenodes[n1a].hID = edgeID;
    }
  }
  treeedge->assigned = true;

  for (k = 0; k < routelen; k++) {
    if (gridsX[k] == gridsX[k + 1]) {
      const int min_y = std::min(gridsY[k], gridsY[k + 1]);

      v_edges_3D_[gridsL[k]][min_y][gridsX[k]].usage
          += net->getLayerEdgeCost(gridsL[k]);
    } else {
      const int min_x = std::min(gridsX[k], gridsX[k + 1]);

      h_edges_3D_[gridsL[k]][gridsY[k]][min_x].usage
          += net->getLayerEdgeCost(gridsL[k]);
    }
  }
}

void FastRouteCore::layerAssignmentV4()
{
  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        const int routeLen = treeedge->route.routelen;
        treeedge->route.gridsL.resize(routeLen + 1, 0);
        treeedge->assigned = false;
      }
    }
  }
  netpinOrderInc();

  std::queue<int> edgeQueue;
  for (int i = 0; i < tree_order_pv_.size(); i++) {
    int netID = tree_order_pv_[i].treeIndex;

    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;
    const int num_terminals = sttrees_[netID].num_terminals;

    for (int nodeID = 0; nodeID < num_terminals; nodeID++) {
      for (int k = 0; k < treenodes[nodeID].conCNT; k++) {
        const int edgeID = treenodes[nodeID].eID[k];
        if (!treeedges[edgeID].assigned) {
          edgeQueue.push(edgeID);
          treeedges[edgeID].assigned = true;
        }
      }
    }

    while (!edgeQueue.empty()) {
      const int edgeID = edgeQueue.front();
      edgeQueue.pop();
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treenodes[treeedge->n1a].assigned) {
        assignEdge(netID, edgeID, true);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n2a].assigned) {
          for (int k = 0; k < treenodes[treeedge->n2a].conCNT; k++) {
            const int edgeID2 = treenodes[treeedge->n2a].eID[k];
            if (!treeedges[edgeID2].assigned) {
              edgeQueue.push(edgeID2);
              treeedges[edgeID2].assigned = true;
            }
          }
          treenodes[treeedge->n2a].assigned = true;
        }
      } else {
        assignEdge(netID, edgeID, false);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n1a].assigned) {
          for (int k = 0; k < treenodes[treeedge->n1a].conCNT; k++) {
            const int edgeID1 = treenodes[treeedge->n1a].eID[k];
            if (!treeedges[edgeID1].assigned) {
              edgeQueue.push(edgeID1);
              treeedges[edgeID1].assigned = true;
            }
          }
          treenodes[treeedge->n1a].assigned = true;
        }
      }
    }

    for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes(); nodeID++) {
      treenodes[nodeID].topL = -1;
      treenodes[nodeID].botL = num_layers_;
      treenodes[nodeID].conCNT = 0;
      treenodes[nodeID].hID = BIG_INT;
      treenodes[nodeID].lID = BIG_INT;
      treenodes[nodeID].status = 0;
      treenodes[nodeID].assigned = false;

      if (nodeID < num_terminals) {
        treenodes[nodeID].botL = nets_[netID]->getPinL()[nodeID];
        treenodes[nodeID].topL = nets_[netID]->getPinL()[nodeID];
        treenodes[nodeID].assigned = true;
        treenodes[nodeID].status = 1;
      }
    }

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);

      if (treeedge->len > 0) {
        const int routeLen = treeedge->route.routelen;

        const int n1 = treeedge->n1;
        const int n2 = treeedge->n2;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        const int n1a = treenodes[n1].stackAlias;
        const int n2a = treenodes[n2].stackAlias;
        const int connectionCNT1 = treenodes[n1a].conCNT;
        treenodes[n1a].heights[connectionCNT1] = gridsL[0];
        treenodes[n1a].eID[connectionCNT1] = edgeID;
        treenodes[n1a].conCNT++;

        if (gridsL[0] > treenodes[n1a].topL) {
          treenodes[n1a].hID = edgeID;
          treenodes[n1a].topL = gridsL[0];
        }
        if (gridsL[0] < treenodes[n1a].botL) {
          treenodes[n1a].lID = edgeID;
          treenodes[n1a].botL = gridsL[0];
        }

        treenodes[n1a].assigned = true;

        const int connectionCNT2 = treenodes[n2a].conCNT;
        treenodes[n2a].heights[connectionCNT2] = gridsL[routeLen];
        treenodes[n2a].eID[connectionCNT2] = edgeID;
        treenodes[n2a].conCNT++;
        if (gridsL[routeLen] > treenodes[n2a].topL) {
          treenodes[n2a].hID = edgeID;
          treenodes[n2a].topL = gridsL[routeLen];
        }
        if (gridsL[routeLen] < treenodes[n2a].botL) {
          treenodes[n2a].lID = edgeID;
          treenodes[n2a].botL = gridsL[routeLen];
        }

        treenodes[n2a].assigned = true;
      }
    }
  }
}

void FastRouteCore::layerAssignment()
{
  for (const int& netID : net_ids_) {
    auto& treenodes = sttrees_[netID].nodes;

    int numpoints = 0;

    for (int d = 0; d < sttrees_[netID].num_nodes(); d++) {
      treenodes[d].topL = -1;
      treenodes[d].botL = num_layers_;
      // treenodes[d].l = 0;
      treenodes[d].assigned = false;
      treenodes[d].stackAlias = d;
      treenodes[d].conCNT = 0;
      treenodes[d].hID = BIG_INT;
      treenodes[d].lID = BIG_INT;
      treenodes[d].status = 0;

      if (d < sttrees_[netID].num_terminals) {
        treenodes[d].botL = nets_[netID]->getPinL()[d];
        treenodes[d].topL = nets_[netID]->getPinL()[d];
        // treenodes[d].l = 0;
        treenodes[d].assigned = true;
        treenodes[d].status = 1;

        xcor_[numpoints] = treenodes[d].x;
        ycor_[numpoints] = treenodes[d].y;
        dcor_[numpoints] = d;
        numpoints++;
      } else {
        bool redundant = false;
        for (int k = 0; k < numpoints; k++) {
          if ((treenodes[d].x == xcor_[k]) && (treenodes[d].y == ycor_[k])) {
            treenodes[d].stackAlias = dcor_[k];

            redundant = true;
            break;
          }
        }
        if (!redundant) {
          xcor_[numpoints] = treenodes[d].x;
          ycor_[numpoints] = treenodes[d].y;
          dcor_[numpoints] = d;
          numpoints++;
        }
      }
    }
  }

  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        const int n1 = treeedge->n1;
        const int n2 = treeedge->n2;

        treeedge->n1a = treenodes[n1].stackAlias;
        treenodes[treeedge->n1a].eID[treenodes[treeedge->n1a].conCNT] = edgeID;
        treenodes[treeedge->n1a].conCNT++;
        treeedge->n2a = treenodes[n2].stackAlias;
        treenodes[treeedge->n2a].eID[treenodes[treeedge->n2a].conCNT] = edgeID;
        treenodes[treeedge->n2a].conCNT++;
      }
    }
  }

  layerAssignmentV4();

  ConvertToFull3DType2();
}

void FastRouteCore::printEdge3D(const int netID, const int edgeID)
{
  const TreeEdge edge = sttrees_[netID].edges[edgeID];
  const auto& nodes = sttrees_[netID].nodes;

  logger_->report("\tedge {}: n1 {} ({}, {})-> n2 {}({}, {})",
                  edgeID,
                  edge.n1,
                  nodes[edge.n1].x,
                  nodes[edge.n1].y,
                  edge.n2,
                  nodes[edge.n2].x,
                  nodes[edge.n2].y);
  if (edge.len > 0) {
    std::string edge_rpt;
    for (int i = 0; i <= edge.route.routelen; i++) {
      int x = tile_size_ * (edge.route.gridsX[i] + 0.5) + x_corner_;
      int y = tile_size_ * (edge.route.gridsY[i] + 0.5) + y_corner_;
      edge_rpt += fmt::format("({} {} {}) ", x, y, edge.route.gridsL[i]);
    }
    logger_->report("\t\t{}", edge_rpt);
  }
}

void FastRouteCore::printTree3D(const int netID)
{
  for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes(); nodeID++) {
    const int x
        = tile_size_ * (sttrees_[netID].nodes[nodeID].x + 0.5) + x_corner_;
    const int y
        = tile_size_ * (sttrees_[netID].nodes[nodeID].y + 0.5) + y_corner_;
    int l = num_layers_;
    if (nodeID < sttrees_[netID].num_terminals) {
      l = nets_[netID]->getPinL()[nodeID];
    }

    logger_->report("nodeID {},  [{}, {}, {}], status: {}",
                    nodeID,
                    x,
                    y,
                    l,
                    sttrees_[netID].nodes[nodeID].status);
  }

  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    printEdge3D(netID, edgeID);
  }
}

void FastRouteCore::checkRoute3D()
{
  for (const int& netID : net_ids_) {
    const auto& treenodes = sttrees_[netID].nodes;
    const int num_terminals = sttrees_[netID].num_terminals;

    for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes(); nodeID++) {
      if (nodeID < num_terminals) {
        if ((treenodes[nodeID].botL > nets_[netID]->getPinL()[nodeID])
            || (treenodes[nodeID].topL < nets_[netID]->getPinL()[nodeID])) {
          logger_->error(GRT, 203, "Caused floating pin node.");
        }
      }
    }
    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      if (sttrees_[netID].edges[edgeID].len == 0) {
        continue;
      }
      TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
      const int edgelength = treeedge->route.routelen;
      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;
      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      const std::vector<short>& gridsL = treeedge->route.gridsL;

      bool gridFlag = false;

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        debugPrint(
            logger_,
            GRT,
            "checkRoute3D",
            1,
            "net {} edge[{}] start node wrong, net num_terminals {}, n1 {}",
            nets_[netID]->getName(),
            edgeID,
            num_terminals,
            n1);
        if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
          printEdge3D(netID, edgeID);
        }
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        debugPrint(
            logger_,
            GRT,
            "checkRoute3D",
            1,
            "net {} edge[{}] end node wrong, net num_terminals {}, n2 {}",
            nets_[netID]->getName(),
            edgeID,
            num_terminals,
            n2);
        if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
          printEdge3D(netID, edgeID);
        }
      }
      for (int i = 0; i < treeedge->route.routelen; i++) {
        const int distance = abs(gridsX[i + 1] - gridsX[i])
                             + abs(gridsY[i + 1] - gridsY[i])
                             + abs(gridsL[i + 1] - gridsL[i]);
        if (distance > 1 || distance < 0) {
          gridFlag = true;
          debugPrint(logger_,
                     GRT,
                     "checkRoute3D",
                     1,
                     "net {} edge[{}] maze route wrong, distance {}, i {}",
                     nets_[netID]->getName(),
                     edgeID,
                     distance,
                     i);
          debugPrint(logger_,
                     GRT,
                     "checkRoute3D",
                     1,
                     "current [{}, {}, {}], next [{}, {}, {}]",
                     gridsL[i],
                     gridsY[i],
                     gridsX[i],
                     gridsL[i + 1],
                     gridsY[i + 1],
                     gridsX[i + 1]);
        }
      }

      for (int i = 0; i <= treeedge->route.routelen; i++) {
        if (gridsL[i] < 0) {
          logger_->error(
              GRT, 204, "Invalid layer value in gridsL, {}.", gridsL[i]);
        }
      }
      if (gridFlag && logger_->debugCheck(GRT, "checkRoute3D", 1)) {
        printEdge3D(netID, edgeID);
      }
    }
  }
}

static bool compareTEL(const OrderTree a, const OrderTree b)
{
  return a.xmin > b.xmin;
}

void FastRouteCore::StNetOrder()
{
  tree_order_cong_.clear();

  tree_order_cong_.resize(net_ids_.size());

  for (int j = 0; j < net_ids_.size(); j++) {
    const int netID = net_ids_[j];

    StTree* stree = &(sttrees_[netID]);
    tree_order_cong_[j].xmin = 0;
    tree_order_cong_[j].treeIndex = netID;

    for (int ind = 0; ind < stree->num_edges(); ind++) {
      const auto& treeedges = stree->edges;
      const TreeEdge* treeedge = &(treeedges[ind]);

      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      for (int i = 0; i < treeedge->route.routelen; i++) {
        if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
          const int min_y = std::min(gridsY[i], gridsY[i + 1]);
          const int cap = getEdgeCapacity(
              nets_[netID], gridsX[i], min_y, EdgeDirection::Vertical);
          tree_order_cong_[j].xmin
              += std::max(0, graph2d_.getUsageV(gridsX[i], min_y) - cap);
        } else {  // a horizontal edge
          const int min_x = std::min(gridsX[i], gridsX[i + 1]);
          const int cap = getEdgeCapacity(
              nets_[netID], min_x, gridsY[i], EdgeDirection::Horizontal);
          tree_order_cong_[j].xmin
              += std::max(0, graph2d_.getUsageH(min_x, gridsY[i]) - cap);
        }
      }
    }
  }

  std::stable_sort(
      tree_order_cong_.begin(), tree_order_cong_.end(), compareTEL);

  // Set the 70% (or less) of non critical nets that doesn't have overflow
  // with the lowest priority
  for (int ord_elID = 0; ord_elID < net_ids_.size(); ord_elID++) {
    auto order_element = tree_order_cong_[ord_elID];
    if (nets_[order_element.treeIndex]->getSlack()
        == std::ceil(std::numeric_limits<float>::lowest())) {
      if (order_element.xmin == 0
          && (ord_elID >= (net_ids_.size() * 30 / 100))) {
        nets_[order_element.treeIndex]->setSlack(
            std::numeric_limits<float>::max());
      }
    }
  }

  auto compareSlack = [this](const OrderTree a, const OrderTree b) {
    const FrNet* net_a = nets_[a.treeIndex];
    const FrNet* net_b = nets_[b.treeIndex];
    return net_a->getSlack() < net_b->getSlack();
  };
  // sort by slack after congestion sort
  std::stable_sort(
      tree_order_cong_.begin(), tree_order_cong_.end(), compareSlack);
}

float FastRouteCore::CalculatePartialSlack()
{
  parasitics_builder_->clearParasitics();
  auto partial_routes = getPlanarRoutes();

  std::vector<float> slacks;
  slacks.reserve(netCount());
  for (auto& net_route : partial_routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    if (!route.empty()) {
      parasitics_builder_->estimateParasitics(db_net, route);
    }
  }
  for (const int& netID : net_ids_) {
    auto fr_net = nets_[netID];
    odb::dbNet* db_net = fr_net->getDbNet();
    float slack = parasitics_builder_->getNetSlack(db_net);
    slacks.push_back(slack);
    fr_net->setSlack(slack);
  }

  std::stable_sort(slacks.begin(), slacks.end());

  // Find the slack threshold based on the percentage of critical nets
  // defined by the user
  const int threshold_index
      = std::ceil(slacks.size() * critical_nets_percentage_ / 100);
  const float slack_th = slacks[threshold_index];

  // Set the non critical nets slack as the lowest float, so they can be
  // ordered by overflow (and ordered first than the critical nets)
  for (const int& netID : net_ids_) {
    if (nets_[netID]->getSlack() > slack_th) {
      nets_[netID]->setSlack(std::ceil(std::numeric_limits<float>::lowest()));
    }
  }

  return slack_th;
}

void FastRouteCore::recoverEdge(const int netID, const int edgeID)
{
  const auto& treeedges = sttrees_[netID].edges;
  const TreeEdge* treeedge = &(treeedges[edgeID]);

  if (treeedge->len == 0) {
    logger_->error(GRT, 206, "Trying to recover a 0-length edge.");
  }

  auto& treenodes = sttrees_[netID].nodes;

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  const std::vector<short>& gridsL = treeedge->route.gridsL;

  const int n1a = treeedge->n1a;
  const int n2a = treeedge->n2a;

  const int connectionCNT1 = treenodes[n1a].conCNT;
  treenodes[n1a].heights[connectionCNT1] = gridsL[0];
  treenodes[n1a].eID[connectionCNT1] = edgeID;
  treenodes[n1a].conCNT++;

  if (gridsL[0] > treenodes[n1a].topL) {
    treenodes[n1a].hID = edgeID;
    treenodes[n1a].topL = gridsL[0];
  }
  if (gridsL[0] < treenodes[n1a].botL) {
    treenodes[n1a].lID = edgeID;
    treenodes[n1a].botL = gridsL[0];
  }

  treenodes[n1a].assigned = true;

  const int routeLen = treeedge->route.routelen;
  const int connectionCNT2 = treenodes[n2a].conCNT;
  treenodes[n2a].heights[connectionCNT2] = gridsL[routeLen];
  treenodes[n2a].eID[connectionCNT2] = edgeID;
  treenodes[n2a].conCNT++;
  if (gridsL[routeLen] > treenodes[n2a].topL) {
    treenodes[n2a].hID = edgeID;
    treenodes[n2a].topL = gridsL[routeLen];
  }
  if (gridsL[routeLen] < treenodes[n2a].botL) {
    treenodes[n2a].lID = edgeID;
    treenodes[n2a].botL = gridsL[routeLen];
  }

  treenodes[n2a].assigned = true;

  FrNet* net = nets_[netID];
  for (int i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        const int ymin = std::min(gridsY[i], gridsY[i + 1]);
        graph2d_.addUsageV(gridsX[i], ymin, net->getEdgeCost());
        v_edges_3D_[gridsL[i]][ymin][gridsX[i]].usage
            += net->getLayerEdgeCost(gridsL[i]);
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        const int xmin = std::min(gridsX[i], gridsX[i + 1]);
        graph2d_.addUsageH(xmin, gridsY[i], net->getEdgeCost());
        h_edges_3D_[gridsL[i]][gridsY[i]][xmin].usage
            += net->getLayerEdgeCost(gridsL[i]);
      }
    }
  }
}

void FastRouteCore::removeLoops()
{
  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;

    const int edgeCost = nets_[netID]->getEdgeCost();

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge edge = sttrees_[netID].edges[edgeID];
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len <= 0) {
        continue;
      }
      std::vector<short>& gridsX = treeedge->route.gridsX;
      std::vector<short>& gridsY = treeedge->route.gridsY;

      for (int i = 1; i <= treeedge->route.routelen; i++) {
        for (int j = 0; j < i; j++) {
          if (gridsX[i] == gridsX[j] && gridsY[i] == gridsY[j]) {
            // Update usage for loop edges to be removed
            for (int k = j; k < i; k++) {
              if (gridsX[k] == gridsX[k + 1]) {
                if (gridsY[k] != gridsY[k + 1]) {
                  const int min_y = std::min(gridsY[k], gridsY[k + 1]);
                  graph2d_.addUsageV(gridsX[k], min_y, -edgeCost);
                }
              } else {
                const int min_x = std::min(gridsX[k], gridsX[k + 1]);
                graph2d_.addUsageH(min_x, gridsY[k], -edgeCost);
              }
            }

            int cnt = 1;
            for (int k = i + 1; k <= treeedge->route.routelen; k++) {
              gridsX[j + cnt] = gridsX[k];
              gridsY[j + cnt] = gridsY[k];
              cnt++;
            }
            treeedge->route.routelen -= i - j;
            i = 0;
            j = 0;
          }
        }
      }
    }
  }
}

void FastRouteCore::verify2DEdgesUsage()
{
  multi_array<int, 2> v_edges(boost::extents[y_grid_ - 1][x_grid_]);
  multi_array<int, 2> h_edges(boost::extents[y_grid_][x_grid_ - 1]);

  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID] == nullptr) {
      continue;
    }
    const auto& treenodes = sttrees_[netID].nodes;
    const auto& treeedges = sttrees_[netID].edges;
    const int edgeCost = nets_[netID]->getEdgeCost();

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len == 0) {
        continue;
      }
      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;

      const auto [ymin, ymax] = std::minmax(y1, y2);

      if (treeedge->route.type == RouteType::LRoute) {
        if (treeedge->route.xFirst) {  // horizontal first
          for (int j = x1; j < x2; j++) {
            h_edges[y1][j] += edgeCost;
          }
          for (int j = ymin; j < ymax; j++) {
            v_edges[j][x2] += edgeCost;
          }
        } else {  // vertical first
          for (int j = ymin; j < ymax; j++) {
            v_edges[j][x1] += edgeCost;
          }
          for (int j = x1; j < x2; j++) {
            h_edges[y2][j] += edgeCost;
          }
        }
      } else if (treeedge->route.type == RouteType::ZRoute) {
        const int Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH)  // HVH
        {
          for (int i = x1; i < Zpoint; i++) {
            h_edges[y1][i] += edgeCost;
          }
          for (int i = Zpoint; i < x2; i++) {
            h_edges[y2][i] += edgeCost;
          }
          for (int i = ymin; i < ymax; i++) {
            v_edges[i][Zpoint] += edgeCost;
          }
        } else {  // VHV
          if (y1 <= y2) {
            for (int i = y1; i < Zpoint; i++) {
              v_edges[i][x1] += edgeCost;
            }
            for (int i = Zpoint; i < y2; i++) {
              v_edges[i][x2] += edgeCost;
            }
            for (int i = x1; i < x2; i++) {
              h_edges[Zpoint][i] += edgeCost;
            }
          } else {
            for (int i = y2; i < Zpoint; i++) {
              v_edges[i][x2] += edgeCost;
            }
            for (int i = Zpoint; i < y1; i++) {
              v_edges[i][x1] += edgeCost;
            }
            for (int i = x1; i < x2; i++) {
              h_edges[Zpoint][i] += edgeCost;
            }
          }
        }
      }
    }
  }
  for (int y = 0; y < y_grid_ - 1; ++y) {
    for (int x = 0; x < x_grid_; ++x) {
      if (v_edges[y][x] != graph2d_.getEstUsageV(x, y)) {
        logger_->error(GRT,
                       247,
                       "v_edge mismatch {} vs {}",
                       v_edges[y][x],
                       graph2d_.getEstUsageV(x, y));
      }
    }
  }
  for (int y = 0; y < y_grid_; ++y) {
    for (int x = 0; x < x_grid_ - 1; ++x) {
      if (h_edges[y][x] != graph2d_.getEstUsageH(x, y)) {
        logger_->error(GRT,
                       248,
                       "h_edge mismatch {} vs {}",
                       h_edges[y][x],
                       graph2d_.getEstUsageH(x, y));
      }
    }
  }
}

void FastRouteCore::verify3DEdgesUsage()
{
  multi_array<std::set<int>, 2> s_v_edges(boost::extents[y_grid_ - 1][x_grid_]);
  multi_array<std::set<int>, 2> s_h_edges(boost::extents[y_grid_][x_grid_ - 1]);

  multi_array<int, 2> v_edges(boost::extents[y_grid_ - 1][x_grid_]);
  multi_array<int, 2> h_edges(boost::extents[y_grid_][x_grid_ - 1]);

  multi_array<int, 3> v_edges_3D(
      boost::extents[num_layers_][y_grid_ - 1][x_grid_]);
  multi_array<int, 3> h_edges_3D(
      boost::extents[num_layers_][y_grid_][x_grid_ - 1]);

  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID] == nullptr) {
      continue;
    }
    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    const int edgeCost = nets_[netID]->getEdgeCost();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      const std::vector<int16_t>& gridsX = treeedge->route.gridsX;
      const std::vector<int16_t>& gridsY = treeedge->route.gridsY;
      const std::vector<int16_t>& gridsL = treeedge->route.gridsL;
      const int routeLen = treeedge->route.routelen;

      for (int i = 0; i < routeLen; i++) {
        if (gridsL[i] != gridsL[i + 1]) {
          continue;
        }
        if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
          const int ymin = std::min(gridsY[i], gridsY[i + 1]);
          s_v_edges[ymin][gridsX[i]].insert(netID);
          v_edges[ymin][gridsX[i]] += edgeCost;
          v_edges_3D[gridsL[i]][ymin][gridsX[i]]
              += nets_[netID]->getLayerEdgeCost(gridsL[i]);
        } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
          const int xmin = std::min(gridsX[i], gridsX[i + 1]);
          s_h_edges[gridsY[i]][xmin].insert(netID);
          h_edges[gridsY[i]][xmin] += edgeCost;
          h_edges_3D[gridsL[i]][gridsY[i]][xmin]
              += nets_[netID]->getLayerEdgeCost(gridsL[i]);
        }
      }
    }
  }

  for (int k = 0; k < num_layers_; k++) {
    for (int y = 0; y < y_grid_ - 1; ++y) {
      for (int x = 0; x < x_grid_; ++x) {
        if (v_edges_3D[k][y][x] != v_edges_3D_[k][y][x].usage) {
          logger_->error(GRT,
                         1247,
                         "v_edge mismatch {} vs {}",
                         v_edges_3D[k][y][x],
                         v_edges_3D_[k][y][x].usage);
        }
      }
    }
  }
  for (int k = 0; k < num_layers_; k++) {
    for (int y = 0; y < y_grid_; ++y) {
      for (int x = 0; x < x_grid_ - 1; ++x) {
        if (h_edges_3D[k][y][x] != h_edges_3D_[k][y][x].usage) {
          logger_->error(GRT,
                         1248,
                         "h_edge mismatch {} vs {}",
                         h_edges_3D[k][y][x],
                         h_edges_3D_[k][y][x].usage);
        }
      }
    }
  }
}

void FastRouteCore::check2DEdgesUsage()
{
  const int max_usage_multiplier = 100;
  int max_h_edge_usage = max_usage_multiplier * h_capacity_;
  int max_v_edge_usage = max_usage_multiplier * v_capacity_;

  // check horizontal edges
  for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
    if (graph2d_.getUsageH(x, y) > max_h_edge_usage) {
      logger_->error(GRT,
                     228,
                     "Horizontal edge usage exceeds the maximum allowed. "
                     "({}, {}) usage={} limit={}",
                     x,
                     y,
                     graph2d_.getUsageH(x, y),
                     max_h_edge_usage);
    }
  }

  // check vertical edges
  for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
    if (graph2d_.getUsageV(x, y) > max_v_edge_usage) {
      logger_->error(GRT,
                     229,
                     "Vertical edge usage exceeds the maximum allowed. "
                     "({}, {}) usage={} limit={}",
                     x,
                     y,
                     graph2d_.getUsageV(x, y),
                     max_v_edge_usage);
    }
  }
}

static bool compareEdgeLen(const OrderNetEdge& a, const OrderNetEdge& b)
{
  return a.length > b.length;
}

void FastRouteCore::netedgeOrderDec(const int netID)
{
  const int numTreeedges = sttrees_[netID].num_edges();

  net_eo_.clear();

  for (int j = 0; j < numTreeedges; j++) {
    OrderNetEdge orderNet;
    orderNet.length = sttrees_[netID].edges[j].route.routelen;
    orderNet.edgeID = j;
    net_eo_.push_back(orderNet);
  }

  std::stable_sort(net_eo_.begin(), net_eo_.end(), compareEdgeLen);
}

void FastRouteCore::printEdge2D(const int netID, const int edgeID)
{
  const TreeEdge edge = sttrees_[netID].edges[edgeID];
  const auto& nodes = sttrees_[netID].nodes;
  const Route& route = edge.route;

  logger_->report("edge {}: n1 {} ({}, {})-> n2 {}({}, {}), routeType {}",
                  edgeID,
                  edge.n1,
                  nodes[edge.n1].x,
                  nodes[edge.n1].y,
                  edge.n2,
                  nodes[edge.n2].x,
                  nodes[edge.n2].y,
                  route.type);
  if (edge.len > 0) {
    std::string edge_rpt;
    for (int i = 0; i <= route.routelen; i++) {
      edge_rpt += fmt::format("({}, {}) ", route.gridsX[i], route.gridsY[i]);
    }
    logger_->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree2D(const int netID)
{
  for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes(); nodeID++) {
    logger_->report("nodeID {},  [{}, {}]",
                    nodeID,
                    sttrees_[netID].nodes[nodeID].y,
                    sttrees_[netID].nodes[nodeID].x);
  }

  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    printEdge2D(netID, edgeID);
  }
}

bool FastRouteCore::checkRoute2DTree(const int netID)
{
  bool STHwrong = false;

  const auto& treenodes = sttrees_[netID].nodes;
  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
    const int edgelength = treeedge->route.routelen;
    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    const int x1 = treenodes[n1].x;
    const int y1 = treenodes[n1].y;
    const int x2 = treenodes[n2].x;
    const int y2 = treenodes[n2].y;
    const std::vector<short>& gridsX = treeedge->route.gridsX;
    const std::vector<short>& gridsY = treeedge->route.gridsY;

    if (treeedge->len < 0) {
      if (verbose_) {
        logger_->warn(
            GRT, 207, "Ripped up edge without edge length reassignment.");
      }
      STHwrong = true;
    }

    if (treeedge->len > 0) {
      if (treeedge->route.routelen < 1) {
        if (verbose_) {
          logger_->warn(GRT,
                        208,
                        "Route length {}, tree length {}.",
                        treeedge->route.routelen,
                        treeedge->len);
        }
        STHwrong = true;
        return true;
      }

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        if (verbose_) {
          logger_->warn(
              GRT,
              164,
              "Initial grid wrong y1 x1 [{} {}], net start [{} {}] routelen "
              "{}.",
              y1,
              x1,
              gridsY[0],
              gridsX[0],
              treeedge->route.routelen);
        }
        STHwrong = true;
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        if (verbose_) {
          logger_->warn(
              GRT,
              165,
              "End grid wrong y2 x2 [{} {}], net start [{} {}] routelen {}.",
              y1,
              x1,
              gridsY[edgelength],
              gridsX[edgelength],
              treeedge->route.routelen);
        }
        STHwrong = true;
      }
      for (int i = 0; i < treeedge->route.routelen; i++) {
        const int distance
            = abs(gridsX[i + 1] - gridsX[i]) + abs(gridsY[i + 1] - gridsY[i]);
        if (distance != 1) {
          if (verbose_) {
            logger_->warn(
                GRT,
                166,
                "Net {} edge[{}] maze route wrong, distance {}, i {}.",
                nets_[netID]->getName(),
                edgeID,
                distance,
                i);
          }
          STHwrong = true;
        }
      }

      if (STHwrong) {
        if (verbose_) {
          logger_->warn(
              GRT, 167, "Invalid 2D tree for net {}.", nets_[netID]->getName());
        }
        return true;
      }
    }
  }

  return (STHwrong);
}

// Copy Routing Solution for the best routing solution so far
void FastRouteCore::copyRS()
{
  if (!sttrees_bk_.empty()) {
    for (const int& netID : net_ids_) {
      const int numEdges = sttrees_bk_[netID].num_edges();
      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }
    sttrees_bk_.clear();
  }

  sttrees_bk_.resize(netCount());

  for (const int& netID : net_ids_) {
    const int numNodes = sttrees_[netID].num_nodes();
    const int numEdges = sttrees_[netID].num_edges();

    sttrees_bk_[netID].nodes.resize(numNodes);

    for (int i = 0; i < numNodes; i++) {
      sttrees_bk_[netID].nodes[i].x = sttrees_[netID].nodes[i].x;
      sttrees_bk_[netID].nodes[i].y = sttrees_[netID].nodes[i].y;
      for (int j = 0; j < 3; j++) {
        sttrees_bk_[netID].nodes[i].nbr[j] = sttrees_[netID].nodes[i].nbr[j];
        sttrees_bk_[netID].nodes[i].edge[j] = sttrees_[netID].nodes[i].edge[j];
      }
    }
    sttrees_bk_[netID].num_terminals = sttrees_[netID].num_terminals;

    sttrees_bk_[netID].edges.resize(numEdges);

    for (int edgeID = 0; edgeID < numEdges; edgeID++) {
      sttrees_bk_[netID].edges[edgeID].len = sttrees_[netID].edges[edgeID].len;
      sttrees_bk_[netID].edges[edgeID].n1 = sttrees_[netID].edges[edgeID].n1;
      sttrees_bk_[netID].edges[edgeID].n2 = sttrees_[netID].edges[edgeID].n2;

      sttrees_bk_[netID].edges[edgeID].route.routelen
          = sttrees_[netID].edges[edgeID].route.routelen;

      if (sttrees_[netID].edges[edgeID].len
          > 0)  // only route the non-degraded edges (len>0)
      {
        sttrees_bk_[netID].edges[edgeID].route.gridsX.resize(
            sttrees_[netID].edges[edgeID].route.routelen + 1, 0);
        sttrees_bk_[netID].edges[edgeID].route.gridsY.resize(
            sttrees_[netID].edges[edgeID].route.routelen + 1, 0);

        for (int i = 0; i <= sttrees_[netID].edges[edgeID].route.routelen;
             i++) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX[i]
              = sttrees_[netID].edges[edgeID].route.gridsX[i];
          sttrees_bk_[netID].edges[edgeID].route.gridsY[i]
              = sttrees_[netID].edges[edgeID].route.gridsY[i];
        }
      }
    }
  }
}

void FastRouteCore::copyBR()
{
  if (!sttrees_bk_.empty()) {
    // Reduce usage with last routes before update
    for (const int& netID : net_ids_) {
      const int numEdges = sttrees_[netID].num_edges();
      const int edgeCost = nets_[netID]->getEdgeCost();

      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        const TreeEdge& edge = sttrees_[netID].edges[edgeID];
        if (edge.len > 0) {
          const std::vector<int16_t>& gridsX = edge.route.gridsX;
          const std::vector<int16_t>& gridsY = edge.route.gridsY;
          for (int i = 0; i < edge.route.routelen; i++) {
            if (gridsX[i] == gridsX[i + 1] && gridsY[i] == gridsY[i + 1]) {
              continue;
            }
            if (gridsX[i] == gridsX[i + 1]) {
              const int min_y = std::min(gridsY[i], gridsY[i + 1]);
              graph2d_.addUsageV(gridsX[i], min_y, -edgeCost);
            } else {
              const int min_x = std::min(gridsX[i], gridsX[i + 1]);
              graph2d_.addUsageH(min_x, gridsY[i], -edgeCost);
            }
          }
        }
      }
    }

    // Clean routes
    for (const int& netID : net_ids_) {
      const int numEdges = sttrees_[netID].num_edges();
      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_[netID].edges[edgeID].len > 0) {
          sttrees_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }

    // Copy saved routes
    for (const int& netID : net_ids_) {
      const int numNodes = sttrees_bk_[netID].num_nodes();
      const int numEdges = sttrees_bk_[netID].num_edges();

      sttrees_[netID].nodes.resize(numNodes);

      for (int i = 0; i < numNodes; i++) {
        sttrees_[netID].nodes[i].x = sttrees_bk_[netID].nodes[i].x;
        sttrees_[netID].nodes[i].y = sttrees_bk_[netID].nodes[i].y;
        for (int j = 0; j < 3; j++) {
          sttrees_[netID].nodes[i].nbr[j] = sttrees_bk_[netID].nodes[i].nbr[j];
          sttrees_[netID].nodes[i].edge[j]
              = sttrees_bk_[netID].nodes[i].edge[j];
        }
      }

      sttrees_[netID].edges.resize(numEdges);

      sttrees_[netID].num_terminals = sttrees_bk_[netID].num_terminals;

      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        sttrees_[netID].edges[edgeID].len
            = sttrees_bk_[netID].edges[edgeID].len;
        sttrees_[netID].edges[edgeID].n1 = sttrees_bk_[netID].edges[edgeID].n1;
        sttrees_[netID].edges[edgeID].n2 = sttrees_bk_[netID].edges[edgeID].n2;

        sttrees_[netID].edges[edgeID].route.type = RouteType::MazeRoute;
        sttrees_[netID].edges[edgeID].route.routelen
            = sttrees_bk_[netID].edges[edgeID].route.routelen;

        if (sttrees_bk_[netID].edges[edgeID].len
            > 0)  // only route the non-degraded edges (len>0)
        {
          sttrees_[netID].edges[edgeID].route.gridsX.resize(
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1, 0);
          sttrees_[netID].edges[edgeID].route.gridsY.resize(
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1, 0);

          for (int i = 0; i <= sttrees_bk_[netID].edges[edgeID].route.routelen;
               i++) {
            sttrees_[netID].edges[edgeID].route.gridsX[i]
                = sttrees_bk_[netID].edges[edgeID].route.gridsX[i];
            sttrees_[netID].edges[edgeID].route.gridsY[i]
                = sttrees_bk_[netID].edges[edgeID].route.gridsY[i];
          }
        }
      }
    }

    // Increase usage with new routes
    for (const int& netID : net_ids_) {
      const int numEdges = sttrees_[netID].num_edges();
      const int edgeCost = nets_[netID]->getEdgeCost();

      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        const TreeEdge& edge = sttrees_[netID].edges[edgeID];
        if (edge.len > 0) {
          const std::vector<int16_t>& gridsX = edge.route.gridsX;
          const std::vector<int16_t>& gridsY = edge.route.gridsY;
          for (int i = 0; i < edge.route.routelen; i++) {
            if (gridsX[i] == gridsX[i + 1] && gridsY[i] == gridsY[i + 1]) {
              continue;
            }
            if (gridsX[i] == gridsX[i + 1]) {
              const int min_y = std::min(gridsY[i], gridsY[i + 1]);
              graph2d_.addUsageV(gridsX[i], min_y, edgeCost);
            } else {
              const int min_x = std::min(gridsX[i], gridsX[i + 1]);
              graph2d_.addUsageH(min_x, gridsY[i], edgeCost);
            }
          }
        }
      }
    }
  }
}

void FastRouteCore::freeRR()
{
  if (!sttrees_bk_.empty()) {
    for (const int& netID : net_ids_) {
      const int numEdges = sttrees_bk_[netID].num_edges();
      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }
    sttrees_bk_.clear();
  }
}

int FastRouteCore::edgeShift(Tree& t, const int net)
{
  // TODO: check this size
  const int sizeV = 2 * nets_[net]->getNumPins();
  multi_array<int, 2> nbr;
  nbr.resize(boost::extents[sizeV][3]);
  std::vector<int> nbrCnt(sizeV);
  std::vector<int> pairN1(nets_[net]->getNumPins());
  std::vector<int> pairN2(nets_[net]->getNumPins());
  std::vector<int> costH(y_grid_);
  std::vector<int> costV(x_grid_);

  const int deg = t.deg;
  // find root of the tree
  int root = 0;
  for (int i = deg; i < t.branchCount(); i++) {
    if (t.branch[i].n == i) {
      root = i;
      break;
    }
  }

  // find all neighbors for steiner nodes
  for (int i = deg; i < t.branchCount(); i++) {
    nbrCnt[i] = 0;
  }
  // edges from pin to steiner
  for (int i = 0; i < deg; i++) {
    const int n = t.branch[i].n;
    if (n >= deg && n < t.branchCount()) {  // ensure n is inside nbrCnt range
      nbr[n][nbrCnt[n]] = i;
      nbrCnt[n]++;
    } else {
      logger_->error(GRT, 149, "Invalid access to nbrCnt vector");
    }
  }
  // edges from steiner to steiner
  for (int i = deg; i < t.branchCount(); i++) {
    if (i != root)  // not the removed steiner nodes and root
    {
      const int n = t.branch[i].n;
      nbr[i][nbrCnt[i]] = n;
      nbrCnt[i]++;
      nbr[n][nbrCnt[n]] = i;
      nbrCnt[n]++;
    }
  }

  int numShift = 0;
  int bestBenefit = BIG_INT;  // used to enter while loop
  while (bestBenefit > 0) {
    // find all H or V edges (steiner pairs)
    int pairCnt = 0;
    for (int i = deg; i < t.branchCount(); i++) {
      const int n = t.branch[i].n;
      if (t.branch[i].x == t.branch[n].x) {
        if (t.branch[i].y < t.branch[n].y) {
          pairN1[pairCnt] = i;
          pairN2[pairCnt] = n;
          pairCnt++;
        } else if (t.branch[i].y > t.branch[n].y) {
          pairN1[pairCnt] = n;
          pairN2[pairCnt] = i;
          pairCnt++;
        }
      } else if (t.branch[i].y == t.branch[n].y) {
        if (t.branch[i].x < t.branch[n].x) {
          pairN1[pairCnt] = i;
          pairN2[pairCnt] = n;
          pairCnt++;
        } else if (t.branch[i].x > t.branch[n].x) {
          pairN1[pairCnt] = n;
          pairN2[pairCnt] = i;
          pairCnt++;
        }
      }
    }

    int bestPair = -1;
    bestBenefit = -1;
    int bestPos;
    // for each H or V edge, find the best benefit by shifting it
    for (int i = 0; i < pairCnt; i++) {
      // find the range of shifting for this pair
      const int n1 = pairN1[i];
      const int n2 = pairN2[i];
      if (t.branch[n1].y == t.branch[n2].y)  // a horizontal edge
      {
        // find the shifting range for the edge (minY~maxY)
        int minY1 = t.branch[n1].y;
        int maxY1 = minY1;
        for (int j = 0; j < 3; j++) {
          const int y = t.branch[nbr[n1][j]].y;
          if (y > maxY1) {
            maxY1 = y;
          } else if (y < minY1) {
            minY1 = y;
          }
        }
        int minY2 = t.branch[n2].y;
        int maxY2 = minY2;
        for (int j = 0; j < 3; j++) {
          const int y = t.branch[nbr[n2][j]].y;
          if (y > maxY2) {
            maxY2 = y;
          } else if (y < minY2) {
            minY2 = y;
          }
        }
        const int minY = std::max(minY1, minY2);
        const int maxY = std::min(maxY1, maxY2);

        // find the best position (least total usage) to shift
        if (minY < maxY)  // more than 1 possible positions
        {
          for (int j = minY; j <= maxY; j++) {
            costH[j] = 0;
            for (int k = t.branch[n1].x; k < t.branch[n2].x; k++) {
              costH[j] += graph2d_.getEstUsageH(k, j);
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (int l = 0; l < nbrCnt[n1]; l++) {
              const int n3 = nbr[n1][l];
              if (n3 != n2) {  // exclude current edge n1-n2
                int bigX;
                int smallX;
                if (t.branch[n1].x < t.branch[n3].x) {
                  smallX = t.branch[n1].x;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = t.branch[n1].x;
                }
                int bigY;
                int smallY;
                if (j < t.branch[n3].y) {
                  smallY = j;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = j;
                }
                int cost1 = 0;
                int cost2 = 0;
                for (int m = smallX; m < bigX; m++) {
                  cost1 += graph2d_.getEstUsageH(m, smallY);
                  cost2 += graph2d_.getEstUsageH(m, bigY);
                }
                for (int m = smallY; m < bigY; m++) {
                  cost1 += graph2d_.getEstUsageV(bigX, m);
                  cost2 += graph2d_.getEstUsageV(smallX, m);
                }
                costH[j] += std::min(cost1, cost2);
              }
            }
            for (int l = 0; l < nbrCnt[n2]; l++) {
              const int n3 = nbr[n2][l];
              if (n3 != n1) {  // exclude current edge n1-n2
                int bigX;
                int smallX;
                if (t.branch[n2].x < t.branch[n3].x) {
                  smallX = t.branch[n2].x;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = t.branch[n2].x;
                }
                int bigY;
                int smallY;
                if (j < t.branch[n3].y) {
                  smallY = j;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = j;
                }
                int cost1 = 0;
                int cost2 = 0;
                for (int m = smallX; m < bigX; m++) {
                  cost1 += graph2d_.getEstUsageH(m, smallY);
                  cost2 += graph2d_.getEstUsageH(m, bigY);
                }
                for (int m = smallY; m < bigY; m++) {
                  cost1 += graph2d_.getEstUsageV(bigX, m);
                  cost2 += graph2d_.getEstUsageV(smallX, m);
                }
                costH[j] += std::min(cost1, cost2);
              }
            }
          }
          int bestCost = BIG_INT;
          int Pos = t.branch[n1].y;
          for (int j = minY; j <= maxY; j++) {
            if (costH[j] < bestCost) {
              bestCost = costH[j];
              Pos = j;
            }
          }
          if (Pos != t.branch[n1].y) {  // find a better position than current
            const int benefit = costH[t.branch[n1].y] - bestCost;
            if (benefit > bestBenefit) {
              bestBenefit = benefit;
              bestPair = i;
              bestPos = Pos;
            }
          }
        }

      } else {  // a vertical edge
        // find the shifting range for the edge (minX~maxX)
        int minX1 = t.branch[n1].x;
        int maxX1 = minX1;
        for (int j = 0; j < 3; j++) {
          const int x = t.branch[nbr[n1][j]].x;
          if (x > maxX1) {
            maxX1 = x;
          } else if (x < minX1) {
            minX1 = x;
          }
        }
        int minX2 = t.branch[n2].x;
        int maxX2 = minX2;
        for (int j = 0; j < 3; j++) {
          const int x = t.branch[nbr[n2][j]].x;
          if (x > maxX2) {
            maxX2 = x;
          } else if (x < minX2) {
            minX2 = x;
          }
        }
        const int minX = std::max(minX1, minX2);
        const int maxX = std::min(maxX1, maxX2);

        // find the best position (least total usage) to shift
        if (minX < maxX) {  // more than 1 possible positions
          for (int j = minX; j <= maxX; j++) {
            costV[j] = 0;
            for (int k = t.branch[n1].y; k < t.branch[n2].y; k++) {
              costV[j] += graph2d_.getEstUsageV(j, k);
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (int l = 0; l < nbrCnt[n1]; l++) {
              const int n3 = nbr[n1][l];
              if (n3 != n2)  // exclude current edge n1-n2
              {
                int bigX;
                int smallX;
                if (j < t.branch[n3].x) {
                  smallX = j;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = j;
                }
                int bigY;
                int smallY;
                if (t.branch[n1].y < t.branch[n3].y) {
                  smallY = t.branch[n1].y;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = t.branch[n1].y;
                }
                int cost1 = 0;
                int cost2 = 0;
                for (int m = smallX; m < bigX; m++) {
                  cost1 += graph2d_.getEstUsageH(m, smallY);
                  cost2 += graph2d_.getEstUsageH(m, bigY);
                }
                for (int m = smallY; m < bigY; m++) {
                  cost1 += graph2d_.getEstUsageV(bigX, m);
                  cost2 += graph2d_.getEstUsageV(smallX, m);
                }
                costV[j] += std::min(cost1, cost2);
              }
            }
            for (int l = 0; l < nbrCnt[n2]; l++) {
              const int n3 = nbr[n2][l];
              if (n3 != n1) {  // exclude current edge n1-n2
                int bigX;
                int smallX;
                if (j < t.branch[n3].x) {
                  smallX = j;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = j;
                }
                int bigY;
                int smallY;
                if (t.branch[n2].y < t.branch[n3].y) {
                  smallY = t.branch[n2].y;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = t.branch[n2].y;
                }
                int cost1 = 0;
                int cost2 = 0;
                for (int m = smallX; m < bigX; m++) {
                  cost1 += graph2d_.getEstUsageH(m, smallY);
                  cost2 += graph2d_.getEstUsageH(m, bigY);
                }
                for (int m = smallY; m < bigY; m++) {
                  cost1 += graph2d_.getEstUsageV(bigX, m);
                  cost2 += graph2d_.getEstUsageV(smallX, m);
                }
                costV[j] += std::min(cost1, cost2);
              }
            }
          }
          int bestCost = BIG_INT;
          int Pos = t.branch[n1].x;
          for (int j = minX; j <= maxX; j++) {
            if (costV[j] < bestCost) {
              bestCost = costV[j];
              Pos = j;
            }
          }
          if (Pos != t.branch[n1].x) {  // find a better position than current
            const int benefit = costV[t.branch[n1].x] - bestCost;
            if (benefit > bestBenefit) {
              bestBenefit = benefit;
              bestPair = i;
              bestPos = Pos;
            }
          }
        }
      }
    }

    if (bestBenefit > 0) {
      const int n1 = pairN1[bestPair];
      const int n2 = pairN2[bestPair];

      if (t.branch[n1].y == t.branch[n2].y) {  // horizontal edge
        t.branch[n1].y = bestPos;
        t.branch[n2].y = bestPos;
      } else {  // vertical edge
        t.branch[n1].x = bestPos;
        t.branch[n2].x = bestPos;
      }
      numShift++;
    }
  }

  return (numShift);
}

// exchange Steiner nodes at the same position, then call edgeShift()
int FastRouteCore::edgeShiftNew(Tree& t, const int net)
{
  int numShift = edgeShift(t, net);
  const int deg = t.deg;

  const int sizeV = nets_[net]->getNumPins();
  std::vector<int> pairN1(sizeV);
  std::vector<int> pairN2(sizeV);

  int iter = 0;
  int cur_pairN1 = -1;
  int cur_pairN2 = -1;
  while (iter < 3) {
    iter++;

    // find all pairs of steiner node at the same position (steiner pairs)
    int pairCnt = 0;
    for (int i = deg; i < t.branchCount(); i++) {
      const int n = t.branch[i].n;
      if (n != i && n != t.branch[n].n && t.branch[i].x == t.branch[n].x
          && t.branch[i].y == t.branch[n].y) {
        pairN1[pairCnt] = i;
        pairN2[pairCnt] = n;
        pairCnt++;
      }
    }

    if (pairCnt > 0) {
      bool isPair = false;
      if (pairN1[0] != cur_pairN1
          || pairN2[0] != cur_pairN2)  // don't try the same as last one
      {
        cur_pairN1 = pairN1[0];
        cur_pairN2 = pairN2[0];
        isPair = true;
      } else if (pairN1[0] == cur_pairN1 && pairN2[0] == cur_pairN2
                 && pairCnt > 1) {
        cur_pairN1 = pairN1[1];
        cur_pairN2 = pairN2[1];
        isPair = true;
      }

      if (isPair) {  // find a new pair to swap
        int N1nbrH = -1;
        int N1nbrV = -1;
        int N2nbrH = -1;
        int N2nbrV = -1;
        // find the nodes directed to cur_pairN1(2 nodes) and cur_pairN2(1
        // nodes)
        for (int j = 0; j < t.branchCount(); j++) {
          const int n = t.branch[j].n;
          if (n == cur_pairN1) {
            if (t.branch[j].x == t.branch[cur_pairN1].x
                && t.branch[j].y != t.branch[cur_pairN1].y) {
              N1nbrV = j;
            } else if (t.branch[j].y == t.branch[cur_pairN1].y
                       && t.branch[j].x != t.branch[cur_pairN1].x) {
              N1nbrH = j;
            }
          } else if (n == cur_pairN2) {
            if (t.branch[j].x == t.branch[cur_pairN2].x
                && t.branch[j].y != t.branch[cur_pairN2].y) {
              N2nbrV = j;
            } else if (t.branch[j].y == t.branch[cur_pairN2].y
                       && t.branch[j].x != t.branch[cur_pairN2].x) {
              N2nbrH = j;
            }
          }
        }
        // find the node cur_pairN2 directed to
        const int n = t.branch[cur_pairN2].n;
        if (t.branch[n].x == t.branch[cur_pairN2].x
            && t.branch[n].y != t.branch[cur_pairN2].y) {
          N2nbrV = n;
        } else if (t.branch[n].y == t.branch[cur_pairN2].y
                   && t.branch[n].x != t.branch[cur_pairN2].x) {
          N2nbrH = n;
        }

        if (N1nbrH >= 0 && N2nbrH >= 0) {
          if (N2nbrH == t.branch[cur_pairN2].n) {
            t.branch[N1nbrH].n = cur_pairN2;
            t.branch[cur_pairN1].n = N2nbrH;
            t.branch[cur_pairN2].n = cur_pairN1;
          } else {
            t.branch[N1nbrH].n = cur_pairN2;
            t.branch[N2nbrH].n = cur_pairN1;
          }
          numShift += edgeShift(t, net);
        } else if (N1nbrV >= 0 && N2nbrV >= 0) {
          if (N2nbrV == t.branch[cur_pairN2].n) {
            t.branch[N1nbrV].n = cur_pairN2;
            t.branch[cur_pairN1].n = N2nbrV;
            t.branch[cur_pairN2].n = cur_pairN1;
          } else {
            t.branch[N1nbrV].n = cur_pairN2;
            t.branch[N2nbrV].n = cur_pairN1;
          }
          numShift += edgeShift(t, net);
        }
      }
    } else {
      iter = 3;
    }
  }

  return numShift;
}

odb::Rect FastRouteCore::globalRoutingToBox(const GSegment& route)
{
  const auto [init_x, final_x] = std::minmax(route.init_x, route.final_x);
  const auto [init_y, final_y] = std::minmax(route.init_y, route.final_y);

  const int llX = init_x - (tile_size_ / 2);
  const int llY = init_y - (tile_size_ / 2);

  int urX = final_x + (tile_size_ / 2);
  int urY = final_y + (tile_size_ / 2);

  if ((x_grid_max_ - urX) / tile_size_ < 1) {
    urX = x_grid_max_;
  }
  if ((y_grid_max_ - urY) / tile_size_ < 1) {
    urY = y_grid_max_;
  }

  return {llX, llY, urX, urY};
}

double FastRouteCore::dbuToMicrons(const int dbu)
{
  return db_->getChip()->getBlock()->dbuToMicrons(dbu);
}

void FastRouteCore::saveCongestion(const int iter)
{
  std::vector<CongestionInformation> congestionGridsV, congestionGridsH;
  if (graph2d_.hasEdges()) {
    getCongestionGrid(congestionGridsV, congestionGridsH);

    std::mt19937 g;
    const int seed = 42;
    g.seed(seed);

    utl::shuffle(congestionGridsH.begin(), congestionGridsH.end(), g);
    utl::shuffle(congestionGridsV.begin(), congestionGridsV.end(), g);
  }

  const std::string marker_group_name = fmt::format(
      "Global route{}", iter == -1 ? "" : fmt::format(" - iter {}", iter));

  if (!congestionGridsV.empty() || !congestionGridsH.empty()) {
    odb::dbMarkerCategory* tool_category
        = odb::dbMarkerCategory::createOrReplace(db_->getChip()->getBlock(),
                                                 marker_group_name.c_str());
    tool_category->setSource("GRT");

    if (!congestionGridsH.empty()) {
      odb::dbMarkerCategory* category = odb::dbMarkerCategory::create(
          tool_category, "Horizontal congestion");

      for (const auto& [seg, tile, srcs] : congestionGridsH) {
        odb::dbMarker* marker = odb::dbMarker::create(category);
        if (marker == nullptr) {
          continue;
        }

        marker->addShape(globalRoutingToBox(seg));

        const int capacity = tile.capacity;
        const int usage = tile.usage;
        marker->setComment(fmt::format("capacity:{} usage:{} overflow:{}",
                                       capacity,
                                       usage,
                                       usage - capacity));
        for (const auto& net : srcs) {
          marker->addSource(net);
        }
      }
    }

    if (!congestionGridsV.empty()) {
      odb::dbMarkerCategory* category
          = odb::dbMarkerCategory::create(tool_category, "Vertical congestion");

      for (const auto& [seg, tile, srcs] : congestionGridsV) {
        odb::dbMarker* marker = odb::dbMarker::create(category);
        if (marker == nullptr) {
          continue;
        }

        marker->addShape(globalRoutingToBox(seg));

        const int capacity = tile.capacity;
        const int usage = tile.usage;
        marker->setComment(fmt::format("capacity:{} usage:{} overflow:{}",
                                       capacity,
                                       usage,
                                       usage - capacity));
        for (const auto& net : srcs) {
          marker->addSource(net);
        }
      }
    }
  }

  // check if the file name is defined
  if (congestion_file_name_.empty()) {
    return;
  }

  // Modify the file name for each iteration
  std::string file_name = congestion_file_name_;
  if (iter != -1) {
    // delete rpt extension
    file_name = file_name.substr(0, file_name.size() - 4);
    // add iteration number
    file_name += "-" + std::to_string(iter) + ".rpt";
  }

  odb::dbMarkerCategory* tool_category
      = db_->getChip()->getBlock()->findMarkerCategory(
          marker_group_name.c_str());
  if (tool_category != nullptr) {
    tool_category->writeTR(file_name);
  }
}

int FastRouteCore::splitEdge(std::vector<TreeEdge>& treeedges,
                             std::vector<TreeNode>& treenodes,
                             const int n1,
                             const int n2,
                             const int edge_n1n2)
{
  const int n2x = treenodes[n2].x;
  const int n2y = treenodes[n2].y;

  // create new node
  const int new_node_id = treenodes.size();
  TreeNode new_node;
  new_node.x = n2x;
  new_node.y = n2y;
  new_node.stackAlias = treenodes[n2].stackAlias;

  // create new edge
  const int new_edge_id = treeedges.size();
  TreeEdge new_edge;

  // find one neighbor node id different to n1
  int nbr;
  int edge_n2_nbr;  // edge id that connects the neighbor
  if (treenodes[n2].nbr[0] == n1) {
    nbr = treenodes[n2].nbr[1];
    edge_n2_nbr = treenodes[n2].edge[1];
  } else {
    nbr = treenodes[n2].nbr[0];
    edge_n2_nbr = treenodes[n2].edge[0];
  }

  // update n2 neighbor
  int cnt = 0;
  for (int i = 0; i < treenodes[n2].nbr_count; i++) {
    if (treenodes[n2].nbr[i] == n1) {
      continue;
    }
    if (treenodes[n2].nbr[i] == nbr) {
      treenodes[n2].nbr[cnt] = new_node_id;
      treenodes[n2].edge[cnt] = new_edge_id;
      cnt++;
    } else {
      treenodes[n2].nbr[cnt] = treenodes[n2].nbr[i];
      treenodes[n2].edge[cnt] = treenodes[n2].edge[i];
      cnt++;
    }
  }
  treenodes[n2].nbr_count = cnt;

  // change edge neighbor
  if (treeedges[edge_n2_nbr].n1 == n2) {
    treeedges[edge_n2_nbr].n1 = new_node_id;
    treeedges[edge_n2_nbr].n1a = new_node.stackAlias;
  } else {
    treeedges[edge_n2_nbr].n2 = new_node_id;
    treeedges[edge_n2_nbr].n2a = new_node.stackAlias;
  }

  // change current edge
  if (treeedges[edge_n1n2].n1 == n2) {
    treeedges[edge_n1n2].n1 = new_node_id;
    treeedges[edge_n1n2].n1a = new_node.stackAlias;
  } else {
    treeedges[edge_n1n2].n2 = new_node_id;
    treeedges[edge_n1n2].n2a = new_node.stackAlias;
  }

  // change node neighbor
  for (int i = 0; i < treenodes[nbr].nbr_count; i++) {
    if (treenodes[nbr].nbr[i] == n2) {
      treenodes[nbr].nbr[i] = new_node_id;
    }
  }

  // change n1 node
  for (int i = 0; i < treenodes[n1].nbr_count; i++) {
    if (treenodes[n1].nbr[i] == n2) {
      treenodes[n1].nbr[i] = new_node_id;
    }
  }

  // config new edge
  new_edge.assigned = false;
  new_edge.len = 0;
  new_edge.n1 = new_node_id;
  new_edge.n1a = new_node.stackAlias;
  new_edge.n2 = n2;
  new_edge.n2a = treenodes[n2].stackAlias;
  new_edge.route.type = RouteType::MazeRoute;
  new_edge.route.routelen = 0;
  new_edge.route.gridsX.push_back(n2x);
  new_edge.route.gridsY.push_back(n2y);

  // config new node
  new_node.assigned = false;
  new_node.nbr_count = 3;
  new_node.nbr[0] = nbr;
  new_node.nbr[1] = n2;
  new_node.nbr[2] = n1;
  new_node.edge[0] = edge_n2_nbr;
  new_node.edge[1] = new_edge_id;
  new_node.edge[2] = edge_n1n2;

  treeedges.push_back(new_edge);
  treenodes.push_back(new_node);

  return new_node_id;
}

void FastRouteCore::setTreeNodesVariables(const int netID)
{
  // Number of nodes without redundancy in their x and y positions
  int numpoints = 0;

  const int num_terminals = sttrees_[netID].num_terminals;
  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;

  int routeLen;
  TreeEdge* treeedge;
  // Setting the values needed for each TreeNode
  for (int d = 0; d < sttrees_[netID].num_nodes(); d++) {
    treenodes[d].topL = -1;
    treenodes[d].botL = num_layers_;
    treenodes[d].assigned = false;
    treenodes[d].stackAlias = d;
    treenodes[d].conCNT = 0;
    treenodes[d].hID = BIG_INT;
    treenodes[d].lID = BIG_INT;
    treenodes[d].status = 0;

    if (d < num_terminals) {
      treenodes[d].botL = nets_[netID]->getPinL()[d];
      treenodes[d].topL = nets_[netID]->getPinL()[d];
      treenodes[d].assigned = true;
      treenodes[d].status = 1;

      xcor_[numpoints] = treenodes[d].x;
      ycor_[numpoints] = treenodes[d].y;
      dcor_[numpoints] = d;
      numpoints++;
    } else {
      bool redundant = false;
      for (int k = 0; k < numpoints; k++) {
        if ((treenodes[d].x == xcor_[k]) && (treenodes[d].y == ycor_[k])) {
          treenodes[d].stackAlias = dcor_[k];
          redundant = true;
          break;
        }
      }
      if (!redundant) {
        xcor_[numpoints] = treenodes[d].x;
        ycor_[numpoints] = treenodes[d].y;
        dcor_[numpoints] = d;
        numpoints++;
      }
    }
  }  // loop nodes
  // Setting the values needed for TreeNodes and TreeEdges
  for (int k = 0; k < sttrees_[netID].num_edges(); k++) {
    treeedge = &(treeedges[k]);

    if (treeedge->len <= 0) {
      continue;
    }
    routeLen = treeedge->route.routelen;

    int n1 = treeedge->n1;
    int n2 = treeedge->n2;
    const std::vector<int16_t>& gridsLtmp = treeedge->route.gridsL;

    int n1a = treenodes[n1].stackAlias;

    int n2a = treenodes[n2].stackAlias;

    treeedge->n1a = n1a;
    treeedge->n2a = n2a;

    int connectionCNT = treenodes[n1a].conCNT;
    treenodes[n1a].heights[connectionCNT] = gridsLtmp[0];
    treenodes[n1a].eID[connectionCNT] = k;
    treenodes[n1a].conCNT++;

    if (gridsLtmp[0] > treenodes[n1a].topL) {
      treenodes[n1a].hID = k;
      treenodes[n1a].topL = gridsLtmp[0];
    }
    if (gridsLtmp[0] < treenodes[n1a].botL) {
      treenodes[n1a].lID = k;
      treenodes[n1a].botL = gridsLtmp[0];
    }

    treenodes[n1a].assigned = true;

    connectionCNT = treenodes[n2a].conCNT;
    treenodes[n2a].heights[connectionCNT] = gridsLtmp[routeLen];
    treenodes[n2a].eID[connectionCNT] = k;
    treenodes[n2a].conCNT++;
    if (gridsLtmp[routeLen] > treenodes[n2a].topL) {
      treenodes[n2a].hID = k;
      treenodes[n2a].topL = gridsLtmp[routeLen];
    }
    if (gridsLtmp[routeLen] < treenodes[n2a].botL) {
      treenodes[n2a].lID = k;
      treenodes[n2a].botL = gridsLtmp[routeLen];
    }

    treenodes[n2a].assigned = true;
  }  // loop edges
}

std::ostream& operator<<(std::ostream& os, const RouteType& type)
{
  switch (type) {
    case RouteType::NoRoute:
      return os << "NoRoute";
    case RouteType::LRoute:
      return os << "LRoute";
    case RouteType::ZRoute:
      return os << "ZRoute";
    case RouteType::MazeRoute:
      return os << "MazeRoute";
  };
  return os << "Bad RouteType";
}

}  // namespace grt
