// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataType.h"
#include "FastRoute.h"
#include "grt/GRoute.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

static int parent_index(int i)
{
  return (i - 1) / 2;
}

static int left_index(int i)
{
  return 2 * i + 1;
}

static int right_index(int i)
{
  return 2 * i + 2;
}

void FastRouteCore::checkAndFixEmbeddedTree(const int net_id)
{
  const auto& treeedges = sttrees_[net_id].edges;
  const int num_edges = sttrees_[net_id].num_edges();

  // group all edges that crosses the same position
  std::unordered_map<std::pair<int16_t, int16_t>,
                     std::vector<int>,
                     boost::hash<std::pair<int, int>>>
      position_to_edges_map;

  auto cmp = [&](int a, int b) { return treeedges[a].len > treeedges[b].len; };
  std::map<int, std::vector<std::pair<int16_t, int16_t>>, decltype(cmp)>
      edges_to_blocked_pos_map(cmp);
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      int routeLen = treeedge->route.routelen;
      const std::vector<GPoint3D>& grids = treeedge->route.grids;

      for (int i = 0; i <= routeLen; i++) {
        const std::pair<int16_t, int16_t> pos_i = {grids[i].x, grids[i].y};
        position_to_edges_map[pos_i].push_back(edgeID);
      }
    }
  }

  // for each position, check if there is an edge that don't share
  // a node with the remaining edges
  for (auto const& [position, edges] : position_to_edges_map) {
    for (int edgeID : edges) {
      if (areEdgesOverlapping(net_id, edgeID, edges)) {
        edges_to_blocked_pos_map[edgeID].emplace_back(position.first,
                                                      position.second);
      }
    }
  }

  // fix the longest edge and break the loop
  if (!edges_to_blocked_pos_map.empty()) {
    int edge = edges_to_blocked_pos_map.begin()->first;
    std::vector<std::pair<int16_t, int16_t>>& blocked_positions
        = edges_to_blocked_pos_map.begin()->second;
    fixOverlappingEdge(net_id, edge, blocked_positions);
  }
}

bool FastRouteCore::areEdgesOverlapping(const int net_id,
                                        const int edge_id,
                                        const std::vector<int>& edges)
{
  if (edges.size() == 1) {
    return false;
  }

  auto& treeedges = sttrees_[net_id].edges;
  TreeEdge* treeedge = &(treeedges[edge_id]);

  int n1 = treeedge->n1;
  int n2 = treeedge->n2;
  int n1a = treeedge->n1a;
  int n2a = treeedge->n2a;

  for (int ed : edges) {
    if (ed != edge_id) {
      int ed_n1 = treeedges[ed].n1;
      int ed_n2 = treeedges[ed].n2;
      int ed_n1a = treeedges[ed].n1a;
      int ed_n2a = treeedges[ed].n2a;
      if ((ed_n1a == n1a) || ((ed_n1a == n2a)) || (ed_n1 == n1)
          || (ed_n1 == n2)) {
        return false;
      }
      if ((ed_n2a == n1a) || ((ed_n2a == n2a)) || (ed_n2 == n1)
          || ((ed_n2 == n2))) {
        return false;
      }
    }
  }

  // if the edge doesn't share any node with the other edges, it is overlapping
  return true;
}

void FastRouteCore::fixOverlappingEdge(
    const int net_id,
    const int edge,
    std::vector<std::pair<int16_t, int16_t>>& blocked_positions)
{
  TreeEdge* treeedge = &(sttrees_[net_id].edges[edge]);
  auto& treenodes = sttrees_[net_id].nodes;

  auto sort_by_x
      = [](std::pair<int16_t, int16_t> a, std::pair<int16_t, int16_t> b) {
          return a.first < b.first;
        };

  std::ranges::stable_sort(blocked_positions, sort_by_x);

  if (treeedge->len > 0) {
    std::vector<GPoint3D> new_route;
    const TreeNode& startpoint = treenodes[treeedge->n1];
    const TreeNode& endpoint = treenodes[treeedge->n2];
    if (startpoint.x == endpoint.x || startpoint.y == endpoint.y) {
      return;
    }
    routeLShape(startpoint, endpoint, blocked_positions, new_route);

    // Updates the usage of the altered edge
    FrNet* net = nets_[net_id];
    const int8_t edgeCost = net->getEdgeCost();
    for (int k = 0; k < treeedge->route.routelen;
         k++) {  // remove the usages of the old edges
      if (treeedge->route.grids[k].x == treeedge->route.grids[k + 1].x) {
        if (treeedge->route.grids[k].y != treeedge->route.grids[k + 1].y) {
          const int min_y = std::min(treeedge->route.grids[k].y,
                                     treeedge->route.grids[k + 1].y);
          graph2d_.updateUsageV(
              treeedge->route.grids[k].x, min_y, net, -edgeCost);
        }
      } else {
        const int min_x = std::min(treeedge->route.grids[k].x,
                                   treeedge->route.grids[k + 1].x);
        graph2d_.updateUsageH(
            min_x, treeedge->route.grids[k].y, net, -edgeCost);
      }
    }
    for (int k = 0; k < new_route.size() - 1;
         k++) {  // update the usage for the new edges
      if (new_route[k].x == new_route[k + 1].x) {
        if (new_route[k].y != new_route[k + 1].y) {
          const int min_y = std::min(new_route[k].y, new_route[k + 1].y);
          graph2d_.updateUsageV(new_route[k].x, min_y, net, edgeCost);
        }
      } else {
        const int min_x = std::min(new_route[k].x, new_route[k + 1].x);
        graph2d_.updateUsageH(min_x, new_route[k].y, net, edgeCost);
      }
    }
    treeedge->route.routelen = new_route.size() - 1;
    treeedge->route.grids = std::move(new_route);
  }
}

void FastRouteCore::routeLShape(
    const TreeNode& startpoint,
    const TreeNode& endpoint,
    std::vector<std::pair<int16_t, int16_t>>& blocked_positions,
    std::vector<GPoint3D>& new_route)
{
  new_route.push_back({startpoint.x, startpoint.y, -1});
  int16_t first_x;
  int16_t first_y;
  if (blocked_positions.front().second == blocked_positions.back().second) {
    // blocked positions are horizontally aligned
    int16_t y;
    // using first_x variable to avoid duplicated points
    if (startpoint.y != blocked_positions[0].second) {
      y = startpoint.y;
      // point (startpoint.x, startpoint.y) already
      // added, so can be skiped in the for below
      first_x = startpoint.x + 1;
    } else {
      y = endpoint.y;
      // point (startpoint.x, endpoint.y) not added yet, so it
      // need to be included in the for below
      first_x = startpoint.x;
      if (startpoint.y == endpoint.y) {
        first_x++;
      }
    }
    for (int16_t x = first_x; x <= endpoint.x; x++) {
      new_route.push_back({x, y, -1});
    }
    if (y < endpoint.y) {
      while (y < endpoint.y) {
        new_route.push_back(
            {new_route.back().x, static_cast<int16_t>(y + 1), -1});
        y++;
      }
    } else {
      while (y > endpoint.y) {
        new_route.push_back(
            {new_route.back().x, static_cast<int16_t>(y - 1), -1});
        y--;
      }
    }
  } else {
    // blocked positions are vertically aligned
    int16_t x;
    if (startpoint.x != blocked_positions[0].first) {
      x = startpoint.x;
      // point (startpoint.x, startpoint.y) already
      // added, so can be skiped in the for below
      first_y
          = (startpoint.y < endpoint.y) ? startpoint.y + 1 : startpoint.y - 1;
    } else {
      x = endpoint.x;
      // point (endpoint.x, startpoint.y) not added yet, so it
      // need to be included in the for below
      first_y = startpoint.y;
      if (startpoint.x == endpoint.x) {
        first_y
            = (startpoint.y < endpoint.y) ? startpoint.y + 1 : startpoint.y - 1;
      }
    }
    if (startpoint.y < endpoint.y) {
      for (int16_t y = first_y; y <= endpoint.y; y++) {
        new_route.push_back({x, y, -1});
      }
    } else {
      for (int16_t y = first_y; y >= endpoint.y; y--) {
        new_route.push_back({x, y, -1});
      }
    }
    while (x < endpoint.x) {
      new_route.push_back(
          {static_cast<int16_t>(x + 1), new_route.back().y, -1});
      x++;
    }
  }
}

void FastRouteCore::convertToMazerouteNet(const int netID)
{
  auto& treenodes = sttrees_[netID].nodes;
  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    treeedge->convertToMazerouteNet(treenodes[n1], treenodes[n2]);
  }
}

