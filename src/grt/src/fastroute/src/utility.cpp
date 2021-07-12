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
#include "flute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

void FastRouteCore::printEdge(int netID, int edgeID)
{
  int i;
  TreeEdge edge;
  TreeNode* nodes;

  edge = sttrees[netID].edges[edgeID];
  nodes = sttrees[netID].nodes;

  logger->report("edge {}: ({}, {})->({}, {})",
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
  logger->report("{}", routes_rpt);
}

void FastRouteCore::ConvertToFull3DType2()
{
  short *gridsX, *gridsY, *gridsL, tmpX[MAXLEN], tmpY[MAXLEN], tmpL[MAXLEN];
  int k, netID, edgeID, routeLen;
  int newCNT, deg, j;
  TreeEdge *treeedges, *treeedge;

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        newCNT = 0;
        routeLen = treeedge->route.routelen;
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        gridsL = treeedge->route.gridsL;
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
          free(treeedges[edgeID].route.gridsX);
          free(treeedges[edgeID].route.gridsY);
          free(treeedges[edgeID].route.gridsL);
        }
        treeedge->route.gridsX = (short*) calloc(newCNT, sizeof(short));
        treeedge->route.gridsY = (short*) calloc(newCNT, sizeof(short));
        treeedge->route.gridsL = (short*) calloc(newCNT, sizeof(short));
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
  for (j = 0; j < numValidNets; j++) {
    d = sttrees[j].deg;
    numTreeedges += 2 * d - 3;
  }

  treeOrderPV.clear();

  treeOrderPV.resize(numValidNets);

  for (j = 0; j < numValidNets; j++) {
    xmin = BIG_INT;
    totalLength = 0;
    treenodes = sttrees[j].nodes;
    stree = &(sttrees[j]);
    d = stree->deg;
    for (ind = 0; ind < 2 * d - 3; ind++) {
      totalLength += stree->edges[ind].len;
      if (xmin < treenodes[stree->edges[ind].n1].x) {
        xmin = treenodes[stree->edges[ind].n1].x;
      }
    }

    npvalue = (float) totalLength / d;

    treeOrderPV[j].npv = npvalue;
    treeOrderPV[j].treeIndex = j;
    treeOrderPV[j].minX = xmin;
  }

  std::stable_sort(treeOrderPV.begin(), treeOrderPV.end(), comparePVMINX);
  std::stable_sort(treeOrderPV.begin(), treeOrderPV.end(), comparePVPV);
}

void FastRouteCore::fillVIA()
{
  short tmpX[MAXLEN], tmpY[MAXLEN], *gridsX, *gridsY, *gridsL, tmpL[MAXLEN];
  int k, netID, edgeID, routeLen, n1a, n2a;
  int newCNT, numVIAT1, numVIAT2, deg, j;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  numVIAT1 = 0;
  numVIAT2 = 0;

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;
    treenodes = sttrees[netID].nodes;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        newCNT = 0;
        routeLen = treeedge->route.routelen;
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        gridsL = treeedge->route.gridsL;

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
            free(treeedges[edgeID].route.gridsX);
            free(treeedges[edgeID].route.gridsY);
            free(treeedges[edgeID].route.gridsL);
          }
          treeedge->route.gridsX = (short*) calloc(newCNT, sizeof(short));
          treeedge->route.gridsY = (short*) calloc(newCNT, sizeof(short));
          treeedge->route.gridsL = (short*) calloc(newCNT, sizeof(short));
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

  if (verbose > 1) {
    logger->info(GRT, 197, "Via related to pin nodes: {}", numVIAT1);
    logger->info(GRT, 198, "Via related Steiner nodes: {}", numVIAT2);
    logger->info(GRT, 199, "Via filling finished.");
  }
}

