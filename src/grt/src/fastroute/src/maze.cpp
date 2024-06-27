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

#include "DataType.h"
#include "FastRoute.h"
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

void FastRouteCore::fixEmbeddedTrees()
{
  // check embedded trees only when maze router is called
  // i.e., when running overflow iterations
  if (overflow_iterations_ > 0) {
    for (const int& netID : net_ids_) {
      checkAndFixEmbeddedTree(netID);
    }
  }
}

void FastRouteCore::checkAndFixEmbeddedTree(const int net_id)
{
  const auto& treeedges = sttrees_[net_id].edges;
  const int num_edges = sttrees_[net_id].num_edges();

  // group all edges that crosses the same position
  std::unordered_map<std::pair<short, short>,
                     std::vector<int>,
                     boost::hash<std::pair<int, int>>>
      position_to_edges_map;

  auto cmp = [&](int a, int b) { return treeedges[a].len > treeedges[b].len; };
  std::map<int, std::vector<std::pair<short, short>>, decltype(cmp)>
      edges_to_blocked_pos_map(cmp);
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    const TreeEdge* treeedge = &(treeedges[edgeID]);
    if (treeedge->len > 0) {
      int routeLen = treeedge->route.routelen;
      const std::vector<short>& gridsX = treeedge->route.gridsX;
      const std::vector<short>& gridsY = treeedge->route.gridsY;

      for (int i = 0; i <= routeLen; i++) {
        const std::pair<short, short> pos_i = {gridsX[i], gridsY[i]};
        position_to_edges_map[pos_i].push_back(edgeID);
      }
    }
  }

  // for each position, check if there is an edge that don't share
  // a node with the remaining edges
  for (auto const& [position, edges] : position_to_edges_map) {
    for (int edgeID : edges) {
      if (areEdgesOverlapping(net_id, edgeID, edges)) {
        edges_to_blocked_pos_map[edgeID].push_back(
            {position.first, position.second});
      }
    }
  }

  // fix the longest edge and break the loop
  if (!edges_to_blocked_pos_map.empty()) {
    int edge = edges_to_blocked_pos_map.begin()->first;
    std::vector<std::pair<short, short>>& blocked_positions
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
    std::vector<std::pair<short, short>>& blocked_positions)
{
  TreeEdge* treeedge = &(sttrees_[net_id].edges[edge]);
  auto& treenodes = sttrees_[net_id].nodes;

  auto sort_by_x = [](std::pair<short, short> a, std::pair<short, short> b) {
    return a.first < b.first;
  };

  std::stable_sort(
      blocked_positions.begin(), blocked_positions.end(), sort_by_x);

  if (treeedge->len > 0) {
    std::vector<short> new_route_x, new_route_y;
    const TreeNode& startpoint = treenodes[treeedge->n1];
    const TreeNode& endpoint = treenodes[treeedge->n2];
    if (startpoint.x == endpoint.x || startpoint.y == endpoint.y) {
      return;
    }
    routeLShape(
        startpoint, endpoint, blocked_positions, new_route_x, new_route_y);

    // Updates the usage of the altered edge
    const int edgeCost = nets_[net_id]->getEdgeCost();
    for (int k = 0; k < treeedge->route.routelen;
         k++) {  // remove the usages of the old edges
      if (treeedge->route.gridsX[k] == treeedge->route.gridsX[k + 1]) {
        if (treeedge->route.gridsY[k] != treeedge->route.gridsY[k + 1]) {
          const int min_y = std::min(treeedge->route.gridsY[k],
                                     treeedge->route.gridsY[k + 1]);
          v_edges_[min_y][treeedge->route.gridsX[k]].usage -= edgeCost;
        }
      } else {
        const int min_x = std::min(treeedge->route.gridsX[k],
                                   treeedge->route.gridsX[k + 1]);
        h_edges_[treeedge->route.gridsY[k]][min_x].usage -= edgeCost;
      }
    }
    for (int k = 0; k < new_route_x.size() - 1;
         k++) {  // update the usage for the new edges
      if (new_route_x[k] == new_route_x[k + 1]) {
        if (new_route_y[k] != new_route_y[k + 1]) {
          const int min_y = std::min(new_route_y[k], new_route_y[k + 1]);
          v_edges_[min_y][new_route_x[k]].usage += edgeCost;
          v_used_ggrid_.insert(std::make_pair(min_y, new_route_x[k]));
        }
      } else {
        const int min_x = std::min(new_route_x[k], new_route_x[k + 1]);
        h_edges_[new_route_y[k]][min_x].usage += edgeCost;
        h_used_ggrid_.insert(std::make_pair(new_route_y[k], min_x));
      }
    }
    treeedge->route.routelen = new_route_x.size() - 1;
    treeedge->route.gridsX = std::move(new_route_x);
    treeedge->route.gridsY = std::move(new_route_y);
  }
}

