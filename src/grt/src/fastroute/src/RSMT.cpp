// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

#include "AbstractFastRouteRenderer.h"
#include "DataType.h"
#include "FastRoute.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

struct Pnt
{
  int x, y;
  int o;
};

static int orderx(const Pnt* a, const Pnt* b)
{
  return a->x < b->x;
}

static int ordery(const Pnt* a, const Pnt* b)
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
    if (nx == nxs[mid]) {
      return (xs[mid]);
    }
    if (nx < nxs[mid]) {
      max = mid - 1;
    } else {
      min = mid + 1;
    }
  }

  return -1;
}

void FastRouteCore::copyStTree(const int ind, const stt::Tree& rsmt)
{
  const int d = rsmt.deg;
  const int numnodes = rsmt.branchCount();
  const int numedges = numnodes - 1;
  sttrees_[ind].num_terminals = d;
  sttrees_[ind].nodes.resize(numnodes);
  sttrees_[ind].edges.resize(numedges);

  auto& treenodes = sttrees_[ind].nodes;
  auto& treeedges = sttrees_[ind].edges;

  // initialize the nbrcnt for treenodes
  const int sizeV = 2 * nets_[ind]->getNumPins();
  std::vector<int> nbrcnt(sizeV);
  for (int i = 0; i < numnodes; i++) {
    nbrcnt[i] = 0;
  }

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
      treeedges[edgecnt].route.grids.clear();
      treenodes[i].nbr[nbrcnt[i]] = n;
      treenodes[i].edge[nbrcnt[i]] = edgecnt;
      treenodes[n].nbr[nbrcnt[n]] = i;
      treenodes[n].edge[nbrcnt[n]] = edgecnt;

      nbrcnt[i]++;
      nbrcnt[n]++;
      edgecnt++;
    }
    if (nbrcnt[i] > 3 || nbrcnt[n] > 3) {
      logger_->error(GRT, 188, "Invalid number of node neighbors.");
    }
  }

  // Map the node indices to the pin indices of the net
  std::map<odb::Point, int> pos_count;
  for (int i = 0; i < d; i++) {
    odb::Point pos{treenodes[i].x, treenodes[i].y};
    pos_count[pos]++;
    const int pin_idx = nets_[ind]->getPinIdxFromPosition(
        treenodes[i].x, treenodes[i].y, pos_count[pos]);
    sttrees_[ind].node_to_pin_idx[i] = pin_idx;
  }

  // Copy num neighbors
  for (int i = 0; i < numnodes; i++) {
    treenodes[i].nbr_count = nbrcnt[i];
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
                                stt::Tree& t)
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
        std::tie(x_mid, x_max) = std::minmax(x[1], x[2]);
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
        std::tie(x_min, x_mid) = std::minmax(x[1], x[2]);
        x_max = x[0];
      }
    }
    int y_max, y_min, y_mid;
    if (y[0] < y[1]) {
      if (y[0] < y[2]) {
        y_min = y[0];
        std::tie(y_mid, y_max) = std::minmax(y[1], y[2]);
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
        std::tie(y_min, y_mid) = std::minmax(y[1], y[2]);
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
    Pnt* pt = new Pnt[d];
    std::vector<Pnt*> ptp(d);

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
      std::ranges::stable_sort(ptp, orderx);
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
      std::ranges::stable_sort(ptp, ordery);
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

    for (auto& branch : t.branch) {
      branch.x /= 100;
      branch.y /= ((int) (100 * coeffV));
    }

    delete[] pt;
  }
}

void FastRouteCore::fluteCongest(const int netID,
                                 const std::vector<int>& x,
                                 const std::vector<int>& y,
                                 const int acc,
                                 const float coeffV,
                                 stt::Tree& t)
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
        std::tie(x_mid, x_max) = std::minmax(x[1], x[2]);
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
        std::tie(x_min, x_mid) = std::minmax(x[1], x[2]);
        x_max = x[0];
      }
    }
    int y_max, y_min, y_mid;
    if (y[0] < y[1]) {
      if (y[0] < y[2]) {
        y_min = y[0];
        std::tie(y_mid, y_max) = std::minmax(y[1], y[2]);
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
        std::tie(y_min, y_mid) = std::minmax(y[1], y[2]);
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
        for (int j = xs[i]; j < xs[i + 1]; j++) {
          usageH += graph2d_.getEstUsageRedH(j, k);
        }
      }
      if (x_seg[i] != 0 && usageH != 0) {
        x_seg[i]
            *= coeffH * usageH / ((xs[i + 1] - xs[i]) * height * h_capacity_);
        x_seg[i] = std::max(1, x_seg[i]);  // the segment len is at least 1 if
                                           // original segment len > 0
      }
      int usageV = 0;
      for (int j = ys[i]; j < ys[i + 1]; j++) {
        for (int k = xs[0]; k <= xs[d - 1]; k++) {  // all grids in the row
          usageV += graph2d_.getEstUsageRedV(k, j);
        }
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
    for (auto& branch : t.branch) {
      branch.x = mapxy(branch.x, xs, nxs, d);
      branch.y = mapxy(branch.y, ys, nys, d);
    }
  }
}

