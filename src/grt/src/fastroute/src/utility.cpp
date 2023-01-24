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

#include <algorithm>
#include <queue>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

void FastRouteCore::printEdge(int const netID, int const edgeID)
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
    routes_rpt = routes_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", "
                 + std::to_string(edge.route.gridsY[i]) + ") ";
  }
  logger_->report("{}", routes_rpt);
}

void FastRouteCore::ConvertToFull3DType2()
{
  short tmpX[MAXLEN], tmpY[MAXLEN], tmpL[MAXLEN];

  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int newCNT = 0;
        const int routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;
        // finish from n1->real route
        int j;
        for (j = 0; j < routeLen; j++) {
          tmpX[newCNT] = gridsX[j];
          tmpY[newCNT] = gridsY[j];
          tmpL[newCNT] = gridsL[j];
          newCNT++;

          if (gridsL[j] > gridsL[j + 1]) {
            for (int k = gridsL[j]; k > gridsL[j + 1]; k--) {
              tmpX[newCNT] = gridsX[j + 1];
              tmpY[newCNT] = gridsY[j + 1];
              tmpL[newCNT] = k;
              newCNT++;
            }
          } else if (gridsL[j] < gridsL[j + 1]) {
            for (int k = gridsL[j]; k < gridsL[j + 1]; k++) {
              tmpX[newCNT] = gridsX[j + 1];
              tmpY[newCNT] = gridsY[j + 1];
              tmpL[newCNT] = k;
              newCNT++;
            }
          }
        }
        tmpX[newCNT] = gridsX[j];
        tmpY[newCNT] = gridsY[j];
        tmpL[newCNT] = gridsL[j];
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

static bool comparePVMINX(const OrderNetPin& a, const OrderNetPin& b)
{
  return a.minX < b.minX;
}

static bool comparePVPV(const OrderNetPin& a, const OrderNetPin& b)
{
  return a.npv < b.npv;
}

void FastRouteCore::netpinOrderInc()
{
  tree_order_pv_.clear();

  for (int j = 0; j < netCount(); j++) {
    if (nets_[j]->isRouted())
      continue;

    int xmin = BIG_INT;
    int totalLength = 0;
    const auto& treenodes = sttrees_[j].nodes;
    StTree* stree = &(sttrees_[j]);
    const int num_edges = stree->num_edges();
    for (int ind = 0; ind < num_edges; ind++) {
      totalLength += stree->edges[ind].len;
      if (xmin < treenodes[stree->edges[ind].n1].x) {
        xmin = treenodes[stree->edges[ind].n1].x;
      }
    }

    float npvalue = (float) totalLength / stree->num_terminals;

    tree_order_pv_.push_back({j, xmin, npvalue});
  }

  std::stable_sort(tree_order_pv_.begin(), tree_order_pv_.end(), comparePVMINX);
  std::stable_sort(tree_order_pv_.begin(), tree_order_pv_.end(), comparePVPV);
}

void FastRouteCore::fillVIA()
{
  short tmpX[MAXLEN], tmpY[MAXLEN], tmpL[MAXLEN];

  int numVIAT1 = 0;
  int numVIAT2 = 0;

  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treeedges = sttrees_[netID].edges;
    int num_terminals = sttrees_[netID].num_terminals;
    const auto& treenodes = sttrees_[netID].nodes;

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int newCNT = 0;
        int routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        int n1a = treeedge->n1a;
        int n2a = treeedge->n2a;

        if (edgeID == treenodes[n1a].hID || edgeID == treenodes[n2a].hID) {
          if (edgeID == treenodes[n1a].hID) {
            for (int k = treenodes[n1a].botL; k < treenodes[n1a].topL; k++) {
              tmpX[newCNT] = gridsX[0];
              tmpY[newCNT] = gridsY[0];
              tmpL[newCNT] = k;
              newCNT++;
              if (n1a < num_terminals) {
                numVIAT1++;
              } else {
                numVIAT2++;
              }
            }
          }

          // finish from n1->real route

          int j;
          for (j = 0; j < routeLen; j++) {
            tmpX[newCNT] = gridsX[j];
            tmpY[newCNT] = gridsY[j];
            tmpL[newCNT] = gridsL[j];
            newCNT++;
          }
          tmpX[newCNT] = gridsX[j];
          tmpY[newCNT] = gridsY[j];
          tmpL[newCNT] = gridsL[j];
          newCNT++;

          if (edgeID == treenodes[n2a].hID) {
            if (treenodes[n2a].topL != treenodes[n2a].botL)
              for (int k = treenodes[n2a].topL - 1; k >= treenodes[n2a].botL;
                   k--) {
                tmpX[newCNT] = gridsX[routeLen];
                tmpY[newCNT] = gridsY[routeLen];
                tmpL[newCNT] = k;
                newCNT++;
                if (n2a < num_terminals) {
                  numVIAT1++;
                } else {
                  numVIAT2++;
                }
              }
          }
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

  if (verbose_) {
    logger_->info(GRT, 197, "Via related to pin nodes: {}", numVIAT1);
    logger_->info(GRT, 198, "Via related Steiner nodes: {}", numVIAT2);
    logger_->info(GRT, 199, "Via filling finished.");
  }
}

int FastRouteCore::threeDVIA()
{
  int numVIA = 0;

  for (int netID = 0; netID < netCount(); netID++) {
    const auto& treeedges = sttrees_[netID].edges;
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
                                      multi_array<Edge3D, 3>& edges_3D,
                                      int x,
                                      int y,
                                      int k,
                                      int l,
                                      bool vertical,
                                      int& best_cost)
{
  bool is_vertical = ((l % 2) - layer_orientation_) != 0;
  // if layer direction doesn't match edge direction or
  // if already found a layer for the edge, ignores the remaining layers
  if (is_vertical != vertical || best_cost > 0) {
    layer_grid_[l][k] = std::numeric_limits<int>::min();
  } else {
    layer_grid_[l][k] = edges_3D[l][y][x].cap - edges_3D[l][y][x].usage;
    best_cost = std::max(best_cost, layer_grid_[l][k]);
    if (best_cost > 0) {
      // set the new min/max routing layer for the net to avoid
      // errors during mazeRouteMSMDOrder3D
      net_layer = l;
    }
  }
}

void FastRouteCore::assignEdge(int netID, int edgeID, bool processDIR)
{
  std::vector<std::vector<long>> gridD;
  int i, k, l, min_x, min_y, routelen, n1a, n2a, last_layer;
  int min_result, endLayer = 0;

  FrNet* net = nets_[netID];
  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;
  TreeEdge* treeedge = &(treeedges[edgeID]);

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  std::vector<short>& gridsL = treeedge->route.gridsL;

  routelen = treeedge->route.routelen;
  n1a = treeedge->n1a;
  n2a = treeedge->n2a;

  gridD.resize(num_layers_);
  for (int i = 0; i < num_layers_; i++) {
    gridD[i].resize(treeedge->route.routelen + 1);
  }

  for (l = 0; l < num_layers_; l++) {
    for (k = 0; k <= routelen; k++) {
      gridD[l][k] = BIG_INT;
      via_link_[l][k] = BIG_INT;
    }
  }

  for (k = 0; k < routelen; k++) {
    int best_cost = std::numeric_limits<int>::min();
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      for (l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        // check if the current layer is vertical to match the edge orientation
        bool is_vertical = ((l % 2) - layer_orientation_) != 0;
        if (is_vertical) {
          layer_grid_[l][k] = v_edges_3D_[l][min_y][gridsX[k]].cap
                              - v_edges_3D_[l][min_y][gridsX[k]].usage;
          best_cost = std::max(best_cost, layer_grid_[l][k]);
        } else {
          layer_grid_[l][k] = std::numeric_limits<int>::min();
        }
      }

      if (best_cost
          <= 0) {  // assigning the edge to the layer range would cause overflow
        // try to assign the edge to the closest layer below the min routing
        // layer
        int min_layer = net->getMinLayer();
        for (l = net->getMinLayer() - 1; l >= 0; l--) {
          fixEdgeAssignment(
              min_layer, v_edges_3D_, gridsX[k], min_y, k, l, true, best_cost);
        }
        net->setMinLayer(min_layer);
        // try to assign the edge to the closest layer above the max routing
        // layer
        int max_layer = net->getMaxLayer();
        for (l = net->getMaxLayer() + 1; l < num_layers_; l++) {
          fixEdgeAssignment(
              max_layer, v_edges_3D_, gridsX[k], min_y, k, l, true, best_cost);
        }
        net->setMaxLayer(max_layer);
      } else {  // the edge was assigned to a layer without causing overflow
        for (l = 0; l < num_layers_; l++) {
          if (l < net->getMinLayer() || l > net->getMaxLayer()) {
            layer_grid_[l][k] = std::numeric_limits<int>::min();
          }
        }
      }
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      for (l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        // check if the current layer is horizontal to match the edge
        // orientation
        bool is_horizontal = ((l % 2) - layer_orientation_) == 0;
        if (is_horizontal) {
          layer_grid_[l][k] = h_edges_3D_[l][gridsY[k]][min_x].cap
                              - h_edges_3D_[l][gridsY[k]][min_x].usage;
          best_cost = std::max(best_cost, layer_grid_[l][k]);
        } else {
          layer_grid_[l][k] = std::numeric_limits<int>::min();
        }
      }

      if (best_cost
          <= 0) {  // assigning the edge to the layer range would cause overflow
        // try to assign the edge to the closest layer below the min routing
        // layer
        int min_layer = net->getMinLayer();
        for (l = net->getMinLayer() - 1; l >= 0; l--) {
          fixEdgeAssignment(
              min_layer, h_edges_3D_, min_x, gridsY[k], k, l, false, best_cost);
        }
        net->setMinLayer(min_layer);
        // try to assign the edge to the closest layer above the max routing
        // layer
        int max_layer = net->getMaxLayer();
        for (l = net->getMaxLayer() + 1; l < num_layers_; l++) {
          fixEdgeAssignment(
              max_layer, h_edges_3D_, min_x, gridsY[k], k, l, false, best_cost);
        }
        net->setMaxLayer(max_layer);
      } else {  // the edge was assigned to a layer without causing overflow
        for (l = 0; l < num_layers_; l++) {
          if (l < net->getMinLayer() || l > net->getMaxLayer()) {
            layer_grid_[l][k] = std::numeric_limits<int>::min();
          }
        }
      }
    }
  }

  if (processDIR) {
    if (treenodes[n1a].assigned) {
      for (l = treenodes[n1a].botL; l <= treenodes[n1a].topL; l++) {
        gridD[l][0] = 0;
      }
    } else {
      if (verbose_)
        logger_->warn(GRT, 200, "Start point not assigned.");
      fflush(stdout);
    }

    for (k = 0; k < routelen; k++) {
      for (l = 0; l < num_layers_; l++) {
        for (i = 0; i < num_layers_; i++) {
          if (k == 0) {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
                via_link_[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
                via_link_[i][k] = l;
              }
            }
          }
        }
      }
      for (l = 0; l < num_layers_; l++) {
        if (layer_grid_[l][k] > 0) {
          gridD[l][k + 1] = gridD[l][k] + 1;
        } else if (layer_grid_[l][k] == std::numeric_limits<int>::min()) {
          // when the layer orientation doesn't match the edge orientation,
          // set a larger weight to avoid assigning to this layer when the
          // routing has 3D overflow
          gridD[l][k + 1] = gridD[l][k] + 2 * BIG_INT;
        } else {
          gridD[l][k + 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (l = 0; l < num_layers_; l++) {
      for (i = 0; i < num_layers_; i++) {
        if (l != i) {
          if (gridD[i][k] > gridD[l][k] + abs(i - l) * 1) {
            gridD[i][k] = gridD[l][k] + abs(i - l) * 1;
            via_link_[i][k] = l;
          }
        }
      }
    }

    k = routelen;

    if (treenodes[n2a].assigned) {
      min_result = BIG_INT;
      for (i = treenodes[n2a].topL; i >= treenodes[n2a].botL; i--) {
        if (gridD[i][routelen] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    } else {
      min_result = gridD[0][routelen];
      endLayer = 0;
      for (i = 0; i < num_layers_; i++) {
        if (gridD[i][routelen] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    }

    if (via_link_[endLayer][routelen] == BIG_INT) {
      last_layer = endLayer;
    } else {
      last_layer = via_link_[endLayer][routelen];
    }

    for (k = routelen; k >= 0; k--) {
      gridsL[k] = last_layer;
      if (via_link_[last_layer][k] != BIG_INT) {
        last_layer = via_link_[last_layer][k];
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
      for (l = treenodes[n2a].botL; l <= treenodes[n2a].topL; l++) {
        gridD[l][routelen] = 0;
      }
    }

    for (k = routelen; k > 0; k--) {
      for (l = 0; l < num_layers_; l++) {
        for (i = 0; i < num_layers_; i++) {
          if (k == routelen) {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
                via_link_[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
                via_link_[i][k] = l;
              }
            }
          }
        }
      }
      for (l = 0; l < num_layers_; l++) {
        if (layer_grid_[l][k - 1] > 0) {
          gridD[l][k - 1] = gridD[l][k] + 1;
        } else if (layer_grid_[l][k] == std::numeric_limits<int>::min()) {
          // when the layer orientation doesn't match the edge orientation,
          // set a larger weight to avoid assigning to this layer when the
          // routing has 3D overflow
          gridD[l][k - 1] = gridD[l][k] + 2 * BIG_INT;
        } else {
          gridD[l][k - 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (l = 0; l < num_layers_; l++) {
      for (i = 0; i < num_layers_; i++) {
        if (l != i) {
          if (gridD[i][0] > gridD[l][0] + abs(i - l) * 1) {
            gridD[i][0] = gridD[l][0] + abs(i - l) * 1;
            via_link_[i][0] = l;
          }
        }
      }
    }

    if (treenodes[n1a].assigned) {
      min_result = BIG_INT;
      for (i = treenodes[n1a].topL; i >= treenodes[n1a].botL; i--) {
        if (gridD[i][k] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][0];
          endLayer = i;
        }
      }

    } else {
      min_result = gridD[0][k];
      endLayer = 0;
      for (i = 0; i < num_layers_; i++) {
        if (gridD[i][k] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][k];
          endLayer = i;
        }
      }
    }

    last_layer = endLayer;

    for (k = 0; k <= routelen; k++) {
      if (via_link_[last_layer][k] != BIG_INT) {
        last_layer = via_link_[last_layer][k];
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
      min_y = std::min(gridsY[k], gridsY[k + 1]);

      v_edges_3D_[gridsL[k]][min_y][gridsX[k]].usage
          += net->getLayerEdgeCost(gridsL[k]);
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);

      h_edges_3D_[gridsL[k]][gridsY[k]][min_x].usage
          += net->getLayerEdgeCost(gridsL[k]);
    }
  }
}

void FastRouteCore::layerAssignmentV4()
{
  int i, k, netID, edgeID, nodeID, routeLen;
  int n1, n2, connectionCNT;

  int n1a, n2a;
  std::queue<int> edgeQueue;

  TreeEdge* treeedge;

  for (netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treeedges = sttrees_[netID].edges;
    for (edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        treeedge->route.gridsL.resize(routeLen + 1, 0);
        treeedge->assigned = false;
      }
    }
  }
  netpinOrderInc();

  for (i = 0; i < tree_order_pv_.size(); i++) {
    netID = tree_order_pv_[i].treeIndex;

    if (nets_[netID]->isRouted())
      continue;

    const auto& treeedges = sttrees_[netID].edges;
    const auto& treenodes = sttrees_[netID].nodes;
    const int num_terminals = sttrees_[netID].num_terminals;

    for (nodeID = 0; nodeID < num_terminals; nodeID++) {
      for (k = 0; k < treenodes[nodeID].conCNT; k++) {
        edgeID = treenodes[nodeID].eID[k];
        if (!treeedges[edgeID].assigned) {
          edgeQueue.push(edgeID);
          treeedges[edgeID].assigned = true;
        }
      }
    }

    while (!edgeQueue.empty()) {
      edgeID = edgeQueue.front();
      edgeQueue.pop();
      treeedge = &(treeedges[edgeID]);
      if (treenodes[treeedge->n1a].assigned) {
        assignEdge(netID, edgeID, 1);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n2a].assigned) {
          for (k = 0; k < treenodes[treeedge->n2a].conCNT; k++) {
            edgeID = treenodes[treeedge->n2a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = true;
            }
          }
          treenodes[treeedge->n2a].assigned = true;
        }
      } else {
        assignEdge(netID, edgeID, 0);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n1a].assigned) {
          for (k = 0; k < treenodes[treeedge->n1a].conCNT; k++) {
            edgeID = treenodes[treeedge->n1a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = true;
            }
          }
          treenodes[treeedge->n1a].assigned = true;
        }
      }
    }

    for (nodeID = 0; nodeID < sttrees_[netID].num_nodes; nodeID++) {
      treenodes[nodeID].topL = -1;
      treenodes[nodeID].botL = num_layers_;
      treenodes[nodeID].conCNT = 0;
      treenodes[nodeID].hID = BIG_INT;
      treenodes[nodeID].lID = BIG_INT;
      treenodes[nodeID].status = 0;
      treenodes[nodeID].assigned = false;

      if (nodeID < num_terminals) {
        treenodes[nodeID].botL = 0;
        treenodes[nodeID].assigned = true;
        treenodes[nodeID].status = 1;
      }
    }

    for (edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      treeedge = &(treeedges[edgeID]);

      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;

        n1 = treeedge->n1;
        n2 = treeedge->n2;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        n1a = treenodes[n1].stackAlias;
        n2a = treenodes[n2].stackAlias;
        connectionCNT = treenodes[n1a].conCNT;
        treenodes[n1a].heights[connectionCNT] = gridsL[0];
        treenodes[n1a].eID[connectionCNT] = edgeID;
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

        connectionCNT = treenodes[n2a].conCNT;
        treenodes[n2a].heights[connectionCNT] = gridsL[routeLen];
        treenodes[n2a].eID[connectionCNT] = edgeID;
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

      }  // edge len > 0
    }    // eunmerating edges
  }
}

void FastRouteCore::layerAssignment()
{
  int netID, d, k, edgeID, numpoints, n1, n2;
  bool redundant;
  TreeEdge* treeedge;

  for (netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treenodes = sttrees_[netID].nodes;

    numpoints = 0;

    for (d = 0; d < sttrees_[netID].num_nodes; d++) {
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
        treenodes[d].botL = treenodes[d].topL = 0;
        // treenodes[d].l = 0;
        treenodes[d].assigned = true;
        treenodes[d].status = 1;

        xcor_[numpoints] = treenodes[d].x;
        ycor_[numpoints] = treenodes[d].y;
        dcor_[numpoints] = d;
        numpoints++;
      } else {
        redundant = false;
        for (k = 0; k < numpoints; k++) {
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

  for (netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treeedges = sttrees_[netID].edges;
    const auto& treenodes = sttrees_[netID].nodes;

    for (edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        n1 = treeedge->n1;
        n2 = treeedge->n2;

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

void FastRouteCore::printEdge3D(int netID, int edgeID)
{
  TreeEdge edge = sttrees_[netID].edges[edgeID];
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
      edge_rpt = edge_rpt + "(" + std::to_string(x) + " " + std::to_string(y)
                 + " " + std::to_string(edge.route.gridsL[i]) + ") ";
    }
    logger_->report("\t\t{}", edge_rpt);
  }
}

void FastRouteCore::printTree3D(int netID)
{
  for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes; nodeID++) {
    int x = tile_size_ * (sttrees_[netID].nodes[nodeID].x + 0.5) + x_corner_;
    int y = tile_size_ * (sttrees_[netID].nodes[nodeID].y + 0.5) + y_corner_;
    logger_->report("nodeID {},  [{}, {}], status: {}",
                    nodeID,
                    x,
                    y,
                    sttrees_[netID].nodes[nodeID].status);
  }

  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    printEdge3D(netID, edgeID);
  }
}

void FastRouteCore::checkRoute3D()
{
  int i, netID, edgeID, nodeID, edgelength;
  int n1, n2, x1, y1, x2, y2;
  int distance;
  bool gridFlag;

  for (netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    const auto& treenodes = sttrees_[netID].nodes;
    const int num_terminals = sttrees_[netID].num_terminals;

    for (nodeID = 0; nodeID < sttrees_[netID].num_nodes; nodeID++) {
      if (nodeID < num_terminals) {
        if (treenodes[nodeID].botL != 0) {
          logger_->error(GRT, 203, "Caused floating pin node.");
        }
      }
    }
    for (edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      if (sttrees_[netID].edges[edgeID].len == 0) {
        continue;
      }
      TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
      edgelength = treeedge->route.routelen;
      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;
      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      const std::vector<short>& gridsL = treeedge->route.gridsL;

      gridFlag = false;

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
      for (i = 0; i < treeedge->route.routelen; i++) {
        distance = abs(gridsX[i + 1] - gridsX[i])
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

      for (i = 0; i <= treeedge->route.routelen; i++) {
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
  int i, j, ind, min_x, min_y;
  StTree* stree;

  tree_order_cong_.clear();

  tree_order_cong_.resize(netCount());

  i = 0;
  for (j = 0; j < netCount(); j++) {
    stree = &(sttrees_[j]);
    tree_order_cong_[j].xmin = 0;
    tree_order_cong_[j].treeIndex = j;

    // if the net is routed
    if (nets_[j]->isRouted())
      continue;

    for (ind = 0; ind < stree->num_edges(); ind++) {
      const auto& treeedges = stree->edges;
      const TreeEdge* treeedge = &(treeedges[ind]);

      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      for (i = 0; i < treeedge->route.routelen; i++) {
        if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
          min_y = std::min(gridsY[i], gridsY[i + 1]);
          const int cap = getEdgeCapacity(
              nets_[j], gridsX[i], min_y, EdgeDirection::Vertical);
          tree_order_cong_[j].xmin
              += std::max(0, v_edges_[min_y][gridsX[i]].usage - cap);
        } else {  // a horizontal edge
          min_x = std::min(gridsX[i], gridsX[i + 1]);
          const int cap = getEdgeCapacity(
              nets_[j], min_x, gridsY[i], EdgeDirection::Horizontal);
          tree_order_cong_[j].xmin
              += std::max(0, h_edges_[gridsY[i]][min_x].usage - cap);
        }
      }
    }
  }

  std::stable_sort(
      tree_order_cong_.begin(), tree_order_cong_.end(), compareTEL);

  auto compareSlack = [this](const OrderTree a, const OrderTree b) {
    const FrNet* net_a = nets_[a.treeIndex];
    const FrNet* net_b = nets_[b.treeIndex];
    return net_a->getSlack() < net_b->getSlack();
  };
  // sort by slack after congestion sort
  std::stable_sort(
      tree_order_cong_.begin(), tree_order_cong_.end(), compareSlack);
}

void FastRouteCore::recoverEdge(int netID, int edgeID)
{
  int i, ymin, xmin, n1a, n2a;
  int connectionCNT, routeLen;

  const auto& treeedges = sttrees_[netID].edges;
  const TreeEdge* treeedge = &(treeedges[edgeID]);

  routeLen = treeedge->route.routelen;

  if (treeedge->len == 0) {
    logger_->error(GRT, 206, "Trying to recover a 0-length edge.");
  }

  const auto& treenodes = sttrees_[netID].nodes;

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  const std::vector<short>& gridsL = treeedge->route.gridsL;

  n1a = treeedge->n1a;
  n2a = treeedge->n2a;

  connectionCNT = treenodes[n1a].conCNT;
  treenodes[n1a].heights[connectionCNT] = gridsL[0];
  treenodes[n1a].eID[connectionCNT] = edgeID;
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

  connectionCNT = treenodes[n2a].conCNT;
  treenodes[n2a].heights[connectionCNT] = gridsL[routeLen];
  treenodes[n2a].eID[connectionCNT] = edgeID;
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
  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        v_edges_[ymin][gridsX[i]].usage += net->getEdgeCost();
        v_edges_3D_[gridsL[i]][ymin][gridsX[i]].usage
            += net->getLayerEdgeCost(gridsL[i]);
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        h_edges_[gridsY[i]][xmin].usage += net->getEdgeCost();
        h_edges_3D_[gridsL[i]][gridsY[i]][xmin].usage
            += net->getLayerEdgeCost(gridsL[i]);
      }
    }
  }
}

void FastRouteCore::removeLoops()
{
  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted()) {
      continue;
    }

    const auto& treeedges = sttrees_[netID].edges;

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
                  v_edges_[min_y][gridsX[k]].usage -= edgeCost;
                }
              } else {
                const int min_x = std::min(gridsX[k], gridsX[k + 1]);
                h_edges_[gridsY[k]][min_x].usage -= edgeCost;
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

// This is a full comparison between edges used by the routing and
// the usage in the h/v edges of the routing graph.  It is somewhat
// expensive but helpful for finding usage errors.
void FastRouteCore::verify2DEdgesUsage()
{
  multi_array<int, 2> v_edges(boost::extents[y_grid_ - 1][x_grid_]);
  multi_array<int, 2> h_edges(boost::extents[y_grid_][x_grid_ - 1]);

  for (int netID = 0; netID < netCount(); netID++) {
    const auto& treenodes = sttrees_[netID].nodes;
    const auto& treeedges = sttrees_[netID].edges;
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

      const int ymin = std::min(y1, y2);
      const int ymax = std::max(y1, y2);

      if (treeedge->route.type == RouteType::LRoute) {
        if (treeedge->route.xFirst) {  // horizontal first
          for (int j = x1; j < x2; j++) {
            h_edges[y1][j]++;
          }
          for (int j = ymin; j < ymax; j++) {
            v_edges[j][x2]++;
          }
        } else {  // vertical first
          for (int j = ymin; j < ymax; j++) {
            v_edges[j][x1]++;
          }
          for (int j = x1; j < x2; j++) {
            h_edges[y2][j]++;
          }
        }
      } else if (treeedge->route.type == RouteType::ZRoute) {
        const int Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH)  // HVH
        {
          for (int i = x1; i < Zpoint; i++) {
            h_edges[y1][i]++;
          }
          for (int i = Zpoint; i < x2; i++) {
            h_edges[y2][i]++;
          }
          for (int i = ymin; i < ymax; i++) {
            v_edges[i][Zpoint]++;
          }
        } else {  // VHV
          if (y1 <= y2) {
            for (int i = y1; i < Zpoint; i++) {
              v_edges[i][x1]++;
            }
            for (int i = Zpoint; i < y2; i++) {
              v_edges[i][x2]++;
            }
            for (int i = x1; i < x2; i++) {
              h_edges[Zpoint][i]++;
            }
          } else {
            for (int i = y2; i < Zpoint; i++) {
              v_edges[i][x2]++;
            }
            for (int i = Zpoint; i < y1; i++) {
              v_edges[i][x1]++;
            }
            for (int i = x1; i < x2; i++) {
              h_edges[Zpoint][i]++;
            }
          }
        }
      }
    }
  }
  for (int y = 0; y < y_grid_ - 1; ++y) {
    for (int x = 0; x < x_grid_; ++x) {
      if (v_edges[y][x] != v_edges_[y][x].est_usage) {
        logger_->error(GRT,
                       247,
                       "v_edge mismatch {} vs {}",
                       v_edges[y][x],
                       v_edges_[y][x].est_usage);
      }
    }
  }
  for (int y = 0; y < y_grid_; ++y) {
    for (int x = 0; x < x_grid_ - 1; ++x) {
      if (h_edges[y][x] != h_edges_[y][x].est_usage) {
        logger_->error(GRT,
                       248,
                       "h_edge mismatch {} vs {}",
                       h_edges[y][x],
                       h_edges_[y][x].est_usage);
      }
    }
  }
}

void FastRouteCore::check2DEdgesUsage()
{
  const int max_usage_multiplier = 40;
  int max_h_edge_usage = max_usage_multiplier * h_capacity_;
  int max_v_edge_usage = max_usage_multiplier * v_capacity_;

  // check horizontal edges
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      if (h_edges_[i][j].usage > max_h_edge_usage) {
        logger_->error(GRT,
                       228,
                       "Horizontal edge usage exceeds the maximum allowed. "
                       "({}, {}) usage={} limit={}",
                       i,
                       j,
                       h_edges_[i][j].usage,
                       max_h_edge_usage);
      }
    }
  }

  // check vertical edges
  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      if (v_edges_[i][j].usage > max_v_edge_usage) {
        logger_->error(GRT,
                       229,
                       "Vertical edge usage exceeds the maximum allowed. "
                       "({}, {}) usage={} limit={}",
                       i,
                       j,
                       v_edges_[i][j].usage,
                       max_v_edge_usage);
      }
    }
  }
}

