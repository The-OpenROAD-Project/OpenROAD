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
#include <iomanip>
#include <sstream>

#include <algorithm>

#include "DataType.h"
#include "FastRoute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

#define PARENT(i) (i - 1) / 2
#define LEFT(i) 2 * i + 1
#define RIGHT(i) 2 * i + 2

void FastRouteCore::convertToMazerouteNet(int netID)
{
  short *gridsX, *gridsY;
  int i, edgeID, edgelength;
  int n1, n2, x1, y1, x2, y2;
  int cnt, Zpoint;
  TreeEdge* treeedge;
  TreeNode* treenodes;

  treenodes = sttrees_[netID].nodes;
  for (edgeID = 0; edgeID < 2 * sttrees_[netID].deg - 3; edgeID++) {
    treeedge = &(sttrees_[netID].edges[edgeID]);
    edgelength = treeedge->len;
    n1 = treeedge->n1;
    n2 = treeedge->n2;
    x1 = treenodes[n1].x;
    y1 = treenodes[n1].y;
    x2 = treenodes[n2].x;
    y2 = treenodes[n2].y;
    treeedge->route.gridsX = (short*) calloc((edgelength + 1), sizeof(short));
    treeedge->route.gridsY = (short*) calloc((edgelength + 1), sizeof(short));
    treeedge->route.gridsL = nullptr;
    gridsX = treeedge->route.gridsX;
    gridsY = treeedge->route.gridsY;
    treeedge->len = abs(x1 - x2) + abs(y1 - y2);

    cnt = 0;
    if (treeedge->route.type == RouteType::NoRoute) {
      gridsX[0] = x1;
      gridsY[0] = y1;
      treeedge->route.type = RouteType::MazeRoute;
      treeedge->route.routelen = 0;
      treeedge->len = 0;
      cnt++;
    } else if (treeedge->route.type == RouteType::LRoute) {
      if (treeedge->route.xFirst)  // horizontal first
      {
        for (i = x1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y1;
          cnt++;
        }
        if (y1 <= y2) {
          for (i = y1 + 1; i <= y2; i++) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (i = y1 - 1; i >= y2; i--) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        }
      } else  // vertical first
      {
        if (y1 <= y2) {
          for (i = y1; i <= y2; i++) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (i = y1; i >= y2; i--) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
        }
        for (i = x1 + 1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y2;
          cnt++;
        }
      }
    } else if (treeedge->route.type == RouteType::ZRoute) {
      Zpoint = treeedge->route.Zpoint;
      if (treeedge->route.HVH)  // HVH
      {
        for (i = x1; i < Zpoint; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y1;
          cnt++;
        }
        if (y1 <= y2) {
          for (i = y1; i <= y2; i++) {
            gridsX[cnt] = Zpoint;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (i = y1; i >= y2; i--) {
            gridsX[cnt] = Zpoint;
            gridsY[cnt] = i;
            cnt++;
          }
        }
        for (i = Zpoint + 1; i <= x2; i++) {
          gridsX[cnt] = i;
          gridsY[cnt] = y2;
          cnt++;
        }
      } else  // VHV
      {
        if (y1 <= y2) {
          for (i = y1; i < Zpoint; i++) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
          for (i = x1; i <= x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = Zpoint;
            cnt++;
          }
          for (i = Zpoint + 1; i <= y2; i++) {
            gridsX[cnt] = x2;
            gridsY[cnt] = i;
            cnt++;
          }
        } else {
          for (i = y1; i > Zpoint; i--) {
            gridsX[cnt] = x1;
            gridsY[cnt] = i;
            cnt++;
          }
          for (i = x1; i <= x2; i++) {
            gridsX[cnt] = i;
            gridsY[cnt] = Zpoint;
            cnt++;
          }
          for (i = Zpoint - 1; i >= y2; i--) {
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
  int i, j, grid, netID;

  for (netID = 0; netID < num_valid_nets_; netID++) {
    convertToMazerouteNet(netID);
  }

  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      h_edges_[grid].usage = h_edges_[grid].est_usage;
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      v_edges_[grid].usage = v_edges_[grid].est_usage;
    }
  }

  // check 2D edges for invalid usage values
  check2DEdgesUsage();
}

// non recursive version of heapify
void heapify(float** array, int heapSize, int i)
{
  int l, r, smallest;
  float* tmp;
  bool STOP = false;

  tmp = array[i];
  do {
    l = LEFT(i);
    r = RIGHT(i);

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
      STOP = true;
    }
  } while (!STOP);
}

// build heap for an list of grid
void buildHeap(float** array, int arrayLen)
{
  int i;

  for (i = arrayLen / 2 - 1; i >= 0; i--)
    heapify(array, arrayLen, i);
}

void updateHeap(float** array, int arrayLen, int i)
{
  int parent;
  float* tmpi;

  tmpi = array[i];
  while (i > 0 && *(array[PARENT(i)]) > *tmpi) {
    parent = PARENT(i);
    array[i] = array[parent];
    i = parent;
  }
  array[i] = tmpi;
}

// extract the entry with minimum distance from Priority queue
void extractMin(float** array, int arrayLen)
{
  array[0] = array[arrayLen - 1];
  heapify(array, arrayLen - 1, 0);
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

void FastRouteCore::updateCongestionHistory(int round, int upType, bool stopDEC, int &max_adj)
{
  int i, j, grid, maxlimit, overflow;

  maxlimit = 0;

  if (upType == 1) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        overflow = h_edges_[grid].usage - h_edges_[grid].cap;

        if (overflow > 0) {
          h_edges_[grid].last_usage += overflow;
          h_edges_[grid].congCNT++;
        } else {
          if (!stopDEC) {
            h_edges_[grid].last_usage = h_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, h_edges_[grid].last_usage);
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        overflow = v_edges_[grid].usage - v_edges_[grid].cap;

        if (overflow > 0) {
          v_edges_[grid].last_usage += overflow;
          v_edges_[grid].congCNT++;
        } else {
          if (!stopDEC) {
            v_edges_[grid].last_usage = v_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, v_edges_[grid].last_usage);
      }
    }
  } else if (upType == 2) {
    if (max_adj < ahth_) {
      stopDEC = true;
    } else {
      stopDEC = false;
    }
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        overflow = h_edges_[grid].usage - h_edges_[grid].cap;

        if (overflow > 0) {
          h_edges_[grid].congCNT++;
          h_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            h_edges_[grid].congCNT--;
            h_edges_[grid].congCNT = std::max<int>(0, h_edges_[grid].congCNT);
            h_edges_[grid].last_usage = h_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, h_edges_[grid].last_usage);
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        overflow = v_edges_[grid].usage - v_edges_[grid].cap;

        if (overflow > 0) {
          v_edges_[grid].congCNT++;
          v_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            v_edges_[grid].congCNT--;
            v_edges_[grid].congCNT = std::max<int>(0, v_edges_[grid].congCNT);
            v_edges_[grid].last_usage = v_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, v_edges_[grid].last_usage);
      }
    }

  } else if (upType == 3) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        overflow = h_edges_[grid].usage - h_edges_[grid].cap;

        if (overflow > 0) {
          h_edges_[grid].congCNT++;
          h_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            h_edges_[grid].congCNT--;
            h_edges_[grid].congCNT = std::max<int>(0, h_edges_[grid].congCNT);
            h_edges_[grid].last_usage += overflow;
            h_edges_[grid].last_usage
                = std::max<int>(h_edges_[grid].last_usage, 0);
          }
        }
        maxlimit = std::max<int>(maxlimit, h_edges_[grid].last_usage);
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        overflow = v_edges_[grid].usage - v_edges_[grid].cap;

        if (overflow > 0) {
          v_edges_[grid].congCNT++;
          v_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            v_edges_[grid].congCNT--;
            v_edges_[grid].last_usage += overflow;
            v_edges_[grid].last_usage
                = std::max<int>(v_edges_[grid].last_usage, 0);
          }
        }
        maxlimit = std::max<int>(maxlimit, v_edges_[grid].last_usage);
      }
    }

  } else if (upType == 4) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        overflow = h_edges_[grid].usage - h_edges_[grid].cap;

        if (overflow > 0) {
          h_edges_[grid].congCNT++;
          h_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            h_edges_[grid].congCNT--;
            h_edges_[grid].congCNT = std::max<int>(0, h_edges_[grid].congCNT);
            h_edges_[grid].last_usage = h_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, h_edges_[grid].last_usage);
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        overflow = v_edges_[grid].usage - v_edges_[grid].cap;

        if (overflow > 0) {
          v_edges_[grid].congCNT++;
          v_edges_[grid].last_usage += overflow;
        } else {
          if (!stopDEC) {
            v_edges_[grid].congCNT--;
            v_edges_[grid].congCNT = std::max<int>(0, v_edges_[grid].congCNT);
            v_edges_[grid].last_usage = v_edges_[grid].last_usage * 0.9;
          }
        }
        maxlimit = std::max<int>(maxlimit, v_edges_[grid].last_usage);
      }
    }
  }

  max_adj = maxlimit;
}

