// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
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
  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  // remove L routing
  if (seg->xFirst) {
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, -edgeCost);
    graph2d_.updateEstUsageV(seg->x2, {ymin, ymax}, net, -edgeCost);
  } else {
    graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, -edgeCost);
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y2, net, -edgeCost);
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

  const RouteType edge_type = treeedge->route.type;
  const auto [ymin, ymax] = std::minmax(y1, y2);
  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

  if (edge_type == RouteType::LRoute)  // remove L routing
  {
    if (treeedge->route.xFirst) {
      graph2d_.updateEstUsageH({x1, x2}, y1, net, -edgeCost);
      graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, -edgeCost);
    } else {
      graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, -edgeCost);
      graph2d_.updateEstUsageH({x1, x2}, y2, net, -edgeCost);
    }
  } else if (edge_type == RouteType::ZRoute) {
    // remove Z routing
    const int Zpoint = treeedge->route.Zpoint;
    if (treeedge->route.HVH) {
      graph2d_.updateEstUsageH({x1, Zpoint}, y1, net, -edgeCost);
      graph2d_.updateEstUsageV(Zpoint, {ymin, ymax}, net, -edgeCost);
      graph2d_.updateEstUsageH({Zpoint, x2}, y2, net, -edgeCost);
    } else {
      if (y1 < y2) {
        graph2d_.updateEstUsageV(x1, {y1, Zpoint}, net, -edgeCost);
        graph2d_.updateEstUsageH({x1, x2}, Zpoint, net, -edgeCost);
        graph2d_.updateEstUsageV(x2, {Zpoint, y2}, net, -edgeCost);
      } else {
        graph2d_.updateEstUsageV(x1, {Zpoint, y1}, net, -edgeCost);
        graph2d_.updateEstUsageH({x1, x2}, Zpoint, net, -edgeCost);
        graph2d_.updateEstUsageV(x2, {y2, Zpoint}, net, -edgeCost);
      }
    }
  } else if (edge_type == RouteType::MazeRoute) {
    const std::vector<GPoint3D>& grids = treeedge->route.grids;
    for (int i = 0; i < treeedge->route.routelen; i++) {
      if (grids[i].x == grids[i + 1].x) {  // a vertical edge
        const int ymin = std::min(grids[i].y, grids[i + 1].y);
        graph2d_.updateEstUsageV(grids[i].x, ymin, net, -edgeCost);
      } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
        const int xmin = std::min(grids[i].x, grids[i + 1].x);
        graph2d_.updateEstUsageH(xmin, grids[i].y, net, -edgeCost);
      } else {
        logger_->error(GRT, 225, "Maze ripup wrong in newRipup.");
      }
    }
  }
}

