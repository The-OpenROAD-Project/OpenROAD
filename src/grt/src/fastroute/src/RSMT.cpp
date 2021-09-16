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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <iostream>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

struct pnt
{
  int x, y;
  int o;
};

int orderx(const pnt* a, const pnt* b)
{
  return a->x < b->x;
}

static int ordery(const pnt* a, const pnt* b)
{
  return a->y < b->y;
}

// binary search to map the new coordinates to original coordinates
static int mapxy(const int nx,
                 const std::vector<int>& xs,
                 const std::vector<int>& nxs,
                 const int d)
{
  int min = 0;
  int max = d - 1;

  while (min <= max) {
    const int mid = (min + max) / 2;
    if (nx == nxs[mid])
      return (xs[mid]);
    if (nx < nxs[mid])
      max = mid - 1;
    else
      min = mid + 1;
  }

  return -1;
}

void FastRouteCore::copyStTree(const int ind, const Tree& rsmt)
{
  const int d = rsmt.deg;
  sttrees_[ind].deg = d;
  const int numnodes = 2 * d - 2;
  const int numedges = 2 * d - 3;
  sttrees_[ind].nodes = new TreeNode[numnodes];
  sttrees_[ind].edges = new TreeEdge[numedges];

  TreeNode* treenodes = sttrees_[ind].nodes;
  TreeEdge* treeedges = sttrees_[ind].edges;

  // initialize the nbrcnt for treenodes
  const int sizeV = 2 * nets_[ind]->numPins;
  int nbrcnt[sizeV];
  for (int i = 0; i < numnodes; i++)
    nbrcnt[i] = 0;

  int edgecnt = 0;
  // original rsmt has 2*d-2 branch (one is a loop for root), in StTree 2*d-3
  // edges (no original loop)
  for (int i = 0; i < numnodes; i++) {
    const int x1 = rsmt.branch[i].x;
    const int y1 = rsmt.branch[i].y;
    const int n = rsmt.branch[i].n;
    const int x2 = rsmt.branch[n].x;
    const int y2 = rsmt.branch[n].y;
    treenodes[i].x = x1;
    treenodes[i].y = y1;
    if (i < d) {
      treenodes[i].status = 2;
    } else {
      treenodes[i].status = 0;
    }
    if (n != i) {  // not root
      treeedges[edgecnt].len = abs(x1 - x2) + abs(y1 - y2);
      // make x1 always less than x2
      if (x1 < x2) {
        treeedges[edgecnt].n1 = i;
        treeedges[edgecnt].n2 = n;
      } else {
        treeedges[edgecnt].n1 = n;
        treeedges[edgecnt].n2 = i;
      }
      treeedges[edgecnt].route.gridsX.clear();
      treeedges[edgecnt].route.gridsY.clear();
      treeedges[edgecnt].route.gridsL.clear();
      treenodes[i].nbr[nbrcnt[i]] = n;
      treenodes[i].edge[nbrcnt[i]] = edgecnt;
      treenodes[n].nbr[nbrcnt[n]] = i;
      treenodes[n].edge[nbrcnt[n]] = edgecnt;

      nbrcnt[i]++;
      nbrcnt[n]++;
      edgecnt++;
    }
    if (nbrcnt[i] > 3 || nbrcnt[n] > 3)
      logger_->error(GRT, 188, "Invalid number of node neighbors.");
  }
  if (edgecnt != numnodes - 1) {
    logger_->error(
        GRT,
        189,
        "Failure in copy tree. Number of edges: {}. Number of nodes: {}.",
        edgecnt,
        numnodes);
  }
}

