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
#include <time.h>

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
  int i;
  int ymin, ymax;

  int edgeCost = nets_[seg->netID]->edgeCost;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  // assign 0.5 to both Ls (x1,y1)-(x1,y2) + (x1,y2)-(x2,y2) + (x1,y1)-(x2,y1) +
  // (x2,y1)-(x2,y2)
  if (seg->x1 == seg->x2)  // a vertical segment
  {
    for (i = ymin; i < ymax; i++)
      v_edges_[i * x_grid_ + seg->x1].est_usage += edgeCost;
  } else if (seg->y1 == seg->y2)  // a horizontal segment
  {
    for (i = seg->x1; i < seg->x2; i++)
      h_edges_[seg->y1 * (x_grid_ - 1) + i].est_usage += edgeCost;
  } else  // a diagonal segment
  {
    for (i = ymin; i < ymax; i++) {
      v_edges_[i * x_grid_ + seg->x1].est_usage += edgeCost/2.0f;
      v_edges_[i * x_grid_ + seg->x2].est_usage += edgeCost/2.0f;
    }
    for (i = seg->x1; i < seg->x2; i++) {
      h_edges_[seg->y1 * (x_grid_ - 1) + i].est_usage += edgeCost/2.0f;
      h_edges_[seg->y2 * (x_grid_ - 1) + i].est_usage += edgeCost/2.0f;
    }
  }
}

void FastRouteCore::routeSegV(Segment* seg)
{
  int i;
  int ymin, ymax;

  int edgeCost = nets_[seg->netID]->edgeCost;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  for (i = ymin; i < ymax; i++)
    v_edges_[i * x_grid_ + seg->x1].est_usage += edgeCost;
}

void FastRouteCore::routeSegH(Segment* seg)
{
  int i;

  int edgeCost = nets_[seg->netID]->edgeCost;

  for (i = seg->x1; i < seg->x2; i++)
    h_edges_[seg->y1 * (x_grid_ - 1) + i].est_usage += edgeCost;
}

// L-route, based on previous L route
void FastRouteCore::routeSegL(Segment* seg)
{
  int i, grid, grid1;
  float costL1, costL2, tmp;
  int ymin, ymax;

  int edgeCost = nets_[seg->netID]->edgeCost;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  if (seg->x1 == seg->x2)  // V route
    routeSegV(seg);
  else if (seg->y1 == seg->y2)  // H route
    routeSegH(seg);
  else  // L route
  {
    costL1 = costL2 = 0;

    for (i = ymin; i < ymax; i++) {
      grid = i * x_grid_;
      tmp = v_edges_[grid + seg->x1].red + v_edges_[grid + seg->x1].est_usage
            - v_capacity_lb_;
      if (tmp > 0)
        costL1 += tmp;
      tmp = v_edges_[grid + seg->x2].red + v_edges_[grid + seg->x2].est_usage
            - v_capacity_lb_;
      if (tmp > 0)
        costL2 += tmp;
    }
    grid = seg->y2 * (x_grid_ - 1);
    grid1 = seg->y1 * (x_grid_ - 1);
    for (i = seg->x1; i < seg->x2; i++) {
      tmp = h_edges_[grid + i].red + h_edges_[grid + i].est_usage - h_capacity_lb_;
      if (tmp > 0)
        costL1 += tmp;
      tmp = h_edges_[grid1 + i].red + h_edges_[grid1 + i].est_usage
            - h_capacity_lb_;
      if (tmp > 0)
        costL2 += tmp;
    }

    if (costL1 < costL2) {
      // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
      for (i = ymin; i < ymax; i++) {
        v_edges_[i * x_grid_ + seg->x1].est_usage += edgeCost;
      }
      grid = seg->y2 * (x_grid_ - 1);
      for (i = seg->x1; i < seg->x2; i++) {
        h_edges_[grid + i].est_usage += edgeCost;
      }
      seg->xFirst = false;
    }  // if costL1<costL2
    else {
      // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
      grid = seg->y1 * (x_grid_ - 1);
      for (i = seg->x1; i < seg->x2; i++) {
        h_edges_[grid + i].est_usage += edgeCost;
      }
      for (i = ymin; i < ymax; i++) {
        v_edges_[i * x_grid_ + seg->x2].est_usage += edgeCost;
      }
      seg->xFirst = true;
    }
  }  // else L route
}

// First time L-route, based on 0.5-0.5 estimation
void FastRouteCore::routeSegLFirstTime(Segment* seg)
{
  int i, vedge, hedge;
  float costL1, costL2, tmp;
  int ymin, ymax;

  int edgeCost = nets_[seg->netID]->edgeCost;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  costL1 = costL2 = 0;

  for (i = ymin; i < ymax; i++) {
    vedge = i * x_grid_ + seg->x1;
    tmp = v_edges_[vedge].red + v_edges_[vedge].est_usage - v_capacity_lb_;
    if (tmp > 0)
      costL1 += tmp;
  }
  for (i = ymin; i < ymax; i++) {
    vedge = i * x_grid_ + seg->x2;
    tmp = v_edges_[vedge].red + v_edges_[vedge].est_usage - v_capacity_lb_;
    if (tmp > 0)
      costL2 += tmp;
  }

  for (i = seg->x1; i < seg->x2; i++) {
    hedge = seg->y2 * (x_grid_ - 1) + i;
    tmp = h_edges_[hedge].red + h_edges_[hedge].est_usage - h_capacity_lb_;
    if (tmp > 0)
      costL1 += tmp;
  }
  for (i = seg->x1; i < seg->x2; i++) {
    hedge = seg->y1 * (x_grid_ - 1) + i;
    tmp = h_edges_[hedge].red + h_edges_[hedge].est_usage - h_capacity_lb_;
    if (tmp > 0)
      costL2 += tmp;
  }

  if (costL1 < costL2) {
    // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
    for (i = ymin; i < ymax; i++) {
      vedge = i * x_grid_ + seg->x1;
      v_edges_[vedge].est_usage += edgeCost/2.0f;
      vedge += seg->x2 - seg->x1;
      v_edges_[vedge].est_usage -= edgeCost/2.0f;
    }
    for (i = seg->x1; i < seg->x2; i++) {
      hedge = seg->y2 * (x_grid_ - 1) + i;
      h_edges_[hedge].est_usage += edgeCost/2.0f;
      hedge = seg->y1 * (x_grid_ - 1) + i;
      h_edges_[hedge].est_usage -= edgeCost/2.0f;
    }
    seg->xFirst = false;
  } else {
    // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
    for (i = seg->x1; i < seg->x2; i++) {
      hedge = seg->y1 * (x_grid_ - 1) + i;
      h_edges_[hedge].est_usage += edgeCost/2.0f;
      hedge = seg->y2 * (x_grid_ - 1) + i;
      h_edges_[hedge].est_usage -= edgeCost/2.0f;
    }
    for (i = ymin; i < ymax; i++) {
      vedge = i * x_grid_ + seg->x2;
      v_edges_[vedge].est_usage += edgeCost/2.0f;
      vedge += seg->x1 - seg->x2;
      v_edges_[vedge].est_usage -= edgeCost/2.0f;
    }
    seg->xFirst = true;
  }
}