// ripup a tree edge according to its ripup type and Z-route it
// put all the nodes in the subtree t1 and t2 into heap1_ and heap2_
// netID   - the ID for the net
// edgeID  - the ID for the tree edge to route
// d1_      - the distance of any grid from the source subtree t1
// d2_      - the distance of any grid from the destination subtree t2
// heap1_   - the heap storing the addresses for d1_[][]
// heap2_   - the heap storing the addresses for d2_[][]
void FastRouteCore::setupHeap(int netID,
               int edgeID,
               int* heapLen1,
               int* heapLen2,
               int regionX1,
               int regionX2,
               int regionY1,
               int regionY2)
{
  int i, j, d, numNodes, n1, n2, x1, y1, x2, y2;
  int nbr, nbrX, nbrY, cur, edge;
  int x_grid, y_grid, heapcnt;
  int queuehead, queuetail;
  std::vector<int> queue;
  std::vector<bool> visited;
  TreeEdge* treeedges;
  TreeNode* treenodes;
  Route* route;

  for (i = regionY1; i <= regionY2; i++) {
    for (j = regionX1; j <= regionX2; j++)
      in_region_[i][j] = true;
  }

  treeedges = sttrees_[netID].edges;
  treenodes = sttrees_[netID].nodes;
  d = sttrees_[netID].deg;

  n1 = treeedges[edgeID].n1;
  n2 = treeedges[edgeID].n2;
  x1 = treenodes[n1].x;
  y1 = treenodes[n1].y;
  x2 = treenodes[n2].x;
  y2 = treenodes[n2].y;

  if (d == 2)  // 2-pin net
  {
    d1_[y1][x1] = 0;
    heap1_[0] = &d1_[y1][x1];
    *heapLen1 = 1;
    d2_[y2][x2] = 0;
    heap2_[0] = &d2_[y2][x2];
    *heapLen2 = 1;
  } else  // net with more than 2 pins
  {
    numNodes = 2 * d - 2;

    visited.resize(numNodes);
    for (i = 0; i < numNodes; i++)
      visited[i] = false;

    queue.resize(numNodes);

    // find all the grids on tree edges in subtree t1 (connecting to n1) and put
    // them into heap1_
    if (n1 < d)  // n1 is a Pin node
    {
      // just need to put n1 itself into heap1_
      d1_[y1][x1] = 0;
      heap1_[0] = &d1_[y1][x1];
      visited[n1] = true;
      *heapLen1 = 1;
    } else  // n1 is a Steiner node
    {
      heapcnt = 0;
      queuehead = queuetail = 0;

      // add n1 into heap1_
      d1_[y1][x1] = 0;
      heap1_[0] = &d1_[y1][x1];
      visited[n1] = true;
      heapcnt++;

      // add n1 into the queue
      queue[queuetail] = n1;
      queuetail++;

      // loop to find all the edges in subtree t1
      while (queuetail > queuehead) {
        // get cur node from the queuehead
        cur = queue[queuehead];
        queuehead++;
        visited[cur] = true;
        if (cur >= d)  // cur node is a Steiner node
        {
          for (i = 0; i < 3; i++) {
            nbr = treenodes[cur].nbr[i];
            edge = treenodes[cur].edge[i];
            if (nbr != n2)  // not n2
            {
              if (visited[nbr] == false) {
                // put all the grids on the two adjacent tree edges into heap1_
                if (treeedges[edge].route.routelen > 0)  // not a degraded edge
                {
                  // put nbr into heap1_ if in enlarged region
                  if (in_region_[treenodes[nbr].y][treenodes[nbr].x]) {
                    nbrX = treenodes[nbr].x;
                    nbrY = treenodes[nbr].y;
                    d1_[nbrY][nbrX] = 0;
                    heap1_[heapcnt] = &d1_[nbrY][nbrX];
                    heapcnt++;
                    corr_edge_[nbrY][nbrX] = edge;
                  }

                  // the coordinates of two end nodes of the edge

                  route = &(treeedges[edge].route);
                  if (route->type == RouteType::MazeRoute) {
                    for (j = 1; j < route->routelen;
                         j++)  // don't put edge_n1 and edge_n2 into heap1_
                    {
                      x_grid = route->gridsX[j];
                      y_grid = route->gridsY[j];

                      if (in_region_[y_grid][x_grid]) {
                        d1_[y_grid][x_grid] = 0;
                        heap1_[heapcnt] = &d1_[y_grid][x_grid];
                        heapcnt++;
                        corr_edge_[y_grid][x_grid] = edge;
                      }
                    }
                  }  // if MazeRoute
                  else {
                    logger_->error(GRT, 125, "Setup heap: not maze routing.");
                  }
                }  // if not a degraded edge (len>0)

                // add the neighbor of cur node into queue
                queue[queuetail] = nbr;
                queuetail++;
              }             // if the node is not visited
            }               // if nbr!=n2
          }                 // loop i (3 neigbors for cur node)
        }                   // if cur node is a Steiner nodes
      }                     // while queue is not empty
      *heapLen1 = heapcnt;  // record the length of heap1_
    }                       // else n1 is not a Pin node

    // find all the grids on subtree t2 (connect to n2) and put them into heap2_
    // find all the grids on tree edges in subtree t2 (connecting to n2) and put
    // them into heap2_
    if (n2 < d)  // n2 is a Pin node
    {
      // just need to put n1 itself into heap1_
      d2_[y2][x2] = 0;
      heap2_[0] = &d2_[y2][x2];
      visited[n2] = true;
      *heapLen2 = 1;
    } else  // n2 is a Steiner node
    {
      heapcnt = 0;
      queuehead = queuetail = 0;

      // add n2 into heap2_
      d2_[y2][x2] = 0;
      heap2_[0] = &d2_[y2][x2];
      visited[n2] = true;
      heapcnt++;

      // add n2 into the queue
      queue[queuetail] = n2;
      queuetail++;

      // loop to find all the edges in subtree t2
      while (queuetail > queuehead) {
        // get cur node form queuehead
        cur = queue[queuehead];
        visited[cur] = true;
        queuehead++;

        if (cur >= d)  // cur node is a Steiner node
        {
          for (i = 0; i < 3; i++) {
            nbr = treenodes[cur].nbr[i];
            edge = treenodes[cur].edge[i];
            if (nbr != n1)  // not n1
            {
              if (visited[nbr] == false) {
                // put all the grids on the two adjacent tree edges into heap2_
                if (treeedges[edge].route.routelen > 0)  // not a degraded edge
                {
                  // put nbr into heap2_
                  if (in_region_[treenodes[nbr].y][treenodes[nbr].x]) {
                    nbrX = treenodes[nbr].x;
                    nbrY = treenodes[nbr].y;
                    d2_[nbrY][nbrX] = 0;
                    heap2_[heapcnt] = &d2_[nbrY][nbrX];
                    heapcnt++;
                    corr_edge_[nbrY][nbrX] = edge;
                  }

                  // the coordinates of two end nodes of the edge

                  route = &(treeedges[edge].route);
                  if (route->type == RouteType::MazeRoute) {
                    for (j = 1; j < route->routelen;
                         j++)  // don't put edge_n1 and edge_n2 into heap2_
                    {
                      x_grid = route->gridsX[j];
                      y_grid = route->gridsY[j];
                      if (in_region_[y_grid][x_grid]) {
                        d2_[y_grid][x_grid] = 0;
                        heap2_[heapcnt] = &d2_[y_grid][x_grid];
                        heapcnt++;
                        corr_edge_[y_grid][x_grid] = edge;
                      }
                    }
                  }  // if MazeRoute
                  else {
                    logger_->error(GRT, 201, "Setup heap: not maze routing.");
                  }
                }  // if the edge is not degraded (len>0)

                // add the neighbor of cur node into queue
                queue[queuetail] = nbr;
                queuetail++;
              }             // if the node is not visited
            }               // if nbr!=n1
          }                 // loop i (3 neigbors for cur node)
        }                   // if cur node is a Steiner nodes
      }                     // while queue is not empty
      *heapLen2 = heapcnt;  // record the length of heap2_
    }                       // else n2 is not a Pin node
  }  // net with more than two pins

  for (i = regionY1; i <= regionY2; i++) {
    for (j = regionX1; j <= regionX2; j++)
      in_region_[i][j] = false;
  }
}

