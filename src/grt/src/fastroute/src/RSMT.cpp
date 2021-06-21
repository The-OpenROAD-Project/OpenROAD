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

#include "RSMT.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include "DataProc.h"
#include "DataType.h"
#include "EdgeShift.h"
#include "RipUp.h"
#include "flute.h"
#include "pdr/pdrev.h"
#include "route.h"
#include "utility.h"
#include "utl/Logger.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

#define FLUTEACCURACY 2

struct pnt
{
  DTYPE x, y;
  int o;
};

struct wire
{
  int x1, y1, x2, y2;
  int netID;
};

int orderx(const struct pnt* a, const struct pnt* b)
{
  return a->x < b->x;
}

static int ordery(const struct pnt* a, const struct pnt* b)
{
  return a->y < b->y;
}

// binary search to map the new coordinates to original coordinates
int mapxy(int nx, int xs[], int nxs[], int d)
{
  int max, min, mid;

  min = 0;
  max = d - 1;

  while (min <= max) {
    mid = (min + max) / 2;
    if (nx == nxs[mid])
      return (xs[mid]);
    if (nx < nxs[mid])
      max = mid - 1;
    else
      min = mid + 1;
  }

  debugPrint(logger, GRT, "fastroute", 3, "Fail when mapping coordinates");

  return -1;
}

void copyStTree(int ind, Tree rsmt)
{
  int i, d, numnodes, numedges;
  int n, x1, y1, x2, y2, edgecnt;
  TreeEdge* treeedges;
  TreeNode* treenodes;

  // TODO: check this size
  const int sizeV = 2 * nets[ind]->numPins;
  int nbrcnt[sizeV];

  d = rsmt.deg;
  sttrees[ind].deg = d;
  numnodes = 2 * d - 2;
  numedges = 2 * d - 3;
  sttrees[ind].nodes = new TreeNode[numnodes];
  sttrees[ind].edges = new TreeEdge[numedges];

  treenodes = sttrees[ind].nodes;
  treeedges = sttrees[ind].edges;

  // initialize the nbrcnt for treenodes
  for (i = 0; i < numnodes; i++)
    nbrcnt[i] = 0;

  edgecnt = 0;
  // original rsmt has 2*d-2 branch (one is a loop for root), in StTree 2*d-3
  // edges (no original loop)
  for (i = 0; i < numnodes; i++) {
    x1 = rsmt.branch[i].x;
    y1 = rsmt.branch[i].y;
    n = rsmt.branch[i].n;
    x2 = rsmt.branch[n].x;
    y2 = rsmt.branch[n].y;
    treenodes[i].x = x1;
    treenodes[i].y = y1;
    if (i < d) {
      treenodes[i].status = 2;
    } else {
      treenodes[i].status = 0;
    }
    if (n != i)  // not root
    {
      treeedges[edgecnt].len = ADIFF(x1, x2) + ADIFF(y1, y2);
      // make x1 always less than x2
      if (x1 < x2) {
        treeedges[edgecnt].n1 = i;
        treeedges[edgecnt].n2 = n;
      } else {
        treeedges[edgecnt].n1 = n;
        treeedges[edgecnt].n2 = i;
      }
      treenodes[i].nbr[nbrcnt[i]] = n;
      treenodes[i].edge[nbrcnt[i]] = edgecnt;
      treenodes[n].nbr[nbrcnt[n]] = i;
      treenodes[n].edge[nbrcnt[n]] = edgecnt;

      nbrcnt[i]++;
      nbrcnt[n]++;
      edgecnt++;
    }
    if (nbrcnt[i] > 3 || nbrcnt[n] > 3)
      logger->error(GRT, 188, "Invalid number of node neighbors.");
  }
  if (edgecnt != numnodes - 1) {
    logger->error(GRT, 189, "Fail in copy tree. Num edges: {}, num nodes: {}.", edgecnt, numnodes);
  }
}