void TreeEdge::convertToMazerouteNet(const TreeNode& p1, const TreeNode& p2)
{
  const int edgelength = len;
  const int x1 = p1.x;
  const int y1 = p1.y;
  const int x2 = p2.x;
  const int y2 = p2.y;
  route.grids.resize(edgelength + 1);
  std::vector<GPoint3D>& grids = route.grids;
  len = abs(x1 - x2) + abs(y1 - y2);

  int cnt = 0;
  if (route.type == RouteType::NoRoute) {
    grids[0].x = x1;
    grids[0].y = y1;
    route.type = RouteType::MazeRoute;
    route.routelen = 0;
    len = 0;
    cnt++;
  } else if (route.type == RouteType::LRoute) {
    if (route.xFirst) {  // horizontal first
      for (int i = x1; i <= x2; i++) {
        grids[cnt].x = i;
        grids[cnt].y = y1;
        cnt++;
      }
      if (y1 <= y2) {
        for (int i = y1 + 1; i <= y2; i++) {
          grids[cnt].x = x2;
          grids[cnt].y = i;
          cnt++;
        }
      } else {
        for (int i = y1 - 1; i >= y2; i--) {
          grids[cnt].x = x2;
          grids[cnt].y = i;
          cnt++;
        }
      }
    } else {  // vertical first
      if (y1 <= y2) {
        for (int i = y1; i <= y2; i++) {
          grids[cnt].x = x1;
          grids[cnt].y = i;
          cnt++;
        }
      } else {
        for (int i = y1; i >= y2; i--) {
          grids[cnt].x = x1;
          grids[cnt].y = i;
          cnt++;
        }
      }
      for (int i = x1 + 1; i <= x2; i++) {
        grids[cnt].x = i;
        grids[cnt].y = y2;
        cnt++;
      }
    }
  } else if (route.type == RouteType::ZRoute) {
    const int Zpoint = route.Zpoint;
    if (route.HVH)  // HVH
    {
      for (int i = x1; i < Zpoint; i++) {
        grids[cnt].x = i;
        grids[cnt].y = y1;
        cnt++;
      }
      if (y1 <= y2) {
        for (int i = y1; i <= y2; i++) {
          grids[cnt].x = Zpoint;
          grids[cnt].y = i;
          cnt++;
        }
      } else {
        for (int i = y1; i >= y2; i--) {
          grids[cnt].x = Zpoint;
          grids[cnt].y = i;
          cnt++;
        }
      }
      for (int i = Zpoint + 1; i <= x2; i++) {
        grids[cnt].x = i;
        grids[cnt].y = y2;
        cnt++;
      }
    } else {  // VHV
      if (y1 <= y2) {
        for (int i = y1; i < Zpoint; i++) {
          grids[cnt].x = x1;
          grids[cnt].y = i;
          cnt++;
        }
        for (int i = x1; i <= x2; i++) {
          grids[cnt].x = i;
          grids[cnt].y = Zpoint;
          cnt++;
        }
        for (int i = Zpoint + 1; i <= y2; i++) {
          grids[cnt].x = x2;
          grids[cnt].y = i;
          cnt++;
        }
      } else {
        for (int i = y1; i > Zpoint; i--) {
          grids[cnt].x = x1;
          grids[cnt].y = i;
          cnt++;
        }
        for (int i = x1; i <= x2; i++) {
          grids[cnt].x = i;
          grids[cnt].y = Zpoint;
          cnt++;
        }
        for (int i = Zpoint - 1; i >= y2; i--) {
          grids[cnt].x = x2;
          grids[cnt].y = i;
          cnt++;
        }
      }
    }
  }

  route.type = RouteType::MazeRoute;
  route.routelen = edgelength;
}

void FastRouteCore::convertToMazeroute()
{
  for (const int& netID : net_ids_) {
    convertToMazerouteNet(netID);
  }

  graph2d_.addEstUsageToUsage();

  // check 2D edges for invalid usage values
  check2DEdgesUsage();
}

// non recursive version of heapify
static void heapify(std::vector<double*>& array)
{
  bool stop = false;
  const int heapSize = array.size();
  int i = 0;

  double* tmp = array[i];
  do {
    const int l = left_index(i);
    const int r = right_index(i);

    int smallest;
    if (l < heapSize && *(array[l]) < *tmp) {
      smallest = l;
      if (r < heapSize && *(array[r]) < *(array[l])) {
        smallest = r;
      }
    } else {
      smallest = i;
      if (r < heapSize && *(array[r]) < *tmp) {
        smallest = r;
      }
    }
    if (smallest != i) {
      array[i] = array[smallest];
      i = smallest;
    } else {
      array[i] = tmp;
      stop = true;
    }
  } while (!stop);
}

static void updateHeap(std::vector<double*>& array, int i)
{
  double* tmpi = array[i];
  while (i > 0 && *(array[parent_index(i)]) > *tmpi) {
    const int parent = parent_index(i);
    array[i] = array[parent];
    i = parent;
  }
  array[i] = tmpi;
}

// remove the entry with minimum distance from Priority queue
static void removeMin(std::vector<double*>& array)
{
  array[0] = array.back();
  heapify(array);
  array.pop_back();
}

// ripup a tree edge according to its ripup type and Z-route it
// put all the nodes in the subtree t1 and t2 into src_heap and dest_heap
// netID     - the ID for the net
// edgeID    - the ID for the tree edge to route
// d1        - the distance of any grid from the source subtree t1
// d2        - the distance of any grid from the destination subtree t2
// src_heap  - the heap storing the addresses for d1
// dest_heap - the heap storing the addresses for d2
void FastRouteCore::setupHeap(const int netID,
                              const int edgeID,
                              std::vector<double*>& src_heap,
                              std::vector<double*>& dest_heap,
                              multi_array<double, 2>& d1,
                              multi_array<double, 2>& d2,
                              const int regionX1,
                              const int regionX2,
                              const int regionY1,
                              const int regionY2)
{
  for (int i = regionY1; i <= regionY2; i++) {
    for (int j = regionX1; j <= regionX2; j++) {
      in_region_[i][j] = true;
    }
  }

  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;
  const int num_terminals = sttrees_[netID].num_terminals;

  const int n1 = treeedges[edgeID].n1;
  const int n2 = treeedges[edgeID].n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  src_heap.clear();
  dest_heap.clear();

  if (num_terminals == 2)  // 2-pin net
  {
    d1[y1][x1] = 0;
    src_heap.push_back(&d1[y1][x1]);
    d2[y2][x2] = 0;
    dest_heap.push_back(&d2[y2][x2]);
  } else {  // net with more than 2 pins
    const int numNodes = sttrees_[netID].num_nodes();

    std::vector<bool> visited(numNodes, false);
    std::vector<int> queue(numNodes);

    // find all the grids on tree edges in subtree t1 (connecting to n1) and put
    // them into src_heap
    int queuehead = 0;
    int queuetail = 0;

    // add n1 into src_heap
    d1[y1][x1] = 0;
    src_heap.push_back(&d1[y1][x1]);
    visited[n1] = true;

    // add n1 into the queue
    queue[queuetail] = n1;
    queuetail++;

    // loop to find all the edges in subtree t1
    while (queuetail > queuehead) {
      // get cur node from the queuehead
      const int cur = queue[queuehead];
      queuehead++;
      visited[cur] = true;

      const int nbrcnt = treenodes[cur].nbr_count;
      for (int i = 0; i < nbrcnt; i++) {
        const int nbr = treenodes[cur].nbr[i];
        const int edge = treenodes[cur].edge[i];

        if (nbr == n2) {
          continue;
        }

        if (visited[nbr]) {
          continue;
        }

        // put all the grids on the two adjacent tree edges into src_heap
        if (treeedges[edge].route.routelen > 0) {  // not a degraded edge
          // put nbr into src_heap if in enlarged region
          const TreeNode& nbr_node = treenodes[nbr];
          if (in_region_[nbr_node.y][nbr_node.x]) {
            const int nbrX = nbr_node.x;
            const int nbrY = nbr_node.y;
            d1[nbrY][nbrX] = 0;
            src_heap.push_back(&d1[nbrY][nbrX]);
            corr_edge_[nbrY][nbrX] = edge;
          }
          const Route* route = &(treeedges[edge].route);
          if (route->type != RouteType::MazeRoute) {
            logger_->error(GRT, 125, "Setup heap: not maze routing.");
          }

          // don't put edge_n1 and edge_n2 into src_heap
          for (int j = 1; j < route->routelen; j++) {
            const int x_grid = route->grids[j].x;
            const int y_grid = route->grids[j].y;

            if (in_region_[y_grid][x_grid]) {
              d1[y_grid][x_grid] = 0;
              src_heap.push_back(&d1[y_grid][x_grid]);
              corr_edge_[y_grid][x_grid] = edge;
            }
          }
        }  // if not a degraded edge (len>0)

        // add the neighbor of cur node into queue
        queue[queuetail] = nbr;
        queuetail++;
      }  // loop i (3 neighbors for cur node)
    }  // while queue is not empty

    // find all the grids on subtree t2 (connect to n2) and put them into
    // dest_heap find all the grids on tree edges in subtree t2 (connecting to
    // n2) and put them into dest_heap
    queuehead = 0;
    queuetail = 0;

    // add n2 into dest_heap
    d2[y2][x2] = 0;
    dest_heap.push_back(&d2[y2][x2]);
    visited[n2] = true;

    // add n2 into the queue
    queue[queuetail] = n2;
    queuetail++;

    // loop to find all the edges in subtree t2
    while (queuetail > queuehead) {
      // get cur node form queuehead
      const int cur = queue[queuehead];
      visited[cur] = true;
      queuehead++;

      const int nbrcnt = treenodes[cur].nbr_count;
      for (int i = 0; i < nbrcnt; i++) {
        const int nbr = treenodes[cur].nbr[i];
        const int edge = treenodes[cur].edge[i];

        if (nbr == n1) {
          continue;
        }

        if (visited[nbr]) {
          continue;
        }

        // put all the grids on the two adjacent tree edges into dest_heap
        if (treeedges[edge].route.routelen > 0) {  // not a degraded edge
          // put nbr into dest_heap
          const TreeNode& nbr_node = treenodes[nbr];
          if (in_region_[nbr_node.y][nbr_node.x]) {
            const int nbrX = nbr_node.x;
            const int nbrY = nbr_node.y;
            d2[nbrY][nbrX] = 0;
            dest_heap.push_back(&d2[nbrY][nbrX]);
            corr_edge_[nbrY][nbrX] = edge;
          }

          const Route* route = &(treeedges[edge].route);
          if (route->type != RouteType::MazeRoute) {
            logger_->error(GRT, 201, "Setup heap: not maze routing.");
          }

          // don't put edge_n1 and edge_n2 into dest_heap
          for (int j = 1; j < route->routelen; j++) {
            const int x_grid = route->grids[j].x;
            const int y_grid = route->grids[j].y;
            if (in_region_[y_grid][x_grid]) {
              d2[y_grid][x_grid] = 0;
              dest_heap.push_back(&d2[y_grid][x_grid]);
              corr_edge_[y_grid][x_grid] = edge;
            }
          }
        }  // if the edge is not degraded (len>0)

        // add the neighbor of cur node into queue
        queue[queuetail] = nbr;
        queuetail++;
      }  // loop i (3 neigbors for cur node)
    }  // while queue is not empty
  }  // net with more than two pins

  for (int i = regionY1; i <= regionY2; i++) {
    for (int j = regionX1; j <= regionX2; j++) {
      in_region_[i][j] = false;
    }
  }
}

