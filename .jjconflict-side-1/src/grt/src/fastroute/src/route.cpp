// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

// estimate the routing by assigning 1 for H and V segments, 0.5 to both
// possible L for L segments
void FastRouteCore::estimateOneSeg(const Segment* seg)
{
  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  // assign 0.5 to both Ls (x1,y1)-(x1,y2) + (x1,y2)-(x2,y2) + (x1,y1)-(x2,y1) +
  // (x2,y1)-(x2,y2)
  if (seg->x1 == seg->x2) {  // a vertical segment
    graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, edgeCost);
  } else if (seg->y1 == seg->y2) {  // a horizontal segment
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, edgeCost);
  } else {  // a diagonal segment
    graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageV(seg->x2, {ymin, ymax}, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y2, net, edgeCost / 2.0f);
  }
}

void FastRouteCore::routeSegV(const Segment* seg)
{
  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, edgeCost);
}

void FastRouteCore::routeSegH(const Segment* seg)
{
  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, edgeCost);
}

// L-route, based on previous L route
void FastRouteCore::routeSegL(Segment* seg)
{
  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  if (seg->x1 == seg->x2) {  // V route
    routeSegV(seg);
  } else if (seg->y1 == seg->y2) {  // H route
    routeSegH(seg);
  } else {  // L route
    double costL1 = 0;
    double costL2 = 0;

    for (int i = ymin; i < ymax; i++) {
      const double tmp1 = graph2d_.getEstUsageRedV(seg->x1, i) - v_capacity_lb_;
      if (tmp1 > 0) {
        costL1 += tmp1;
      }
      const double tmp2 = graph2d_.getEstUsageRedV(seg->x2, i) - v_capacity_lb_;
      if (tmp2 > 0) {
        costL2 += tmp2;
      }
    }
    for (int i = seg->x1; i < seg->x2; i++) {
      const double tmp1 = graph2d_.getEstUsageRedH(i, seg->y2) - h_capacity_lb_;
      if (tmp1 > 0) {
        costL1 += tmp1;
      }
      const double tmp2 = graph2d_.getEstUsageRedH(i, seg->y1) - h_capacity_lb_;
      if (tmp2 > 0) {
        costL2 += tmp2;
      }
    }

    if (costL1 < costL2) {
      // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
      graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, edgeCost);
      graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y2, net, edgeCost);
      seg->xFirst = false;
    } else {
      // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
      graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, edgeCost);
      graph2d_.updateEstUsageV(seg->x2, {ymin, ymax}, net, edgeCost);
      seg->xFirst = true;
    }
  }
}