void FastRouteCore::fluteNormal(const int netID,
                                const std::vector<int>& x,
                                const std::vector<int>& y,
                                const int acc,
                                const float coeffV,
                                Tree& t)
{
  const int d = x.size();

  if (d == 2) {
    t.deg = 2;
    t.length = abs(x[0] - x[1]) + abs(y[0] - y[1]);
    t.branch.resize(2);
    t.branch[0].x = x[0];
    t.branch[0].y = y[0];
    t.branch[0].n = 1;
    t.branch[1].x = x[1];
    t.branch[1].y = y[1];
    t.branch[1].n = 1;
  } else if (d == 3) {
    t.deg = 3;
    int x_max, x_min, x_mid;
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
    int y_max, y_min, y_mid;
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

    t.length = abs(x_max - x_min) + abs(y_max - y_min);
    t.branch.resize(4);
    t.branch[0].x = x[0];
    t.branch[0].y = y[0];
    t.branch[0].n = 3;
    t.branch[1].x = x[1];
    t.branch[1].y = y[1];
    t.branch[1].n = 3;
    t.branch[2].x = x[2];
    t.branch[2].y = y[2];
    t.branch[2].n = 3;
    t.branch[3].x = x_mid;
    t.branch[3].y = y_mid;
    t.branch[3].n = 3;
  } else {
    std::vector<int> xs(d);
    std::vector<int> ys(d);

    std::vector<int> tmp_xs(d);
    std::vector<int> tmp_ys(d);
    std::vector<int> s(d);
    pnt* pt = new pnt[d];
    std::vector<pnt*> ptp(d);

    for (int i = 0; i < d; i++) {
      pt[i].x = x[i];
      pt[i].y = y[i];
      ptp[i] = &pt[i];
    }

    if (d < 1000) {
      for (int i = 0; i < d - 1; i++) {
        int minval = ptp[i]->x;
        int minidx = i;
        for (int j = i + 1; j < d; j++) {
          if (minval > ptp[j]->x) {
            minval = ptp[j]->x;
            minidx = j;
          }
        }
        std::swap(ptp[i], ptp[minidx]);
      }
    } else {
      std::stable_sort(ptp.begin(), ptp.end(), orderx);
    }

    for (int i = 0; i < d; i++) {
      xs[i] = ptp[i]->x;
      ptp[i]->o = i;
    }

    // sort y to find s[]
    if (d < 1000) {
      for (int i = 0; i < d - 1; i++) {
        int minval = ptp[i]->y;
        int minidx = i;
        for (int j = i + 1; j < d; j++) {
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
      for (int i = 0; i < d; i++) {
        ys[i] = ptp[i]->y;
        s[i] = ptp[i]->o;
      }
    }

    gxs_[netID].resize(d);
    gys_[netID].resize(d);
    gs_[netID].resize(d);

    for (int i = 0; i < d; i++) {
      gxs_[netID][i] = xs[i];
      gys_[netID][i] = ys[i];
      gs_[netID][i] = s[i];

      tmp_xs[i] = xs[i] * 100;
      tmp_ys[i] = ys[i] * ((int) (100 * coeffV));
    }

    t = stt_builder_->makeSteinerTree(tmp_xs, tmp_ys, s, acc);

    for (int i = 0; i < 2 * d - 2; i++) {
      t.branch[i].x = t.branch[i].x / 100;
      t.branch[i].y = t.branch[i].y / ((int) (100 * coeffV));
    }

    delete[] pt;
  }
}

void FastRouteCore::fluteCongest(const int netID,
                                 const std::vector<int>& x,
                                 const std::vector<int>& y,
                                 const int acc,
                                 const float coeffV,
                                 Tree& t)
{
  const float coeffH = 1;
  const int d = x.size();

  if (d == 2) {
    t.deg = 2;
    t.length = abs(x[0] - x[1]) + abs(y[0] - y[1]);
    t.branch.resize(2);
    t.branch[0].x = x[0];
    t.branch[0].y = y[0];
    t.branch[0].n = 1;
    t.branch[1].x = x[1];
    t.branch[1].y = y[1];
    t.branch[1].n = 1;
  } else if (d == 3) {
    t.deg = 3;
    int x_max, x_min, x_mid;
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
    int y_max, y_min, y_mid;
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

    t.length = abs(x_max - x_min) + abs(y_max - y_min);
    t.branch.resize(4);
    t.branch[0].x = x[0];
    t.branch[0].y = y[0];
    t.branch[0].n = 3;
    t.branch[1].x = x[1];
    t.branch[1].y = y[1];
    t.branch[1].n = 3;
    t.branch[2].x = x[2];
    t.branch[2].y = y[2];
    t.branch[2].n = 3;
    t.branch[3].x = x_mid;
    t.branch[3].y = y_mid;
    t.branch[3].n = 3;
  } else {
    std::vector<int> xs(d);
    std::vector<int> ys(d);
    std::vector<int> nxs(d);
    std::vector<int> nys(d);
    std::vector<int> x_seg(d - 1);
    std::vector<int> y_seg(d - 1);
    std::vector<int> s(d);

    for (int i = 0; i < d; i++) {
      xs[i] = gxs_[netID][i];
      ys[i] = gys_[netID][i];
      s[i] = gs_[netID][i];
    }

    // get the new coordinates considering congestion
    for (int i = 0; i < d - 1; i++) {
      x_seg[i] = (xs[i + 1] - xs[i]) * 100;
      y_seg[i] = (ys[i + 1] - ys[i]) * 100;
    }

    const int height = ys[d - 1] - ys[0] + 1;  // # vertical grids the net span
    const int width = xs[d - 1] - xs[0] + 1;  // # horizontal grids the net span

    for (int i = 0; i < d - 1; i++) {
      int usageH = 0;
      for (int k = ys[0]; k <= ys[d - 1]; k++)  // all grids in the column
      {
        for (int j = xs[i]; j < xs[i + 1]; j++)
          usageH += (h_edges_[k][j].est_usage + h_edges_[k][j].red);
      }
      if (x_seg[i] != 0 && usageH != 0) {
        x_seg[i]
            *= coeffH * usageH / ((xs[i + 1] - xs[i]) * height * h_capacity_);
        x_seg[i] = std::max(1, x_seg[i]);  // the segment len is at least 1 if
                                           // original segment len > 0
      }
      int usageV = 0;
      for (int j = ys[i]; j < ys[i + 1]; j++) {
        for (int k = xs[0]; k <= xs[d - 1]; k++)  // all grids in the row
          usageV += (v_edges_[j][k].est_usage + v_edges_[j][k].red);
      }
      if (y_seg[i] != 0 && usageV != 0) {
        y_seg[i]
            *= coeffV * usageV / ((ys[i + 1] - ys[i]) * width * v_capacity_);
        y_seg[i] = std::max(1, y_seg[i]);  // the segment len is at least 1 if
                                           // original segment len > 0
      }
    }

    nxs[0] = xs[0];
    nys[0] = ys[0];
    for (int i = 0; i < d - 1; i++) {
      nxs[i + 1] = nxs[i] + x_seg[i];
      nys[i + 1] = nys[i] + y_seg[i];
    }

    t = stt_builder_->makeSteinerTree(nxs, nys, s, acc);

    // map the new coordinates back to original coordinates
    for (int i = 0; i < 2 * d - 2; i++) {
      t.branch[i].x = mapxy(t.branch[i].x, xs, nxs, d);
      t.branch[i].y = mapxy(t.branch[i].y, ys, nys, d);
    }
  }
}

bool FastRouteCore::netCongestion(const int netID)
{
  for (int j = seglist_index_[netID];
       j < seglist_index_[netID] + seglist_cnt_[netID];
       j++) {
    const Segment* seg = &seglist_[j];

    const int ymin = std::min(seg->y1, seg->y2);
    const int ymax = std::max(seg->y1, seg->y2);

    // remove L routing
    if (seg->xFirst) {
      for (int i = seg->x1; i < seg->x2; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], i, seg->y1, EdgeDirection::Horizontal);
        if (h_edges_[seg->y1][i].est_usage >= cap) {
          return true;
        }
      }
      for (int i = ymin; i < ymax; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], seg->x2, i, EdgeDirection::Vertical);
        if (v_edges_[i][seg->x2].est_usage >= cap) {
          return true;
        }
      }
    } else {
      for (int i = ymin; i < ymax; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], seg->x1, i, EdgeDirection::Vertical);
        if (v_edges_[i][seg->x1].est_usage >= cap) {
          return true;
        }
      }
      for (int i = seg->x1; i < seg->x2; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], i, seg->y2, EdgeDirection::Horizontal);
        if (h_edges_[seg->y2][i].est_usage >= cap) {
          return true;
        }
      }
    }
  }
  return false;
}

