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

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

// rip-up a L segment
void FastRouteCore::ripupSegL(Segment* seg)
{
  int edgeCost = nets_[seg->netID]->edgeCost;
  int i, grid;
  int ymin, ymax;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  // remove L routing
  if (seg->xFirst) {
    grid = seg->y1 * (x_grid_ - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges_[grid + i].est_usage -= edgeCost;
    for (i = ymin; i < ymax; i++)
      v_edges_[i * x_grid_ + seg->x2].est_usage -= edgeCost;
  } else {
    for (i = ymin; i < ymax; i++)
      v_edges_[i * x_grid_ + seg->x1].est_usage -= edgeCost;
    grid = seg->y2 * (x_grid_ - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges_[grid + i].est_usage -= edgeCost;
  }
}

void FastRouteCore::ripupSegZ(Segment* seg)
{
  int i, grid;
  int ymin, ymax;

  int edgeCost = nets_[seg->netID]->edgeCost;

  if (seg->y1 < seg->y2) {
    ymin = seg->y1;
    ymax = seg->y2;
  } else {
    ymin = seg->y2;
    ymax = seg->y1;
  }

  if (seg->x1 == seg->x2) {
    // remove V routing
    for (i = ymin; i < ymax; i++)
      v_edges_[i * x_grid_ + seg->x1].est_usage -= edgeCost;
  } else if (seg->y1 == seg->y2) {
    // remove H routing
    grid = seg->y1 * (x_grid_ - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges_[grid + i].est_usage -= edgeCost;
  } else {
    // remove Z routing
    if (seg->HVH) {
      grid = seg->y1 * (x_grid_ - 1);
      for (i = seg->x1; i < seg->Zpoint; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
      grid = seg->y2 * (x_grid_ - 1);
      for (i = seg->Zpoint; i < seg->x2; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges_[i * x_grid_ + seg->Zpoint].est_usage -= edgeCost;
    } else {
      if (seg->y1 < seg->y2) {
        for (i = seg->y1; i < seg->Zpoint; i++)
          v_edges_[i * x_grid_ + seg->x1].est_usage -= edgeCost;
        for (i = seg->Zpoint; i < seg->y2; i++)
          v_edges_[i * x_grid_ + seg->x2].est_usage -= edgeCost;
        grid = seg->Zpoint * (x_grid_ - 1);
        for (i = seg->x1; i < seg->x2; i++)
          h_edges_[grid + i].est_usage -= 1;
      } else {
        for (i = seg->y2; i < seg->Zpoint; i++)
          v_edges_[i * x_grid_ + seg->x2].est_usage -= edgeCost;
        for (i = seg->Zpoint; i < seg->y1; i++)
          v_edges_[i * x_grid_ + seg->x1].est_usage -= edgeCost;
        grid = seg->Zpoint * (x_grid_ - 1);
        for (i = seg->x1; i < seg->x2; i++)
          h_edges_[grid + i].est_usage -= 1;
      }
    }
  }
}

void FastRouteCore::newRipup(TreeEdge* treeedge,
              TreeNode* treenodes,
              int x1,
              int y1,
              int x2,
              int y2,
              int netID)
{
  int i, grid, Zpoint, ymin, ymax, xmin;
  RouteType ripuptype;

  int edgeCost = nets_[netID]->edgeCost;

  if (treeedge->len == 0) {
    return;  // not ripup for degraded edge
  }

  ripuptype = treeedge->route.type;
  if (y1 < y2) {
    ymin = y1;
    ymax = y2;
  } else {
    ymin = y2;
    ymax = y1;
  }

  if (ripuptype == RouteType::LRoute)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      grid = y1 * (x_grid_ - 1);
      for (i = x1; i < x2; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
    } else {
      for (i = ymin; i < ymax; i++)
        v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
      grid = y2 * (x_grid_ - 1);
      for (i = x1; i < x2; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
    }
  } else if (ripuptype == RouteType::ZRoute) {
    // remove Z routing
    Zpoint = treeedge->route.Zpoint;
    if (treeedge->route.HVH) {
      grid = y1 * (x_grid_ - 1);
      for (i = x1; i < Zpoint; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
      grid = y2 * (x_grid_ - 1);
      for (i = Zpoint; i < x2; i++)
        h_edges_[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges_[i * x_grid_ + Zpoint].est_usage -= edgeCost;
    } else {
      if (y1 < y2) {
        for (i = y1; i < Zpoint; i++)
          v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
        for (i = Zpoint; i < y2; i++)
          v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
        grid = Zpoint * (x_grid_ - 1);
        for (i = x1; i < x2; i++)
          h_edges_[grid + i].est_usage -= edgeCost;
      } else {
        for (i = y2; i < Zpoint; i++)
          v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
        for (i = Zpoint; i < y1; i++)
          v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
        grid = Zpoint * (x_grid_ - 1);
        for (i = x1; i < x2; i++)
          h_edges_[grid + i].est_usage -= edgeCost;
      }
    }
  } else if (ripuptype == RouteType::MazeRoute) {
    const std::vector<short>& gridsX = treeedge->route.gridsX;
    const std::vector<short>& gridsY = treeedge->route.gridsY;
    for (i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        v_edges_[ymin * x_grid_ + gridsX[i]].est_usage -= edgeCost;
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        h_edges_[gridsY[i] * (x_grid_ - 1) + xmin].est_usage -= edgeCost;
      } else {
        logger_->error(GRT, 225, "Maze ripup wrong in newRipup.");
      }
    }
  }
}

bool FastRouteCore::newRipupType2(TreeEdge* treeedge,
                   TreeNode* treenodes,
                   int x1,
                   int y1,
                   int x2,
                   int y2,
                   int deg,
                   int netID)
{
  int i, grid, ymin, ymax, n1, n2;
  RouteType ripuptype;
  bool needRipup = false;

  int edgeCost = nets_[netID]->edgeCost;

  if (treeedge->len == 0) {
    return false;  // not ripup for degraded edge
  }

  ripuptype = treeedge->route.type;
  if (y1 < y2) {
    ymin = y1;
    ymax = y2;
  } else {
    ymin = y2;
    ymax = y1;
  }

  if (ripuptype == RouteType::LRoute)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      grid = y1 * (x_grid_ - 1);
      for (i = x1; i < x2; i++) {
        if (h_edges_[grid + i].est_usage > h_edges_[grid + i].cap) {
          needRipup = true;
          break;
        }
      }

      for (i = ymin; i < ymax; i++) {
        if (v_edges_[i * x_grid_ + x2].est_usage > v_edges_[i * x_grid_ + x2].cap) {
          needRipup = true;
          break;
        }
      }
    } else {
      for (i = ymin; i < ymax; i++) {
        if (v_edges_[i * x_grid_ + x1].est_usage > v_edges_[i * x_grid_ + x1].cap) {
          needRipup = true;
          break;
        }
      }
      grid = y2 * (x_grid_ - 1);
      for (i = x1; i < x2; i++) {
        if (h_edges_[grid + i].est_usage > h_edges_[grid + i].cap) {
          needRipup = true;
          break;
        }
      }
    }

    if (needRipup) {
      n1 = treeedge->n1;
      n2 = treeedge->n2;

      if (treeedge->route.xFirst) {
        if (n1 >= deg) {
          treenodes[n1].status -= 2;
        }
        treenodes[n2].status -= 1;

        grid = y1 * (x_grid_ - 1);
        for (i = x1; i < x2; i++)
          h_edges_[grid + i].est_usage -= edgeCost;
        for (i = ymin; i < ymax; i++)
          v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
      } else {
        if (n2 >= deg) {
          treenodes[n2].status -= 2;
        }
        treenodes[n1].status -= 1;

        for (i = ymin; i < ymax; i++)
          v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
        grid = y2 * (x_grid_ - 1);
        for (i = x1; i < x2; i++)
          h_edges_[grid + i].est_usage -= edgeCost;
      }
    }
    return (needRipup);

  } else {
    logger_->error(GRT, 226, "Type2 ripup not type L.");
  }
}

bool FastRouteCore::newRipupCheck(TreeEdge* treeedge,
                   int x1,
                   int y1,
                   int x2,
                   int y2,
                   int ripup_threshold,
                   int netID,
                   int edgeID)
{
  int i, grid, ymin, xmin;
  bool needRipup = false;

  int edgeCost = nets_[netID]->edgeCost;

  if (treeedge->len == 0) {
    return false;
  }  // not ripup for degraded edge

  if (treeedge->route.type == RouteType::MazeRoute) {
    const std::vector<short>& gridsX = treeedge->route.gridsX;
    const std::vector<short>& gridsY = treeedge->route.gridsY;
    for (i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = ymin * x_grid_ + gridsX[i];
        if (v_edges_[grid].usage + v_edges_[grid].red
            >= v_capacity_ - ripup_threshold) {
          needRipup = true;
          break;
        }
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsY[i] * (x_grid_ - 1) + xmin;
        if (h_edges_[grid].usage + h_edges_[grid].red
            >= h_capacity_ - ripup_threshold) {
          needRipup = true;
          break;
        }
      }
    }

    if (needRipup) {
      for (i = 0; i < treeedge->route.routelen; i++) {
        if (gridsX[i] == gridsX[i + 1])  // a vertical edge
        {
          ymin = std::min(gridsY[i], gridsY[i + 1]);
          v_edges_[ymin * x_grid_ + gridsX[i]].usage -= edgeCost;
        } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
        {
          xmin = std::min(gridsX[i], gridsX[i + 1]);
          h_edges_[gridsY[i] * (x_grid_ - 1) + xmin].usage -= edgeCost;
        }
      }
      return true;
    } else {
      return false;
    }
  } else {
    printEdge(netID, edgeID);
    logger_->error(GRT, 121, "Route type is not maze, netID {}.", netID);
  }
}

bool FastRouteCore::newRipup3DType3(int netID, int edgeID)
{
  int i, k, grid, ymin, xmin, n1a, n2a, hl, bl, hid, bid,
      deg;
  std::vector<int> edge_cost_per_layer = nets_[netID]->edge_cost_per_layer;

  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees_[netID].edges;
  treeedge = &(treeedges[edgeID]);

  if (treeedge->len == 0) {
    return false;  // not ripup for degraded edge
  }

  treenodes = sttrees_[netID].nodes;

  deg = sttrees_[netID].deg;

  n1a = treeedge->n1a;
  n2a = treeedge->n2a;

  if (n1a < deg) {
    bl = 0;
  } else {
    bl = BIG_INT;
  }
  hl = 0;
  hid = bid = BIG_INT;

  for (i = 0; i < treenodes[n1a].conCNT; i++) {
    if (treenodes[n1a].eID[i] == edgeID) {
      for (k = i + 1; k < treenodes[n1a].conCNT; k++) {
        treenodes[n1a].eID[k - 1] = treenodes[n1a].eID[k];
        treenodes[n1a].heights[k - 1] = treenodes[n1a].heights[k];
        if (bl > treenodes[n1a].heights[k]) {
          bl = treenodes[n1a].heights[k];
          bid = treenodes[n1a].eID[k];
        }
        if (hl < treenodes[n1a].heights[k]) {
          hl = treenodes[n1a].heights[k];
          hid = treenodes[n1a].eID[k];
        }
      }
      break;
    } else {
      if (bl > treenodes[n1a].heights[i]) {
        bl = treenodes[n1a].heights[i];
        bid = treenodes[n1a].eID[i];
      }
      if (hl < treenodes[n1a].heights[i]) {
        hl = treenodes[n1a].heights[i];
        hid = treenodes[n1a].eID[i];
      }
    }
  }
  treenodes[n1a].conCNT--;

  treenodes[n1a].botL = bl;
  treenodes[n1a].lID = bid;
  treenodes[n1a].topL = hl;
  treenodes[n1a].hID = hid;

  if (n2a < deg) {
    bl = 0;
  } else {
    bl = BIG_INT;
  }
  hl = 0;
  hid = bid = BIG_INT;

  for (i = 0; i < treenodes[n2a].conCNT; i++) {
    if (treenodes[n2a].eID[i] == edgeID) {
      for (k = i + 1; k < treenodes[n2a].conCNT; k++) {
        treenodes[n2a].eID[k - 1] = treenodes[n2a].eID[k];
        treenodes[n2a].heights[k - 1] = treenodes[n2a].heights[k];
        if (bl > treenodes[n2a].heights[k]) {
          bl = treenodes[n2a].heights[k];
          bid = treenodes[n2a].eID[k];
        }
        if (hl < treenodes[n2a].heights[k]) {
          hl = treenodes[n2a].heights[k];
          hid = treenodes[n2a].eID[k];
        }
      }
      break;
    } else {
      if (bl > treenodes[n2a].heights[i]) {
        bl = treenodes[n2a].heights[i];
        bid = treenodes[n2a].eID[i];
      }
      if (hl < treenodes[n2a].heights[i]) {
        hl = treenodes[n2a].heights[i];
        hid = treenodes[n2a].eID[i];
      }
    }
  }
  treenodes[n2a].conCNT--;

  treenodes[n2a].botL = bl;
  treenodes[n2a].lID = bid;
  treenodes[n2a].topL = hl;
  treenodes[n2a].hID = hid;

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  const std::vector<short>& gridsL = treeedge->route.gridsL;
  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = gridsL[i] * grid_v_ + ymin * x_grid_ + gridsX[i];
        v_edges_3D_[grid].usage -= edge_cost_per_layer[gridsL[i]];
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsL[i] * grid_h_ + gridsY[i] * (x_grid_ - 1) + xmin;
        h_edges_3D_[grid].usage -= edge_cost_per_layer[gridsL[i]];
      } else {
        logger_->error(GRT, 122, "Maze ripup wrong.");
      }
    }
  }

  return true;
}