int FastRouteCore::copyGrids(const std::vector<TreeNode>& treenodes,
                             const int n1,
                             const int n2,
                             const std::vector<TreeEdge>& treeedges,
                             const int edge_n1n2,
                             std::vector<int>& gridsX_n1n2,
                             std::vector<int>& gridsY_n1n2)
{
  const int n1x = treenodes[n1].x;
  const int n1y = treenodes[n1].y;

  int cnt = 0;
  if (treeedges[edge_n1n2].n1 == n1)  // n1 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
      gridsX_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      gridsY_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      for (int i = 0; i <= treeedges[edge_n1n2].route.routelen; i++) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.grids[i].x;
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.grids[i].y;
        cnt++;
      }
    }  // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2.resize(1);
      gridsY_n1n2.resize(1);
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      cnt++;
    }
  }  // if n1 is the first node of (n1, n2)
  else  // n2 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
      gridsX_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      gridsY_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      for (int i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.grids[i].x;
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.grids[i].y;
        cnt++;
      }
    }  // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2.resize(1);
      gridsY_n1n2.resize(1);
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      cnt++;
    }  // MazeRoute
  }

  return cnt;
}

bool FastRouteCore::updateRouteType1(const int net_id,
                                     const std::vector<TreeNode>& treenodes,
                                     const int n1,
                                     const int A1,
                                     const int A2,
                                     const int E1x,
                                     const int E1y,
                                     std::vector<TreeEdge>& treeedges,
                                     const int edge_n1A1,
                                     const int edge_n1A2)
{
  std::vector<int> gridsX_n1A1;
  std::vector<int> gridsY_n1A1;
  std::vector<int> gridsX_n1A2;
  std::vector<int> gridsY_n1A2;

  const int A1x = treenodes[A1].x;
  const int A1y = treenodes[A1].y;
  const int A2x = treenodes[A2].x;
  const int A2y = treenodes[A2].y;

  // copy all the grids on (n1, A1) and (n2, A2) to tmp arrays, and keep the
  // grids order A1->n1->A2 copy (n1, A1)
  const int cnt_n1A1 = copyGrids(
      treenodes, A1, n1, treeedges, edge_n1A1, gridsX_n1A1, gridsY_n1A1);

  // copy (n1, A2)
  const int cnt_n1A2 = copyGrids(
      treenodes, n1, A2, treeedges, edge_n1A2, gridsX_n1A2, gridsY_n1A2);

  // update route for (n1, A1) and (n1, A2)
  // find the index of E1 in (n1, A1)
  int E1_pos = -1;
  for (int i = 0; i < cnt_n1A1; i++) {
    // reach the E1
    if (gridsX_n1A1[i] == E1x && gridsY_n1A1[i] == E1y) {
      E1_pos = i;
      break;
    }
  }

  if (E1_pos == -1) {
    int x_pos = tile_size_ * (E1x + 0.5) + x_corner_;
    int y_pos = tile_size_ * (E1y + 0.5) + y_corner_;
    if (verbose_) {
      logger_->error(
          GRT,
          169,
          "Net {}: Invalid index for position ({}, {}). Net degree: {}.",
          nets_[net_id]->getName(),
          x_pos,
          y_pos,
          nets_[net_id]->getNumPins());
    }
    return false;
  }

  // reallocate memory for route.gridsX and route.gridsY
  if (treeedges[edge_n1A1].route.type
      == RouteType::MazeRoute)  // if originally allocated, free them first
  {
    treeedges[edge_n1A1].route.grids.clear();
  }
  treeedges[edge_n1A1].route.grids.resize(E1_pos + 1);

  if (A1x <= E1x) {
    int cnt = 0;
    for (int i = 0; i <= E1_pos; i++) {
      treeedges[edge_n1A1].route.grids[cnt].x = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.grids[cnt].y = gridsY_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = A1;
    treeedges[edge_n1A1].n2 = n1;
  } else {
    int cnt = 0;
    for (int i = E1_pos; i >= 0; i--) {
      treeedges[edge_n1A1].route.grids[cnt].x = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.grids[cnt].y = gridsY_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = n1;
    treeedges[edge_n1A1].n2 = A1;
  }

  treeedges[edge_n1A1].route.type = RouteType::MazeRoute;
  treeedges[edge_n1A1].route.routelen = E1_pos;
  treeedges[edge_n1A1].len = abs(A1x - E1x) + abs(A1y - E1y);

  // reallocate memory for route.gridsX and route.gridsY
  if (treeedges[edge_n1A2].route.type
      == RouteType::MazeRoute)  // if originally allocated, free them first
  {
    treeedges[edge_n1A2].route.grids.clear();
  }
  treeedges[edge_n1A2].route.grids.resize(cnt_n1A1 + cnt_n1A2 - E1_pos - 1);

  int cnt = 0;
  if (E1x <= A2x) {
    for (int i = E1_pos; i < cnt_n1A1; i++) {
      treeedges[edge_n1A2].route.grids[cnt].x = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.grids[cnt].y = gridsY_n1A1[i];
      cnt++;
    }
    for (int i = 1; i < cnt_n1A2; i++)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.grids[cnt].x = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.grids[cnt].y = gridsY_n1A2[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = n1;
    treeedges[edge_n1A2].n2 = A2;
  } else {
    for (int i = cnt_n1A2 - 1; i >= 1; i--)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.grids[cnt].x = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.grids[cnt].y = gridsY_n1A2[i];
      cnt++;
    }
    for (int i = cnt_n1A1 - 1; i >= E1_pos; i--) {
      treeedges[edge_n1A2].route.grids[cnt].x = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.grids[cnt].y = gridsY_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = A2;
    treeedges[edge_n1A2].n2 = n1;
  }
  treeedges[edge_n1A2].route.type = RouteType::MazeRoute;
  treeedges[edge_n1A2].route.routelen = cnt - 1;
  treeedges[edge_n1A2].len = abs(A2x - E1x) + abs(A2y - E1y);

  return true;
}

bool FastRouteCore::updateRouteType2(const int net_id,
                                     const std::vector<TreeNode>& treenodes,
                                     const int n1,
                                     const int A1,
                                     const int A2,
                                     const int C1,
                                     const int C2,
                                     const int E1x,
                                     const int E1y,
                                     std::vector<TreeEdge>& treeedges,
                                     const int edge_n1A1,
                                     const int edge_n1A2,
                                     const int edge_C1C2)
{
  std::vector<int> gridsX_n1A1;
  std::vector<int> gridsY_n1A1;
  std::vector<int> gridsX_n1A2;
  std::vector<int> gridsY_n1A2;
  std::vector<int> gridsX_C1C2;
  std::vector<int> gridsY_C1C2;

  const int A1x = treenodes[A1].x;
  const int A1y = treenodes[A1].y;
  const int A2x = treenodes[A2].x;
  const int A2y = treenodes[A2].y;
  const int C1x = treenodes[C1].x;
  const int C1y = treenodes[C1].y;
  const int C2x = treenodes[C2].x;
  const int C2y = treenodes[C2].y;

  const int edge_n1C1 = edge_n1A1;
  const int edge_n1C2 = edge_n1A2;
  const int edge_A1A2 = edge_C1C2;

  // combine (n1, A1) and (n1, A2) into (A1, A2), A1 is the first node and A2 is
  // the second grids order A1->n1->A2 copy (A1, n1)
  const int cnt_n1A1 = copyGrids(
      treenodes, A1, n1, treeedges, edge_n1A1, gridsX_n1A1, gridsY_n1A1);

  // copy (n1, A2)
  const int cnt_n1A2 = copyGrids(
      treenodes, n1, A2, treeedges, edge_n1A2, gridsX_n1A2, gridsY_n1A2);

  // copy all the grids on (C1, C2) to gridsX_C1C2[] and gridsY_C1C2[]
  const int cnt_C1C2 = copyGrids(
      treenodes, C1, C2, treeedges, edge_C1C2, gridsX_C1C2, gridsY_C1C2);

  // combine grids on original (A1, n1) and (n1, A2) to new (A1, A2)
  // allocate memory for grids[].x and grids[].y of edge_A1A2
  if (treeedges[edge_A1A2].route.type == RouteType::MazeRoute) {
    treeedges[edge_A1A2].route.grids.clear();
  }
  const int len_A1A2 = cnt_n1A1 + cnt_n1A2 - 1;

  treeedges[edge_A1A2].route.grids.resize(len_A1A2);
  treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
  treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);

  int cnt = 0;
  for (int i = 0; i < cnt_n1A1; i++) {
    treeedges[edge_A1A2].route.grids[cnt].x = gridsX_n1A1[i];
    treeedges[edge_A1A2].route.grids[cnt].y = gridsY_n1A1[i];
    cnt++;
  }
  for (int i = 1; i < cnt_n1A2; i++)  // do not repeat point n1
  {
    treeedges[edge_A1A2].route.grids[cnt].x = gridsX_n1A2[i];
    treeedges[edge_A1A2].route.grids[cnt].y = gridsY_n1A2[i];
    cnt++;
  }

  // find the index of E1 in (C1, C2)
  int E1_pos = -1;
  for (int i = 0; i < cnt_C1C2; i++) {
    if (gridsX_C1C2[i] == E1x && gridsY_C1C2[i] == E1y) {
      E1_pos = i;
      break;
    }
  }

  if (E1_pos == -1) {
    int x_pos = tile_size_ * (E1x + 0.5) + x_corner_;
    int y_pos = tile_size_ * (E1y + 0.5) + y_corner_;
    debugPrint(logger_,
               utl::GRT,
               "maze_2d",
               1,
               "Net {}: Invalid index for position ({}, {}). Net degree: {}.",
               nets_[net_id]->getName(),
               x_pos,
               y_pos,
               nets_[net_id]->getNumPins());
    return false;
  }

  // allocate memory for grids[].x and grids[].y of edge_n1C1 and edge_n1C2
  if (treeedges[edge_n1C1].route.type == RouteType::MazeRoute) {
    treeedges[edge_n1C1].route.grids.clear();
  }
  const int len_n1C1 = E1_pos + 1;
  treeedges[edge_n1C1].route.grids.resize(len_n1C1);
  treeedges[edge_n1C1].route.routelen = len_n1C1 - 1;
  treeedges[edge_n1C1].len = abs(C1x - E1x) + abs(C1y - E1y);

  if (treeedges[edge_n1C2].route.type == RouteType::MazeRoute) {
    treeedges[edge_n1C2].route.grids.clear();
  }
  const int len_n1C2 = cnt_C1C2 - E1_pos;
  treeedges[edge_n1C2].route.grids.resize(len_n1C2);
  treeedges[edge_n1C2].route.routelen = len_n1C2 - 1;
  treeedges[edge_n1C2].len = abs(C2x - E1x) + abs(C2y - E1y);

  // split original (C1, C2) to (C1, n1) and (n1, C2)
  cnt = 0;
  for (int i = 0; i <= E1_pos; i++) {
    treeedges[edge_n1C1].route.grids[i].x = gridsX_C1C2[i];
    treeedges[edge_n1C1].route.grids[i].y = gridsY_C1C2[i];
    cnt++;
  }

  cnt = 0;
  for (int i = E1_pos; i < cnt_C1C2; i++) {
    treeedges[edge_n1C2].route.grids[cnt].x = gridsX_C1C2[i];
    treeedges[edge_n1C2].route.grids[cnt].y = gridsY_C1C2[i];
    cnt++;
  }
  return true;
}

void FastRouteCore::reInitTree(const int netID)
{
  const int numEdges = sttrees_[netID].num_edges();
  for (int edgeID = 0; edgeID < numEdges; edgeID++) {
    TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
    if (treeedge->len > 0) {
      treeedge->route.grids.clear();
    }
  }
  sttrees_[netID].nodes.clear();
  sttrees_[netID].edges.clear();

  stt::Tree rsmt;
  const float net_alpha = stt_builder_->getAlpha(nets_[netID]->getDbNet());
  // if failing tree was created with pd, fall back to flute with fluteNormal
  // first so the structs necessary for fluteCongest are filled
  if (net_alpha > 0.0) {
    fluteNormal(
        netID, nets_[netID]->getPinX(), nets_[netID]->getPinY(), 2, 1.2, rsmt);
  }

  fluteCongest(
      netID, nets_[netID]->getPinX(), nets_[netID]->getPinY(), 2, 1.2, rsmt);

  if (nets_[netID]->getNumPins() > 3) {
    edgeShiftNew(rsmt, netID);
  }

  copyStTree(netID, rsmt);
  newrouteLInMaze(netID);
  convertToMazerouteNet(netID);
  checkAndFixEmbeddedTree(netID);
}

double FastRouteCore::getCost(const int index,
                              const bool is_horizontal,
                              const CostParams& cost_params)
{
  const auto& cost_table = is_horizontal ? h_cost_table_ : v_cost_table_;
  const int capacity = is_horizontal ? h_capacity_ : v_capacity_;

  if (index < cost_table.size()) {
    return cost_table[index];
  }

  const int slope = cost_params.slope;
  const double logistic_coef = cost_params.logistic_coef;
  const double cost_height = cost_params.cost_height;

  double cost
      = cost_height / (std::exp((capacity - index) * logistic_coef) + 1) + 1;
  if (index >= capacity) {
    cost += (cost_height / slope * (index - capacity));
  }

  return cost;
}

void FastRouteCore::mazeRouteMSMD(const int iter,
                                  const int expand,
                                  const int ripup_threshold,
                                  const int maze_edge_threshold,
                                  const bool ordering,
                                  const int via,
                                  const int L,
                                  const CostParams& cost_params,
                                  float& slack_th)
{
  // maze routing for multi-source, multi-destination
  int tmpX, tmpY;

  const int max_usage_multiplier = 40;

  for (int i = 0; i < max_usage_multiplier * h_capacity_; i++) {
    h_cost_table_.push_back(getCost(i, true, cost_params));
  }
  for (int i = 0; i < max_usage_multiplier * v_capacity_; i++) {
    v_cost_table_.push_back(getCost(i, false, cost_params));
  }

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      in_region_[i][j] = false;
    }
  }

  if (ordering) {
    if (critical_nets_percentage_) {
      slack_th = CalculatePartialSlack();
    }
    StNetOrder();
  }

  std::vector<double*> src_heap;
  std::vector<double*> dest_heap;
  src_heap.reserve(y_grid_ * x_grid_);
  dest_heap.reserve(y_grid_ * x_grid_);

  multi_array<double, 2> d1(boost::extents[y_range_][x_range_]);
  multi_array<double, 2> d2(boost::extents[y_range_][x_range_]);

  std::vector<bool> pop_heap2(y_grid_ * x_range_, false);

  /**
   * @brief Updates the cost of an adjacent grid if the new cost is lower,
   * updating the heap accordingly. Also updates parent indexes if cost was
   * updated. Throws an error if the position can't be found.
   * */
  auto updateAdjacent = [&](const int cur_x,
                            const int cur_y,
                            const int adj_x,
                            const int adj_y,
                            double cost,
                            const int net_id) {
    double adj_cost = d1[adj_y][adj_x];
    if (adj_cost <= cost) {
      return;
    }

    d1[adj_y][adj_x] = cost;

    if (cur_x != adj_x) {
      parent_x3_[adj_y][adj_x] = cur_x;
      parent_y3_[adj_y][adj_x] = cur_y;
      hv_[adj_y][adj_x] = false;
    } else {
      parent_x1_[adj_y][adj_x] = cur_x;
      parent_y1_[adj_y][adj_x] = cur_y;
      hv_[adj_y][adj_x] = true;
    }

    if (adj_cost >= BIG_INT) {  // neighbor has not been put into src_heap
      src_heap.push_back(&d1[adj_y][adj_x]);
      updateHeap(src_heap, src_heap.size() - 1);
    } else if (adj_cost > cost) {  // neighbor has been put into src_heap
                                   // but needs update
      double* dtmp = &d1[adj_y][adj_x];
      const auto it = std::ranges::find(src_heap, dtmp);
      if (it != src_heap.end()) {
        const int pos = it - src_heap.begin();
        updateHeap(src_heap, pos);
      } else {
        logger_->error(
            GRT,
            607,
            "Unable to update: position not found in 2D heap for net {}.",
            nets_[net_id]->getName());
      }
    }
  };

  /**
   * @brief Relaxes the cost for adjacent grids based on current grid's cost and
   * edge usage. It optionally adds a via cost and checks for potential hyper
   * edges, updating adjacent grids accordingly.
   */
  auto relaxAdjacent = [&](const int cur_x,
                           const int cur_y,
                           const int d_x,
                           const int d_y,
                           const bool add_via,
                           const bool maybe_hyper,
                           const int net_id) {
    const bool is_horizontal = d_x != 0;
    auto& hyper = is_horizontal ? hyper_h_ : hyper_v_;

    const int p1_x = cur_x - (d_x == -1);
    const int p1_y = cur_y - (d_y == -1);
    const int p2_x = cur_x - (d_x == 1);
    const int p2_y = cur_y - (d_y == 1);

    auto usage_red
        = is_horizontal ? &Graph2D::getUsageRedH : &Graph2D::getUsageRedV;
    auto last_usage
        = is_horizontal ? &Graph2D::getLastUsageH : &Graph2D::getLastUsageV;
    const int pos1 = (graph2d_.*usage_red)(p1_x, p1_y)
                     + L * (graph2d_.*last_usage)(p1_x, p1_y);

    double cost1 = getCost(pos1, is_horizontal, cost_params);

    double tmp = d1[cur_y][cur_x] + cost1;

    if (add_via && d1[cur_y][cur_x] != 0) {
      tmp += via;

      if (maybe_hyper) {
        const int pos2 = (graph2d_.*usage_red)(p2_x, p2_y)
                         + L * (graph2d_.*last_usage)(p2_x, p2_y);

        double cost2 = getCost(pos2, is_horizontal, cost_params);

        const int tmp_cost = d1[cur_y - d_y][cur_x - d_x] + cost2;
        if (tmp_cost < d1[cur_y][cur_x] + via) {
          hyper[cur_y][cur_x] = true;
        }
      }
    }

    updateAdjacent(cur_x, cur_y, cur_x + d_x, cur_y + d_y, tmp, net_id);
  };

  std::vector<OrderNetEdge> net_eo;
  net_eo.reserve(2ul * max_degree_);

  for (int nidRPC = 0; nidRPC < net_ids_.size(); nidRPC++) {
    const int netID
        = ordering ? tree_order_cong_[nidRPC].treeIndex : net_ids_[nidRPC];

    const int num_terminals = sttrees_[netID].num_terminals;

    const int origENG = expand;

    netedgeOrderDec(netID, net_eo);

    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;
    // loop for all the tree edges
    const int num_edges = sttrees_[netID].num_edges();
    for (int edgeREC = 0; edgeREC < num_edges; edgeREC++) {
      const int edgeID = net_eo[edgeREC].edgeID;
      TreeEdge* treeedge = &(treeedges[edgeID]);

      int n1 = treeedge->n1;
      int n2 = treeedge->n2;
      const int n1x = treenodes[n1].x;
      const int n1y = treenodes[n1].y;
      const int n2x = treenodes[n2].x;
      const int n2y = treenodes[n2].y;
      treeedge->len = abs(n2x - n1x) + abs(n2y - n1y);

      if (treeedge->len
          <= maze_edge_threshold)  // only route the non-degraded edges (len>0)
      {
        continue;
      }

      const bool enter = newRipupCheck(treeedge,
                                       n1x,
                                       n1y,
                                       n2x,
                                       n2y,
                                       ripup_threshold,
                                       slack_th,
                                       netID,
                                       edgeID);

      if (!enter) {
        continue;
      }

      // ripup the routing for the edge
      const auto [ymin, ymax] = std::minmax(n1y, n2y);
      const auto [xmin, xmax] = std::minmax(n1x, n2x);

      enlarge_ = std::min(origENG, (iter / 6 + 3) * treeedge->route.routelen);

      int decrease = 0;

      if (nets_[netID]->isCritical()) {
        decrease = std::min((iter / 7) * 5, enlarge_ / 2);
      }
      const int regionX1 = std::max(xmin - enlarge_ + decrease, 0);
      const int regionX2 = std::min(xmax + enlarge_ - decrease, x_grid_ - 1);
      const int regionY1 = std::max(ymin - enlarge_ + decrease, 0);
      const int regionY2 = std::min(ymax + enlarge_ - decrease, y_grid_ - 1);

      // initialize d1[][] and d2[][] as BIG_INT
      for (int i = regionY1; i <= regionY2; i++) {
        for (int j = regionX1; j <= regionX2; j++) {
          d1[i][j] = BIG_INT;
          d2[i][j] = BIG_INT;
          hyper_h_[i][j] = false;
          hyper_v_[i][j] = false;
        }
      }

      // setup src_heap, dest_heap and initialize d1[][] and d2[][] for all the
      // grids on the two subtrees
      setupHeap(netID,
                edgeID,
                src_heap,
                dest_heap,
                d1,
                d2,
                regionX1,
                regionX2,
                regionY1,
                regionY2);

      // while loop to find shortest path
      int ind1 = (src_heap[0] - &d1[0][0]);
      for (auto& dest : dest_heap) {
        pop_heap2[dest - &d2[0][0]] = true;
      }

      // stop when the grid position been popped out from both src_heap and
      // dest_heap
      while (pop_heap2[ind1] == false) {
        // relax all the adjacent grids within the enlarged region for
        // source subtree
        const int curX = ind1 % x_range_;
        const int curY = ind1 / x_range_;

        int preX = curX;
        int preY = curY;
        if (d1[curY][curX] != 0) {
          preX = hv_[curY][curX] ? parent_x1_[curY][curX]
                                 : parent_x3_[curY][curX];
          preY = hv_[curY][curX] ? parent_y1_[curY][curX]
                                 : parent_y3_[curY][curX];
        }

        removeMin(src_heap);

        if (curX > regionX1) {  // left
          relaxAdjacent(
              curX, curY, -1, 0, preY != curY, curX < regionX2 - 1, netID);
        }
        if (curX < regionX2) {  // right
          relaxAdjacent(
              curX, curY, 1, 0, preY != curY, curX > regionX1 + 1, netID);
        }
        if (curY > regionY1) {  // bottom
          relaxAdjacent(
              curX, curY, 0, -1, preX != curX, curY < regionY2 - 1, netID);
        }
        if (curY < regionY2) {  // top
          relaxAdjacent(
              curX, curY, 0, 1, preX != curX, curY > regionY1 + 1, netID);
        }

        // update ind1 for next loop
        ind1 = (src_heap[0] - &d1[0][0]);

      }  // while loop

      for (auto& dest : dest_heap) {
        pop_heap2[dest - &d2[0][0]] = false;
      }

      const int16_t crossX = ind1 % x_range_;
      const int16_t crossY = ind1 / x_range_;

      int cnt = 0;
      int16_t curX = crossX;
      int16_t curY = crossY;
      std::vector<GPoint3D> tmp_grids;
      while (d1[curY][curX] != 0)  // loop until reach subtree1
      {
        bool hypered = false;
        if (cnt != 0) {
          if (curX != tmpX && hyper_h_[curY][curX]) {
            curX = 2 * curX - tmpX;
            hypered = true;
          }

          if (curY != tmpY && hyper_v_[curY][curX]) {
            curY = 2 * curY - tmpY;
            hypered = true;
          }
        }
        tmpX = curX;
        tmpY = curY;
        if (!hypered) {
          if (hv_[tmpY][tmpX]) {
            curY = parent_y1_[tmpY][tmpX];
          } else {
            curX = parent_x3_[tmpY][tmpX];
          }
        }
        tmp_grids.push_back({curX, curY, -1});
        cnt++;
      }
      // reverse the grids on the path
      std::vector<GPoint3D> grids(tmp_grids.rbegin(), tmp_grids.rend());

      // add the connection point (crossX, crossY)
      grids.push_back({crossX, crossY, -1});
      cnt++;

      curX = crossX;
      curY = crossY;
      const int cnt_n1n2 = cnt;

      // change the tree structure according to the new routing for the tree
      // edge find E1 and E2, and the endpoints of the edges they are on
      const int E1x = grids[0].x;
      const int E1y = grids[0].y;
      const int E2x = grids.back().x;
      const int E2y = grids.back().y;

      const int edge_n1n2 = edgeID;
      // (1) consider subtree1
      if (n1 < num_terminals && (E1x != n1x || E1y != n1y)) {
        // split neighbor edge and return id new node
        n1 = splitEdge(treeedges, treenodes, n2, n1, edgeID);
      }
      if (n1 >= num_terminals && (E1x != n1x || E1y != n1y))
      // n1 is not a pin and E1!=n1, then make change to subtree1,
      // otherwise, no change to subtree1
      {
        // find the endpoints of the edge E1 is on
        const int endpt1 = treeedges[corr_edge_[E1y][E1x]].n1;
        const int endpt2 = treeedges[corr_edge_[E1y][E1x]].n2;

        // find A1, A2 and edge_n1A1, edge_n1A2
        int A1, A2;
        int edge_n1A1, edge_n1A2;
        if (treenodes[n1].nbr[0] == n2) {
          A1 = treenodes[n1].nbr[1];
          A2 = treenodes[n1].nbr[2];
          edge_n1A1 = treenodes[n1].edge[1];
          edge_n1A2 = treenodes[n1].edge[2];
        } else if (treenodes[n1].nbr[1] == n2) {
          A1 = treenodes[n1].nbr[0];
          A2 = treenodes[n1].nbr[2];
          edge_n1A1 = treenodes[n1].edge[0];
          edge_n1A2 = treenodes[n1].edge[2];
        } else {
          A1 = treenodes[n1].nbr[0];
          A2 = treenodes[n1].nbr[1];
          edge_n1A1 = treenodes[n1].edge[0];
          edge_n1A2 = treenodes[n1].edge[1];
        }

        if (endpt1 == n1 || endpt2 == n1)  // E1 is on (n1, A1) or (n1, A2)
        {
          // if E1 is on (n1, A2), switch A1 and A2 so that E1 is always on
          // (n1, A1)
          if (endpt1 == A2 || endpt2 == A2) {
            std::swap(A1, A2);
            std::swap(edge_n1A1, edge_n1A2);
          }

          // update route for edge (n1, A1), (n1, A2)
          bool route_ok = updateRouteType1(netID,
                                           treenodes,
                                           n1,
                                           A1,
                                           A2,
                                           E1x,
                                           E1y,
                                           treeedges,
                                           edge_n1A1,
                                           edge_n1A2);
          if (!route_ok) {
            if (verbose_) {
              logger_->error(GRT,
                             150,
                             "Net {} has errors during updateRouteType1.",
                             nets_[netID]->getName());
            }
            reInitTree(netID);
            nidRPC--;
            break;
          }
          // update position for n1
          treenodes[n1].x = E1x;
          treenodes[n1].y = E1y;
        }  // if E1 is on (n1, A1) or (n1, A2)
        else  // E1 is not on (n1, A1) or (n1, A2), but on (C1, C2)
        {
          const int C1 = endpt1;
          const int C2 = endpt2;
          const int edge_C1C2 = corr_edge_[E1y][E1x];

          // update route for edge (n1, C1), (n1, C2) and (A1, A2)
          bool route_ok = updateRouteType2(netID,
                                           treenodes,
                                           n1,
                                           A1,
                                           A2,
                                           C1,
                                           C2,
                                           E1x,
                                           E1y,
                                           treeedges,
                                           edge_n1A1,
                                           edge_n1A2,
                                           edge_C1C2);
          if (!route_ok) {
            debugPrint(logger_,
                       utl::GRT,
                       "maze_2d",
                       1,
                       "Net {} has errors during updateRouteType2.",
                       nets_[netID]->getName());
            reInitTree(netID);
            nidRPC--;
            break;
          }
          // update position for n1
          treenodes[n1].x = E1x;
          treenodes[n1].y = E1y;
          // update 3 edges (n1, A1)->(C1, n1), (n1, A2)->(n1, C2), (C1,
          // C2)->(A1, A2)
          const int edge_n1C1 = edge_n1A1;
          treeedges[edge_n1C1].n1 = C1;
          treeedges[edge_n1C1].n2 = n1;
          const int edge_n1C2 = edge_n1A2;
          treeedges[edge_n1C2].n1 = n1;
          treeedges[edge_n1C2].n2 = C2;
          const int edge_A1A2 = edge_C1C2;
          treeedges[edge_A1A2].n1 = A1;
          treeedges[edge_A1A2].n2 = A2;
          // update nbr and edge for 5 nodes n1, A1, A2, C1, C2
          // n1's nbr (n2, A1, A2)->(n2, C1, C2)
          treenodes[n1].nbr[0] = n2;
          treenodes[n1].edge[0] = edge_n1n2;
          treenodes[n1].nbr[1] = C1;
          treenodes[n1].edge[1] = edge_n1C1;
          treenodes[n1].nbr[2] = C2;
          treenodes[n1].edge[2] = edge_n1C2;
          // A1's nbr n1->A2
          for (int i = 0; i < 3; i++) {
            if (treenodes[A1].nbr[i] == n1) {
              treenodes[A1].nbr[i] = A2;
              treenodes[A1].edge[i] = edge_A1A2;
              break;
            }
          }
          // A2's nbr n1->A1
          for (int i = 0; i < 3; i++) {
            if (treenodes[A2].nbr[i] == n1) {
              treenodes[A2].nbr[i] = A1;
              treenodes[A2].edge[i] = edge_A1A2;
              break;
            }
          }
          // C1's nbr C2->n1
          for (int i = 0; i < 3; i++) {
            if (treenodes[C1].nbr[i] == C2) {
              treenodes[C1].nbr[i] = n1;
              treenodes[C1].edge[i] = edge_n1C1;
              break;
            }
          }
          // C2's nbr C1->n1
          for (int i = 0; i < 3; i++) {
            if (treenodes[C2].nbr[i] == C1) {
              treenodes[C2].nbr[i] = n1;
              treenodes[C2].edge[i] = edge_n1C2;
              break;
            }
          }

        }  // else E1 is not on (n1, A1) or (n1, A2), but on (C1, C2)
      }  // n1 is not a pin and E1!=n1

      // (2) consider subtree2
      if (n2 < num_terminals && (E2x != n2x || E2y != n2y)) {
        // split neighbor edge and return id new node
        n2 = splitEdge(treeedges, treenodes, n1, n2, edgeID);
      }
      if (n2 >= num_terminals && (E2x != n2x || E2y != n2y))
      // n2 is not a pin and E2!=n2, then make change to subtree2,
      // otherwise, no change to subtree2
      {
        // find the endpoints of the edge E1 is on
        const int endpt1 = treeedges[corr_edge_[E2y][E2x]].n1;
        const int endpt2 = treeedges[corr_edge_[E2y][E2x]].n2;

        // find B1, B2
        int B1, B2;
        int edge_n2B1, edge_n2B2;
        if (treenodes[n2].nbr[0] == n1) {
          B1 = treenodes[n2].nbr[1];
          B2 = treenodes[n2].nbr[2];
          edge_n2B1 = treenodes[n2].edge[1];
          edge_n2B2 = treenodes[n2].edge[2];
        } else if (treenodes[n2].nbr[1] == n1) {
          B1 = treenodes[n2].nbr[0];
          B2 = treenodes[n2].nbr[2];
          edge_n2B1 = treenodes[n2].edge[0];
          edge_n2B2 = treenodes[n2].edge[2];
        } else {
          B1 = treenodes[n2].nbr[0];
          B2 = treenodes[n2].nbr[1];
          edge_n2B1 = treenodes[n2].edge[0];
          edge_n2B2 = treenodes[n2].edge[1];
        }

        if (endpt1 == n2 || endpt2 == n2)  // E2 is on (n2, B1) or (n2, B2)
        {
          // if E2 is on (n2, B2), switch B1 and B2 so that E2 is always on
          // (n2, B1)
          if (endpt1 == B2 || endpt2 == B2) {
            std::swap(B1, B2);
            std::swap(edge_n2B1, edge_n2B2);
          }

          // update route for edge (n2, B1), (n2, B2)
          bool route_ok = updateRouteType1(netID,
                                           treenodes,
                                           n2,
                                           B1,
                                           B2,
                                           E2x,
                                           E2y,
                                           treeedges,
                                           edge_n2B1,
                                           edge_n2B2);
          if (!route_ok) {
            debugPrint(logger_,
                       utl::GRT,
                       "maze_2d",
                       1,
                       "Net {} has errors during updateRouteType1.",
                       nets_[netID]->getName());
            reInitTree(netID);
            nidRPC--;
            break;
          }

          // update position for n2
          treenodes[n2].x = E2x;
          treenodes[n2].y = E2y;
        }  // if E2 is on (n2, B1) or (n2, B2)
        else  // E2 is not on (n2, B1) or (n2, B2), but on (D1, D2)
        {
          const int D1 = endpt1;
          const int D2 = endpt2;
          const int edge_D1D2 = corr_edge_[E2y][E2x];

          // update route for edge (n2, D1), (n2, D2) and (B1, B2)
          bool route_ok = updateRouteType2(netID,
                                           treenodes,
                                           n2,
                                           B1,
                                           B2,
                                           D1,
                                           D2,
                                           E2x,
                                           E2y,
                                           treeedges,
                                           edge_n2B1,
                                           edge_n2B2,
                                           edge_D1D2);
          if (!route_ok) {
            debugPrint(logger_,
                       utl::GRT,
                       "maze_2d",
                       1,
                       "Net {} has errors during updateRouteType2.",
                       nets_[netID]->getName());
            reInitTree(netID);
            nidRPC--;
            break;
          }
          // update position for n2
          treenodes[n2].x = E2x;
          treenodes[n2].y = E2y;
          // update 3 edges (n2, B1)->(D1, n2), (n2, B2)->(n2, D2), (D1,
          // D2)->(B1, B2)
          const int edge_n2D1 = edge_n2B1;
          treeedges[edge_n2D1].n1 = D1;
          treeedges[edge_n2D1].n2 = n2;
          const int edge_n2D2 = edge_n2B2;
          treeedges[edge_n2D2].n1 = n2;
          treeedges[edge_n2D2].n2 = D2;
          const int edge_B1B2 = edge_D1D2;
          treeedges[edge_B1B2].n1 = B1;
          treeedges[edge_B1B2].n2 = B2;
          // update nbr and edge for 5 nodes n2, B1, B2, D1, D2
          // n1's nbr (n1, B1, B2)->(n1, D1, D2)
          treenodes[n2].nbr[0] = n1;
          treenodes[n2].edge[0] = edge_n1n2;
          treenodes[n2].nbr[1] = D1;
          treenodes[n2].edge[1] = edge_n2D1;
          treenodes[n2].nbr[2] = D2;
          treenodes[n2].edge[2] = edge_n2D2;
          // B1's nbr n2->B2
          for (int i = 0; i < 3; i++) {
            if (treenodes[B1].nbr[i] == n2) {
              treenodes[B1].nbr[i] = B2;
              treenodes[B1].edge[i] = edge_B1B2;
              break;
            }
          }
          // B2's nbr n2->B1
          for (int i = 0; i < 3; i++) {
            if (treenodes[B2].nbr[i] == n2) {
              treenodes[B2].nbr[i] = B1;
              treenodes[B2].edge[i] = edge_B1B2;
              break;
            }
          }
          // D1's nbr D2->n2
          for (int i = 0; i < 3; i++) {
            if (treenodes[D1].nbr[i] == D2) {
              treenodes[D1].nbr[i] = n2;
              treenodes[D1].edge[i] = edge_n2D1;
              break;
            }
          }
          // D2's nbr D1->n2
          for (int i = 0; i < 3; i++) {
            if (treenodes[D2].nbr[i] == D1) {
              treenodes[D2].nbr[i] = n2;
              treenodes[D2].edge[i] = edge_n2D2;
              break;
            }
          }
        }  // else E2 is not on (n2, B1) or (n2, B2), but on (D1, D2)
      }  // n2 is not a pin and E2!=n2

      // update route for edge (n1, n2) and edge usage
      if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
        treeedges[edge_n1n2].route.grids.clear();
      }
      treeedges[edge_n1n2].route.grids.resize(cnt_n1n2);
      treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
      treeedges[edge_n1n2].route.routelen = cnt_n1n2 - 1;
      treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y - E2y);

      for (int i = 0; i < cnt_n1n2; i++) {
        treeedges[edge_n1n2].route.grids[i].x = grids[i].x;
        treeedges[edge_n1n2].route.grids[i].y = grids[i].y;
      }

      FrNet* net = nets_[netID];
      int8_t edgeCost = net->getEdgeCost();

      // update edge usage
      for (int i = 0; i < cnt_n1n2 - 1; i++) {
        if (grids[i].x == grids[i + 1].x)  // a vertical edge
        {
          const int min_y = std::min(grids[i].y, grids[i + 1].y);
          graph2d_.updateUsageV(grids[i].x, min_y, net, edgeCost);
        } else  // a horizontal edge
        {
          const int min_x = std::min(grids[i].x, grids[i + 1].x);
          graph2d_.updateUsageH(min_x, grids[i].y, net, edgeCost);
        }
      }
    }  // loop edgeID
  }  // loop netID

  h_cost_table_.clear();
  v_cost_table_.clear();
}

