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

// estimate the routing by assigning 1 for H and V segments, 0.5 to both
// possible L for L segments
void FastRouteCore::estimateOneSeg(Segment* seg)
{
  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  const int ymin = std::min(seg->y1, seg->y2);
  const int ymax = std::max(seg->y1, seg->y2);

  // assign 0.5 to both Ls (x1,y1)-(x1,y2) + (x1,y2)-(x2,y2) + (x1,y1)-(x2,y1) +
  // (x2,y1)-(x2,y2)
  if (seg->x1 == seg->x2) {  // a vertical segment
    for (int i = ymin; i < ymax; i++) {
      v_edges_[i][seg->x1].est_usage += edgeCost;
      v_used_ggrid_.insert(std::make_pair(i, seg->x1));
    }
  } else if (seg->y1 == seg->y2) {  // a horizontal segment
    for (int i = seg->x1; i < seg->x2; i++) {
      h_edges_[seg->y1][i].est_usage += edgeCost;
      h_used_ggrid_.insert(std::make_pair(seg->y1, i));
    }
  } else {  // a diagonal segment
    for (int i = ymin; i < ymax; i++) {
      v_edges_[i][seg->x1].est_usage += edgeCost / 2.0f;
      v_edges_[i][seg->x2].est_usage += edgeCost / 2.0f;
      v_used_ggrid_.insert(std::make_pair(i, seg->x1));
      v_used_ggrid_.insert(std::make_pair(i, seg->x2));
    }
    for (int i = seg->x1; i < seg->x2; i++) {
      h_edges_[seg->y1][i].est_usage += edgeCost / 2.0f;
      h_edges_[seg->y2][i].est_usage += edgeCost / 2.0f;
      h_used_ggrid_.insert(std::make_pair(seg->y1, i));
      h_used_ggrid_.insert(std::make_pair(seg->y2, i));
    }
  }
}

void FastRouteCore::routeSegV(Segment* seg)
{
  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  const int ymin = std::min(seg->y1, seg->y2);
  const int ymax = std::max(seg->y1, seg->y2);

  for (int i = ymin; i < ymax; i++) {
    v_edges_[i][seg->x1].est_usage += edgeCost;
    v_used_ggrid_.insert(std::make_pair(i, seg->x1));
  }
}

void FastRouteCore::routeSegH(Segment* seg)
{
  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  for (int i = seg->x1; i < seg->x2; i++) {
    h_edges_[seg->y1][i].est_usage += edgeCost;
    h_used_ggrid_.insert(std::make_pair(seg->y1, i));
  }
}

// L-route, based on previous L route
void FastRouteCore::routeSegL(Segment* seg)
{
  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  const int ymin = std::min(seg->y1, seg->y2);
  const int ymax = std::max(seg->y1, seg->y2);

  if (seg->x1 == seg->x2)  // V route
    routeSegV(seg);
  else if (seg->y1 == seg->y2)  // H route
    routeSegH(seg);
  else {  // L route
    double costL1 = 0;
    double costL2 = 0;

    for (int i = ymin; i < ymax; i++) {
      const double tmp1 = v_edges_[i][seg->x1].est_usage_red() - v_capacity_lb_;
      if (tmp1 > 0)
        costL1 += tmp1;
      const double tmp2 = v_edges_[i][seg->x2].est_usage_red() - v_capacity_lb_;
      if (tmp2 > 0)
        costL2 += tmp2;
    }
    for (int i = seg->x1; i < seg->x2; i++) {
      const double tmp1 = h_edges_[seg->y2][i].est_usage_red() - h_capacity_lb_;
      if (tmp1 > 0)
        costL1 += tmp1;
      const double tmp2 = h_edges_[seg->y1][i].est_usage_red() - h_capacity_lb_;
      if (tmp2 > 0)
        costL2 += tmp2;
    }

    if (costL1 < costL2) {
      // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
      for (int i = ymin; i < ymax; i++) {
        v_edges_[i][seg->x1].est_usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, seg->x1));
      }
      for (int i = seg->x1; i < seg->x2; i++) {
        h_edges_[seg->y2][i].est_usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(seg->y2, i));
      }
      seg->xFirst = false;
    }  // if costL1<costL2
    else {
      // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
      for (int i = seg->x1; i < seg->x2; i++) {
        h_edges_[seg->y1][i].est_usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(seg->y1, i));
      }
      for (int i = ymin; i < ymax; i++) {
        v_edges_[i][seg->x2].est_usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, seg->y2));
      }
      seg->xFirst = true;
    }
  }  // else L route
}

// First time L-route, based on 0.5-0.5 estimation
void FastRouteCore::routeSegLFirstTime(Segment* seg)
{
  const int ymin = std::min(seg->y1, seg->y2);
  const int ymax = std::max(seg->y1, seg->y2);

  double costL1 = 0;
  double costL2 = 0;

  for (int i = ymin; i < ymax; i++) {
    const double tmp = v_edges_[i][seg->x1].est_usage_red() - v_capacity_lb_;
    if (tmp > 0)
      costL1 += tmp;
  }
  for (int i = ymin; i < ymax; i++) {
    const double tmp = v_edges_[i][seg->x2].est_usage_red() - v_capacity_lb_;
    if (tmp > 0)
      costL2 += tmp;
  }

  for (int i = seg->x1; i < seg->x2; i++) {
    const double tmp = h_edges_[seg->y2][i].est_usage_red() - h_capacity_lb_;
    if (tmp > 0)
      costL1 += tmp;
  }
  for (int i = seg->x1; i < seg->x2; i++) {
    const double tmp = h_edges_[seg->y1][i].est_usage_red() - h_capacity_lb_;
    if (tmp > 0)
      costL2 += tmp;
  }

  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  if (costL1 < costL2) {
    // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
    for (int i = ymin; i < ymax; i++) {
      v_edges_[i][seg->x1].est_usage += edgeCost / 2.0f;
      v_edges_[i][seg->x2].est_usage -= edgeCost / 2.0f;
      v_used_ggrid_.insert(std::make_pair(i, seg->x1));
    }
    for (int i = seg->x1; i < seg->x2; i++) {
      h_edges_[seg->y2][i].est_usage += edgeCost / 2.0f;
      h_edges_[seg->y1][i].est_usage -= edgeCost / 2.0f;
      h_used_ggrid_.insert(std::make_pair(seg->y2, i));
    }
    seg->xFirst = false;
  } else {
    // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
    for (int i = seg->x1; i < seg->x2; i++) {
      h_edges_[seg->y1][i].est_usage += edgeCost / 2.0f;
      h_edges_[seg->y2][i].est_usage -= edgeCost / 2.0f;
      h_used_ggrid_.insert(std::make_pair(seg->y1, i));
    }
    for (int i = ymin; i < ymax; i++) {
      v_edges_[i][seg->x2].est_usage += edgeCost / 2.0f;
      v_edges_[i][seg->x1].est_usage -= edgeCost / 2.0f;
      v_used_ggrid_.insert(std::make_pair(i, seg->x2));
    }
    seg->xFirst = true;
  }
}