bool FastRouteCore::VTreeSuite(const int netID)
{
  int xmax = 0;
  int ymax = 0;
  int xmin = BIG_INT;
  int ymin = BIG_INT;

  const int deg = nets_[netID]->deg;
  for (int i = 0; i < deg; i++) {
    if (xmin > nets_[netID]->pinX[i]) {
      xmin = nets_[netID]->pinX[i];
    }
    if (xmax < nets_[netID]->pinX[i]) {
      xmax = nets_[netID]->pinX[i];
    }
    if (ymin > nets_[netID]->pinY[i]) {
      ymin = nets_[netID]->pinY[i];
    }
    if (ymax < nets_[netID]->pinY[i]) {
      ymax = nets_[netID]->pinY[i];
    }
  }

  if ((ymax - ymin) > 3 * (xmax - xmin)) {
    return true;
  } else {
    return false;
  }
}

bool FastRouteCore::HTreeSuite(const int netID)
{
  int xmax = 0;
  int ymax = 0;
  int xmin = BIG_INT;
  int ymin = BIG_INT;

  const int deg = nets_[netID]->deg;
  for (int i = 0; i < deg; i++) {
    if (xmin > nets_[netID]->pinX[i]) {
      xmin = nets_[netID]->pinX[i];
    }
    if (xmax < nets_[netID]->pinX[i]) {
      xmax = nets_[netID]->pinX[i];
    }
    if (ymin > nets_[netID]->pinY[i]) {
      ymin = nets_[netID]->pinY[i];
    }
    if (ymax < nets_[netID]->pinY[i]) {
      ymax = nets_[netID]->pinY[i];
    }
  }

  if (5 * (ymax - ymin) < (xmax - xmin)) {
    return true;
  } else {
    return false;
  }
}

