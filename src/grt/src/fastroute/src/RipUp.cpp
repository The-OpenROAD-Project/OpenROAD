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

#include "DataProc.h"
#include "DataType.h"
#include "FastRoute.h"
#include "flute.h"
#include "utility.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

// rip-up a L segment
void FastRouteCore::ripupSegL(Segment* seg)
{
  int edgeCost = nets[seg->netID]->edgeCost;
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
    grid = seg->y1 * (xGrid - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges[grid + i].est_usage -= edgeCost;
    for (i = ymin; i < ymax; i++)
      v_edges[i * xGrid + seg->x2].est_usage -= edgeCost;
  } else {
    for (i = ymin; i < ymax; i++)
      v_edges[i * xGrid + seg->x1].est_usage -= edgeCost;
    grid = seg->y2 * (xGrid - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges[grid + i].est_usage -= edgeCost;
  }
}

void FastRouteCore::ripupSegZ(Segment* seg)
{
  int i, grid;
  int ymin, ymax;

  int edgeCost = nets[seg->netID]->edgeCost;

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
      v_edges[i * xGrid + seg->x1].est_usage -= edgeCost;
  } else if (seg->y1 == seg->y2) {
    // remove H routing
    grid = seg->y1 * (xGrid - 1);
    for (i = seg->x1; i < seg->x2; i++)
      h_edges[grid + i].est_usage -= edgeCost;
  } else {
    // remove Z routing
    if (seg->HVH) {
      grid = seg->y1 * (xGrid - 1);
      for (i = seg->x1; i < seg->Zpoint; i++)
        h_edges[grid + i].est_usage -= edgeCost;
      grid = seg->y2 * (xGrid - 1);
      for (i = seg->Zpoint; i < seg->x2; i++)
        h_edges[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges[i * xGrid + seg->Zpoint].est_usage -= edgeCost;
    } else {
      if (seg->y1 < seg->y2) {
        for (i = seg->y1; i < seg->Zpoint; i++)
          v_edges[i * xGrid + seg->x1].est_usage -= edgeCost;
        for (i = seg->Zpoint; i < seg->y2; i++)
          v_edges[i * xGrid + seg->x2].est_usage -= edgeCost;
        grid = seg->Zpoint * (xGrid - 1);
        for (i = seg->x1; i < seg->x2; i++)
          h_edges[grid + i].est_usage -= 1;
      } else {
        for (i = seg->y2; i < seg->Zpoint; i++)
          v_edges[i * xGrid + seg->x2].est_usage -= edgeCost;
        for (i = seg->Zpoint; i < seg->y1; i++)
          v_edges[i * xGrid + seg->x1].est_usage -= edgeCost;
        grid = seg->Zpoint * (xGrid - 1);
        for (i = seg->x1; i < seg->x2; i++)
          h_edges[grid + i].est_usage -= 1;
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
  short *gridsX, *gridsY;
  int i, grid, Zpoint, ymin, ymax, xmin;
  RouteType ripuptype;

  int edgeCost = nets[netID]->edgeCost;

  if (treeedge->len == 0) {
    return;  // not ripup for degraded edge
  }

  int n1 = treeedge->n1;
  int n2 = treeedge->n2;

  ripuptype = treeedge->route.type;
  if (y1 < y2) {
    ymin = y1;
    ymax = y2;
  } else {
    ymin = y2;
    ymax = y1;
  }

  if (ripuptype == LROUTE)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      grid = y1 * (xGrid - 1);
      for (i = x1; i < x2; i++)
        h_edges[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges[i * xGrid + x2].est_usage -= edgeCost;
    } else {
      for (i = ymin; i < ymax; i++)
        v_edges[i * xGrid + x1].est_usage -= edgeCost;
      grid = y2 * (xGrid - 1);
      for (i = x1; i < x2; i++)
        h_edges[grid + i].est_usage -= edgeCost;
    }
  } else if (ripuptype == ZROUTE) {
    // remove Z routing
    Zpoint = treeedge->route.Zpoint;
    if (treeedge->route.HVH) {
      grid = y1 * (xGrid - 1);
      for (i = x1; i < Zpoint; i++)
        h_edges[grid + i].est_usage -= edgeCost;
      grid = y2 * (xGrid - 1);
      for (i = Zpoint; i < x2; i++)
        h_edges[grid + i].est_usage -= edgeCost;
      for (i = ymin; i < ymax; i++)
        v_edges[i * xGrid + Zpoint].est_usage -= edgeCost;
    } else {
      if (y1 < y2) {
        for (i = y1; i < Zpoint; i++)
          v_edges[i * xGrid + x1].est_usage -= edgeCost;
        for (i = Zpoint; i < y2; i++)
          v_edges[i * xGrid + x2].est_usage -= edgeCost;
        grid = Zpoint * (xGrid - 1);
        for (i = x1; i < x2; i++)
          h_edges[grid + i].est_usage -= edgeCost;
      } else {
        for (i = y2; i < Zpoint; i++)
          v_edges[i * xGrid + x2].est_usage -= edgeCost;
        for (i = Zpoint; i < y1; i++)
          v_edges[i * xGrid + x1].est_usage -= edgeCost;
        grid = Zpoint * (xGrid - 1);
        for (i = x1; i < x2; i++)
          h_edges[grid + i].est_usage -= edgeCost;
      }
    }
  } else if (ripuptype == MAZEROUTE) {
    gridsX = treeedge->route.gridsX;
    gridsY = treeedge->route.gridsY;
    for (i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        v_edges[ymin * xGrid + gridsX[i]].est_usage -= edgeCost;
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        h_edges[gridsY[i] * (xGrid - 1) + xmin].est_usage -= edgeCost;
      } else {
        logger->error(GRT, 225, "Maze ripup wrong in newRipup.");
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

  int edgeCost = nets[netID]->edgeCost;

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

  if (ripuptype == LROUTE)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      grid = y1 * (xGrid - 1);
      for (i = x1; i < x2; i++) {
        if (h_edges[grid + i].est_usage > h_edges[grid + i].cap) {
          needRipup = true;
          break;
        }
      }

      for (i = ymin; i < ymax; i++) {
        if (v_edges[i * xGrid + x2].est_usage > v_edges[i * xGrid + x2].cap) {
          needRipup = true;
          break;
        }
      }
    } else {
      for (i = ymin; i < ymax; i++) {
        if (v_edges[i * xGrid + x1].est_usage > v_edges[i * xGrid + x1].cap) {
          needRipup = true;
          break;
        }
      }
      grid = y2 * (xGrid - 1);
      for (i = x1; i < x2; i++) {
        if (h_edges[grid + i].est_usage > h_edges[grid + i].cap) {
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

        grid = y1 * (xGrid - 1);
        for (i = x1; i < x2; i++)
          h_edges[grid + i].est_usage -= edgeCost;
        for (i = ymin; i < ymax; i++)
          v_edges[i * xGrid + x2].est_usage -= edgeCost;
      } else {
        if (n2 >= deg) {
          treenodes[n2].status -= 2;
        }
        treenodes[n1].status -= 1;

        for (i = ymin; i < ymax; i++)
          v_edges[i * xGrid + x1].est_usage -= edgeCost;
        grid = y2 * (xGrid - 1);
        for (i = x1; i < x2; i++)
          h_edges[grid + i].est_usage -= edgeCost;
      }
    }
    return (needRipup);

  } else {
    logger->error(GRT, 226, "Type2 ripup not type L.");
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
  short *gridsX, *gridsY;
  int i, grid, ymin, xmin;
  bool needRipup = false;

  int edgeCost = nets[netID]->edgeCost;

  if (treeedge->len == 0) {
    return false;
  }  // not ripup for degraded edge

  if (treeedge->route.type == MAZEROUTE) {
    gridsX = treeedge->route.gridsX;
    gridsY = treeedge->route.gridsY;
    for (i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = ymin * xGrid + gridsX[i];
        if (v_edges[grid].usage + v_edges[grid].red
            >= vCapacity - ripup_threshold) {
          needRipup = true;
          break;
        }
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsY[i] * (xGrid - 1) + xmin;
        if (h_edges[grid].usage + h_edges[grid].red
            >= hCapacity - ripup_threshold) {
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
          v_edges[ymin * xGrid + gridsX[i]].usage -= edgeCost;
        } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
        {
          xmin = std::min(gridsX[i], gridsX[i + 1]);
          h_edges[gridsY[i] * (xGrid - 1) + xmin].usage -= edgeCost;
        }
      }
      return true;
    } else {
      return false;
    }
  } else {
    printEdge(netID, edgeID);
    logger->error(GRT, 121, "Route type is not maze, netID {}.", netID);
  }
}

bool FastRouteCore::newRipup3DType3(int netID, int edgeID)
{
  short *gridsX, *gridsY, *gridsL;
  int i, k, grid, ymin, xmin, n1a, n2a, hl, bl, hid, bid,
      deg;
  std::vector<int> edge_cost_per_layer = nets[netID]->edge_cost_per_layer;

  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  treeedges = sttrees[netID].edges;
  treeedge = &(treeedges[edgeID]);

  if (treeedge->len == 0) {
    return false;  // not ripup for degraded edge
  }

  treenodes = sttrees[netID].nodes;

  deg = sttrees[netID].deg;

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

  gridsX = treeedge->route.gridsX;
  gridsY = treeedge->route.gridsY;
  gridsL = treeedge->route.gridsL;
  for (i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1])  // a vertical edge
      {
        ymin = std::min(gridsY[i], gridsY[i + 1]);
        grid = gridsL[i] * gridV + ymin * xGrid + gridsX[i];
        v_edges3D[grid].usage -= edge_cost_per_layer[gridsL[i]];
      } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
      {
        xmin = std::min(gridsX[i], gridsX[i + 1]);
        grid = gridsL[i] * gridH + gridsY[i] * (xGrid - 1) + xmin;
        h_edges3D[grid].usage -= edge_cost_per_layer[gridsL[i]];
      } else {
        logger->error(GRT, 122, "Maze ripup wrong.");
      }
    }
  }

  return true;
}

void FastRouteCore::newRipupNet(int netID)
{
  short *gridsX, *gridsY;
  int i, grid, Zpoint, ymin, ymax, xmin, n1, n2, edgeID;

  int edgeCost = nets[netID]->edgeCost;

  RouteType ripuptype;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;
  int x1, y1, x2, y2, deg;

  treeedges = sttrees[netID].edges;
  treenodes = sttrees[netID].nodes;
  deg = sttrees[netID].deg;

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

      if (ripuptype == LROUTE)  // remove L routing
      {
        if (treeedge->route.xFirst) {
          grid = y1 * (xGrid - 1);
          for (i = x1; i < x2; i++)
            h_edges[grid + i].est_usage -= edgeCost;
          for (i = ymin; i < ymax; i++)
            v_edges[i * xGrid + x2].est_usage -= edgeCost;
        } else {
          for (i = ymin; i < ymax; i++)
            v_edges[i * xGrid + x1].est_usage -= edgeCost;
          grid = y2 * (xGrid - 1);
          for (i = x1; i < x2; i++)
            h_edges[grid + i].est_usage -= edgeCost;
        }
      } else if (ripuptype == ZROUTE) {
        // remove Z routing
        Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH) {
          grid = y1 * (xGrid - 1);
          for (i = x1; i < Zpoint; i++)
            h_edges[grid + i].est_usage -= edgeCost;
          grid = y2 * (xGrid - 1);
          for (i = Zpoint; i < x2; i++)
            h_edges[grid + i].est_usage -= edgeCost;
          for (i = ymin; i < ymax; i++)
            v_edges[i * xGrid + Zpoint].est_usage -= edgeCost;
        } else {
          if (y1 < y2) {
            for (i = y1; i < Zpoint; i++)
              v_edges[i * xGrid + x1].est_usage -= edgeCost;
            for (i = Zpoint; i < y2; i++)
              v_edges[i * xGrid + x2].est_usage -= edgeCost;
            grid = Zpoint * (xGrid - 1);
            for (i = x1; i < x2; i++)
              h_edges[grid + i].est_usage -= edgeCost;
          } else {
            for (i = y2; i < Zpoint; i++)
              v_edges[i * xGrid + x2].est_usage -= edgeCost;
            for (i = Zpoint; i < y1; i++)
              v_edges[i * xGrid + x1].est_usage -= edgeCost;
            grid = Zpoint * (xGrid - 1);
            for (i = x1; i < x2; i++)
              h_edges[grid + i].est_usage -= edgeCost;
          }
        }
      } else if (ripuptype == MAZEROUTE) {
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;
        for (i = 0; i < treeedge->route.routelen; i++) {
          if (gridsX[i] == gridsX[i + 1])  // a vertical edge
          {
            ymin = std::min(gridsY[i], gridsY[i + 1]);
            v_edges[ymin * xGrid + gridsX[i]].est_usage -= edgeCost;
          } else if (gridsY[i] == gridsY[i + 1])  // a horizontal edge
          {
            xmin = std::min(gridsX[i], gridsX[i + 1]);
            h_edges[gridsY[i] * (xGrid - 1) + xmin].est_usage -= edgeCost;
          } else {
            logger->error(GRT, 123, "Maze ripup wrong in newRipupNet for net {}.",
                          netName(nets[netID]));
          }
        }
      }
    }
  }
}

}  // namespace grt