static bool compareEdgeLen(const OrderNetEdge a, const OrderNetEdge b)
{
  return a.length > b.length;
}

void FastRouteCore::netedgeOrderDec(int netID)
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

void FastRouteCore::printEdge2D(int netID, int edgeID)
{
  const TreeEdge edge = sttrees_[netID].edges[edgeID];
  const auto& nodes = sttrees_[netID].nodes;

  logger_->report("edge {}: n1 {} ({}, {})-> n2 {}({}, {}), routeType {}",
                  edgeID,
                  edge.n1,
                  nodes[edge.n1].x,
                  nodes[edge.n1].y,
                  edge.n2,
                  nodes[edge.n2].x,
                  nodes[edge.n2].y,
                  edge.route.type);
  if (edge.len > 0) {
    std::string edge_rpt;
    for (int i = 0; i <= edge.route.routelen; i++) {
      edge_rpt = edge_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", "
                 + std::to_string(edge.route.gridsY[i]) + ") ";
    }
    logger_->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree2D(int netID)
{
  for (int nodeID = 0; nodeID < sttrees_[netID].num_nodes; nodeID++) {
    logger_->report("nodeID {},  [{}, {}]",
                    nodeID,
                    sttrees_[netID].nodes[nodeID].y,
                    sttrees_[netID].nodes[nodeID].x);
  }

  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    printEdge2D(netID, edgeID);
  }
}

bool FastRouteCore::checkRoute2DTree(int netID)
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
      if (verbose_)
        logger_->warn(
            GRT, 207, "Ripped up edge without edge length reassignment.");
      STHwrong = true;
    }

    if (treeedge->len > 0) {
      if (treeedge->route.routelen < 1) {
        if (verbose_)
          logger_->warn(GRT,
                        208,
                        "Route length {}, tree length {}.",
                        treeedge->route.routelen,
                        treeedge->len);
        STHwrong = true;
        return true;
      }

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        if (verbose_)
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
        STHwrong = true;
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        if (verbose_)
          logger_->warn(
              GRT,
              165,
              "End grid wrong y2 x2 [{} {}], net start [{} {}] routelen {}.",
              y1,
              x1,
              gridsY[edgelength],
              gridsX[edgelength],
              treeedge->route.routelen);
        STHwrong = true;
      }
      for (int i = 0; i < treeedge->route.routelen; i++) {
        const int distance
            = abs(gridsX[i + 1] - gridsX[i]) + abs(gridsY[i + 1] - gridsY[i]);
        if (distance != 1) {
          if (verbose_)
            logger_->warn(
                GRT,
                166,
                "Net {} edge[{}] maze route wrong, distance {}, i {}.",
                nets_[netID]->getName(),
                edgeID,
                distance,
                i);
          STHwrong = true;
        }
      }

      if (STHwrong) {
        if (verbose_)
          logger_->warn(
              GRT, 167, "Invalid 2D tree for net {}.", nets_[netID]->getName());
        return true;
      }
    }
  }

  return (STHwrong);
}