float FastRouteCore::coeffADJ(const int netID)
{
  const int deg = nets_[netID]->deg;
  int xmax = 0;
  int ymax = 0;
  int xmin = BIG_INT;
  int ymin = BIG_INT;

  for (int i = 0; i < deg; i++) {
    if (xmin > nets_[netID]->pinX[i]) {
      xmin = nets_[netID]->pinX[i];
    }
    if (xmax < nets_[netID]->pinX[i]) {
      xmax = nets_[netID]->pinX[i];
    }
    if (ymin > nets_[netID]->pinY[i]) {
      ymin = nets_[netID]->pinY[i];
    }
    if (ymax < nets_[netID]->pinY[i]) {
      ymax = nets_[netID]->pinY[i];
    }
  }

  int Hcap = 0;
  int Vcap = 0;
  float Husage = 0;
  float Vusage = 0;
  float coef;
  if (xmin == xmax) {
    for (int j = ymin; j < ymax; j++) {
      Vcap += getEdgeCapacity(nets_[netID], xmin, j, EdgeDirection::Vertical);
      Vusage += v_edges_[j][xmin].est_usage;
    }
    coef = 1;
  } else if (ymin == ymax) {
    for (int i = xmin; i < xmax; i++) {
      Hcap += getEdgeCapacity(nets_[netID], i, ymin, EdgeDirection::Horizontal);
      Husage += h_edges_[ymin][i].est_usage;
    }
    coef = 1;
  } else {
    for (int j = ymin; j <= ymax; j++) {
      for (int i = xmin; i < xmax; i++) {
        Hcap += getEdgeCapacity(nets_[netID], i, j, EdgeDirection::Horizontal);
        Husage += h_edges_[j][i].est_usage;
      }
    }
    for (int j = ymin; j < ymax; j++) {
      for (int i = xmin; i <= xmax; i++) {
        Vcap += getEdgeCapacity(nets_[netID], i, j, EdgeDirection::Vertical);
        Vusage += v_edges_[j][i].est_usage;
      }
    }
    // (Husage * Vcap) resulting in zero is unlikely, but
    // this check was added to avoid undefined behavior if
    // the expression results in zero
    if ((Husage * Vcap) > 0) {
      coef = (Hcap * Vusage) / (Husage * Vcap);
    } else {
      coef = 1.2;
    }
  }

  if (coef < 1.2) {
    coef = 1.2;
  }

  return coef;
}