void FastRouteCore::bendEdge(
    TreeEdge* treeedge,
    std::vector<TreeNode>& treenodes,
    std::vector<short>& new_route_x,
    std::vector<short>& new_route_y,
    std::vector<std::pair<short, short>>& blocked_positions)
{
  const std::vector<short>& gridsX = treeedge->route.gridsX;
  const std::vector<short>& gridsY = treeedge->route.gridsY;

  for (int i = 0; i <= treeedge->route.routelen; i++) {
    std::pair<short, short> pos = {gridsX[i], gridsY[i]};
    if (pos == blocked_positions.front()) {
      break;
    } else {
      new_route_x.push_back(pos.first);
      new_route_y.push_back(pos.second);
    }
  }

  short x_min = std::min(treenodes[treeedge->n1].x, treenodes[treeedge->n2].x);
  short y_min = std::min(treenodes[treeedge->n1].y, treenodes[treeedge->n2].y);

  const TreeNode& endpoint = treenodes[treeedge->n2];
  if (blocked_positions.front().second == blocked_positions.back().second) {
    // blocked positions are horizontally aligned
    short y = (new_route_y.back() == y_min) ? new_route_y.back() + 1
                                            : new_route_y.back() - 1;
    new_route_x.push_back(new_route_x.back());
    new_route_y.push_back(y);

    for (short x = new_route_x.back(); x < endpoint.x; x++) {
      new_route_x.push_back(x + 1);
      new_route_y.push_back(y);
    }

    new_route_x.push_back(endpoint.x);
    new_route_y.push_back(endpoint.y);
  } else if (blocked_positions.front().first
             == blocked_positions.back().first) {
    // blocked positions are vertically aligned
    short x = (new_route_x.back() == x_min) ? new_route_x.back() + 1
                                            : new_route_x.back() - 1;
    new_route_x.push_back(x);
    new_route_y.push_back(new_route_y.back());

    if (new_route_y.back() < endpoint.y) {
      for (short y = new_route_y.back(); y < endpoint.y; y++) {
        new_route_x.push_back(x);
        new_route_y.push_back(y + 1);
      }
    } else {
      for (short y = new_route_y.back(); y > endpoint.y; y--) {
        new_route_x.push_back(x);
        new_route_y.push_back(y - 1);
      }
    }

    new_route_x.push_back(endpoint.x);
    new_route_y.push_back(endpoint.y);
  }
}

void FastRouteCore::routeLShape(
    const TreeNode& startpoint,
    const TreeNode& endpoint,
    std::vector<std::pair<short, short>>& blocked_positions,
    std::vector<short>& new_route_x,
    std::vector<short>& new_route_y)
{
  new_route_x.push_back(startpoint.x);
  new_route_y.push_back(startpoint.y);
  short first_x;
  short first_y;
  if (blocked_positions.front().second == blocked_positions.back().second) {
    // blocked positions are horizontally aligned
    short y;
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
    for (short x = first_x; x <= endpoint.x; x++) {
      new_route_x.push_back(x);
      new_route_y.push_back(y);
    }
    if (y < endpoint.y) {
      while (y < endpoint.y) {
        new_route_x.push_back(new_route_x.back());
        new_route_y.push_back(y + 1);
        y++;
      }
    } else {
      while (y > endpoint.y) {
        new_route_x.push_back(new_route_x.back());
        new_route_y.push_back(y - 1);
        y--;
      }
    }
  } else {
    // blocked positions are vertically aligned
    short x;
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
      for (short y = first_y; y <= endpoint.y; y++) {
        new_route_x.push_back(x);
        new_route_y.push_back(y);
      }
    } else {
      for (short y = first_y; y >= endpoint.y; y--) {
        new_route_x.push_back(x);
        new_route_y.push_back(y);
      }
    }
    while (x < endpoint.x) {
      new_route_y.push_back(new_route_y.back());
      new_route_x.push_back(x + 1);
      x++;
    }
  }
}

void FastRouteCore::convertToMazerouteNet(const int netID)
{
  auto& treenodes = sttrees_[netID].nodes;
  for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
    TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
    const int edgelength = treeedge->len;
    const int n1 = treeedge->n1;
    const int n2 = treeedge->n2;
    const int x1 = treenodes[n1].x;
    const int y1 = treenodes[n1].y;
    const int x2 = treenodes[n2].x;
    const int y2 = treenodes[n2].y;
    treeedge->route.gridsX.resize(edgelength + 1, 0);
    treeedge->route.gridsY.resize(edgelength + 1, 0);
    std::vector<short>& gridsX = treeedge->route.gridsX;
    std::vector<short>& gridsY = treeedge->route.gridsY;
    treeedge->len = abs(x1 - x2) + abs(y1 - y2);

    int cnt = 0;
    if (treeedge->route.type == RouteType::NoRoute) {
      gridsX[0] = x1;
      gridsY[0] = y1;
      treeedge->route.type = RouteType::MazeRoute;
      treeedge->route.routelen = 0;
      treeedge->len = 0;
      cnt++;
    } else if (treeedge->route.type == RouteType::LRoute) {
      if (treeedge->route.xFirst) {  // horizontal first
        for (int i = x1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y1;
          cnt++;
        }
        if (y1 <= y2) {
          for (int i = y1 + 1; i <= y2; i++) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (int i = y1 - 1; i >= y2; i--) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        }
      } else {  // vertical first
        if (y1 <= y2) {
          for (int i = y1; i <= y2; i++) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (int i = y1; i >= y2; i--) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
        }
        for (int i = x1 + 1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y2;
          cnt++;
        }
      }
    } else if (treeedge->route.type == RouteType::ZRoute) {
      const int Zpoint = treeedge->route.Zpoint;
      if (treeedge->route.HVH)  // HVH
      {
        for (int i = x1; i < Zpoint; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y1;
          cnt++;
        }
        if (y1 <= y2) {
          for (int i = y1; i <= y2; i++) {
            gridsX[cnt] = Zpoint;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (int i = y1; i >= y2; i--) {
            gridsX[cnt] = Zpoint;
            gridsY[cnt] = i;
            cnt++;
          }
        }
        for (int i = Zpoint + 1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y2;
          cnt++;
        }
      } else {  // VHV
        if (y1 <= y2) {
          for (int i = y1; i < Zpoint; i++) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
          for (int i = x1; i <= x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = Zpoint;
            cnt++;
          }
          for (int i = Zpoint + 1; i <= y2; i++) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (int i = y1; i > Zpoint; i--) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
          for (int i = x1; i <= x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = Zpoint;
            cnt++;
          }
          for (int i = Zpoint - 1; i >= y2; i--) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        }
      }
    }

    treeedge->route.type = RouteType::MazeRoute;
    treeedge->route.routelen = edgelength;

  }  // loop for all the edges
}

