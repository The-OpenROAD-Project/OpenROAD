// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

// rip-up a L segment
void FastRouteCore::ripupSegL(const Segment* seg)
{
  const int edgeCost = nets_[seg->netID]->getEdgeCost();

  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  // remove L routing
  if (seg->xFirst) {
    graph2d_.addEstUsageH({seg->x1, seg->x2}, seg->y1, -edgeCost);
    graph2d_.addEstUsageV(seg->x2, {ymin, ymax}, -edgeCost);
  } else {
    graph2d_.addEstUsageV(seg->x1, {ymin, ymax}, -edgeCost);
    graph2d_.addEstUsageH({seg->x1, seg->x2}, seg->y2, -edgeCost);
  }
}

void FastRouteCore::newRipup(const TreeEdge* treeedge,
                             const int x1,
                             const int y1,
                             const int x2,
                             const int y2,
                             const int netID)
{
  if (treeedge->len == 0) {
    return;  // not ripup for degraded edge
  }

  const RouteType ripuptype = treeedge->route.type;
  const auto [ymin, ymax] = std::minmax(y1, y2);
  const int edgeCost = nets_[netID]->getEdgeCost();

  if (ripuptype == RouteType::LRoute)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      graph2d_.addEstUsageH({x1, x2}, y1, -edgeCost);
      graph2d_.addEstUsageV(x2, {ymin, ymax}, -edgeCost);
    } else {
      graph2d_.addEstUsageV(x1, {ymin, ymax}, -edgeCost);
      graph2d_.addEstUsageH({x1, x2}, y2, -edgeCost);
    }
  } else if (ripuptype == RouteType::ZRoute) {
    // remove Z routing
    const int Zpoint = treeedge->route.Zpoint;
    if (treeedge->route.HVH) {
      graph2d_.addEstUsageH({x1, Zpoint}, y1, -edgeCost);
      graph2d_.addEstUsageV(Zpoint, {ymin, ymax}, -edgeCost);
      graph2d_.addEstUsageH({Zpoint, x2}, y2, -edgeCost);
    } else {
      if (y1 < y2) {
        graph2d_.addEstUsageV(x1, {y1, Zpoint}, -edgeCost);
        graph2d_.addEstUsageH({x1, x2}, Zpoint, -edgeCost);
        graph2d_.addEstUsageV(x2, {Zpoint, y2}, -edgeCost);
      } else {
        graph2d_.addEstUsageV(x1, {Zpoint, y1}, -edgeCost);
        graph2d_.addEstUsageH({x1, x2}, Zpoint, -edgeCost);
        graph2d_.addEstUsageV(x2, {y2, Zpoint}, -edgeCost);
      }
    }
  } else if (ripuptype == RouteType::MazeRoute) {
    const std::vector<short>& gridsX = treeedge->route.gridsX;
    const std::vector<short>& gridsY = treeedge->route.gridsY;
    for (int i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
        const int ymin = std::min(gridsY[i], gridsY[i + 1]);
        graph2d_.addEstUsageV(gridsX[i], ymin, -edgeCost);
      } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
        const int xmin = std::min(gridsX[i], gridsX[i + 1]);
        graph2d_.addEstUsageH(xmin, gridsY[i], -edgeCost);
      } else {
        logger_->error(GRT, 225, "Maze ripup wrong in newRipup.");
      }
    }
  }
}