void fluteNormal(int netID,
                 int d,
                 DTYPE x[],
                 DTYPE y[],
                 int acc,
                 float coeffV,
                 Tree* t)
{
  DTYPE *xs, *ys, minval, x_max, x_min, x_mid, y_max, y_min, y_mid, *tmp_xs,
      *tmp_ys;
  int* s;
  int i, j, minidx;
  struct pnt *pt, *tmpp;

  if (d == 2) {
    t->deg = 2;
    t->length = ADIFF(x[0], x[1]) + ADIFF(y[0], y[1]);
    t->branch = new Branch[2];
    t->branch[0].x = x[0];
    t->branch[0].y = y[0];
    t->branch[0].n = 1;
    t->branch[1].x = x[1];
    t->branch[1].y = y[1];
    t->branch[1].n = 1;
  } else if (d == 3) {
    t->deg = 3;
    if (x[0] < x[1]) {
      if (x[0] < x[2]) {
        x_min = x[0];
        x_mid = std::min(x[1], x[2]);
        x_max = std::max(x[1], x[2]);
      } else {
        x_min = x[2];
        x_mid = x[0];
        x_max = x[1];
      }
    } else {
      if (x[0] < x[2]) {
        x_min = x[1];
        x_mid = x[0];
        x_max = x[2];
      } else {
        x_min = std::min(x[1], x[2]);
        x_mid = std::max(x[1], x[2]);
        x_max = x[0];
      }
    }
    if (y[0] < y[1]) {
      if (y[0] < y[2]) {
        y_min = y[0];
        y_mid = std::min(y[1], y[2]);
        y_max = std::max(y[1], y[2]);
      } else {
        y_min = y[2];
        y_mid = y[0];
        y_max = y[1];
      }
    } else {
      if (y[0] < y[2]) {
        y_min = y[1];
        y_mid = y[0];
        y_max = y[2];
      } else {
        y_min = std::min(y[1], y[2]);
        y_mid = std::max(y[1], y[2]);
        y_max = y[0];
      }
    }

    t->length = ADIFF(x_max, x_min) + ADIFF(y_max, y_min);
    t->branch = new Branch[4];
    t->branch[0].x = x[0];
    t->branch[0].y = y[0];
    t->branch[0].n = 3;
    t->branch[1].x = x[1];
    t->branch[1].y = y[1];
    t->branch[1].n = 3;
    t->branch[2].x = x[2];
    t->branch[2].y = y[2];
    t->branch[2].n = 3;
    t->branch[3].x = x_mid;
    t->branch[3].y = y_mid;
    t->branch[3].n = 3;
  } else {
    stt::Tree fluteTree;
    xs = new DTYPE[d];
    ys = new DTYPE[d];

    tmp_xs = new DTYPE[d];
    tmp_ys = new DTYPE[d];

    s = new int[d];
    pt = new struct pnt[d];
    std::vector<struct pnt*> ptp(d);

    for (i = 0; i < d; i++) {
      pt[i].x = x[i];
      pt[i].y = y[i];
      ptp[i] = &pt[i];
    }

    if (d < 1000) {
      for (i = 0; i < d - 1; i++) {
        minval = ptp[i]->x;
        minidx = i;
        for (j = i + 1; j < d; j++) {
          if (minval > ptp[j]->x) {
            minval = ptp[j]->x;
            minidx = j;
          }
        }
        tmpp = ptp[i];
        ptp[i] = ptp[minidx];
        ptp[minidx] = tmpp;
      }
    } else {
      std::stable_sort(ptp.begin(), ptp.end(), orderx);
    }

    for (i = 0; i < d; i++) {
      xs[i] = ptp[i]->x;
      ptp[i]->o = i;
    }

    // sort y to find s[]
    if (d < 1000) {
      for (i = 0; i < d - 1; i++) {
        minval = ptp[i]->y;
        minidx = i;
        for (j = i + 1; j < d; j++) {
          if (minval > ptp[j]->y) {
            minval = ptp[j]->y;
            minidx = j;
          }
        }
        ys[i] = ptp[minidx]->y;
        s[i] = ptp[minidx]->o;
        ptp[minidx] = ptp[i];
      }
      ys[d - 1] = ptp[d - 1]->y;
      s[d - 1] = ptp[d - 1]->o;
    } else {
      std::stable_sort(ptp.begin(), ptp.end(), ordery);
      for (i = 0; i < d; i++) {
        ys[i] = ptp[i]->y;
        s[i] = ptp[i]->o;
      }
    }

    gxs[netID] = new DTYPE[d];
    gys[netID] = new DTYPE[d];
    gs[netID] = new DTYPE[d];

    for (i = 0; i < d; i++) {
      gxs[netID][i] = xs[i];
      gys[netID][i] = ys[i];
      gs[netID][i] = s[i];

      tmp_xs[i] = xs[i] * 100;
      tmp_ys[i] = ys[i] * ((int) (100 * coeffV));
    }

    fluteTree = stt::flutes(d, tmp_xs, tmp_ys, s, acc);
    (*t) = fluteToTree(fluteTree);

    for (i = 0; i < 2 * d - 2; i++) {
      t->branch[i].x = t->branch[i].x / 100;
      t->branch[i].y = t->branch[i].y / ((int) (100 * coeffV));
    }

    delete[] xs;
    delete[] ys;
    delete[] tmp_xs;
    delete[] tmp_ys;
    delete[] s;
    delete[] pt;
  }
}

