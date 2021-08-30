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

#include <algorithm>
#include <iomanip>
#include <sstream>

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

void FastRouteCore::convertToMazerouteNet(const int netID)
{
  TreeNode* treenodes = sttrees_[netID].nodes;
  for (int edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
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
  for (int netID = 0; netID < num_valid_nets_; netID++) {
    convertToMazerouteNet(netID);
  }

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      h_edges_[i][j].usage = h_edges_[i][j].est_usage;
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      v_edges_[i][j].usage = v_edges_[i][j].est_usage;
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
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
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
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
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
    }
  } else if (upType == 2) {
    if (max_adj < ahth_) {
      stopDEC = true;
    } else {
      stopDEC = false;
    }
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
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
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
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

  } else if (upType == 3) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
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
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
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
    }

  } else if (upType == 4) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
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
    }

    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
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

  const TreeEdge* treeedges = sttrees_[netID].edges;
  const TreeNode* treenodes = sttrees_[netID].nodes;
  const int degree = sttrees_[netID].deg;

  const int n1 = treeedges[edgeID].n1;
  const int n2 = treeedges[edgeID].n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  src_heap.clear();
  dest_heap.clear();

  if (degree == 2)  // 2-pin net
  {
    d1[y1][x1] = 0;
    src_heap.push_back(&d1[y1][x1]);
    d2[y2][x2] = 0;
    dest_heap.push_back(&d2[y2][x2]);
  } else {  // net with more than 2 pins
    const int numNodes = 2 * degree - 2;

    std::vector<bool> visited(numNodes, false);
    std::vector<int> queue(numNodes);

    // find all the grids on tree edges in subtree t1 (connecting to n1) and put
    // them into src_heap
    if (n1 < degree) {  // n1 is a Pin node
      // just need to put n1 itself into src_heap
      d1[y1][x1] = 0;
      src_heap.push_back(&d1[y1][x1]);
      visited[n1] = true;
    } else {  // n1 is a Steiner node
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
        if (cur < degree) {  // cur node isn't a Steiner node
          continue;
        }
        for (int i = 0; i < 3; i++) {
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
        }  // loop i (3 neigbors for cur node)
      }    // while queue is not empty
    }      // else n1 is not a Pin node

    // find all the grids on subtree t2 (connect to n2) and put them into
    // dest_heap find all the grids on tree edges in subtree t2 (connecting to
    // n2) and put them into dest_heap
    if (n2 < degree) {  // n2 is a Pin node
      // just need to put n1 itself into src_heap
      d2[y2][x2] = 0;
      dest_heap.push_back(&d2[y2][x2]);
      visited[n2] = true;
    } else {  // n2 is a Steiner node
      int queuehead = 0;
      int queuetail = 0;

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

        if (cur < degree) {  // cur node isn't a Steiner node
          continue;
        }
        for (int i = 0; i < 3; i++) {
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
    }      // else n2 is not a Pin node
  }        // net with more than two pins

  for (int i = regionY1; i <= regionY2; i++) {
    for (int j = regionX1; j <= regionX2; j++)
      in_region_[i][j] = false;
  }
}

int FastRouteCore::copyGrids(const TreeNode* treenodes,
                             const int n1,
                             const int n2,
                             const TreeEdge* treeedges,
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
      for (int i = 0; i <= treeedges[edge_n1n2].route.routelen; i++) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        cnt++;
      }
    }     // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      cnt++;
    }
  }     // if n1 is the first node of (n1, n2)
  else  // n2 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
      for (int i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        cnt++;
      }
    }     // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      cnt++;
    }  // MazeRoute
  }

  return cnt;
}

void FastRouteCore::updateRouteType1(const TreeNode* treenodes,
                                     const int n1,
                                     const int A1,
                                     const int A2,
                                     const int E1x,
                                     const int E1y,
                                     TreeEdge* treeedges,
                                     const int edge_n1A1,
                                     const int edge_n1A2)
{
  std::vector<int> gridsX_n1A1(x_range_ + y_range_);
  std::vector<int> gridsY_n1A1(x_range_ + y_range_);
  std::vector<int> gridsX_n1A2(x_range_ + y_range_);
  std::vector<int> gridsY_n1A2(x_range_ + y_range_);

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
    logger_->error(GRT, 169, "Invalid index for position ({}, {}).", E1x, E1y);
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
}