// Copy Routing Solution for the best routing solution so far
void FastRouteCore::copyRS(void)
{
  int i, j, netID, edgeID, numEdges, numNodes;

  if (!sttrees_bk_.empty()) {
    for (netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->isRouted())
        continue;

      numEdges = sttrees_bk_[netID].num_edges();
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }
    sttrees_bk_.clear();
  }

  sttrees_bk_.resize(netCount());

  for (netID = 0; netID < netCount(); netID++) {
    if (nets_[netID]->isRouted())
      continue;

    numNodes = sttrees_[netID].num_nodes;
    numEdges = sttrees_[netID].num_edges();

    sttrees_bk_[netID].nodes.reset(new TreeNode[numNodes]);

    for (i = 0; i < numNodes; i++) {
      sttrees_bk_[netID].nodes[i].x = sttrees_[netID].nodes[i].x;
      sttrees_bk_[netID].nodes[i].y = sttrees_[netID].nodes[i].y;
      for (j = 0; j < 3; j++) {
        sttrees_bk_[netID].nodes[i].nbr[j] = sttrees_[netID].nodes[i].nbr[j];
        sttrees_bk_[netID].nodes[i].edge[j] = sttrees_[netID].nodes[i].edge[j];
      }
    }
    sttrees_bk_[netID].num_nodes = sttrees_[netID].num_nodes;
    sttrees_bk_[netID].num_terminals = sttrees_[netID].num_terminals;

    sttrees_bk_[netID].edges.reset(new TreeEdge[numEdges]);

    for (edgeID = 0; edgeID < numEdges; edgeID++) {
      sttrees_bk_[netID].edges[edgeID].len = sttrees_[netID].edges[edgeID].len;
      sttrees_bk_[netID].edges[edgeID].n1 = sttrees_[netID].edges[edgeID].n1;
      sttrees_bk_[netID].edges[edgeID].n2 = sttrees_[netID].edges[edgeID].n2;

      if (sttrees_[netID].edges[edgeID].len
          > 0)  // only route the non-degraded edges (len>0)
      {
        sttrees_bk_[netID].edges[edgeID].route.routelen
            = sttrees_[netID].edges[edgeID].route.routelen;
        sttrees_bk_[netID].edges[edgeID].route.gridsX.resize(
            sttrees_[netID].edges[edgeID].route.routelen + 1, 0);
        sttrees_bk_[netID].edges[edgeID].route.gridsY.resize(
            sttrees_[netID].edges[edgeID].route.routelen + 1, 0);

        for (i = 0; i <= sttrees_[netID].edges[edgeID].route.routelen; i++) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX[i]
              = sttrees_[netID].edges[edgeID].route.gridsX[i];
          sttrees_bk_[netID].edges[edgeID].route.gridsY[i]
              = sttrees_[netID].edges[edgeID].route.gridsY[i];
        }
      }
    }
  }
}