void FastRouteCore::findCongestedEdgesNets(
    NetsPerCongestedArea& nets_in_congested_edges,
    bool vertical)
{
  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID] == nullptr) {
      continue;
    }

    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      const TreeEdge* treeedge = &(treeedges[edgeID]);
      if (treeedge->len > 0) {
        int routeLen = treeedge->route.routelen;
        const std::vector<GPoint3D>& grids = treeedge->route.grids;
        int lastX = tile_size_ * (grids[0].x + 0.5) + x_corner_;
        int lastY = tile_size_ * (grids[0].y + 0.5) + y_corner_;

        for (int i = 1; i <= routeLen; i++) {
          const int xreal = tile_size_ * (grids[i].x + 0.5) + x_corner_;
          const int yreal = tile_size_ * (grids[i].y + 0.5) + y_corner_;

          // a zero-length edge does not store any congestion info.
          if (lastX == xreal && lastY == yreal) {
            continue;
          }

          bool vertical_edge = xreal == lastX;

          if (vertical_edge == vertical) {
            // the congestion information on an edge (x1, y1) -> (x2, y2) is
            // stored only in (x1, y1), where x1 <= x2 and y1 <= y2. therefore,
            // it gets the min between each value to properly define which nets
            // are crossing a congested area.
            int x = std::min(lastX, xreal);
            int y = std::min(lastY, yreal);
            NetsPerCongestedArea::iterator it
                = nets_in_congested_edges.find({x, y});
            if (it != nets_in_congested_edges.end()) {
              it->second.nets.insert(nets_[netID]->getDbNet());
            }
          }

          lastX = xreal;
          lastY = yreal;
        }
      }
    }
  }
}