void FastRouteCore::gen_brk_RSMT(const bool congestionDriven,
                                 const bool reRoute,
                                 const bool genTree,
                                 const bool newType,
                                 const bool noADJ)
{
  Tree rsmt;
  int numShift = 0;

  int wl = 0;
  int wl1 = 0;
  int totalNumSeg = 0;

  const int flute_accuracy = 2;

  for (int i = 0; i < num_valid_nets_; i++) {
    FrNet* net = nets_[i];
    float coeffV = 1.36;

    bool cong;
    if (congestionDriven) {
      coeffV = coeffADJ(i);
      cong = netCongestion(i);

    } else if (HTreeSuite(i)) {
      coeffV = 1.2;
    }

    int d = net->deg;

    if (reRoute) {
      if (newType) {
        const TreeEdge* treeedges = sttrees_[i].edges;
        const TreeNode* treenodes = sttrees_[i].nodes;
        for (int j = 0; j < 2 * d - 3; j++) {
          // only route the non-degraded edges (len>0)
          if (sttrees_[i].edges[j].len > 0) {
            const TreeEdge* treeedge = &(treeedges[j]);
            const int n1 = treeedge->n1;
            const int n2 = treeedge->n2;
            const int x1 = treenodes[n1].x;
            const int y1 = treenodes[n1].y;
            const int x2 = treenodes[n2].x;
            const int y2 = treenodes[n2].y;
            newRipup(treeedge, treenodes, x1, y1, x2, y2, i);
          }
        }
      } else {
        // remove the est_usage due to the segments in this net
        for (int j = seglist_index_[i]; j < seglist_index_[i] + seglist_cnt_[i];
             j++) {
          ripupSegL(&seglist_[j]);
        }
      }
    }

    if (noADJ) {
      coeffV = 1.2;
    }

    // check net alpha because FastRoute has a special implementation of flute
    // TODO: move this flute implementation to SteinerTreeBuilder
    const float net_alpha = stt_builder_->getAlpha(net->db_net);
    if (net_alpha > 0.0) {
      rsmt = stt_builder_->makeSteinerTree(
          net->db_net, net->pinX, net->pinY, net->driver_idx);
    } else {
      if (congestionDriven) {
        // call congestion driven flute to generate RSMT
        if (cong) {
          fluteCongest(i, net->pinX, net->pinY, flute_accuracy, coeffV, rsmt);
        } else {
          fluteNormal(i, net->pinX, net->pinY, flute_accuracy, coeffV, rsmt);
        }
        if (d > 3) {
          numShift += edgeShiftNew(rsmt, i);
        }
      } else {
        // call FLUTE to generate RSMT for each net
        fluteNormal(i, net->pinX, net->pinY, flute_accuracy, coeffV, rsmt);
      }
    }
    if (debug_->isOn_ && debug_->steinerTree_ && net->db_net == debug_->net_) {
      steinerTreeVisualization(rsmt, net);
    }

    if (genTree) {
      copyStTree(i, rsmt);
    }

    if (net->deg != rsmt.deg) {
      d = rsmt.deg;
    }

    if (congestionDriven) {
      for (int j = 0; j < 2 * d - 3; j++)
        wl1 += sttrees_[i].edges[j].len;
    }

    int segcnt = 0;
    for (int j = 0; j < 2 * d - 2; j++) {
      const int x1 = rsmt.branch[j].x;
      const int y1 = rsmt.branch[j].y;
      const int n = rsmt.branch[j].n;
      const int x2 = rsmt.branch[n].x;
      const int y2 = rsmt.branch[n].y;

      wl += abs(x1 - x2) + abs(y1 - y2);

      if (x1 != x2 || y1 != y2) {  // the branch is not degraded (a point)
        // the position of this segment in seglist
        const int segPos = seglist_index_[i] + segcnt;
        if (x1 < x2) {
          seglist_[segPos].x1 = x1;
          seglist_[segPos].x2 = x2;
          seglist_[segPos].y1 = y1;
          seglist_[segPos].y2 = y2;
        } else {
          seglist_[segPos].x1 = x2;
          seglist_[segPos].x2 = x1;
          seglist_[segPos].y1 = y2;
          seglist_[segPos].y2 = y1;
        }

        seglist_[segPos].netID = i;
        segcnt++;
      }
    }  // loop j

    seglist_cnt_[i] = segcnt;  // the number of segments for net i
    totalNumSeg += segcnt;

    if (reRoute) {
      // update the est_usage due to the segments in this net
      newrouteL(
          i,
          RouteType::NoRoute,
          true);  // route the net with no previous route for each tree edge
    }
  }  // loop i

  if (verbose_ > 1) {
    logger_->info(GRT, 191, "Wirelength: {}, Wirelength1: {}", wl, wl1);
    logger_->info(GRT, 192, "Number of segments: {}", totalNumSeg);
    logger_->info(GRT, 193, "Number of shifts: {}", numShift);
  }
}

}  // namespace grt