// remove L routing
bool FastRouteCore::newRipupType2(const TreeEdge* treeedge,
                                  std::vector<TreeNode>& treenodes,
                                  const int x1,
                                  const int y1,
                                  const int x2,
                                  const int y2,
                                  const int deg,
                                  const int netID)
{
  if (treeedge->len == 0) {
    return false;  // no ripup for degraded edge
  }

  const RouteType ripuptype = treeedge->route.type;
  if (ripuptype != RouteType::LRoute) {
    logger_->error(GRT,
                   226,
                   "Net {} ripup type is {}. Expected LRoute.",
                   nets_[netID]->getName(),
                   ripuptype);
  }

  const auto [ymin, ymax] = std::minmax(y1, y2);

  bool needRipup = false;

  if (treeedge->route.xFirst) {
    for (int i = x1; i < x2; i++) {
      const int cap
          = getEdgeCapacity(nets_[netID], i, y1, EdgeDirection::Horizontal);
      if (graph2d_.getEstUsageH(i, y1) > cap) {
        needRipup = true;
        break;
      }
    }

    for (int i = ymin; i < ymax; i++) {
      const int cap
          = getEdgeCapacity(nets_[netID], x2, i, EdgeDirection::Vertical);
      if (graph2d_.getEstUsageV(x2, i) > cap) {
        needRipup = true;
        break;
      }
    }
  } else {
    for (int i = ymin; i < ymax; i++) {
      const int cap
          = getEdgeCapacity(nets_[netID], x1, i, EdgeDirection::Vertical);
      if (graph2d_.getEstUsageV(x1, i) > cap) {
        needRipup = true;
        break;
      }
    }
    for (int i = x1; i < x2; i++) {
      const int cap
          = getEdgeCapacity(nets_[netID], i, y2, EdgeDirection::Horizontal);
      if (graph2d_.getEstUsageH(i, y2) > cap) {
        needRipup = true;
        break;
      }
    }
  }

  if (needRipup) {
    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    const int edgeCost = nets_[netID]->getEdgeCost();

    if (treeedge->route.xFirst) {
      if (n1 >= deg) {
        treenodes[n1].status -= 2;
      }
      treenodes[n2].status -= 1;

      graph2d_.addEstUsageH({x1, x2}, y1, -edgeCost);
      graph2d_.addEstUsageV(x2, {ymin, ymax}, -edgeCost);
    } else {
      if (n2 >= deg) {
        treenodes[n2].status -= 2;
      }
      treenodes[n1].status -= 1;

      graph2d_.addEstUsageV(x1, {ymin, ymax}, -edgeCost);
      graph2d_.addEstUsageH({x1, x2}, y2, -edgeCost);
    }
  }
  return needRipup;
}

bool FastRouteCore::newRipupCheck(const TreeEdge* treeedge,
                                  const int x1,
                                  const int y1,
                                  const int x2,
                                  const int y2,
                                  const int ripup_threshold,
                                  const float critical_slack,
                                  const int netID,
                                  const int edgeID)
{
  if (treeedge->len == 0) {
    return false;
  }  // no ripup for degraded edge

  if (treeedge->route.type != RouteType::MazeRoute) {
    printEdge(netID, edgeID);
    logger_->error(GRT, 500, "Route type is not maze, netID {}.", netID);
  }

  bool needRipup = false;

  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;
  for (int i = 0; i < treeedge->route.routelen; i++) {
    if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
      const int ymin = std::min(gridsY[i], gridsY[i + 1]);
      if (graph2d_.getUsageRedV(gridsX[i], ymin)
          >= v_capacity_ - ripup_threshold) {
        needRipup = true;
        break;
      }
    } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
      const int xmin = std::min(gridsX[i], gridsX[i + 1]);
      if (graph2d_.getUsageRedH(xmin, gridsY[i])
          >= h_capacity_ - ripup_threshold) {
        needRipup = true;
        break;
      }
    }
  }
  if (!needRipup && critical_nets_percentage_ && treeedge->route.last_routelen
      && critical_slack) {
    const float delta = (float) treeedge->route.routelen
                        / (float) treeedge->route.last_routelen;
    if (nets_[netID]->getSlack() <= critical_slack
        && (nets_[netID]->getSlack()
            > std::ceil(std::numeric_limits<float>::lowest()))
        && (delta >= 2)) {
      nets_[netID]->setIsCritical(true);
      needRipup = true;
    }
  }
  if (needRipup) {
    const int edgeCost = nets_[netID]->getEdgeCost();

    for (int i = 0; i < treeedge->route.routelen; i++) {
      if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
        const int ymin = std::min(gridsY[i], gridsY[i + 1]);
        graph2d_.addUsageV(gridsX[i], ymin, -edgeCost);
      } else {  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
        const int xmin = std::min(gridsX[i], gridsX[i + 1]);
        graph2d_.addUsageH(xmin, gridsY[i], -edgeCost);
      }
    }
  }
  return needRipup;
}