void FastRouteCore::getCongestionGrid(
    std::vector<CongestionInformation>& congestionGridV,
    std::vector<CongestionInformation>& congestionGridH)
{
  NetsPerCongestedArea nets_in_congested_edges;

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      const int overflow = graph2d_.getOverflowH(j, i);
      if (overflow > 0) {
        const int xreal = tile_size_ * (j + 0.5) + x_corner_;
        const int yreal = tile_size_ * (i + 0.5) + y_corner_;
        const int usage = graph2d_.getUsageH(j, i);
        const int capacity = graph2d_.getCapH(j, i);
        nets_in_congested_edges[{xreal, yreal}].congestion = {capacity, usage};
      }
    }
  }
  findCongestedEdgesNets(nets_in_congested_edges, false);
  for (const auto& [edge, tile_info] : nets_in_congested_edges) {
    TileCongestion congestion = tile_info.congestion;
    const auto& segment
        = GSegment(edge.first, edge.second, 1, edge.first, edge.second, 1);
    const auto& horizontal_srcs
        = nets_in_congested_edges[{segment.init_x, segment.init_y}].nets;
    congestionGridH.push_back({segment, congestion, horizontal_srcs});
  }
  nets_in_congested_edges.clear();

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      const int overflow = graph2d_.getOverflowV(j, i);
      if (overflow > 0) {
        const int xreal = tile_size_ * (j + 0.5) + x_corner_;
        const int yreal = tile_size_ * (i + 0.5) + y_corner_;
        const int usage = graph2d_.getUsageV(j, i);
        const int capacity = graph2d_.getCapV(j, i);
        nets_in_congested_edges[{xreal, yreal}].congestion = {capacity, usage};
      }
    }
  }
  findCongestedEdgesNets(nets_in_congested_edges, true);
  for (const auto& [edge, tile_info] : nets_in_congested_edges) {
    TileCongestion congestion = tile_info.congestion;
    const auto& segment
        = GSegment(edge.first, edge.second, 1, edge.first, edge.second, 1);
    const auto& vertical_srcs
        = nets_in_congested_edges[{segment.init_x, segment.init_y}].nets;
    congestionGridV.push_back({segment, congestion, vertical_srcs});
  }
}