int FastRouteCore::copyGrids(TreeNode* treenodes,
              int n1,
              int n2,
              TreeEdge* treeedges,
              int edge_n1n2,
              std::vector<int>& gridsX_n1n2,
              std::vector<int>& gridsY_n1n2)
{
  int i, cnt;
  int n1x, n1y;

  n1x = treenodes[n1].x;
  n1y = treenodes[n1].y;

  cnt = 0;
  if (treeedges[edge_n1n2].n1 == n1)  // n1 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
      for (i = 0; i <= treeedges[edge_n1n2].route.routelen; i++) {
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
      for (i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
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

void FastRouteCore::updateRouteType1(TreeNode* treenodes,
                      int n1,
                      int A1,
                      int A2,
                      int E1x,
                      int E1y,
                      TreeEdge* treeedges,
                      int edge_n1A1,
                      int edge_n1A2)
{
  int i, cnt, A1x, A1y, A2x, A2y;
  int cnt_n1A1, cnt_n1A2, E1_pos;
  std::vector<int> gridsX_n1A1(x_range_ + y_range_);
  std::vector<int> gridsY_n1A1(x_range_ + y_range_);
  std::vector<int> gridsX_n1A2(x_range_ + y_range_);
  std::vector<int> gridsY_n1A2(x_range_ + y_range_);

  A1x = treenodes[A1].x;
  A1y = treenodes[A1].y;
  A2x = treenodes[A2].x;
  A2y = treenodes[A2].y;

  // copy all the grids on (n1, A1) and (n2, A2) to tmp arrays, and keep the
  // grids order A1->n1->A2 copy (n1, A1)
  cnt_n1A1 = copyGrids(
      treenodes, A1, n1, treeedges, edge_n1A1, gridsX_n1A1, gridsY_n1A1);

  // copy (n1, A2)
  cnt_n1A2 = copyGrids(
      treenodes, n1, A2, treeedges, edge_n1A2, gridsX_n1A2, gridsY_n1A2);

  // update route for (n1, A1) and (n1, A2)
  // find the index of E1 in (n1, A1)
  E1_pos = -1;
  for (i = 0; i < cnt_n1A1; i++) {
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
    free(treeedges[edge_n1A1].route.gridsX);
    free(treeedges[edge_n1A1].route.gridsY);
  }
  treeedges[edge_n1A1].route.gridsX
      = (short*) calloc((E1_pos + 1), sizeof(short));
  treeedges[edge_n1A1].route.gridsY
      = (short*) calloc((E1_pos + 1), sizeof(short));

  if (A1x <= E1x) {
    cnt = 0;
    for (i = 0; i <= E1_pos; i++) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = A1;
    treeedges[edge_n1A1].n2 = n1;
  } else {
    cnt = 0;
    for (i = E1_pos; i >= 0; i--) {
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
    free(treeedges[edge_n1A2].route.gridsX);
    free(treeedges[edge_n1A2].route.gridsY);
  }
  treeedges[edge_n1A2].route.gridsX
      = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos - 1), sizeof(short));
  treeedges[edge_n1A2].route.gridsY
      = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos - 1), sizeof(short));

  if (E1x <= A2x) {
    cnt = 0;
    for (i = E1_pos; i < cnt_n1A1; i++) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
      cnt++;
    }
    for (i = 1; i < cnt_n1A2; i++)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = n1;
    treeedges[edge_n1A2].n2 = A2;
  } else {
    cnt = 0;
    for (i = cnt_n1A2 - 1; i >= 1; i--)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      cnt++;
    }
    for (i = cnt_n1A1 - 1; i >= E1_pos; i--) {
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

void FastRouteCore::updateRouteType2(TreeNode* treenodes,
                      int n1,
                      int A1,
                      int A2,
                      int C1,
                      int C2,
                      int E1x,
                      int E1y,
                      TreeEdge* treeedges,
                      int edge_n1A1,
                      int edge_n1A2,
                      int edge_C1C2)
{
  int i, cnt, A1x, A1y, A2x, A2y, C1x, C1y, C2x, C2y;
  int edge_n1C1, edge_n1C2, edge_A1A2;
  int cnt_n1A1, cnt_n1A2, cnt_C1C2, E1_pos;
  int len_A1A2, len_n1C1, len_n1C2;
  
  std::vector<int> gridsX_n1A1(x_range_ + y_range_);
  std::vector<int> gridsY_n1A1(x_range_ + y_range_);
  std::vector<int> gridsX_n1A2(x_range_ + y_range_);
  std::vector<int> gridsY_n1A2(x_range_ + y_range_);
  std::vector<int> gridsX_C1C2(x_range_ + y_range_);
  std::vector<int> gridsY_C1C2(x_range_ + y_range_);

  A1x = treenodes[A1].x;
  A1y = treenodes[A1].y;
  A2x = treenodes[A2].x;
  A2y = treenodes[A2].y;
  C1x = treenodes[C1].x;
  C1y = treenodes[C1].y;
  C2x = treenodes[C2].x;
  C2y = treenodes[C2].y;

  edge_n1C1 = edge_n1A1;
  edge_n1C2 = edge_n1A2;
  edge_A1A2 = edge_C1C2;

  // combine (n1, A1) and (n1, A2) into (A1, A2), A1 is the first node and A2 is
  // the second grids order A1->n1->A2 copy (A1, n1)
  cnt_n1A1 = copyGrids(
      treenodes, A1, n1, treeedges, edge_n1A1, gridsX_n1A1, gridsY_n1A1);

  // copy (n1, A2)
  cnt_n1A2 = copyGrids(
      treenodes, n1, A2, treeedges, edge_n1A2, gridsX_n1A2, gridsY_n1A2);

  // copy all the grids on (C1, C2) to gridsX_C1C2[] and gridsY_C1C2[]
  cnt_C1C2 = copyGrids(
      treenodes, C1, C2, treeedges, edge_C1C2, gridsX_C1C2, gridsY_C1C2);

  // combine grids on original (A1, n1) and (n1, A2) to new (A1, A2)
  // allocate memory for gridsX[] and gridsY[] of edge_A1A2
  if (treeedges[edge_A1A2].route.type == RouteType::MazeRoute) {
    free(treeedges[edge_A1A2].route.gridsX);
    free(treeedges[edge_A1A2].route.gridsY);
  }
  len_A1A2 = cnt_n1A1 + cnt_n1A2 - 1;

  treeedges[edge_A1A2].route.gridsX = (short*) calloc(len_A1A2, sizeof(short));
  treeedges[edge_A1A2].route.gridsY = (short*) calloc(len_A1A2, sizeof(short));
  treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
  treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);

  cnt = 0;
  for (i = 0; i < cnt_n1A1; i++) {
    treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A1[i];
    treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A1[i];
    cnt++;
  }
  for (i = 1; i < cnt_n1A2; i++)  // do not repeat point n1
  {
    treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[i];
    treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[i];
    cnt++;
  }

  // find the index of E1 in (C1, C2)
  E1_pos = -1;
  for (i = 0; i < cnt_C1C2; i++) {
    if (gridsX_C1C2[i] == E1x && gridsY_C1C2[i] == E1y) {
      E1_pos = i;
      break;
    }
  }

  if (E1_pos == -1) {
    logger_->error(GRT, 170, "Invalid index for position ({}, {}).", E1x, E1y);
  }

  // allocate memory for gridsX[] and gridsY[] of edge_n1C1 and edge_n1C2
  if (treeedges[edge_n1C1].route.type == RouteType::MazeRoute) {
    free(treeedges[edge_n1C1].route.gridsX);
    free(treeedges[edge_n1C1].route.gridsY);
  }
  len_n1C1 = E1_pos + 1;
  treeedges[edge_n1C1].route.gridsX = (short*) calloc(len_n1C1, sizeof(short));
  treeedges[edge_n1C1].route.gridsY = (short*) calloc(len_n1C1, sizeof(short));
  treeedges[edge_n1C1].route.routelen = len_n1C1 - 1;
  treeedges[edge_n1C1].len = abs(C1x - E1x) + abs(C1y - E1y);

  if (treeedges[edge_n1C2].route.type == RouteType::MazeRoute) {
    free(treeedges[edge_n1C2].route.gridsX);
    free(treeedges[edge_n1C2].route.gridsY);
  }
  len_n1C2 = cnt_C1C2 - E1_pos;
  treeedges[edge_n1C2].route.gridsX = (short*) calloc(len_n1C2, sizeof(short));
  treeedges[edge_n1C2].route.gridsY = (short*) calloc(len_n1C2, sizeof(short));
  treeedges[edge_n1C2].route.routelen = len_n1C2 - 1;
  treeedges[edge_n1C2].len = abs(C2x - E1x) + abs(C2y - E1y);

  // split original (C1, C2) to (C1, n1) and (n1, C2)
  cnt = 0;
  for (i = 0; i <= E1_pos; i++) {
    treeedges[edge_n1C1].route.gridsX[i] = gridsX_C1C2[i];
    treeedges[edge_n1C1].route.gridsY[i] = gridsY_C1C2[i];
    cnt++;
  }

  cnt = 0;
  for (i = E1_pos; i < cnt_C1C2; i++) {
    treeedges[edge_n1C2].route.gridsX[cnt] = gridsX_C1C2[i];
    treeedges[edge_n1C2].route.gridsY[cnt] = gridsY_C1C2[i];
    cnt++;
  }
}