// First time L-route, based on 0.5-0.5 estimation
void FastRouteCore::routeSegLFirstTime(Segment* seg)
{
  const auto [ymin, ymax] = std::minmax(seg->y1, seg->y2);

  double costL1 = 0;
  double costL2 = 0;

  for (int i = ymin; i < ymax; i++) {
    const double tmp = graph2d_.getEstUsageRedV(seg->x1, i) - v_capacity_lb_;
    if (tmp > 0) {
      costL1 += tmp;
    }
  }
  for (int i = ymin; i < ymax; i++) {
    const double tmp = graph2d_.getEstUsageRedV(seg->x2, i) - v_capacity_lb_;
    if (tmp > 0) {
      costL2 += tmp;
    }
  }

  for (int i = seg->x1; i < seg->x2; i++) {
    const double tmp = graph2d_.getEstUsageRedH(i, seg->y2) - h_capacity_lb_;
    if (tmp > 0) {
      costL1 += tmp;
    }
  }
  for (int i = seg->x1; i < seg->x2; i++) {
    const double tmp = graph2d_.getEstUsageRedH(i, seg->y1) - h_capacity_lb_;
    if (tmp > 0) {
      costL2 += tmp;
    }
  }

  FrNet* net = nets_[seg->netID];
  const int8_t edgeCost = seg->cost;

  if (costL1 < costL2) {
    // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
    graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageV(seg->x2, {ymin, ymax}, net, -edgeCost / 2.0f);

    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y2, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageH(
        {seg->x1, seg->x2}, seg->y1, net, -edgeCost / 2.0f);

    seg->xFirst = false;
  } else {
    // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
    graph2d_.updateEstUsageH({seg->x1, seg->x2}, seg->y1, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageH(
        {seg->x1, seg->x2}, seg->y2, net, -edgeCost / 2.0f);

    graph2d_.updateEstUsageV(seg->x2, {ymin, ymax}, net, edgeCost / 2.0f);
    graph2d_.updateEstUsageV(seg->x1, {ymin, ymax}, net, -edgeCost / 2.0f);

    seg->xFirst = true;
  }
}

// route all segments with L, firstTime: true, no previous route, false -
// previous is L-route
void FastRouteCore::routeLAll(const bool firstTime)
{
  if (firstTime) {  // no previous route
    // estimate congestion with 0.5+0.5 L
    for (const int netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        estimateOneSeg(&seg);
      }
    }
    // L route
    for (const int netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        // no need to reroute the H or V segs
        if (seg.x1 != seg.x2 && seg.y1 != seg.y2) {
          routeSegLFirstTime(&seg);
        }
      }
    }
  } else {  // previous is L-route
    for (const int netID : net_ids_) {
      for (auto& seg : seglist_[netID]) {
        // no need to reroute the H or V segs
        if (seg.x1 != seg.x2 && seg.y1 != seg.y2) {
          ripupSegL(&seg);
          routeSegL(&seg);
        }
      }
    }
  }
}

// L-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteL(const int netID,
                              const RouteType ripuptype,
                              const bool viaGuided)
{
  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

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

      const auto [ymin, ymax] = std::minmax(y1, y2);

      // ripup the original routing
      if (ripuptype > RouteType::NoRoute) {  // it's been routed
        newRipup(treeedge, x1, y1, x2, y2, netID);
      }

      treeedge->route.type = RouteType::LRoute;
      if (x1 == x2) {  // V-routing
        graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
        treeedge->route.xFirst = false;
        if (treenodes[n1].status % 2 == 0) {
          treenodes[n1].status += 1;
        }
        if (treenodes[n2].status % 2 == 0) {
          treenodes[n2].status += 1;
        }
      } else if (y1 == y2) {  // H-routing
        graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
        treeedge->route.xFirst = true;
        if (treenodes[n2].status < 2) {
          treenodes[n2].status += 2;
        }
        if (treenodes[n1].status < 2) {
          treenodes[n1].status += 2;
        }
      } else {  // L-routing
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
          const double tmp1 = graph2d_.getEstUsageRedV(x1, j) - v_capacity_lb_;
          if (tmp1 > 0) {
            costL1 += tmp1;
          }
          const double tmp2 = graph2d_.getEstUsageRedV(x2, j) - v_capacity_lb_;
          if (tmp2 > 0) {
            costL2 += tmp2;
          }
        }
        for (int j = x1; j < x2; j++) {
          const double tmp1 = graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_;
          if (tmp1 > 0) {
            costL1 += tmp1;
          }
          const double tmp2 = graph2d_.getEstUsageRedH(j, y1) - h_capacity_lb_;
          if (tmp2 > 0) {
            costL2 += tmp2;
          }
        }

        if (costL1 < costL2) {
          if (treenodes[n1].status % 2 == 0) {
            treenodes[n1].status += 1;
          }
          if (treenodes[n2].status < 2) {
            treenodes[n2].status += 2;
          }

          // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
          graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
          graph2d_.updateEstUsageH({x1, x2}, y2, net, edgeCost);

          treeedge->route.xFirst = false;
        } else {  // if costL1<costL2
          if (treenodes[n2].status % 2 == 0) {
            treenodes[n2].status += 1;
          }
          if (treenodes[n1].status < 2) {
            treenodes[n1].status += 2;
          }

          // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
          graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
          graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, edgeCost);
          treeedge->route.xFirst = true;
        }

      }  // else L-routing
    } else {  // if non-degraded edge
      sttrees_[netID].edges[i].route.type = RouteType::NoRoute;
    }
  }  // loop i
}

