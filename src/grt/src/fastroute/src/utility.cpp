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

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <queue>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

void FastRouteCore::printEdge(int netID, int edgeID)
{
  int i;
  TreeEdge edge;
  TreeNode* nodes;

  edge = sttrees_[netID].edges[edgeID];
  nodes = sttrees_[netID].nodes;

  logger_->report("edge {}: ({}, {})->({}, {})",
                 edgeID,
                 nodes[edge.n1].x,
                 nodes[edge.n1].y,
                 nodes[edge.n2].x,
                 nodes[edge.n2].y);
  std::string routes_rpt;
  for (i = 0; i <= edge.route.routelen; i++) {
    routes_rpt = routes_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", "
                 + std::to_string(edge.route.gridsY[i]) + ") ";
  }
  logger_->report("{}", routes_rpt);
}

void FastRouteCore::ConvertToFull3DType2()
{
  short tmpX[MAXLEN], tmpY[MAXLEN], tmpL[MAXLEN];
  int k, netID, edgeID, routeLen;
  int newCNT, deg, j;
  TreeEdge *treeedges, *treeedge;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    deg = sttrees_[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        newCNT = 0;
        routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;
        // finish from n1->real route
        for (j = 0; j < routeLen; j++) {
          tmpX[newCNT] = gridsX[j];
          tmpY[newCNT] = gridsY[j];
          tmpL[newCNT] = gridsL[j];
          newCNT++;

          if (gridsL[j] > gridsL[j + 1]) {
            for (k = gridsL[j]; k > gridsL[j + 1]; k--) {
              tmpX[newCNT] = gridsX[j + 1];
              tmpY[newCNT] = gridsY[j + 1];
              tmpL[newCNT] = k;
              newCNT++;
            }
          } else if (gridsL[j] < gridsL[j + 1]) {
            for (k = gridsL[j]; k < gridsL[j + 1]; k++) {
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
        treeedge->route.gridsX.resize(newCNT);
        treeedge->route.gridsY.resize(newCNT);
        treeedge->route.gridsL.resize(newCNT);
        treeedge->route.type = RouteType::MazeRoute;
        treeedge->route.routelen = newCNT - 1;

        for (k = 0; k < newCNT; k++) {
          treeedge->route.gridsX[k] = tmpX[k];
          treeedge->route.gridsY[k] = tmpY[k];
          treeedge->route.gridsL[k] = tmpL[k];
        }
      }
    }
  }
}

static int comparePVMINX(const OrderNetPin a, const OrderNetPin b)
{
  return a.minX < b.minX;
}

static int comparePVPV(const OrderNetPin a, const OrderNetPin b)
{
  return a.npv < b.npv;
}

void FastRouteCore::netpinOrderInc()
{
  int j, d, ind, totalLength, xmin;
  TreeNode* treenodes;
  StTree* stree;

  float npvalue;

  int numTreeedges = 0;
  for (j = 0; j < num_valid_nets_; j++) {
    d = sttrees_[j].deg;
    numTreeedges += 2 * d - 3;
  }

  tree_order_pv_.clear();

  tree_order_pv_.resize(num_valid_nets_);

  for (j = 0; j < num_valid_nets_; j++) {
    xmin = BIG_INT;
    totalLength = 0;
    treenodes = sttrees_[j].nodes;
    stree = &(sttrees_[j]);
    d = stree->deg;
    for (ind = 0; ind < 2 * d - 3; ind++) {
      totalLength += stree->edges[ind].len;
      if (xmin < treenodes[stree->edges[ind].n1].x) {
        xmin = treenodes[stree->edges[ind].n1].x;
      }
    }

    npvalue = (float) totalLength / d;

    tree_order_pv_[j].npv = npvalue;
    tree_order_pv_[j].treeIndex = j;
    tree_order_pv_[j].minX = xmin;
  }

  std::stable_sort(tree_order_pv_.begin(), tree_order_pv_.end(), comparePVMINX);
  std::stable_sort(tree_order_pv_.begin(), tree_order_pv_.end(), comparePVPV);
}

void FastRouteCore::fillVIA()
{
  short tmpX[MAXLEN], tmpY[MAXLEN], tmpL[MAXLEN];
  int k, netID, edgeID, routeLen, n1a, n2a;
  int newCNT, numVIAT1, numVIAT2, deg, j;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  numVIAT1 = 0;
  numVIAT2 = 0;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    deg = sttrees_[netID].deg;
    treenodes = sttrees_[netID].nodes;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        newCNT = 0;
        routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        n1a = treeedge->n1a;
        n2a = treeedge->n2a;

        if (edgeID == treenodes[n1a].hID || edgeID == treenodes[n2a].hID) {
          if (edgeID == treenodes[n1a].hID) {
            for (k = treenodes[n1a].botL; k < treenodes[n1a].topL; k++) {
              tmpX[newCNT] = gridsX[0];
              tmpY[newCNT] = gridsY[0];
              tmpL[newCNT] = k;
              newCNT++;
              if (n1a < deg) {
                numVIAT1++;
              } else {
                numVIAT2++;
              }
            }
          }

          // finish from n1->real route

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
              for (k = treenodes[n2a].topL - 1; k >= treenodes[n2a].botL; k--) {
                tmpX[newCNT] = gridsX[routeLen];
                tmpY[newCNT] = gridsY[routeLen];
                tmpL[newCNT] = k;
                newCNT++;
                if (n2a < deg) {
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
          treeedge->route.gridsX.resize(newCNT);
          treeedge->route.gridsY.resize(newCNT);
          treeedge->route.gridsL.resize(newCNT);
          treeedge->route.type = RouteType::MazeRoute;
          treeedge->route.routelen = newCNT - 1;

          for (k = 0; k < newCNT; k++) {
            treeedge->route.gridsX[k] = tmpX[k];
            treeedge->route.gridsY[k] = tmpY[k];
            treeedge->route.gridsL[k] = tmpL[k];
          }
        }
      }
    }
  }

  if (verbose_ > 1) {
    logger_->info(GRT, 197, "Via related to pin nodes: {}", numVIAT1);
    logger_->info(GRT, 198, "Via related Steiner nodes: {}", numVIAT2);
    logger_->info(GRT, 199, "Via filling finished.");
  }
}