// remove L routing if passing through congested edges
bool FastRouteCore::newRipupCongestedL(const TreeEdge* treeedge,
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

  const RouteType edge_type = treeedge->route.type;
  if (edge_type != RouteType::LRoute) {
    logger_->error(GRT,
                   226,
                   "Net {} ripup type is {}. Expected LRoute.",
                   nets_[netID]->getName(),
                   edge_type);
  }

  const auto [ymin, ymax] = std::minmax(y1, y2);

  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

  bool needRipup = false;

  // The x value for the vertical check
  const int x_check = treeedge->route.xFirst ? x2 : x1;
  // The y value for the horizontal check
  const int y_check = treeedge->route.xFirst ? y1 : y2;

  for (int i = ymin; i < ymax; i++) {
    const int cap
        = getEdgeCapacity(nets_[netID], x_check, i, EdgeDirection::Vertical);
    if (graph2d_.getEstUsageV(x_check, i) > cap) {
      needRipup = true;
      break;
    }
  }
  if (!needRipup) {
    for (int i = x1; i < x2; i++) {
      const int cap = getEdgeCapacity(
          nets_[netID], i, y_check, EdgeDirection::Horizontal);
      if (graph2d_.getEstUsageH(i, y_check) > cap) {
        needRipup = true;
        break;
      }
    }
  }

  if (needRipup) {
    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;

    if (treeedge->route.xFirst) {
      if (n1 >= deg) {
        treenodes[n1].status -= 2;
      }
      treenodes[n2].status -= 1;

      graph2d_.updateEstUsageH({x1, x2}, y1, net, -edgeCost);
      graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, -edgeCost);
    } else {
      if (n2 >= deg) {
        treenodes[n2].status -= 2;
      }
      treenodes[n1].status -= 1;

      graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, -edgeCost);
      graph2d_.updateEstUsageH({x1, x2}, y2, net, -edgeCost);
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
  FrNet* net = nets_[netID];

  const std::vector<GPoint3D>& grids = treeedge->route.grids;
  for (int i = 0; i < treeedge->route.routelen; i++) {
    if (grids[i].x == grids[i + 1].x) {  // a vertical edge
      const int ymin = std::min(grids[i].y, grids[i + 1].y);

      if (graph2d_.getUsageRedV(grids[i].x, ymin)
          >= v_capacity_ - ripup_threshold) {
        needRipup = true;
        break;
      }
    } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
      const int xmin = std::min(grids[i].x, grids[i + 1].x);

      if (graph2d_.getUsageRedH(xmin, grids[i].y)
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
    const int8_t edgeCost = net->getEdgeCost();

    for (int i = 0; i < treeedge->route.routelen; i++) {
      if (grids[i].x == grids[i + 1].x) {  // a vertical edge
        const int ymin = std::min(grids[i].y, grids[i + 1].y);
        graph2d_.updateUsageV(grids[i].x, ymin, net, -edgeCost);
      } else {  /// if(grids[i].y==grids[i+1].y)// a horizontal edge
        const int xmin = std::min(grids[i].x, grids[i + 1].x);
        graph2d_.updateUsageH(xmin, grids[i].y, net, -edgeCost);
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

  int bl, hl;
  if (n1a < num_terminals) {
    const int pin_idx = sttrees_[netID].node_to_pin_idx[n1a];
    bl = nets_[netID]->getPinL()[pin_idx];
    hl = nets_[netID]->getPinL()[pin_idx];
  } else {
    bl = BIG_INT;
    hl = 0;
  }
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

  if (n2a < num_terminals) {
    const int pin_idx = sttrees_[netID].node_to_pin_idx[n2a];
    bl = nets_[netID]->getPinL()[pin_idx];
    hl = nets_[netID]->getPinL()[pin_idx];
  } else {
    bl = BIG_INT;
    hl = 0;
  }
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

  const std::vector<GPoint3D>& grids = treeedge->route.grids;
  for (int i = 0; i < treeedge->route.routelen; i++) {
    if (grids[i].layer == grids[i + 1].layer) {
      if (grids[i].x == grids[i + 1].x) {  // a vertical edge
        const int ymin = std::min(grids[i].y, grids[i + 1].y);
        graph2d_.updateUsageV(grids[i].x, ymin, net, -net->getEdgeCost());
        v_edges_3D_[grids[i].layer][ymin][grids[i].x].usage
            -= net->getLayerEdgeCost(grids[i].layer);
      } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
        const int xmin = std::min(grids[i].x, grids[i + 1].x);
        graph2d_.updateUsageH(xmin, grids[i].y, net, -net->getEdgeCost());
        h_edges_3D_[grids[i].layer][grids[i].y][xmin].usage
            -= net->getLayerEdgeCost(grids[i].layer);
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
  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();
  const auto& treeedges = sttrees_[netID].edges;
  const int num_edges = sttrees_[netID].num_edges();

  // Only release resources if they were created at first place.
  // Cases like "read_guides" can call this function multiple times,
  // without creating treeedges inside the core code.
  if (!treeedges.empty()) {
    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      const std::vector<GPoint3D>& grids = treeedge->route.grids;
      const int routeLen = treeedge->route.routelen;

      for (int i = 0; i < routeLen; i++) {
        if (grids[i].layer != grids[i + 1].layer) {
          continue;
        }
        if (grids[i].x == grids[i + 1].x) {  // a vertical edge
          const int ymin = std::min(grids[i].y, grids[i + 1].y);
          graph2d_.updateUsageV(grids[i].x, ymin, net, -edgeCost);
          Edge3D* edge_3D = &v_edges_3D_[grids[i].layer][ymin][grids[i].x];
          edge_3D->usage -= net->getLayerEdgeCost(grids[i].layer);
        } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
          const int xmin = std::min(grids[i].x, grids[i + 1].x);
          graph2d_.updateUsageH(xmin, grids[i].y, net, -edgeCost);
          Edge3D* edge_3D = &h_edges_3D_[grids[i].layer][grids[i].y][xmin];
          edge_3D->usage -= nets_[netID]->getLayerEdgeCost(grids[i].layer);
        }
      }
    }
  }
}

void FastRouteCore::newRipupNet(const int netID)
{
  const int8_t edgeCost = nets_[netID]->getEdgeCost();

  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;
  const int num_edges = sttrees_[netID].num_edges();

  FrNet* net = nets_[netID];

  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;

      const RouteType edge_type = treeedge->route.type;
      const auto [ymin, ymax] = std::minmax(y1, y2);

      if (edge_type == RouteType::LRoute)  // remove L routing
      {
        if (treeedge->route.xFirst) {
          graph2d_.updateEstUsageH({x1, x2}, y1, net, -edgeCost);
          graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, -edgeCost);
        } else {
          graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, -edgeCost);
          graph2d_.updateEstUsageH({x1, x2}, y2, net, -edgeCost);
        }
      } else if (edge_type == RouteType::ZRoute) {
        // remove Z routing
        const int Zpoint = treeedge->route.Zpoint;
        if (treeedge->route.HVH) {
          graph2d_.updateEstUsageH({x1, Zpoint}, y1, net, -edgeCost);
          graph2d_.updateEstUsageV(Zpoint, {ymin, ymax}, net, -edgeCost);
          graph2d_.updateEstUsageH({Zpoint, x2}, y2, net, -edgeCost);
        } else {
          if (y1 < y2) {
            graph2d_.updateEstUsageV(x1, {y1, Zpoint}, net, -edgeCost);
            graph2d_.updateEstUsageH({x1, x2}, Zpoint, net, -edgeCost);
            graph2d_.updateEstUsageV(x2, {Zpoint, y2}, net, -edgeCost);
          } else {
            graph2d_.updateEstUsageV(x1, {Zpoint, y1}, net, -edgeCost);
            graph2d_.updateEstUsageH({x1, x2}, Zpoint, net, -edgeCost);
            graph2d_.updateEstUsageV(x2, {y2, Zpoint}, net, -edgeCost);
          }
        }
      } else if (edge_type == RouteType::MazeRoute) {
        const std::vector<GPoint3D>& grids = treeedge->route.grids;
        for (int i = 0; i < treeedge->route.routelen; i++) {
          if (grids[i].x == grids[i + 1].x) {  // a vertical edge
            const int ymin = std::min(grids[i].y, grids[i + 1].y);
            graph2d_.updateEstUsageV(grids[i].x, ymin, net, -edgeCost);
          } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
            const int xmin = std::min(grids[i].x, grids[i + 1].x);
            graph2d_.updateEstUsageH(xmin, grids[i].y, net, -edgeCost);
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