void fluteCongest(int netID,
                  int d,
                  DTYPE x[],
                  DTYPE y[],
                  int acc,
                  float coeffV,
                  Tree* t)
{
  DTYPE *xs, *ys, *nxs, *nys, *x_seg, *y_seg, x_max, x_min, x_mid,
      y_max, y_min, y_mid;
  int* s;
  int i, j, k, grid;
  DTYPE height, width;
  int usageH, usageV;
  float coeffH = 1;
  //  float coeffV = 2;//1.36;//hCapacity/vCapacity;//1;//

  if (d == 2) {
    t->deg = 2;
    t->length = ADIFF(x[0], x[1]) + ADIFF(y[0], y[1]);
    t->branch = new Branch[2];
    t->branch[0].x = x[0];
    t->branch[0].y = y[0];
    t->branch[0].n = 1;
    t->branch[1].x = x[1];
    t->branch[1].y = y[1];
    t->branch[1].n = 1;
  } else if (d == 3) {
    t->deg = 3;
    if (x[0] < x[1]) {
      if (x[0] < x[2]) {
        x_min = x[0];
        x_mid = std::min(x[1], x[2]);
        x_max = std::max(x[1], x[2]);
      } else {
        x_min = x[2];
        x_mid = x[0];
        x_max = x[1];
      }
    } else {
      if (x[0] < x[2]) {
        x_min = x[1];
        x_mid = x[0];
        x_max = x[2];
      } else {
        x_min = std::min(x[1], x[2]);
        x_mid = std::max(x[1], x[2]);
        x_max = x[0];
      }
    }
    if (y[0] < y[1]) {
      if (y[0] < y[2]) {
        y_min = y[0];
        y_mid = std::min(y[1], y[2]);
        y_max = std::max(y[1], y[2]);
      } else {
        y_min = y[2];
        y_mid = y[0];
        y_max = y[1];
      }
    } else {
      if (y[0] < y[2]) {
        y_min = y[1];
        y_mid = y[0];
        y_max = y[2];
      } else {
        y_min = std::min(y[1], y[2]);
        y_mid = std::max(y[1], y[2]);
        y_max = y[0];
      }
    }

    t->length = ADIFF(x_max, x_min) + ADIFF(y_max, y_min);
    t->branch = new Branch[4];
    t->branch[0].x = x[0];
    t->branch[0].y = y[0];
    t->branch[0].n = 3;
    t->branch[1].x = x[1];
    t->branch[1].y = y[1];
    t->branch[1].n = 3;
    t->branch[2].x = x[2];
    t->branch[2].y = y[2];
    t->branch[2].n = 3;
    t->branch[3].x = x_mid;
    t->branch[3].y = y_mid;
    t->branch[3].n = 3;
  } else {
    stt::Tree fluteTree;
    xs = new DTYPE[d];
    ys = new DTYPE[d];
    nxs = new DTYPE[d];
    nys = new DTYPE[d];
    x_seg = new DTYPE[d - 1];
    y_seg = new DTYPE[d - 1];
    s = new int[d];

    for (i = 0; i < d; i++) {
      xs[i] = gxs[netID][i];
      ys[i] = gys[netID][i];
      s[i] = gs[netID][i];
    }

    // get the new coordinates considering congestion
    for (i = 0; i < d - 1; i++) {
      x_seg[i] = (xs[i + 1] - xs[i]) * 100;
      y_seg[i] = (ys[i + 1] - ys[i]) * 100;
    }

    height = ys[d - 1] - ys[0] + 1;  // # vertical grids the net span
    width = xs[d - 1] - xs[0] + 1;   // # horizontal grids the net span

    for (i = 0; i < d - 1; i++) {
      usageH = 0;
      for (k = ys[0]; k <= ys[d - 1]; k++)  // all grids in the column
      {
        grid = k * (xGrid - 1);
        for (j = xs[i]; j < xs[i + 1]; j++)
          usageH += (h_edges[grid + j].est_usage + h_edges[grid + j].red);
      }
      if (x_seg[i] != 0 && usageH != 0) {
        x_seg[i]
            *= coeffH * usageH / ((xs[i + 1] - xs[i]) * height * hCapacity);
        x_seg[i] = std::max(1, x_seg[i]);  // the segment len is at least 1 if
                                           // original segment len > 0
      }
      usageV = 0;
      for (j = ys[i]; j < ys[i + 1]; j++) {
        grid = j * xGrid;
        for (k = xs[0]; k <= xs[d - 1]; k++)  // all grids in the row
          usageV += (v_edges[grid + k].est_usage + v_edges[grid + k].red);
      }
      if (y_seg[i] != 0 && usageV != 0) {
        y_seg[i] *= coeffV * usageV / ((ys[i + 1] - ys[i]) * width * vCapacity);
        y_seg[i] = std::max(1, y_seg[i]);  // the segment len is at least 1 if
                                           // original segment len > 0
      }
    }

    nxs[0] = xs[0];
    nys[0] = ys[0];
    for (i = 0; i < d - 1; i++) {
      nxs[i + 1] = nxs[i] + x_seg[i];
      nys[i + 1] = nys[i] + y_seg[i];
    }

    fluteTree = stt::flutes(d, nxs, nys, s, acc);
    (*t) = fluteToTree(fluteTree);

    // map the new coordinates back to original coordinates
    for (i = 0; i < 2 * d - 2; i++) {
      t->branch[i].x = mapxy(t->branch[i].x, xs, nxs, d);
      t->branch[i].y = mapxy(t->branch[i].y, ys, nys, d);
    }

    delete[] xs;
    delete[] ys;
    delete[] nxs;
    delete[] nys;
    delete[] x_seg;
    delete[] y_seg;
    delete[] s;
  }

  // return t;
}