bool FastRouteCore::newRipup3DType3(const int netID, const int edgeID)
{
  FrNet* net = nets_[netID];

  const auto& treeedges = sttrees_[netID].edges;
  const TreeEdge* treeedge = &(treeedges[edgeID]);

  if (treeedge->len == 0) {
    return false;  // not ripup for degraded edge
  }

  auto& treenodes = sttrees_[netID].nodes;

  const int num_terminals = sttrees_[netID].num_terminals;

  const int n1a = treeedge->n1a;
  const int n2a = treeedge->n2a;

  int bl = (n1a < num_terminals) ? nets_[netID]->getPinL()[n1a] : BIG_INT;
  int hl = (n1a < num_terminals) ? nets_[netID]->getPinL()[n1a] : 0;
  int hid = BIG_INT;
  int bid = BIG_INT;

  for (int i = 0; i < treenodes[n1a].conCNT; i++) {
    if (treenodes[n1a].eID[i] == edgeID) {
      for (int k = i + 1; k < treenodes[n1a].conCNT; k++) {
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
    }
    if (bl > treenodes[n1a].heights[i]) {
      bl = treenodes[n1a].heights[i];
      bid = treenodes[n1a].eID[i];
    }
    if (hl < treenodes[n1a].heights[i]) {
      hl = treenodes[n1a].heights[i];
      hid = treenodes[n1a].eID[i];
    }
  }
  treenodes[n1a].conCNT--;

  treenodes[n1a].botL = bl;
  treenodes[n1a].lID = bid;
  treenodes[n1a].topL = hl;
  treenodes[n1a].hID = hid;

  bl = (n2a < num_terminals) ? nets_[netID]->getPinL()[n2a] : BIG_INT;
  hl = (n2a < num_terminals) ? nets_[netID]->getPinL()[n2a] : 0;
  hid = bid = BIG_INT;

  for (int i = 0; i < treenodes[n2a].conCNT; i++) {
    if (treenodes[n2a].eID[i] == edgeID) {
      for (int k = i + 1; k < treenodes[n2a].conCNT; k++) {
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
    }
    if (bl > treenodes[n2a].heights[i]) {
      bl = treenodes[n2a].heights[i];
      bid = treenodes[n2a].eID[i];
    }
    if (hl < treenodes[n2a].heights[i]) {
      hl = treenodes[n2a].heights[i];
      hid = treenodes[n2a].eID[i];
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
  for (int i = 0; i < treeedge->route.routelen; i++) {
    if (gridsL[i] == gridsL[i + 1]) {
      if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
        const int ymin = std::min(gridsY[i], gridsY[i + 1]);
        graph2d_.addUsageV(gridsX[i], ymin, -net->getEdgeCost());
        v_edges_3D_[gridsL[i]][ymin][gridsX[i]].usage
            -= net->getLayerEdgeCost(gridsL[i]);
      } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
        const int xmin = std::min(gridsX[i], gridsX[i + 1]);
        graph2d_.addUsageH(xmin, gridsY[i], -net->getEdgeCost());
        h_edges_3D_[gridsL[i]][gridsY[i]][xmin].usage
            -= net->getLayerEdgeCost(gridsL[i]);
      } else {
        logger_->error(
            GRT, 122, "Maze ripup wrong for net {}.", nets_[netID]->getName());
      }
    }
  }

  return true;
}

void FastRouteCore::releaseNetResources(const int netID)
{
  const int edgeCost = nets_[netID]->getEdgeCost();
  const auto& treeedges = sttrees_[netID].edges;
  const int num_edges = sttrees_[netID].num_edges();

  // Only release resources if they were created at first place.
  // Cases like "read_guides" can call this function multiple times,
  // without creating treeedges inside the core code.
  if (!treeedges.empty()) {
    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;
      const std::vector<short>& gridsL = treeedge->route.gridsL;
      const int routeLen = treeedge->route.routelen;

      for (int i = 0; i < routeLen; i++) {
        if (gridsL[i] != gridsL[i + 1]) {
          continue;
        }
        if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
          const int ymin = std::min(gridsY[i], gridsY[i + 1]);
          graph2d_.addUsageV(gridsX[i], ymin, -edgeCost);
          Edge3D* edge_3D = &v_edges_3D_[gridsL[i]][ymin][gridsX[i]];
          edge_3D->usage -= nets_[netID]->getLayerEdgeCost(gridsL[i]);
        } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
          const int xmin = std::min(gridsX[i], gridsX[i + 1]);
          graph2d_.addUsageH(xmin, gridsY[i], -edgeCost);
          Edge3D* edge_3D = &h_edges_3D_[gridsL[i]][gridsY[i]][xmin];
          edge_3D->usage -= nets_[netID]->getLayerEdgeCost(gridsL[i]);
        }
      }
    }
  }
}