// route all segments with L, firstTime: true, no previous route, false -
// previous is L-route
void FastRouteCore::routeLAll(bool firstTime)
{
  if (firstTime) {  // no previous route
    // estimate congestion with 0.5+0.5 L
    for (const int& netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        estimateOneSeg(&seg);
      }
    }
    // L route
    for (const int& netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        // no need to reroute the H or V segs
        if (seg.x1 != seg.x2 || seg.y1 != seg.y2)
          routeSegLFirstTime(&seg);
      }
    }
  } else {  // previous is L-route
    for (const int& netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        // no need to reroute the H or V segs
        if (seg.x1 != seg.x2 || seg.y1 != seg.y2) {
          ripupSegL(&seg);
          routeSegL(&seg);
        }
      }
    }
  }
}

// L-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteL(int netID, RouteType ripuptype, bool viaGuided)
{
  const int edgeCost = nets_[netID]->getEdgeCost();

  const int num_edges = sttrees_[netID].num_edges();
  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;

  // loop for all the tree edges
  for (int i = 0; i < num_edges; i++) {
    // only route the non-degraded edges (len>0)
    if (sttrees_[netID].edges[i].len > 0) {
      TreeEdge* treeedge = &(treeedges[i]);

      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;

      const int ymin = std::min(y1, y2);
      const int ymax = std::max(y1, y2);

      // ripup the original routing
      if (ripuptype > RouteType::NoRoute)  // it's been routed
        newRipup(treeedge, x1, y1, x2, y2, netID);

      treeedge->route.type = RouteType::LRoute;
      if (x1 == x2)  // V-routing
      {
        for (int j = ymin; j < ymax; j++) {
          v_edges_[j][x1].est_usage += edgeCost;
          v_used_ggrid_.insert(std::make_pair(j, x1));
        }
        treeedge->route.xFirst = false;
        if (treenodes[n1].status % 2 == 0) {
          treenodes[n1].status += 1;
        }
        if (treenodes[n2].status % 2 == 0) {
          treenodes[n2].status += 1;
        }
      } else if (y1 == y2)  // H-routing
      {
        for (int j = x1; j < x2; j++) {
          h_edges_[y1][j].est_usage += edgeCost;
          h_used_ggrid_.insert(std::make_pair(y1, j));
        }
        treeedge->route.xFirst = true;
        if (treenodes[n2].status < 2) {
          treenodes[n2].status += 2;
        }
        if (treenodes[n1].status < 2) {
          treenodes[n1].status += 2;
        }
      } else  // L-routing
      {
        double costL1 = 0;
        double costL2 = 0;

        if (viaGuided) {
          if (treenodes[n1].status == 0 || treenodes[n1].status == 3) {
            costL1 = costL2 = 0;
          } else if (treenodes[n1].status == 2) {
            costL1 = via_cost_;
            costL2 = 0;
          } else if (treenodes[n1].status == 1) {
            costL1 = 0;
            costL2 = via_cost_;
          } else if (verbose_) {
            logger_->warn(
                GRT, 179, "Wrong node status {}.", treenodes[n1].status);
          }
          if (treenodes[n2].status == 2) {
            costL2 += via_cost_;
          } else if (treenodes[n2].status == 1) {
            costL1 += via_cost_;
          }
        } else {
          costL1 = costL2 = 0;
        }

        for (int j = ymin; j < ymax; j++) {
          const double tmp1 = v_edges_[j][x1].est_usage_red() - v_capacity_lb_;
          if (tmp1 > 0)
            costL1 += tmp1;
          const double tmp2 = v_edges_[j][x2].est_usage_red() - v_capacity_lb_;
          if (tmp2 > 0)
            costL2 += tmp2;
        }
        for (int j = x1; j < x2; j++) {
          const double tmp1 = h_edges_[y2][j].est_usage_red() - h_capacity_lb_;
          if (tmp1 > 0)
            costL1 += tmp1;
          const double tmp2 = h_edges_[y1][j].est_usage_red() - h_capacity_lb_;
          if (tmp2 > 0)
            costL2 += tmp2;
        }

        if (costL1 < costL2) {
          if (treenodes[n1].status % 2 == 0) {
            treenodes[n1].status += 1;
          }
          if (treenodes[n2].status < 2) {
            treenodes[n2].status += 2;
          }

          // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
          for (int j = ymin; j < ymax; j++) {
            v_edges_[j][x1].est_usage += edgeCost;
            v_used_ggrid_.insert(std::make_pair(j, x1));
          }
          for (int j = x1; j < x2; j++) {
            h_edges_[y2][j].est_usage += edgeCost;
            h_used_ggrid_.insert(std::make_pair(y2, j));
          }
          treeedge->route.xFirst = false;
        }  // if costL1<costL2
        else {
          if (treenodes[n2].status % 2 == 0) {
            treenodes[n2].status += 1;
          }
          if (treenodes[n1].status < 2) {
            treenodes[n1].status += 2;
          }

          // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
          for (int j = x1; j < x2; j++) {
            h_edges_[y1][j].est_usage += edgeCost;
            h_used_ggrid_.insert(std::make_pair(y1, j));
          }
          for (int j = ymin; j < ymax; j++) {
            v_edges_[j][x2].est_usage += edgeCost;
            v_used_ggrid_.insert(std::make_pair(j, x2));
          }
          treeedge->route.xFirst = true;
        }

      }  // else L-routing
    }    // if non-degraded edge
    else
      sttrees_[netID].edges[i].route.type = RouteType::NoRoute;
  }  // loop i
}