void FastRouteCore::convertToMazeroute()
{
  for (const int& netID : net_ids_) {
    convertToMazerouteNet(netID);
  }

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      // Add to keep the usage values of the last incremental routing performed
      h_edges_[i][j].usage += h_edges_[i][j].est_usage;
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      // Add to keep the usage values of the last incremental routing performed
      v_edges_[i][j].usage += v_edges_[i][j].est_usage;
    }
  }

  // check 2D edges for invalid usage values
  check2DEdgesUsage();
}

// non recursive version of heapify
static void heapify(std::vector<float*>& array)
{
  bool stop = false;
  const int heapSize = array.size();
  int i = 0;

  float* tmp = array[i];
  do {
    const int l = left_index(i);
    const int r = right_index(i);

    int smallest;
    if (l < heapSize && *(array[l]) < *tmp) {
      smallest = l;
      if (r < heapSize && *(array[r]) < *(array[l]))
        smallest = r;
    } else {
      smallest = i;
      if (r < heapSize && *(array[r]) < *tmp)
        smallest = r;
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

static void updateHeap(std::vector<float*>& array, int i)
{
  float* tmpi = array[i];
  while (i > 0 && *(array[parent_index(i)]) > *tmpi) {
    const int parent = parent_index(i);
    array[i] = array[parent];
    i = parent;
  }
  array[i] = tmpi;
}

// remove the entry with minimum distance from Priority queue
static void removeMin(std::vector<float*>& array)
{
  array[0] = array.back();
  heapify(array);
  array.pop_back();
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

void FastRouteCore::updateCongestionHistory(const int upType,
                                            bool stopDEC,
                                            int& max_adj)
{
  int maxlimit = 0;

  if (upType == 1) {
    for (const auto& [i, j] : h_used_ggrid_) {
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;

      if (overflow > 0) {
        h_edges_[i][j].last_usage += overflow;
        h_edges_[i][j].congCNT++;
      } else {
        if (!stopDEC) {
          h_edges_[i][j].last_usage = h_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, h_edges_[i][j].last_usage);
    }

    for (const auto& [i, j] : v_used_ggrid_) {
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;

      if (overflow > 0) {
        v_edges_[i][j].last_usage += overflow;
        v_edges_[i][j].congCNT++;
      } else {
        if (!stopDEC) {
          v_edges_[i][j].last_usage = v_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, v_edges_[i][j].last_usage);
    }
  } else if (upType == 2) {
    if (max_adj < ahth_) {
      stopDEC = true;
    } else {
      stopDEC = false;
    }
    for (const auto& [i, j] : h_used_ggrid_) {
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;

      if (overflow > 0) {
        h_edges_[i][j].congCNT++;
        h_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          h_edges_[i][j].congCNT--;
          h_edges_[i][j].congCNT = std::max<int>(0, h_edges_[i][j].congCNT);
          h_edges_[i][j].last_usage = h_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, h_edges_[i][j].last_usage);
    }

    for (const auto& [i, j] : v_used_ggrid_) {
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;

      if (overflow > 0) {
        v_edges_[i][j].congCNT++;
        v_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          v_edges_[i][j].congCNT--;
          v_edges_[i][j].congCNT = std::max<int>(0, v_edges_[i][j].congCNT);
          v_edges_[i][j].last_usage = v_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, v_edges_[i][j].last_usage);
    }

  } else if (upType == 3) {
    for (const auto& [i, j] : h_used_ggrid_) {
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;

      if (overflow > 0) {
        h_edges_[i][j].congCNT++;
        h_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          h_edges_[i][j].congCNT--;
          h_edges_[i][j].congCNT = std::max<int>(0, h_edges_[i][j].congCNT);
          h_edges_[i][j].last_usage += overflow;
          h_edges_[i][j].last_usage
              = std::max<int>(h_edges_[i][j].last_usage, 0);
        }
      }
      maxlimit = std::max<int>(maxlimit, h_edges_[i][j].last_usage);
    }

    for (const auto& [i, j] : v_used_ggrid_) {
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;

      if (overflow > 0) {
        v_edges_[i][j].congCNT++;
        v_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          v_edges_[i][j].congCNT--;
          v_edges_[i][j].last_usage += overflow;
          v_edges_[i][j].last_usage
              = std::max<int>(v_edges_[i][j].last_usage, 0);
        }
      }
      maxlimit = std::max<int>(maxlimit, v_edges_[i][j].last_usage);
    }

  } else if (upType == 4) {
    for (const auto& [i, j] : h_used_ggrid_) {
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;

      if (overflow > 0) {
        h_edges_[i][j].congCNT++;
        h_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          h_edges_[i][j].congCNT--;
          h_edges_[i][j].congCNT = std::max<int>(0, h_edges_[i][j].congCNT);
          h_edges_[i][j].last_usage = h_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, h_edges_[i][j].last_usage);
    }

    for (const auto& [i, j] : v_used_ggrid_) {
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;

      if (overflow > 0) {
        v_edges_[i][j].congCNT++;
        v_edges_[i][j].last_usage += overflow;
      } else {
        if (!stopDEC) {
          v_edges_[i][j].congCNT--;
          v_edges_[i][j].congCNT = std::max<int>(0, v_edges_[i][j].congCNT);
          v_edges_[i][j].last_usage = v_edges_[i][j].last_usage * 0.9;
        }
      }
      maxlimit = std::max<int>(maxlimit, v_edges_[i][j].last_usage);
    }
  }

  max_adj = maxlimit;
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
                              std::vector<float*>& src_heap,
                              std::vector<float*>& dest_heap,
                              multi_array<float, 2>& d1,
                              multi_array<float, 2>& d2,
                              const int regionX1,
                              const int regionX2,
                              const int regionY1,
                              const int regionY2)
{
  for (int i = regionY1; i <= regionY2; i++) {
    for (int j = regionX1; j <= regionX2; j++)
      in_region_[i][j] = true;
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
            const int x_grid = route->gridsX[j];
            const int y_grid = route->gridsY[j];

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
    }    // while queue is not empty

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
            const int x_grid = route->gridsX[j];
            const int y_grid = route->gridsY[j];
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
    }    // while queue is not empty
  }      // net with more than two pins

  for (int i = regionY1; i <= regionY2; i++) {
    for (int j = regionX1; j <= regionX2; j++)
      in_region_[i][j] = false;
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
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        cnt++;
      }
    }     // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2.resize(1);
      gridsY_n1n2.resize(1);
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      cnt++;
    }
  }     // if n1 is the first node of (n1, n2)
  else  // n2 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
      gridsX_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      gridsY_n1n2.resize(treeedges[edge_n1n2].route.routelen + 1);
      for (int i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        cnt++;
      }
    }     // MazeRoute
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
    if (verbose_)
      logger_->error(
          GRT,
          169,
          "Net {}: Invalid index for position ({}, {}). Net degree: {}.",
          nets_[net_id]->getName(),
          x_pos,
          y_pos,
          nets_[net_id]->getNumPins());
    return false;
  }

  // reallocate memory for route.gridsX and route.gridsY
  if (treeedges[edge_n1A1].route.type
      == RouteType::MazeRoute)  // if originally allocated, free them first
  {
    treeedges[edge_n1A1].route.gridsX.clear();
    treeedges[edge_n1A1].route.gridsY.clear();
  }
  treeedges[edge_n1A1].route.gridsX.resize(E1_pos + 1, 0);
  treeedges[edge_n1A1].route.gridsY.resize(E1_pos + 1, 0);

  if (A1x <= E1x) {
    int cnt = 0;
    for (int i = 0; i <= E1_pos; i++) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = A1;
    treeedges[edge_n1A1].n2 = n1;
  } else {
    int cnt = 0;
    for (int i = E1_pos; i >= 0; i--) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
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
    treeedges[edge_n1A2].route.gridsX.clear();
    treeedges[edge_n1A2].route.gridsY.clear();
  }
  treeedges[edge_n1A2].route.gridsX.resize(cnt_n1A1 + cnt_n1A2 - E1_pos - 1, 0);
  treeedges[edge_n1A2].route.gridsY.resize(cnt_n1A1 + cnt_n1A2 - E1_pos - 1, 0);

  int cnt = 0;
  if (E1x <= A2x) {
    for (int i = E1_pos; i < cnt_n1A1; i++) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
      cnt++;
    }
    for (int i = 1; i < cnt_n1A2; i++)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = n1;
    treeedges[edge_n1A2].n2 = A2;
  } else {
    for (int i = cnt_n1A2 - 1; i >= 1; i--)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      cnt++;
    }
    for (int i = cnt_n1A1 - 1; i >= E1_pos; i--) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
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
  // allocate memory for gridsX[] and gridsY[] of edge_A1A2
  if (treeedges[edge_A1A2].route.type == RouteType::MazeRoute) {
    treeedges[edge_A1A2].route.gridsX.clear();
    treeedges[edge_A1A2].route.gridsY.clear();
  }
  const int len_A1A2 = cnt_n1A1 + cnt_n1A2 - 1;

  treeedges[edge_A1A2].route.gridsX.resize(len_A1A2, 0);
  treeedges[edge_A1A2].route.gridsY.resize(len_A1A2, 0);
  treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
  treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);

  int cnt = 0;
  for (int i = 0; i < cnt_n1A1; i++) {
    treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A1[i];
    treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A1[i];
    cnt++;
  }
  for (int i = 1; i < cnt_n1A2; i++)  // do not repeat point n1
  {
    treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[i];
    treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[i];
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

  // allocate memory for gridsX[] and gridsY[] of edge_n1C1 and edge_n1C2
  if (treeedges[edge_n1C1].route.type == RouteType::MazeRoute) {
    treeedges[edge_n1C1].route.gridsX.clear();
    treeedges[edge_n1C1].route.gridsY.clear();
  }
  const int len_n1C1 = E1_pos + 1;
  treeedges[edge_n1C1].route.gridsX.resize(len_n1C1, 0);
  treeedges[edge_n1C1].route.gridsY.resize(len_n1C1, 0);
  treeedges[edge_n1C1].route.routelen = len_n1C1 - 1;
  treeedges[edge_n1C1].len = abs(C1x - E1x) + abs(C1y - E1y);

  if (treeedges[edge_n1C2].route.type == RouteType::MazeRoute) {
    treeedges[edge_n1C2].route.gridsX.clear();
    treeedges[edge_n1C2].route.gridsY.clear();
  }
  const int len_n1C2 = cnt_C1C2 - E1_pos;
  treeedges[edge_n1C2].route.gridsX.resize(len_n1C2, 0);
  treeedges[edge_n1C2].route.gridsY.resize(len_n1C2, 0);
  treeedges[edge_n1C2].route.routelen = len_n1C2 - 1;
  treeedges[edge_n1C2].len = abs(C2x - E1x) + abs(C2y - E1y);

  // split original (C1, C2) to (C1, n1) and (n1, C2)
  cnt = 0;
  for (int i = 0; i <= E1_pos; i++) {
    treeedges[edge_n1C1].route.gridsX[i] = gridsX_C1C2[i];
    treeedges[edge_n1C1].route.gridsY[i] = gridsY_C1C2[i];
    cnt++;
  }

  cnt = 0;
  for (int i = E1_pos; i < cnt_C1C2; i++) {
    treeedges[edge_n1C2].route.gridsX[cnt] = gridsX_C1C2[i];
    treeedges[edge_n1C2].route.gridsY[cnt] = gridsY_C1C2[i];
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
      treeedge->route.gridsX.clear();
      treeedge->route.gridsY.clear();
    }
  }
  sttrees_[netID].nodes.clear();
  sttrees_[netID].edges.clear();

  Tree rsmt;
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