void FastRouteCore::newRipupNet(const int netID)
{
  const int edgeCost = nets_[netID]->getEdgeCost();

  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;
  const int num_edges = sttrees_[netID].num_edges();

  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;

      const RouteType ripuptype = treeedge->route.type;
      const auto [ymin, ymax] = std::minmax(y1, y2);

      if (ripuptype == RouteType::LRoute)  // remove L routing
      {
        if (treeedge->route.xFirst) {
          graph2d_.addEstUsageH({x1, x2}, y1, -edgeCost);
          graph2d_.addEstUsageV(x2, {ymin, ymax}, -edgeCost);
        } else {
          graph2d_.addEstUsageV(x1, {ymin, ymax}, -edgeCost);
          graph2d_.addEstUsageH({x1, x2}, y2, -edgeCost);
        }
      } else if (ripuptype == RouteType::ZRoute) {
        // remove Z routing
        const int Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH) {
          graph2d_.addEstUsageH({x1, Zpoint}, y1, -edgeCost);
          graph2d_.addEstUsageV(Zpoint, {ymin, ymax}, -edgeCost);
          graph2d_.addEstUsageH({Zpoint, x2}, y2, -edgeCost);
        } else {
          if (y1 < y2) {
            graph2d_.addEstUsageV(x1, {y1, Zpoint}, -edgeCost);
            graph2d_.addEstUsageH({x1, x2}, Zpoint, -edgeCost);
            graph2d_.addEstUsageV(x2, {Zpoint, y2}, -edgeCost);
          } else {
            graph2d_.addEstUsageV(x1, {Zpoint, y1}, -edgeCost);
            graph2d_.addEstUsageH({x1, x2}, Zpoint, -edgeCost);
            graph2d_.addEstUsageV(x2, {y2, Zpoint}, -edgeCost);
          }
        }
      } else if (ripuptype == RouteType::MazeRoute) {
        const std::vector<short>& gridsX = treeedge->route.gridsX;
        const std::vector<short>& gridsY = treeedge->route.gridsY;
        for (int i = 0; i < treeedge->route.routelen; i++) {
          if (gridsX[i] == gridsX[i + 1]) {  // a vertical edge
            const int ymin = std::min(gridsY[i], gridsY[i + 1]);
            graph2d_.addEstUsageV(gridsX[i], ymin, -edgeCost);
          } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
            const int xmin = std::min(gridsX[i], gridsX[i + 1]);
            graph2d_.addEstUsageH(xmin, gridsY[i], -edgeCost);
          } else {
            logger_->error(GRT,
                           123,
                           "Maze ripup wrong in newRipupNet for net {}.",
                           nets_[netID]->getName());
          }
        }
      }
    }
  }
}

}  // namespace grt