// route all segments with L, firstTime: true, first newrouteLAll, false - not
// first
void FastRouteCore::newrouteLAll(bool firstTime, bool viaGuided)
{
  if (firstTime) {
    for (const int& netID : net_ids_) {
      newrouteL(netID, RouteType::NoRoute, viaGuided);  // do L-routing
    }
  } else {
    for (const int& netID : net_ids_) {
      newrouteL(netID, RouteType::LRoute, viaGuided);
    }
  }
}

void FastRouteCore::newrouteZ_edge(int netID, int edgeID)
{
  const int edgeCost = nets_[netID]->getEdgeCost();

  // only route the non-degraded edges (len>0)
  if (sttrees_[netID].edges[edgeID].len <= 0) {
    return;
  }

  auto& treeedges = sttrees_[netID].edges;
  TreeEdge* treeedge = &(treeedges[edgeID]);
  const auto& treenodes = sttrees_[netID].nodes;
  const int n1 = treeedge->n1;
  const int n2 = treeedge->n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  // Do Z-routing if not an H or V edge
  if (x1 == x2 || y1 == y2) {
    return;
  }
  // ripup the original routing
  newRipup(treeedge, x1, y1, x2, y2, netID);

  treeedge->route.type = RouteType::ZRoute;

  const int segWidth = x2 - x1;
  int ymin, ymax;
  if (y1 < y2) {
    ymin = y1;
    ymax = y2;
  } else {
    ymin = y2;
    ymax = y1;
  }

  // compute the cost for all Z routing

  for (int i = 0; i <= segWidth; i++) {
    cost_hvh_[i] = 0;
    cost_v_[i] = 0;
    cost_tb_[i] = 0;

    cost_hvh_test_[i] = 0;
    cost_v_test_[i] = 0;
    cost_tb_test_[i] = 0;
  }

  // compute the cost for all H-segs and V-segs and partial boundary seg
  // cost for V-segs
  for (int i = x1; i <= x2; i++) {
    for (int j = ymin; j < ymax; j++) {
      const double tmp = v_edges_[j][i].est_usage_red() - v_capacity_lb_;
      if (tmp > 0) {
        cost_v_[i - x1] += tmp;
        cost_v_test_[i - x1] += HCOST;
      } else {
        cost_v_test_[i - x1] += tmp;
      }
    }
  }
  // cost for Top&Bot boundary segs (form Z with V-seg)
  for (int j = x1; j < x2; j++) {
    const double tmp = h_edges_[y2][j].est_usage_red() - h_capacity_lb_;
    if (tmp > 0) {
      cost_tb_[0] += tmp;
      cost_tb_test_[0] += HCOST;
    } else {
      cost_tb_test_[0] += tmp;
    }
  }
  for (int i = 1; i <= segWidth; i++) {
    cost_tb_[i] = cost_tb_[i - 1];
    const double tmp1
        = h_edges_[y1][x1 + i - 1].est_usage_red() - h_capacity_lb_;
    if (tmp1 > 0) {
      cost_tb_[i] += tmp1;
      cost_tb_test_[i] += HCOST;
    } else {
      cost_tb_test_[i] += tmp1;
    }
    const double tmp2
        = h_edges_[y2][x1 + i - 1].est_usage_red() - h_capacity_lb_;
    if (tmp2 > 0) {
      cost_tb_[i] -= tmp2;
      cost_tb_test_[i] -= HCOST;
    } else {
      cost_tb_test_[i] -= tmp2;
    }
  }
  // compute cost for all Z routing
  double bestcost = BIG_INT;
  double btTEST = BIG_INT;
  int bestZ = 0;
  for (int i = 0; i <= segWidth; i++) {
    cost_hvh_[i] = cost_v_[i] + cost_tb_[i];
    cost_hvh_test_[i] = cost_v_test_[i] + cost_tb_test_[i];
    if (cost_hvh_[i] < bestcost) {
      bestcost = cost_hvh_[i];
      btTEST = cost_hvh_test_[i];
      bestZ = i + x1;
    } else if (cost_hvh_[i] == bestcost) {
      if (cost_hvh_test_[i] < btTEST) {
        btTEST = cost_hvh_test_[i];
        bestZ = i + x1;
      }
    }
  }

  for (int i = x1; i < bestZ; i++) {
    h_edges_[y1][i].est_usage += edgeCost;
    h_used_ggrid_.insert(std::make_pair(y1, i));
  }
  for (int i = bestZ; i < x2; i++) {
    h_edges_[y2][i].est_usage += edgeCost;
    h_used_ggrid_.insert(std::make_pair(y2, i));
  }
  for (int i = ymin; i < ymax; i++) {
    v_edges_[i][bestZ].est_usage += edgeCost;
    v_used_ggrid_.insert(std::make_pair(i, bestZ));
  }
  treeedge->route.HVH = true;
  treeedge->route.Zpoint = bestZ;
}

