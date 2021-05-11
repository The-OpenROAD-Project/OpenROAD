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

#include "DataProc.h"
#include "DataType.h"
#include "RSMT.h"
#include "RipUp.h"
#include "flute.h"
#include "maze.h"
#include "maze3D.h"
#include "utl/Logger.h"
#include "pdrev/pdrev.h"
#include "opendb/db.h"

#include "route.h"
#include "utility.h"

namespace grt {

using utl::GRT;

int newnetID;
int segcount;
int pinInd;
int numAdjust;
int vCapacity;
int hCapacity;
int MD;

FastRouteCore::FastRouteCore(odb::dbDatabase* db, utl::Logger* log)
{
  newnetID = 0;
  segcount = 0;
  pinInd = 0;
  numAdjust = 0;
  vCapacity = 0;
  hCapacity = 0;
  MD = 0;
  numNets = 0;
  invalidNets = 0;
  maxNetDegree = 0;
  logger = log;
  db_ = db;
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
  if (nets) {
    for (int i = 0; i < numNets; i++) {
      if (nets[i])
        delete nets[i];
      nets[i] = nullptr;
    }
  
    delete[] nets;
    nets = nullptr;
  }

  if (h_edges)
    delete[] h_edges;
  h_edges = nullptr;

  if (v_edges)
    delete[] v_edges;
  v_edges = nullptr;

  if (seglist)
    delete[] seglist;
  seglist = nullptr;

  if (seglistIndex)
    delete[] seglistIndex;
  seglistIndex = nullptr;

  if (seglistCnt)
    delete[] seglistCnt;
  seglistCnt = nullptr;

  if (gxs)
    delete[] gxs;
  gxs = nullptr;

  if (gys)
    delete[] gys;
  gys = nullptr;

  if (gs)
    delete[] gs;
  gs = nullptr;

  treeOrderPV.clear();
  treeOrderCong.clear();

  if (h_edges3D)
    delete[] h_edges3D;
  h_edges3D = nullptr;

  if (v_edges3D)
    delete[] v_edges3D;
  v_edges3D = nullptr;

  if (trees)
    delete[] trees;
  trees = nullptr;

  if (sttrees) {
    for (int i = 0; i < numValidNets; i++) {
      int deg = sttrees[i].deg;
      int numEdges = 2 * deg - 3;
      for (int edgeID = 0; edgeID < numEdges; edgeID++) {
        TreeEdge* treeedge = &(sttrees[i].edges[edgeID]);
        if (treeedge->len > 0) {
          if (treeedge->route.gridsX)
            free(treeedge->route.gridsX);
          if (treeedge->route.gridsY)
            free(treeedge->route.gridsY);
          if (treeedge->route.gridsL)
            free(treeedge->route.gridsL);
          treeedge->route.gridsX = nullptr;
          treeedge->route.gridsY = nullptr;
          treeedge->route.gridsL = nullptr;
        }
      }

      if (sttrees[i].nodes)
        delete[] sttrees[i].nodes;
      sttrees[i].nodes = nullptr;

      if (sttrees[i].edges)
        delete[] sttrees[i].edges;
      sttrees[i].edges = nullptr;
    }
    delete[] sttrees;
    sttrees = nullptr;
  }
  
  parentX1.resize(boost::extents[0][0]);
  parentY1.resize(boost::extents[0][0]);
  parentX3.resize(boost::extents[0][0]);
  parentY3.resize(boost::extents[0][0]);

  if (pop_heap2)
    delete[] pop_heap2;
  if (heap1)
    delete[] heap1;
  if (heap2)
    delete[] heap2;

  pop_heap2 = nullptr;
  heap1 = nullptr;
  heap2 = nullptr;

  if (xcor)
    delete[] xcor;
  if (ycor)
    delete[] ycor;
  if (dcor)
    delete[] dcor;

  netEO.clear();

  xcor = nullptr;
  ycor = nullptr;
  dcor = nullptr;

  if (HV) {
    for (int i = 0; i < YRANGE; i++) {
      if (HV[i])
        delete[] HV[i];
      HV[i] = nullptr;
    }

    delete[] HV;
    HV = nullptr;
  }

  if (hyperV) {
    for (int i = 0; i < YRANGE; i++) {
      if (hyperV[i])
        delete[] hyperV[i];
      hyperV[i] = nullptr;
    }

    delete[] hyperV;
    hyperV = nullptr;
  }

  if (hyperH) {
    for (int i = 0; i < XRANGE; i++) {
      if (hyperH[i])
        delete[] hyperH[i];
      hyperH[i] = nullptr;
    }

    delete[] hyperH;
    hyperH = nullptr;
  }

  if (inRegion) {
    for (int i = 0; i < YRANGE; i++) {
      if (inRegion[i])
        delete[] inRegion[i];
      inRegion[i] = nullptr;
    }

    delete[] inRegion;
    inRegion = nullptr;
  }

  if (corrEdge) {
    for (int i = 0; i < YRANGE; i++) {
      if (corrEdge[i])
        delete[] corrEdge[i];
      corrEdge[i] = nullptr;
    }

    delete[] corrEdge;
    corrEdge = nullptr;
  }

  d13D.resize(boost::extents[0][0][0]);
  d23D.resize(boost::extents[0][0][0]);
  d1.resize(boost::extents[0][0]);
  d2.resize(boost::extents[0][0]);

  if (vCapacity3D)
    delete[] vCapacity3D;
  if (hCapacity3D)
    delete[] hCapacity3D;

  vCapacity3D = nullptr;
  hCapacity3D = nullptr;

  if (MinWidth)
    delete[] MinWidth;
  if (MinSpacing)
    delete[] MinSpacing;
  if (ViaSpacing)
    delete[] ViaSpacing;

  MinWidth = nullptr;
  MinSpacing = nullptr;
  ViaSpacing = nullptr;

  if (gridHs)
    delete[] gridHs;
  if (gridVs)
    delete[] gridVs;

  gridHs = nullptr;
  gridVs = nullptr;

  if (layerGrid) {
    for (int i = 0; i < numLayers; i++) {
      if (layerGrid[i])
        delete[] layerGrid[i];
      layerGrid[i] = nullptr;
    }

    delete[] layerGrid;
    layerGrid = nullptr;
  }

  if (gridD) {
    for (int i = 0; i < numLayers; i++) {
      if (gridD[i])
        delete[] gridD[i];
      gridD[i] = nullptr;
    }

    delete[] gridD;
    gridD = nullptr;
  }

  if (viaLink) {
    for (int i = 0; i < numLayers; i++) {
      if (viaLink[i])
        delete[] viaLink[i];
      viaLink[i] = nullptr;
    }

    delete[] viaLink;
    viaLink = nullptr;
  }

  if (costHVH)
    delete[] costHVH;
  if (costVHV)
    delete[] costVHV;
  if (costH)
    delete[] costH;
  if (costV)
    delete[] costV;
  if (costLR)
    delete[] costLR;
  if (costTB)
    delete[] costTB;
  if (costHVHtest)
    delete[] costHVHtest;
  if (costVtest)
    delete[] costVtest;
  if (costTBtest)
    delete[] costTBtest;

  costHVH = nullptr;
  costVHV = nullptr;
  costH = nullptr;
  costV = nullptr;
  costLR = nullptr;
  costTB = nullptr;
  costHVHtest = nullptr;
  costVtest = nullptr;
  costTBtest = nullptr;

  newnetID = 0;
  segcount = 0;
  pinInd = 0;
  numAdjust = 0;
  vCapacity = 0;
  hCapacity = 0;
  MD = 0;
  numNets = 0;
}

void FastRouteCore::setGridsAndLayers(int x, int y, int nLayers)
{
  xGrid = x;
  yGrid = y;
  numLayers = nLayers;
  numGrids = xGrid * yGrid;
  if (std::max(xGrid, yGrid) >= 1000) {
    XRANGE = std::max(xGrid, yGrid);
    YRANGE = std::max(xGrid, yGrid);
  } else {
    XRANGE = 1000;
    YRANGE = 1000;
  }

  vCapacity3D = new short[numLayers];
  hCapacity3D = new short[numLayers];

  for (int i = 0; i < numLayers; i++) {
    vCapacity3D[i] = 0;
    hCapacity3D[i] = 0;
  }

  MinWidth = new int[numLayers];
  MinSpacing = new int[numLayers];
  ViaSpacing = new int[numLayers];
  gridHs = new int[numLayers];
  gridVs = new int[numLayers];

  layerGrid = new int*[numLayers];
  for (int i = 0; i < numLayers; i++) {
    layerGrid[i] = new int[MAXLEN];
  }

  gridD = new int*[numLayers];
  for (int i = 0; i < numLayers; i++) {
    gridD[i] = new int[MAXLEN];
  }

  viaLink = new int*[numLayers];
  for (int i = 0; i < numLayers; i++) {
    viaLink[i] = new int[MAXLEN];
  }

  d13D.resize(boost::extents[numLayers][YRANGE][XRANGE]);
  d23D.resize(boost::extents[numLayers][YRANGE][XRANGE]);

  d1.resize(boost::extents[YRANGE][XRANGE]);
  d2.resize(boost::extents[YRANGE][XRANGE]);

  HV = new Bool*[YRANGE];
  for (int i = 0; i < YRANGE; i++) {
    HV[i] = new Bool[XRANGE];
  }

  hyperV = new Bool*[YRANGE];
  for (int i = 0; i < YRANGE; i++) {
    hyperV[i] = new Bool[XRANGE];
  }

  hyperH = new Bool*[YRANGE];
  for (int i = 0; i < YRANGE; i++) {
    hyperH[i] = new Bool[XRANGE];
  }

  corrEdge = new int*[YRANGE];
  for (int i = 0; i < YRANGE; i++) {
    corrEdge[i] = new int[XRANGE];
  }

  inRegion = new Bool*[YRANGE];
  for (int i = 0; i < YRANGE; i++) {
    inRegion[i] = new Bool[XRANGE];
  }

  costHVH = new float[XRANGE];  // Horizontal first Z
  costVHV = new float[YRANGE];  // Vertical first Z
  costH = new float[YRANGE];    // Horizontal segment cost
  costV = new float[XRANGE];    // Vertical segment cost
  costLR = new float[YRANGE];   // Left and right boundary cost
  costTB = new float[XRANGE];   // Top and bottom boundary cost

  costHVHtest = new float[YRANGE];  // Vertical first Z
  costVtest = new float[XRANGE];    // Vertical segment cost
  costTBtest = new float[XRANGE];   // Top and bottom boundary cost
}

void FastRouteCore::addVCapacity(short verticalCapacity, int layer)
{
  vCapacity3D[layer - 1] = verticalCapacity;
  vCapacity += vCapacity3D[layer - 1];
}

void FastRouteCore::addHCapacity(short horizontalCapacity, int layer)
{
  hCapacity3D[layer - 1] = horizontalCapacity;
  hCapacity += hCapacity3D[layer - 1];
}

void FastRouteCore::addMinWidth(int width, int layer)
{
  MinWidth[layer - 1] = width;
}

void FastRouteCore::addMinSpacing(int spacing, int layer)
{
  MinSpacing[layer - 1] = spacing;
}

void FastRouteCore::addViaSpacing(int spacing, int layer)
{
  ViaSpacing[layer - 1] = spacing;
}

void FastRouteCore::setNumberNets(int nNets)
{
  numNets = nNets;
  nets = new FrNet*[numNets];
  for (int i = 0; i < numNets; i++)
    nets[i] = new FrNet;
  seglistIndex = new int[numNets + 1];
}

void FastRouteCore::setLowerLeft(int x, int y)
{
  xcorner = x;
  ycorner = y;
}

void FastRouteCore::setTileSize(int width, int height)
{
  wTile = width;
  hTile = height;
}

void FastRouteCore::setLayerOrientation(int x)
{
  layerOrientation = x;
}

void FastRouteCore::addPin(int netID, int x, int y, int layer)
{
  nets[netID]->pinX.push_back(x);
  nets[netID]->pinY.push_back(y);
  nets[netID]->pinL.push_back(layer);
}

int FastRouteCore::addNet(odb::dbNet* db_net,
                          int nPins,
                          int validPins,
                          float alpha,
                          bool isClock,
                          int cost,
                          std::vector<int> edgeCostPerLayer)
{
  int netID = newnetID;
  pinInd = validPins;
  MD = std::max(MD, pinInd);
  nets[newnetID]->db_net = db_net;
  nets[newnetID]->numPins = nPins;
  nets[newnetID]->deg = pinInd;
  nets[newnetID]->alpha = alpha;
  nets[newnetID]->isClock = isClock;
  nets[newnetID]->edgeCost = cost;
  nets[newnetID]->edgeCostPerLayer = edgeCostPerLayer;

  seglistIndex[newnetID] = segcount;
  newnetID++;
  // at most (2*nPins-2) nodes -> (2*nPins-3) segs for a net
  segcount += 2 * pinInd - 3;
  return netID;
}

void FastRouteCore::initEdges()
{
  LB = 0.9;
  UB = 1.3;
  vCapacity_lb = LB * vCapacity;
  hCapacity_lb = LB * hCapacity;
  vCapacity_ub = UB * vCapacity;
  hCapacity_ub = UB * hCapacity;

  // TODO: check this, there was an if pinInd > 1 && pinInd < 2000
  if (pinInd > 1) {
    seglistIndex[newnetID] = segcount;  // the end pointer of the seglist
  }
  numValidNets = newnetID;

  // allocate memory and initialize for edges

  h_edges = new Edge[((xGrid - 1) * yGrid)];
  v_edges = new Edge[(xGrid * (yGrid - 1))];

  init_usage();

  v_edges3D = new Edge3D[(numLayers * xGrid * yGrid)];
  h_edges3D = new Edge3D[(numLayers * xGrid * yGrid)];

  // 2D edge initialization
  int TC = 0;
  for (int i = 0; i < yGrid; i++) {
    for (int j = 0; j < xGrid - 1; j++) {
      int grid = i * (xGrid - 1) + j;
      h_edges[grid].cap = hCapacity;
      TC += hCapacity;
      h_edges[grid].usage = 0;
      h_edges[grid].est_usage = 0;
      h_edges[grid].red = 0;
      h_edges[grid].last_usage = 0;
    }
  }
  for (int i = 0; i < yGrid - 1; i++) {
    for (int j = 0; j < xGrid; j++) {
      int grid = i * xGrid + j;
      v_edges[grid].cap = vCapacity;
      TC += vCapacity;
      v_edges[grid].usage = 0;
      v_edges[grid].est_usage = 0;
      v_edges[grid].red = 0;
      v_edges[grid].last_usage = 0;
    }
  }

  // 3D edge initialization
  for (int k = 0; k < numLayers; k++) {
    for (int i = 0; i < yGrid; i++) {
      for (int j = 0; j < xGrid; j++) {
        int grid = i * (xGrid - 1) + j + k * (xGrid - 1) * yGrid;
        h_edges3D[grid].cap = hCapacity3D[k];
        h_edges3D[grid].usage = 0;
        h_edges3D[grid].red = 0;
      }
    }
    for (int i = 0; i < yGrid; i++) {
      for (int j = 0; j < xGrid; j++) {
        int grid = i * xGrid + j + k * xGrid * (yGrid - 1);
        v_edges3D[grid].cap = vCapacity3D[k];
        v_edges3D[grid].usage = 0;
        v_edges3D[grid].red = 0;
      }
    }
  }
}

void FastRouteCore::setNumAdjustments(int nAdjustments)
{
  numAdjust = nAdjustments;
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
    grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    resource = h_edges3D[grid].cap - h_edges3D[grid].usage;
  } else if (x1 == x2) {
    grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    resource = v_edges3D[grid].cap - v_edges3D[grid].usage;
  } else
  {
    logger->error(GRT, 212, "Cannot get edge resource: edge is not vertical or horizontal.");
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
    grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    usage = h_edges3D[grid].usage;
  } else if (x1 == x2) {
    grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    usage = v_edges3D[grid].usage;
  } else
  {
    logger->error(GRT, 213, "Cannot get edge usage: edge is not vertical or horizontal.");
  }