Bool netCongestion(int netID)
{
  int i, j;
  int grid, ymin, ymax;
  //  Bool Congested;
  Segment* seg;

  for (j = seglistIndex[netID]; j < seglistIndex[netID] + seglistCnt[netID];
       j++) {
    seg = &seglist[j];

    if (seg->y1 < seg->y2) {
      ymin = seg->y1;
      ymax = seg->y2;
    } else {
      ymin = seg->y2;
      ymax = seg->y1;
    }

    // remove L routing
    if (seg->xFirst) {
      grid = seg->y1 * (xGrid - 1);
      for (i = seg->x1; i < seg->x2; i++) {
        if (h_edges[grid + i].est_usage >= h_edges[grid + i].cap) {
          return (TRUE);
        }
      }
      for (i = ymin; i < ymax; i++) {
        if (v_edges[i * xGrid + seg->x2].est_usage
            >= v_edges[i * xGrid + seg->x2].cap) {
          return (TRUE);
        }
      }
    } else {
      for (i = ymin; i < ymax; i++) {
        if (v_edges[i * xGrid + seg->x1].est_usage
            >= v_edges[i * xGrid + seg->x1].cap) {
          return (TRUE);
        }
      }
      grid = seg->y2 * (xGrid - 1);
      for (i = seg->x1; i < seg->x2; i++) {
        if (h_edges[grid + i].est_usage >= h_edges[grid + i].cap) {
          return (TRUE);
        }
      }
    }
  }
  return (FALSE);
}