// route all segments with L, firstTime: true, first newrouteLAll, false - not
// first
void FastRouteCore::newrouteLAll(const bool firstTime, const bool viaGuided)
{
  if (firstTime) {
    for (const int netID : net_ids_) {
      newrouteL(netID, RouteType::NoRoute, viaGuided);  // do L-routing
    }
  } else {
    for (const int netID : net_ids_) {
      newrouteL(netID, RouteType::LRoute, viaGuided);
    }
  }
}

void FastRouteCore::newrouteZ_edge(const int netID, const int edgeID)
{
  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

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
      const double tmp = graph2d_.getEstUsageRedV(i, j) - v_capacity_lb_;
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
    const double tmp = graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_;
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
        = graph2d_.getEstUsageRedH(x1 + i - 1, y1) - h_capacity_lb_;
    if (tmp1 > 0) {
      cost_tb_[i] += tmp1;
      cost_tb_test_[i] += HCOST;
    } else {
      cost_tb_test_[i] += tmp1;
    }
    const double tmp2
        = graph2d_.getEstUsageRedH(x1 + i - 1, y2) - h_capacity_lb_;
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

  graph2d_.updateEstUsageH({x1, bestZ}, y1, net, edgeCost);
  graph2d_.updateEstUsageH({bestZ, x2}, y2, net, edgeCost);
  graph2d_.updateEstUsageV(bestZ, {ymin, ymax}, net, edgeCost);
  treeedge->route.HVH = true;
  treeedge->route.Zpoint = bestZ;
}