void FastRouteCore::newRipupNet(int netID)
{
  int i, grid, Zpoint, ymin, ymax, xmin, n1, n2, edgeID;

  int edgeCost = nets_[netID]->edgeCost;

  RouteType ripuptype;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;
  int x1, y1, x2, y2, deg;

  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;
  deg = sttrees_[netID].deg;

  for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
    treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      n1 = treeedge->n1;
      n2 = treeedge->n2;
      x1 = treenodes[n1].x;
      y1 = treenodes[n1].y;
      x2 = treenodes[n2].x;
      y2 = treenodes[n2].y;

      ripuptype = treeedge->route.type;
      if (y1 < y2) {
        ymin = y1;
        ymax = y2;
      } else {
        ymin = y2;
        ymax = y1;
      }

      if (ripuptype == RouteType::LRoute)  // remove L routing
      {
        if (treeedge->route.xFirst) {
          grid = y1 * (x_grid_ - 1);
          for (i = x1; i < x2; i++)
            h_edges_[grid + i].est_usage -= edgeCost;
          for (i = ymin; i < ymax; i++)
            v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
        } else {
          for (i = ymin; i < ymax; i++)
            v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
          grid = y2 * (x_grid_ - 1);
          for (i = x1; i < x2; i++)
            h_edges_[grid + i].est_usage -= edgeCost;
        }
      } else if (ripuptype == RouteType::ZRoute) {
        // remove Z routing
        Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH) {
          grid = y1 * (x_grid_ - 1);
          for (i = x1; i < Zpoint; i++)
            h_edges_[grid + i].est_usage -= edgeCost;
          grid = y2 * (x_grid_ - 1);
          for (i = Zpoint; i < x2; i++)
            h_edges_[grid + i].est_usage -= edgeCost;
          for (i = ymin; i < ymax; i++)
            v_edges_[i * x_grid_ + Zpoint].est_usage -= edgeCost;
        } else {
          if (y1 < y2) {
            for (i = y1; i < Zpoint; i++)
              v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
            for (i = Zpoint; i < y2; i++)
              v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
            grid = Zpoint * (x_grid_ - 1);
            for (i = x1; i < x2; i++)
              h_edges_[grid + i].est_usage -= edgeCost;
          } else {
            for (i = y2; i < Zpoint; i++)
              v_edges_[i * x_grid_ + x2].est_usage -= edgeCost;
            for (i = Zpoint; i < y1; i++)
              v_edges_[i * x_grid_ + x1].est_usage -= edgeCost;
            grid = Zpoint * (x_grid_ - 1);
            for (i = x1; i < x2; i++)
              h_edges_[grid + i].est_usage -= edgeCost;
          }
        }
      } else if (ripuptype == RouteType::MazeRoute) {
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        for (i = 0; i < treeedge->route.routelen; i++) {
          if (gridsX[i] == gridsX[i + 1])  // a vertical edge
          {
            ymin = std::min(gridsY[i], gridsY[i + 1]);
            v_edges_[ymin * x_grid_ + gridsX[i]].est_usage -= edgeCost;
          } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
          {
            xmin = std::min(gridsX[i], gridsX[i + 1]);
            h_edges_[gridsY[i] * (x_grid_ - 1) + xmin].est_usage -= edgeCost;
          } else {
            logger_->error(GRT, 123, "Maze ripup wrong in newRipupNet for net {}.",
                          netName(nets_[netID]));
          }
        }
      }
    }
  }
}

}  // namespace grt
