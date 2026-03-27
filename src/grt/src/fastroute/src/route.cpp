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
      const auto usageV1 = graph2d_.getEstUsageRedV(seg->x1, i);
      costL1 += std::max(0.0, usageV1 - v_capacity_lb_);
      const auto usageV2 = graph2d_.getEstUsageRedV(seg->x2, i);
      costL2 += std::max(0.0, usageV2 - v_capacity_lb_);
    }
    for (int i = seg->x1; i < seg->x2; i++) {
      const auto usageH1 = graph2d_.getEstUsageRedH(i, seg->y2);
      costL1 += std::max(0.0, usageH1 - h_capacity_lb_);
      const auto usageH2 = graph2d_.getEstUsageRedH(i, seg->y1);
      costL2 += std::max(0.0, usageH2 - h_capacity_lb_);
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
    const auto usageV1 = graph2d_.getEstUsageRedV(seg->x1, i);
    costL1 += std::max(0.0, usageV1 - v_capacity_lb_);
    const auto usageV2 = graph2d_.getEstUsageRedV(seg->x2, i);
    costL2 += std::max(0.0, usageV2 - v_capacity_lb_);
  }
  for (int i = seg->x1; i < seg->x2; i++) {
    const auto usageH1 = graph2d_.getEstUsageRedH(i, seg->y2);
    costL1 += std::max(0.0, usageH1 - h_capacity_lb_);
    const auto usageH2 = graph2d_.getEstUsageRedH(i, seg->y1);
    costL2 += std::max(0.0, usageH2 - h_capacity_lb_);
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

  /**
   * @brief Sets the vertical-connection status bit (bit 0, value 1) for a
   * node if not already set.
   * */
  auto markV = [&](int n) {
    if (treenodes[n].status % 2 == 0) {
      treenodes[n].status += 1;
    }
  };

  /**
   * @brief Sets the horizontal-connection status bit (bit 1, value 2) for a
   * node if not already set.
   * */
  auto markH = [&](int n) {
    if (treenodes[n].status < 2) {
      treenodes[n].status += 2;
    }
  };

  for (int i = 0; i < num_edges; i++) {
    if (sttrees_[netID].edges[i].len > 0) {
      TreeEdge* treeedge = &(treeedges[i]);

      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
      const int x1 = treenodes[n1].x;
      const int y1 = treenodes[n1].y;
      const int x2 = treenodes[n2].x;
      const int y2 = treenodes[n2].y;

      const auto [ymin, ymax] = std::minmax(y1, y2);

      if (ripuptype > RouteType::NoRoute) {
        newRipup(treeedge, x1, y1, x2, y2, netID);
      }

      treeedge->route.type = RouteType::LRoute;
      if (x1 == x2) {  // V-routing
        graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
        treeedge->route.xFirst = false;
        markV(n1);
        markV(n2);
      } else if (y1 == y2) {  // H-routing
        graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
        treeedge->route.xFirst = true;
        markH(n1);
        markH(n2);
      } else {  // L-routing
        double costL1 = 0;
        double costL2 = 0;

        if (viaGuided) {
          if (treenodes[n1].status == 2) {
            costL1 = via_cost_;
          } else if (treenodes[n1].status == 1) {
            costL2 = via_cost_;
          } else if (treenodes[n1].status != 0 && treenodes[n1].status != 3
                     && verbose_) {
            logger_->warn(
                GRT, 179, "Wrong node status {}.", treenodes[n1].status);
          }
          if (treenodes[n2].status == 2) {
            costL2 += via_cost_;
          } else if (treenodes[n2].status == 1) {
            costL1 += via_cost_;
          }
        }

        for (int j = ymin; j < ymax; j++) {
          const auto usageV1 = graph2d_.getEstUsageRedV(x1, j);
          costL1 += std::max(0.0, usageV1 - v_capacity_lb_);
          const auto usageV2 = graph2d_.getEstUsageRedV(x2, j);
          costL2 += std::max(0.0, usageV2 - v_capacity_lb_);
        }
        for (int j = x1; j < x2; j++) {
          const auto usageH1 = graph2d_.getEstUsageRedH(j, y2);
          costL1 += std::max(0.0, usageH1 - h_capacity_lb_);
          const auto usageH2 = graph2d_.getEstUsageRedH(j, y1);
          costL2 += std::max(0.0, usageH2 - h_capacity_lb_);
        }

        if (costL1 < costL2) {
          markV(n1);
          markH(n2);
          // Route: (x1,y1)-(x1,y2) vertical, then (x1,y2)-(x2,y2) horizontal
          graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
          graph2d_.updateEstUsageH({x1, x2}, y2, net, edgeCost);
          treeedge->route.xFirst = false;
        } else {
          markV(n2);
          markH(n1);
          // Route: (x1,y1)-(x2,y1) horizontal, then (x2,y1)-(x2,y2) vertical
          graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
          graph2d_.updateEstUsageV(x2, {ymin, ymax}, net, edgeCost);
          treeedge->route.xFirst = true;
        }
      }  // else L-routing
    } else {
      sttrees_[netID].edges[i].route.type = RouteType::NoRoute;
    }
  }
}