// Z-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteZ(int netID, int threshold)
{
  const int edgeCost = nets_[netID]->getEdgeCost();

  const int num_terminals = sttrees_[netID].num_terminals;
  const int num_edges = sttrees_[netID].num_edges();

  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;

  // loop for all the tree edges
  for (int ind = 0; ind < num_edges; ind++) {
    TreeEdge* treeedge = &(treeedges[ind]);

    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    const int x1 = treenodes[n1].x;
    const int y1 = treenodes[n1].y;
    const int x2 = treenodes[n2].x;
    const int y2 = treenodes[n2].y;

    // only route the edges with len>5
    if (sttrees_[netID].edges[ind].len > threshold) {
      if (x1 == x2 || y1 == y2) {  // skip H or V edge
        continue;
      }
      // ripup the original routing
      if (newRipupType2(
              treeedge, treenodes, x1, y1, x2, y2, num_terminals, netID)) {
        const int n1a = treenodes[n1].stackAlias;
        const int n2a = treenodes[n2].stackAlias;
        const int status1 = treenodes[n1a].status;
        const int status2 = treenodes[n2a].status;

        treeedge->route.type = RouteType::ZRoute;

        const int segWidth = x2 - x1;
        int ymin, ymax;
        bool y1Smaller;  // true - y1<y2, false y1>y2
        if (y1 < y2) {
          ymin = y1;
          ymax = y2;
          y1Smaller = true;
        } else {
          ymin = y2;
          ymax = y1;
          y1Smaller = false;
        }
        const int segHeight = ymax - ymin;

        // compute the cost for all Z routing

        if (status1 == 0 || status1 == 3) {
          for (int i = 0; i < segWidth; i++) {
            cost_hvh_[i] = 0;
            cost_hvh_test_[i] = 0;
          }
          for (int i = 0; i < segHeight; i++) {
            cost_vhv_[i] = 0;
          }
        } else if (status1 == 2) {
          for (int i = 0; i < segWidth; i++) {
            cost_hvh_[i] = 0;
            cost_hvh_test_[i] = 0;
          }
          for (int i = 0; i < segHeight; i++) {
            cost_vhv_[i] = via_cost_;
          }
        } else {
          for (int i = 0; i < segWidth; i++) {
            cost_hvh_[i] = via_cost_;
            cost_hvh_test_[i] = via_cost_;
          }
          for (int i = 0; i < segHeight; i++) {
            cost_vhv_[i] = 0;
          }
        }

        if (status2 == 2) {
          for (int i = 0; i < segHeight; i++) {
            cost_vhv_[i] += via_cost_;
          }

        } else if (status2 == 1) {
          for (int i = 0; i < segWidth; i++) {
            cost_hvh_[i] += via_cost_;
            cost_hvh_test_[i] += via_cost_;
          }
        }

        for (int i = 0; i < segWidth; i++) {
          cost_v_[i] = 0;
          cost_tb_[i] = 0;

          cost_v_test_[i] = 0;
          cost_tb_test_[i] = 0;
        }
        for (int i = 0; i < segHeight; i++) {
          cost_h_[i] = 0;
          cost_lr_[i] = 0;
        }

        // compute the cost for all H-segs and V-segs and partial boundary seg
        // cost for V-segs
        for (int i = x1; i < x2; i++) {
          for (int j = ymin; j < ymax; j++) {
            const double tmp = v_edges_[j][i].est_usage_red() - v_capacity_lb_;
            if (tmp > 0) {
              cost_v_[i - x1] += tmp;
              cost_v_test_[i - x1] += HCOST;
            } else {
              cost_v_test_[i - x1] += tmp;
            }
          }
        }
        // cost for Top&Bot boundary segs (form Z with V-seg)
        for (int j = x1; j < x2; j++) {
          const double tmp = h_edges_[y2][j].est_usage_red() - h_capacity_lb_;
          if (tmp > 0) {
            cost_tb_[0] += tmp;
            cost_tb_test_[0] += HCOST;
          } else {
            cost_tb_test_[0] += tmp;
          }
        }
        for (int i = 1; i < segWidth; i++) {
          cost_tb_[i] = cost_tb_[i - 1];
          const double tmp1
              = h_edges_[y1][x1 + i - 1].est_usage_red() - h_capacity_lb_;
          if (tmp1 > 0) {
            cost_tb_[i] += tmp1;
            cost_tb_test_[0] += HCOST;
          } else {
            cost_tb_test_[0] += tmp1;
          }
          const double tmp2
              = h_edges_[y2][x1 + i - 1].est_usage_red() - h_capacity_lb_;
          if (tmp2 > 0) {
            cost_tb_[i] -= tmp2;
            cost_tb_test_[0] -= HCOST;
          } else {
            cost_tb_test_[0] -= tmp2;
          }
        }
        // cost for H-segs
        for (int i = ymin; i < ymax; i++) {
          for (int j = x1; j < x2; j++) {
            const double tmp = h_edges_[i][j].est_usage_red() - h_capacity_lb_;
            if (tmp > 0)
              cost_h_[i - ymin] += tmp;
          }
        }
        // cost for Left&Right boundary segs (form Z with H-seg)
        if (y1Smaller) {
          for (int j = y1; j < y2; j++) {
            const double tmp = v_edges_[j][x2].est_usage_red() - v_capacity_lb_;
            if (tmp > 0)
              cost_lr_[0] += tmp;
          }
          for (int i = 1; i < segHeight; i++) {
            cost_lr_[i] = cost_lr_[i - 1];
            const double tmp1
                = v_edges_[y1 + i - 1][x1].est_usage_red() - v_capacity_lb_;
            if (tmp1 > 0)
              cost_lr_[i] += tmp1;
            const double tmp2
                = v_edges_[y1 + i - 1][x2].est_usage_red() - v_capacity_lb_;
            if (tmp2 > 0)
              cost_lr_[i] -= tmp2;
          }
        } else {
          for (int j = y2; j < y1; j++) {
            const double tmp = v_edges_[j][x1].est_usage - v_capacity_lb_;
            if (tmp > 0)
              cost_lr_[0] += tmp;
          }
          for (int i = 1; i < segHeight; i++) {
            cost_lr_[i] = cost_lr_[i - 1];
            const double tmp1
                = v_edges_[y2 + i - 1][x2].est_usage_red() - v_capacity_lb_;
            if (tmp1 > 0)
              cost_lr_[i] += tmp1;
            const double tmp2
                = v_edges_[y2 + i - 1][x1].est_usage_red() - v_capacity_lb_;
            if (tmp2 > 0)
              cost_lr_[i] -= tmp2;
          }
        }

        // compute cost for all Z routing
        bool HVH = true;  // the shape of Z routing (true - HVH, false - VHV)
        double bestcost = BIG_INT;
        double btTEST = BIG_INT;
        int bestZ = 0;
        for (int i = 0; i < segWidth; i++) {
          cost_hvh_[i] += cost_v_[i] + cost_tb_[i];
          if (cost_hvh_[i] < bestcost) {
            bestcost = cost_hvh_[i];
            btTEST = cost_hvh_test_[i];
            bestZ = i + x1;
          } else if (cost_hvh_[i] == bestcost) {
            if (cost_hvh_test_[i] < btTEST) {
              btTEST = cost_hvh_test_[i];
              bestZ = i + x1;
            }
          }
        }
        for (int i = 0; i < segHeight; i++) {
          cost_vhv_[i] += cost_h_[i] + cost_lr_[i];
          if (cost_vhv_[i] < bestcost) {
            bestcost = cost_vhv_[i];
            bestZ = i + ymin;
            HVH = false;
          }
        }

        if (HVH) {
          if (treenodes[n1a].status < 2) {
            treenodes[n1a].status += 2;
          }
          if (treenodes[n2a].status < 2) {
            treenodes[n2a].status += 2;
          }

          treenodes[n1a].hID++;
          treenodes[n2a].hID++;

          for (int i = x1; i < bestZ; i++) {
            h_edges_[y1][i].est_usage += edgeCost;
            h_used_ggrid_.insert(std::make_pair(y1, i));
          }
          for (int i = bestZ; i < x2; i++) {
            h_edges_[y2][i].est_usage += edgeCost;
            h_used_ggrid_.insert(std::make_pair(y2, i));
          }
          for (int i = ymin; i < ymax; i++) {
            v_edges_[i][bestZ].est_usage += edgeCost;
            v_used_ggrid_.insert(std::make_pair(i, bestZ));
          }
          treeedge->route.HVH = HVH;
          treeedge->route.Zpoint = bestZ;
        } else {
          if (treenodes[n2a].status % 2 == 0) {
            treenodes[n2a].status += 1;
          }
          if (treenodes[n1a].status % 2 == 0) {
            treenodes[n1a].status += 1;
          }

          treenodes[n1a].lID++;
          treenodes[n2a].lID++;
          if (y1Smaller) {
            for (int i = y1; i < bestZ; i++) {
              v_edges_[i][x1].est_usage += edgeCost;
              v_used_ggrid_.insert(std::make_pair(i, x1));
            }
            for (int i = bestZ; i < y2; i++) {
              v_edges_[i][x2].est_usage += edgeCost;
              v_used_ggrid_.insert(std::make_pair(i, x2));
            }
            for (int i = x1; i < x2; i++) {
              h_edges_[bestZ][i].est_usage += edgeCost;
              h_used_ggrid_.insert(std::make_pair(bestZ, i));
            }
            treeedge->route.HVH = HVH;
            treeedge->route.Zpoint = bestZ;
          } else {
            for (int i = y2; i < bestZ; i++) {
              v_edges_[i][x2].est_usage += edgeCost;
              v_used_ggrid_.insert(std::make_pair(i, x2));
            }
            for (int i = bestZ; i < y1; i++) {
              v_edges_[i][x1].est_usage += edgeCost;
              v_used_ggrid_.insert(std::make_pair(i, x1));
            }
            for (int i = x1; i < x2; i++) {
              h_edges_[bestZ][i].est_usage += edgeCost;
              h_used_ggrid_.insert(std::make_pair(bestZ, i));
            }
            treeedge->route.HVH = HVH;
            treeedge->route.Zpoint = bestZ;
          }
        }
      } else if (num_terminals == 2) {
        newrouteZ_edge(netID, ind);
      }
    } else if (num_terminals == 2 && sttrees_[netID].edges[ind].len > threshold
               && threshold > 4) {
      newrouteZ_edge(netID, ind);
    }
  }
}