// Z-route, rip-up the previous route according to the ripuptype
void FastRouteCore::newrouteZ(const int netID, const int threshold)
{
  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

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
      if (newRipupCongestedL(
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
            const double tmp = graph2d_.getEstUsageRedV(i, j) - v_capacity_lb_;
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
          const double tmp = graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_;
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
              = graph2d_.getEstUsageRedH(x1 + i - 1, y1) - h_capacity_lb_;
          if (tmp1 > 0) {
            cost_tb_[i] += tmp1;
            cost_tb_test_[0] += HCOST;
          } else {
            cost_tb_test_[0] += tmp1;
          }
          const double tmp2
              = graph2d_.getEstUsageRedH(x1 + i - 1, y2) - h_capacity_lb_;
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
            const double tmp = graph2d_.getEstUsageRedH(j, i) - h_capacity_lb_;
            if (tmp > 0) {
              cost_h_[i - ymin] += tmp;
            }
          }
        }
        // cost for Left&Right boundary segs (form Z with H-seg)
        if (y1Smaller) {
          for (int j = y1; j < y2; j++) {
            const double tmp = graph2d_.getEstUsageRedV(x2, j) - v_capacity_lb_;
            if (tmp > 0) {
              cost_lr_[0] += tmp;
            }
          }
          for (int i = 1; i < segHeight; i++) {
            cost_lr_[i] = cost_lr_[i - 1];
            const double tmp1
                = graph2d_.getEstUsageRedV(x1, y1 + i - 1) - v_capacity_lb_;
            if (tmp1 > 0) {
              cost_lr_[i] += tmp1;
            }
            const double tmp2
                = graph2d_.getEstUsageRedV(x2, y1 + i - 1) - v_capacity_lb_;
            if (tmp2 > 0) {
              cost_lr_[i] -= tmp2;
            }
          }
        } else {
          for (int j = y2; j < y1; j++) {
            const double tmp = graph2d_.getEstUsageV(x1, j) - v_capacity_lb_;
            if (tmp > 0) {
              cost_lr_[0] += tmp;
            }
          }
          for (int i = 1; i < segHeight; i++) {
            cost_lr_[i] = cost_lr_[i - 1];
            const double tmp1
                = graph2d_.getEstUsageRedV(x2, y2 + i - 1) - v_capacity_lb_;
            if (tmp1 > 0) {
              cost_lr_[i] += tmp1;
            }
            const double tmp2
                = graph2d_.getEstUsageRedV(x1, y2 + i - 1) - v_capacity_lb_;
            if (tmp2 > 0) {
              cost_lr_[i] -= tmp2;
            }
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

          graph2d_.updateEstUsageH({x1, bestZ}, y1, net, edgeCost);
          graph2d_.updateEstUsageH({bestZ, x2}, y2, net, edgeCost);
          graph2d_.updateEstUsageV(bestZ, {ymin, ymax}, net, edgeCost);

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
            graph2d_.updateEstUsageV(x1, {y1, bestZ}, net, edgeCost);
            graph2d_.updateEstUsageV(x2, {bestZ, y2}, net, edgeCost);
            graph2d_.updateEstUsageH({x1, x2}, bestZ, net, edgeCost);

            treeedge->route.HVH = HVH;
            treeedge->route.Zpoint = bestZ;
          } else {
            graph2d_.updateEstUsageV(x2, {y2, bestZ}, net, edgeCost);
            graph2d_.updateEstUsageV(x1, {bestZ, y1}, net, edgeCost);
            graph2d_.updateEstUsageH({x1, x2}, bestZ, net, edgeCost);
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
void FastRouteCore::newrouteZAll(const int threshold)
{
  for (const int netID : net_ids_) {
    newrouteZ(netID, threshold);  // ripup previous route and do Z-routing
  }
}

void FastRouteCore::spiralRoute(const int netID, const int edgeID)
{
  auto& treeedges = sttrees_[netID].edges;
  auto& treenodes = sttrees_[netID].nodes;

  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

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
    graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
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
    graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
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
      const double tmp1 = graph2d_.getEstUsageRedV(x1, j) - v_capacity_lb_;
      if (tmp1 > 0) {
        costL1 += tmp1;
      }
      const double tmp2 = graph2d_.getEstUsageRedV(x2, j) - v_capacity_lb_;
      if (tmp2 > 0) {
        costL2 += tmp2;
      }
    }
    for (int j = x1; j < x2; j++) {
      const double tmp1 = graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_;
      if (tmp1 > 0) {
        costL1 += tmp1;
      }
      const double tmp2 = graph2d_.getEstUsageRedH(j, y1) - h_capacity_lb_;
      if (tmp2 > 0) {
        costL2 += tmp2;
      }
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
      graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
      graph2d_.updateEstUsageH({x1, x2}, y2, net, edgeCost);

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
      graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
      graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, edgeCost);
      treeedge->route.xFirst = true;
    }
  }  // else L-routing
}