float getCost(const int i,
              const float logis_cof,
              const float cost_height,
              const int slope,
              const int capacity,
              const int cost_type)
{
  float cost;
  if (cost_type == 2) {
    if (i < capacity - 1)
      cost = cost_height / (std::exp((capacity - i - 1) * logis_cof) + 1) + 1;
    else
      cost = cost_height / (std::exp((capacity - i - 1) * logis_cof) + 1) + 1
             + cost_height / slope * (i - capacity);
  } else {
    if (i < capacity)
      cost = cost_height / (std::exp((capacity - i) * logis_cof) + 1) + 1;
    else
      cost = cost_height / (std::exp((capacity - i) * logis_cof) + 1) + 1
             + cost_height / slope * (i - capacity);
  }
  return cost;
}

void FastRouteCore::mazeRouteMSMD(const int iter,
                                  const int expand,
                                  const float cost_height,
                                  const int ripup_threshold,
                                  const int maze_edge_threshold,
                                  const bool ordering,
                                  const int cost_type,
                                  const float logis_cof,
                                  const int via,
                                  const int slope,
                                  const int L,
                                  float& slack_th)
{
  // maze routing for multi-source, multi-destination
  int tmpX, tmpY;

  const int max_usage_multiplier = 40;

  // allocate memory for distance and parent and pop_heap
  h_cost_table_.resize(max_usage_multiplier * h_capacity_);
  v_cost_table_.resize(max_usage_multiplier * v_capacity_);

  for (int i = 0; i < max_usage_multiplier * h_capacity_; i++) {
    h_cost_table_[i]
        = getCost(i, logis_cof, cost_height, slope, h_capacity_, cost_type);
  }
  for (int i = 0; i < max_usage_multiplier * v_capacity_; i++) {
    v_cost_table_[i]
        = getCost(i, logis_cof, cost_height, slope, v_capacity_, cost_type);
  }

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++)
      in_region_[i][j] = false;
  }

  if (ordering) {
    if (critical_nets_percentage_) {
      slack_th = CalculatePartialSlack();
    }
    StNetOrder();
  }

  std::vector<float*> src_heap;
  std::vector<float*> dest_heap;
  src_heap.reserve(y_grid_ * x_grid_);
  dest_heap.reserve(y_grid_ * x_grid_);

  multi_array<float, 2> d1(boost::extents[y_range_][x_range_]);
  multi_array<float, 2> d2(boost::extents[y_range_][x_range_]);

  std::vector<bool> pop_heap2(y_grid_ * x_range_, false);

  for (int nidRPC = 0; nidRPC < net_ids_.size(); nidRPC++) {
    const int netID
        = ordering ? tree_order_cong_[nidRPC].treeIndex : net_ids_[nidRPC];

    const int num_terminals = sttrees_[netID].num_terminals;

    const int origENG = expand;

    netedgeOrderDec(netID);

    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;
    // loop for all the tree edges
    const int num_edges = sttrees_[netID].num_edges();
    for (int edgeREC = 0; edgeREC < num_edges; edgeREC++) {
      const int edgeID = net_eo_[edgeREC].edgeID;
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
      const int ymin = std::min(n1y, n2y);
      const int ymax = std::max(n1y, n2y);

      const int xmin = std::min(n1x, n2x);
      const int xmax = std::max(n1x, n2x);

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
      for (int i = 0; i < dest_heap.size(); i++)
        pop_heap2[(dest_heap[i] - &d2[0][0])] = true;

      // stop when the grid position been popped out from both src_heap and
      // dest_heap
      while (pop_heap2[ind1] == false) {
        // relax all the adjacent grids within the enlarged region for
        // source subtree
        const int curX = ind1 % x_range_;
        const int curY = ind1 / x_range_;
        int preX, preY;
        if (d1[curY][curX] != 0) {
          if (hv_[curY][curX]) {
            preX = parent_x1_[curY][curX];
            preY = parent_y1_[curY][curX];
          } else {
            preX = parent_x3_[curY][curX];
            preY = parent_y3_[curY][curX];
          }
        } else {
          preX = curX;
          preY = curY;
        }

        removeMin(src_heap);

        // left
        if (curX > regionX1) {
          float tmp, cost1, cost2;
          const int pos1 = h_edges_[curY][curX - 1].usage_red()
                           + L * h_edges_[curY][(curX - 1)].last_usage;

          if (pos1 < h_cost_table_.size())
            cost1 = h_cost_table_.at(pos1);
          else
            cost1 = getCost(
                pos1, logis_cof, cost_height, slope, h_capacity_, cost_type);

          if ((preY == curY) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX] + cost1;
          } else {
            if (curX < regionX2 - 1) {
              const int pos2 = h_edges_[curY][curX].usage_red()
                               + L * h_edges_[curY][curX].last_usage;

              if (pos2 < h_cost_table_.size())
                cost2 = h_cost_table_.at(pos2);
              else
                cost2 = getCost(pos2,
                                logis_cof,
                                cost_height,
                                slope,
                                h_capacity_,
                                cost_type);

              const int tmp_cost = d1[curY][curX + 1] + cost2;

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_h_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via + cost1;
          }
          tmpX = curX - 1;  // the left neighbor

          if (d1[curY][tmpX]
              >= BIG_INT)  // left neighbor not been put into src_heap
          {
            d1[curY][tmpX] = tmp;
            parent_x3_[curY][tmpX] = curX;
            parent_y3_[curY][tmpX] = curY;
            hv_[curY][tmpX] = false;
            src_heap.push_back(&d1[curY][tmpX]);
            updateHeap(src_heap, src_heap.size() - 1);
          } else if (d1[curY][tmpX] > tmp)  // left neighbor been put into
                                            // src_heap but needs update
          {
            d1[curY][tmpX] = tmp;
            parent_x3_[curY][tmpX] = curX;
            parent_y3_[curY][tmpX] = curY;
            hv_[curY][tmpX] = false;
            float* dtmp = &d1[curY][tmpX];
            int ind = 0;
            while (src_heap[ind] != dtmp)
              ind++;
            updateHeap(src_heap, ind);
          }
        }
        // right
        if (curX < regionX2) {
          float tmp, cost1, cost2;
          const int pos1 = h_edges_[curY][curX].usage_red()
                           + L * h_edges_[curY][curX].last_usage;

          if (pos1 < h_cost_table_.size())
            cost1 = h_cost_table_.at(pos1);
          else
            cost1 = getCost(
                pos1, logis_cof, cost_height, slope, h_capacity_, cost_type);

          if ((preY == curY) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX] + cost1;
          } else {
            if (curX > regionX1 + 1) {
              const int pos2 = h_edges_[curY][curX - 1].usage_red()
                               + L * h_edges_[curY][curX - 1].last_usage;

              if (pos2 < h_cost_table_.size())
                cost2 = h_cost_table_.at(pos2);
              else
                cost2 = getCost(pos2,
                                logis_cof,
                                cost_height,
                                slope,
                                h_capacity_,
                                cost_type);
              const int tmp_cost = d1[curY][curX - 1] + cost2;

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_h_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via + cost1;
          }
          tmpX = curX + 1;  // the right neighbor

          if (d1[curY][tmpX]
              >= BIG_INT)  // right neighbor not been put into src_heap
          {
            d1[curY][tmpX] = tmp;
            parent_x3_[curY][tmpX] = curX;
            parent_y3_[curY][tmpX] = curY;
            hv_[curY][tmpX] = false;
            src_heap.push_back(&d1[curY][tmpX]);
            updateHeap(src_heap, src_heap.size() - 1);
          } else if (d1[curY][tmpX] > tmp)  // right neighbor been put into
                                            // src_heap but needs update
          {
            d1[curY][tmpX] = tmp;
            parent_x3_[curY][tmpX] = curX;
            parent_y3_[curY][tmpX] = curY;
            hv_[curY][tmpX] = false;
            float* dtmp = &d1[curY][tmpX];
            int ind = 0;
            while (src_heap[ind] != dtmp)
              ind++;
            updateHeap(src_heap, ind);
          }
        }
        // bottom
        if (curY > regionY1) {
          float tmp, cost1, cost2;
          const int pos1 = v_edges_[curY - 1][curX].usage_red()
                           + L * v_edges_[curY - 1][curX].last_usage;

          if (pos1 < v_cost_table_.size())
            cost1 = v_cost_table_.at(pos1);
          else
            cost1 = getCost(
                pos1, logis_cof, cost_height, slope, v_capacity_, cost_type);

          if ((preX == curX) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX] + cost1;
          } else {
            if (curY < regionY2 - 1) {
              const int pos2 = v_edges_[curY][curX].usage_red()
                               + L * v_edges_[curY][curX].last_usage;

              if (pos2 < v_cost_table_.size())
                cost2 = v_cost_table_.at(pos2);
              else
                cost2 = getCost(pos2,
                                logis_cof,
                                cost_height,
                                slope,
                                v_capacity_,
                                cost_type);
              const int tmp_cost = d1[curY + 1][curX] + cost2;

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_v_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via + cost1;
          }
          tmpY = curY - 1;  // the bottom neighbor
          if (d1[tmpY][curX]
              >= BIG_INT)  // bottom neighbor not been put into src_heap
          {
            d1[tmpY][curX] = tmp;
            parent_x1_[tmpY][curX] = curX;
            parent_y1_[tmpY][curX] = curY;
            hv_[tmpY][curX] = true;
            src_heap.push_back(&d1[tmpY][curX]);
            updateHeap(src_heap, src_heap.size() - 1);
          } else if (d1[tmpY][curX] > tmp)  // bottom neighbor been put into
                                            // src_heap but needs update
          {
            d1[tmpY][curX] = tmp;
            parent_x1_[tmpY][curX] = curX;
            parent_y1_[tmpY][curX] = curY;
            hv_[tmpY][curX] = true;
            float* dtmp = &d1[tmpY][curX];
            int ind = 0;
            while (src_heap[ind] != dtmp)
              ind++;
            updateHeap(src_heap, ind);
          }
        }
        // top
        if (curY < regionY2) {
          float tmp, cost1, cost2;
          const int pos1 = v_edges_[curY][curX].usage_red()
                           + L * v_edges_[curY][curX].last_usage;

          if (pos1 < v_cost_table_.size())
            cost1 = v_cost_table_.at(pos1);
          else
            cost1 = getCost(
                pos1, logis_cof, cost_height, slope, v_capacity_, cost_type);

          if ((preX == curX) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX] + cost1;
          } else {
            if (curY > regionY1 + 1) {
              const int pos2 = v_edges_[curY - 1][curX].usage_red()
                               + L * v_edges_[curY - 1][curX].last_usage;

              if (pos2 < v_cost_table_.size())
                cost2 = v_cost_table_.at(pos2);
              else
                cost2 = getCost(pos2,
                                logis_cof,
                                cost_height,
                                slope,
                                v_capacity_,
                                cost_type);

              const int tmp_cost = d1[curY - 1][curX] + cost2;

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_v_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via + cost1;
          }
          tmpY = curY + 1;  // the top neighbor
          if (d1[tmpY][curX]
              >= BIG_INT)  // top neighbor not been put into src_heap
          {
            d1[tmpY][curX] = tmp;
            parent_x1_[tmpY][curX] = curX;
            parent_y1_[tmpY][curX] = curY;
            hv_[tmpY][curX] = true;
            src_heap.push_back(&d1[tmpY][curX]);
            updateHeap(src_heap, src_heap.size() - 1);
          } else if (d1[tmpY][curX] > tmp)  // top neighbor been put into
                                            // src_heap but needs update
          {
            d1[tmpY][curX] = tmp;
            parent_x1_[tmpY][curX] = curX;
            parent_y1_[tmpY][curX] = curY;
            hv_[tmpY][curX] = true;
            float* dtmp = &d1[tmpY][curX];
            int ind = 0;
            while (src_heap[ind] != dtmp)
              ind++;
            updateHeap(src_heap, ind);
          }
        }

        // update ind1 for next loop
        ind1 = (src_heap[0] - &d1[0][0]);

      }  // while loop

      for (int i = 0; i < dest_heap.size(); i++)
        pop_heap2[(dest_heap[i] - &d2[0][0])] = false;

      const int crossX = ind1 % x_range_;
      const int crossY = ind1 / x_range_;

      int cnt = 0;
      int curX = crossX;
      int curY = crossY;
      std::vector<int> tmp_gridsX, tmp_gridsY;
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
        tmp_gridsX.push_back(curX);
        tmp_gridsY.push_back(curY);
        cnt++;
      }
      // reverse the grids on the path
      std::vector<int> gridsX(tmp_gridsX.rbegin(), tmp_gridsX.rend());
      std::vector<int> gridsY(tmp_gridsY.rbegin(), tmp_gridsY.rend());

      // add the connection point (crossX, crossY)
      gridsX.push_back(crossX);
      gridsY.push_back(crossY);
      cnt++;

      curX = crossX;
      curY = crossY;
      const int cnt_n1n2 = cnt;

      // change the tree structure according to the new routing for the tree
      // edge find E1 and E2, and the endpoints of the edges they are on
      const int E1x = gridsX[0];
      const int E1y = gridsY[0];
      const int E2x = gridsX.back();
      const int E2y = gridsY.back();

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
            if (verbose_)
              logger_->error(GRT,
                             150,
                             "Net {} has errors during updateRouteType1.",
                             nets_[netID]->getName());
            reInitTree(netID);
            nidRPC--;
            break;
          }
          // update position for n1
          treenodes[n1].x = E1x;
          treenodes[n1].y = E1y;
        }     // if E1 is on (n1, A1) or (n1, A2)
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
      }    // n1 is not a pin and E1!=n1

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
        }     // if E2 is on (n2, B1) or (n2, B2)
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
      }    // n2 is not a pin and E2!=n2

      // update route for edge (n1, n2) and edge usage
      if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
        treeedges[edge_n1n2].route.gridsX.clear();
        treeedges[edge_n1n2].route.gridsY.clear();
      }
      treeedges[edge_n1n2].route.gridsX.resize(cnt_n1n2, 0);
      treeedges[edge_n1n2].route.gridsY.resize(cnt_n1n2, 0);
      treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
      treeedges[edge_n1n2].route.routelen = cnt_n1n2 - 1;
      treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y - E2y);

      for (int i = 0; i < cnt_n1n2; i++) {
        treeedges[edge_n1n2].route.gridsX[i] = gridsX[i];
        treeedges[edge_n1n2].route.gridsY[i] = gridsY[i];
      }

      int edgeCost = nets_[netID]->getEdgeCost();

      // update edge usage
      for (int i = 0; i < cnt_n1n2 - 1; i++) {
        if (gridsX[i] == gridsX[i + 1])  // a vertical edge
        {
          const int min_y = std::min(gridsY[i], gridsY[i + 1]);
          v_edges_[min_y][gridsX[i]].usage += edgeCost;
          v_used_ggrid_.insert(std::make_pair(min_y, gridsX[i]));
        } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
        {
          const int min_x = std::min(gridsX[i], gridsX[i + 1]);
          h_edges_[gridsY[i]][min_x].usage += edgeCost;
          h_used_ggrid_.insert(std::make_pair(gridsY[i], min_x));
        }
      }
    }  // loop edgeID
  }    // loop netID

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
        const std::vector<int16_t>& gridsX = treeedge->route.gridsX;
        const std::vector<int16_t>& gridsY = treeedge->route.gridsY;
        int lastX = tile_size_ * (gridsX[0] + 0.5) + x_corner_;
        int lastY = tile_size_ * (gridsY[0] + 0.5) + y_corner_;

        for (int i = 1; i <= routeLen; i++) {
          const int xreal = tile_size_ * (gridsX[i] + 0.5) + x_corner_;
          const int yreal = tile_size_ * (gridsY[i] + 0.5) + y_corner_;

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
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;
      if (overflow > 0) {
        const int xreal = tile_size_ * (j + 0.5) + x_corner_;
        const int yreal = tile_size_ * (i + 0.5) + y_corner_;
        const int usage = h_edges_[i][j].usage;
        const int capacity = h_edges_[i][j].cap;
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
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;
      if (overflow > 0) {
        const int xreal = tile_size_ * (j + 0.5) + x_corner_;
        const int yreal = tile_size_ * (i + 0.5) + y_corner_;
        const int usage = v_edges_[i][j].usage;
        const int capacity = v_edges_[i][j].cap;
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

void FastRouteCore::setCongestionNets(std::set<odb::dbNet*>& congestion_nets,
                                      int& posX,
                                      int& posY,
                                      int dir,
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
          if (abs(ymin - posY) <= radius && abs(gridsX[i] - posX) <= radius
              && dir == 0) {
            congestion_nets.insert(nets_[netID]->getDbNet());
          }
        } else if (gridsY[i] == gridsY[i + 1]) {  // a horizontal edge
          const int xmin = std::min(gridsX[i], gridsX[i + 1]);
          if (abs(gridsY[i] - posY) <= radius && abs(xmin - posX) <= radius
              && dir == 1) {
            congestion_nets.insert(nets_[netID]->getDbNet());
          }
        }
      }
    }
  }
}