// ripup a tree edge according to its ripup type and Z-route it
// route all segments with L, firstTime: true, first newrouteLAll, false - not
// first
void FastRouteCore::newrouteZAll(int threshold)
{
  for (const int& netID : net_ids_) {
    newrouteZ(netID, threshold);  // ripup previous route and do Z-routing
  }
}

void FastRouteCore::spiralRoute(int netID, int edgeID)
{
  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;

  const int edgeCost = nets_[netID]->getEdgeCost();

  TreeEdge* treeedge = &(treeedges[edgeID]);
  if (treeedge->len <= 0)  // only route the non-degraded edges (len>0)
  {
    sttrees_[netID].edges[edgeID].route.type = RouteType::NoRoute;
    return;
  }

  const int n1 = treeedge->n1;
  const int n2 = treeedge->n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  const int n1a = treenodes[n1].stackAlias;
  const int n2a = treenodes[n2].stackAlias;

  int ymin, ymax;
  if (y1 < y2) {
    ymin = y1;
    ymax = y2;
  } else {
    ymin = y2;
    ymax = y1;
  }

  // ripup the original routing

  treeedge->route.type = RouteType::LRoute;
  if (x1 == x2) {  // V-routing
    for (int j = ymin; j < ymax; j++) {
      v_edges_[j][x1].est_usage += edgeCost;
      v_used_ggrid_.insert(std::make_pair(j, x1));
    }
    treeedge->route.xFirst = false;
    if (treenodes[n1].status % 2 == 0) {
      treenodes[n1].status += 1;
    }
    if (treenodes[n2].status % 2 == 0) {
      treenodes[n2].status += 1;
    }

    if (treenodes[n1a].status % 2 == 0) {
      treenodes[n1a].status += 1;
    }
    if (treenodes[n2a].status % 2 == 0) {
      treenodes[n2a].status += 1;
    }
  } else if (y1 == y2) {  // H-routing
    for (int j = x1; j < x2; j++) {
      h_edges_[y1][j].est_usage += edgeCost;
      h_used_ggrid_.insert(std::make_pair(y1, j));
    }
    treeedge->route.xFirst = true;
    if (treenodes[n2].status < 2) {
      treenodes[n2].status += 2;
    }
    if (treenodes[n1].status < 2) {
      treenodes[n1].status += 2;
    }

    if (treenodes[n2a].status < 2) {
      treenodes[n2a].status += 2;
    }
    if (treenodes[n1a].status < 2) {
      treenodes[n1a].status += 2;
    }
  } else {  // L-routing
    double costL1 = 0;
    double costL2 = 0;
    if (treenodes[n1].status == 0 || treenodes[n1].status == 3) {
      costL1 = costL2 = 0;
    } else if (treenodes[n1].status == 2) {
      costL1 = via_cost_;
      costL2 = 0;
    } else if (treenodes[n1].status == 1) {
      costL1 = 0;
      costL2 = via_cost_;
    } else if (verbose_) {
      logger_->warn(GRT, 181, "Wrong node status {}.", treenodes[n1].status);
    }
    if (treenodes[n2].status == 2) {
      costL2 += via_cost_;
    } else if (treenodes[n2].status == 1) {
      costL1 += via_cost_;
    }

    for (int j = ymin; j < ymax; j++) {
      const double tmp1 = v_edges_[j][x1].est_usage_red() - v_capacity_lb_;
      if (tmp1 > 0)
        costL1 += tmp1;
      const double tmp2 = v_edges_[j][x2].est_usage_red() - v_capacity_lb_;
      if (tmp2 > 0)
        costL2 += tmp2;
    }
    for (int j = x1; j < x2; j++) {
      const double tmp1 = h_edges_[y2][j].est_usage_red() - h_capacity_lb_;
      if (tmp1 > 0)
        costL1 += tmp1;
      const double tmp2 = h_edges_[y1][j].est_usage_red() - h_capacity_lb_;
      if (tmp2 > 0)
        costL2 += tmp2;
    }

    if (costL1 < costL2) {
      if (treenodes[n1].status % 2 == 0) {
        treenodes[n1].status += 1;
      }
      if (treenodes[n2].status < 2) {
        treenodes[n2].status += 2;
      }

      if (treenodes[n1a].status % 2 == 0) {
        treenodes[n1a].status += 1;
      }
      if (treenodes[n2a].status < 2) {
        treenodes[n2a].status += 2;
      }
      treenodes[n2a].hID++;
      treenodes[n1a].lID++;

      // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
      for (int j = ymin; j < ymax; j++) {
        v_edges_[j][x1].est_usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(j, x1));
      }
      for (int j = x1; j < x2; j++) {
        h_edges_[y2][j].est_usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y2, j));
      }
      treeedge->route.xFirst = false;
    } else {
      if (treenodes[n2].status % 2 == 0) {
        treenodes[n2].status += 1;
      }
      if (treenodes[n1].status < 2) {
        treenodes[n1].status += 2;
      }

      if (treenodes[n2a].status % 2 == 0) {
        treenodes[n2a].status += 1;
      }

      if (treenodes[n1a].status < 2) {
        treenodes[n1a].status += 2;
      }
      treenodes[n1a].hID++;
      treenodes[n2a].lID++;

      // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
      for (int j = x1; j < x2; j++) {
        h_edges_[y1][j].est_usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y1, j));
      }
      for (int j = ymin; j < ymax; j++) {
        v_edges_[j][x2].est_usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(j, x2));
      }
      treeedge->route.xFirst = true;
    }
  }  // else L-routing
}