void FastRouteCore::spiralRouteAll()
{
  for (const int netID : net_ids_) {
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
        const int pin_idx = sttrees_[netID].node_to_pin_idx[d];
        treenodes[d].botL = nets_[netID]->getPinL()[pin_idx];
        treenodes[d].topL = nets_[netID]->getPinL()[pin_idx];
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

  for (const int netID : net_ids_) {
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
  for (const int netID : net_ids_) {
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

  for (const int netID : net_ids_) {
    auto& treenodes = sttrees_[netID].nodes;

    for (int d = 0; d < sttrees_[netID].num_nodes(); d++) {
      const int na = treenodes[d].stackAlias;

      treenodes[d].status = treenodes[na].status;
    }
  }
}

void FastRouteCore::routeMonotonic(const int netID,
                                   const int edgeID,
                                   multi_array<double, 2>& d1,
                                   multi_array<double, 2>& d2,
                                   const int threshold,
                                   const int enlarge)
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
  const int16_t x1 = treenodes[n1].x;
  const int16_t y1 = treenodes[n1].y;
  const int16_t x2 = treenodes[n2].x;
  const int16_t y2 = treenodes[n2].y;

  FrNet* net = nets_[netID];

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
      size_t index = graph2d_.getUsageRedH(i, j);
      index = std::min(index, h_cost_table_.size() - 1);
      const double tmp = h_cost_table_[index];
      d1[j][i + 1] = d1[j][i] + tmp;
    }
    // update the cost of a column of grids by v-edges
  }

  for (int j = ymin; j < ymax; j++) {
    // update the cost of a column of grids by h-edges
    for (int i = xmin; i <= xmax; i++) {
      size_t index = graph2d_.getUsageRedV(i, j);
      index = std::min(index, h_cost_table_.size() - 1);
      const double tmp = h_cost_table_[index];
      d2[j + 1][i] = d2[j][i] + tmp;
    }
    // update the cost of a column of grids by v-edges
  }

  double best = BIG_INT;
  int16_t bestp1x = 0;
  int16_t bestp1y = 0;
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
            + std::abs(d1[j][x2] - d1[j][i]);  // xfirst for mid point

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
  std::vector<GPoint3D>& grids = treeedge->route.grids;
  grids.resize(x_range_ + y_range_);
  const int8_t edgeCost = nets_[netID]->getEdgeCost();

  if (BL1) {
    if (bestp1x > x1) {
      for (int16_t i = x1; i < bestp1x; i++) {
        grids[cnt] = {i, y1};
        graph2d_.updateUsageH(i, y1, net, edgeCost);
        cnt++;
      }
    } else {
      for (int16_t i = x1; i > bestp1x; i--) {
        grids[cnt] = {i, y1};
        graph2d_.updateUsageH(i - 1, y1, net, edgeCost);
        cnt++;
      }
    }
    if (bestp1y > y1) {
      for (int16_t i = y1; i < bestp1y; i++) {
        grids[cnt] = {bestp1x, i};
        cnt++;
        graph2d_.updateUsageV(bestp1x, i, net, edgeCost);
      }
    } else {
      for (int16_t i = y1; i > bestp1y; i--) {
        grids[cnt] = {bestp1x, i};
        cnt++;
        graph2d_.updateUsageV(bestp1x, i - 1, net, edgeCost);
      }
    }
  } else {
    if (bestp1y > y1) {
      for (int16_t i = y1; i < bestp1y; i++) {
        grids[cnt] = {x1, i};
        cnt++;
        graph2d_.updateUsageV(x1, i, net, edgeCost);
      }
    } else {
      for (int16_t i = y1; i > bestp1y; i--) {
        grids[cnt] = {x1, i};
        cnt++;
        graph2d_.updateUsageV(x1, i - 1, net, edgeCost);
      }
    }
    if (bestp1x > x1) {
      for (int16_t i = x1; i < bestp1x; i++) {
        grids[cnt] = {i, bestp1y};
        graph2d_.updateUsageH(i, bestp1y, net, edgeCost);
        cnt++;
      }
    } else {
      for (int16_t i = x1; i > bestp1x; i--) {
        grids[cnt] = {i, bestp1y};
        graph2d_.updateUsageH(i - 1, bestp1y, net, edgeCost);
        cnt++;
      }
    }
  }

  if (BL2) {
    if (bestp1x < x2) {
      for (int16_t i = bestp1x; i < x2; i++) {
        grids[cnt] = {i, bestp1y};
        graph2d_.updateUsageH(i, bestp1y, net, edgeCost);
        cnt++;
      }
    } else {
      for (int16_t i = bestp1x; i > x2; i--) {
        grids[cnt] = {i, bestp1y};
        graph2d_.updateUsageH(i - 1, bestp1y, net, edgeCost);
        cnt++;
      }
    }

    if (y2 > bestp1y) {
      for (int16_t i = bestp1y; i < y2; i++) {
        grids[cnt] = {x2, i};
        cnt++;
        graph2d_.updateUsageV(x2, i, net, edgeCost);
      }
    } else {
      for (int16_t i = bestp1y; i > y2; i--) {
        grids[cnt] = {x2, i};
        cnt++;
        graph2d_.updateUsageV(x2, i - 1, net, edgeCost);
      }
    }
  } else {
    if (y2 > bestp1y) {
      for (int16_t i = bestp1y; i < y2; i++) {
        grids[cnt] = {bestp1x, i};
        cnt++;
        graph2d_.updateUsageV(bestp1x, i, net, edgeCost);
      }
    } else {
      for (int16_t i = bestp1y; i > y2; i--) {
        grids[cnt] = {bestp1x, i};
        cnt++;
        graph2d_.updateUsageV(bestp1x, i - 1, net, edgeCost);
      }
    }
    if (x2 > bestp1x) {
      for (int16_t i = bestp1x; i < x2; i++) {
        grids[cnt] = {i, y2};
        graph2d_.updateUsageH(i, y2, net, edgeCost);
        cnt++;
      }
    } else {
      for (int16_t i = bestp1x; i > x2; i--) {
        grids[cnt] = {i, y2};
        graph2d_.updateUsageH(i - 1, y2, net, edgeCost);
        cnt++;
      }
    }
  }

  grids[cnt] = {x2, y2};
  cnt++;

  treeedge->route.routelen = cnt - 1;

  grids.resize(cnt);
}