void FastRouteCore::copyBR(void)
{
  int i, j, netID, edgeID, numEdges, numNodes, min_y, min_x;

  if (!sttrees_bk_.empty()) {
    for (netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->isRouted())
        continue;

      numEdges = sttrees_[netID].num_edges();
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_[netID].edges[edgeID].len > 0) {
          sttrees_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }
    sttrees_.clear();

    sttrees_.resize(netCount());

    for (netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->isRouted())
        continue;

      numNodes = sttrees_bk_[netID].num_nodes;
      numEdges = sttrees_bk_[netID].num_edges();

      sttrees_[netID].nodes.reset(new TreeNode[numNodes]);

      for (i = 0; i < numNodes; i++) {
        sttrees_[netID].nodes[i].x = sttrees_bk_[netID].nodes[i].x;
        sttrees_[netID].nodes[i].y = sttrees_bk_[netID].nodes[i].y;
        for (j = 0; j < 3; j++) {
          sttrees_[netID].nodes[i].nbr[j] = sttrees_bk_[netID].nodes[i].nbr[j];
          sttrees_[netID].nodes[i].edge[j]
              = sttrees_bk_[netID].nodes[i].edge[j];
        }
      }

      sttrees_[netID].edges.reset(new TreeEdge[numEdges]);

      sttrees_[netID].num_nodes = sttrees_bk_[netID].num_nodes;
      sttrees_[netID].num_terminals = sttrees_bk_[netID].num_terminals;

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
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
          sttrees_[netID].edges[edgeID].route.type = RouteType::MazeRoute;
          sttrees_[netID].edges[edgeID].route.routelen
              = sttrees_bk_[netID].edges[edgeID].route.routelen;
          sttrees_[netID].edges[edgeID].route.gridsX.resize(
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1, 0);
          sttrees_[netID].edges[edgeID].route.gridsY.resize(
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1, 0);

          for (i = 0; i <= sttrees_bk_[netID].edges[edgeID].route.routelen;
               i++) {
            sttrees_[netID].edges[edgeID].route.gridsX[i]
                = sttrees_bk_[netID].edges[edgeID].route.gridsX[i];
            sttrees_[netID].edges[edgeID].route.gridsY[i]
                = sttrees_bk_[netID].edges[edgeID].route.gridsY[i];
          }
        }
      }
    }

    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        h_edges_[i][j].usage = 0;
      }
    }
    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        v_edges_[i][j].usage = 0;
      }
    }
    for (netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->isRouted())
        continue;

      numEdges = sttrees_[netID].num_edges();
      int edgeCost = nets_[netID]->getEdgeCost();

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_[netID].edges[edgeID].len > 0) {
          const std::vector<short>& gridsX
              = sttrees_[netID].edges[edgeID].route.gridsX;
          const std::vector<short>& gridsY
              = sttrees_[netID].edges[edgeID].route.gridsY;
          for (i = 0; i < sttrees_[netID].edges[edgeID].route.routelen; i++) {
            if (gridsX[i] == gridsX[i + 1])  // a vertical edge
            {
              min_y = std::min(gridsY[i], gridsY[i + 1]);
              v_edges_[min_y][gridsX[i]].usage += edgeCost;
            } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
            {
              min_x = std::min(gridsX[i], gridsX[i + 1]);
              h_edges_[gridsY[i]][min_x].usage += edgeCost;
            }
          }
        }
      }
    }
  }
}