void FastRouteCore::spiralRouteAll()
{
  for (const int& netID : net_ids_) {
    auto& treenodes = sttrees_[netID].nodes;
    const int num_terminals = sttrees_[netID].num_terminals;

    int numpoints = 0;

    for (int d = 0; d < sttrees_[netID].num_nodes(); d++) {
      treenodes[d].topL = -1;
      treenodes[d].botL = num_layers_;
      // treenodes[d].l = 0;
      treenodes[d].assigned = false;
      treenodes[d].stackAlias = d;
      treenodes[d].conCNT = 0;
      treenodes[d].hID = 0;
      treenodes[d].lID = 0;
      treenodes[d].status = 0;

      if (d < num_terminals) {
        treenodes[d].botL = nets_[netID]->getPinL()[d];
        treenodes[d].topL = nets_[netID]->getPinL()[d];
        // treenodes[d].l = 0;
        treenodes[d].assigned = true;
        treenodes[d].status = 2;

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
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        const int n1 = treeedge->n1;
        const int n2 = treeedge->n2;

        treeedge->n1a = treenodes[n1].stackAlias;
        treenodes[treeedge->n1a].eID[treenodes[treeedge->n1a].conCNT] = edgeID;
        treenodes[treeedge->n1a].conCNT++;

        treeedge->n2a = treenodes[n2].stackAlias;
        treenodes[treeedge->n2a].eID[treenodes[treeedge->n2a].conCNT] = edgeID;
        (treenodes[treeedge->n2a].conCNT)++;
        treeedges[edgeID].assigned = false;
      } else {
        treeedges[edgeID].assigned = true;
      }
    }
  }

  std::queue<int> edgeQueue;
  for (const int& netID : net_ids_) {
    newRipupNet(netID);

    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;

    for (int nodeID = 0; nodeID < sttrees_[netID].num_terminals; nodeID++) {
      treenodes[nodeID].assigned = true;
      for (int k = 0; k < treenodes[nodeID].conCNT; k++) {
        const int edgeID = treenodes[nodeID].eID[k];

        if (treeedges[edgeID].assigned == false) {
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
        spiralRoute(netID, edgeID);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n2a].assigned) {
          for (int k = 0; k < treenodes[treeedge->n2a].conCNT; k++) {
            const int edgeID = treenodes[treeedge->n2a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = true;
            }
          }
          treenodes[treeedge->n2a].assigned = true;
        }
      } else {
        spiralRoute(netID, edgeID);
        treeedge->assigned = true;
        if (!treenodes[treeedge->n1a].assigned) {
          for (int k = 0; k < treenodes[treeedge->n1a].conCNT; k++) {
            const int edgeID = treenodes[treeedge->n1a].eID[k];
            if (!treeedges[edgeID].assigned) {
              edgeQueue.push(edgeID);
              treeedges[edgeID].assigned = true;
            }
          }
          treenodes[treeedge->n1a].assigned = true;
        }
      }
    }
  }

  for (const int& netID : net_ids_) {
    auto& treenodes = sttrees_[netID].nodes;

    for (int d = 0; d < sttrees_[netID].num_nodes(); d++) {
      const int na = treenodes[d].stackAlias;

      treenodes[d].status = treenodes[na].status;
    }
  }
}