void FastRouteCore::updateRouteType2(const int net_id,
                                     const TreeNode* treenodes,
                                     const int n1,
                                     const int A1,
                                     const int A2,
                                     const int C1,
                                     const int C2,
                                     const int E1x,
                                     const int E1y,
                                     TreeEdge* treeedges,
                                     const int edge_n1A1,
                                     const int edge_n1A2,
                                     const int edge_C1C2)
{
  std::vector<int> gridsX_n1A1(x_range_ + y_range_);
  std::vector<int> gridsY_n1A1(x_range_ + y_range_);
  std::vector<int> gridsX_n1A2(x_range_ + y_range_);
  std::vector<int> gridsY_n1A2(x_range_ + y_range_);
  std::vector<int> gridsX_C1C2(x_range_ + y_range_);
  std::vector<int> gridsY_C1C2(x_range_ + y_range_);

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
    int x_pos = w_tile_ * (E1x + 0.5) + x_corner_;
    int y_pos = h_tile_ * (E1y + 0.5) + y_corner_;
    logger_->error(GRT,
                   170,
                   "Net {}: Invalid index for position ({}, {}).",
                   netName(nets_[net_id]),
                   net_id,
                   x_pos,
                   y_pos);
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
}

void FastRouteCore::reInitTree(const int netID)
{
  newRipupNet(netID);

  const int deg = sttrees_[netID].deg;
  const int numEdges = 2 * deg - 3;
  for (int edgeID = 0; edgeID < numEdges; edgeID++) {
    TreeEdge* treeedge = &(sttrees_[netID].edges[edgeID]);
    if (treeedge->len > 0) {
      treeedge->route.gridsX.clear();
      treeedge->route.gridsY.clear();
    }
  }
  delete[] sttrees_[netID].nodes;
  delete[] sttrees_[netID].edges;

  Tree rsmt;
  fluteCongest(netID, nets_[netID]->pinX, nets_[netID]->pinY, 2, 1.2, rsmt);

  if (nets_[netID]->deg > 3) {
    edgeShiftNew(rsmt, netID);
  }

  copyStTree(netID, rsmt);
  newrouteLInMaze(netID);
  convertToMazerouteNet(netID);
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
                                  const int L)
{
  // maze routing for multi-source, multi-destination
  int tmpX, tmpY;

  const int max_usage_multiplier = 40;

  // allocate memory for distance and parent and pop_heap
  h_cost_table_.resize(max_usage_multiplier * h_capacity_);
  v_cost_table_.resize(max_usage_multiplier * v_capacity_);

  if (cost_type == 2) {
    for (int i = 0; i < max_usage_multiplier * h_capacity_; i++) {
      if (i < h_capacity_ - 1)
        h_cost_table_[i]
            = cost_height / (exp((float) (h_capacity_ - i - 1) * logis_cof) + 1)
              + 1;
      else
        h_cost_table_[i]
            = cost_height / (exp((float) (h_capacity_ - i - 1) * logis_cof) + 1)
              + 1 + cost_height / slope * (i - h_capacity_);
    }
    for (int i = 0; i < max_usage_multiplier * v_capacity_; i++) {
      if (i < v_capacity_ - 1)
        v_cost_table_[i]
            = cost_height / (exp((float) (v_capacity_ - i - 1) * logis_cof) + 1)
              + 1;
      else
        v_cost_table_[i]
            = cost_height / (exp((float) (v_capacity_ - i - 1) * logis_cof) + 1)
              + 1 + cost_height / slope * (i - v_capacity_);
    }
  } else {
    for (int i = 0; i < max_usage_multiplier * h_capacity_; i++) {
      if (i < h_capacity_)
        h_cost_table_[i]
            = cost_height / (exp((float) (h_capacity_ - i) * logis_cof) + 1)
              + 1;
      else
        h_cost_table_[i]
            = cost_height / (exp((float) (h_capacity_ - i) * logis_cof) + 1) + 1
              + cost_height / slope * (i - h_capacity_);
    }
    for (int i = 0; i < max_usage_multiplier * v_capacity_; i++) {
      if (i < v_capacity_)
        v_cost_table_[i]
            = cost_height / (exp((float) (v_capacity_ - i) * logis_cof) + 1)
              + 1;
      else
        v_cost_table_[i]
            = cost_height / (exp((float) (v_capacity_ - i) * logis_cof) + 1) + 1
              + cost_height / slope * (i - v_capacity_);
    }
  }

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++)
      in_region_[i][j] = false;
  }

  if (ordering) {
    StNetOrder();
  }

  std::vector<float*> src_heap;
  std::vector<float*> dest_heap;
  src_heap.reserve(y_grid_ * x_grid_);
  dest_heap.reserve(y_grid_ * x_grid_);

  multi_array<float, 2> d1(boost::extents[y_range_][x_range_]);
  multi_array<float, 2> d2(boost::extents[y_range_][x_range_]);

  std::vector<bool> pop_heap2(y_grid_ * x_range_, false);

  for (int nidRPC = 0; nidRPC < num_valid_nets_; nidRPC++) {
    const int netID = ordering ? tree_order_cong_[nidRPC].treeIndex : nidRPC;

    const int deg = sttrees_[netID].deg;

    const int origENG = expand;

    netedgeOrderDec(netID);

    TreeEdge* treeedges = sttrees_[netID].edges;
    TreeNode* treenodes = sttrees_[netID].nodes;
    // loop for all the tree edges (2*deg-3)
    const int num_edges = 2 * deg - 3;
    for (int edgeREC = 0; edgeREC < num_edges; edgeREC++) {
      const int edgeID = net_eo_[edgeREC].edgeID;
      TreeEdge* treeedge = &(treeedges[edgeID]);

      const int n1 = treeedge->n1;
      const int n2 = treeedge->n2;
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

      const bool enter = newRipupCheck(
          treeedge, n1x, n1y, n2x, n2y, ripup_threshold, netID, edgeID);

      if (!enter) {
        continue;
      }

      // ripup the routing for the edge
      const int ymin = std::min(n1y, n2y);
      const int ymax = std::max(n1y, n2y);

      const int xmin = std::min(n1x, n2x);
      const int xmax = std::max(n1x, n2x);

      enlarge_ = std::min(origENG, (iter / 6 + 3) * treeedge->route.routelen);
      const int regionX1 = std::max(xmin - enlarge_, 0);
      const int regionX2 = std::min(xmax + enlarge_, x_grid_ - 1);
      const int regionY1 = std::max(ymin - enlarge_, 0);
      const int regionY2 = std::min(ymax + enlarge_, y_grid_ - 1);

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
          float tmp;
          if ((preY == curY) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX]
                  + h_cost_table_[h_edges_[curY][curX - 1].usage
                                  + h_edges_[curY][curX - 1].red
                                  + L * h_edges_[curY][(curX - 1)].last_usage];
          } else {
            if (curX < regionX2 - 1) {
              const int tmp_cost
                  = d1[curY][curX + 1]
                    + h_cost_table_[h_edges_[curY][curX].usage
                                    + h_edges_[curY][curX].red
                                    + L * h_edges_[curY][curX].last_usage];

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_h_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via
                  + h_cost_table_[h_edges_[curY][curX - 1].usage
                                  + h_edges_[curY][curX - 1].red
                                  + L * h_edges_[curY][curX - 1].last_usage];
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
          float tmp;
          if ((preY == curY) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX]
                  + h_cost_table_[h_edges_[curY][curX].usage
                                  + h_edges_[curY][curX].red
                                  + L * h_edges_[curY][curX].last_usage];
          } else {
            if (curX > regionX1 + 1) {
              const int tmp_cost
                  = d1[curY][curX - 1]
                    + h_cost_table_[h_edges_[curY][curX - 1].usage
                                    + h_edges_[curY][curX - 1].red
                                    + L * h_edges_[curY][curX - 1].last_usage];

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_h_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via
                  + h_cost_table_[h_edges_[curY][curX].usage
                                  + h_edges_[curY][curX].red
                                  + L * h_edges_[curY][curX].last_usage];
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
          float tmp;

          if ((preX == curX) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX]
                  + v_cost_table_[v_edges_[curY - 1][curX].usage
                                  + v_edges_[curY - 1][curX].red
                                  + L * v_edges_[curY - 1][curX].last_usage];
          } else {
            if (curY < regionY2 - 1) {
              const int tmp_cost
                  = d1[curY + 1][curX]
                    + v_cost_table_[v_edges_[curY][curX].usage
                                    + v_edges_[curY][curX].red
                                    + L * v_edges_[curY][curX].last_usage];

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_v_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via
                  + v_cost_table_[v_edges_[curY - 1][curX].usage
                                  + v_edges_[curY - 1][curX].red
                                  + L * v_edges_[curY - 1][curX].last_usage];
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
          float tmp;

          if ((preX == curX) || (d1[curY][curX] == 0)) {
            tmp = d1[curY][curX]
                  + v_cost_table_[v_edges_[curY][curX].usage
                                  + v_edges_[curY][curX].red
                                  + L * v_edges_[curY][curX].last_usage];
          } else {
            if (curY > regionY1 + 1) {
              const int tmp_cost
                  = d1[curY - 1][curX]
                    + v_cost_table_[v_edges_[curY - 1][curX].usage
                                    + v_edges_[curY - 1][curX].red
                                    + L * v_edges_[curY - 1][curX].last_usage];

              if (tmp_cost < d1[curY][curX] + via) {
                hyper_v_[curY][curX] = true;
              }
            }
            tmp = d1[curY][curX] + via
                  + v_cost_table_[v_edges_[curY][curX].usage
                                  + v_edges_[curY][curX].red
                                  + L * v_edges_[curY][curX].last_usage];
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
      if (n1 >= deg && (E1x != n1x || E1y != n1y))
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
          updateRouteType1(
              treenodes, n1, A1, A2, E1x, E1y, treeedges, edge_n1A1, edge_n1A2);
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
          updateRouteType2(netID,
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
      if (n2 >= deg && (E2x != n2x || E2y != n2y))
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
          updateRouteType1(
              treenodes, n2, B1, B2, E2x, E2y, treeedges, edge_n2B1, edge_n2B2);

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
          updateRouteType2(netID,
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

      int edgeCost = nets_[netID]->edgeCost;

      // update edge usage
      for (int i = 0; i < cnt_n1n2 - 1; i++) {
        if (gridsX[i] == gridsX[i + 1])  // a vertical edge
        {
          const int min_y = std::min(gridsY[i], gridsY[i + 1]);
          v_edges_[min_y][gridsX[i]].usage += edgeCost;
        } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
        {
          const int min_x = std::min(gridsX[i], gridsX[i + 1]);
          h_edges_[gridsY[i]][min_x].usage += edgeCost;
        }
      }

      if (checkRoute2DTree(netID)) {
        reInitTree(netID);
        return;
      }
    }  // loop edgeID
  }    // loop netID

  h_cost_table_.clear();
  v_cost_table_.clear();
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
  int total_cap = 0;

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      total_usage += h_edges_[i][j].usage;
      const int overflow = h_edges_[i][j].usage - h_edges_[i][j].cap;
      total_cap += h_edges_[i][j].cap;
      if (overflow > 0) {
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
        numedges++;
      }
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      total_usage += v_edges_[i][j].usage;
      const int overflow = v_edges_[i][j].usage - v_edges_[i][j].cap;
      total_cap += v_edges_[i][j].cap;
      if (overflow > 0) {
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
        numedges++;
      }
    }
  }

  int max_overflow = std::max(max_H_overflow, max_V_overflow);
  total_overflow_ = H_overflow + V_overflow;
  *maxOverflow = max_overflow;

  if (verbose_ > 1) {
    logger_->info(GRT, 126, "Overflow report:");
    logger_->info(GRT, 127, "Total usage          : {}", total_usage);
    logger_->info(GRT, 128, "Max H overflow       : {}", max_H_overflow);
    logger_->info(GRT, 129, "Max V overflow       : {}", max_V_overflow);
    logger_->info(GRT, 130, "Max overflow         : {}", max_overflow);
    logger_->info(GRT, 131, "Number overflow edges: {}", numedges);
    logger_->info(GRT, 132, "H   overflow         : {}", H_overflow);
    logger_->info(GRT, 133, "V   overflow         : {}", V_overflow);
    logger_->info(GRT, 134, "Final overflow       : {}\n", total_overflow_);
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
  int total_cap = 0;

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_ - 1; j++) {
      total_usage += h_edges_[i][j].est_usage;
      const int overflow = h_edges_[i][j].est_usage - h_edges_[i][j].cap;
      total_cap += h_edges_[i][j].cap;
      hCap += h_edges_[i][j].cap;
      if (overflow > 0) {
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
        numedges++;
      }
    }
  }

  for (int i = 0; i < y_grid_ - 1; i++) {
    for (int j = 0; j < x_grid_; j++) {
      total_usage += v_edges_[i][j].est_usage;
      const int overflow = v_edges_[i][j].est_usage - v_edges_[i][j].cap;
      total_cap += v_edges_[i][j].cap;
      vCap += v_edges_[i][j].cap;
      if (overflow > 0) {
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
        numedges++;
      }
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

  if (verbose_ > 1) {
    logger_->info(GRT, 135, "Overflow report.");
    logger_->info(GRT, 136, "Total hCap               : {}", hCap);
    logger_->info(GRT, 137, "Total vCap               : {}", vCap);
    logger_->info(GRT, 138, "Total usage              : {}", total_usage);
    logger_->info(GRT, 139, "Max H overflow           : {}", max_H_overflow);
    logger_->info(GRT, 140, "Max V overflow           : {}", max_V_overflow);
    logger_->info(GRT, 141, "Max overflow             : {}", max_overflow);
    logger_->info(GRT, 142, "Number of overflow edges : {}", numedges);
    logger_->info(GRT, 143, "H   overflow             : {}", H_overflow);
    logger_->info(GRT, 144, "V   overflow             : {}", V_overflow);
    logger_->info(GRT, 145, "Final overflow           : {}\n", total_overflow_);
  }

  return total_overflow_;
}

int FastRouteCore::getOverflow3D()
{
  // get overflow
  int overflow = 0;
  int max_overflow = 0;
  int H_overflow = 0;
  int V_overflow = 0;
  int max_H_overflow = 0;
  int max_V_overflow = 0;

  int total_usage = 0;
  int cap = 0;

  for (int k = 0; k < num_layers_; k++) {
    for (int i = 0; i < y_grid_; i++) {
      for (int j = 0; j < x_grid_ - 1; j++) {
        total_usage += h_edges_3D_[k][i][j].usage;
        overflow = h_edges_3D_[k][i][j].usage - h_edges_3D_[k][i][j].cap;
        cap += h_edges_3D_[k][i][j].cap;

        if (overflow > 0) {
          H_overflow += overflow;
          max_H_overflow = std::max(max_H_overflow, overflow);
        }
      }
    }
    for (int i = 0; i < y_grid_ - 1; i++) {
      for (int j = 0; j < x_grid_; j++) {
        total_usage += v_edges_3D_[k][i][j].usage;
        overflow = v_edges_3D_[k][i][j].usage - v_edges_3D_[k][i][j].cap;
        cap += v_edges_3D_[k][i][j].cap;

        if (overflow > 0) {
          V_overflow += overflow;
          max_V_overflow = std::max(max_V_overflow, overflow);
        }
      }
    }
  }

  max_overflow = std::max(max_H_overflow, max_V_overflow);
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

}  // namespace grt