void FastRouteCore::routeMonotonicAll(const int threshold,
                                      const int expand,
                                      const float logis_cof)
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

  for (const int netID : net_ids_) {
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

void FastRouteCore::newrouteLInMaze(const int netID)
{
  const int num_edges = sttrees_[netID].num_edges();
  auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;

  FrNet* net = nets_[netID];
  const int8_t edgeCost = net->getEdgeCost();

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

    const auto [ymin, ymax] = std::minmax(y1, y2);

    treeedge->route.type = RouteType::LRoute;
    if (x1 == x2) {  // V-routing
      graph2d_.updateUsageV(x1, {ymin, ymax}, net, edgeCost);
      treeedge->route.xFirst = false;
    } else if (y1 == y2) {  // H-routing
      graph2d_.updateUsageH({x1, x2}, y1, net, edgeCost);
      treeedge->route.xFirst = true;
    } else {  // L-routing
      int costL1 = 0;
      int costL2 = 0;

      for (int j = ymin; j < ymax; j++) {
        const int tmp1 = graph2d_.getUsageRedV(x1, j) - v_capacity_lb_;
        if (tmp1 > 0) {
          costL1 += tmp1;
        }
        const int tmp2 = graph2d_.getUsageRedV(x2, j) - v_capacity_lb_;
        if (tmp2 > 0) {
          costL2 += tmp2;
        }
      }
      for (int j = x1; j < x2; j++) {
        const int tmp1 = graph2d_.getUsageRedH(j, y2) - h_capacity_lb_;
        if (tmp1 > 0) {
          costL1 += tmp1;
        }
        const int tmp2 = graph2d_.getUsageRedH(j, y1) - h_capacity_lb_;
        if (tmp2 > 0) {
          costL2 += tmp2;
        }
      }

      if (costL1 < costL2) {
        // two parts (x1, y1)-(x1, y2) and (x1, y2)-(x2, y2)
        graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
        graph2d_.updateEstUsageH({x1, x2}, y2, net, edgeCost);

        treeedge->route.xFirst = false;
      } else {
        // two parts (x1, y1)-(x2, y1) and (x2, y1)-(x2, y2)
        graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
        graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, edgeCost);

        treeedge->route.xFirst = true;
      }
    }  // else L-routing
  }  // loop i
}

}  // namespace grt