void FastRouteCore::reInitTree(int netID)
{
  int deg, numEdges, edgeID, d, j;
  TreeEdge* treeedge;
  Tree rsmt;

  newRipupNet(netID);

  deg = sttrees_[netID].deg;
  numEdges = 2 * deg - 3;
  for (edgeID = 0; edgeID < numEdges; edgeID++) {
    treeedge = &(sttrees_[netID].edges[edgeID]);
    if (treeedge->len > 0) {
      delete[] treeedge->route.gridsX;
      delete[] treeedge->route.gridsY;
    }
  }
  delete[] sttrees_[netID].nodes;
  delete[] sttrees_[netID].edges;

  d = nets_[netID]->deg;
  int x[d];
  int y[d];

  for (j = 0; j < d; j++) {
    x[j] = nets_[netID]->pinX[j];
    y[j] = nets_[netID]->pinY[j];
  }

  fluteCongest(netID, d, x, y, 2, 1.2, &rsmt);

  if (d > 3) {
    edgeShiftNew(&rsmt, netID);
  }

  copyStTree(netID, rsmt);
  newrouteLInMaze(netID);
  convertToMazerouteNet(netID);
}

void FastRouteCore::mazeRouteMSMD(int iter,
                   int expand,
                   float costHeight,
                   int ripup_threshold,
                   int mazeedgeThreshold,
                   bool Ordering,
                   int cost_type,
                   float LOGIS_COF,
                   int VIA,
                   int slope,
                   int L)
{
  int grid, netID, nidRPC;
  float forange;

  // maze routing for multi-source, multi-destination
  bool hypered, enter;
  int i, j, deg, edgeID, n1, n2, n1x, n1y, n2x, n2y, ymin, ymax, xmin, xmax,
      curX, curY, crossX, crossY, tmpX, tmpY, tmpi, min_x, min_y, num_edges;
  int regionX1, regionX2, regionY1, regionY2;
  int heapLen1, heapLen2, ind, ind1;
  int endpt1, endpt2, A1, A2, B1, B2, C1, C2, D1, D2, cnt, cnt_n1n2;
  int edge_n1n2, edge_n1A1, edge_n1A2, edge_n1C1, edge_n1C2, edge_A1A2,
      edge_C1C2;
  int edge_n2B1, edge_n2B2, edge_n2D1, edge_n2D2, edge_B1B2, edge_D1D2;
  int E1x, E1y, E2x, E2y;
  int tmp_grid, tmp_cost;
  int preX, preY, origENG, edgeREC;

  float tmp, *dtmp;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  const int max_usage_multiplier = 40;

  // allocate memory for distance and parent and pop_heap
  h_cost_table_.resize(max_usage_multiplier * h_capacity_);
  v_cost_table_.resize(max_usage_multiplier * v_capacity_);

  forange = max_usage_multiplier * h_capacity_;

  if (cost_type == 2) {
    for (i = 0; i < forange; i++) {
      if (i < h_capacity_ - 1)
        h_cost_table_[i]
            = costHeight / (exp((float) (h_capacity_ - i - 1) * LOGIS_COF) + 1)
              + 1;
      else
        h_cost_table_[i]
            = costHeight / (exp((float) (h_capacity_ - i - 1) * LOGIS_COF) + 1)
              + 1 + costHeight / slope * (i - h_capacity_);
    }
    forange = max_usage_multiplier * v_capacity_;
    for (i = 0; i < forange; i++) {
      if (i < v_capacity_ - 1)
        v_cost_table_[i]
            = costHeight / (exp((float) (v_capacity_ - i - 1) * LOGIS_COF) + 1)
              + 1;
      else
        v_cost_table_[i]
            = costHeight / (exp((float) (v_capacity_ - i - 1) * LOGIS_COF) + 1)
              + 1 + costHeight / slope * (i - v_capacity_);
    }
  } else {
    for (i = 0; i < forange; i++) {
      if (i < h_capacity_)
        h_cost_table_[i]
            = costHeight / (exp((float) (h_capacity_ - i) * LOGIS_COF) + 1) + 1;
      else
        h_cost_table_[i]
            = costHeight / (exp((float) (h_capacity_ - i) * LOGIS_COF) + 1) + 1
              + costHeight / slope * (i - h_capacity_);
    }
    forange = max_usage_multiplier * v_capacity_;
    for (i = 0; i < forange; i++) {
      if (i < v_capacity_)
        v_cost_table_[i]
            = costHeight / (exp((float) (v_capacity_ - i) * LOGIS_COF) + 1) + 1;
      else
        v_cost_table_[i]
            = costHeight / (exp((float) (v_capacity_ - i) * LOGIS_COF) + 1) + 1
              + costHeight / slope * (i - v_capacity_);
    }
  }

  forange = y_grid_ * x_range_;
  for (i = 0; i < forange; i++) {
    pop_heap2_[i] = false;
  }
  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_; j++)
      in_region_[i][j] = false;
  }

  if (Ordering) {
    StNetOrder();
  }

  for (nidRPC = 0; nidRPC < num_valid_nets_; nidRPC++) {
    if (Ordering) {
      netID = tree_order_cong_[nidRPC].treeIndex;
    } else {
      netID = nidRPC;
    }

    deg = sttrees_[netID].deg;

    origENG = expand;

    netedgeOrderDec(netID);

    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    // loop for all the tree edges (2*deg-3)
    num_edges = 2 * deg - 3;
    for (edgeREC = 0; edgeREC < num_edges; edgeREC++) {
      edgeID = net_eo_[edgeREC].edgeID;
      treeedge = &(treeedges[edgeID]);

      n1 = treeedge->n1;
      n2 = treeedge->n2;
      n1x = treenodes[n1].x;
      n1y = treenodes[n1].y;
      n2x = treenodes[n2].x;
      n2y = treenodes[n2].y;
      treeedge->len = abs(n2x - n1x) + abs(n2y - n1y);

      if (treeedge->len
          > mazeedgeThreshold)  // only route the non-degraded edges (len>0)
      {
        enter = newRipupCheck(
            treeedge, n1x, n1y, n2x, n2y, ripup_threshold, netID, edgeID);

        // ripup the routing for the edge
        if (enter) {
          if (n1y <= n2y) {
            ymin = n1y;
            ymax = n2y;
          } else {
            ymin = n2y;
            ymax = n1y;
          }

          if (n1x <= n2x) {
            xmin = n1x;
            xmax = n2x;
          } else {
            xmin = n2x;
            xmax = n1x;
          }

          enlarge_
              = std::min(origENG, (iter / 6 + 3) * treeedge->route.routelen);
          regionX1 = std::max(0, xmin - enlarge_);
          regionX2 = std::min(x_grid_ - 1, xmax + enlarge_);
          regionY1 = std::max(0, ymin - enlarge_);
          regionY2 = std::min(y_grid_ - 1, ymax + enlarge_);

          // initialize d1_[][] and d2_[][] as BIG_INT
          for (i = regionY1; i <= regionY2; i++) {
            for (j = regionX1; j <= regionX2; j++) {
              d1_[i][j] = BIG_INT;
              d2_[i][j] = BIG_INT;
              hyper_h_[i][j] = false;
              hyper_v_[i][j] = false;
            }
          }

          // setup heap1_, heap2_ and initialize d1_[][] and d2_[][] for all the
          // grids on the two subtrees
          setupHeap(netID,
                    edgeID,
                    &heapLen1,
                    &heapLen2,
                    regionX1,
                    regionX2,
                    regionY1,
                    regionY2);

          // while loop to find shortest path
          ind1 = (heap1_[0] - &d1_[0][0]);
          for (i = 0; i < heapLen2; i++)
            pop_heap2_[(heap2_[i] - &d2_[0][0])] = true;

          while (pop_heap2_[ind1]
                 == false)  // stop until the grid position been popped out from
                            // both heap1_ and heap2_
          {
            // relax all the adjacent grids within the enlarged region for
            // source subtree
            curX = ind1 % x_range_;
            curY = ind1 / x_range_;
            if (d1_[curY][curX] != 0) {
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

            extractMin(heap1_, heapLen1);
            heapLen1--;

            // left
            if (curX > regionX1) {
              grid = curY * (x_grid_ - 1) + curX - 1;
              if ((preY == curY) || (d1_[curY][curX] == 0)) {
                tmp = d1_[curY][curX]
                      + h_cost_table_[h_edges_[grid].usage + h_edges_[grid].red
                                    + L * h_edges_[grid].last_usage];
              } else {
                if (curX < regionX2 - 1) {
                  tmp_grid = curY * (x_grid_ - 1) + curX;
                  tmp_cost = d1_[curY][curX + 1]
                             + h_cost_table_[h_edges_[tmp_grid].usage
                                           + h_edges_[tmp_grid].red
                                           + L * h_edges_[tmp_grid].last_usage];

                  if (tmp_cost < d1_[curY][curX] + VIA) {
                    hyper_h_[curY][curX] = true;
                  }
                }
                tmp = d1_[curY][curX] + VIA
                      + h_cost_table_[h_edges_[grid].usage + h_edges_[grid].red
                                    + L * h_edges_[grid].last_usage];
              }
              tmpX = curX - 1;  // the left neighbor

              if (d1_[curY][tmpX]
                  >= BIG_INT)  // left neighbor not been put into heap1_
              {
                d1_[curY][tmpX] = tmp;
                parent_x3_[curY][tmpX] = curX;
                parent_y3_[curY][tmpX] = curY;
                hv_[curY][tmpX] = false;
                heap1_[heapLen1] = &d1_[curY][tmpX];
                heapLen1++;
                updateHeap(heap1_, heapLen1, heapLen1 - 1);
              } else if (d1_[curY][tmpX] > tmp)  // left neighbor been put into
                                                // heap1_ but needs update
              {
                d1_[curY][tmpX] = tmp;
                parent_x3_[curY][tmpX] = curX;
                parent_y3_[curY][tmpX] = curY;
                hv_[curY][tmpX] = false;
                dtmp = &d1_[curY][tmpX];
                ind = 0;
                while (heap1_[ind] != dtmp)
                  ind++;
                updateHeap(heap1_, heapLen1, ind);
              }
            }
            // right
            if (curX < regionX2) {
              grid = curY * (x_grid_ - 1) + curX;
              if ((preY == curY) || (d1_[curY][curX] == 0)) {
                tmp = d1_[curY][curX]
                      + h_cost_table_[h_edges_[grid].usage + h_edges_[grid].red
                                    + L * h_edges_[grid].last_usage];
              } else {
                if (curX > regionX1 + 1) {
                  tmp_grid = curY * (x_grid_ - 1) + curX - 1;
                  tmp_cost = d1_[curY][curX - 1]
                             + h_cost_table_[h_edges_[tmp_grid].usage
                                           + h_edges_[tmp_grid].red
                                           + L * h_edges_[tmp_grid].last_usage];

                  if (tmp_cost < d1_[curY][curX] + VIA) {
                    hyper_h_[curY][curX] = true;
                  }
                }
                tmp = d1_[curY][curX] + VIA
                      + h_cost_table_[h_edges_[grid].usage + h_edges_[grid].red
                                    + L * h_edges_[grid].last_usage];
              }
              tmpX = curX + 1;  // the right neighbor

              if (d1_[curY][tmpX]
                  >= BIG_INT)  // right neighbor not been put into heap1_
              {
                d1_[curY][tmpX] = tmp;
                parent_x3_[curY][tmpX] = curX;
                parent_y3_[curY][tmpX] = curY;
                hv_[curY][tmpX] = false;
                heap1_[heapLen1] = &d1_[curY][tmpX];
                heapLen1++;
                updateHeap(heap1_, heapLen1, heapLen1 - 1);
              } else if (d1_[curY][tmpX] > tmp)  // right neighbor been put into
                                                // heap1_ but needs update
              {
                d1_[curY][tmpX] = tmp;
                parent_x3_[curY][tmpX] = curX;
                parent_y3_[curY][tmpX] = curY;
                hv_[curY][tmpX] = false;
                dtmp = &d1_[curY][tmpX];
                ind = 0;
                while (heap1_[ind] != dtmp)
                  ind++;
                updateHeap(heap1_, heapLen1, ind);
              }
            }
            // bottom
            if (curY > regionY1) {
              grid = (curY - 1) * x_grid_ + curX;

              if ((preX == curX) || (d1_[curY][curX] == 0)) {
                tmp = d1_[curY][curX]
                      + v_cost_table_[v_edges_[grid].usage + v_edges_[grid].red
                                    + L * v_edges_[grid].last_usage];
              } else {
                if (curY < regionY2 - 1) {
                  tmp_grid = curY * x_grid_ + curX;
                  tmp_cost = d1_[curY + 1][curX]
                             + v_cost_table_[v_edges_[tmp_grid].usage
                                           + v_edges_[tmp_grid].red
                                           + L * v_edges_[tmp_grid].last_usage];

                  if (tmp_cost < d1_[curY][curX] + VIA) {
                    hyper_v_[curY][curX] = true;
                  }
                }
                tmp = d1_[curY][curX] + VIA
                      + v_cost_table_[v_edges_[grid].usage + v_edges_[grid].red
                                    + L * v_edges_[grid].last_usage];
              }
              tmpY = curY - 1;  // the bottom neighbor
              if (d1_[tmpY][curX]
                  >= BIG_INT)  // bottom neighbor not been put into heap1_
              {
                d1_[tmpY][curX] = tmp;
                parent_x1_[tmpY][curX] = curX;
                parent_y1_[tmpY][curX] = curY;
                hv_[tmpY][curX] = true;
                heap1_[heapLen1] = &d1_[tmpY][curX];
                heapLen1++;
                updateHeap(heap1_, heapLen1, heapLen1 - 1);
              } else if (d1_[tmpY][curX] > tmp)  // bottom neighbor been put into
                                                // heap1_ but needs update
              {
                d1_[tmpY][curX] = tmp;
                parent_x1_[tmpY][curX] = curX;
                parent_y1_[tmpY][curX] = curY;
                hv_[tmpY][curX] = true;
                dtmp = &d1_[tmpY][curX];
                ind = 0;
                while (heap1_[ind] != dtmp)
                  ind++;
                updateHeap(heap1_, heapLen1, ind);
              }
            }
            // top
            if (curY < regionY2) {
              grid = curY * x_grid_ + curX;

              if ((preX == curX) || (d1_[curY][curX] == 0)) {
                tmp = d1_[curY][curX]
                      + v_cost_table_[v_edges_[grid].usage + v_edges_[grid].red
                                    + L * v_edges_[grid].last_usage];
              } else {
                if (curY > regionY1 + 1) {
                  tmp_grid = (curY - 1) * x_grid_ + curX;
                  tmp_cost = d1_[curY - 1][curX]
                             + v_cost_table_[v_edges_[tmp_grid].usage
                                           + v_edges_[tmp_grid].red
                                           + L * v_edges_[tmp_grid].last_usage];

                  if (tmp_cost < d1_[curY][curX] + VIA) {
                    hyper_v_[curY][curX] = true;
                  }
                }
                tmp = d1_[curY][curX] + VIA
                      + v_cost_table_[v_edges_[grid].usage + v_edges_[grid].red
                                    + L * v_edges_[grid].last_usage];
              }
              tmpY = curY + 1;  // the top neighbor
              if (d1_[tmpY][curX]
                  >= BIG_INT)  // top neighbor not been put into heap1_
              {
                d1_[tmpY][curX] = tmp;
                parent_x1_[tmpY][curX] = curX;
                parent_y1_[tmpY][curX] = curY;
                hv_[tmpY][curX] = true;
                heap1_[heapLen1] = &d1_[tmpY][curX];
                heapLen1++;
                updateHeap(heap1_, heapLen1, heapLen1 - 1);
              } else if (d1_[tmpY][curX] > tmp)  // top neighbor been put into
                                                // heap1_ but needs update
              {
                d1_[tmpY][curX] = tmp;
                parent_x1_[tmpY][curX] = curX;
                parent_y1_[tmpY][curX] = curY;
                hv_[tmpY][curX] = true;
                dtmp = &d1_[tmpY][curX];
                ind = 0;
                while (heap1_[ind] != dtmp)
                  ind++;
                updateHeap(heap1_, heapLen1, ind);
              }
            }

            // update ind1 for next loop
            ind1 = (heap1_[0] - &d1_[0][0]);

          }  // while loop

          for (i = 0; i < heapLen2; i++)
            pop_heap2_[(heap2_[i] - &d2_[0][0])] = false;

          crossX = ind1 % x_range_;
          crossY = ind1 / x_range_;

          cnt = 0;
          curX = crossX;
          curY = crossY;
          std::vector<int> tmp_gridsX, tmp_gridsY;
          while (d1_[curY][curX] != 0)  // loop until reach subtree1
          {
            hypered = false;
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
          cnt_n1n2 = cnt;

          // change the tree structure according to the new routing for the tree
          // edge find E1 and E2, and the endpoints of the edges they are on
          E1x = gridsX[0];
          E1y = gridsY[0];
          E2x = gridsX.back();
          E2y = gridsY.back();

          edge_n1n2 = edgeID;
          // (1) consider subtree1
          if (n1 >= deg && (E1x != n1x || E1y != n1y))
          // n1 is not a pin and E1!=n1, then make change to subtree1,
          // otherwise, no change to subtree1
          {
            // find the endpoints of the edge E1 is on
            endpt1 = treeedges[corr_edge_[E1y][E1x]].n1;
            endpt2 = treeedges[corr_edge_[E1y][E1x]].n2;

            // find A1, A2 and edge_n1A1, edge_n1A2
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
                tmpi = A1;
                A1 = A2;
                A2 = tmpi;
                tmpi = edge_n1A1;
                edge_n1A1 = edge_n1A2;
                edge_n1A2 = tmpi;
              }

              // update route for edge (n1, A1), (n1, A2)
              updateRouteType1(treenodes,
                               n1,
                               A1,
                               A2,
                               E1x,
                               E1y,
                               treeedges,
                               edge_n1A1,
                               edge_n1A2);
              // update position for n1
              treenodes[n1].x = E1x;
              treenodes[n1].y = E1y;
            }     // if E1 is on (n1, A1) or (n1, A2)
            else  // E1 is not on (n1, A1) or (n1, A2), but on (C1, C2)
            {
              C1 = endpt1;
              C2 = endpt2;
              edge_C1C2 = corr_edge_[E1y][E1x];

              // update route for edge (n1, C1), (n1, C2) and (A1, A2)
              updateRouteType2(treenodes,
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
              edge_n1C1 = edge_n1A1;
              treeedges[edge_n1C1].n1 = C1;
              treeedges[edge_n1C1].n2 = n1;
              edge_n1C2 = edge_n1A2;
              treeedges[edge_n1C2].n1 = n1;
              treeedges[edge_n1C2].n2 = C2;
              edge_A1A2 = edge_C1C2;
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
              for (i = 0; i < 3; i++) {
                if (treenodes[A1].nbr[i] == n1) {
                  treenodes[A1].nbr[i] = A2;
                  treenodes[A1].edge[i] = edge_A1A2;
                  break;
                }
              }
              // A2's nbr n1->A1
              for (i = 0; i < 3; i++) {
                if (treenodes[A2].nbr[i] == n1) {
                  treenodes[A2].nbr[i] = A1;
                  treenodes[A2].edge[i] = edge_A1A2;
                  break;
                }
              }
              // C1's nbr C2->n1
              for (i = 0; i < 3; i++) {
                if (treenodes[C1].nbr[i] == C2) {
                  treenodes[C1].nbr[i] = n1;
                  treenodes[C1].edge[i] = edge_n1C1;
                  break;
                }
              }
              // C2's nbr C1->n1
              for (i = 0; i < 3; i++) {
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
            endpt1 = treeedges[corr_edge_[E2y][E2x]].n1;
            endpt2 = treeedges[corr_edge_[E2y][E2x]].n2;

            // find B1, B2
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
                tmpi = B1;
                B1 = B2;
                B2 = tmpi;
                tmpi = edge_n2B1;
                edge_n2B1 = edge_n2B2;
                edge_n2B2 = tmpi;
              }

              // update route for edge (n2, B1), (n2, B2)
              updateRouteType1(treenodes,
                               n2,
                               B1,
                               B2,
                               E2x,
                               E2y,
                               treeedges,
                               edge_n2B1,
                               edge_n2B2);

              // update position for n2
              treenodes[n2].x = E2x;
              treenodes[n2].y = E2y;
            }     // if E2 is on (n2, B1) or (n2, B2)
            else  // E2 is not on (n2, B1) or (n2, B2), but on (D1, D2)
            {
              D1 = endpt1;
              D2 = endpt2;
              edge_D1D2 = corr_edge_[E2y][E2x];

              // update route for edge (n2, D1), (n2, D2) and (B1, B2)
              updateRouteType2(treenodes,
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
              edge_n2D1 = edge_n2B1;
              treeedges[edge_n2D1].n1 = D1;
              treeedges[edge_n2D1].n2 = n2;
              edge_n2D2 = edge_n2B2;
              treeedges[edge_n2D2].n1 = n2;
              treeedges[edge_n2D2].n2 = D2;
              edge_B1B2 = edge_D1D2;
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
              for (i = 0; i < 3; i++) {
                if (treenodes[B1].nbr[i] == n2) {
                  treenodes[B1].nbr[i] = B2;
                  treenodes[B1].edge[i] = edge_B1B2;
                  break;
                }
              }
              // B2's nbr n2->B1
              for (i = 0; i < 3; i++) {
                if (treenodes[B2].nbr[i] == n2) {
                  treenodes[B2].nbr[i] = B1;
                  treenodes[B2].edge[i] = edge_B1B2;
                  break;
                }
              }
              // D1's nbr D2->n2
              for (i = 0; i < 3; i++) {
                if (treenodes[D1].nbr[i] == D2) {
                  treenodes[D1].nbr[i] = n2;
                  treenodes[D1].edge[i] = edge_n2D1;
                  break;
                }
              }
              // D2's nbr D1->n2
              for (i = 0; i < 3; i++) {
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
            free(treeedges[edge_n1n2].route.gridsX);
            free(treeedges[edge_n1n2].route.gridsY);
          }
          treeedges[edge_n1n2].route.gridsX
              = (short*) calloc(cnt_n1n2, sizeof(short));
          treeedges[edge_n1n2].route.gridsY
              = (short*) calloc(cnt_n1n2, sizeof(short));
          treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
          treeedges[edge_n1n2].route.routelen = cnt_n1n2 - 1;
          treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y - E2y);

          for (i = 0; i < cnt_n1n2; i++) {
            treeedges[edge_n1n2].route.gridsX[i] = gridsX[i];
            treeedges[edge_n1n2].route.gridsY[i] = gridsY[i];
          }

          int edgeCost = nets_[netID]->edgeCost;

          // update edge usage
          for (i = 0; i < cnt_n1n2 - 1; i++) {
            if (gridsX[i] == gridsX[i + 1])  // a vertical edge
            {
              min_y = std::min(gridsY[i], gridsY[i + 1]);
              v_edges_[min_y * x_grid_ + gridsX[i]].usage += edgeCost;
            } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
            {
              min_x = std::min(gridsX[i], gridsX[i + 1]);
              h_edges_[gridsY[i] * (x_grid_ - 1) + min_x].usage += edgeCost;
            }
          }

          if (checkRoute2DTree(netID)) {
            reInitTree(netID);
            return;
          }
        }  // congested route
      }    // maze routing
    }      // loop edgeID
  }

  
  h_cost_table_.clear();
  v_cost_table_.clear();
}

int FastRouteCore::getOverflow2Dmaze(int* maxOverflow, int* tUsage)
{
  int H_overflow = 0;
  int V_overflow = 0;
  int grid = 0;
  int i = 0;
  int j = 0;
  int max_H_overflow = 0;
  int max_V_overflow = 0;
  int max_overflow = 0;
  int numedges = 0;
  int overflow = 0;
  int total_cap = 0;
  int total_usage = 0;

  // check 2D edges for invalid usage values
  check2DEdgesUsage();

  total_usage = 0;
  total_cap = 0;

  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      total_usage += h_edges_[grid].usage;
      overflow = h_edges_[grid].usage - h_edges_[grid].cap;
      total_cap += h_edges_[grid].cap;
      if (overflow > 0) {
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
        numedges++;
      }
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      total_usage += v_edges_[grid].usage;
      overflow = v_edges_[grid].usage - v_edges_[grid].cap;
      total_cap += v_edges_[grid].cap;
      if (overflow > 0) {
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
        numedges++;
      }
    }
  }

  max_overflow = std::max(max_H_overflow, max_V_overflow);
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
  int i, j, grid, overflow, max_overflow, H_overflow, max_H_overflow,
      V_overflow, max_V_overflow, numedges;
  int total_usage, total_cap, hCap, vCap;

  // check 2D edges for invalid usage values
  check2DEdgesUsage();

  // get overflow
  overflow = max_overflow = H_overflow = max_H_overflow = V_overflow
      = max_V_overflow = 0;
  hCap = vCap = numedges = 0;

  total_usage = 0;
  total_cap = 0;

  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      total_usage += h_edges_[grid].est_usage;
      overflow = h_edges_[grid].est_usage - h_edges_[grid].cap;
      total_cap += h_edges_[grid].cap;
      hCap += h_edges_[grid].cap;
      if (overflow > 0) {
        H_overflow += overflow;
        max_H_overflow = std::max(max_H_overflow, overflow);
        numedges++;
      }
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      total_usage += v_edges_[grid].est_usage;
      overflow = v_edges_[grid].est_usage - v_edges_[grid].cap;
      total_cap += v_edges_[grid].cap;
      vCap += v_edges_[grid].cap;
      if (overflow > 0) {
        V_overflow += overflow;
        max_V_overflow = std::max(max_V_overflow, overflow);
        numedges++;
      }
    }
  }

  max_overflow = std::max(max_H_overflow, max_V_overflow);
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