void FastRouteCore::freeRR(void)
{
  int netID, edgeID, numEdges;
  if (!sttrees_bk_.empty()) {
    for (netID = 0; netID < netCount(); netID++) {
      if (nets_[netID]->isRouted())
        continue;

      numEdges = sttrees_bk_[netID].num_edges();
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
    }
    sttrees_bk_.clear();
  }
}

int FastRouteCore::edgeShift(Tree& t, int net)
{
  int i, j, k, l, m, deg, root = 0, x, y, n, n1, n2, n3;
  int maxX, minX, maxY, minY, maxX1, minX1, maxY1, minY1, maxX2, minX2, maxY2,
      minY2, bigX, smallX, bigY, smallY;
  int pairCnt;
  int benefit, bestBenefit, bestCost;
  int cost1, cost2, bestPair, Pos, bestPos, numShift = 0;

  // TODO: check this size
  const int sizeV = 2 * nets_[net]->getNumPins();
  int nbr[sizeV][3];
  int nbrCnt[sizeV];
  int pairN1[nets_[net]->getNumPins()];
  int pairN2[nets_[net]->getNumPins()];
  int costH[y_grid_];
  int costV[x_grid_];

  deg = t.deg;
  // find root of the tree
  for (i = deg; i < t.branchCount(); i++) {
    if (t.branch[i].n == i) {
      root = i;
      break;
    }
  }

  // find all neighbors for steiner nodes
  for (i = deg; i < t.branchCount(); i++)
    nbrCnt[i] = 0;
  // edges from pin to steiner
  for (i = 0; i < deg; i++) {
    n = t.branch[i].n;
    nbr[n][nbrCnt[n]] = i;
    nbrCnt[n]++;
  }
  // edges from steiner to steiner
  for (i = deg; i < t.branchCount(); i++) {
    if (i != root)  // not the removed steiner nodes and root
    {
      n = t.branch[i].n;
      nbr[i][nbrCnt[i]] = n;
      nbrCnt[i]++;
      nbr[n][nbrCnt[n]] = i;
      nbrCnt[n]++;
    }
  }

  bestBenefit = BIG_INT;   // used to enter while loop
  while (bestBenefit > 0)  // && numShift<60)
  {
    // find all H or V edges (steiner pairs)
    pairCnt = 0;
    for (i = deg; i < t.branchCount(); i++) {
      n = t.branch[i].n;
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

    bestPair = -1;
    bestBenefit = -1;
    // for each H or V edge, find the best benefit by shifting it
    for (i = 0; i < pairCnt; i++) {
      // find the range of shifting for this pair
      n1 = pairN1[i];
      n2 = pairN2[i];
      if (t.branch[n1].y == t.branch[n2].y)  // a horizontal edge
      {
        // find the shifting range for the edge (minY~maxY)
        maxY1 = minY1 = t.branch[n1].y;
        for (j = 0; j < 3; j++) {
          y = t.branch[nbr[n1][j]].y;
          if (y > maxY1)
            maxY1 = y;
          else if (y < minY1)
            minY1 = y;
        }
        maxY2 = minY2 = t.branch[n2].y;
        for (j = 0; j < 3; j++) {
          y = t.branch[nbr[n2][j]].y;
          if (y > maxY2)
            maxY2 = y;
          else if (y < minY2)
            minY2 = y;
        }
        minY = std::max(minY1, minY2);
        maxY = std::min(maxY1, maxY2);

        // find the best position (least total usage) to shift
        if (minY < maxY)  // more than 1 possible positions
        {
          for (j = minY; j <= maxY; j++) {
            costH[j] = 0;
            for (k = t.branch[n1].x; k < t.branch[n2].x; k++) {
              costH[j] += h_edges_[j][k].est_usage;
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (l = 0; l < nbrCnt[n1]; l++) {
              n3 = nbr[n1][l];
              if (n3 != n2)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (t.branch[n1].x < t.branch[n3].x) {
                  smallX = t.branch[n1].x;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = t.branch[n1].x;
                }
                if (j < t.branch[n3].y) {
                  smallY = j;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = j;
                }
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[smallY][m].est_usage;
                  cost2 += h_edges_[bigY][m].est_usage;
                }
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[m][bigX].est_usage;
                  cost2 += v_edges_[m][smallX].est_usage;
                }
                costH[j] += std::min(cost1, cost2);
              }  // if(n3!=n2)
            }    // loop l
            for (l = 0; l < nbrCnt[n2]; l++) {
              n3 = nbr[n2][l];
              if (n3 != n1)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (t.branch[n2].x < t.branch[n3].x) {
                  smallX = t.branch[n2].x;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = t.branch[n2].x;
                }
                if (j < t.branch[n3].y) {
                  smallY = j;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = j;
                }
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[smallY][m].est_usage;
                  cost2 += h_edges_[bigY][m].est_usage;
                }
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[m][bigX].est_usage;
                  cost2 += v_edges_[m][smallX].est_usage;
                }
                costH[j] += std::min(cost1, cost2);
              }  // if(n3!=n1)
            }    // loop l
          }      // loop j
          bestCost = BIG_INT;
          Pos = t.branch[n1].y;
          for (j = minY; j <= maxY; j++) {
            if (costH[j] < bestCost) {
              bestCost = costH[j];
              Pos = j;
            }
          }
          if (Pos != t.branch[n1].y)  // find a better position than current
          {
            benefit = costH[t.branch[n1].y] - bestCost;
            if (benefit > bestBenefit) {
              bestBenefit = benefit;
              bestPair = i;
              bestPos = Pos;
            }
          }
        }

      } else  // a vertical edge
      {
        // find the shifting range for the edge (minX~maxX)
        maxX1 = minX1 = t.branch[n1].x;
        for (j = 0; j < 3; j++) {
          x = t.branch[nbr[n1][j]].x;
          if (x > maxX1)
            maxX1 = x;
          else if (x < minX1)
            minX1 = x;
        }
        maxX2 = minX2 = t.branch[n2].x;
        for (j = 0; j < 3; j++) {
          x = t.branch[nbr[n2][j]].x;
          if (x > maxX2)
            maxX2 = x;
          else if (x < minX2)
            minX2 = x;
        }
        minX = std::max(minX1, minX2);
        maxX = std::min(maxX1, maxX2);

        // find the best position (least total usage) to shift
        if (minX < maxX)  // more than 1 possible positions
        {
          for (j = minX; j <= maxX; j++) {
            costV[j] = 0;
            for (k = t.branch[n1].y; k < t.branch[n2].y; k++) {
              costV[j] += v_edges_[k][j].est_usage;
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (l = 0; l < nbrCnt[n1]; l++) {
              n3 = nbr[n1][l];
              if (n3 != n2)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (j < t.branch[n3].x) {
                  smallX = j;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = j;
                }
                if (t.branch[n1].y < t.branch[n3].y) {
                  smallY = t.branch[n1].y;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = t.branch[n1].y;
                }
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[smallY][m].est_usage;
                  cost2 += h_edges_[bigY][m].est_usage;
                }
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[m][bigX].est_usage;
                  cost2 += v_edges_[m][smallX].est_usage;
                }
                costV[j] += std::min(cost1, cost2);
              }  // if(n3!=n2)
            }    // loop l
            for (l = 0; l < nbrCnt[n2]; l++) {
              n3 = nbr[n2][l];
              if (n3 != n1)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (j < t.branch[n3].x) {
                  smallX = j;
                  bigX = t.branch[n3].x;
                } else {
                  smallX = t.branch[n3].x;
                  bigX = j;
                }
                if (t.branch[n2].y < t.branch[n3].y) {
                  smallY = t.branch[n2].y;
                  bigY = t.branch[n3].y;
                } else {
                  smallY = t.branch[n3].y;
                  bigY = t.branch[n2].y;
                }
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[smallY][m].est_usage;
                  cost2 += h_edges_[bigY][m].est_usage;
                }
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[m][bigX].est_usage;
                  cost2 += v_edges_[m][smallX].est_usage;
                }
                costV[j] += std::min(cost1, cost2);
              }  // if(n3!=n1)
            }    // loop l
          }      // loop j
          bestCost = BIG_INT;
          Pos = t.branch[n1].x;
          for (j = minX; j <= maxX; j++) {
            if (costV[j] < bestCost) {
              bestCost = costV[j];
              Pos = j;
            }
          }
          if (Pos != t.branch[n1].x)  // find a better position than current
          {
            benefit = costV[t.branch[n1].x] - bestCost;
            if (benefit > bestBenefit) {
              bestBenefit = benefit;
              bestPair = i;
              bestPos = Pos;
            }
          }
        }

      }  // else (a vertical edge)

    }  // loop i

    if (bestBenefit > 0) {
      n1 = pairN1[bestPair];
      n2 = pairN2[bestPair];

      if (t.branch[n1].y == t.branch[n2].y)  // horizontal edge
      {
        t.branch[n1].y = bestPos;
        t.branch[n2].y = bestPos;
      }  // vertical edge
      else {
        t.branch[n1].x = bestPos;
        t.branch[n2].x = bestPos;
      }
      numShift++;
    }
  }  // while(bestBenefit>0)

  return (numShift);
}