void FastRouteCore::findNetsNearPosition(std::set<odb::dbNet*>& congestion_nets,
                                         const odb::Point& position,
                                         bool is_horizontal,
                                         int& radius)
{
  // get Nets with overflow
  for (int netID = 0; netID < netCount(); netID++) {
    if (nets_[netID] == nullptr
        || (congestion_nets.find(nets_[netID]->getDbNet())
            != congestion_nets.end())) {
      continue;
    }

    const auto& treeedges = sttrees_[netID].edges;
    const int num_edges = sttrees_[netID].num_edges();

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
          if (abs(ymin - position.getY()) <= radius
              && abs(grids[i].x - position.getX()) <= radius
              && !is_horizontal) {
            congestion_nets.insert(nets_[netID]->getDbNet());
          }
        } else if (grids[i].y == grids[i + 1].y) {  // a horizontal edge
          const int xmin = std::min(grids[i].x, grids[i + 1].x);
          if (abs(grids[i].y - position.getY()) <= radius
              && abs(xmin - position.getX()) <= radius && is_horizontal) {
            congestion_nets.insert(nets_[netID]->getDbNet());
          }
        }
      }
    }
  }
}

// Get overflow positions
void FastRouteCore::getOverflowPositions(
    std::vector<std::pair<odb::Point, bool>>& overflow_pos)
{
  // Find horizontal ggrids with congestion
  for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
    const int overflow = graph2d_.getOverflowH(x, y);
    if (overflow > 0) {
      overflow_pos.emplace_back(odb::Point(x, y), true);
    }
  }
  // Find vertical ggrids with congestion
  for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
    const int overflow = graph2d_.getOverflowV(x, y);
    if (overflow > 0) {
      overflow_pos.emplace_back(odb::Point(x, y), false);
    }
  }
}