int FastRouteCore::getOverflow3D(void)
{
  int i, j, k, grid, overflow, max_overflow, H_overflow, max_H_overflow,
      V_overflow, max_V_overflow;
  int cap;
  int total_usage;

  // get overflow
  overflow = max_overflow = H_overflow = max_H_overflow = V_overflow
      = max_V_overflow = 0;

  total_usage = 0;
  cap = 0;

  for (k = 0; k < num_layers_; k++) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j + k * (x_grid_ - 1) * y_grid_;
        total_usage += h_edges_3D_[grid].usage;
        overflow = h_edges_3D_[grid].usage - h_edges_3D_[grid].cap;
        cap += h_edges_3D_[grid].cap;

        if (overflow > 0) {
          H_overflow += overflow;
          max_H_overflow = std::max(max_H_overflow, overflow);
        }
      }
    }
    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j + k * x_grid_ * (y_grid_ - 1);
        total_usage += v_edges_3D_[grid].usage;
        overflow = v_edges_3D_[grid].usage - v_edges_3D_[grid].cap;
        cap += v_edges_3D_[grid].cap;

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
  int i, j, grid;
  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      h_edges_[grid].est_usage = 0;
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      v_edges_[grid].est_usage = 0;
    }
  }
}

void FastRouteCore::str_accu(int rnd)
{
  int i, j, grid, overflow;
  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      overflow = h_edges_[grid].usage - h_edges_[grid].cap;
      if (overflow > 0 || h_edges_[grid].congCNT > rnd) {
        h_edges_[grid].last_usage += h_edges_[grid].congCNT * overflow / 2;
      }
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      overflow = v_edges_[grid].usage - v_edges_[grid].cap;
      if (overflow > 0 || v_edges_[grid].congCNT > rnd) {
        v_edges_[grid].last_usage += v_edges_[grid].congCNT * overflow / 2;
      }
    }
  }
}

void FastRouteCore::InitLastUsage(int upType)
{
  int i, j, grid;
  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_ - 1; j++) {
      grid = i * (x_grid_ - 1) + j;
      h_edges_[grid].last_usage = 0;
    }
  }

  for (i = 0; i < y_grid_ - 1; i++) {
    for (j = 0; j < x_grid_; j++) {
      grid = i * x_grid_ + j;
      v_edges_[grid].last_usage = 0;
    }
  }

  if (upType == 1) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        h_edges_[grid].congCNT = 0;
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        v_edges_[grid].congCNT = 0;
      }
    }
  } else if (upType == 2) {
    for (i = 0; i < y_grid_; i++) {
      for (j = 0; j < x_grid_ - 1; j++) {
        grid = i * (x_grid_ - 1) + j;
        h_edges_[grid].last_usage = h_edges_[grid].last_usage * 0.2;
      }
    }

    for (i = 0; i < y_grid_ - 1; i++) {
      for (j = 0; j < x_grid_; j++) {
        grid = i * x_grid_ + j;
        v_edges_[grid].last_usage = v_edges_[grid].last_usage * 0.2;
      }
    }
  }
}

}  // namespace grt
