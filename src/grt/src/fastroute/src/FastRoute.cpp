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
  num_nets_ = 0;
  maxNetDegree = 0;
  logger = log;
  db_ = db;
  allow_overflow_ = false;
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
    for (int i = 0; i < num_nets_; i++) {
      if (nets[i])
        delete nets[i];
      nets[i] = nullptr;
    }
  
    delete[] nets;
    nets = nullptr;
  }

  h_edges.clear();
  v_edges.clear();
  seglist.clear();
  seglistIndex.clear();
  seglistCnt.clear();

  gxs.clear();
  gys.clear();
  gs.clear();

  treeOrderPV.clear();
  treeOrderCong.clear();

  h_edges3D.clear();
  v_edges3D.clear();

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

  if (heap1)
    delete[] heap1;
  if (heap2)
    delete[] heap2;

  pop_heap2.clear();
  heap1 = nullptr;
  heap2 = nullptr;

  netEO.clear();

  xcor.clear();
  ycor.clear();
  dcor.clear();

  HV.resize(boost::extents[0][0]);
  hyperV.resize(boost::extents[0][0]);
  hyperH.resize(boost::extents[0][0]);
  corrEdge.resize(boost::extents[0][0]);
  
  inRegion.resize(boost::extents[0][0]);

  d13D.resize(boost::extents[0][0][0]);
  d23D.resize(boost::extents[0][0][0]);
  d1.resize(boost::extents[0][0]);
  d2.resize(boost::extents[0][0]);

  vCapacity3D.clear();
  hCapacity3D.clear();

  gridHs.clear();
  gridVs.clear();

  layerGrid.resize(boost::extents[0][0]);
  viaLink.resize(boost::extents[0][0]);

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
  num_nets_ = 0;
}