// Search in range of 5 the correct adjustment to fix the overflow
void FastRouteCore::getPrecisionAdjustment(const int x,
                                           const int y,
                                           bool is_horizontal,
                                           int& adjustment)
{
  int new_2D_cap, usage_2D, range = 5;
  if (is_horizontal) {
    usage_2D = graph2d_.getUsageH(x, y);
  } else {
    usage_2D = graph2d_.getUsageV(x, y);
  }

  new_2D_cap = 0;
  while (range > 0 && new_2D_cap < usage_2D) {
    // calculate new capacity with adjustment
    for (int l = 0; l < num_layers_; l++) {
      if (is_horizontal) {
        new_2D_cap
            += (1.0 - (adjustment / 100.0)) * h_edges_3D_[l][y][x].real_cap;
      } else {
        new_2D_cap
            += (1.0 - (adjustment / 100.0)) * v_edges_3D_[l][y][x].real_cap;
      }
    }
    // Reduce adjustment to increase capacity
    if (usage_2D > new_2D_cap) {
      adjustment--;
      new_2D_cap = 0;
    }
    range--;
  }
}

// For each edge with overflow, calculate the ideal adjustment
// Return the minimum value of all or -1 if no value can be found
bool FastRouteCore::computeSuggestedAdjustment(int& suggested_adjustment)
{
  int horizontal_suggest = 100, local_suggest;
  bool has_new_adj;
  // Find horizontal ggrids with congestion
  for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
    const int overflow = graph2d_.getOverflowH(x, y);
    if (overflow > 0) {
      has_new_adj
          = graph2d_.computeSuggestedAdjustment(x, y, true, local_suggest);
      if (has_new_adj) {
        // modify the value to resolve the congestion at the position
        getPrecisionAdjustment(x, y, true, local_suggest);
        horizontal_suggest = std::min(horizontal_suggest, local_suggest);
      } else {
        return false;
      }
    }
  }
  int vertical_suggest = 100;
  // Find vertical ggrids with congestion
  for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
    const int overflow = graph2d_.getOverflowV(x, y);
    if (overflow > 0) {
      has_new_adj
          = graph2d_.computeSuggestedAdjustment(x, y, false, local_suggest);
      if (has_new_adj) {
        // modify the value to resolve the congestion at the position
        getPrecisionAdjustment(x, y, false, local_suggest);
        vertical_suggest = std::min(vertical_suggest, local_suggest);
      } else {
        return false;
      }
    }
  }
  suggested_adjustment = std::min(horizontal_suggest, vertical_suggest);
  return true;
}