// route all segments with L, firstTime: true, no previous route, false -
// previous is L-route
void FastRouteCore::routeLAll(bool firstTime)
{
  int i, j;

  if (firstTime)  // no previous route
  {
    // estimate congestion with 0.5+0.5 L
    for (i = 0; i < num_valid_nets_; i++) {
      for (j = seglist_index_[i]; j < seglist_index_[i] + seglist_cnt_[i]; j++) {
        estimateOneSeg(&seglist_[j]);
      }
    }
    // L route
    for (i = 0; i < num_valid_nets_; i++) {
      for (j = seglist_index_[i]; j < seglist_index_[i] + seglist_cnt_[i]; j++) {
        // no need to reroute the H or V segs
        if (seglist_[j].x1 != seglist_[j].x2 || seglist_[j].y1 != seglist_[j].y2)
          routeSegLFirstTime(&seglist_[j]);
      }
    }
  } else  // previous is L-route
  {
    for (i = 0; i < num_valid_nets_; i++) {
      for (j = seglist_index_[i]; j < seglist_index_[i] + seglist_cnt_[i]; j++) {
        // no need to reroute the H or V segs
        if (seglist_[j].x1 != seglist_[j].x2 || seglist_[j].y1 != seglist_[j].y2) {
          ripupSegL(&seglist_[j]);
          routeSegL(&seglist_[j]);
        }
      }
    }
  }
}