bool FastRouteCore::netCongestion(const int netID)
{
  for (const Segment& seg : seglist_[netID]) {
    const auto [ymin, ymax] = std::minmax(seg.y1, seg.y2);

    // remove L routing
    if (seg.xFirst) {
      for (int i = seg.x1; i < seg.x2; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], i, seg.y1, EdgeDirection::Horizontal);
        if (graph2d_.getEstUsageH(i, seg.y1) >= cap) {
          return true;
        }
      }
      for (int i = ymin; i < ymax; i++) {
        const int cap
            = getEdgeCapacity(nets_[netID], seg.x2, i, EdgeDirection::Vertical);
        if (graph2d_.getEstUsageV(seg.x2, i) >= cap) {
          return true;
        }
      }
    } else {
      for (int i = ymin; i < ymax; i++) {
        const int cap
            = getEdgeCapacity(nets_[netID], seg.x1, i, EdgeDirection::Vertical);
        if (graph2d_.getEstUsageV(seg.x1, i) >= cap) {
          return true;
        }
      }
      for (int i = seg.x1; i < seg.x2; i++) {
        const int cap = getEdgeCapacity(
            nets_[netID], i, seg.y2, EdgeDirection::Horizontal);
        if (graph2d_.getEstUsageH(i, seg.y2) >= cap) {
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

  const int deg = nets_[netID]->getNumPins();
  for (int i = 0; i < deg; i++) {
    xmin = std::min(xmin, nets_[netID]->getPinX(i));
    xmax = std::max(xmax, nets_[netID]->getPinX(i));
    ymin = std::min(ymin, nets_[netID]->getPinY(i));
    ymax = std::max(ymax, nets_[netID]->getPinY(i));
  }

  return (ymax - ymin) > 3 * (xmax - xmin);
}

bool FastRouteCore::HTreeSuite(const int netID)
{
  int xmax = 0;
  int ymax = 0;
  int xmin = BIG_INT;
  int ymin = BIG_INT;

  const int deg = nets_[netID]->getNumPins();
  for (int i = 0; i < deg; i++) {
    xmin = std::min(xmin, nets_[netID]->getPinX(i));
    xmax = std::max(xmax, nets_[netID]->getPinX(i));
    ymin = std::min(ymin, nets_[netID]->getPinY(i));
    ymax = std::max(ymax, nets_[netID]->getPinY(i));
  }

  return 5 * (ymax - ymin) < (xmax - xmin);
}

float FastRouteCore::coeffADJ(const int netID)
{
  const int deg = nets_[netID]->getNumPins();
  int xmax = 0;
  int ymax = 0;
  int xmin = BIG_INT;
  int ymin = BIG_INT;

  for (int i = 0; i < deg; i++) {
    xmin = std::min(xmin, nets_[netID]->getPinX(i));
    xmax = std::max(xmax, nets_[netID]->getPinX(i));
    ymin = std::min(ymin, nets_[netID]->getPinY(i));
    ymax = std::max(ymax, nets_[netID]->getPinY(i));
  }

  int Hcap = 0;
  int Vcap = 0;
  float Husage = 0;
  float Vusage = 0;
  float coef;
  if (xmin == xmax) {
    for (int j = ymin; j < ymax; j++) {
      Vcap += getEdgeCapacity(nets_[netID], xmin, j, EdgeDirection::Vertical);
      Vusage += graph2d_.getEstUsageV(xmin, j);
    }
    coef = 1;
  } else if (ymin == ymax) {
    for (int i = xmin; i < xmax; i++) {
      Hcap += getEdgeCapacity(nets_[netID], i, ymin, EdgeDirection::Horizontal);
      Husage += graph2d_.getEstUsageH(i, ymin);
    }
    coef = 1;
  } else {
    for (int j = ymin; j <= ymax; j++) {
      for (int i = xmin; i < xmax; i++) {
        Hcap += getEdgeCapacity(nets_[netID], i, j, EdgeDirection::Horizontal);
        Husage += graph2d_.getEstUsageH(i, j);
      }
    }
    for (int j = ymin; j < ymax; j++) {
      for (int i = xmin; i <= xmax; i++) {
        Vcap += getEdgeCapacity(nets_[netID], i, j, EdgeDirection::Vertical);
        Vusage += graph2d_.getEstUsageV(i, j);
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

  coef = std::max<double>(coef, 1.2);

  return coef;
}

void FastRouteCore::gen_brk_RSMT(const bool congestionDriven,
                                 const bool reRoute,
                                 const bool genTree,
                                 const bool newType,
                                 const bool noADJ)
{
  stt::Tree rsmt;
  int numShift = 0;

  int wl = 0;
  int wl1 = 0;
  int totalNumSeg = 0;

  const int flute_accuracy = 2;

  for (const int& netID : net_ids_) {
    FrNet* net = nets_[netID];

    int d = net->getNumPins();

    if (reRoute) {
      if (newType) {
        const auto& treeedges = sttrees_[netID].edges;
        const auto& treenodes = sttrees_[netID].nodes;
        for (int j = 0; j < sttrees_[netID].num_edges(); j++) {
          // only route the non-degraded edges (len>0)
          if (sttrees_[netID].edges[j].len > 0) {
            const TreeEdge* treeedge = &(treeedges[j]);
            const int n1 = treeedge->n1;
            const int n2 = treeedge->n2;
            const int x1 = treenodes[n1].x;
            const int y1 = treenodes[n1].y;
            const int x2 = treenodes[n2].x;
            const int y2 = treenodes[n2].y;
            newRipup(treeedge, x1, y1, x2, y2, netID);
          }
        }
      } else {
        // remove the est_usage due to the segments in this net
        for (auto& seg : seglist_[netID]) {
          ripupSegL(&seg);
        }
      }
    }

    // check net alpha because FastRoute has a special implementation of flute
    // TODO: move this flute implementation to SteinerTreeBuilder
    const float net_alpha = stt_builder_->getAlpha(net->getDbNet());
    if (net_alpha > 0.0) {
      rsmt = stt_builder_->makeSteinerTree(
          net->getDbNet(), net->getPinX(), net->getPinY(), net->getDriverIdx());
    } else {
      float coeffV = 1.36;

      if (congestionDriven) {
        // call congestion driven flute to generate RSMT
        bool cong;
        coeffV = noADJ ? 1.2 : coeffADJ(netID);
        cong = netCongestion(netID);
        if (cong) {
          fluteCongest(netID,
                       net->getPinX(),
                       net->getPinY(),
                       flute_accuracy,
                       coeffV,
                       rsmt);
        } else {
          fluteNormal(netID,
                      net->getPinX(),
                      net->getPinY(),
                      flute_accuracy,
                      coeffV,
                      rsmt);
        }
        if (d > 3) {
          numShift += edgeShiftNew(rsmt, netID);
        }
      } else {
        // call FLUTE to generate RSMT for each net
        if (noADJ || HTreeSuite(netID)) {
          coeffV = 1.2;
        }
        fluteNormal(netID,
                    net->getPinX(),
                    net->getPinY(),
                    flute_accuracy,
                    coeffV,
                    rsmt);
      }
    }
    if (debug_->isOn() && debug_->steinerTree
        && net->getDbNet() == debug_->net) {
      logger_->report("Steiner Tree 2D");
      steinerTreeVisualization(rsmt, net);
    }

    if (genTree) {
      copyStTree(netID, rsmt);
    }

    if (net->getNumPins() != rsmt.deg) {
      d = rsmt.deg;
    }

    if (congestionDriven) {
      for (int j = 0; j < sttrees_[netID].num_edges(); j++) {
        wl1 += sttrees_[netID].edges[j].len;
      }
    }

    for (int j = 0; j < rsmt.branchCount(); j++) {
      const int x1 = rsmt.branch[j].x;
      const int y1 = rsmt.branch[j].y;
      const int n = rsmt.branch[j].n;
      const int x2 = rsmt.branch[n].x;
      const int y2 = rsmt.branch[n].y;

      wl += abs(x1 - x2) + abs(y1 - y2);

      if (x1 != x2 || y1 != y2) {  // the branch is not degraded (a point)
        // the position of this segment in seglist
        const int8_t cost = nets_[netID]->getEdgeCost();
        if (x1 < x2) {
          seglist_[netID].emplace_back(netID, x1, y1, x2, y2, cost);
        } else {
          seglist_[netID].emplace_back(netID, x2, y2, x1, y1, cost);
        }
      }
    }  // loop j

    totalNumSeg += seglist_[netID].size();

    if (reRoute) {
      // update the est_usage due to the segments in this net
      newrouteL(
          netID,
          RouteType::NoRoute,
          true);  // route the net with no previous route for each tree edge
    }
  }  // loop i

  debugPrint(logger_,
             GRT,
             "rsmt",
             1,
             "Wirelength: {}, Wirelength1: {}\nNumber of segments: {}\nNumber "
             "of shifts: {}",
             wl,
             wl1,
             totalNumSeg,
             numShift);
}

}  // namespace grt