int FastRouteCore::threeDVIA()
{
  int netID, edgeID, deg;
  int routeLen, numVIA, j;
  TreeEdge *treeedges, *treeedge;

  numVIA = 0;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    deg = sttrees_[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);

      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        const std::vector<short>& gridsL = treeedge->route.gridsL;

        for (j = 0; j < routeLen; j++) {
          if (gridsL[j] != gridsL[j + 1]) {
            numVIA++;
          }
        }
      }
    }
  }

  return (numVIA);
}

void FastRouteCore::assignEdge(int netID, int edgeID, bool processDIR)
{
  std::vector<std::vector<int>> gridD;
  int i, k, l, grid, min_x, min_y, routelen, n1a, n2a, last_layer;
  int min_result, endLayer;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;
  treeedge = &(treeedges[edgeID]);

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
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      for (l = 0; l < num_layers_; l++) {
        grid = l * grid_v_ + min_y * x_grid_ + gridsX[k];
        layer_grid_[l][k] = v_edges_3D_[grid].cap - v_edges_3D_[grid].usage;
      }
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      for (l = 0; l < num_layers_; l++) {
        grid = l * grid_h_ + gridsY[k] * (x_grid_ - 1) + min_x;
        layer_grid_[l][k] = h_edges_3D_[grid].cap - h_edges_3D_[grid].usage;
      }
    }
  }

  if (processDIR) {
    if (treenodes[n1a].assigned) {
      for (l = treenodes[n1a].botL; l <= treenodes[n1a].topL; l++) {
        gridD[l][0] = 0;
      }
    } else {
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
        } else {
          gridD[l][k + 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (l = 0; l < num_layers_; l++) {
      for (i = 0; i < num_layers_; i++) {
        if (l != i) {
          if (gridD[i][k]
              > gridD[l][k] + abs(i - l) * 1) {
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

  std::vector<int> edge_cost_per_layer = nets_[netID]->edge_cost_per_layer;

  for (k = 0; k < routelen; k++) {
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      grid = gridsL[k] * grid_v_ + min_y * x_grid_ + gridsX[k];

      v_edges_3D_[grid].usage += edge_cost_per_layer[gridsL[k]];
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      grid = gridsL[k] * grid_h_ + gridsY[k] * (x_grid_ - 1) + min_x;

      h_edges_3D_[grid].usage += edge_cost_per_layer[gridsL[k]];
    }
  }
}

void FastRouteCore::newLayerAssignmentV4()
{
  int i, k, netID, edgeID, nodeID, routeLen;
  int n1, n2, connectionCNT, deg;

  int n1a, n2a;
  std::queue<int> edgeQueue;

  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;
    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        treeedge->route.gridsL.resize(routeLen + 1);
        treeedge->assigned = false;
      }
    }
  }
  netpinOrderInc();

  for (i = 0; i < num_valid_nets_; i++) {
    netID = tree_order_pv_[i].treeIndex;
    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;

    for (nodeID = 0; nodeID < deg; nodeID++) {
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

    deg = sttrees_[netID].deg;

    for (nodeID = 0; nodeID < 2 * deg - 2; nodeID++) {
      treenodes[nodeID].topL = -1;
      treenodes[nodeID].botL = num_layers_;
      treenodes[nodeID].conCNT = 0;
      treenodes[nodeID].hID = BIG_INT;
      treenodes[nodeID].lID = BIG_INT;
      treenodes[nodeID].status = 0;
      treenodes[nodeID].assigned = false;

      if (nodeID < deg) {
        treenodes[nodeID].botL = 0;
        treenodes[nodeID].assigned = true;
        treenodes[nodeID].status = 1;
      }
    }

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
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

void FastRouteCore::newLA()
{
  int netID, d, k, edgeID, deg, numpoints, n1, n2;
  bool redundant;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;

    numpoints = 0;

    for (d = 0; d < 2 * deg - 2; d++) {
      treenodes[d].topL = -1;
      treenodes[d].botL = num_layers_;
      // treenodes[d].l = 0;
      treenodes[d].assigned = false;
      treenodes[d].stackAlias = d;
      treenodes[d].conCNT = 0;
      treenodes[d].hID = BIG_INT;
      treenodes[d].lID = BIG_INT;
      treenodes[d].status = 0;

      if (d < deg) {
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

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
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

  newLayerAssignmentV4();

  ConvertToFull3DType2();
}

void FastRouteCore::printEdge3D(int netID, int edgeID)
{
  int i;
  TreeEdge edge;
  TreeNode* nodes;

  edge = sttrees_[netID].edges[edgeID];
  nodes = sttrees_[netID].nodes;

  logger_->report("edge {}: n1 {} ({}, {})-> n2 {}({}, {})",
                 edgeID,
                 edge.n1,
                 nodes[edge.n1].x,
                 nodes[edge.n1].y,
                 edge.n2,
                 nodes[edge.n2].x,
                 nodes[edge.n2].y);
  if (edge.len > 0) {
    std::string edge_rpt;
    for (i = 0; i <= edge.route.routelen; i++) {
      edge_rpt = edge_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", "
                 + std::to_string(edge.route.gridsY[i]) + ", "
                 + std::to_string(edge.route.gridsL[i]) + ") ";
    }
    logger_->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree3D(int netID)
{
  int edgeID, nodeID;
  for (nodeID = 0; nodeID < 2 * sttrees_[netID].deg - 2; nodeID++) {
    logger_->report("nodeID {},  [{}, {}]",
                   nodeID,
                   sttrees_[netID].nodes[nodeID].y,
                   sttrees_[netID].nodes[nodeID].x);
  }

  for (edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
    printEdge3D(netID, edgeID);
  }
}

void FastRouteCore::checkRoute3D()
{
  int i, netID, edgeID, nodeID, edgelength;
  int n1, n2, x1, y1, x2, y2, deg;
  int distance;
  bool gridFlag;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;

    for (nodeID = 0; nodeID < 2 * deg - 2; nodeID++) {
      if (nodeID < deg) {
        if (treenodes[nodeID].botL != 0) {
          logger_->error(GRT, 203, "Caused floating pin node.");
        }
      }
    }
    for (edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
      if (sttrees_[netID].edges[edgeID].len == 0) {
        continue;
      }
      treeedge = &(sttrees_[netID].edges[edgeID]);
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
        debugPrint(logger_,
                   GRT,
                   "checkRoute3D",
                   1,
                   "net {} edge[{}] start node wrong, net deg {}, n1 {}",
                   netName(nets_[netID]),
                   edgeID,
                   deg,
                   n1);
        if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
          printEdge3D(netID, edgeID);
        }
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        debugPrint(logger_,
                   GRT,
                   "checkRoute3D",
                   1,
                   "net {} edge[{}] end node wrong, net deg {}, n2 {}",
                   netName(nets_[netID]),
                   edgeID,
                   deg,
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
                     netName(nets_[netID]),
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
      if (gridFlag) {
        printEdge3D(netID, edgeID);
      }
    }
  }
}

static int compareTEL(const OrderTree a, const OrderTree b)
{
  return a.xmin > b.xmin;
}

void FastRouteCore::StNetOrder()
{
  int i, j, d, ind, grid, min_x, min_y;
  TreeEdge *treeedges, *treeedge;
  StTree* stree;

  tree_order_cong_.clear();

  tree_order_cong_.resize(num_valid_nets_);

  i = 0;
  for (j = 0; j < num_valid_nets_; j++) {
    stree = &(sttrees_[j]);
    d = stree->deg;
    tree_order_cong_[j].xmin = 0;
    tree_order_cong_[j].treeIndex = j;
    for (ind = 0; ind < 2 * d - 3; ind++) {
      treeedges = stree->edges;
      treeedge = &(treeedges[ind]);

      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      for (i = 0; i < treeedge->route.routelen; i++) {
        if (gridsX[i] == gridsX[i + 1])  // a vertical edge
        {
          min_y = std::min(gridsY[i], gridsY[i + 1]);
          grid = min_y * x_grid_ + gridsX[i];
          tree_order_cong_[j].xmin
              += std::max(0, v_edges_[grid].usage - v_edges_[grid].cap);
        } else  // a horizontal edge
        {
          min_x = std::min(gridsX[i], gridsX[i + 1]);
          grid = gridsY[i] * (x_grid_ - 1) + min_x;
          tree_order_cong_[j].xmin
              += std::max(0, h_edges_[grid].usage - h_edges_[grid].cap);
        }
      }
    }
  }

  std::stable_sort(tree_order_cong_.begin(), tree_order_cong_.end(), compareTEL);
}

void FastRouteCore::recoverEdge(int netID, int edgeID)
{
  int i, grid, ymin, xmin, n1a, n2a;
  int connectionCNT, routeLen;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees_[netID].edges;
  treeedge = &(treeedges[edgeID]);

  routeLen = treeedge->route.routelen;

  if (treeedge->len == 0) {
    logger_->error(GRT, 206, "Trying to recover a 0-length edge.");
  }

  treenodes = sttrees_[netID].nodes;

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

  std::vector<int> edge_cost_per_layer = nets_[netID]->edge_cost_per_layer;

  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = gridsL[i] * grid_v_ + ymin * x_grid_ + gridsX[i];
        v_edges_3D_[grid].usage += edge_cost_per_layer[gridsL[i]];
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsL[i] * grid_h_ + gridsY[i] * (x_grid_ - 1) + xmin;
        h_edges_3D_[grid].usage += edge_cost_per_layer[gridsL[i]];
      }
    }
  }
}

void FastRouteCore::checkUsage()
{
  int netID, i, k, edgeID, deg;
  int j, cnt;
  bool redsus;
  TreeEdge *treeedges, *treeedge;
  TreeEdge edge;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treeedges = sttrees_[netID].edges;
    deg = sttrees_[netID].deg;

    int edgeCost = nets_[netID]->edgeCost;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      edge = sttrees_[netID].edges[edgeID];
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        std::vector<short>& gridsX = treeedge->route.gridsX;
        std::vector<short>& gridsY = treeedge->route.gridsY;

        redsus = true;

        while (redsus) {
          redsus = false;

          for (i = 0; i <= treeedge->route.routelen; i++) {
            for (j = 0; j < i; j++) {
              if (gridsX[i] == gridsX[j]
                  && gridsY[i] == gridsY[j])  // a vertical edge
              {
                // Update usage for edges to be removed
                for (k = j; k < i; k++) {
                  if (gridsX[k] == gridsX[k + 1]) {
                    int min_y = std::min(gridsY[k], gridsY[k + 1]);
                    int grid = min_y * x_grid_ + gridsX[k];
                    v_edges_[grid].usage -= edgeCost;
                  } else {
                    int min_x = std::min(gridsX[k], gridsX[k + 1]);
                    int grid = gridsY[k] * (x_grid_ - 1) + min_x;
                    h_edges_[grid].usage -= edgeCost;
                  }
                }

                cnt = 1;
                for (k = i + 1; k <= treeedge->route.routelen; k++) {
                  gridsX[j + cnt] = gridsX[k];
                  gridsY[j + cnt] = gridsY[k];
                  cnt++;
                }
                treeedge->route.routelen -= i - j;
                redsus = true;
                i = 0;
                j = 0;
              }
            }
          }
        }
      }
    }
  }
  if (verbose_ > 1) {
    logger_->report("Usage checked");
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
      int grid = i * (x_grid_ - 1) + j;
      if (h_edges_[grid].usage >= max_h_edge_usage) {
        logger_->error(
            GRT, 228, "Horizontal edge usage exceeds the maximum allowed.");
      }
    }
  }

  // check vertical edges
  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      int grid = i * x_grid_ + j;
      if (v_edges_[grid].usage >= max_v_edge_usage) {
        logger_->error(
            GRT, 229, "Vertical edge usage exceeds the maximum allowed.");
      }
    }
  }
}

