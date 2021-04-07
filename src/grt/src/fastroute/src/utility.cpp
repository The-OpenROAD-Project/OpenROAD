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

#include "DataProc.h"
#include "DataType.h"
#include "flute.h"
#include "pdrev/pdrev.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

void printEdge(int netID, int edgeID)
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
    routes_rpt = routes_rpt + "(" + std::to_string(edge.route.gridsX[i]) +
                 ", " + std::to_string(edge.route.gridsY[i]) + ") ";
  }
  logger->report("{}", routes_rpt);
}

void plotTree(int netID)
{
  short *gridsX, *gridsY;
  int i, j, Zpoint, n1, n2, x1, x2, y1, y2, ymin, ymax, xmin, xmax;

  RouteType routetype;
  TreeEdge* treeedge;
  TreeNode* treenodes;
  FILE* fp;

  xmin = ymin = 1e5;
  xmax = ymax = 0;

  fp = fopen("plottree", "w");
  if (fp == NULL) {
    logger->error(GRT, 194, "Fail when open file plottree.");
  }

  treenodes = sttrees[netID].nodes;
  for (i = 0; i < sttrees[netID].deg; i++) {
    x1 = treenodes[i].x;
    y1 = treenodes[i].y;
    fprintf(fp, "%f %f\n", (float) x1 - 0.1, (float) y1);
    fprintf(fp, "%f %f\n", (float) x1, (float) y1 - 0.1);
    fprintf(fp, "%f %f\n", (float) x1 + 0.1, (float) y1);
    fprintf(fp, "%f %f\n", (float) x1, (float) y1 + 0.1);
    fprintf(fp, "%f %f\n", (float) x1 - 0.1, (float) y1);
    fprintf(fp, "\n");
  }
  for (i = sttrees[netID].deg; i < sttrees[netID].deg * 2 - 2; i++) {
    x1 = treenodes[i].x;
    y1 = treenodes[i].y;
    fprintf(fp, "%f %f\n", (float) x1 - 0.1, (float) y1 + 0.1);
    fprintf(fp, "%f %f\n", (float) x1 + 0.1, (float) y1 - 0.1);
    fprintf(fp, "\n");
    fprintf(fp, "%f %f\n", (float) x1 + 0.1, (float) y1 + 0.1);
    fprintf(fp, "%f %f\n", (float) x1 - 0.1, (float) y1 - 0.1);
    fprintf(fp, "\n");
  }

  for (i = 0; i < sttrees[netID].deg * 2 - 3; i++) {
    if (1)  // i!=14)
    {
      treeedge = &(sttrees[netID].edges[i]);

      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;
      xmin = std::min(xmin, std::min(x1, x2));
      xmax = std::max(xmax, std::max(x1, x2));
      ymin = std::min(ymin, std::min(y1, y2));
      ymax = std::max(ymax, std::max(y1, y2));

      routetype = treeedge->route.type;

      if (routetype == LROUTE)  // remove L routing
      {
        if (treeedge->route.xFirst) {
          fprintf(fp, "%d %d\n", x1, y1);
          fprintf(fp, "%d %d\n", x2, y1);
          fprintf(fp, "%d %d\n", x2, y2);
          fprintf(fp, "\n");
        } else {
          fprintf(fp, "%d %d\n", x1, y1);
          fprintf(fp, "%d %d\n", x1, y2);
          fprintf(fp, "%d %d\n", x2, y2);
          fprintf(fp, "\n");
        }
      } else if (routetype == ZROUTE) {
        Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH) {
          fprintf(fp, "%d %d\n", x1, y1);
          fprintf(fp, "%d %d\n", Zpoint, y1);
          fprintf(fp, "%d %d\n", Zpoint, y2);
          fprintf(fp, "%d %d\n", x2, y2);
          fprintf(fp, "\n");
        } else {
          fprintf(fp, "%d %d\n", x1, y1);
          fprintf(fp, "%d %d\n", x1, Zpoint);
          fprintf(fp, "%d %d\n", x2, Zpoint);
          fprintf(fp, "%d %d\n", x2, y2);
          fprintf(fp, "\n");
        }
      } else if (routetype == MAZEROUTE) {
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        for (j = 0; j <= treeedge->route.routelen; j++) {
          fprintf(fp, "%d %d\n", gridsX[j], gridsY[j]);
        }
        fprintf(fp, "\n");
      }
    }
  }

  fprintf(fp, "%d %d\n", xmin - 2, ymin - 2);
  fprintf(fp, "\n");
  fprintf(fp, "%d %d\n", xmax + 2, ymax + 2);
  fclose(fp);
}