// L-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteL(int netID, RouteType ripuptype, bool viaGuided)
{
  int i, j, d, n1, n2, x1, y1, x2, y2, grid, grid1;
  float costL1 = 0;
  float costL2 = 0;
  float tmp;
  int ymin, ymax;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int edgeCost = nets_[netID]->edgeCost;

  d = sttrees_[netID].deg;
  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;

  // loop for all the tree edges (2*d-3)
  for (i = 0; i < 2 * d - 3; i++) {
    if (sttrees_[netID].edges[i].len
        > 0)  // only route the non-degraded edges (len>0)
    {
      treeedge = &(treeedges[i]);

      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;

      if (y1 < y2) {
        ymin = y1;
        ymax = y2;
      } else {
        ymin = y2;
        ymax = y1;
      }

      // ripup the original routing
      if (ripuptype > RouteType::NoRoute)  // it's been routed
        newRipup(treeedge, treenodes, x1, y1, x2, y2, netID);

      treeedge->route.type = RouteType::LRoute;
      if (x1 == x2)  // V-routing
      {
        for (j = ymin; j < ymax; j++)
          v_edges_[j * x_grid_ + x1].est_usage += edgeCost;
        treeedge->route.xFirst = false;
        if (treenodes[n1].status % 2 == 0) {
          treenodes[n1].status += 1;
        }
        if (treenodes[n2].status % 2 == 0) {
          treenodes[n2].status += 1;
        }
      } else if (y1 == y2)  // H-routing
      {
        for (j = x1; j < x2; j++)
          h_edges_[y1 * (x_grid_ - 1) + j].est_usage += edgeCost;
        treeedge->route.xFirst = true;
        if (treenodes[n2].status < 2) {
          treenodes[n2].status += 2;
        }
        if (treenodes[n1].status < 2) {
          treenodes[n1].status += 2;
        }
      } else  // L-routing
      {
        if (viaGuided) {
          if (treenodes[n1].status == 0 || treenodes[n1].status == 3) {
            costL1 = costL2 = 0;
          } else if (treenodes[n1].status == 2) {
            costL1 = via_cost_;
            costL2 = 0;
          } else if (treenodes[n1].status == 1) {
            costL1 = 0;
            costL2 = via_cost_;
          } else {
            logger_->warn(GRT, 179, "Wrong node status {}.", treenodes[n1].status);
          }
          if (treenodes[n2].status == 2) {
            costL2 += via_cost_;
          } else if (treenodes[n2].status == 1) {
            costL1 += via_cost_;
          }
        } else {
          costL1 = costL2 = 0;
        }

        for (j = ymin; j < ymax; j++) {
          grid = j * x_grid_;
          tmp = v_edges_[grid + x1].est_usage - v_capacity_lb_
                + v_edges_[grid + x1].red;
          if (tmp > 0)
            costL1 += tmp;
          tmp = v_edges_[grid + x2].est_usage - v_capacity_lb_
                + v_edges_[grid + x2].red;
          if (tmp > 0)
            costL2 += tmp;
        }
        grid = y2 * (x_grid_ - 1);
        grid1 = y1 * (x_grid_ - 1);
        for (j = x1; j < x2; j++) {
          tmp = h_edges_[grid + j].est_usage - h_capacity_lb_
                + h_edges_[grid + j].red;
          if (tmp > 0)
            costL1 += tmp;
          tmp = h_edges_[grid1 + j].est_usage - h_capacity_lb_
                + h_edges_[grid1 + j].red;
          if (tmp > 0)
            costL2 += tmp;
        }

        if (costL1 < costL2) {
          if (treenodes[n1].status % 2 == 0) {
            treenodes[n1].status += 1;
          }
          if (treenodes[n2].status < 2) {
            treenodes[n2].status += 2;
          }

          // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
          for (j = ymin; j < ymax; j++) {
            v_edges_[j * x_grid_ + x1].est_usage += edgeCost;
          }
          grid = y2 * (x_grid_ - 1);
          for (j = x1; j < x2; j++) {
            h_edges_[grid + j].est_usage += edgeCost;
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
          grid = y1 * (x_grid_ - 1);
          for (j = x1; j < x2; j++) {
            h_edges_[grid + j].est_usage += edgeCost;
          }
          for (j = ymin; j < ymax; j++) {
            v_edges_[j * x_grid_ + x2].est_usage += edgeCost;
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
  int i;

  if (firstTime) {
    for (i = 0; i < num_valid_nets_; i++) {
      newrouteL(i, RouteType::NoRoute, viaGuided);  // do L-routing
    }
  } else {
    for (i = 0; i < num_valid_nets_; i++) {
      newrouteL(i, RouteType::LRoute, viaGuided);
    }
  }
}

void FastRouteCore::newrouteZ_edge(int netID, int edgeID)
{
  int i, j, n1, n2, x1, y1, x2, y2, segWidth, bestZ, grid, grid1, grid2, ymin,
      ymax;
  float tmp, bestcost, btTEST;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int edgeCost = nets_[netID]->edgeCost;

  if (sttrees_[netID].edges[edgeID].len
      > 0)  // only route the non-degraded edges (len>0)
  {
    treeedges = sttrees_[netID].edges;
    treeedge = &(treeedges[edgeID]);
    treenodes = sttrees_[netID].nodes;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;

    if (x1 != x2 || y1 != y2)  // not H or V edge, do Z-routing (if H or V edge,
                               // no need to reroute)
    {
      // ripup the original routing
      newRipup(treeedge, treenodes, x1, y1, x2, y2, netID);

      treeedge->route.type = RouteType::ZRoute;

      segWidth = x2 - x1;
      if (y1 < y2) {
        ymin = y1;
        ymax = y2;
      } else {
        ymin = y2;
        ymax = y1;
      }

      // compute the cost for all Z routing

      for (i = 0; i <= segWidth; i++) {
        cost_hvh_[i] = 0;
        cost_v_[i] = 0;
        cost_tb_[i] = 0;

        cost_hvh_test_[i] = 0;
        cost_v_test_[i] = 0;
        cost_tb_test_[i] = 0;
      }

      // compute the cost for all H-segs and V-segs and partial boundary seg
      // cost for V-segs
      for (i = x1; i <= x2; i++) {
        grid = ymin * x_grid_;
        for (j = ymin; j < ymax; j++) {
          tmp = v_edges_[grid + i].est_usage - v_capacity_lb_
                + v_edges_[grid + i].red;
          grid += x_grid_;
          if (tmp > 0) {
            cost_v_[i - x1] += tmp;
            cost_v_test_[i - x1] += HCOST;
          } else {
            cost_v_test_[i - x1] += tmp;
          }
        }
      }
      // cost for Top&Bot boundary segs (form Z with V-seg)
      grid = y2 * (x_grid_ - 1);
      for (j = x1; j < x2; j++) {
        tmp = h_edges_[grid + j].est_usage - h_capacity_lb_
              + h_edges_[grid + j].red;
        if (tmp > 0) {
          cost_tb_[0] += tmp;
          cost_tb_test_[0] += HCOST;
        } else {
          cost_tb_test_[0] += tmp;
        }
      }
      grid1 = y1 * (x_grid_ - 1) + x1;
      grid2 = y2 * (x_grid_ - 1) + x1;
      for (i = 1; i <= segWidth; i++) {
        cost_tb_[i] = cost_tb_[i - 1];
        tmp = h_edges_[grid1 + i - 1].est_usage - h_capacity_lb_
              + h_edges_[grid1 + i - 1].red;
        if (tmp > 0) {
          cost_tb_[i] += tmp;
          cost_tb_test_[i] += HCOST;
        } else {
          cost_tb_test_[i] += tmp;
        }
        tmp = h_edges_[grid2 + i - 1].est_usage - h_capacity_lb_
              + h_edges_[grid2 + i - 1].red;
        if (tmp > 0) {
          cost_tb_[i] -= tmp;
          cost_tb_test_[i] -= HCOST;
        } else {
          cost_tb_test_[i] -= tmp;
        }
      }
      // compute cost for all Z routing
      bestcost = BIG_INT;
      btTEST = BIG_INT;
      bestZ = 0;
      for (i = 0; i <= segWidth; i++) {
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

      grid = y1 * (x_grid_ - 1);
      for (i = x1; i < bestZ; i++) {
        h_edges_[grid + i].est_usage += edgeCost;
      }
      grid = y2 * (x_grid_ - 1);
      for (i = bestZ; i < x2; i++) {
        h_edges_[grid + i].est_usage += edgeCost;
      }
      grid = ymin * x_grid_;
      for (i = ymin; i < ymax; i++) {
        v_edges_[grid + bestZ].est_usage += edgeCost;
        grid += x_grid_;
      }
      treeedge->route.HVH = true;
      treeedge->route.Zpoint = bestZ;
    }  // else Z route

  }  // if non-degraded edge
}

// Z-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteZ(int netID, int threshold)
{
  int ind, i, j, d, n1, n2, x1, y1, x2, y2, segWidth, segHeight, bestZ, grid,
      grid1, grid2, ymin, ymax, n1a, n2a, status1, status2;
  float tmp, bestcost, btTEST;
  bool HVH;        // the shape of Z routing (true - HVH, false - VHV)
  bool y1Smaller;  // true - y1<y2, false y1>y2
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int edgeCost = nets_[netID]->edgeCost;

  d = sttrees_[netID].deg;

  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;

  // loop for all the tree edges (2*d-3)

  for (ind = 0; ind < 2 * d - 3; ind++) {
    treeedge = &(treeedges[ind]);

    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;

    if (sttrees_[netID].edges[ind].len
        > threshold)  // only route the edges with len>5
    {
      if (x1 != x2 && y1 != y2)  // not H or V edge, do Z-routing
      {
        // ripup the original routing
        if (newRipupType2(treeedge, treenodes, x1, y1, x2, y2, d, netID)) {
          n1a = treenodes[n1].stackAlias;
          n2a = treenodes[n2].stackAlias;
          status1 = treenodes[n1a].status;
          status2 = treenodes[n2a].status;

          treeedge->route.type = RouteType::ZRoute;

          segWidth = x2 - x1;
          if (y1 < y2) {
            ymin = y1;
            ymax = y2;
            y1Smaller = true;
          } else {
            ymin = y2;
            ymax = y1;
            y1Smaller = false;
          }
          segHeight = ymax - ymin;

          // compute the cost for all Z routing

          if (status1 == 0 || status1 == 3) {
            for (i = 0; i < segWidth; i++) {
              cost_hvh_[i] = 0;
              cost_hvh_test_[i] = 0;
            }
            for (i = 0; i < segHeight; i++) {
              cost_vhv_[i] = 0;
            }
          } else if (status1 == 2) {
            for (i = 0; i < segWidth; i++) {
              cost_hvh_[i] = 0;
              cost_hvh_test_[i] = 0;
            }
            for (i = 0; i < segHeight; i++) {
              cost_vhv_[i] = via_cost_;
            }
          } else {
            for (i = 0; i < segWidth; i++) {
              cost_hvh_[i] = via_cost_;
              cost_hvh_test_[i] = via_cost_;
            }
            for (i = 0; i < segHeight; i++) {
              cost_vhv_[i] = 0;
            }
          }

          if (status2 == 2) {
            for (i = 0; i < segHeight; i++) {
              cost_vhv_[i] += via_cost_;
            }

          } else if (status2 == 1) {
            for (i = 0; i < segWidth; i++) {
              cost_hvh_[i] += via_cost_;
              cost_hvh_test_[i] += via_cost_;
            }
          }

          for (i = 0; i < segWidth; i++) {
            cost_v_[i] = 0;
            cost_tb_[i] = 0;

            cost_v_test_[i] = 0;
            cost_tb_test_[i] = 0;
          }
          for (i = 0; i < segHeight; i++) {
            cost_h_[i] = 0;
            cost_lr_[i] = 0;
          }

          // compute the cost for all H-segs and V-segs and partial boundary seg
          // cost for V-segs
          for (i = x1; i < x2; i++) {
            grid = ymin * x_grid_;
            for (j = ymin; j < ymax; j++) {
              tmp = v_edges_[grid + i].est_usage - v_capacity_lb_
                    + v_edges_[grid + i].red;
              grid += x_grid_;
              if (tmp > 0) {
                cost_v_[i - x1] += tmp;
                cost_v_test_[i - x1] += HCOST;
              } else {
                cost_v_test_[i - x1] += tmp;
              }
            }
          }
          // cost for Top&Bot boundary segs (form Z with V-seg)
          grid = y2 * (x_grid_ - 1);
          for (j = x1; j < x2; j++) {
            tmp = h_edges_[grid + j].est_usage - h_capacity_lb_
                  + h_edges_[grid + j].red;
            if (tmp > 0) {
              cost_tb_[0] += tmp;
              cost_tb_test_[0] += HCOST;
            } else {
              cost_tb_test_[0] += tmp;
            }
          }
          grid1 = y1 * (x_grid_ - 1) + x1;
          grid2 = y2 * (x_grid_ - 1) + x1;
          for (i = 1; i < segWidth; i++) {
            cost_tb_[i] = cost_tb_[i - 1];
            tmp = h_edges_[grid1 + i - 1].est_usage - h_capacity_lb_
                  + h_edges_[grid1 + i - 1].red;
            if (tmp > 0) {
              cost_tb_[i] += tmp;
              cost_tb_test_[0] += HCOST;
            } else {
              cost_tb_test_[0] += tmp;
            }
            tmp = h_edges_[grid2 + i - 1].est_usage - h_capacity_lb_
                  + h_edges_[grid2 + i - 1].red;
            if (tmp > 0) {
              cost_tb_[i] -= tmp;
              cost_tb_test_[0] -= HCOST;
            } else {
              cost_tb_test_[0] -= tmp;
            }
          }
          // cost for H-segs
          grid = ymin * (x_grid_ - 1);
          for (i = ymin; i < ymax; i++) {
            for (j = x1; j < x2; j++) {
              tmp = h_edges_[grid + j].est_usage - h_capacity_lb_
                    + h_edges_[grid + j].red;
              if (tmp > 0)
                cost_h_[i - ymin] += tmp;
            }
            grid += x_grid_ - 1;
          }
          // cost for Left&Right boundary segs (form Z with H-seg)
          if (y1Smaller) {
            for (j = y1; j < y2; j++) {
              tmp = v_edges_[j * x_grid_ + x2].est_usage - v_capacity_lb_
                    + v_edges_[j * x_grid_ + x2].red;
              if (tmp > 0)
                cost_lr_[0] += tmp;
            }
            for (i = 1; i < segHeight; i++) {
              cost_lr_[i] = cost_lr_[i - 1];
              grid = (y1 + i - 1) * x_grid_;
              tmp = v_edges_[grid + x1].est_usage - v_capacity_lb_
                    + v_edges_[grid + x1].red;
              if (tmp > 0)
                cost_lr_[i] += tmp;
              tmp = v_edges_[grid + x2].est_usage - v_capacity_lb_
                    + v_edges_[grid + x2].red;
              if (tmp > 0)
                cost_lr_[i] -= tmp;
            }
          } else {
            for (j = y2; j < y1; j++) {
              tmp = v_edges_[j * x_grid_ + x1].est_usage - v_capacity_lb_;
              if (tmp > 0)
                cost_lr_[0] += tmp;
            }
            for (i = 1; i < segHeight; i++) {
              cost_lr_[i] = cost_lr_[i - 1];
              grid = (y2 + i - 1) * x_grid_;
              tmp = v_edges_[grid + x2].est_usage - v_capacity_lb_
                    + v_edges_[grid + x2].red;
              if (tmp > 0)
                cost_lr_[i] += tmp;
              tmp = v_edges_[grid + x1].est_usage - v_capacity_lb_
                    + v_edges_[grid + x1].red;
              if (tmp > 0)
                cost_lr_[i] -= tmp;
            }
          }

          // compute cost for all Z routing
          HVH = true;
          bestcost = BIG_INT;
          btTEST = BIG_INT;
          bestZ = 0;
          for (i = 0; i < segWidth; i++) {
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
          for (i = 0; i < segHeight; i++) {
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

            grid = y1 * (x_grid_ - 1);
            for (i = x1; i < bestZ; i++) {
              h_edges_[grid + i].est_usage += edgeCost;
            }
            grid = y2 * (x_grid_ - 1);
            for (i = bestZ; i < x2; i++) {
              h_edges_[grid + i].est_usage += edgeCost;
            }
            grid = ymin * x_grid_;
            for (i = ymin; i < ymax; i++) {
              v_edges_[grid + bestZ].est_usage += edgeCost;
              grid += x_grid_;
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
              grid = y1 * x_grid_;
              for (i = y1; i < bestZ; i++) {
                v_edges_[grid + x1].est_usage += edgeCost;
                grid += x_grid_;
              }
              grid = bestZ * x_grid_;
              for (i = bestZ; i < y2; i++) {
                v_edges_[grid + x2].est_usage += edgeCost;
                grid += x_grid_;
              }
              grid = bestZ * (x_grid_ - 1);
              for (i = x1; i < x2; i++) {
                h_edges_[grid + i].est_usage += edgeCost;
              }
              treeedge->route.HVH = HVH;
              treeedge->route.Zpoint = bestZ;
            } else {
              grid = y2 * x_grid_;
              for (i = y2; i < bestZ; i++) {
                v_edges_[grid + x2].est_usage += edgeCost;
                grid += x_grid_;
              }
              grid = bestZ * x_grid_;
              for (i = bestZ; i < y1; i++) {
                v_edges_[grid + x1].est_usage += edgeCost;
                grid += x_grid_;
              }
              grid = bestZ * (x_grid_ - 1);
              for (i = x1; i < x2; i++) {
                h_edges_[grid + i].est_usage += edgeCost;
              }
              treeedge->route.HVH = HVH;
              treeedge->route.Zpoint = bestZ;
            }
          }
        } else {  // if ripuped by type 2
          if (d == 2) {
            newrouteZ_edge(netID, ind);
          }
        }
      }

    } else
        /* TODO:  <19-07-19, add parentesis in the if below > */
        if (d == 2 && sttrees_[netID].edges[ind].len > threshold
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
  int i;
  for (i = 0; i < num_valid_nets_; i++) {
    newrouteZ(i, threshold);  // ripup previous route and do Z-routing
  }
}

// Ripup the original route and do Monotonic routing within bounding box
void FastRouteCore::routeMonotonic(int netID, int edgeID, int threshold)
{
  int i, j, cnt, x, xl, yl, xr, yr, n1, n2, x1, y1, x2, y2, grid, xGrid_1,
      ind_i, ind_j, ind_x;
  int vedge, hedge, segWidth, segHeight, curX, curY;
  std::vector<int> gridsX(x_range_ + y_range_);
  std::vector<int> gridsY(x_range_ + y_range_);
  float tmp;
  multi_array<float, 2> cost;
  multi_array<bool, 2> parent;  // remember the parent of a grid on the shortest path, true -
                  // same x, false - same y
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int edgeCost = nets_[netID]->edgeCost;

  static const bool same_x =  false;
  static const bool same_y =  true;

  if (sttrees_[netID].edges[edgeID].route.routelen
      > threshold)  // only route the non-degraded edges (len>0)
  {
    treeedges = sttrees_[netID].edges;
    treeedge = &(treeedges[edgeID]);
    treenodes = sttrees_[netID].nodes;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;

    if (x1 != x2 || y1 != y2)  // not H or V edge, do Z-routing (if H or V edge,
                               // no need to reroute)
    {
      // ripup the original routing
      newRipup(treeedge, treenodes, x1, y1, x2, y2, netID);

      segWidth = abs(x1 - x2);
      segHeight = abs(y1 - y2);
      if (x1 <= x2) {
        xl = x1;
        yl = y1;
        xr = x2;
        yr = y2;
      } else {
        xl = x2;
        yl = y2;
        xr = x1;
        yr = y1;
      }

      // find the best monotonic path from (x1, y1) to (x2, y2)
      cost.resize(boost::extents[segHeight + 1][segWidth + 1]);
      parent.resize(boost::extents[segHeight + 1][segWidth + 1]);

      xGrid_1 = x_grid_ - 1;  // tmp variable to save runtime
      if (yl <= yr) {
        // initialize first column
        cost[0][0] = 0;
        grid = yl * x_grid_;
        for (j = 0; j < segHeight; j++) {
          cost[j + 1][0]
              = cost[j][0]
                + std::max(0.0f,
                           v_edges_[grid + xl].red + v_edges_[grid + xl].est_usage
                               - v_capacity_lb_);
          parent[j + 1][0] = same_x;
          grid += x_grid_;
        }
        // update other columns
        for (i = 0; i < segWidth; i++) {
          x = xl + i;
          // update the cost of a column of grids by h-edges
          grid = yl * xGrid_1;
          for (j = 0; j <= segHeight; j++) {
            tmp = std::max(0.0f,
                           h_edges_[grid + x].red + h_edges_[grid + x].est_usage
                               - h_capacity_lb_);
            cost[j][i + 1] = cost[j][i] + tmp;
            parent[j][i + 1] = same_y;
            grid += x_grid_ - 1;
          }
          // update the cost of a column of grids by v-edges
          grid = yl * x_grid_;
          ind_x = x + 1;
          ind_i = i + 1;
          for (j = 0; j < segHeight; j++) {
            ind_j = j + 1;
            tmp = cost[j][ind_i]
                  + std::max(0.0f,
                             v_edges_[grid + ind_x].red
                                 + v_edges_[grid + ind_x].est_usage
                                 - v_capacity_lb_);
            if (cost[ind_j][ind_i] > tmp) {
              cost[ind_j][ind_i] = tmp;
              parent[ind_j][ind_i] = same_x;
            }
            grid += x_grid_;
          }
        }

        // store the shortest path and update the usage
        curX = xr;
        curY = yr;
        cnt = 0;

        while (curX != xl || curY != yl) {
          gridsX[cnt] = curX;
          gridsY[cnt] = curY;
          cnt++;
          if (parent[curY - yl][curX - xl] == same_x) {
            curY--;
            vedge = curY * x_grid_ + curX;
            v_edges_[vedge].est_usage += edgeCost;
          } else {
            curX--;
            hedge = curY * (x_grid_ - 1) + curX;
            h_edges_[hedge].est_usage += edgeCost;
          }
        }

        gridsX[cnt] = xl;
        gridsY[cnt] = yl;
        cnt++;

      }  // yl<=yr

      else  // yl>yr
      {
        // initialize first column
        cost[segHeight][0] = 0;
        grid = (yl - 1) * x_grid_;
        for (j = segHeight - 1; j >= 0; j--) {
          cost[j][0]
              = cost[j + 1][0]
                + std::max(0.0f,
                           v_edges_[grid + xl].red + v_edges_[grid + xl].est_usage
                               - v_capacity_lb_);
          parent[j][0] = same_x;
          grid -= x_grid_;
        }
        // update other columns
        for (i = 0; i < segWidth; i++) {
          x = xl + i;
          // update the cost of a column of grids by h-edges
          grid = yl * (x_grid_ - 1);
          ind_i = i + 1;
          for (j = segHeight; j >= 0; j--) {
            tmp = std::max(0.0f,
                           h_edges_[grid + x].red + h_edges_[grid + x].est_usage
                               - h_capacity_lb_);
            cost[j][ind_i] = cost[j][i] + tmp;
            parent[j][ind_i] = same_y;
            grid -= x_grid_ - 1;
          }
          // update the cost of a column of grids by v-edges
          grid = (yl - 1) * x_grid_;
          ind_x = x + 1;
          for (j = segHeight - 1; j >= 0; j--) {
            tmp = cost[j + 1][ind_i]
                  + std::max(0.0f,
                             v_edges_[grid + ind_x].red
                                 + v_edges_[grid + ind_x].est_usage
                                 - v_capacity_lb_);
            if (cost[j][ind_i] > tmp) {
              cost[j][ind_i] = tmp;
              parent[j][ind_i] = same_x;
            }
            grid -= x_grid_;
          }
        }

        // store the shortest path and update the usage
        curX = xr;
        curY = yr;
        cnt = 0;
        while (curX != xl || curY != yl) {
          gridsX[cnt] = curX;
          gridsY[cnt] = curY;
          cnt++;
          if (parent[curY - yr][curX - xl] == same_x) {
            vedge = curY * x_grid_ + curX;
            v_edges_[vedge].est_usage += edgeCost;
            curY++;
          } else {
            curX--;
            hedge = curY * (x_grid_ - 1) + curX;
            h_edges_[hedge].est_usage += edgeCost;
          }
        }
        gridsX[cnt] = xl;
        gridsY[cnt] = yl;
        cnt++;

      }  // yl>yr
      treeedge->route.routelen = cnt - 1;

      treeedge->route.gridsX
          = (short*) realloc(treeedge->route.gridsX, cnt * sizeof(short));
      treeedge->route.gridsY
          = (short*) realloc(treeedge->route.gridsY, cnt * sizeof(short));
      if (x1 != gridsX[0] || y1 != gridsY[0])  // gridsX[] and gridsY[] store
                                               // the path from n2 to n1
      {
        cnt = 0;
        for (i = treeedge->route.routelen; i >= 0; i--) {
          treeedge->route.gridsX[cnt] = gridsX[i];
          treeedge->route.gridsY[cnt] = gridsY[i];
          cnt++;
        }
      } else  // gridsX[] and gridsY[] store the path from n1 to n2
      {
        for (i = 0; i <= treeedge->route.routelen; i++) {
          treeedge->route.gridsX[i] = gridsX[i];
          treeedge->route.gridsY[i] = gridsY[i];
        }
      }

      cost.resize(boost::extents[0][0]);
      parent.resize(boost::extents[0][0]);

    }  // if(x1!=x2 || y1!=y2)
  }    // non-degraded edge
}

void FastRouteCore::routeMonotonicAll(int threshold)
{
  int netID, edgeID;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    for (edgeID = 0; edgeID < sttrees_[netID].deg * 2 - 3; edgeID++) {
      routeMonotonic(
          netID,
          edgeID,
          threshold);  // ripup previous route and do Monotonic routing
    }
  }
}

void FastRouteCore::spiralRoute(int netID, int edgeID)
{
  int j, n1, n2, x1, y1, x2, y2, grid, grid1, n1a, n2a;
  float costL1 = 0;
  float costL2 = 0;
  float tmp;
  int ymin, ymax;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;

  int edgeCost = nets_[netID]->edgeCost;

  treeedge = &(treeedges[edgeID]);
  if (treeedge->len > 0)  // only route the non-degraded edges (len>0)
  {
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;

    n1a = treenodes[n1].stackAlias;
    n2a = treenodes[n2].stackAlias;

    if (y1 < y2) {
      ymin = y1;
      ymax = y2;
    } else {
      ymin = y2;
      ymax = y1;
    }

    // ripup the original routing

    treeedge->route.type = RouteType::LRoute;
    if (x1 == x2)  // V-routing
    {
      for (j = ymin; j < ymax; j++)
        v_edges_[j * x_grid_ + x1].est_usage += edgeCost;
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
    } else if (y1 == y2)  // H-routing
    {
      for (j = x1; j < x2; j++)
        h_edges_[y1 * (x_grid_ - 1) + j].est_usage += edgeCost;
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
    } else  // L-routing
    {
      if (treenodes[n1].status == 0 || treenodes[n1].status == 3) {
        costL1 = costL2 = 0;
      } else if (treenodes[n1].status == 2) {
        costL1 = via_cost_;
        costL2 = 0;
      } else if (treenodes[n1].status == 1) {
        costL1 = 0;
        costL2 = via_cost_;
      } else {
        logger_->warn(GRT, 181, "Wrong node status {}.", treenodes[n1].status);
      }
      if (treenodes[n2].status == 2) {
        costL2 += via_cost_;
      } else if (treenodes[n2].status == 1) {
        costL1 += via_cost_;
      }

      for (j = ymin; j < ymax; j++) {
        grid = j * x_grid_;
        tmp = v_edges_[grid + x1].est_usage - v_capacity_lb_
              + v_edges_[grid + x1].red;
        if (tmp > 0)
          costL1 += tmp;
        tmp = v_edges_[grid + x2].est_usage - v_capacity_lb_
              + v_edges_[grid + x2].red;
        if (tmp > 0)
          costL2 += tmp;
      }
      grid = y2 * (x_grid_ - 1);
      grid1 = y1 * (x_grid_ - 1);
      for (j = x1; j < x2; j++) {
        tmp = h_edges_[grid + j].est_usage - h_capacity_lb_
              + h_edges_[grid + j].red;
        if (tmp > 0)
          costL1 += tmp;
        tmp = h_edges_[grid1 + j].est_usage - h_capacity_lb_
              + h_edges_[grid1 + j].red;
        if (tmp > 0)
          costL2 += tmp;
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
        for (j = ymin; j < ymax; j++) {
          v_edges_[j * x_grid_ + x1].est_usage += edgeCost;
        }
        grid = y2 * (x_grid_ - 1);
        for (j = x1; j < x2; j++) {
          h_edges_[grid + j].est_usage += edgeCost;
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

        if (treenodes[n2a].status % 2 == 0) {
          treenodes[n2a].status += 1;
        }

        if (treenodes[n1a].status < 2) {
          treenodes[n1a].status += 2;
        }
        treenodes[n1a].hID++;
        treenodes[n2a].lID++;

        // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
        grid = y1 * (x_grid_ - 1);
        for (j = x1; j < x2; j++) {
          h_edges_[grid + j].est_usage += edgeCost;
        }
        for (j = ymin; j < ymax; j++) {
          v_edges_[j * x_grid_ + x2].est_usage += edgeCost;
        }
        treeedge->route.xFirst = true;
      }

    }  // else L-routing
  }    // if non-degraded edge
  else
    sttrees_[netID].edges[edgeID].route.type = RouteType::NoRoute;
}

void FastRouteCore::spiralRouteAll()
{
  int netID, d, k, edgeID, nodeID, deg, numpoints, n1, n2;
  int na;
  bool redundant;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;
  std::queue<int> edgeQueue;

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
      treenodes[d].hID = 0;
      treenodes[d].lID = 0;
      treenodes[d].status = 0;

      if (d < deg) {
        treenodes[d].botL = treenodes[d].topL = 0;
        // treenodes[d].l = 0;
        treenodes[d].assigned = true;
        treenodes[d].status = 2;

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
        (treenodes[treeedge->n2a].conCNT)++;
        treeedges[edgeID].assigned = false;
      } else {
        treeedges[edgeID].assigned = true;
      }
    }
  }

  for (netID = 0; netID < num_valid_nets_; netID++) {
    newRipupNet(netID);

    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;
    /* edgeQueue.clear(); */

    for (nodeID = 0; nodeID < deg; nodeID++) {
      treenodes[nodeID].assigned = true;
      for (k = 0; k < treenodes[nodeID].conCNT; k++) {
        edgeID = treenodes[nodeID].eID[k];

        if (treeedges[edgeID].assigned == false) {
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
        spiralRoute(netID, edgeID);
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
        spiralRoute(netID, edgeID);
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
  }

  for (netID = 0; netID < num_valid_nets_; netID++) {
    treenodes = sttrees_[netID].nodes;
    deg = sttrees_[netID].deg;

    for (d = 0; d < 2 * deg - 2; d++) {
      na = treenodes[d].stackAlias;

      treenodes[d].status = treenodes[na].status;
    }
  }
}

void FastRouteCore::routeLVEnew(int netID, int edgeID, int threshold, int enlarge)
{
  int i, j, cnt, xmin, xmax, ymin, ymax, n1, n2, x1, y1, x2, y2, grid, xGrid_1,
      deg, yminorig, ymaxorig;
  int vedge, hedge;
  int bestp1x = 0;
  int bestp1y = 0;
  std::vector<int> gridsX(x_range_ + y_range_);
  std::vector<int> gridsY(x_range_ + y_range_);
  float tmp1, tmp2, tmp3, tmp4, tmp, best;
  bool LH1, LH2;
  bool BL1 = false;
  bool BL2 = false;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int edgeCost = nets_[netID]->edgeCost;

  if (sttrees_[netID].edges[edgeID].len
      > threshold)  // only route the non-degraded edges (len>0)
  {
    treeedges = sttrees_[netID].edges;
    treeedge = &(treeedges[edgeID]);
    treenodes = sttrees_[netID].nodes;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;

    // ripup the original routing
    if (newRipupCheck(treeedge, x1, y1, x2, y2, threshold, netID, edgeID)) {
      deg = sttrees_[netID].deg;
      xmin = std::max(x1 - enlarge, 0);
      xmax = std::min(x_grid_ - 1, x2 + enlarge);

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

      if (deg > 2) {
        for (j = 0; j < deg * 2 - 2; j++) {
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

      xGrid_1 = x_grid_ - 1;  // tmp variable to save runtime

      for (j = ymin; j <= ymax; j++) {
        d1_[j][xmin] = 0;
      }
      // update other columns
      for (i = xmin; i <= xmax; i++) {
        d2_[ymin][i] = 0;
      }

      for (j = ymin; j <= ymax; j++) {
        grid = j * xGrid_1 + xmin;
        for (i = xmin; i < xmax; i++) {
          tmp = h_cost_table_[h_edges_[grid].red + h_edges_[grid].usage];
          d1_[j][i + 1] = d1_[j][i] + tmp;
          grid++;
        }
        // update the cost of a column of grids by v-edges
      }

      for (j = ymin; j < ymax; j++) {
        // update the cost of a column of grids by h-edges
        grid = j * x_grid_ + xmin;
        for (i = xmin; i <= xmax; i++) {
          tmp = h_cost_table_[v_edges_[grid].red + v_edges_[grid].usage];
          d2_[j + 1][i] = d2_[j][i] + tmp;
          grid++;
        }
        // update the cost of a column of grids by v-edges
      }

      best = BIG_INT;

      for (j = ymin; j <= ymax; j++) {
        for (i = xmin; i <= xmax; i++) {
          tmp1 = abs(d2_[j][x1] - d2_[y1][x1])
                 + abs(d1_[j][i] - d1_[j][x1]);  // yfirst for point 1
          tmp2 = abs(d2_[j][i] - d2_[y1][i]) + abs(d1_[y1][i] - d1_[y1][x1]);
          tmp3 = abs(d2_[y2][i] - d2_[j][i]) + abs(d1_[y2][i] - d1_[y2][x2]);
          tmp4 = abs(d2_[y2][x2] - d2_[j][x2])
                 + abs(d1_[j][x2] - d1_[j][i]);  // xifrst for mid point

          tmp = tmp1 + tmp4;
          LH1 = false;
          LH2 = true;

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
      cnt = 0;

      if (BL1) {
        if (bestp1x > x1) {
          for (i = x1; i < bestp1x; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = y1;
            hedge = y1 * xGrid_1 + i;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        } else {
          for (i = x1; i > bestp1x; i--) {
            gridsX[cnt] = i;
            gridsY[cnt] = y1;
            hedge = y1 * xGrid_1 + i - 1;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        }
        if (bestp1y > y1) {
          for (i = y1; i < bestp1y; i++) {
            gridsX[cnt] = bestp1x;
            gridsY[cnt] = i;
            cnt++;
            vedge = i * x_grid_ + bestp1x;
            v_edges_[vedge].usage += edgeCost;
          }
        } else {
          for (i = y1; i > bestp1y; i--) {
            gridsX[cnt] = bestp1x;
            gridsY[cnt] = i;
            cnt++;
            vedge = (i - 1) * x_grid_ + bestp1x;
            v_edges_[vedge].usage += edgeCost;
          }
        }
      } else {
        if (bestp1y > y1) {
          for (i = y1; i < bestp1y; i++) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
            vedge = i * x_grid_ + x1;
            v_edges_[vedge].usage += edgeCost;
          }
        } else {
          for (i = y1; i > bestp1y; i--) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
            vedge = (i - 1) * x_grid_ + x1;
            v_edges_[vedge].usage += edgeCost;
          }
        }
        if (bestp1x > x1) {
          for (i = x1; i < bestp1x; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = bestp1y;
            hedge = bestp1y * xGrid_1 + i;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        } else {
          for (i = x1; i > bestp1x; i--) {
            gridsX[cnt] = i;
            gridsY[cnt] = bestp1y;
            hedge = bestp1y * xGrid_1 + i - 1;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        }
      }

      if (BL2) {
        if (bestp1x < x2) {
          for (i = bestp1x; i < x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = bestp1y;
            hedge = bestp1y * xGrid_1 + i;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        } else {
          for (i = bestp1x; i > x2; i--) {
            gridsX[cnt] = i;
            gridsY[cnt] = bestp1y;
            hedge = bestp1y * xGrid_1 + i - 1;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        }

        if (y2 > bestp1y) {
          for (i = bestp1y; i < y2; i++) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
            vedge = i * x_grid_ + x2;
            v_edges_[vedge].usage += edgeCost;
          }
        } else {
          for (i = bestp1y; i > y2; i--) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
            vedge = (i - 1) * x_grid_ + x2;
            v_edges_[vedge].usage += edgeCost;
          }
        }
      } else {
        if (y2 > bestp1y) {
          for (i = bestp1y; i < y2; i++) {
            gridsX[cnt] = bestp1x;
            gridsY[cnt] = i;
            cnt++;
            vedge = i * x_grid_ + bestp1x;
            v_edges_[vedge].usage += edgeCost;
          }
        } else {
          for (i = bestp1y; i > y2; i--) {
            gridsX[cnt] = bestp1x;
            gridsY[cnt] = i;
            cnt++;
            vedge = (i - 1) * x_grid_ + bestp1x;
            v_edges_[vedge].usage += edgeCost;
          }
        }
        if (x2 > bestp1x) {
          for (i = bestp1x; i < x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = y2;
            hedge = y2 * xGrid_1 + i;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        } else {
          for (i = bestp1x; i > x2; i--) {
            gridsX[cnt] = i;
            gridsY[cnt] = y2;
            hedge = y2 * xGrid_1 + i - 1;
            h_edges_[hedge].usage += edgeCost;
            cnt++;
          }
        }
      }

      gridsX[cnt] = x2;
      gridsY[cnt] = y2;
      cnt++;

      treeedge->route.routelen = cnt - 1;
      if (treeedge->route.gridsX)
        free(treeedge->route.gridsX);
      if (treeedge->route.gridsY)
        free(treeedge->route.gridsY);

      treeedge->route.gridsX = (short*) calloc(cnt, sizeof(short));
      treeedge->route.gridsY = (short*) calloc(cnt, sizeof(short));

      for (i = 0; i < cnt; i++) {
        treeedge->route.gridsX[i] = gridsX[i];
        treeedge->route.gridsY[i] = gridsY[i];
      }

    }  // if(x1!=x2 || y1!=y2)
  }    // non-degraded edge
}

void FastRouteCore::routeLVAll(int threshold, int expand, float logis_cof)
{
  int netID, edgeID, numEdges, i, forange;

  if (verbose_ > 1)
    logger_->info(GRT, 182, "{} threshold, {} expand.", threshold, expand);

  h_cost_table_.resize(10 * h_capacity_);

  forange = 10 * h_capacity_;
  for (i = 0; i < forange; i++) {
    h_cost_table_[i] = costheight_ / (exp((float) (h_capacity_ - i) * logis_cof) + 1)
                     + 1;
  }

  for (netID = 0; netID < num_valid_nets_; netID++) {
    numEdges = 2 * sttrees_[netID].deg - 3;
    for (edgeID = 0; edgeID < numEdges; edgeID++) {
      routeLVEnew(netID,
                  edgeID,
                  threshold,
                  expand);  // ripup previous route and do Monotonic routing
    }
  }
  h_cost_table_.clear();
}

void FastRouteCore::newrouteLInMaze(int netID)
{
  int i, j, d, n1, n2, x1, y1, x2, y2, grid, grid1;
  int costL1, costL2, tmp;
  int ymin, ymax;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  d = sttrees_[netID].deg;
  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;

  int edgeCost = nets_[netID]->edgeCost;

  // loop for all the tree edges (2*d-3)
  for (i = 0; i < 2 * d - 3; i++) {
    if (sttrees_[netID].edges[i].len
        > 0)  // only route the non-degraded edges (len>0)
    {
      treeedge = &(treeedges[i]);

      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;

      if (y1 < y2) {
        ymin = y1;
        ymax = y2;
      } else {
        ymin = y2;
        ymax = y1;
      }

      treeedge->route.type = RouteType::LRoute;
      if (x1 == x2)  // V-routing
      {
        for (j = ymin; j < ymax; j++)
          v_edges_[j * x_grid_ + x1].usage += edgeCost;
        treeedge->route.xFirst = false;

      } else if (y1 == y2)  // H-routing
      {
        for (j = x1; j < x2; j++)
          h_edges_[y1 * (x_grid_ - 1) + j].usage += edgeCost;
        treeedge->route.xFirst = true;

      } else  // L-routing
      {
        costL1 = costL2 = 0;

        for (j = ymin; j < ymax; j++) {
          grid = j * x_grid_;
          tmp = v_edges_[grid + x1].usage - v_capacity_lb_
                + v_edges_[grid + x1].red;
          if (tmp > 0)
            costL1 += tmp;
          tmp = v_edges_[grid + x2].usage - v_capacity_lb_
                + v_edges_[grid + x2].red;
          if (tmp > 0)
            costL2 += tmp;
        }
        grid = y2 * (x_grid_ - 1);
        grid1 = y1 * (x_grid_ - 1);
        for (j = x1; j < x2; j++) {
          tmp = h_edges_[grid + j].usage - h_capacity_lb_ + h_edges_[grid + j].red;
          if (tmp > 0)
            costL1 += tmp;
          tmp = h_edges_[grid1 + j].usage - h_capacity_lb_
                + h_edges_[grid1 + j].red;
          if (tmp > 0)
            costL2 += tmp;
        }

        if (costL1 < costL2) {
          // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
          for (j = ymin; j < ymax; j++) {
            v_edges_[j * x_grid_ + x1].usage += edgeCost;
          }
          grid = y2 * (x_grid_ - 1);
          for (j = x1; j < x2; j++) {
            h_edges_[grid + j].usage += edgeCost;
          }
          treeedge->route.xFirst = false;
        }  // if costL1<costL2
        else {
          // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
          grid = y1 * (x_grid_ - 1);
          for (j = x1; j < x2; j++) {
            h_edges_[grid + j].usage += edgeCost;
          }
          for (j = ymin; j < ymax; j++) {
            v_edges_[j * x_grid_ + x2].usage += edgeCost;
          }
          treeedge->route.xFirst = true;
        }

      }  // else L-routing
    }    // if non-degraded edge
    else
      sttrees_[netID].edges[i].route.type = RouteType::NoRoute;
  }  // loop i
}
}  // namespace grt