static int compareEdgeLen(const OrderNetEdge a, const OrderNetEdge b)
{
  return a.length > b.length;
}

void FastRouteCore::netedgeOrderDec(int netID)
{
  int j, d, numTreeedges;

  d = sttrees_[netID].deg;
  numTreeedges = 2 * d - 3;

  net_eo_.clear();

  for (j = 0; j < numTreeedges; j++) {
    OrderNetEdge orderNet;
    orderNet.length = sttrees_[netID].edges[j].route.routelen;
    orderNet.edgeID = j;
    net_eo_.push_back(orderNet);
  }

  std::stable_sort(net_eo_.begin(), net_eo_.end(), compareEdgeLen);
}

void FastRouteCore::printEdge2D(int netID, int edgeID)
{
  int i;
  TreeEdge edge;
  TreeNode* nodes;

  edge = sttrees_[netID].edges[edgeID];
  nodes = sttrees_[netID].nodes;

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
    for (i = 0; i <= edge.route.routelen; i++) {
      edge_rpt = edge_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", "
                 + std::to_string(edge.route.gridsY[i]) + ") ";
    }
    logger_->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree2D(int netID)
{
  int edgeID, nodeID;
  for (nodeID = 0; nodeID < 2 * sttrees_[netID].deg - 2; nodeID++) {
    logger_->report("nodeID {},  [{}, {}]",
                   nodeID,
                   sttrees_[netID].nodes[nodeID].y,
                   sttrees_[netID].nodes[nodeID].x);
  }

  for (edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
    printEdge2D(netID, edgeID);
  }
}

bool FastRouteCore::checkRoute2DTree(int netID)
{
  bool STHwrong;
  int i, edgeID, edgelength;
  int n1, n2, x1, y1, x2, y2;
  int distance;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  STHwrong = false;

  treenodes = sttrees_[netID].nodes;
  for (edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
    treeedge = &(sttrees_[netID].edges[edgeID]);
    edgelength = treeedge->route.routelen;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;
    const std::vector<short>& gridsX = treeedge->route.gridsX;
    const std::vector<short>& gridsY = treeedge->route.gridsY;

    if (treeedge->len < 0) {
      logger_->warn(
          GRT, 207, "Ripped up edge without edge length reassignment.");
      STHwrong = true;
    }

    if (treeedge->len > 0) {
      if (treeedge->route.routelen < 1) {
        logger_->warn(GRT,
                     208,
                     "Route length {}, tree length {}.",
                     treeedge->route.routelen,
                     treeedge->len);
        STHwrong = true;
        return true;
      }

      if (gridsX[0] != x1 || gridsY[0] != y1) {
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
      for (i = 0; i < treeedge->route.routelen; i++) {
        distance
            = abs(gridsX[i + 1] - gridsX[i]) + abs(gridsY[i + 1] - gridsY[i]);
        if (distance != 1) {
          logger_->warn(GRT,
                       166,
                       "Net {} edge[{}] maze route wrong, distance {}, i {}.",
                       netName(nets_[netID]),
                       edgeID,
                       distance,
                       i);
          STHwrong = true;
        }
      }

      if (STHwrong) {
        logger_->error(
            GRT, 167, "Invalid 2D tree for net {}.", netName(nets_[netID]));
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
    for (netID = 0; netID < num_valid_nets_; netID++) {
      numEdges = 2 * sttrees_bk_[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
      delete[] sttrees_bk_[netID].nodes;
      delete[] sttrees_bk_[netID].edges;
    }
    sttrees_bk_.clear();
  }

  sttrees_bk_.resize(num_valid_nets_);

  for (netID = 0; netID < num_valid_nets_; netID++) {
    numNodes = 2 * sttrees_[netID].deg - 2;
    numEdges = 2 * sttrees_[netID].deg - 3;

    sttrees_bk_[netID].nodes = new TreeNode[numNodes];

    for (i = 0; i < numNodes; i++) {
      sttrees_bk_[netID].nodes[i].x = sttrees_[netID].nodes[i].x;
      sttrees_bk_[netID].nodes[i].y = sttrees_[netID].nodes[i].y;
      for (j = 0; j < 3; j++) {
        sttrees_bk_[netID].nodes[i].nbr[j] = sttrees_[netID].nodes[i].nbr[j];
        sttrees_bk_[netID].nodes[i].edge[j] = sttrees_[netID].nodes[i].edge[j];
      }
    }
    sttrees_bk_[netID].deg = sttrees_[netID].deg;

    sttrees_bk_[netID].edges = new TreeEdge[numEdges];

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
            sttrees_[netID].edges[edgeID].route.routelen + 1);
        sttrees_bk_[netID].edges[edgeID].route.gridsY.resize(
            sttrees_[netID].edges[edgeID].route.routelen + 1);

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
  int i, j, netID, edgeID, numEdges, numNodes, grid, min_y, min_x;

  if (!sttrees_bk_.empty()) {
    for (netID = 0; netID < num_valid_nets_; netID++) {
      numEdges = 2 * sttrees_[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_[netID].edges[edgeID].len > 0) {
          sttrees_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
      delete[] sttrees_[netID].nodes;
      delete[] sttrees_[netID].edges;
    }
    sttrees_.clear();

    sttrees_.resize(num_valid_nets_);

    for (netID = 0; netID < num_valid_nets_; netID++) {
      numNodes = 2 * sttrees_bk_[netID].deg - 2;
      numEdges = 2 * sttrees_bk_[netID].deg - 3;

      sttrees_[netID].nodes = new TreeNode[numNodes];

      for (i = 0; i < numNodes; i++) {
        sttrees_[netID].nodes[i].x = sttrees_bk_[netID].nodes[i].x;
        sttrees_[netID].nodes[i].y = sttrees_bk_[netID].nodes[i].y;
        for (j = 0; j < 3; j++) {
          sttrees_[netID].nodes[i].nbr[j] = sttrees_bk_[netID].nodes[i].nbr[j];
          sttrees_[netID].nodes[i].edge[j] = sttrees_bk_[netID].nodes[i].edge[j];
        }
      }

      sttrees_[netID].edges = new TreeEdge[numEdges];

      sttrees_[netID].deg = sttrees_bk_[netID].deg;

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        sttrees_[netID].edges[edgeID].len = sttrees_bk_[netID].edges[edgeID].len;
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
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1);
          sttrees_[netID].edges[edgeID].route.gridsY.resize(
              sttrees_bk_[netID].edges[edgeID].route.routelen + 1);

          for (i = 0; i <= sttrees_bk_[netID].edges[edgeID].route.routelen; i++) {
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
        grid = i * (x_grid_ - 1) + j;
        h_edges_[grid].usage = 0;
      }
    }
    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        v_edges_[grid].usage = 0;
      }
    }
    for (netID = 0; netID < num_valid_nets_; netID++) {
      numEdges = 2 * sttrees_[netID].deg - 3;
      int edgeCost = nets_[netID]->edgeCost;

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_[netID].edges[edgeID].len > 0) {
          const std::vector<short>& gridsX = sttrees_[netID].edges[edgeID].route.gridsX;
          const std::vector<short>& gridsY = sttrees_[netID].edges[edgeID].route.gridsY;
          for (i = 0; i < sttrees_[netID].edges[edgeID].route.routelen; i++) {
            if (gridsX[i] == gridsX[i + 1])  // a vertical edge
            {
              min_y = std::min(gridsY[i], gridsY[i + 1]);
              v_edges_[min_y * x_grid_ + gridsX[i]].usage += edgeCost;
            } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
            {
              min_x = std::min(gridsX[i], gridsX[i + 1]);
              h_edges_[gridsY[i] * (x_grid_ - 1) + min_x].usage += edgeCost;
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
    for (netID = 0; netID < num_valid_nets_; netID++) {
      numEdges = 2 * sttrees_bk_[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees_bk_[netID].edges[edgeID].len > 0) {
          sttrees_bk_[netID].edges[edgeID].route.gridsX.clear();
          sttrees_bk_[netID].edges[edgeID].route.gridsY.clear();
        }
      }
      delete[] sttrees_bk_[netID].nodes;
      delete[] sttrees_bk_[netID].edges;
    }
    sttrees_bk_.clear();
  }
}

Tree FastRouteCore::fluteToTree(stt::Tree fluteTree)
{
  Tree tree;
  tree.deg = fluteTree.deg;
  tree.totalDeg = 2 * fluteTree.deg - 2;
  tree.length = (DTYPE) fluteTree.length;
  tree.branch.resize(tree.totalDeg);
  for (int i = 0; i < tree.totalDeg; i++) {
    tree.branch[i].x = (DTYPE) fluteTree.branch[i].x;
    tree.branch[i].y = (DTYPE) fluteTree.branch[i].y;
    tree.branch[i].n = fluteTree.branch[i].n;
  }
  return tree;
}

stt::Tree FastRouteCore::treeToFlute(Tree tree)
{
  stt::Tree fluteTree;
  fluteTree.deg = tree.deg;
  fluteTree.length = (stt::DTYPE) tree.length;
  fluteTree.branch.resize(tree.totalDeg);
  for (int i = 0; i < tree.totalDeg; i++) {
    fluteTree.branch[i].x = (stt::DTYPE) tree.branch[i].x;
    fluteTree.branch[i].y = (stt::DTYPE) tree.branch[i].y;
    fluteTree.branch[i].n = tree.branch[i].n;
  }
  return fluteTree;
}

int FastRouteCore::edgeShift(Tree* t, int net)
{
  int i, j, k, l, m, deg, root, x, y, n, n1, n2, n3;
  int maxX, minX, maxY, minY, maxX1, minX1, maxY1, minY1, maxX2, minX2, maxY2,
      minY2, bigX, smallX, bigY, smallY, grid, grid1, grid2;
  int pairCnt;
  int benefit, bestBenefit, bestCost;
  int cost1, cost2, bestPair, Pos, bestPos, numShift = 0;

  // TODO: check this size
  const int sizeV = 2 * nets_[net]->numPins;
  int nbr[sizeV][3];
  int nbrCnt[sizeV];
  int pairN1[nets_[net]->numPins];
  int pairN2[nets_[net]->numPins];
  int costH[y_grid_];
  int costV[x_grid_];

  deg = t->deg;
  // find root of the tree
  for (i = deg; i < 2 * deg - 2; i++) {
    if (t->branch[i].n == i) {
      root = i;
      break;
    }
  }

  // find all neighbors for steiner nodes
  for (i = deg; i < 2 * deg - 2; i++)
    nbrCnt[i] = 0;
  // edges from pin to steiner
  for (i = 0; i < deg; i++) {
    n = t->branch[i].n;
    nbr[n][nbrCnt[n]] = i;
    nbrCnt[n]++;
  }
  // edges from steiner to steiner
  for (i = deg; i < 2 * deg - 2; i++) {
    if (i != root)  // not the removed steiner nodes and root
    {
      n = t->branch[i].n;
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
    for (i = deg; i < 2 * deg - 2; i++) {
      n = t->branch[i].n;
      if (t->branch[i].x == t->branch[n].x) {
        if (t->branch[i].y < t->branch[n].y) {
          pairN1[pairCnt] = i;
          pairN2[pairCnt] = n;
          pairCnt++;
        } else if (t->branch[i].y > t->branch[n].y) {
          pairN1[pairCnt] = n;
          pairN2[pairCnt] = i;
          pairCnt++;
        }
      } else if (t->branch[i].y == t->branch[n].y) {
        if (t->branch[i].x < t->branch[n].x) {
          pairN1[pairCnt] = i;
          pairN2[pairCnt] = n;
          pairCnt++;
        } else if (t->branch[i].x > t->branch[n].x) {
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
      if (t->branch[n1].y == t->branch[n2].y)  // a horizontal edge
      {
        // find the shifting range for the edge (minY~maxY)
        maxY1 = minY1 = t->branch[n1].y;
        for (j = 0; j < 3; j++) {
          y = t->branch[nbr[n1][j]].y;
          if (y > maxY1)
            maxY1 = y;
          else if (y < minY1)
            minY1 = y;
        }
        maxY2 = minY2 = t->branch[n2].y;
        for (j = 0; j < 3; j++) {
          y = t->branch[nbr[n2][j]].y;
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
            grid = j * (x_grid_ - 1);
            for (k = t->branch[n1].x; k < t->branch[n2].x; k++) {
              costH[j] += h_edges_[grid + k].est_usage;
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (l = 0; l < nbrCnt[n1]; l++) {
              n3 = nbr[n1][l];
              if (n3 != n2)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (t->branch[n1].x < t->branch[n3].x) {
                  smallX = t->branch[n1].x;
                  bigX = t->branch[n3].x;
                } else {
                  smallX = t->branch[n3].x;
                  bigX = t->branch[n1].x;
                }
                if (j < t->branch[n3].y) {
                  smallY = j;
                  bigY = t->branch[n3].y;
                } else {
                  smallY = t->branch[n3].y;
                  bigY = j;
                }
                grid1 = smallY * (x_grid_ - 1);
                grid2 = bigY * (x_grid_ - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[grid1 + m].est_usage;
                  cost2 += h_edges_[grid2 + m].est_usage;
                }
                grid1 = smallY * x_grid_;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[grid1 + bigX].est_usage;
                  cost2 += v_edges_[grid1 + smallX].est_usage;
                  grid1 += x_grid_;
                }
                costH[j] += std::min(cost1, cost2);
              }  // if(n3!=n2)
            }    // loop l
            for (l = 0; l < nbrCnt[n2]; l++) {
              n3 = nbr[n2][l];
              if (n3 != n1)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (t->branch[n2].x < t->branch[n3].x) {
                  smallX = t->branch[n2].x;
                  bigX = t->branch[n3].x;
                } else {
                  smallX = t->branch[n3].x;
                  bigX = t->branch[n2].x;
                }
                if (j < t->branch[n3].y) {
                  smallY = j;
                  bigY = t->branch[n3].y;
                } else {
                  smallY = t->branch[n3].y;
                  bigY = j;
                }
                grid1 = smallY * (x_grid_ - 1);
                grid2 = bigY * (x_grid_ - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[grid1 + m].est_usage;
                  cost2 += h_edges_[grid2 + m].est_usage;
                }
                grid1 = smallY * x_grid_;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[grid1 + bigX].est_usage;
                  cost2 += v_edges_[grid1 + smallX].est_usage;
                  grid1 += x_grid_;
                }
                costH[j] += std::min(cost1, cost2);
              }  // if(n3!=n1)
            }    // loop l
          }      // loop j
          bestCost = BIG_INT;
          Pos = t->branch[n1].y;
          for (j = minY; j <= maxY; j++) {
            if (costH[j] < bestCost) {
              bestCost = costH[j];
              Pos = j;
            }
          }
          if (Pos != t->branch[n1].y)  // find a better position than current
          {
            benefit = costH[t->branch[n1].y] - bestCost;
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
        maxX1 = minX1 = t->branch[n1].x;
        for (j = 0; j < 3; j++) {
          x = t->branch[nbr[n1][j]].x;
          if (x > maxX1)
            maxX1 = x;
          else if (x < minX1)
            minX1 = x;
        }
        maxX2 = minX2 = t->branch[n2].x;
        for (j = 0; j < 3; j++) {
          x = t->branch[nbr[n2][j]].x;
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
            for (k = t->branch[n1].y; k < t->branch[n2].y; k++) {
              costV[j] += v_edges_[k * x_grid_ + j].est_usage;
            }
            // add the cost of all edges adjacent to the two steiner nodes
            for (l = 0; l < nbrCnt[n1]; l++) {
              n3 = nbr[n1][l];
              if (n3 != n2)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (j < t->branch[n3].x) {
                  smallX = j;
                  bigX = t->branch[n3].x;
                } else {
                  smallX = t->branch[n3].x;
                  bigX = j;
                }
                if (t->branch[n1].y < t->branch[n3].y) {
                  smallY = t->branch[n1].y;
                  bigY = t->branch[n3].y;
                } else {
                  smallY = t->branch[n3].y;
                  bigY = t->branch[n1].y;
                }
                grid1 = smallY * (x_grid_ - 1);
                grid2 = bigY * (x_grid_ - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[grid1 + m].est_usage;
                  cost2 += h_edges_[grid2 + m].est_usage;
                }
                grid1 = smallY * x_grid_;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[grid1 + bigX].est_usage;
                  cost2 += v_edges_[grid1 + smallX].est_usage;
                  grid1 += x_grid_;
                }
                costV[j] += std::min(cost1, cost2);
              }  // if(n3!=n2)
            }    // loop l
            for (l = 0; l < nbrCnt[n2]; l++) {
              n3 = nbr[n2][l];
              if (n3 != n1)  // exclude current edge n1-n2
              {
                cost1 = cost2 = 0;
                if (j < t->branch[n3].x) {
                  smallX = j;
                  bigX = t->branch[n3].x;
                } else {
                  smallX = t->branch[n3].x;
                  bigX = j;
                }
                if (t->branch[n2].y < t->branch[n3].y) {
                  smallY = t->branch[n2].y;
                  bigY = t->branch[n3].y;
                } else {
                  smallY = t->branch[n3].y;
                  bigY = t->branch[n2].y;
                }
                grid1 = smallY * (x_grid_ - 1);
                grid2 = bigY * (x_grid_ - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges_[grid1 + m].est_usage;
                  cost2 += h_edges_[grid2 + m].est_usage;
                }
                grid1 = smallY * x_grid_;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges_[grid1 + bigX].est_usage;
                  cost2 += v_edges_[grid1 + smallX].est_usage;
                  grid1 += x_grid_;
                }
                costV[j] += std::min(cost1, cost2);
              }  // if(n3!=n1)
            }    // loop l
          }      // loop j
          bestCost = BIG_INT;
          Pos = t->branch[n1].x;
          for (j = minX; j <= maxX; j++) {
            if (costV[j] < bestCost) {
              bestCost = costV[j];
              Pos = j;
            }
          }
          if (Pos != t->branch[n1].x)  // find a better position than current
          {
            benefit = costV[t->branch[n1].x] - bestCost;
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

      if (t->branch[n1].y == t->branch[n2].y)  // horizontal edge
      {
        t->branch[n1].y = bestPos;
        t->branch[n2].y = bestPos;
      }  // vertical edge
      else {
        t->branch[n1].x = bestPos;
        t->branch[n2].x = bestPos;
      }
      numShift++;
    }
  }  // while(bestBenefit>0)

  return (numShift);
}

// exchange Steiner nodes at the same position, then call edgeShift()
int FastRouteCore::edgeShiftNew(Tree* t, int net)
{
  int i, j, n;
  int deg, pairCnt, cur_pairN1, cur_pairN2;
  int N1nbrH, N1nbrV, N2nbrH, N2nbrV, iter;
  int numShift;
  bool isPair;

  numShift = edgeShift(t, net);
  deg = t->deg;

  const int sizeV = nets_[net]->numPins;
  int pairN1[sizeV];
  int pairN2[sizeV];

  iter = 0;
  cur_pairN1 = cur_pairN2 = -1;
  while (iter < 3) {
    iter++;

    // find all pairs of steiner node at the same position (steiner pairs)
    pairCnt = 0;
    for (i = deg; i < 2 * deg - 2; i++) {
      n = t->branch[i].n;
      if (n != i && n != t->branch[n].n && t->branch[i].x == t->branch[n].x
          && t->branch[i].y == t->branch[n].y) {
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
        for (j = 0; j < 2 * deg - 2; j++) {
          n = t->branch[j].n;
          if (n == cur_pairN1) {
            if (t->branch[j].x == t->branch[cur_pairN1].x
                && t->branch[j].y != t->branch[cur_pairN1].y)
              N1nbrV = j;
            else if (t->branch[j].y == t->branch[cur_pairN1].y
                     && t->branch[j].x != t->branch[cur_pairN1].x)
              N1nbrH = j;
          } else if (n == cur_pairN2) {
            if (t->branch[j].x == t->branch[cur_pairN2].x
                && t->branch[j].y != t->branch[cur_pairN2].y)
              N2nbrV = j;
            else if (t->branch[j].y == t->branch[cur_pairN2].y
                     && t->branch[j].x != t->branch[cur_pairN2].x)
              N2nbrH = j;
          }
        }
        // find the node cur_pairN2 directed to
        n = t->branch[cur_pairN2].n;
        if (t->branch[n].x == t->branch[cur_pairN2].x
            && t->branch[n].y != t->branch[cur_pairN2].y)
          N2nbrV = n;
        else if (t->branch[n].y == t->branch[cur_pairN2].y
                 && t->branch[n].x != t->branch[cur_pairN2].x)
          N2nbrH = n;

        if (N1nbrH >= 0 && N2nbrH >= 0) {
          if (N2nbrH == t->branch[cur_pairN2].n) {
            t->branch[N1nbrH].n = cur_pairN2;
            t->branch[cur_pairN1].n = N2nbrH;
            t->branch[cur_pairN2].n = cur_pairN1;
          } else {
            t->branch[N1nbrH].n = cur_pairN2;
            t->branch[N2nbrH].n = cur_pairN1;
          }
          numShift += edgeShift(t, net);
        } else if (N1nbrV >= 0 && N2nbrV >= 0) {
          if (N2nbrV == t->branch[cur_pairN2].n) {
            t->branch[N1nbrV].n = cur_pairN2;
            t->branch[cur_pairN1].n = N2nbrV;
            t->branch[cur_pairN2].n = cur_pairN1;
          } else {
            t->branch[N1nbrV].n = cur_pairN2;
            t->branch[N2nbrV].n = cur_pairN1;
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

}  // namespace grt