void getlen()
{
  int i, edgeID, totlen = 0;
  TreeEdge* treeedge;

  for (i = 0; i < numValidNets; i++) {
    for (edgeID = 0; edgeID < 2 * sttrees[i].deg - 3; edgeID++) {
      treeedge = &(sttrees[i].edges[edgeID]);
      if (treeedge->route.type < MAZEROUTE)
        logger->error(GRT, 195, "Invalid route type.");
      else
        totlen += treeedge->route.routelen;
    }
  }
  logger->info(GRT, 196, "Routed len: {}", totlen);
}

void ConvertToFull3DType2()
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
        if (treeedges[edgeID].route.type == MAZEROUTE) {
          free(treeedges[edgeID].route.gridsX);
          free(treeedges[edgeID].route.gridsY);
          free(treeedges[edgeID].route.gridsL);
        }
        treeedge->route.gridsX = (short*) calloc(newCNT, sizeof(short));
        treeedge->route.gridsY = (short*) calloc(newCNT, sizeof(short));
        treeedge->route.gridsL = (short*) calloc(newCNT, sizeof(short));
        treeedge->route.type = MAZEROUTE;
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

void netpinOrderInc()
{
  int j, d, ind, totalLength, xmin;
  TreeNode* treenodes;
  StTree* stree;

  float npvalue;

  numTreeedges = 0;
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

void fillVIA()
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

          if (treeedges[edgeID].route.type == MAZEROUTE) {
            free(treeedges[edgeID].route.gridsX);
            free(treeedges[edgeID].route.gridsY);
            free(treeedges[edgeID].route.gridsL);
          }
          treeedge->route.gridsX = (short*) calloc(newCNT, sizeof(short));
          treeedge->route.gridsY = (short*) calloc(newCNT, sizeof(short));
          treeedge->route.gridsL = (short*) calloc(newCNT, sizeof(short));
          treeedge->route.type = MAZEROUTE;
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
   logger->info(GRT, 198, "Via related stiner nodes: {}", numVIAT2);
   logger->info(GRT, 199, "Via filling finished.");
  }
}

int threeDVIA()
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