void FastRouteCore::routeMonotonic(int netID,
                                   int edgeID,
                                   multi_array<double, 2>& d1,
                                   multi_array<double, 2>& d2,
                                   int threshold,
                                   int enlarge)
{
  // only route the non-degraded edges (len>0)
  if (sttrees_[netID].edges[edgeID].len <= threshold) {
    return;
  }

  auto& treeedges = sttrees_[netID].edges;
  TreeEdge* treeedge = &(treeedges[edgeID]);
  const auto& treenodes = sttrees_[netID].nodes;
  const int n1 = treeedge->n1;
  const int n2 = treeedge->n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  // ripup the original routing
  if (!newRipupCheck(treeedge, x1, y1, x2, y2, threshold, 0, netID, edgeID)) {
    return;
  }

  int xmin = std::max(x1 - enlarge, 0);
  int xmax = std::min(x_grid_ - 1, x2 + enlarge);

  int yminorig, ymaxorig;
  int ymin, ymax;
  if (y1 < y2) {
    ymin = std::max(y1 - enlarge, 0);
    ymax = std::min(y_grid_ - 1, y2 + enlarge);
    yminorig = y1;
    ymaxorig = y2;
  } else {
    ymin = std::max(y2 - enlarge, 0);
    ymax = std::min(y_grid_ - 1, y1 + enlarge);
    yminorig = y2;
    ymaxorig = y1;
  }

  if (sttrees_[netID].num_terminals > 2) {
    for (int j = 0; j < sttrees_[netID].num_nodes(); j++) {
      if (treenodes[j].x < x1) {
        xmin = x1;
      }
      if (treenodes[j].x > x2) {
        xmax = x2;
      }
      if (treenodes[j].y < yminorig) {
        ymin = yminorig;
      }
      if (treenodes[j].y > ymaxorig) {
        ymax = ymaxorig;
      }
    }
  }

  for (int j = ymin; j <= ymax; j++) {
    d1[j][xmin] = 0;
  }
  // update other columns
  for (int i = xmin; i <= xmax; i++) {
    d2[ymin][i] = 0;
  }

  for (int j = ymin; j <= ymax; j++) {
    for (int i = xmin; i < xmax; i++) {
      size_t index = h_edges_[j][i].usage_red();
      index = std::min(index, h_cost_table_.size() - 1);
      const double tmp = h_cost_table_[index];
      d1[j][i + 1] = d1[j][i] + tmp;
    }
    // update the cost of a column of grids by v-edges
  }

  for (int j = ymin; j < ymax; j++) {
    // update the cost of a column of grids by h-edges
    for (int i = xmin; i <= xmax; i++) {
      size_t index = v_edges_[j][i].usage_red();
      index = std::min(index, h_cost_table_.size() - 1);
      const double tmp = h_cost_table_[index];
      d2[j + 1][i] = d2[j][i] + tmp;
    }
    // update the cost of a column of grids by v-edges
  }

  double best = BIG_INT;
  int bestp1x = 0;
  int bestp1y = 0;
  bool BL1 = false;
  bool BL2 = false;

  for (int j = ymin; j <= ymax; j++) {
    for (int i = xmin; i <= xmax; i++) {
      const double tmp1
          = std::abs(d2[j][x1] - d2[y1][x1])
            + std::abs(d1[j][i] - d1[j][x1]);  // yfirst for point 1
      const double tmp2
          = std::abs(d2[j][i] - d2[y1][i]) + std::abs(d1[y1][i] - d1[y1][x1]);
      const double tmp3
          = std::abs(d2[y2][i] - d2[j][i]) + std::abs(d1[y2][i] - d1[y2][x2]);
      const double tmp4
          = std::abs(d2[y2][x2] - d2[j][x2])
            + std::abs(d1[j][x2] - d1[j][i]);  // xifrst for mid point

      double tmp = tmp1 + tmp4;
      bool LH1 = false;
      bool LH2 = true;

      if (tmp2 + tmp3 < tmp) {
        tmp = tmp2 + tmp3;
        LH1 = true;
        LH2 = false;
      }

      if (tmp1 + tmp3 + via_cost_ < tmp) {
        LH1 = false;
        LH2 = false;
        tmp = tmp1 + tmp3 + via_cost_;
      }

      if (tmp2 + tmp4 + via_cost_ < tmp) {
        LH1 = true;
        LH2 = true;
        tmp = tmp2 + tmp4 + via_cost_;
      }

      if (tmp < best) {
        bestp1x = i;
        bestp1y = j;
        BL1 = LH1;
        BL2 = LH2;
        best = tmp;
      }
    }
  }
  int cnt = 0;
  std::vector<short int>& gridsX = treeedge->route.gridsX;
  gridsX.resize(x_range_ + y_range_);
  std::vector<short int>& gridsY = treeedge->route.gridsY;
  gridsY.resize(x_range_ + y_range_);
  const int edgeCost = nets_[netID]->getEdgeCost();

  if (BL1) {
    if (bestp1x > x1) {
      for (int i = x1; i < bestp1x; i++) {
        gridsX[cnt] = i;
        gridsY[cnt] = y1;
        h_edges_[y1][i].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y1, i));
        cnt++;
      }
    } else {
      for (int i = x1; i > bestp1x; i--) {
        gridsX[cnt] = i;
        gridsY[cnt] = y1;
        h_edges_[y1][i - 1].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y1, i - 1));
        cnt++;
      }
    }
    if (bestp1y > y1) {
      for (int i = y1; i < bestp1y; i++) {
        gridsX[cnt] = bestp1x;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[i][bestp1x].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, bestp1x));
      }
    } else {
      for (int i = y1; i > bestp1y; i--) {
        gridsX[cnt] = bestp1x;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[(i - 1)][bestp1x].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i - 1, bestp1x));
      }
    }
  } else {
    if (bestp1y > y1) {
      for (int i = y1; i < bestp1y; i++) {
        gridsX[cnt] = x1;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[i][x1].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, x1));
      }
    } else {
      for (int i = y1; i > bestp1y; i--) {
        gridsX[cnt] = x1;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[(i - 1)][x1].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i - 1, x1));
      }
    }
    if (bestp1x > x1) {
      for (int i = x1; i < bestp1x; i++) {
        gridsX[cnt] = i;
        gridsY[cnt] = bestp1y;
        h_edges_[bestp1y][i].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(bestp1y, i));
        cnt++;
      }
    } else {
      for (int i = x1; i > bestp1x; i--) {
        gridsX[cnt] = i;
        gridsY[cnt] = bestp1y;
        h_edges_[bestp1y][(i - 1)].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(bestp1y, i - 1));
        cnt++;
      }
    }
  }

  if (BL2) {
    if (bestp1x < x2) {
      for (int i = bestp1x; i < x2; i++) {
        gridsX[cnt] = i;
        gridsY[cnt] = bestp1y;
        h_edges_[bestp1y][i].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(bestp1y, i));
        cnt++;
      }
    } else {
      for (int i = bestp1x; i > x2; i--) {
        gridsX[cnt] = i;
        gridsY[cnt] = bestp1y;
        h_edges_[bestp1y][i - 1].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(bestp1y, i - 1));
        cnt++;
      }
    }

    if (y2 > bestp1y) {
      for (int i = bestp1y; i < y2; i++) {
        gridsX[cnt] = x2;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[i][x2].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, x2));
      }
    } else {
      for (int i = bestp1y; i > y2; i--) {
        gridsX[cnt] = x2;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[(i - 1)][x2].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i - 1, x2));
      }
    }
  } else {
    if (y2 > bestp1y) {
      for (int i = bestp1y; i < y2; i++) {
        gridsX[cnt] = bestp1x;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[i][bestp1x].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i, bestp1x));
      }
    } else {
      for (int i = bestp1y; i > y2; i--) {
        gridsX[cnt] = bestp1x;
        gridsY[cnt] = i;
        cnt++;
        v_edges_[(i - 1)][bestp1x].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(i - 1, bestp1x));
      }
    }
    if (x2 > bestp1x) {
      for (int i = bestp1x; i < x2; i++) {
        gridsX[cnt] = i;
        gridsY[cnt] = y2;
        h_edges_[y2][i].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y2, i));
        cnt++;
      }
    } else {
      for (int i = bestp1x; i > x2; i--) {
        gridsX[cnt] = i;
        gridsY[cnt] = y2;
        h_edges_[y2][(i - 1)].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y2, i - 1));
        cnt++;
      }
    }
  }

  gridsX[cnt] = x2;
  gridsY[cnt] = y2;
  cnt++;

  treeedge->route.routelen = cnt - 1;

  gridsX.resize(cnt);
  gridsY.resize(cnt);
}