int FastRouteCore::threeDVIA()
{
  short* gridsL;
  int netID, edgeID, deg;
  int routeLen, numVIA, j;
  TreeEdge *treeedges, *treeedge;

  numVIA = 0;

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);

      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        gridsL = treeedge->route.gridsL;

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
  short *gridsX, *gridsY, *gridsL;
  std::vector<std::vector<int>> gridD;
  int i, k, l, grid, min_x, min_y, routelen, n1a, n2a, last_layer;
  int min_result, endLayer;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees[netID].edges;
  treenodes = sttrees[netID].nodes;
  treeedge = &(treeedges[edgeID]);

  gridsX = treeedge->route.gridsX;
  gridsY = treeedge->route.gridsY;
  gridsL = treeedge->route.gridsL;

  routelen = treeedge->route.routelen;
  n1a = treeedge->n1a;
  n2a = treeedge->n2a;

  gridD.resize(numLayers);
  for (int i = 0; i < numLayers; i++) {
    gridD[i].resize(treeedge->route.routelen + 1);
  }

  for (l = 0; l < numLayers; l++) {
    for (k = 0; k <= routelen; k++) {
      gridD[l][k] = BIG_INT;
      viaLink[l][k] = BIG_INT;
    }
  }

  for (k = 0; k < routelen; k++) {
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      for (l = 0; l < numLayers; l++) {
        grid = l * gridV + min_y * xGrid + gridsX[k];
        layerGrid[l][k] = v_edges3D[grid].cap - v_edges3D[grid].usage;
      }
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      for (l = 0; l < numLayers; l++) {
        grid = l * gridH + gridsY[k] * (xGrid - 1) + min_x;
        layerGrid[l][k] = h_edges3D[grid].cap - h_edges3D[grid].usage;
      }
    }
  }

  if (processDIR) {
    if (treenodes[n1a].assigned) {
      for (l = treenodes[n1a].botL; l <= treenodes[n1a].topL; l++) {
        gridD[l][0] = 0;
      }
    } else {
      logger->warn(GRT, 200, "Start point not assigned.");
      fflush(stdout);
    }

    for (k = 0; k < routelen; k++) {
      for (l = 0; l < numLayers; l++) {
        for (i = 0; i < numLayers; i++) {
          if (k == 0) {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
                viaLink[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
                viaLink[i][k] = l;
              }
            }
          }
        }
      }
      for (l = 0; l < numLayers; l++) {
        if (layerGrid[l][k] > 0) {
          gridD[l][k + 1] = gridD[l][k] + 1;
        } else {
          gridD[l][k + 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (l = 0; l < numLayers; l++) {
      for (i = 0; i < numLayers; i++) {
        if (l != i) {
          if (gridD[i][k]
              > gridD[l][k] + abs(i - l) * 1) {
            gridD[i][k] = gridD[l][k] + abs(i - l) * 1;
            viaLink[i][k] = l;
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
      for (i = 0; i < numLayers; i++) {
        if (gridD[i][routelen] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    }

    if (viaLink[endLayer][routelen] == BIG_INT) {
      last_layer = endLayer;
    } else {
      last_layer = viaLink[endLayer][routelen];
    }

    for (k = routelen; k >= 0; k--) {
      gridsL[k] = last_layer;
      if (viaLink[last_layer][k] != BIG_INT) {
        last_layer = viaLink[last_layer][k];
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
        logger->error(GRT,
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
      for (l = 0; l < numLayers; l++) {
        for (i = 0; i < numLayers; i++) {
          if (k == routelen) {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 2) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 2;
                viaLink[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + abs(i - l) * 3) {
                gridD[i][k] = gridD[l][k] + abs(i - l) * 3;
                viaLink[i][k] = l;
              }
            }
          }
        }
      }
      for (l = 0; l < numLayers; l++) {
        if (layerGrid[l][k - 1] > 0) {
          gridD[l][k - 1] = gridD[l][k] + 1;
        } else {
          gridD[l][k - 1] = gridD[l][k] + BIG_INT;
        }
      }
    }

    for (l = 0; l < numLayers; l++) {
      for (i = 0; i < numLayers; i++) {
        if (l != i) {
          if (gridD[i][0] > gridD[l][0] + abs(i - l) * 1) {
            gridD[i][0] = gridD[l][0] + abs(i - l) * 1;
            viaLink[i][0] = l;
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
      for (i = 0; i < numLayers; i++) {
        if (gridD[i][k] < min_result || (min_result == BIG_INT)) {
          min_result = gridD[i][k];
          endLayer = i;
        }
      }
    }

    last_layer = endLayer;

    for (k = 0; k <= routelen; k++) {
      if (viaLink[last_layer][k] != BIG_INT) {
        last_layer = viaLink[last_layer][k];
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

  std::vector<int> edge_cost_per_layer = nets[netID]->edge_cost_per_layer;

  for (k = 0; k < routelen; k++) {
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      grid = gridsL[k] * gridV + min_y * xGrid + gridsX[k];

      v_edges3D[grid].usage += edge_cost_per_layer[gridsL[k]];
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      grid = gridsL[k] * gridH + gridsY[k] * (xGrid - 1) + min_x;

      h_edges3D[grid].usage += edge_cost_per_layer[gridsL[k]];
    }
  }
}

void FastRouteCore::newLayerAssignmentV4()
{
  short* gridsL;
  int i, k, netID, edgeID, nodeID, routeLen;
  int n1, n2, connectionCNT, deg;

  int n1a, n2a;
  std::queue<int> edgeQueue;

  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;
    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        treeedge->route.gridsL = (short*) calloc(routeLen + 1, sizeof(short));
        treeedge->assigned = false;
      }
    }
  }
  netpinOrderInc();

  for (i = 0; i < numValidNets; i++) {
    netID = treeOrderPV[i].treeIndex;
    treeedges = sttrees[netID].edges;
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;

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

    deg = sttrees[netID].deg;

    for (nodeID = 0; nodeID < 2 * deg - 2; nodeID++) {
      treenodes[nodeID].topL = -1;
      treenodes[nodeID].botL = numLayers;
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
        gridsL = treeedge->route.gridsL;

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

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;

    numpoints = 0;

    for (d = 0; d < 2 * deg - 2; d++) {
      treenodes[d].topL = -1;
      treenodes[d].botL = numLayers;
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

        xcor[numpoints] = treenodes[d].x;
        ycor[numpoints] = treenodes[d].y;
        dcor[numpoints] = d;
        numpoints++;
      } else {
        redundant = false;
        for (k = 0; k < numpoints; k++) {
          if ((treenodes[d].x == xcor[k]) && (treenodes[d].y == ycor[k])) {
            treenodes[d].stackAlias = dcor[k];

            redundant = true;
            break;
          }
        }
        if (!redundant) {
          xcor[numpoints] = treenodes[d].x;
          ycor[numpoints] = treenodes[d].y;
          dcor[numpoints] = d;
          numpoints++;
        }
      }
    }
  }

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;

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

  edge = sttrees[netID].edges[edgeID];
  nodes = sttrees[netID].nodes;

  logger->report("edge {}: n1 {} ({}, {})-> n2 {}({}, {})",
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
    logger->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree3D(int netID)
{
  int edgeID, nodeID;
  for (nodeID = 0; nodeID < 2 * sttrees[netID].deg - 2; nodeID++) {
    logger->report("nodeID {},  [{}, {}]",
                   nodeID,
                   sttrees[netID].nodes[nodeID].y,
                   sttrees[netID].nodes[nodeID].x);
  }

  for (edgeID = 0; edgeID < 2 * sttrees[netID].deg - 3; edgeID++) {
    printEdge3D(netID, edgeID);
  }
}

void FastRouteCore::checkRoute3D()
{
  short *gridsX, *gridsY, *gridsL;
  int i, netID, edgeID, nodeID, edgelength;
  int n1, n2, x1, y1, x2, y2, deg;
  int distance;
  bool gridFlag;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < numValidNets; netID++) {
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;

    for (nodeID = 0; nodeID < 2 * deg - 2; nodeID++) {
      if (nodeID < deg) {
        if (treenodes[nodeID].botL != 0) {
          logger->error(GRT, 203, "Caused floating pin node.");
        }
      }
    }
    for (edgeID = 0; edgeID < 2 * sttrees[netID].deg - 3; edgeID++) {
      if (sttrees[netID].edges[edgeID].len == 0) {
        continue;
      }
      treeedge = &(sttrees[netID].edges[edgeID]);
      edgelength = treeedge->route.routelen;
      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;
      gridsX = treeedge->route.gridsX;
      gridsY = treeedge->route.gridsY;
      gridsL = treeedge->route.gridsL;

      gridFlag = false;

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        debugPrint(logger,
                   GRT,
                   "checkRoute3D",
                   1,
                   "net {} edge[{}] start node wrong, net deg {}, n1 {}",
                   netName(nets[netID]),
                   edgeID,
                   deg,
                   n1);
        if (logger->debugCheck(GRT, "checkRoute3D", 1)) {
          printEdge3D(netID, edgeID);
        }
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        debugPrint(logger,
                   GRT,
                   "checkRoute3D",
                   1,
                   "net {} edge[{}] end node wrong, net deg {}, n2 {}",
                   netName(nets[netID]),
                   edgeID,
                   deg,
                   n2);
        if (logger->debugCheck(GRT, "checkRoute3D", 1)) {
          printEdge3D(netID, edgeID);
        }
      }
      for (i = 0; i < treeedge->route.routelen; i++) {
        distance = abs(gridsX[i + 1] - gridsX[i])
                   + abs(gridsY[i + 1] - gridsY[i])
                   + abs(gridsL[i + 1] - gridsL[i]);
        if (distance > 1 || distance < 0) {
          gridFlag = true;
          debugPrint(logger,
                     GRT,
                     "checkRoute3D",
                     1,
                     "net {} edge[{}] maze route wrong, distance {}, i {}",
                     netName(nets[netID]),
                     edgeID,
                     distance,
                     i);
          debugPrint(logger,
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
          logger->error(
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
  short *gridsX, *gridsY;
  int i, j, d, ind, grid, min_x, min_y;
  TreeEdge *treeedges, *treeedge;
  StTree* stree;

  int numTreeedges = 0;

  treeOrderCong.clear();

  treeOrderCong.resize(numValidNets);

  i = 0;
  for (j = 0; j < numValidNets; j++) {
    stree = &(sttrees[j]);
    d = stree->deg;
    treeOrderCong[j].xmin = 0;
    treeOrderCong[j].treeIndex = j;
    for (ind = 0; ind < 2 * d - 3; ind++) {
      treeedges = stree->edges;
      treeedge = &(treeedges[ind]);

      gridsX = treeedge->route.gridsX;
      gridsY = treeedge->route.gridsY;
      for (i = 0; i < treeedge->route.routelen; i++) {
        if (gridsX[i] == gridsX[i + 1])  // a vertical edge
        {
          min_y = std::min(gridsY[i], gridsY[i + 1]);
          grid = min_y * xGrid + gridsX[i];
          treeOrderCong[j].xmin
              += std::max(0, v_edges[grid].usage - v_edges[grid].cap);
        } else  // a horizontal edge
        {
          min_x = std::min(gridsX[i], gridsX[i + 1]);
          grid = gridsY[i] * (xGrid - 1) + min_x;
          treeOrderCong[j].xmin
              += std::max(0, h_edges[grid].usage - h_edges[grid].cap);
        }
      }
    }
  }

  std::stable_sort(treeOrderCong.begin(), treeOrderCong.end(), compareTEL);
}

void FastRouteCore::recoverEdge(int netID, int edgeID)
{
  short *gridsX, *gridsY, *gridsL;
  int i, grid, ymin, xmin, n1a, n2a;
  int connectionCNT, routeLen;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees[netID].edges;
  treeedge = &(treeedges[edgeID]);

  routeLen = treeedge->route.routelen;

  if (treeedge->len == 0) {
    logger->error(GRT, 206, "Trying to recover a 0-length edge.");
  }

  treenodes = sttrees[netID].nodes;

  gridsX = treeedge->route.gridsX;
  gridsY = treeedge->route.gridsY;
  gridsL = treeedge->route.gridsL;

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

  std::vector<int> edge_cost_per_layer = nets[netID]->edge_cost_per_layer;

  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = gridsL[i] * gridV + ymin * xGrid + gridsX[i];
        v_edges3D[grid].usage += edge_cost_per_layer[gridsL[i]];
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsL[i] * gridH + gridsY[i] * (xGrid - 1) + xmin;
        h_edges3D[grid].usage += edge_cost_per_layer[gridsL[i]];
      }
    }
  }
}

void FastRouteCore::checkUsage()
{
  short *gridsX, *gridsY;
  int netID, i, k, edgeID, deg;
  int j, cnt;
  bool redsus;
  TreeEdge *treeedges, *treeedge;
  TreeEdge edge;

  for (netID = 0; netID < numValidNets; netID++) {
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;

    int edgeCost = nets[netID]->edgeCost;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      edge = sttrees[netID].edges[edgeID];
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;

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
                    int grid = min_y * xGrid + gridsX[k];
                    v_edges[grid].usage -= edgeCost;
                  } else {
                    int min_x = std::min(gridsX[k], gridsX[k + 1]);
                    int grid = gridsY[k] * (xGrid - 1) + min_x;
                    h_edges[grid].usage -= edgeCost;
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
  if (verbose > 1) {
    logger->report("Usage checked");
  }
}

void FastRouteCore::check2DEdgesUsage()
{
  const int max_usage_multiplier = 40;
  int max_h_edge_usage = max_usage_multiplier * hCapacity;
  int max_v_edge_usage = max_usage_multiplier * vCapacity;

  // check horizontal edges
  for (int i = 0; i < yGrid; i++) {
    for (int j = 0; j < xGrid - 1; j++) {
      int grid = i * (xGrid - 1) + j;
      if (h_edges[grid].usage >= max_h_edge_usage) {
        logger->error(
            GRT, 228, "Horizontal edge usage exceeds the maximum allowed.");
      }
    }
  }

  // check vertical edges
  for (int i = 0; i < yGrid - 1; i++) {
    for (int j = 0; j < xGrid; j++) {
      int grid = i * xGrid + j;
      if (v_edges[grid].usage >= max_v_edge_usage) {
        logger->error(
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

  d = sttrees[netID].deg;
  numTreeedges = 2 * d - 3;

  netEO.clear();

  for (j = 0; j < numTreeedges; j++) {
    OrderNetEdge orderNet;
    orderNet.length = sttrees[netID].edges[j].route.routelen;
    orderNet.edgeID = j;
    netEO.push_back(orderNet);
  }

  std::stable_sort(netEO.begin(), netEO.end(), compareEdgeLen);
}

void FastRouteCore::printEdge2D(int netID, int edgeID)
{
  int i;
  TreeEdge edge;
  TreeNode* nodes;

  edge = sttrees[netID].edges[edgeID];
  nodes = sttrees[netID].nodes;

  logger->report("edge {}: n1 {} ({}, {})-> n2 {}({}, {}), routeType {}",
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
    logger->report("{}", edge_rpt);
  }
}

void FastRouteCore::printTree2D(int netID)
{
  int edgeID, nodeID;
  for (nodeID = 0; nodeID < 2 * sttrees[netID].deg - 2; nodeID++) {
    logger->report("nodeID {},  [{}, {}]",
                   nodeID,
                   sttrees[netID].nodes[nodeID].y,
                   sttrees[netID].nodes[nodeID].x);
  }

  for (edgeID = 0; edgeID < 2 * sttrees[netID].deg - 3; edgeID++) {
    printEdge2D(netID, edgeID);
  }
}

bool FastRouteCore::checkRoute2DTree(int netID)
{
  bool STHwrong;
  short *gridsX, *gridsY;
  int i, edgeID, edgelength;
  int n1, n2, x1, y1, x2, y2;
  int distance;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  STHwrong = false;

  treenodes = sttrees[netID].nodes;
  for (edgeID = 0; edgeID < 2 * sttrees[netID].deg - 3; edgeID++) {
    treeedge = &(sttrees[netID].edges[edgeID]);
    edgelength = treeedge->route.routelen;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;
    gridsX = treeedge->route.gridsX;
    gridsY = treeedge->route.gridsY;

    if (treeedge->len < 0) {
      logger->warn(
          GRT, 207, "Ripped up edge without edge length reassignment.");
      STHwrong = true;
    }

    if (treeedge->len > 0) {
      if (treeedge->route.routelen < 1) {
        logger->warn(GRT,
                     208,
                     "Route length {}, tree length {}.",
                     treeedge->route.routelen,
                     treeedge->len);
        STHwrong = true;
        return true;
      }

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        logger->warn(
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
        logger->warn(
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
          logger->warn(GRT,
                       166,
                       "Net {} edge[{}] maze route wrong, distance {}, i {}.",
                       netName(nets[netID]),
                       edgeID,
                       distance,
                       i);
          STHwrong = true;
        }
      }

      if (STHwrong) {
        logger->error(
            GRT, 167, "Invalid 2D tree for net {}.", netName(nets[netID]));
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

  if (sttreesBK != NULL) {
    for (netID = 0; netID < numValidNets; netID++) {
      numEdges = 2 * sttreesBK[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttreesBK[netID].edges[edgeID].len > 0) {
          delete[] sttreesBK[netID].edges[edgeID].route.gridsX;
          delete[] sttreesBK[netID].edges[edgeID].route.gridsY;
        }
      }
      delete[] sttreesBK[netID].nodes;
      delete[] sttreesBK[netID].edges;
    }
    delete[] sttreesBK;
  }

  sttreesBK = new StTree[numValidNets];

  for (netID = 0; netID < numValidNets; netID++) {
    numNodes = 2 * sttrees[netID].deg - 2;
    numEdges = 2 * sttrees[netID].deg - 3;

    sttreesBK[netID].nodes = new TreeNode[numNodes];

    for (i = 0; i < numNodes; i++) {
      sttreesBK[netID].nodes[i].x = sttrees[netID].nodes[i].x;
      sttreesBK[netID].nodes[i].y = sttrees[netID].nodes[i].y;
      for (j = 0; j < 3; j++) {
        sttreesBK[netID].nodes[i].nbr[j] = sttrees[netID].nodes[i].nbr[j];
        sttreesBK[netID].nodes[i].edge[j] = sttrees[netID].nodes[i].edge[j];
      }
    }
    sttreesBK[netID].deg = sttrees[netID].deg;

    sttreesBK[netID].edges = new TreeEdge[numEdges];

    for (edgeID = 0; edgeID < numEdges; edgeID++) {
      sttreesBK[netID].edges[edgeID].len = sttrees[netID].edges[edgeID].len;
      sttreesBK[netID].edges[edgeID].n1 = sttrees[netID].edges[edgeID].n1;
      sttreesBK[netID].edges[edgeID].n2 = sttrees[netID].edges[edgeID].n2;

      if (sttrees[netID].edges[edgeID].len
          > 0)  // only route the non-degraded edges (len>0)
      {
        sttreesBK[netID].edges[edgeID].route.routelen
            = sttrees[netID].edges[edgeID].route.routelen;
        sttreesBK[netID].edges[edgeID].route.gridsX
            = new short[(sttrees[netID].edges[edgeID].route.routelen + 1)];
        sttreesBK[netID].edges[edgeID].route.gridsY
            = new short[(sttrees[netID].edges[edgeID].route.routelen + 1)];

        for (i = 0; i <= sttrees[netID].edges[edgeID].route.routelen; i++) {
          sttreesBK[netID].edges[edgeID].route.gridsX[i]
              = sttrees[netID].edges[edgeID].route.gridsX[i];
          sttreesBK[netID].edges[edgeID].route.gridsY[i]
              = sttrees[netID].edges[edgeID].route.gridsY[i];
        }
      }
    }
  }
}

void FastRouteCore::copyBR(void)
{
  short *gridsX, *gridsY;
  int i, j, netID, edgeID, numEdges, numNodes, grid, min_y, min_x;

  if (sttreesBK != NULL) {
    for (netID = 0; netID < numValidNets; netID++) {
      numEdges = 2 * sttrees[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees[netID].edges[edgeID].len > 0) {
          delete[] sttrees[netID].edges[edgeID].route.gridsX;
          delete[] sttrees[netID].edges[edgeID].route.gridsY;
        }
      }
      delete[] sttrees[netID].nodes;
      delete[] sttrees[netID].edges;
    }
    delete[] sttrees;

    sttrees = new StTree[numValidNets];

    for (netID = 0; netID < numValidNets; netID++) {
      numNodes = 2 * sttreesBK[netID].deg - 2;
      numEdges = 2 * sttreesBK[netID].deg - 3;

      sttrees[netID].nodes = new TreeNode[numNodes];

      for (i = 0; i < numNodes; i++) {
        sttrees[netID].nodes[i].x = sttreesBK[netID].nodes[i].x;
        sttrees[netID].nodes[i].y = sttreesBK[netID].nodes[i].y;
        for (j = 0; j < 3; j++) {
          sttrees[netID].nodes[i].nbr[j] = sttreesBK[netID].nodes[i].nbr[j];
          sttrees[netID].nodes[i].edge[j] = sttreesBK[netID].nodes[i].edge[j];
        }
      }

      sttrees[netID].edges = new TreeEdge[numEdges];

      sttrees[netID].deg = sttreesBK[netID].deg;

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        sttrees[netID].edges[edgeID].len = sttreesBK[netID].edges[edgeID].len;
        sttrees[netID].edges[edgeID].n1 = sttreesBK[netID].edges[edgeID].n1;
        sttrees[netID].edges[edgeID].n2 = sttreesBK[netID].edges[edgeID].n2;

        sttrees[netID].edges[edgeID].route.type = RouteType::MazeRoute;
        sttrees[netID].edges[edgeID].route.routelen
            = sttreesBK[netID].edges[edgeID].route.routelen;

        if (sttreesBK[netID].edges[edgeID].len
            > 0)  // only route the non-degraded edges (len>0)
        {
          sttrees[netID].edges[edgeID].route.type = RouteType::MazeRoute;
          sttrees[netID].edges[edgeID].route.routelen
              = sttreesBK[netID].edges[edgeID].route.routelen;
          sttrees[netID].edges[edgeID].route.gridsX
              = new short[(sttreesBK[netID].edges[edgeID].route.routelen + 1)];
          sttrees[netID].edges[edgeID].route.gridsY
              = new short[(sttreesBK[netID].edges[edgeID].route.routelen + 1)];

          for (i = 0; i <= sttreesBK[netID].edges[edgeID].route.routelen; i++) {
            sttrees[netID].edges[edgeID].route.gridsX[i]
                = sttreesBK[netID].edges[edgeID].route.gridsX[i];
            sttrees[netID].edges[edgeID].route.gridsY[i]
                = sttreesBK[netID].edges[edgeID].route.gridsY[i];
          }
        }
      }
    }

    for (i = 0; i < yGrid; i++) {
      for (j = 0; j < xGrid - 1; j++) {
        grid = i * (xGrid - 1) + j;
        h_edges[grid].usage = 0;
      }
    }
    for (i = 0; i < yGrid - 1; i++) {
      for (j = 0; j < xGrid; j++) {
        grid = i * xGrid + j;
        v_edges[grid].usage = 0;
      }
    }
    for (netID = 0; netID < numValidNets; netID++) {
      numEdges = 2 * sttrees[netID].deg - 3;
      int edgeCost = nets[netID]->edgeCost;

      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttrees[netID].edges[edgeID].len > 0) {
          gridsX = sttrees[netID].edges[edgeID].route.gridsX;
          gridsY = sttrees[netID].edges[edgeID].route.gridsY;
          for (i = 0; i < sttrees[netID].edges[edgeID].route.routelen; i++) {
            if (gridsX[i] == gridsX[i + 1])  // a vertical edge
            {
              min_y = std::min(gridsY[i], gridsY[i + 1]);
              v_edges[min_y * xGrid + gridsX[i]].usage += edgeCost;
            } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
            {
              min_x = std::min(gridsX[i], gridsX[i + 1]);
              h_edges[gridsY[i] * (xGrid - 1) + min_x].usage += edgeCost;
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
  if (sttreesBK != NULL) {
    for (netID = 0; netID < numValidNets; netID++) {
      numEdges = 2 * sttreesBK[netID].deg - 3;
      for (edgeID = 0; edgeID < numEdges; edgeID++) {
        if (sttreesBK[netID].edges[edgeID].len > 0) {
          delete[] sttreesBK[netID].edges[edgeID].route.gridsX;
          delete[] sttreesBK[netID].edges[edgeID].route.gridsY;
        }
      }
      delete[] sttreesBK[netID].nodes;
      delete[] sttreesBK[netID].edges;
    }
    delete[] sttreesBK;
  }
}

Tree FastRouteCore::fluteToTree(stt::Tree fluteTree)
{
  Tree tree;
  tree.deg = fluteTree.deg;
  tree.totalDeg = 2 * fluteTree.deg - 2;
  tree.length = (DTYPE) fluteTree.length;
  tree.branch = new Branch[tree.totalDeg];
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
  fluteTree.branch = new stt::Branch[tree.totalDeg];
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
  const int sizeV = 2 * nets[net]->numPins;
  int nbr[sizeV][3];
  int nbrCnt[sizeV];
  int pairN1[nets[net]->numPins];
  int pairN2[nets[net]->numPins];
  int costH[yGrid];
  int costV[xGrid];

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
            grid = j * (xGrid - 1);
            for (k = t->branch[n1].x; k < t->branch[n2].x; k++) {
              costH[j] += h_edges[grid + k].est_usage;
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
                grid1 = smallY * (xGrid - 1);
                grid2 = bigY * (xGrid - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges[grid1 + m].est_usage;
                  cost2 += h_edges[grid2 + m].est_usage;
                }
                grid1 = smallY * xGrid;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges[grid1 + bigX].est_usage;
                  cost2 += v_edges[grid1 + smallX].est_usage;
                  grid1 += xGrid;
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
                grid1 = smallY * (xGrid - 1);
                grid2 = bigY * (xGrid - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges[grid1 + m].est_usage;
                  cost2 += h_edges[grid2 + m].est_usage;
                }
                grid1 = smallY * xGrid;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges[grid1 + bigX].est_usage;
                  cost2 += v_edges[grid1 + smallX].est_usage;
                  grid1 += xGrid;
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
              costV[j] += v_edges[k * xGrid + j].est_usage;
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
                grid1 = smallY * (xGrid - 1);
                grid2 = bigY * (xGrid - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges[grid1 + m].est_usage;
                  cost2 += h_edges[grid2 + m].est_usage;
                }
                grid1 = smallY * xGrid;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges[grid1 + bigX].est_usage;
                  cost2 += v_edges[grid1 + smallX].est_usage;
                  grid1 += xGrid;
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
                grid1 = smallY * (xGrid - 1);
                grid2 = bigY * (xGrid - 1);
                for (m = smallX; m < bigX; m++) {
                  cost1 += h_edges[grid1 + m].est_usage;
                  cost2 += h_edges[grid2 + m].est_usage;
                }
                grid1 = smallY * xGrid;
                for (m = smallY; m < bigY; m++) {
                  cost1 += v_edges[grid1 + bigX].est_usage;
                  cost2 += v_edges[grid1 + smallX].est_usage;
                  grid1 += xGrid;
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

  const int sizeV = nets[net]->numPins;
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