void assignEdge(int netID, int edgeID, Bool processDIR)
{
  short *gridsX, *gridsY, *gridsL;
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
              if (gridD[i][k] > gridD[l][k] + ADIFF(i, l) * 2) {
                gridD[i][k] = gridD[l][k] + ADIFF(i, l) * 2;
                viaLink[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + ADIFF(i, l) * 3) {
                gridD[i][k] = gridD[l][k] + ADIFF(i, l) * 3;
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
              > gridD[l][k] + ADIFF(i, l) * 1) {          //+ ADIFF(i,l) * 3 ) {
            gridD[i][k] = gridD[l][k] + ADIFF(i, l) * 1;  //+ ADIFF(i,l) * 3 ;
            viaLink[i][k] = l;
          }
        }
      }
    }

    k = routelen;

    if (treenodes[n2a].assigned) {
      min_result = BIG_INT;
      for (i = treenodes[n2a].topL; i >= treenodes[n2a].botL; i--) {
        if (gridD[i][routelen] < min_result
            || (min_result == BIG_INT && allowOverflow)) {
          min_result = gridD[i][routelen];
          endLayer = i;
        }
      }
    } else {
      min_result = gridD[0][routelen];
      endLayer = 0;
      for (i = 0; i < numLayers; i++) {
        if (gridD[i][routelen] < min_result
            || (min_result == BIG_INT && allowOverflow)) {
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
      treenodes[n2a].topL
          = gridsL[routelen];
      treenodes[n2a].botL
          = gridsL[routelen];
      treenodes[n2a].lID = treenodes[n2a].hID = edgeID;
    }

    if (treenodes[n2a].assigned) {
      if (gridsL[routelen] > treenodes[n2a].topL
          || gridsL[routelen] < treenodes[n2a].botL) {
        logger->error(GRT, 202, "Target ending layer ({}) out of range.", gridsL[routelen]);
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
              if (gridD[i][k] > gridD[l][k] + ADIFF(i, l) * 2) {
                gridD[i][k] = gridD[l][k] + ADIFF(i, l) * 2;
                viaLink[i][k] = l;
              }
            }
          } else {
            if (l != i) {
              if (gridD[i][k] > gridD[l][k] + ADIFF(i, l) * 3) {
                gridD[i][k] = gridD[l][k] + ADIFF(i, l) * 3;
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
          if (gridD[i][0] > gridD[l][0] + ADIFF(i, l) * 1) {
            gridD[i][0] = gridD[l][0] + ADIFF(i, l) * 1;
            viaLink[i][0] = l;
          }
        }
      }
    }

    if (treenodes[n1a].assigned) {
      min_result = BIG_INT;
      for (i = treenodes[n1a].topL; i >= treenodes[n1a].botL; i--) {
        if (gridD[i][k] < min_result
            || (min_result == BIG_INT && allowOverflow)) {
          min_result = gridD[i][0];
          endLayer = i;
        }
      }

    } else {
      min_result = gridD[0][k];
      endLayer = 0;
      for (i = 0; i < numLayers; i++) {
        if (gridD[i][k] < min_result
            || (min_result == BIG_INT && allowOverflow)) {
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
      // treenodes[n1a].assigned = TRUE;
      treenodes[n1a].topL = gridsL[0];  // std::max(endLayer, gridsL[0]);
      treenodes[n1a].botL = gridsL[0];  // std::min(endLayer, gridsL[0]);
      treenodes[n1a].lID = treenodes[n1a].hID = edgeID;
    }
  }
  treeedge->assigned = TRUE;

  int edgeCost = nets[netID]->edgeCost;

  for (k = 0; k < routelen; k++) {
    if (gridsX[k] == gridsX[k + 1]) {
      min_y = std::min(gridsY[k], gridsY[k + 1]);
      grid = gridsL[k] * gridV + min_y * xGrid + gridsX[k];

      v_edges3D[grid].usage += edgeCost;
    } else {
      min_x = std::min(gridsX[k], gridsX[k + 1]);
      grid = gridsL[k] * gridH + gridsY[k] * (xGrid - 1) + min_x;

      h_edges3D[grid].usage += edgeCost;
    }
  }
}

void newLayerAssignmentV4()
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
        treeedge->assigned = FALSE;
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
          treeedges[edgeID].assigned = TRUE;
        }
      }
    }

    while (!edgeQueue.empty()) {
      edgeID = edgeQueue.front();
      edgeQueue.pop();
      treeedge = &(treeedges[edgeID]);
      if (treenodes[treeedge->n1a].assigned) {
        assignEdge(netID, edgeID, 1);
        treeedge->assigned = TRUE;
        if (!treenodes[treeedge->n2a].assigned) {
          for (k = 0; k < treenodes[treeedge->n2a].conCNT; k++) {
            edgeID = treenodes[treeedge->n2a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = TRUE;
            }
          }
          treenodes[treeedge->n2a].assigned = TRUE;
        }
      } else {
        assignEdge(netID, edgeID, 0);
        treeedge->assigned = TRUE;
        if (!treenodes[treeedge->n1a].assigned) {
          for (k = 0; k < treenodes[treeedge->n1a].conCNT; k++) {
            edgeID = treenodes[treeedge->n1a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = TRUE;
            }
          }
          treenodes[treeedge->n1a].assigned = TRUE;
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
      treenodes[nodeID].assigned = FALSE;

      if (nodeID < deg) {
        treenodes[nodeID].botL = 0;
        treenodes[nodeID].assigned = TRUE;
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

        treenodes[n1a].assigned = TRUE;

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

        treenodes[n2a].assigned = TRUE;

      }  // edge len > 0
    }    // eunmerating edges
  }
}