void FastRouteCore::routeMonotonicAll(int threshold,
                                      int expand,
                                      float logis_cof)
{
  debugPrint(logger_,
             GRT,
             "patternRouting",
             1,
             "{} threshold, {} expand.",
             threshold,
             expand);

  h_cost_table_.resize(10 * h_capacity_);

  const int forange = 10 * h_capacity_;
  for (int i = 0; i < forange; i++) {
    h_cost_table_[i]
        = costheight_ / (exp((double) (h_capacity_ - i) * logis_cof) + 1) + 1;
  }

  multi_array<double, 2> d1(boost::extents[y_range_][x_range_]);
  multi_array<double, 2> d2(boost::extents[y_range_][x_range_]);

  for (const int& netID : net_ids_) {
    const int numEdges = sttrees_[netID].num_edges();
    for (int edgeID = 0; edgeID < numEdges; edgeID++) {
      routeMonotonic(netID,
                     edgeID,
                     d1,
                     d2,
                     threshold,
                     expand);  // ripup previous route and do Monotonic routing
    }
  }
  h_cost_table_.clear();
}

void FastRouteCore::newrouteLInMaze(int netID)
{
  const int num_edges = sttrees_[netID].num_edges();
  auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;

  const int edgeCost = nets_[netID]->getEdgeCost();

  // loop for all the tree edges
  for (int i = 0; i < num_edges; i++) {
    if (sttrees_[netID].edges[i].len <= 0) {
      // only route the non-degraded edges (len>0)
      sttrees_[netID].edges[i].route.type = RouteType::NoRoute;
      continue;
    }
    TreeEdge* treeedge = &(treeedges[i]);

    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    const int x1 = treenodes[n1].x;
    const int y1 = treenodes[n1].y;
    const int x2 = treenodes[n2].x;
    const int y2 = treenodes[n2].y;

    int ymin, ymax;
    if (y1 < y2) {
      ymin = y1;
      ymax = y2;
    } else {
      ymin = y2;
      ymax = y1;
    }

    treeedge->route.type = RouteType::LRoute;
    if (x1 == x2) {  // V-routing
      for (int j = ymin; j < ymax; j++) {
        v_edges_[j][x1].usage += edgeCost;
        v_used_ggrid_.insert(std::make_pair(j, x1));
      }
      treeedge->route.xFirst = false;
    } else if (y1 == y2) {  // H-routing
      for (int j = x1; j < x2; j++) {
        h_edges_[y1][j].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(y1, j));
      }
      treeedge->route.xFirst = true;
    } else {  // L-routing
      int costL1 = 0;
      int costL2 = 0;

      for (int j = ymin; j < ymax; j++) {
        const int tmp1 = v_edges_[j][x1].usage_red() - v_capacity_lb_;
        if (tmp1 > 0)
          costL1 += tmp1;
        const int tmp2 = v_edges_[j][x2].usage_red() - v_capacity_lb_;
        if (tmp2 > 0)
          costL2 += tmp2;
      }
      for (int j = x1; j < x2; j++) {
        const int tmp1 = h_edges_[y2][j].usage_red() - h_capacity_lb_;
        if (tmp1 > 0)
          costL1 += tmp1;
        const int tmp2 = h_edges_[y1][j].usage_red() - h_capacity_lb_;
        if (tmp2 > 0)
          costL2 += tmp2;
      }

      if (costL1 < costL2) {
        // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
        for (int j = ymin; j < ymax; j++) {
          v_edges_[j][x1].usage += edgeCost;
          v_used_ggrid_.insert(std::make_pair(j, x1));
        }
        for (int j = x1; j < x2; j++) {
          h_edges_[y2][j].usage += edgeCost;
          h_used_ggrid_.insert(std::make_pair(y2, j));
        }
        treeedge->route.xFirst = false;
      } else {
        // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
        for (int j = x1; j < x2; j++) {
          h_edges_[y1][j].usage += edgeCost;
          h_used_ggrid_.insert(std::make_pair(y1, j));
        }
        for (int j = ymin; j < ymax; j++) {
          v_edges_[j][x2].usage += edgeCost;
          v_used_ggrid_.insert(std::make_pair(j, x2));
        }
        treeedge->route.xFirst = true;
      }
    }  // else L-routing
  }    // loop i
}

}  // namespace grt