// The function will add the new nets to the congestion_nets set
void FastRouteCore::getCongestionNets(std::set<odb::dbNet*>& congestion_nets)
{
  std::vector<int> xs, ys, dirs;
  int n = 0;
  // Find horizontal ggrids with congestion
  for (const auto& [i, j] : h_used_ggrid_) {
    const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;
    if (overflow > 0) {
      xs.push_back(j);
      ys.push_back(i);
      dirs.push_back(1);
      n++;
    }
  }
  // Find vertical ggrids with congestion
  for (const auto& [i, j] : v_used_ggrid_) {
    const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;
    if (overflow > 0) {
      xs.push_back(j);
      ys.push_back(i);
      dirs.push_back(0);
      n++;
    }
  }

  int old_size = congestion_nets.size();

  // The radius around the congested zone is increased when no new nets are
  // obtained
  for (int radius = 0; radius < 5 && old_size == congestion_nets.size();
       radius++) {
    // Find nets for each congestion ggrid
    for (int i = 0; i < n; i++) {
      setCongestionNets(congestion_nets, xs[i], ys[i], dirs[i], radius);
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
  for (const auto& [i, j] : h_used_ggrid_) {
    total_usage += h_edges_[i][j].usage;
    const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;
    if (overflow > 0) {
      H_overflow += overflow;
      max_H_overflow = std::max(max_H_overflow, overflow);
      numedges++;
    }
  }

  for (const auto& [i, j] : v_used_ggrid_) {
    total_usage += v_edges_[i][j].usage;
    const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;
    if (overflow > 0) {
      V_overflow += overflow;
      max_V_overflow = std::max(max_V_overflow, overflow);
      numedges++;
    }
  }

  int max_overflow = std::max(max_H_overflow, max_V_overflow);
  total_overflow_ = H_overflow + V_overflow;
  *maxOverflow = max_overflow;

  if (logger_->debugCheck(GRT, "congestion", 1)) {
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

  for (const auto& [i, j] : h_used_ggrid_) {
    total_usage += h_edges_[i][j].est_usage;
    const int overflow = h_edges_[i][j].est_usage - h_edges_[i][j].cap;
    hCap += h_edges_[i][j].cap;
    if (overflow > 0) {
      H_overflow += overflow;
      max_H_overflow = std::max(max_H_overflow, overflow);
      numedges++;
    }
  }

  for (const auto& [i, j] : v_used_ggrid_) {
    total_usage += v_edges_[i][j].est_usage;
    const int overflow = v_edges_[i][j].est_usage - v_edges_[i][j].cap;
    vCap += v_edges_[i][j].cap;
    if (overflow > 0) {
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

  if (logger_->debugCheck(GRT, "checkRoute3D", 1)) {
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
    for (const auto& [i, j] : h_used_ggrid_) {
      total_usage += h_edges_3D_[k][i][j].usage;
      overflow = h_edges_3D_[k][i][j].usage - h_edges_3D_[k][i][j].cap;

      if (overflow > 0) {
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
      }
    }
    for (const auto& [i, j] : v_used_ggrid_) {
      total_usage += v_edges_3D_[k][i][j].usage;
      overflow = v_edges_3D_[k][i][j].usage - v_edges_3D_[k][i][j].cap;
      if (overflow > 0) {
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
      }
    }
  }

  total_overflow_ = H_overflow + V_overflow;

  return total_usage;
}

void FastRouteCore::InitEstUsage()
{
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      h_edges_[i][j].est_usage = 0;
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      v_edges_[i][j].est_usage = 0;
    }
  }
}

void FastRouteCore::str_accu(const int rnd)
{
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;
      if (overflow > 0 || h_edges_[i][j].congCNT > rnd) {
        h_edges_[i][j].last_usage += h_edges_[i][j].congCNT * overflow / 2;
      }
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;
      if (overflow > 0 || v_edges_[i][j].congCNT > rnd) {
        v_edges_[i][j].last_usage += v_edges_[i][j].congCNT * overflow / 2;
      }
    }
  }
}

void FastRouteCore::InitLastUsage(const int upType)
{
  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      h_edges_[i][j].last_usage = 0;
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      v_edges_[i][j].last_usage = 0;
    }
  }

  if (upType == 1) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
        h_edges_[i][j].congCNT = 0;
      }
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
        v_edges_[i][j].congCNT = 0;
      }
    }
  } else if (upType == 2) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
        h_edges_[i][j].last_usage = h_edges_[i][j].last_usage * 0.2;
      }
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
        v_edges_[i][j].last_usage = v_edges_[i][j].last_usage * 0.2;
      }
    }
  }
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