void newLA()
{
  int netID, d, k, edgeID, deg, numpoints, n1, n2;
  Bool redundant;
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
      treenodes[d].assigned = FALSE;
      treenodes[d].stackAlias = d;
      treenodes[d].conCNT = 0;
      treenodes[d].hID = BIG_INT;
      treenodes[d].lID = BIG_INT;
      treenodes[d].status = 0;

      if (d < deg) {
        treenodes[d].botL = treenodes[d].topL = 0;
        // treenodes[d].l = 0;
        treenodes[d].assigned = TRUE;
        treenodes[d].status = 1;

        xcor[numpoints] = treenodes[d].x;
        ycor[numpoints] = treenodes[d].y;
        dcor[numpoints] = d;
        numpoints++;
      } else {
        redundant = FALSE;
        for (k = 0; k < numpoints; k++) {
          if ((treenodes[d].x == xcor[k]) && (treenodes[d].y == ycor[k])) {
            treenodes[d].stackAlias = dcor[k];

            redundant = TRUE;
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

void printEdge3D(int netID, int edgeID)
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
      edge_rpt = edge_rpt + "(" + std::to_string(edge.route.gridsX[i]) + ", " +
                 std::to_string(edge.route.gridsY[i]) + ", " +
                 std::to_string(edge.route.gridsL[i]) + ") ";
    }
    logger->report("{}", edge_rpt);
  }
}

void printTree3D(int netID)
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

void checkRoute3D()
{
  short *gridsX, *gridsY, *gridsL;
  int i, netID, edgeID, nodeID, edgelength;
  int n1, n2, x1, y1, x2, y2, deg;
  int distance;
  Bool gridFlag;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  for (netID = 0; netID < numValidNets; netID++) {
    treenodes = sttrees[netID].nodes;
    deg = sttrees[netID].deg;

    for (nodeID = 0; nodeID < 2 * deg - 2; nodeID++) {
      if (nodeID < deg) {
        if (treenodes[nodeID].botL != 0) {
          logger->error(GRT, 203, "Causing pin node floating.");
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

      gridFlag = FALSE;

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        logger->report("net[{}] edge[{}] start node wrong, net deg {}, n1 {}",
               netID,
               edgeID,
               deg,
               n1);
        printEdge3D(netID, edgeID);
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        logger->report("net[{}] edge[{}] end node wrong, net deg {}, n2 {}",
               netID,
               edgeID,
               deg,
               n2);
        printEdge3D(netID, edgeID);
      }
      for (i = 0; i < treeedge->route.routelen; i++) {
        distance = ADIFF(gridsX[i + 1], gridsX[i])
                   + ADIFF(gridsY[i + 1], gridsY[i])
                   + ADIFF(gridsL[i + 1], gridsL[i]);
        if (distance > 1 || distance < 0) {
          gridFlag = TRUE;
          logger->report("net {} edge[{}] maze route wrong, distance {}, i {}",
                 netName(nets[netID]),
                 edgeID,
                 distance,
                 i);
          logger->report("current [{}, {}, {}], next [{}, {}, {}]",
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
          logger->error(GRT, 204, "Invalid layer value in gridsL, {}.", gridsL[i]);
        }
      }
      if (gridFlag) {
        printEdge3D(netID, edgeID);
      }
    }
  }
}

void write3D()
{
  short *gridsX, *gridsY, *gridsL;
  int netID, i, edgeID, deg, lastX, lastY, lastL, xreal, yreal, routeLen;
  TreeEdge *treeedges, *treeedge;
  FILE* fp;
  TreeEdge edge;

  fp = fopen("output.out", "w");
  if (fp == NULL) {
    logger->error(GRT, 205, "Error in opening output.out.");
  }

  for (netID = 0; netID < numValidNets; netID++) {
    fprintf(fp, "%s %d\n", netName(nets[netID]), netID);
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      edge = sttrees[netID].edges[edgeID];
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        gridsL = treeedge->route.gridsL;
        lastX = wTile * (gridsX[0] + 0.5) + xcorner;
        lastY = hTile * (gridsY[0] + 0.5) + ycorner;
        lastL = gridsL[0];
        for (i = 1; i <= routeLen; i++) {
          xreal = wTile * (gridsX[i] + 0.5) + xcorner;
          yreal = hTile * (gridsY[i] + 0.5) + ycorner;

          fprintf(fp,
                  "(%d,%d,%d)-(%d,%d,%d)\n",
                  lastX,
                  lastY,
                  lastL + 1,
                  xreal,
                  yreal,
                  gridsL[i] + 1);
          lastX = xreal;
          lastY = yreal;
          lastL = gridsL[i];
        }
      }
    }
    fprintf(fp, "!\n");
  }
  fclose(fp);
}

static int compareTEL(const OrderTree a, const OrderTree b)
{
  return a.xmin > b.xmin;
}

void StNetOrder()
{
  short *gridsX, *gridsY;
  int i, j, d, ind, grid, min_x, min_y;
  TreeEdge *treeedges, *treeedge;
  StTree* stree;

  numTreeedges = 0;

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

void recoverEdge(int netID, int edgeID)
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
    logger->error(GRT, 206, "trying to recover an 0 length edge.");
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

  treenodes[n1a].assigned = TRUE;

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

  treenodes[n2a].assigned = TRUE;

  int edgeCost = nets[netID]->edgeCost;

  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = gridsL[i] * gridV + ymin * xGrid + gridsX[i];
        v_edges3D[grid].usage += edgeCost;
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsL[i] * gridH + gridsY[i] * (xGrid - 1) + xmin;
        h_edges3D[grid].usage += edgeCost;
      }
    }
  }
}