void FastRouteCore::setGridsAndLayers(int x, int y, int nLayers)
{
  xGrid = x;
  yGrid = y;
  numLayers = nLayers;
  if (std::max(xGrid, yGrid) >= 1000) {
    XRANGE = std::max(xGrid, yGrid);
    YRANGE = std::max(xGrid, yGrid);
  } else {
    XRANGE = 1000;
    YRANGE = 1000;
  }

  vCapacity3D.resize(numLayers);
  hCapacity3D.resize(numLayers);

  for (int i = 0; i < numLayers; i++) {
    vCapacity3D[i] = 0;
    hCapacity3D[i] = 0;
  }

  gridHs.resize(numLayers);
  gridVs.resize(numLayers);

  layerGrid.resize(boost::extents[numLayers][MAXLEN]);
  viaLink.resize(boost::extents[numLayers][MAXLEN]);

  d13D.resize(boost::extents[numLayers][YRANGE][XRANGE]);
  d23D.resize(boost::extents[numLayers][YRANGE][XRANGE]);

  d1.resize(boost::extents[YRANGE][XRANGE]);
  d2.resize(boost::extents[YRANGE][XRANGE]);

  HV.resize(boost::extents[YRANGE][XRANGE]);
  hyperV.resize(boost::extents[YRANGE][XRANGE]);
  hyperH.resize(boost::extents[YRANGE][XRANGE]);
  corrEdge.resize(boost::extents[YRANGE][XRANGE]);

  inRegion.resize(boost::extents[YRANGE][XRANGE]);

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

void FastRouteCore::setNumberNets(int nNets)
{
  num_nets_ = nNets;
  nets = new FrNet*[num_nets_];
  for (int i = 0; i < num_nets_; i++)
    nets[i] = new FrNet;
  seglistIndex.resize(num_nets_ + 1);
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
  layer_orientation_ = x;
}

void FastRouteCore::addPin(int netID, int x, int y, int layer)
{
  FrNet* net = nets[netID];
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
                          std::vector<int> edge_cost_per_layer)
{
  int netID = newnetID;
  FrNet* net = nets[newnetID];
  pinInd = num_pins;
  MD = std::max(MD, pinInd);
  net->db_net = db_net;
  net->numPins = num_pins;
  net->deg = pinInd;
  net->alpha = alpha;
  net->is_clock = is_clock;
  net->driver_idx = driver_idx;
  net->edgeCost = cost;
  net->edge_cost_per_layer = edge_cost_per_layer;

  seglistIndex[newnetID] = segcount;
  newnetID++;
  // at most (2*num_pins-2) nodes -> (2*num_pins-3) segs for a net
  segcount += 2 * pinInd - 3;
  return netID;
}

void FastRouteCore::init_usage()
{
  for (int i = 0; i < yGrid * (xGrid - 1); i++)
    h_edges[i].usage = 0;
  for (int i = 0; i < (yGrid - 1) * xGrid; i++)
    v_edges[i].usage = 0;
}

void FastRouteCore::initEdges()
{
  const float LB = 0.9;
  const float UB = 1.3;
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

  h_edges.resize((xGrid - 1) * yGrid);
  v_edges.resize(xGrid * (yGrid - 1));

  init_usage();

  v_edges3D.resize(numLayers * xGrid * yGrid);
  h_edges3D.resize(numLayers * xGrid * yGrid);

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

  seglistCnt.resize(numValidNets);
  seglist.resize(segcount);
  sttrees = new StTree[numValidNets];
  gxs.resize(numValidNets);
  gys.resize(numValidNets);
  gs.resize(numValidNets);

  gridHV = XRANGE * YRANGE;
  gridH = (xGrid - 1) * yGrid;
  gridV = xGrid * (yGrid - 1);
  for (int k = 0; k < numLayers; k++) {
    gridHs[k] = k * gridH;
    gridVs[k] = k * gridV;
  }

  parentX1.resize(boost::extents[yGrid][xGrid]);
  parentY1.resize(boost::extents[yGrid][xGrid]);
  parentX3.resize(boost::extents[yGrid][xGrid]);
  parentY3.resize(boost::extents[yGrid][xGrid]);

  pop_heap2.resize(yGrid * XRANGE);

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
  int minoflrnd = 0;
  int bwcnt = 0;

  // TODO: check this size
  int maxPin = maxNetDegree;
  maxPin = 2 * maxPin;
  xcor.resize(maxPin);
  ycor.resize(maxPin);
  dcor.resize(maxPin);
  netEO.reserve(maxPin);

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
  // viacost = VIA;
  viacost = 0;
  gen_brk_RSMT(false, false, false, false, noADJ, logger);
  if (verbose > 1)
    logger->info(GRT, 97, "First L Route.");
  routeLAll(true);
  gen_brk_RSMT(true, true, true, false, noADJ, logger);
  getOverflow2D(&maxOverflow);
  if (verbose > 1)
    logger->info(GRT, 98, "Second L Route.");
  newrouteLAll(false, true);
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
  bool stopDEC = false;
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
    routeLVAll(newTH, enlarge, LOGIS_COF);

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
  stopDEC = false;

  slope = 20;
  L = 1;
  int cost_type = 1;

  InitLastUsage(upType);
  if (totalOverflow > 0 && overflow_iterations_ > 0) {
    logger->info(GRT, 101, "Running extra iterations to remove overflow.");
  }

  static const int max_overflow_increases_ = 15;

  // set overflow_increases as -1 since the first iteration always sum 1
  int overflow_increases = -1;
  int last_total_overflow = 0;
  while (totalOverflow > 0 &&
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

    if (totalOverflow > 2000) {
      enlarge += ESTEP1;  // ENLARGE+(i-1)*ESTEP;
      cost_step = CSTEP1;
      updateCongestionHistory(i, upType, stopDEC, max_adj);
    } else if (totalOverflow < 500) {
      cost_step = CSTEP3;
      enlarge += ESTEP3;
      ripup_threshold = -1;
      updateCongestionHistory(i, upType, stopDEC, max_adj);
    } else {
      cost_step = CSTEP2;
      enlarge += ESTEP2;
      updateCongestionHistory(i, upType, stopDEC, max_adj);
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

    mazeRouteMSMD(i,
                  enlarge,
                  costheight,
                  ripup_threshold,
                  mazeedge_Threshold,
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
        logger->info(GRT, 103, "Extra Run for hard benchmark.");
        L = 0;
        upType = 3;
        stopDEC = true;
        slope = 5;
        mazeRouteMSMD(i,
                      enlarge,
                      costheight,
                      ripup_threshold,
                      mazeedge_Threshold,
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

    if (totalOverflow > last_total_overflow) {
      overflow_increases++;
    }
    last_total_overflow = totalOverflow;
  } // end overflow iterations

  bool has_2D_overflow = totalOverflow > 0;

  if (minofl > 0) {
    logger->info(GRT, 104, "Minimal overflow {} occurring at round {}.", minofl, minoflrnd);
    copyBR();
  }

  if (overflow_increases > max_overflow_increases_) {
    logger->warn(GRT,
                 230,
                 "Congestion iterations reached the maximum number of total overflow increases.");
  }

  freeRR();

  checkUsage();

  if (verbose > 1)
    logger->info(GRT, 105, "Maze routing finished.");

  if (verbose > 1) {
    logger->report("Final 2D results:");
  }
  getOverflow2Dmaze(&maxOverflow, &tUsage);

  if (verbose > 1)
    logger->info(GRT, 106, "Layer assignment begins.");
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
      logger->info(GRT, 108, "Post-processing begins.");
    mazeRouteMSMDOrder3D(enlarge, 0, ripupTH3D, layer_orientation_);

    if (gen_brk_Time > 120) {
      mazeRouteMSMDOrder3D(enlarge, 0, 12, layer_orientation_);
    }
    if (verbose > 1)
      logger->info(GRT, 109, "Post-processing finished.\n Starting via filling.");
  }

  fillVIA();
  int finallength = getOverflow3D();
  int numVia = threeDVIA();
  checkRoute3D();

  logger->info(GRT, 111, "Final number of vias: {}", numVia);
  logger->info(GRT, 112, "Final usage 3D: {}", (finallength + 3 * numVia));

  NetRouteMap routes = getRoutes();

  netEO.clear();

  updateDbCongestion();

  if (has_2D_overflow && !allow_overflow_) {
    logger->error(GRT, 118, "Routing congestion too high.");
  }

  if (totalOverflow > 0) {
    logger->warn(GRT, 115, "Global routing finished with overflow.");
  }

  return routes;
}

void FastRouteCore::setVerbose(int v)
{
  verbose = v;
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