Bool VTreeSuite(int netID)
{
  int xmin, xmax, ymin, ymax;

  int i, deg;

  deg = nets[netID]->deg;
  xmax = ymax = 0;
  xmin = ymin = BIG_INT;

  for (i = 0; i < deg; i++) {
    if (xmin > nets[netID]->pinX[i]) {
      xmin = nets[netID]->pinX[i];
    }
    if (xmax < nets[netID]->pinX[i]) {
      xmax = nets[netID]->pinX[i];
    }
    if (ymin > nets[netID]->pinY[i]) {
      ymin = nets[netID]->pinY[i];
    }
    if (ymax < nets[netID]->pinY[i]) {
      ymax = nets[netID]->pinY[i];
    }
  }

  if ((ymax - ymin) > 3 * (xmax - xmin)) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

Bool HTreeSuite(int netID)
{
  int xmin, xmax, ymin, ymax;

  int i, deg;

  deg = nets[netID]->deg;
  xmax = ymax = 0;
  xmin = ymin = BIG_INT;

  for (i = 0; i < deg; i++) {
    if (xmin > nets[netID]->pinX[i]) {
      xmin = nets[netID]->pinX[i];
    }
    if (xmax < nets[netID]->pinX[i]) {
      xmax = nets[netID]->pinX[i];
    }
    if (ymin > nets[netID]->pinY[i]) {
      ymin = nets[netID]->pinY[i];
    }
    if (ymax < nets[netID]->pinY[i]) {
      ymax = nets[netID]->pinY[i];
    }
  }

  if (5 * (ymax - ymin) < (xmax - xmin)) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

float coeffADJ(int netID)
{
  int xmin, xmax, ymin, ymax, Hcap, Vcap;
  float Husage, Vusage, coef;

  int i, j, deg, grid;

  deg = nets[netID]->deg;
  xmax = ymax = 0;
  xmin = ymin = BIG_INT;
  Hcap = Vcap = 0;
  Husage = Vusage = 0;

  for (i = 0; i < deg; i++) {
    if (xmin > nets[netID]->pinX[i]) {
      xmin = nets[netID]->pinX[i];
    }
    if (xmax < nets[netID]->pinX[i]) {
      xmax = nets[netID]->pinX[i];
    }
    if (ymin > nets[netID]->pinY[i]) {
      ymin = nets[netID]->pinY[i];
    }
    if (ymax < nets[netID]->pinY[i]) {
      ymax = nets[netID]->pinY[i];
    }
  }

  if (xmin == xmax) {
    for (j = ymin; j < ymax; j++) {
      grid = j * xGrid + xmin;
      Vcap += v_edges[grid].cap;
      Vusage += v_edges[grid].est_usage;
    }
    coef = 1;
  } else if (ymin == ymax) {
    for (i = xmin; i < xmax; i++) {
      grid = ymin * (xGrid - 1) + i;
      Hcap += h_edges[grid].cap;
      Husage += h_edges[grid].est_usage;
    }
    coef = 1;
  } else {
    for (j = ymin; j <= ymax; j++) {
      for (i = xmin; i < xmax; i++) {
        grid = j * (xGrid - 1) + i;
        Hcap += h_edges[grid].cap;
        Husage += h_edges[grid].est_usage;
      }
    }
    for (j = ymin; j < ymax; j++) {
      for (i = xmin; i <= xmax; i++) {
        grid = j * xGrid + i;
        Vcap += v_edges[grid].cap;
        Vusage += v_edges[grid].est_usage;
      }
    }
    // coef  = (Husage*Vcap)/ (Hcap*Vusage);
    coef = (Hcap * Vusage) / (Husage * Vcap);
  }

  if (coef < 1.2) {
    coef = 1.2;
  }

  return (coef);
}

void gen_brk_RSMT(Bool congestionDriven,
                  Bool reRoute,
                  Bool genTree,
                  Bool newType,
                  Bool noADJ,
                  Logger* logger)
{
  int i, j, d, n, n1, n2;
  int x1, y1, x2, y2;
  int segPos, segcnt;
  Tree rsmt;
  int wl, wl1, numShift = 0;
  float coeffV;

  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  Bool cong;

  wl = wl1 = 0;
  totalNumSeg = 0;

  for (i = 0; i < numValidNets; i++) {
    coeffV = 1.36;
    int sizeV = nets[i]->numPins;
    int x[sizeV];
    int y[sizeV];

    if (congestionDriven) {
      coeffV = coeffADJ(i);
      cong = netCongestion(i);

    } else {
      if (HTreeSuite(i)) {
        coeffV = 1.2;
      }
    }

    d = nets[i]->deg;
    for (j = 0; j < d; j++) {
      x[j] = nets[i]->pinX[j];
      y[j] = nets[i]->pinY[j];
    }

    if (reRoute) {
      if (newType) {
        treeedges = sttrees[i].edges;
        treenodes = sttrees[i].nodes;
        for (j = 0; j < 2 * d - 3; j++) {
          if (sttrees[i].edges[j].len
              > 0)  // only route the non-degraded edges (len>0)
          {
            treeedge = &(treeedges[j]);
            n1 = treeedge->n1;
            n2 = treeedge->n2;
            x1 = treenodes[n1].x;
            y1 = treenodes[n1].y;
            x2 = treenodes[n2].x;
            y2 = treenodes[n2].y;
            newRipup(treeedge, treenodes, x1, y1, x2, y2, i);
          }
        }
      } else {
        // remove the est_usage due to the segments in this net
        for (j = seglistIndex[i]; j < seglistIndex[i] + seglistCnt[i]; j++) {
          ripupSegL(&seglist[j]);
        }
      }
    }

    if (noADJ) {
      coeffV = 1.2;
    }
    if (pdRevForHighFanout > 0 &&
        nets[i]->deg >= pdRevForHighFanout &&
        nets[i]->is_clock) {
      stt::Tree tree = pdr::primDijkstraRevII(nets[i]->pinX, nets[i]->pinY, nets[i]->driver_idx, nets[i]->alpha, logger);
      rsmt = fluteToTree(tree);
    } else {
      if (congestionDriven) {
        // call congestion driven flute to generate RSMT
        if (cong) {
          fluteCongest(i, d, x, y, FLUTEACCURACY, coeffV, &rsmt);
        } else {
          fluteNormal(i, d, x, y, FLUTEACCURACY, coeffV, &rsmt);
        }
        if (d > 3) {
          numShift += edgeShiftNew(&rsmt, i);
        }
      } else {
        // call FLUTE to generate RSMT for each net
        fluteNormal(i, d, x, y, FLUTEACCURACY, coeffV, &rsmt);
      }
    }

    if (genTree) {
      copyStTree(i, rsmt);
    }

    if (nets[i]->deg != rsmt.deg) {
      d = rsmt.deg;
    }

    if (congestionDriven) {
      for (j = 0; j < 2 * d - 3; j++)
        wl1 += sttrees[i].edges[j].len;
    }

    segcnt = 0;
    for (j = 0; j < 2 * d - 2; j++) {
      x1 = rsmt.branch[j].x;
      y1 = rsmt.branch[j].y;
      n = rsmt.branch[j].n;
      x2 = rsmt.branch[n].x;
      y2 = rsmt.branch[n].y;

      wl += ADIFF(x1, x2) + ADIFF(y1, y2);

      if (x1 != x2 || y1 != y2)  // the branch is not degraded (a point)
      {
        segPos = seglistIndex[i]
                 + segcnt;  // the position of this segment in seglist
        if (x1 < x2) {
          seglist[segPos].x1 = x1;
          seglist[segPos].x2 = x2;
          seglist[segPos].y1 = y1;
          seglist[segPos].y2 = y2;
        } else {
          seglist[segPos].x1 = x2;
          seglist[segPos].x2 = x1;
          seglist[segPos].y1 = y2;
          seglist[segPos].y2 = y1;
        }

        seglist[segPos].netID = i;
        segcnt++;
      }
    }  // loop j

    seglistCnt[i] = segcnt;  // the number of segments for net i
    totalNumSeg += segcnt;

    if (reRoute) {
      // update the est_usage due to the segments in this net
      newrouteL(
          i,
          NOROUTE,
          TRUE);  // route the net with no previous route for each tree edge
    }
  }  // loop i

  if (verbose > 1) {
    logger->info(GRT, 191, "Wirelength: {}, Wirelength1: {}", wl, wl1);
    logger->info(GRT, 192, "Number of segments: {}", totalNumSeg);
    logger->info(GRT, 193, "Number of shifts: {}", numShift);
  }
}

}  // namespace grt