void checkUsage()
{
  short *gridsX, *gridsY;
  int netID, i, k, edgeID, deg;
  int j, cnt;
  Bool redsus;
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

        redsus = TRUE;

        while (redsus) {
          redsus = FALSE;

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
                redsus = TRUE;
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

static int compareEdgeLen(const void* a, const void* b)
{
  int ret = -2;
  if (((OrderNetEdge*) a)->length < ((OrderNetEdge*) b)->length) {
    ret = 1;
  } else if (((OrderNetEdge*) a)->length == ((OrderNetEdge*) b)->length) {
    ret = 0;
  } else if (((OrderNetEdge*) a)->length > ((OrderNetEdge*) b)->length) {
    ret = -1;
  }
  if (ret == -2) {
    logger->error(GRT, 178, "Invalid EdgeLen comparison.");
  } else {
    return ret;
  }
}

void netedgeOrderDec(int netID)
{
  int j, d, numTreeedges;

  d = sttrees[netID].deg;
  numTreeedges = 2 * d - 3;

  for (j = 0; j < numTreeedges; j++) {
    netEO[j].length = sttrees[netID].edges[j].route.routelen;
    netEO[j].edgeID = j;
  }

  qsort(netEO, numTreeedges, sizeof(OrderNetEdge), compareEdgeLen);
}

void printEdge2D(int netID, int edgeID)
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
      edge_rpt = edge_rpt + "(" + std::to_string(edge.route.gridsX[i]) +
                 ", " + std::to_string(edge.route.gridsY[i]) + ") ";
    }
    logger->report("{}", edge_rpt);
  }
}

void printTree2D(int netID)
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

Bool checkRoute2DTree(int netID)
{
  Bool STHwrong, gridFlag;
  short *gridsX, *gridsY;
  int i, edgeID, edgelength;
  int n1, n2, x1, y1, x2, y2;
  int distance;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  STHwrong = FALSE;

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

    gridFlag = FALSE;

    if (treeedge->len < 0) {
      logger->warn(GRT, 207, "rip upped edge without edge len re assignment.");
      STHwrong = TRUE;
    }

    if (treeedge->len > 0) {
      if (treeedge->route.routelen < 1) {
        logger->warn(GRT, 208, ".routelen {} len {}.",
          treeedge->route.routelen, treeedge->len);
        STHwrong = TRUE;
        return (TRUE);
      }

      if (gridsX[0] != x1 || gridsY[0] != y1) {
        logger->warn(GRT, 164, "initial grid wrong y1 x1 [{} {}] , net start [{} {}] routelen "
            "{}.",
            y1,
            x1,
            gridsY[0],
            gridsX[0],
            treeedge->route.routelen);
        STHwrong = TRUE;
      }
      if (gridsX[edgelength] != x2 || gridsY[edgelength] != y2) {
        logger->warn(GRT, 165, "end grid wrong y2 x2 [{} {}] , net start [{} {}] routelen {}.",
            y1,
            x1,
            gridsY[edgelength],
            gridsX[edgelength],
            treeedge->route.routelen);
        STHwrong = TRUE;
      }
      for (i = 0; i < treeedge->route.routelen; i++) {
        distance
            = ADIFF(gridsX[i + 1], gridsX[i]) + ADIFF(gridsY[i + 1], gridsY[i]);
        if (distance != 1) {
          logger->warn(GRT, 166, "net {} edge[{}] maze route wrong, distance {}, i {}.",
                 netName(nets[netID]),
                 edgeID,
                 distance,
                 i);
          gridFlag = TRUE;
          STHwrong = TRUE;
        }
      }

      if (STHwrong) {
        logger->warn(GRT, 167, "checking failed {}.", netID);
        return (TRUE);
      }
    }
  }

  return (STHwrong);
}