  return usage;
}

void FastRouteCore::setMaxNetDegree(int deg)
{
  maxNetDegree = deg;
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
    int grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    int cap = h_edges3D[grid].cap;
    int reduce;

    if (((int) cap - reducedCap) < 0) {
      if (isReduce) {
        logger->warn(GRT, 113, "Underflow in reduce: cap, reducedCap: {}, {}", cap, reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    h_edges3D[grid].cap = reducedCap;
    h_edges3D[grid].red = reduce;

    grid = y1 * (xGrid - 1) + x1;
    if (!isReduce) {
      int increase = reducedCap - cap;
      h_edges[grid].cap += increase;
    }

    h_edges[grid].cap -= reduce;
    h_edges[grid].red += reduce;

  } else if (x1 == x2)  // vertical edge
  {
    int grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    int cap = v_edges3D[grid].cap;
    int reduce;

    if (((int) cap - reducedCap) < 0) {
      if (isReduce) {
        logger->warn(GRT, 114, "Underflow in reduce: cap, reducedCap: {}, {}", cap, reducedCap);
      }
      reduce = 0;
    } else {
      reduce = cap - reducedCap;
    }

    v_edges3D[grid].cap = reducedCap;
    v_edges3D[grid].red = reduce;

    grid = y1 * xGrid + x1;
    if (!isReduce) {
      int increase = reducedCap - cap;
      v_edges[grid].cap += increase;
    }

    v_edges[grid].cap -= reduce;
    v_edges[grid].red += reduce;
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
    int grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    cap = h_edges3D[grid].cap;
  } else if (x1 == x2)  // vertical edge
  {
    int grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    cap = v_edges3D[grid].cap;
  } else
  {
    logger->error(GRT, 214, "Cannot get edge capacity: edge is not vertical or horizontal.");
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
    grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    int currCap = h_edges3D[grid].cap;
    h_edges3D[grid].cap = newCap;

    grid = y1 * (xGrid - 1) + x1;
    reduce = currCap - newCap;
    h_edges[grid].cap -= reduce;
  } else if (x1 == x2)  // vertical edge
  {
    grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    int currCap = v_edges3D[grid].cap;
    v_edges3D[grid].cap = newCap;

    grid = y1 * xGrid + x1;
    reduce = currCap - newCap;
    v_edges[grid].cap -= reduce;
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
  int reduce;

  if (y1 == y2)  // horizontal edge
  {
    grid = y1 * (xGrid - 1) + x1 + k * (xGrid - 1) * yGrid;
    h_edges3D[grid].usage = newUsage;

    grid = y1 * (xGrid - 1) + x1;
    h_edges[grid].usage += newUsage;
  } else if (x1 == x2)  // vertical edge
  {
    grid = y1 * xGrid + x1 + k * xGrid * (yGrid - 1);
    v_edges3D[grid].usage = newUsage;

    grid = y1 * xGrid + x1;
    v_edges[grid].usage += newUsage;
  }
}

void FastRouteCore::initAuxVar()
{
  treeOrderCong.clear();
  stopDEC = FALSE;

  seglistCnt = new int[numValidNets];
  seglist = new Segment[segcount];
  trees = new Tree[numValidNets];
  sttrees = new StTree[numValidNets];
  gxs = new DTYPE*[numValidNets];
  gys = new DTYPE*[numValidNets];
  gs = new DTYPE*[numValidNets];

  gridHV = XRANGE * YRANGE;
  gridH = (xGrid - 1) * yGrid;
  gridV = xGrid * (yGrid - 1);
  for (int k = 0; k < numLayers; k++) {
    gridHs[k] = k * gridH;
    gridVs[k] = k * gridV;
  }

  MaxDegree = MD;

  parentX1.resize(boost::extents[yGrid][xGrid]);
  parentY1.resize(boost::extents[yGrid][xGrid]);
  parentX3.resize(boost::extents[yGrid][xGrid]);
  parentY3.resize(boost::extents[yGrid][xGrid]);

  pop_heap2 = new Bool[yGrid * XRANGE];

  // allocate memory for priority queue
  heap1 = new float*[yGrid * xGrid];
  heap2 = new float*[yGrid * xGrid];

  sttreesBK = NULL;
}

NetRouteMap FastRouteCore::getRoutes()
{
  NetRouteMap routes;
  for (int netID = 0; netID < numValidNets; netID++) {
    odb::dbNet* db_net = nets[netID]->db_net;
    GRoute& route = routes[db_net];

    TreeEdge* treeedges = sttrees[netID].edges;
    int deg = sttrees[netID].deg;

    TreeNode* nodes = sttrees[netID].nodes;
    for (int edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int routeLen = treeedge->route.routelen;
        short* gridsX = treeedge->route.gridsX;
        short* gridsY = treeedge->route.gridsY;
        short* gridsL = treeedge->route.gridsL;
        int lastX = wTile * (gridsX[0] + 0.5) + xcorner;
        int lastY = hTile * (gridsY[0] + 0.5) + ycorner;
        int lastL = gridsL[0];
        for (int i = 1; i <= routeLen; i++) {
          int xreal = wTile * (gridsX[i] + 0.5) + xcorner;
          int yreal = hTile * (gridsY[i] + 0.5) + ycorner;

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
    logger->warn(utl::GRT, 211, "dbGcellGrid already exists in db. Clearing existing dbGCellGrid");
    db_gcell->resetGrid();
  }
  db_gcell->addGridPatternX(xcorner, xGrid, wTile);
  db_gcell->addGridPatternY(ycorner, yGrid + 1, hTile);
  for (int k = 0; k < numLayers; k++) {
    auto layer = db_->getTech()->findRoutingLayer(k+1);
    if(layer == nullptr)
    {
      logger->warn(utl::GRT, 215, "skipping layer {} not found in db", k+1);
      continue;
    }

    for (int y = 0; y < yGrid; y++) {
      for (int x = 0; x < xGrid - 1; x++) {
        int gridH = y * (xGrid - 1) + x + k * (xGrid - 1) * yGrid;

        unsigned short capH = hCapacity3D[k];
        unsigned short blockageH = (hCapacity3D[k] - h_edges3D[gridH].cap);
        unsigned short usageH = h_edges3D[gridH].usage + blockageH;

        db_gcell->setHorizontalCapacity(layer, x, y, (uint) capH);
        db_gcell->setHorizontalUsage(layer, x, y, (uint) usageH);
        db_gcell->setHorizontalBlockage(layer, x, y, (uint) blockageH);
      }
    }

    for (int y = 0; y < yGrid - 1; y++) {
      for (int x = 0; x < xGrid; x++) {
        int gridV = y * xGrid + x + k * xGrid * (yGrid - 1);

        unsigned short capV = vCapacity3D[k];
        unsigned short blockageV = (vCapacity3D[k] - v_edges3D[gridV].cap);
        unsigned short usageV = v_edges3D[gridV].usage + blockageV;

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
  int minoflrnd;
  int bwcnt = 0;

  // TODO: check this size
  int maxPin = maxNetDegree;
  maxPin = 2 * maxPin;
  xcor = new int[maxPin];
  ycor = new int[maxPin];
  dcor = new int[maxPin];
  netEO.reserve(maxPin);

  Bool input, WriteOut;
  input = WriteOut = 0;

  LB = 0.9;
  UB = 1.3;

  SLOPE = 5;
  THRESH_M = 20;
  ENLARGE = 15;     // 5
  int ESTEP1 = 10;  // 10
  int ESTEP2 = 5;   // 5
  int ESTEP3 = 5;   // 5
  int CSTEP1 = 2;   // 5
  int CSTEP2 = 2;   // 3
  int CSTEP3 = 5;   // 15
  int CSTEP4 = 1000;
  COSHEIGHT = 4;
  L = 0;
  VIA = 2;
  int L_afterSTOP = 1;
  int Ripvalue = -1;
  int ripupTH3D = 10;
  Bool goingLV = TRUE;
  Bool noADJ = FALSE;
  int thStep1 = 10;
  int thStep2 = 4;
  Bool healingNeed = FALSE;
  int updateType = 0;
  int LVIter = 3;
  Bool extremeNeeded = FALSE;
  int mazeRound = 500;
  int bmfl = BIG_INT;
  int minofl = BIG_INT;

  // call FLUTE to generate RSMT and break the nets into segments (2-pin nets)

  clock_t t1 = clock();

  VIA = 2;
  // viacost = VIA;
  viacost = 0;
  gen_brk_RSMT(FALSE, FALSE, FALSE, FALSE, noADJ, logger);
  if (verbose > 1)
    logger->info(GRT, 97, "First L Route.");
  routeLAll(TRUE);
  gen_brk_RSMT(TRUE, TRUE, TRUE, FALSE, noADJ, logger);
  getOverflow2D(&maxOverflow);
  if (verbose > 1)
    logger->info(GRT, 98, "Second L Route.");
  newrouteLAll(FALSE, TRUE);
  getOverflow2D(&maxOverflow);
  spiralRouteAll();
  newrouteZAll(10);
  if (verbose > 1)
    logger->info(GRT, 99, "First Z Route.");
  int past_cong = getOverflow2D(&maxOverflow);

  convertToMazeroute();

  int enlarge = 10;
  int newTH = 10;
  int healingTrigger = 0;
  stopDEC = 0;
  int upType = 1;
  // iniBDE();
  costheight = COSHEIGHT;
  if (maxOverflow > 700) {
    costheight = 8;
    LOGIS_COF = 1.33;
    VIA = 0;
    THRESH_M = 0;
    CSTEP1 = 30;
    slope = BIG_INT;
  }

  for (int i = 0; i < LVIter; i++) {
    LOGIS_COF = std::max<float>(2.0 / (1 + log(maxOverflow)), LOGIS_COF);
    LOGIS_COF = 2.0 / (1 + log(maxOverflow));
    if (verbose > 1)
      logger->info(GRT, 100, "LV routing round {}, enlarge {}.", i, enlarge);
    routeLVAll(newTH, enlarge);

    past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

    enlarge += 5;
    newTH -= 5;
    if (newTH < 1) {
      newTH = 1;
    }
  }

  //  past_cong = getOverflow2Dmaze( &maxOverflow);

  clock_t t3 = clock();
  InitEstUsage();

  int i = 1;
  costheight = COSHEIGHT;
  enlarge = ENLARGE;
  int ripup_threshold = Ripvalue;

  minofl = totalOverflow;
  stopDEC = FALSE;

  slope = 20;
  L = 1;
  int cost_type = 1;

  InitLastUsage(upType);
  if (totalOverflow > 0) {
    logger->info(GRT, 101, "Running extra iterations to remove overflow.");
  }

  while (totalOverflow > 0 && i <= overflowIterations) {
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

    if (totalOverflow > 2000) {
      enlarge += ESTEP1;  // ENLARGE+(i-1)*ESTEP;
      cost_step = CSTEP1;
      updateCongestionHistory(i, upType);
    } else if (totalOverflow < 500) {
      cost_step = CSTEP3;
      enlarge += ESTEP3;
      ripup_threshold = -1;
      updateCongestionHistory(i, upType);
    } else {
      cost_step = CSTEP2;
      enlarge += ESTEP2;
      updateCongestionHistory(i, upType);
    }

    if (totalOverflow > 15000 && maxOverflow > 400) {
      enlarge = std::max(xGrid, yGrid) / 30;
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

    enlarge = std::min(enlarge, xGrid / 2);
    costheight += cost_step;
    mazeedge_Threshold = THRESH_M;

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

    logger->info(GRT, 102, "iteration {}, enlarge {}, costheight {},"
                  " threshold {} via cost {}. \n"
                  "log_coef {}, healingTrigger {} cost_step {} L {} cost_type"
                  " {} updatetype {}.",
        i,
        enlarge,
        costheight,
        mazeedge_Threshold,
        VIA,
        LOGIS_COF,
        healingTrigger,
        cost_step,
        L,
        cost_type,
        upType);
    mazeRouteMSMD(i,
                  enlarge,
                  costheight,
                  ripup_threshold,
                  mazeedge_Threshold,
                  !(i % 3),
                  cost_type);
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
      stopDEC = TRUE;
    }

    if (maxOverflow < 150) {
      if (i == 20 && past_cong > 200) {
        logger->info(GRT, 103, "Extra Run for hard benchmark.");
        L = 0;
        upType = 3;
        stopDEC = TRUE;
        slope = 5;
        mazeRouteMSMD(i,
                      enlarge,
                      costheight,
                      ripup_threshold,
                      mazeedge_Threshold,
                      !(i % 3),
                      cost_type);
        last_cong = past_cong;
        past_cong = getOverflow2Dmaze(&maxOverflow, &tUsage);

        str_accu(12);
        L = 1;
        stopDEC = FALSE;
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
        stopDEC = TRUE;
      }
    }

    if (past_cong > 0.7 * last_cong) {
      costheight += CSTEP3;
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
                      enlarge,
                      costheight,
                      ripup_threshold,
                      mazeedge_Threshold,
                      !(i % 3),
                      cost_type);
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
  }

  if (minofl > 0) {
    logger->info(GRT, 104, "minimal ofl {}, occuring at round {}.", minofl, minoflrnd);
    copyBR();
  }

  freeRR();

  checkUsage();

  if (verbose > 1)
    logger->info(GRT, 105, "Maze routing finished.");

  clock_t t4 = clock();
  float maze_Time = (float) (t4 - t3) / CLOCKS_PER_SEC;

  if (verbose > 1) {
    logger->report("Final 2D results:");
  }
  getOverflow2Dmaze(&maxOverflow, &tUsage);

  if (verbose > 1)
    logger->info(GRT, 106, "Layer Assignment Begins.");
  newLA();
  if (verbose > 1)
    logger->info(GRT, 107, "Layer assignment finished.");

  clock_t t2 = clock();
  float gen_brk_Time = (float) (t2 - t1) / CLOCKS_PER_SEC;

  costheight = 3;
  viacost = 1;

  if (gen_brk_Time < 60) {
    ripupTH3D = 15;
  } else if (gen_brk_Time < 120) {
    ripupTH3D = 18;
  } else {
    ripupTH3D = 20;
  }

  if (goingLV && past_cong == 0) {
    if (verbose > 1)
      logger->info(GRT, 108, "Post Processing Begins.");
    mazeRouteMSMDOrder3D(enlarge, 0, ripupTH3D);

    //  mazeRouteMSMDOrder3D(enlarge, 0, 10 );
    if (gen_brk_Time > 120) {
      mazeRouteMSMDOrder3D(enlarge, 0, 12);
    }
    if (verbose > 1)
      logger->info(GRT, 109, "Post Processsing finished.\n Starting via filling.");
  }

  fillVIA();
  int finallength = getOverflow3D();
  int numVia = threeDVIA();
  checkRoute3D();

  clock_t t5 = clock();
  maze_Time = (float) (t5 - t1) / CLOCKS_PER_SEC;
  logger->info(GRT, 111, "Final number of vias: {}", numVia);
  logger->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));

  NetRouteMap routes = getRoutes();

  netEO.clear();

  updateDbCongestion();

  if (totalOverflow > 0 && !allowOverflow) {
    logger->error(GRT, 118, "Routing congestion too high.");
  }

  if (allowOverflow && totalOverflow > 0) {
    logger->warn(GRT, 115, "Global routing finished with overflow.");
  }

  return routes;
}

void FastRouteCore::setAlpha(float a)
{
  alpha = a;
}

void FastRouteCore::setVerbose(int v)
{
  verbose = v;
}

void FastRouteCore::setOverflowIterations(int iterations)
{
  overflowIterations = iterations;
}

void FastRouteCore::setPDRevForHighFanout(int pdRevHihgFanout)
{
  pdRevForHighFanout = pdRevHihgFanout;
}

void FastRouteCore::setAllowOverflow(bool allow)
{
  allowOverflow = allow;
}

std::vector<int> FastRouteCore::getOriginalResources()
{
  std::vector<int> original_resources;
  original_resources.resize(numLayers);
  for (int l = 0; l < numLayers; l++) {
    original_resources[l] += (vCapacity3D[l]+hCapacity3D[l])*yGrid*xGrid;
  }

  return original_resources;
}

void FastRouteCore::computeCongestionInformation()
{
  cap_per_layer.resize(numLayers);
  usage_per_layer.resize(numLayers);
  overflow_per_layer.resize(numLayers);
  max_h_overflow.resize(numLayers);
  max_v_overflow.resize(numLayers);

  for (int l = 0; l < numLayers; l++) {
    cap_per_layer[l] = 0;
    usage_per_layer[l] = 0;
    overflow_per_layer[l] = 0;
    max_h_overflow[l] = 0;
    max_v_overflow[l] = 0;

    for (int i = 0; i < yGrid; i++) {
      for (int j = 0; j < xGrid - 1; j++) {
        int grid = i * (xGrid - 1) + j + l * (xGrid - 1) * yGrid;
        cap_per_layer[l] += h_edges3D[grid].cap;
        usage_per_layer[l] += h_edges3D[grid].usage;

        int overflow = h_edges3D[grid].usage - h_edges3D[grid].cap;
        if (overflow > 0) {
          overflow_per_layer[l] += overflow;
          max_h_overflow[l] = std::max(max_h_overflow[l], overflow);
        }
      }
    }
    for (int i = 0; i < yGrid - 1; i++) {
      for (int j = 0; j < xGrid; j++) {
        int grid = i * xGrid + j + l * xGrid * (yGrid - 1);
        cap_per_layer[l] += v_edges3D[grid].cap;
        usage_per_layer[l] += v_edges3D[grid].usage;

        int overflow = v_edges3D[grid].usage - v_edges3D[grid].cap;
        if (overflow > 0) {
          overflow_per_layer[l] += overflow;
          max_v_overflow[l] = std::max(max_v_overflow[l], overflow);
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