// exchange Steiner nodes at the same position, then call edgeShift()
int FastRouteCore::edgeShiftNew(Tree& t, int net)
{
  int i, j, n;
  int deg, pairCnt, cur_pairN1, cur_pairN2;
  int N1nbrH, N1nbrV, N2nbrH, N2nbrV, iter;
  int numShift;
  bool isPair;

  numShift = edgeShift(t, net);
  deg = t.deg;

  const int sizeV = nets_[net]->getNumPins();
  int pairN1[sizeV];
  int pairN2[sizeV];

  iter = 0;
  cur_pairN1 = cur_pairN2 = -1;
  while (iter < 3) {
    iter++;

    // find all pairs of steiner node at the same position (steiner pairs)
    pairCnt = 0;
    for (i = deg; i < t.branchCount(); i++) {
      n = t.branch[i].n;
      if (n != i && n != t.branch[n].n && t.branch[i].x == t.branch[n].x
          && t.branch[i].y == t.branch[n].y) {
        pairN1[pairCnt] = i;
        pairN2[pairCnt] = n;
        pairCnt++;
      }
    }

    if (pairCnt > 0) {
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
      } else
        isPair = false;

      if (isPair)  // find a new pair to swap
      {
        N1nbrH = N1nbrV = N2nbrH = N2nbrV = -1;
        // find the nodes directed to cur_pairN1(2 nodes) and cur_pairN2(1
        // nodes)
        for (j = 0; j < t.branchCount(); j++) {
          n = t.branch[j].n;
          if (n == cur_pairN1) {
            if (t.branch[j].x == t.branch[cur_pairN1].x
                && t.branch[j].y != t.branch[cur_pairN1].y)
              N1nbrV = j;
            else if (t.branch[j].y == t.branch[cur_pairN1].y
                     && t.branch[j].x != t.branch[cur_pairN1].x)
              N1nbrH = j;
          } else if (n == cur_pairN2) {
            if (t.branch[j].x == t.branch[cur_pairN2].x
                && t.branch[j].y != t.branch[cur_pairN2].y)
              N2nbrV = j;
            else if (t.branch[j].y == t.branch[cur_pairN2].y
                     && t.branch[j].x != t.branch[cur_pairN2].x)
              N2nbrH = j;
          }
        }
        // find the node cur_pairN2 directed to
        n = t.branch[cur_pairN2].n;
        if (t.branch[n].x == t.branch[cur_pairN2].x
            && t.branch[n].y != t.branch[cur_pairN2].y)
          N2nbrV = n;
        else if (t.branch[n].y == t.branch[cur_pairN2].y
                 && t.branch[n].x != t.branch[cur_pairN2].x)
          N2nbrH = n;

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
      }  // if(isPair)

    }  // if(pairCnt>0)
    else
      iter = 3;

  }  // while

  return (numShift);
}

std::ostream& operator<<(std::ostream& os, RouteType type)
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