void writeRoute3D(char routingfile3D[])
{
  short *gridsX, *gridsY, *gridsL;
  int netID, i, edgeID, deg, lastX, lastY, lastL, xreal, yreal, routeLen;
  TreeEdge *treeedges, *treeedge;
  FILE* fp;
  TreeEdge edge;

  fp = fopen(routingfile3D, "w");
  if (fp == NULL) {
    logger->error(GRT, 168, "Error in opening {}.", routingfile3D);
  }

  for (netID = 0; netID < numValidNets; netID++) {
    fprintf(fp, "%s %d\n", netName(nets[netID]), netID);
    treeedges = sttrees[netID].edges;
    deg = sttrees[netID].deg;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      edge = sttrees[netID].edges[edgeID];
      treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        routeLen = treeedge->route.routelen;
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        gridsL = treeedge->route.gridsL;
        lastX = wTile * (gridsX[0] + 0.5) + xcorner;
        lastY = hTile * (gridsY[0] + 0.5) + ycorner;
        lastL = gridsL[0];
        for (i = 1; i <= routeLen; i++) {
          xreal = wTile * (gridsX[i] + 0.5) + xcorner;
          yreal = hTile * (gridsY[i] + 0.5) + ycorner;

          fprintf(fp,
                  "(%d,%d,%d)-(%d,%d,%d)\n",
                  lastX,
                  lastY,
                  lastL + 1,
                  xreal,
                  yreal,
                  gridsL[i] + 1);
          lastX = xreal;
          lastY = yreal;
          lastL = gridsL[i];
        }
      }
    }
    fprintf(fp, "!\n");
  }
  fclose(fp);
}

float* pH;
float* pV;
struct BBox* netBox;
struct BBox** pnetBox;

struct TD
{
  int id;
  float cost;
};

struct BBox
{
  int xmin;
  int ymin;
  int xmax;
  int ymax;
  int hSpan;
  int vSpan;
};  // lower_left corner and upper_right corner

struct wire
{
  int x1, y1, x2, y2;
  int netID;
};

// Copy Routing Solution for the best routing solution so far
void copyRS(void)
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

void copyBR(void)
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

        sttrees[netID].edges[edgeID].route.type = MAZEROUTE;
        sttrees[netID].edges[edgeID].route.routelen
            = sttreesBK[netID].edges[edgeID].route.routelen;

        if (sttreesBK[netID].edges[edgeID].len
            > 0)  // only route the non-degraded edges (len>0)
        {
          sttrees[netID].edges[edgeID].route.type = MAZEROUTE;
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

void freeRR(void)
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

Tree fluteToTree(stt::Tree fluteTree)
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

stt::Tree treeToFlute(Tree tree)
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

Tree pdToTree(PD::Tree pdTree)
{
  Tree tree;
  tree.deg = pdTree.deg;
  tree.totalDeg = 2 * pdTree.deg - 2;
  tree.length = (DTYPE) pdTree.length;
  tree.branch = new Branch[tree.totalDeg];
  for (int i = 0; i < tree.totalDeg; i++) {
    tree.branch[i].x = (DTYPE) pdTree.branch[i].x;
    tree.branch[i].y = (DTYPE) pdTree.branch[i].y;
    tree.branch[i].n = pdTree.branch[i].n;
  }
  return tree;
}
}  // namespace grt