// route all segments with L, firstTime: true, first newrouteLAll, false - not
// first
void FastRouteCore::newrouteLAll(const bool firstTime, const bool viaGuided)
{
  const RouteType ripuptype
      = firstTime ? RouteType::NoRoute : RouteType::LRoute;
  for (const int netID : net_ids_) {
    newrouteL(netID, ripuptype, viaGuided);
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
  newRipup(treeedge, x1, y1, x2, y2, netID);

  treeedge->route.type = RouteType::ZRoute;

  const int segWidth = x2 - x1;
  const auto [ymin, ymax] = std::minmax(y1, y2);
  const int npts = segWidth + 1;

  std::fill(cost_hvh_.begin(), cost_hvh_.begin() + npts, 0.0);
  std::fill(cost_hvh_test_.begin(), cost_hvh_test_.begin() + npts, 0.0);
  std::fill(cost_v_.begin(), cost_v_.begin() + npts, 0.0);
  std::fill(cost_v_test_.begin(), cost_v_test_.begin() + npts, 0.0);
  std::fill(cost_tb_.begin(), cost_tb_.begin() + npts, 0.0);
  std::fill(cost_tb_test_.begin(), cost_tb_test_.begin() + npts, 0.0);

  /**
   * @brief Accumulates the overflow of a congested edge into a primary cost
   * and applies a fixed HCOST penalty to a secondary test cost. When the
   * edge has spare capacity (tmp <= 0), the negative slack is added only to
   * the test cost to bias tie-breaking toward less-loaded edges.
   * */
  auto addCongestionCost = [&](double tmp, double& cost, double& cost_test) {
    if (tmp > 0) {
      cost += tmp;
      cost_test += HCOST;
    } else {
      cost_test += tmp;
    }
  };

  // cost for V-segs
  for (int i = x1; i <= x2; i++) {
    for (int j = ymin; j < ymax; j++) {
      addCongestionCost(graph2d_.getEstUsageRedV(i, j) - v_capacity_lb_,
                        cost_v_[i - x1],
                        cost_v_test_[i - x1]);
    }
  }
  // cost for Top&Bot boundary segs (form Z with V-seg)
  for (int j = x1; j < x2; j++) {
    addCongestionCost(graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_,
                      cost_tb_[0],
                      cost_tb_test_[0]);
  }
  for (int i = 1; i <= segWidth; i++) {
    cost_tb_[i] = cost_tb_[i - 1];
    addCongestionCost(graph2d_.getEstUsageRedH(x1 + i - 1, y1) - h_capacity_lb_,
                      cost_tb_[i],
                      cost_tb_test_[i]);
    // subtract the y2 boundary no longer shared by this Z-column
    const double tmp2
        = graph2d_.getEstUsageRedH(x1 + i - 1, y2) - h_capacity_lb_;
    if (tmp2 > 0) {
      cost_tb_[i] -= tmp2;
      cost_tb_test_[i] -= HCOST;
    } else {
      cost_tb_test_[i] -= tmp2;
    }
  }
  // Find the best Z-point
  double bestcost = BIG_INT;
  double btTEST = BIG_INT;
  int bestZ = 0;
  for (int i = 0; i <= segWidth; i++) {
    cost_hvh_[i] = cost_v_[i] + cost_tb_[i];
    cost_hvh_test_[i] = cost_v_test_[i] + cost_tb_test_[i];
    if (cost_hvh_[i] < bestcost
        || (cost_hvh_[i] == bestcost && cost_hvh_test_[i] < btTEST)) {
      bestcost = cost_hvh_[i];
      btTEST = cost_hvh_test_[i];
      bestZ = i + x1;
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
      if (newRipupCongestedL(
              treeedge, treenodes, x1, y1, x2, y2, num_terminals, netID)) {
        const int n1a = treenodes[n1].stackAlias;
        const int n2a = treenodes[n2].stackAlias;
        const int status1 = treenodes[n1a].status;
        const int status2 = treenodes[n2a].status;

        treeedge->route.type = RouteType::ZRoute;

        const int segWidth = x2 - x1;
        const bool y1Smaller = (y1 < y2);
        const auto [ymin, ymax] = std::minmax(y1, y2);
        const int segHeight = ymax - ymin;

        // Initialize HVH/VHV base costs from status1:
        // status1==1 (vertical only): HVH needs an extra via at n1
        // status1==2 (horizontal only): VHV needs an extra via at n1
        const double hvh_base = (status1 == 1) ? via_cost_ : 0.0;
        const double vhv_base = (status1 == 2) ? via_cost_ : 0.0;
        std::fill(cost_hvh_.begin(), cost_hvh_.begin() + segWidth, hvh_base);
        std::fill(cost_hvh_test_.begin(),
                  cost_hvh_test_.begin() + segWidth,
                  hvh_base);
        std::fill(cost_vhv_.begin(), cost_vhv_.begin() + segHeight, vhv_base);

        // status2 adds a via penalty to the opposite shape at n2
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

        std::fill(cost_v_.begin(), cost_v_.begin() + segWidth, 0.0);
        std::fill(cost_tb_.begin(), cost_tb_.begin() + segWidth, 0.0);
        std::fill(cost_v_test_.begin(), cost_v_test_.begin() + segWidth, 0.0);
        std::fill(cost_tb_test_.begin(), cost_tb_test_.begin() + segWidth, 0.0);
        std::fill(cost_h_.begin(), cost_h_.begin() + segHeight, 0.0);
        std::fill(cost_lr_.begin(), cost_lr_.begin() + segHeight, 0.0);

        /**
         * @brief Accumulates the overflow of a congested edge into a primary
         * cost and applies a fixed HCOST penalty to a secondary test cost.
         * When the edge has spare capacity (tmp <= 0), the negative slack is
         * added only to the test cost to bias tie-breaking toward less-loaded
         * edges.
         * */
        auto addCongestionCost
            = [&](double tmp, double& cost, double& cost_test) {
                if (tmp > 0) {
                  cost += tmp;
                  cost_test += HCOST;
                } else {
                  cost_test += tmp;
                }
              };

        // cost for V-segs
        for (int i = x1; i < x2; i++) {
          for (int j = ymin; j < ymax; j++) {
            const double tmp = graph2d_.getEstUsageRedV(i, j) - v_capacity_lb_;
            addCongestionCost(tmp, cost_v_[i - x1], cost_v_test_[i - x1]);
          }
        }
        // cost for Top&Bot boundary segs (form Z with V-seg)
        for (int j = x1; j < x2; j++) {
          const double tmp = graph2d_.getEstUsageRedH(j, y2) - h_capacity_lb_;
          addCongestionCost(tmp, cost_tb_[0], cost_tb_test_[0]);
        }
        for (int i = 1; i < segWidth; i++) {
          cost_tb_[i] = cost_tb_[i - 1];
          const double tmp1
              = graph2d_.getEstUsageRedH(x1 + i - 1, y1) - h_capacity_lb_;
          addCongestionCost(tmp1, cost_tb_[i], cost_tb_test_[0]);
          // subtract the y2 boundary no longer shared by this Z-column
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
            const auto usageH = graph2d_.getEstUsageRedH(j, i);
            cost_h_[i - ymin] += std::max(0.0, usageH - h_capacity_lb_);
          }
        }
        // cost for Left&Right boundary segs (form Z with H-seg).
        // Note: the y1Smaller=false case uses getEstUsageV (not Reduced) for
        // the initial far-side accumulation.
        if (y1Smaller) {
          for (int j = ymin; j < ymax; j++) {
            const auto usageV = graph2d_.getEstUsageRedV(x2, j);
            cost_lr_[0] += std::max(0.0, usageV - v_capacity_lb_);
          }
        } else {
          for (int j = ymin; j < ymax; j++) {
            const auto usageV = graph2d_.getEstUsageV(x1, j);
            cost_lr_[0] += std::max(0.0, usageV - v_capacity_lb_);
          }
        }
        const int lr_near = y1Smaller ? x1 : x2;
        const int lr_far = y1Smaller ? x2 : x1;
        for (int i = 1; i < segHeight; i++) {
          cost_lr_[i] = cost_lr_[i - 1];
          const auto usageNear
              = graph2d_.getEstUsageRedV(lr_near, ymin + i - 1);
          cost_lr_[i] += std::max(0.0, usageNear - v_capacity_lb_);
          const auto usageFar = graph2d_.getEstUsageRedV(lr_far, ymin + i - 1);
          cost_lr_[i] -= std::max(0.0, usageFar - v_capacity_lb_);
        }

        // Find the best Z-point across both HVH and VHV shapes
        bool HVH = true;
        double bestcost = BIG_INT;
        double btTEST = BIG_INT;
        int bestZ = 0;
        for (int i = 0; i < segWidth; i++) {
          cost_hvh_[i] += cost_v_[i] + cost_tb_[i];
          if (cost_hvh_[i] < bestcost
              || (cost_hvh_[i] == bestcost && cost_hvh_test_[i] < btTEST)) {
            bestcost = cost_hvh_[i];
            btTEST = cost_hvh_test_[i];
            bestZ = i + x1;
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
        } else {
          if (treenodes[n1a].status % 2 == 0) {
            treenodes[n1a].status += 1;
          }
          if (treenodes[n2a].status % 2 == 0) {
            treenodes[n2a].status += 1;
          }
          treenodes[n1a].lID++;
          treenodes[n2a].lID++;
          if (y1Smaller) {
            graph2d_.updateEstUsageV(x1, {y1, bestZ}, net, edgeCost);
            graph2d_.updateEstUsageV(x2, {bestZ, y2}, net, edgeCost);
          } else {
            graph2d_.updateEstUsageV(x2, {y2, bestZ}, net, edgeCost);
            graph2d_.updateEstUsageV(x1, {bestZ, y1}, net, edgeCost);
          }
          graph2d_.updateEstUsageH({.lo = x1, .hi = x2}, bestZ, net, edgeCost);
        }
        treeedge->route.HVH = HVH;
        treeedge->route.Zpoint = bestZ;
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

  const auto [ymin, ymax] = std::minmax(y1, y2);

  treeedge->route.type = RouteType::LRoute;

  /**
   * @brief Sets the vertical-connection status bit (bit 0, value 1) for a
   * node and its stack alias if not already set, indicating a vertical segment
   * is incident to this location.
   * */
  auto markVertical = [&](int n, int na) {
    if (treenodes[n].status % 2 == 0) {
      treenodes[n].status += 1;
    }
    if (treenodes[na].status % 2 == 0) {
      treenodes[na].status += 1;
    }
  };

  /**
   * @brief Sets the horizontal-connection status bit (bit 1, value 2) for a
   * node and its stack alias if not already set, indicating a horizontal
   * segment is incident to this location.
   * */
  auto markHorizontal = [&](int n, int na) {
    if (treenodes[n].status < 2) {
      treenodes[n].status += 2;
    }
    if (treenodes[na].status < 2) {
      treenodes[na].status += 2;
    }
  };

  if (x1 == x2) {  // V-routing
    graph2d_.updateEstUsageV(x1, {.lo = ymin, .hi = ymax}, net, edgeCost);
    treeedge->route.xFirst = false;
    markVertical(n1, n1a);
    markVertical(n2, n2a);
  } else if (y1 == y2) {  // H-routing
    graph2d_.updateEstUsageH({x1, x2}, y1, net, edgeCost);
    treeedge->route.xFirst = true;
    markHorizontal(n1, n1a);
    markHorizontal(n2, n2a);
  } else {  // L-routing
    double costL1 = 0;
    double costL2 = 0;
    if (treenodes[n1].status == 2) {
      costL1 = via_cost_;
    } else if (treenodes[n1].status == 1) {
      costL2 = via_cost_;
    } else if (treenodes[n1].status != 0 && treenodes[n1].status != 3
               && verbose_) {
      logger_->warn(GRT, 181, "Wrong node status {}.", treenodes[n1].status);
    }
    if (treenodes[n2].status == 2) {
      costL2 += via_cost_;
    } else if (treenodes[n2].status == 1) {
      costL1 += via_cost_;
    }

    for (int j = ymin; j < ymax; j++) {
      const auto usageV1 = graph2d_.getEstUsageRedV(x1, j);
      costL1 += std::max(0.0, usageV1 - v_capacity_lb_);
      const auto usageV2 = graph2d_.getEstUsageRedV(x2, j);
      costL2 += std::max(0.0, usageV2 - v_capacity_lb_);
    }
    for (int j = x1; j < x2; j++) {
      const auto usageH1 = graph2d_.getEstUsageRedH(j, y2);
      costL1 += std::max(0.0, usageH1 - h_capacity_lb_);
      const auto usageH2 = graph2d_.getEstUsageRedH(j, y1);
      costL2 += std::max(0.0, usageH2 - h_capacity_lb_);
    }

    if (costL1 < costL2) {
      // Route: (x1,y1)-(x1,y2) vertical, then (x1,y2)-(x2,y2) horizontal
      markVertical(n1, n1a);
      markHorizontal(n2, n2a);
      treenodes[n2a].hID++;
      treenodes[n1a].lID++;
      graph2d_.updateEstUsageV(x1, {ymin, ymax}, net, edgeCost);
      graph2d_.updateEstUsageH({x1, x2}, y2, net, edgeCost);
      treeedge->route.xFirst = false;
    } else {
      // Route: (x1,y1)-(x2,y1) horizontal, then (x2,y1)-(x2,y2) vertical
      markVertical(n2, n2a);
      markHorizontal(n1, n1a);
      treenodes[n1a].hID++;
      treenodes[n2a].lID++;
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

    /**
     * @brief Resolves the stack alias for tree node `n`, registers `eid`
     * in that alias node's connection list, and increments its connection
     * count. The alias node field of `treeedge` is updated in place.
     * */
    auto registerAlias = [&](int& alias_field, int n, int eid) {
      alias_field = treenodes[n].stackAlias;
      treenodes[alias_field].eID[treenodes[alias_field].conCNT] = eid;
      treenodes[alias_field].conCNT++;
    };

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        registerAlias(treeedge->n1a, treeedge->n1, edgeID);
        registerAlias(treeedge->n2a, treeedge->n2, edgeID);
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

    /**
     * @brief Enqueues all unassigned edges connected to `node_alias`,
     * marking each as assigned when pushed to prevent duplicate processing.
     * */
    auto enqueueNodeEdges = [&](int node_alias) {
      for (int k = 0; k < treenodes[node_alias].conCNT; k++) {
        const int eid = treenodes[node_alias].eID[k];
        if (!treeedges[eid].assigned) {
          edgeQueue.push(eid);
          treeedges[eid].assigned = true;
        }
      }
    };

    for (int nodeID = 0; nodeID < sttrees_[netID].num_terminals; nodeID++) {
      treenodes[nodeID].assigned = true;
      enqueueNodeEdges(nodeID);
    }

    while (!edgeQueue.empty()) {
      const int edgeID = edgeQueue.front();
      edgeQueue.pop();
      TreeEdge* treeedge = &(treeedges[edgeID]);

      spiralRoute(netID, edgeID);
      treeedge->assigned = true;

      // Expand the unassigned endpoint: if n1a is already assigned the edge
      // was seeded from that side, so expand n2a next, and vice versa.
      const int next_alias
          = treenodes[treeedge->n1a].assigned ? treeedge->n2a : treeedge->n1a;
      if (!treenodes[next_alias].assigned) {
        enqueueNodeEdges(next_alias);
        treenodes[next_alias].assigned = true;
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

  const auto [yminorig, ymaxorig] = std::minmax(y1, y2);
  int ymin = std::max(yminorig - enlarge, 0);
  int ymax = std::min(y_grid_ - 1, ymaxorig + enlarge);

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

  // d1[j][i]: cumulative H-edge cost along row j from xmin to i
  for (int j = ymin; j <= ymax; j++) {
    for (int i = xmin; i < xmax; i++) {
      const size_t idx
          = std::min(static_cast<size_t>(graph2d_.getUsageRedH(i, j)),
                     h_cost_table_.size() - 1);
      d1[j][i + 1] = d1[j][i] + h_cost_table_[idx];
    }
  }

  // d2[j][i]: cumulative V-edge cost along column i from ymin to j
  for (int j = ymin; j < ymax; j++) {
    for (int i = xmin; i <= xmax; i++) {
      const size_t idx
          = std::min(static_cast<size_t>(graph2d_.getUsageRedV(i, j)),
                     h_cost_table_.size() - 1);
      d2[j + 1][i] = d2[j][i] + h_cost_table_[idx];
    }
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

  /**
   * @brief Appends grid points while walking horizontally from from_x to to_x
   * at fixed row y, updating the H-edge usage for each traversed edge.
   * The H-edge between column i and i+1 is indexed at i (left endpoint).
   * */
  auto walkH = [&](int16_t from_x, int16_t to_x, int16_t y) {
    const int step = (to_x >= from_x) ? 1 : -1;
    for (int16_t i = from_x; i != to_x; i += step) {
      grids[cnt++] = {.x = i, .y = y};
      graph2d_.updateUsageH(step > 0 ? i : i - 1, y, net, edgeCost);
    }
  };

  /**
   * @brief Appends grid points while walking vertically from from_y to to_y
   * at fixed column x, updating the V-edge usage for each traversed edge.
   * The V-edge between row j and j+1 is indexed at j (bottom endpoint).
   * */
  auto walkV = [&](int16_t x, int16_t from_y, int16_t to_y) {
    const int step = (to_y >= from_y) ? 1 : -1;
    for (int16_t i = from_y; i != to_y; i += step) {
      grids[cnt++] = {.x = x, .y = i};
      graph2d_.updateUsageV(x, step > 0 ? i : i - 1, net, edgeCost);
    }
  };

  /**
   * @brief Routes one L-shaped segment from (from_x, from_y) to (to_x, to_y).
   * h_first=true routes horizontally then vertically (bend at (to_x, from_y));
   * h_first=false routes vertically then horizontally (bend at (from_x, to_y)).
   * */
  auto walkSegment = [&](bool h_first,
                         int16_t from_x,
                         int16_t from_y,
                         int16_t to_x,
                         int16_t to_y) {
    if (h_first) {
      walkH(from_x, to_x, from_y);
      walkV(to_x, from_y, to_y);
    } else {
      walkV(from_x, from_y, to_y);
      walkH(from_x, to_x, to_y);
    }
  };

  walkSegment(BL1, x1, y1, bestp1x, bestp1y);
  walkSegment(BL2, bestp1x, bestp1y, x2, y2);

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
      float costL1 = 0;
      float costL2 = 0;

      for (int j = ymin; j < ymax; j++) {
        const auto usageV1 = graph2d_.getUsageRedV(x1, j);
        costL1 += std::max(0.0f, usageV1 - v_capacity_lb_);
        const auto usageV2 = graph2d_.getUsageRedV(x2, j);
        costL2 += std::max(0.0f, usageV2 - v_capacity_lb_);
      }
      for (int j = x1; j < x2; j++) {
        const auto usageH1 = graph2d_.getUsageRedH(j, y2);
        costL1 += std::max(0.0f, usageH1 - h_capacity_lb_);
        const auto usageH2 = graph2d_.getUsageRedH(j, y1);
        costL2 += std::max(0.0f, usageH2 - h_capacity_lb_);
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