// The function will add the new nets to the congestion_nets set
void FastRouteCore::getCongestionNets(std::set<odb::dbNet*>& congestion_nets)
{
  // Get overflow position -- [(x,y), is horizontal]
  std::vector<std::pair<odb::Point, bool>> overflow_positions;
  getOverflowPositions(overflow_positions);

  int old_size = congestion_nets.size();

  // The radius around the congested zone is increased when no new nets are
  // obtained
  for (int radius = 0; radius < 5 && old_size == congestion_nets.size();
       radius++) {
    // Find nets for each congestion ggrid
    for (const auto& position : overflow_positions) {
      findNetsNearPosition(
          congestion_nets, position.first, position.second, radius);
    }
  }
}

int FastRouteCore::getOverflow2Dmaze(int* maxOverflow, int* tUsage)
{
  int H_overflow = 0;
  int V_overflow = 0;
  int max_H_overflow = 0;
  int max_V_overflow = 0;
  int numedges = 0;

  // check 2D edges for invalid usage values
  check2DEdgesUsage();

  int total_usage = 0;
  for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
    total_usage += graph2d_.getUsageH(x, y);
    const int overflow = graph2d_.getOverflowH(x, y);
    if (overflow > 0) {
      if (logger_->debugCheck(GRT, "congestion2D", 1)) {
        // Convert to real coordinates
        int x_real = tile_size_ * (x + 0.5) + x_corner_;
        int y_real = tile_size_ * (y + 0.5) + y_corner_;
        logger_->report("H 2D Overflow x{} y{} ({} {})", x, y, x_real, y_real);
      }
      H_overflow += overflow;
      max_H_overflow = std::max(max_H_overflow, overflow);
      numedges++;
    }
  }

  for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
    total_usage += graph2d_.getUsageV(x, y);
    const int overflow = graph2d_.getOverflowV(x, y);
    if (overflow > 0) {
      if (logger_->debugCheck(GRT, "congestion2D", 1)) {
        // Convert to real coordinates
        int x_real = tile_size_ * (x + 0.5) + x_corner_;
        int y_real = tile_size_ * (y + 0.5) + y_corner_;
        logger_->report("V 2D Overflow x{} y{} ({} {})", x, y, x_real, y_real);
      }
      V_overflow += overflow;
      max_V_overflow = std::max(max_V_overflow, overflow);
      numedges++;
    }
  }

  int max_overflow = std::max(max_H_overflow, max_V_overflow);
  total_overflow_ = H_overflow + V_overflow;
  *maxOverflow = max_overflow;

  if (logger_->debugCheck(GRT, "congestion2D", 1)) {
    logger_->report("Overflow report:");
    logger_->report("Total usage          : {}", total_usage);
    logger_->report("Max H overflow       : {}", max_H_overflow);
    logger_->report("Max V overflow       : {}", max_V_overflow);
    logger_->report("Max overflow         : {}", max_overflow);
    logger_->report("Number overflow edges: {}", numedges);
    logger_->report("H   overflow         : {}", H_overflow);
    logger_->report("V   overflow         : {}", V_overflow);
    logger_->report("Final overflow       : {}\n", total_overflow_);
  }

  *tUsage = total_usage;

  if (total_usage > 800000) {
    ahth_ = 30;
  } else {
    ahth_ = 20;
  }

  return total_overflow_;
}

int FastRouteCore::getOverflow2D(int* maxOverflow)
{
  // check 2D edges for invalid usage values
  check2DEdgesUsage();

  // get overflow
  int H_overflow = 0;
  int V_overflow = 0;
  int max_H_overflow = 0;
  int max_V_overflow = 0;
  int hCap = 0;
  int vCap = 0;
  int numedges = 0;

  int total_usage = 0;

  for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
    total_usage += graph2d_.getEstUsageH(x, y);
    const int overflow = graph2d_.getEstUsageH(x, y) - graph2d_.getCapH(x, y);
    hCap += graph2d_.getCapH(x, y);
    if (overflow > 0) {
      if (logger_->debugCheck(GRT, "congestion2D", 1)) {
        // Convert to real coordinates
        int x_real = tile_size_ * (x + 0.5) + x_corner_;
        int y_real = tile_size_ * (y + 0.5) + y_corner_;
        logger_->report("H 2D Overflow x{} y{} ({} {})", x, y, x_real, y_real);
      }
      H_overflow += overflow;
      max_H_overflow = std::max(max_H_overflow, overflow);
      numedges++;
    }
  }

  for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
    total_usage += graph2d_.getEstUsageV(x, y);
    const int overflow = graph2d_.getEstUsageV(x, y) - graph2d_.getCapV(x, y);
    vCap += graph2d_.getCapV(x, y);
    if (overflow > 0) {
      if (logger_->debugCheck(GRT, "congestion2D", 1)) {
        // Convert to real coordinates
        int x_real = tile_size_ * (x + 0.5) + x_corner_;
        int y_real = tile_size_ * (y + 0.5) + y_corner_;
        logger_->report("V 2D Overflow x{} y{} ({} {})", x, y, x_real, y_real);
      }
      V_overflow += overflow;
      max_V_overflow = std::max(max_V_overflow, overflow);
      numedges++;
    }
  }

  const int max_overflow = std::max(max_H_overflow, max_V_overflow);
  total_overflow_ = H_overflow + V_overflow;
  *maxOverflow = max_overflow;

  if (total_usage > 800000) {
    ahth_ = 30;
  } else {
    ahth_ = 20;
  }

  if (logger_->debugCheck(GRT, "congestion2D", 1)) {
    logger_->report("Overflow report.");
    logger_->report("Total hCap               : {}", hCap);
    logger_->report("Total vCap               : {}", vCap);
    logger_->report("Total usage              : {}", total_usage);
    logger_->report("Max H overflow           : {}", max_H_overflow);
    logger_->report("Max V overflow           : {}", max_V_overflow);
    logger_->report("Max overflow             : {}", max_overflow);
    logger_->report("Number of overflow edges : {}", numedges);
    logger_->report("H   overflow             : {}", H_overflow);
    logger_->report("V   overflow             : {}", V_overflow);
    logger_->report("Final overflow           : {}\n", total_overflow_);
  }

  return total_overflow_;
}

int FastRouteCore::getOverflow3D()
{
  // get overflow
  int overflow = 0;
  int H_overflow = 0;
  int V_overflow = 0;
  int max_H_overflow = 0;
  int max_V_overflow = 0;

  int total_usage = 0;

  for (int k = 0; k < num_layers_; k++) {
    for (const auto& [x, y] : graph2d_.getUsedGridsH()) {
      total_usage += h_edges_3D_[k][y][x].usage;
      overflow = h_edges_3D_[k][y][x].usage - h_edges_3D_[k][y][x].cap;

      if (overflow > 0) {
        if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
          // Convert to real coordinates
          int x_real = tile_size_ * (x + 0.5) + x_corner_;
          int y_real = tile_size_ * (y + 0.5) + y_corner_;
          logger_->report(
              ">>> 3D H Overflow: x{} y{} l{} - Real coordinates: ({}, {})",
              x,
              y,
              k + 1,
              x_real,
              y_real);
        }
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
      }
    }
    for (const auto& [x, y] : graph2d_.getUsedGridsV()) {
      total_usage += v_edges_3D_[k][y][x].usage;
      overflow = v_edges_3D_[k][y][x].usage - v_edges_3D_[k][y][x].cap;
      if (overflow > 0) {
        if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
          // Convert to real coordinates
          int x_real = tile_size_ * (x + 0.5) + x_corner_;
          int y_real = tile_size_ * (y + 0.5) + y_corner_;
          logger_->report(
              ">>> 3D V Overflow: x{} y{} l{} - Real coordinates: ({}, {})",
              x,
              y,
              k + 1,
              x_real,
              y_real);
        }
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
      }
    }
  }

  total_overflow_ = H_overflow + V_overflow;

  if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
    logger_->report("=== Total 3D Overflow Summary ===");
    logger_->report("Total H overflow: {}", H_overflow);
    logger_->report("Total V overflow: {}", V_overflow);
    logger_->report("Max H overflow: {}", max_H_overflow);
    logger_->report("Max V overflow: {}", max_V_overflow);
    logger_->report("Total overflow: {}", total_overflow_);
  }

  return total_usage;
}

void FastRouteCore::SaveLastRouteLen()
{
  for (const int& netID : net_ids_) {
    auto& treeedges = sttrees_[netID].edges;
    // loop for all the tree edges
    const int num_edges = sttrees_[netID].num_edges();
    for (int edgeID = 0; edgeID < num_edges; edgeID++) {
      auto& edge = treeedges[edgeID];
      edge.route.last_routelen = edge.route.routelen;
    }
  }
}

}  // namespace grt
