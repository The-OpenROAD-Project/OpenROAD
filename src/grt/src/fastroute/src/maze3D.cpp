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
#include "odb/db.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

struct parent3D
{
  short l;
  int x, y;
};

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

// non recursive version of heapify-
static void heapify3D(std::vector<int*>& array)
{
  bool stop = false;
  const int heapSize = array.size();
  int i = 0;

  int* tmp = array[i];
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

static void updateHeap3D(std::vector<int*>& array, int i)
{
  int* tmpi = array[i];
  while (i > 0 && *(array[parent_index(i)]) > *tmpi) {
    const int parent = parent_index(i);
    array[i] = array[parent];
    i = parent;
  }
  array[i] = tmpi;
}

// extract the entry with minimum distance from Priority queue
static void removeMin3D(std::vector<int*>& array)
{
  array[0] = array.back();
  heapify3D(array);
  array.pop_back();
}

void FastRouteCore::addNeighborPoints(const int netID,
                                      const int n1,
                                      const int n2,
                                      std::vector<int*>& points_heap_3D,
                                      multi_array<int, 3>& dist_3D,
                                      multi_array<Direction, 3>& directions_3D,
                                      multi_array<int, 3>& corr_edge_3D)
{
  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;

  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;

  const int numNodes = sttrees_[netID].num_nodes();
  std::vector<bool> heapVisited(numNodes, false);
  std::vector<int> heapQueue(numNodes);

  int queuehead = 0;
  int queuetail = 0;

  int nt = treenodes[n1].stackAlias;

  // add n1 into heap1_3D
  for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
    dist_3D[l][y1][x1] = 0;
    directions_3D[l][y1][x1] = Direction::Origin;
    points_heap_3D.push_back(&dist_3D[l][y1][x1]);
    heapVisited[n1] = true;
  }

  // add n1 into the heapQueue
  heapQueue[queuetail] = n1;
  queuetail++;

  // loop to find all the edges in subtree t1
  while (queuetail > queuehead) {
    // get cur node from the queuehead
    const int cur = heapQueue[queuehead];
    queuehead++;
    heapVisited[cur] = true;
    const int nbrCount = treenodes[cur].nbr_count;
    for (int i = 0; i < nbrCount; i++) {
      const int nbr = treenodes[cur].nbr[i];
      const int edge = treenodes[cur].edge[i];
      if (nbr == n2) {
        continue;
      }
      if (heapVisited[nbr]) {
        continue;
      }
      // put all the grids of the adjacent tree edges into
      // points_heap_3D
      if (treeedges[edge].route.routelen > 0) {
        // not a degraded edge
        // put nbr into points_heap_3D if in enlarged region
        if (in_region_[treenodes[nbr].y][treenodes[nbr].x]) {
          const int nbrX = treenodes[nbr].x;
          const int nbrY = treenodes[nbr].y;
          nt = treenodes[nbr].stackAlias;
          for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
            dist_3D[l][nbrY][nbrX] = 0;
            directions_3D[l][nbrY][nbrX] = Direction::Origin;
            points_heap_3D.push_back(&dist_3D[l][nbrY][nbrX]);
            corr_edge_3D[l][nbrY][nbrX] = edge;
          }
        }

        // the coordinates of two end nodes of the edge

        const Route* route = &(treeedges[edge].route);
        if (route->type == RouteType::MazeRoute) {
          for (int j = 1; j < route->routelen; j++) {
            // don't put edge_n1 and edge_n2 into points_heap_3D
            const int x_grid = route->gridsX[j];
            const int y_grid = route->gridsY[j];
            const int l_grid = route->gridsL[j];

            if (in_region_[y_grid][x_grid]) {
              dist_3D[l_grid][y_grid][x_grid] = 0;
              points_heap_3D.push_back(&dist_3D[l_grid][y_grid][x_grid]);
              directions_3D[l_grid][y_grid][x_grid] = Direction::Origin;
              corr_edge_3D[l_grid][y_grid][x_grid] = edge;
            }
          }

        }  // if MazeRoute
      }    // if not a degraded edge (len>0)

      // add the neighbor of cur node into heapQueue
      heapQueue[queuetail] = nbr;
      queuetail++;

    }  // loop i (neigbors for cur node)
  }    // while heapQueue is not empty
}

void FastRouteCore::setupHeap3D(int netID,
                                int edgeID,
                                std::vector<int*>& src_heap_3D,
                                std::vector<int*>& dest_heap_3D,
                                multi_array<Direction, 3>& directions_3D,
                                multi_array<int, 3>& corr_edge_3D,
                                multi_array<int, 3>& d1_3D,
                                multi_array<int, 3>& d2_3D,
                                int regionX1,
                                int regionX2,
                                int regionY1,
                                int regionY2)
{
  const auto& treeedges = sttrees_[netID].edges;
  const auto& treenodes = sttrees_[netID].nodes;

  const int num_terminals = sttrees_[netID].num_terminals;

  const int n1 = treeedges[edgeID].n1;
  const int n2 = treeedges[edgeID].n2;
  const int x1 = treenodes[n1].x;
  const int y1 = treenodes[n1].y;
  const int x2 = treenodes[n2].x;
  const int y2 = treenodes[n2].y;

  src_heap_3D.clear();
  dest_heap_3D.clear();

  if (num_terminals == 2) {  // 2-pin net
    const int node1_alias = treenodes[n1].stackAlias;
    const int node2_alias = treenodes[n2].stackAlias;
    const int node1_access_layer = nets_[netID]->getPinL()[node1_alias];
    const int node2_access_layer = nets_[netID]->getPinL()[node2_alias];

    d1_3D[node1_access_layer][y1][x1] = 0;
    directions_3D[node1_access_layer][y1][x1] = Direction::Origin;
    src_heap_3D.push_back(&d1_3D[node1_access_layer][y1][x1]);
    d2_3D[node2_access_layer][y2][x2] = 0;
    directions_3D[node2_access_layer][y2][x2] = Direction::Origin;
    dest_heap_3D.push_back(&d2_3D[node2_access_layer][y2][x2]);
  } else {  // net with more than 2 pins
    for (int i = regionY1; i <= regionY2; i++) {
      for (int j = regionX1; j <= regionX2; j++) {
        in_region_[i][j] = true;
      }
    }
    // find all the grids on tree edges in subtree t1 (connecting to n1) and put
    // them into src_heap_3D
    addNeighborPoints(
        netID, n1, n2, src_heap_3D, d1_3D, directions_3D, corr_edge_3D);

    // find all the grids on tree edges in subtree t2 (connecting
    // to n2) and put them into dest_heap_3D
    addNeighborPoints(
        netID, n2, n1, dest_heap_3D, d2_3D, directions_3D, corr_edge_3D);

    for (int i = regionY1; i <= regionY2; i++) {
      for (int j = regionX1; j <= regionX2; j++) {
        in_region_[i][j] = false;
      }
    }
  }  // net with more than two pins
}

void FastRouteCore::newUpdateNodeLayers(std::vector<TreeNode>& treenodes,
                                        const int edgeID,
                                        const int n1,
                                        const int lastL)
{
  const int con = treenodes[n1].conCNT;

  treenodes[n1].heights[con] = lastL;
  treenodes[n1].eID[con] = edgeID;
  treenodes[n1].conCNT++;
  if (treenodes[n1].topL < lastL) {
    treenodes[n1].topL = lastL;
    treenodes[n1].hID = edgeID;
  }
  if (treenodes[n1].botL > lastL) {
    treenodes[n1].botL = lastL;
    treenodes[n1].lID = edgeID;
  }
}

int FastRouteCore::copyGrids3D(std::vector<TreeNode>& treenodes,
                               int n1,
                               int n2,
                               std::vector<TreeEdge>& treeedges,
                               int edge_n1n2,
                               std::vector<int>& gridsX_n1n2,
                               std::vector<int>& gridsY_n1n2,
                               std::vector<int>& gridsL_n1n2)
{
  const int n1x = treenodes[n1].x;
  const int n1y = treenodes[n1].y;
  const int n1l = treenodes[n1].botL;
  const int routelen = treeedges[edge_n1n2].route.routelen;

  if (routelen > 0) {
    gridsX_n1n2.reserve(routelen + 1);
    gridsY_n1n2.reserve(routelen + 1);
    gridsL_n1n2.reserve(routelen + 1);
  }

  int cnt = 0;
  if (treeedges[edge_n1n2].n1 == n1) {  // n1 is the first node of (n1, n2)
    if (treeedges[edge_n1n2].route.routelen > 0) {
      for (int i = 0; i <= treeedges[edge_n1n2].route.routelen; i++) {
        gridsX_n1n2.push_back(treeedges[edge_n1n2].route.gridsX[i]);
        gridsY_n1n2.push_back(treeedges[edge_n1n2].route.gridsY[i]);
        gridsL_n1n2.push_back(treeedges[edge_n1n2].route.gridsL[i]);
        cnt++;
      }
    }  // MazeRoute
    else
    // NoRoute
    {
      fflush(stdout);
      gridsX_n1n2.push_back(n1x);
      gridsY_n1n2.push_back(n1y);
      gridsL_n1n2.push_back(n1l);
      cnt++;
    }
  }     // if n1 is the first node of (n1, n2)
  else  // n2 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.routelen > 0) {
      for (int i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
        gridsX_n1n2.push_back(treeedges[edge_n1n2].route.gridsX[i]);
        gridsY_n1n2.push_back(treeedges[edge_n1n2].route.gridsY[i]);
        gridsL_n1n2.push_back(treeedges[edge_n1n2].route.gridsL[i]);
        cnt++;
      }
    }     // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2.push_back(n1x);
      gridsY_n1n2.push_back(n1y);
      gridsL_n1n2.push_back(n1l);
      cnt++;
    }  // MazeRoute
  }

  return cnt;
}

void FastRouteCore::updateRouteType13D(int netID,
                                       std::vector<TreeNode>& treenodes,
                                       int n1,
                                       int A1,
                                       int A2,
                                       int E1x,
                                       int E1y,
                                       std::vector<TreeEdge>& treeedges,
                                       int edge_n1A1,
                                       int edge_n1A2)
{
  std::vector<int> gridsX_n1A1;
  std::vector<int> gridsY_n1A1;
  std::vector<int> gridsL_n1A1;
  std::vector<int> gridsX_n1A2;
  std::vector<int> gridsY_n1A2;
  std::vector<int> gridsL_n1A2;

  // copy all the grids on (n1, A1) and (n2, A2) to tmp arrays, and keep the
  // grids order A1->n1->A2 copy (n1, A1)
  const int cnt_n1A1 = copyGrids3D(treenodes,
                                   A1,
                                   n1,
                                   treeedges,
                                   edge_n1A1,
                                   gridsX_n1A1,
                                   gridsY_n1A1,
                                   gridsL_n1A1);

  // copy (n1, A2)
  const int cnt_n1A2 = copyGrids3D(treenodes,
                                   n1,
                                   A2,
                                   treeedges,
                                   edge_n1A2,
                                   gridsX_n1A2,
                                   gridsY_n1A2,
                                   gridsL_n1A2);

  if (cnt_n1A1 == 1) {
    logger_->error(
        GRT, 187, "In 3D maze routing, type 1 node shift, cnt_n1A1 is 1.");
  }

  int E1_pos1 = -1;
  for (int i = 0; i < cnt_n1A1; i++) {
    if (gridsX_n1A1[i] == E1x && gridsY_n1A1[i] == E1y)  // reach the E1
    {
      E1_pos1 = i;
      break;
    }
  }

  if (E1_pos1 == -1) {
    logger_->error(GRT, 171, "Invalid index for position ({}, {}).", E1x, E1y);
  }

  int E1_pos2 = 0;
  for (int i = cnt_n1A1 - 1; i >= 0; i--) {
    if (gridsX_n1A1[i] == E1x && gridsY_n1A1[i] == E1y)  // reach the E1
    {
      E1_pos2 = i;
      break;
    }
  }

  // reallocate memory for route.gridsX and route.gridsY
  if (treeedges[edge_n1A1].route.type == RouteType::MazeRoute
      && treeedges[edge_n1A1].route.routelen
             > 0)  // if originally allocated, free them first
  {
    treeedges[edge_n1A1].route.gridsX.clear();
    treeedges[edge_n1A1].route.gridsY.clear();
    treeedges[edge_n1A1].route.gridsL.clear();
  }
  treeedges[edge_n1A1].route.gridsX.resize(E1_pos1 + 1, 0);
  treeedges[edge_n1A1].route.gridsY.resize(E1_pos1 + 1, 0);
  treeedges[edge_n1A1].route.gridsL.resize(E1_pos1 + 1, 0);

  const int A1x = treenodes[A1].x;
  const int A1y = treenodes[A1].y;
  const int A2x = treenodes[A2].x;
  const int A2y = treenodes[A2].y;

  if (A1x <= E1x) {
    int cnt = 0;
    for (int i = 0; i <= E1_pos1; i++) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A1].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = A1;
    treeedges[edge_n1A1].n2 = n1;
  } else {
    int cnt = 0;
    for (int i = E1_pos1; i >= 0; i--) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A1].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = n1;
    treeedges[edge_n1A1].n2 = A1;
  }
  treeedges[edge_n1A1].len = abs(A1x - E1x) + abs(A1y - E1y);

  treeedges[edge_n1A1].route.type = RouteType::MazeRoute;
  treeedges[edge_n1A1].route.routelen = E1_pos1;

  // reallocate memory for route.gridsX and route.gridsY
  if (treeedges[edge_n1A2].route.type == RouteType::MazeRoute
      && treeedges[edge_n1A2].route.routelen > 0)
  // if originally allocated, free them first
  {
    treeedges[edge_n1A2].route.gridsX.clear();
    treeedges[edge_n1A2].route.gridsY.clear();
    treeedges[edge_n1A2].route.gridsL.clear();
  }

  if (cnt_n1A2 > 1) {
    treeedges[edge_n1A2].route.gridsX.resize(
        cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
            + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0]),
        0);
    treeedges[edge_n1A2].route.gridsY.resize(
        cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
            + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0]),
        0);
    treeedges[edge_n1A2].route.gridsL.resize(
        cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
            + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0]),
        0);
  } else {
    treeedges[edge_n1A2].route.gridsX.resize(cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1,
                                             0);
    treeedges[edge_n1A2].route.gridsY.resize(cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1,
                                             0);
    treeedges[edge_n1A2].route.gridsL.resize(cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1,
                                             0);
  }

  int cnt;
  if (E1x <= A2x) {
    cnt = 0;
    for (int i = E1_pos2; i < cnt_n1A1; i++) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    if (cnt_n1A2 > 1) {
      if (gridsL_n1A1[cnt_n1A1 - 1] > gridsL_n1A2[0]) {
        for (int l = gridsL_n1A1[cnt_n1A1 - 1] - 1; l >= gridsL_n1A2[0]; l--) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      } else if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (int l = gridsL_n1A1[cnt_n1A1 - 1] + 1; l <= gridsL_n1A2[0]; l++) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      }
    }

    for (int i = 1; i < cnt_n1A2; i++)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A2[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = n1;
    treeedges[edge_n1A2].n2 = A2;
  } else {
    cnt = 0;
    for (int i = cnt_n1A2 - 1; i >= 1; i--)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A2[i];
      cnt++;
    }

    if (cnt_n1A2 > 1) {
      if (gridsL_n1A1[cnt_n1A1 - 1] > gridsL_n1A2[0]) {
        for (int l = gridsL_n1A2[0]; l < gridsL_n1A1[cnt_n1A1 - 1]; l++) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      } else if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (int l = gridsL_n1A2[0]; l > gridsL_n1A1[cnt_n1A1 - 1]; l--) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      }
    }
    for (int i = cnt_n1A1 - 1; i >= E1_pos2; i--) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A2].n1 = A2;
    treeedges[edge_n1A2].n2 = n1;
  }
  treeedges[edge_n1A2].route.type = RouteType::MazeRoute;
  treeedges[edge_n1A2].route.routelen = cnt - 1;
  treeedges[edge_n1A2].len = abs(A2x - E1x) + abs(A2y - E1y);

  treenodes[n1].x = E1x;
  treenodes[n1].y = E1y;
}

void FastRouteCore::updateRouteType23D(int netID,
                                       std::vector<TreeNode>& treenodes,
                                       int n1,
                                       int A1,
                                       int A2,
                                       int C1,
                                       int C2,
                                       int E1x,
                                       int E1y,
                                       std::vector<TreeEdge>& treeedges,
                                       int edge_n1A1,
                                       int edge_n1A2,
                                       int edge_C1C2)
{
  int cnt;
  std::vector<int> gridsX_n1A1;
  std::vector<int> gridsY_n1A1;
  std::vector<int> gridsL_n1A1;
  std::vector<int> gridsX_n1A2;
  std::vector<int> gridsY_n1A2;
  std::vector<int> gridsL_n1A2;
  std::vector<int> gridsX_C1C2;
  std::vector<int> gridsY_C1C2;
  std::vector<int> gridsL_C1C2;

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

  // copy (A1, n1)
  const int cnt_n1A1 = copyGrids3D(treenodes,
                                   A1,
                                   n1,
                                   treeedges,
                                   edge_n1A1,
                                   gridsX_n1A1,
                                   gridsY_n1A1,
                                   gridsL_n1A1);

  // copy (n1, A2)
  const int cnt_n1A2 = copyGrids3D(treenodes,
                                   n1,
                                   A2,
                                   treeedges,
                                   edge_n1A2,
                                   gridsX_n1A2,
                                   gridsY_n1A2,
                                   gridsL_n1A2);

  // copy all the grids on (C1, C2) to gridsX_C1C2[] and gridsY_C1C2[]
  const int cnt_C1C2 = copyGrids3D(treenodes,
                                   C1,
                                   C2,
                                   treeedges,
                                   edge_C1C2,
                                   gridsX_C1C2,
                                   gridsY_C1C2,
                                   gridsL_C1C2);

  // combine grids on original (A1, n1) and (n1, A2) to new (A1, A2)
  // allocate memory for gridsX[] and gridsY[] of edge_A1A2
  if (treeedges[edge_A1A2].route.type == RouteType::MazeRoute) {
    treeedges[edge_A1A2].route.gridsX.clear();
    treeedges[edge_A1A2].route.gridsY.clear();
    treeedges[edge_A1A2].route.gridsL.clear();
  }
  int len_A1A2 = cnt_n1A1 + cnt_n1A2 - 1;

  if (len_A1A2 == 1) {
    treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
    treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);
  } else {
    int extraLen = 0;
    if (cnt_n1A1 > 1 && cnt_n1A2 > 1) {
      extraLen = abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0]);
      len_A1A2 += extraLen;
    }
    treeedges[edge_A1A2].route.gridsX.resize(len_A1A2, 0);
    treeedges[edge_A1A2].route.gridsY.resize(len_A1A2, 0);
    treeedges[edge_A1A2].route.gridsL.resize(len_A1A2, 0);
    treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
    treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);

    cnt = 0;
    int startIND = 0;

    if (cnt_n1A1 > 1) {
      startIND = 1;
      for (int i = 0; i < cnt_n1A1; i++) {
        treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A1[i];
        treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A1[i];
        treeedges[edge_A1A2].route.gridsL[cnt] = gridsL_n1A1[i];
        cnt++;
      }
    }

    if (extraLen > 0) {
      if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (int i = gridsL_n1A1[cnt_n1A1 - 1] + 1; i <= gridsL_n1A2[0]; i++) {
          treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_A1A2].route.gridsL[cnt] = i;
          cnt++;
        }
      } else {
        for (int i = gridsL_n1A1[cnt_n1A1 - 1] - 1; i >= gridsL_n1A2[1]; i--) {
          treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_A1A2].route.gridsL[cnt] = i;
          cnt++;
        }
      }
    }

    for (int i = startIND; i < cnt_n1A2; i++)  // do not repeat point n1
    {
      treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      treeedges[edge_A1A2].route.gridsL[cnt] = gridsL_n1A2[i];
      cnt++;
    }
  }

  if (cnt_C1C2 == 1) {
    debugPrint(
        logger_, utl::GRT, "maze_3d", 1, "Shift to 0 length edge, type2.");
  }

  // find the index of E1 in (C1, C2)
  int E1_pos1 = -1;
  for (int i = 0; i < cnt_C1C2; i++) {
    if (gridsX_C1C2[i] == E1x && gridsY_C1C2[i] == E1y) {
      E1_pos1 = i;
      break;
    }
  }

  int E1_pos2 = -1;

  for (int i = cnt_C1C2 - 1; i >= 0; i--) {
    if (gridsX_C1C2[i] == E1x && gridsY_C1C2[i] == E1y) {
      E1_pos2 = i;
      break;
    }
  }

  if (E1_pos2 == -1) {
    logger_->error(GRT, 172, "Invalid index for position ({}, {}).", E1x, E1y);
  }

  // allocate memory for gridsX[] and gridsY[] of edge_n1C1 and edge_n1C2
  if (treeedges[edge_n1C1].route.type == RouteType::MazeRoute
      && treeedges[edge_n1C1].route.routelen > 0) {
    treeedges[edge_n1C1].route.gridsX.clear();
    treeedges[edge_n1C1].route.gridsY.clear();
    treeedges[edge_n1C1].route.gridsL.clear();
  }
  const int len_n1C1 = E1_pos1 + 1;

  treeedges[edge_n1C1].route.gridsX.resize(len_n1C1, 0);
  treeedges[edge_n1C1].route.gridsY.resize(len_n1C1, 0);
  treeedges[edge_n1C1].route.gridsL.resize(len_n1C1, 0);
  treeedges[edge_n1C1].route.routelen = len_n1C1 - 1;
  treeedges[edge_n1C1].len = abs(C1x - E1x) + abs(C1y - E1y);

  if (treeedges[edge_n1C2].route.type == RouteType::MazeRoute
      && treeedges[edge_n1C2].route.routelen > 0) {
    treeedges[edge_n1C2].route.gridsX.clear();
    treeedges[edge_n1C2].route.gridsY.clear();
    treeedges[edge_n1C2].route.gridsL.clear();
  }
  const int len_n1C2 = cnt_C1C2 - E1_pos2;

  treeedges[edge_n1C2].route.gridsX.resize(len_n1C2, 0);
  treeedges[edge_n1C2].route.gridsY.resize(len_n1C2, 0);
  treeedges[edge_n1C2].route.gridsL.resize(len_n1C2, 0);
  treeedges[edge_n1C2].route.routelen = len_n1C2 - 1;
  treeedges[edge_n1C2].len = abs(C2x - E1x) + abs(C2y - E1y);

  // split original (C1, C2) to (C1, n1) and (n1, C2)
  for (int i = 0; i <= E1_pos1; i++) {
    treeedges[edge_n1C1].route.gridsX[i] = gridsX_C1C2[i];
    treeedges[edge_n1C1].route.gridsY[i] = gridsY_C1C2[i];
    treeedges[edge_n1C1].route.gridsL[i] = gridsL_C1C2[i];
  }

  cnt = 0;
  for (int i = E1_pos2; i < cnt_C1C2; i++) {
    treeedges[edge_n1C2].route.gridsX[cnt] = gridsX_C1C2[i];
    treeedges[edge_n1C2].route.gridsY[cnt] = gridsY_C1C2[i];
    treeedges[edge_n1C2].route.gridsL[cnt] = gridsL_C1C2[i];
    cnt++;
  }
}

void FastRouteCore::mazeRouteMSMDOrder3D(int expand,
                                         int ripupTHlb,
                                         int ripupTHub)
{
  static multi_array<Direction, 3> directions_3D(
      boost::extents[num_layers_][y_grid_][x_grid_]);
  static multi_array<int, 3> corr_edge_3D(
      boost::extents[num_layers_][y_grid_][x_grid_]);
  static multi_array<parent3D, 3> pr_3D_(
      boost::extents[num_layers_][y_grid_][x_grid_]);

  int64 total_size = static_cast<int64>(num_layers_) * y_range_ * x_range_;
  static std::vector<bool> pop_heap2_3D(total_size, false);

  // allocate memory for priority queue
  total_size = static_cast<int64>(y_grid_) * x_grid_ * num_layers_;
  static std::vector<int*> src_heap_3D(total_size);
  static std::vector<int*> dest_heap_3D(total_size);

  for (int i = 0; i < y_grid_; i++) {
    for (int j = 0; j < x_grid_; j++) {
      in_region_[i][j] = false;
    }
  }

  const int endIND = tree_order_pv_.size() * 0.9;

  static multi_array<int, 3> d1_3D(
      boost::extents[num_layers_][y_range_][x_range_]);
  static multi_array<int, 3> d2_3D(
      boost::extents[num_layers_][y_range_][x_range_]);

  for (int orderIndex = 0; orderIndex < endIND; orderIndex++) {
    const int netID = tree_order_pv_[orderIndex].treeIndex;

    FrNet* net = nets_[netID];

    int enlarge = expand;
    const int num_terminals = sttrees_[netID].num_terminals;
    auto& treeedges = sttrees_[netID].edges;
    auto& treenodes = sttrees_[netID].nodes;
    const int origEng = enlarge;

    for (int edgeID = 0; edgeID < sttrees_[netID].num_edges(); edgeID++) {
      TreeEdge* treeedge = &(treeedges[edgeID]);

      if (treeedge->len >= ripupTHub || treeedge->len <= ripupTHlb) {
        continue;
      }
      int n1 = treeedge->n1;
      int n2 = treeedge->n2;
      const int n1x = treenodes[n1].x;
      const int n1y = treenodes[n1].y;
      const int n2x = treenodes[n2].x;
      const int n2y = treenodes[n2].y;

      const int ymin = std::min(n1y, n2y);
      const int ymax = std::max(n1y, n2y);

      const int xmin = std::min(n1x, n2x);
      const int xmax = std::max(n1x, n2x);

      // ripup the routing for the edge
      if (!newRipup3DType3(netID, edgeID)) {
        continue;
      }
      enlarge = std::min(origEng, treeedge->route.routelen);

      const int regionX1 = std::max(0, xmin - enlarge);
      const int regionX2 = std::min(x_grid_ - 1, xmax + enlarge);
      const int regionY1 = std::max(0, ymin - enlarge);
      const int regionY2 = std::min(y_grid_ - 1, ymax + enlarge);

      bool n1Shift = false;
      bool n2Shift = false;
      int n1a = treeedge->n1a;
      int n2a = treeedge->n2a;

      // initialize pop_src_heap_3D_[] and pop_heap2_3D[] as false (for
      // detecting the shortest path is found or not)

      for (int k = 0; k < num_layers_; k++) {
        for (int i = regionY1; i <= regionY2; i++) {
          for (int j = regionX1; j <= regionX2; j++) {
            d1_3D[k][i][j] = BIG_INT;
            d2_3D[k][i][j] = BIG_INT;
          }
        }
      }

      // setup src_heap_3D, dest_heap_3D and initialize d1_3D[][] and
      // d2_3D[][] for all the grids on the two subtrees
      setupHeap3D(netID,
                  edgeID,
                  src_heap_3D,
                  dest_heap_3D,
                  directions_3D,
                  corr_edge_3D,
                  d1_3D,
                  d2_3D,
                  regionX1,
                  regionX2,
                  regionY1,
                  regionY2);

      // while loop to find shortest path
      int ind1 = (src_heap_3D[0] - &d1_3D[0][0][0]);

      for (int i = 0; i < dest_heap_3D.size(); i++)
        pop_heap2_3D[dest_heap_3D[i] - &d2_3D[0][0][0]] = true;

      while (pop_heap2_3D[ind1]
             == false)  // stop until the grid position been popped out from
                        // both src_heap_3D and dest_heap_3D
      {
        // relax all the adjacent grids within the enlarged region for
        // source subtree
        const int curL = ind1 / (grid_hv_);
        const int remd = ind1 % (grid_hv_);
        const int curX = remd % x_range_;
        const int curY = remd / x_range_;
        removeMin3D(src_heap_3D);

        const bool Horizontal
            = layer_directions_[curL] == odb::dbTechLayerDir::HORIZONTAL;

        if (Horizontal) {
          // left
          if (curX > regionX1
              && directions_3D[curL][curY][curX] != Direction::East) {
            const float tmp = d1_3D[curL][curY][curX] + 1;
            if (h_edges_3D_[curL][curY][curX - 1].usage
                    < h_edges_3D_[curL][curY][curX - 1].cap
                && net->getMinLayer() <= curL && curL <= net->getMaxLayer()) {
              const int tmpX = curX - 1;  // the left neighbor

              if (d1_3D[curL][curY][tmpX] >= BIG_INT)  // left neighbor not been
                                                       // put into src_heap_3D
              {
                d1_3D[curL][curY][tmpX] = tmp;
                pr_3D_[curL][curY][tmpX].l = curL;
                pr_3D_[curL][curY][tmpX].x = curX;
                pr_3D_[curL][curY][tmpX].y = curY;
                directions_3D[curL][curY][tmpX] = Direction::West;
                src_heap_3D.push_back(&d1_3D[curL][curY][tmpX]);
                updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
              } else if (d1_3D[curL][curY][tmpX]
                         > tmp)  // left neighbor been put into src_heap_3D
                                 // but needs update
              {
                d1_3D[curL][curY][tmpX] = tmp;
                pr_3D_[curL][curY][tmpX].l = curL;
                pr_3D_[curL][curY][tmpX].x = curX;
                pr_3D_[curL][curY][tmpX].y = curY;
                directions_3D[curL][curY][tmpX] = Direction::West;
                const int* dtmp = &d1_3D[curL][curY][tmpX];
                int ind = 0;
                while (src_heap_3D[ind] != dtmp)
                  ind++;
                updateHeap3D(src_heap_3D, ind);
              }
            }
          }
          // right
          if (Horizontal && curX < regionX2
              && directions_3D[curL][curY][curX] != Direction::West) {
            const float tmp = d1_3D[curL][curY][curX] + 1;
            const int tmpX = curX + 1;  // the right neighbor

            if (h_edges_3D_[curL][curY][curX].usage
                    < h_edges_3D_[curL][curY][curX].cap
                && net->getMinLayer() <= curL && curL <= net->getMaxLayer()) {
              if (d1_3D[curL][curY][tmpX]
                  >= BIG_INT)  // right neighbor not been put into
                               // src_heap_3D
              {
                d1_3D[curL][curY][tmpX] = tmp;
                pr_3D_[curL][curY][tmpX].l = curL;
                pr_3D_[curL][curY][tmpX].x = curX;
                pr_3D_[curL][curY][tmpX].y = curY;
                directions_3D[curL][curY][tmpX] = Direction::East;
                src_heap_3D.push_back(&d1_3D[curL][curY][tmpX]);
                updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
              } else if (d1_3D[curL][curY][tmpX]
                         > tmp)  // right neighbor been put into src_heap_3D
                                 // but needs update
              {
                d1_3D[curL][curY][tmpX] = tmp;
                pr_3D_[curL][curY][tmpX].l = curL;
                pr_3D_[curL][curY][tmpX].x = curX;
                pr_3D_[curL][curY][tmpX].y = curY;
                directions_3D[curL][curY][tmpX] = Direction::East;
                const int* dtmp = &d1_3D[curL][curY][tmpX];
                int ind = 0;
                while (src_heap_3D[ind] != dtmp)
                  ind++;
                updateHeap3D(src_heap_3D, ind);
              }
            }
          }
        } else {
          // bottom
          if (!Horizontal && curY > regionY1
              && directions_3D[curL][curY][curX] != Direction::South) {
            const float tmp = d1_3D[curL][curY][curX] + 1;
            const int tmpY = curY - 1;  // the bottom neighbor
            if (v_edges_3D_[curL][curY - 1][curX].usage
                    < v_edges_3D_[curL][curY - 1][curX].cap
                && net->getMinLayer() <= curL && curL <= net->getMaxLayer()) {
              if (d1_3D[curL][tmpY][curX]
                  >= BIG_INT)  // bottom neighbor not been put into
                               // src_heap_3D
              {
                d1_3D[curL][tmpY][curX] = tmp;
                pr_3D_[curL][tmpY][curX].l = curL;
                pr_3D_[curL][tmpY][curX].x = curX;
                pr_3D_[curL][tmpY][curX].y = curY;
                directions_3D[curL][tmpY][curX] = Direction::North;
                src_heap_3D.push_back(&d1_3D[curL][tmpY][curX]);
                updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
              } else if (d1_3D[curL][tmpY][curX]
                         > tmp)  // bottom neighbor been put into
                                 // src_heap_3D but needs update
              {
                d1_3D[curL][tmpY][curX] = tmp;
                pr_3D_[curL][tmpY][curX].l = curL;
                pr_3D_[curL][tmpY][curX].x = curX;
                pr_3D_[curL][tmpY][curX].y = curY;
                directions_3D[curL][tmpY][curX] = Direction::North;
                const int* dtmp = &d1_3D[curL][tmpY][curX];
                int ind = 0;
                while (src_heap_3D[ind] != dtmp)
                  ind++;
                updateHeap3D(src_heap_3D, ind);
              }
            }
          }
          // top
          if (!Horizontal && curY < regionY2
              && directions_3D[curL][curY][curX] != Direction::North) {
            const float tmp = d1_3D[curL][curY][curX] + 1;
            const int tmpY = curY + 1;  // the top neighbor
            if (v_edges_3D_[curL][curY][curX].usage
                    < v_edges_3D_[curL][curY][curX].cap
                && net->getMinLayer() <= curL && curL <= net->getMaxLayer()) {
              if (d1_3D[curL][tmpY][curX]
                  >= BIG_INT)  // top neighbor not been put into src_heap_3D
              {
                d1_3D[curL][tmpY][curX] = tmp;
                pr_3D_[curL][tmpY][curX].l = curL;
                pr_3D_[curL][tmpY][curX].x = curX;
                pr_3D_[curL][tmpY][curX].y = curY;
                directions_3D[curL][tmpY][curX] = Direction::South;
                src_heap_3D.push_back(&d1_3D[curL][tmpY][curX]);
                updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
              } else if (d1_3D[curL][tmpY][curX]
                         > tmp)  // top neighbor been put into src_heap_3D
                                 // but needs update
              {
                d1_3D[curL][tmpY][curX] = tmp;
                pr_3D_[curL][tmpY][curX].l = curL;
                pr_3D_[curL][tmpY][curX].x = curX;
                pr_3D_[curL][tmpY][curX].y = curY;
                directions_3D[curL][tmpY][curX] = Direction::South;
                const int* dtmp = &d1_3D[curL][tmpY][curX];
                int ind = 0;
                while (src_heap_3D[ind] != dtmp)
                  ind++;
                updateHeap3D(src_heap_3D, ind);
              }
            }
          }
        }

        // down
        if (curL > 0 && directions_3D[curL][curY][curX] != Direction::Up) {
          const float tmp = d1_3D[curL][curY][curX] + via_cost_;
          const int tmpL = curL - 1;  // the bottom neighbor

          if (d1_3D[tmpL][curY][curX]
              >= BIG_INT)  // bottom neighbor not been put into src_heap_3D
          {
            d1_3D[tmpL][curY][curX] = tmp;
            pr_3D_[tmpL][curY][curX].l = curL;
            pr_3D_[tmpL][curY][curX].x = curX;
            pr_3D_[tmpL][curY][curX].y = curY;
            directions_3D[tmpL][curY][curX] = Direction::Down;
            src_heap_3D.push_back(&d1_3D[tmpL][curY][curX]);
            updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
          } else if (d1_3D[tmpL][curY][curX]
                     > tmp)  // bottom neighbor been put into src_heap_3D
                             // but needs update
          {
            d1_3D[tmpL][curY][curX] = tmp;
            pr_3D_[tmpL][curY][curX].l = curL;
            pr_3D_[tmpL][curY][curX].x = curX;
            pr_3D_[tmpL][curY][curX].y = curY;
            directions_3D[tmpL][curY][curX] = Direction::Down;
            const int* dtmp = &d1_3D[tmpL][curY][curX];
            int ind = 0;
            while (src_heap_3D[ind] != dtmp)
              ind++;
            updateHeap3D(src_heap_3D, ind);
          }
        }

        // up
        if (curL < num_layers_ - 1
            && directions_3D[curL][curY][curX] != Direction::Down) {
          const float tmp = d1_3D[curL][curY][curX] + via_cost_;
          const int tmpL = curL + 1;  // the bottom neighbor
          if (d1_3D[tmpL][curY][curX]
              >= BIG_INT)  // bottom neighbor not been put into src_heap_3D
          {
            d1_3D[tmpL][curY][curX] = tmp;
            pr_3D_[tmpL][curY][curX].l = curL;
            pr_3D_[tmpL][curY][curX].x = curX;
            pr_3D_[tmpL][curY][curX].y = curY;
            directions_3D[tmpL][curY][curX] = Direction::Up;
            src_heap_3D.push_back(&d1_3D[tmpL][curY][curX]);
            updateHeap3D(src_heap_3D, src_heap_3D.size() - 1);
          } else if (d1_3D[tmpL][curY][curX]
                     > tmp)  // bottom neighbor been put into src_heap_3D
                             // but needs update
          {
            d1_3D[tmpL][curY][curX] = tmp;
            pr_3D_[tmpL][curY][curX].l = curL;
            pr_3D_[tmpL][curY][curX].x = curX;
            pr_3D_[tmpL][curY][curX].y = curY;
            directions_3D[tmpL][curY][curX] = Direction::Up;
            const int* dtmp = &d1_3D[tmpL][curY][curX];
            int ind = 0;
            while (src_heap_3D[ind] != dtmp)
              ind++;
            updateHeap3D(src_heap_3D, ind);
          }
        }

        if (src_heap_3D.empty()) {
          logger_->error(GRT,
                         183,
                         "Net {}: heap underflow during 3D maze routing.",
                         nets_[netID]->getName());
        }
        // update ind1 for next loop
        ind1 = (src_heap_3D[0] - &d1_3D[0][0][0]);
      }  // while loop

      for (int i = 0; i < dest_heap_3D.size(); i++)
        pop_heap2_3D[dest_heap_3D[i] - &d2_3D[0][0][0]] = false;

      // get the new route for the edge and store it in gridsX[] and
      // gridsY[] temporarily

      const int crossL = ind1 / (grid_hv_);
      const int crossX = (ind1 % (grid_hv_)) % x_range_;
      const int crossY = (ind1 % (grid_hv_)) / x_range_;

      int cnt = 0;
      int curX = crossX;
      int curY = crossY;
      int curL = crossL;

      if (d1_3D[curL][curY][curX] == 0) {
        recoverEdge(netID, edgeID);
        break;
      }

      std::vector<int> tmp_gridsX, tmp_gridsY, tmp_gridsL;

      while (d1_3D[curL][curY][curX] != 0)  // loop until reach subtree1
      {
        const int tmpL = pr_3D_[curL][curY][curX].l;
        const int tmpX = pr_3D_[curL][curY][curX].x;
        const int tmpY = pr_3D_[curL][curY][curX].y;
        curX = tmpX;
        curY = tmpY;
        curL = tmpL;
        fflush(stdout);
        tmp_gridsX.push_back(curX);
        tmp_gridsY.push_back(curY);
        tmp_gridsL.push_back(curL);
        cnt++;
      }

      std::vector<int> gridsX(tmp_gridsX.rbegin(), tmp_gridsX.rend());
      std::vector<int> gridsY(tmp_gridsY.rbegin(), tmp_gridsY.rend());
      std::vector<int> gridsL(tmp_gridsL.rbegin(), tmp_gridsL.rend());

      // add the connection point (crossX, crossY)
      gridsX.push_back(crossX);
      gridsY.push_back(crossY);
      gridsL.push_back(crossL);
      cnt++;

      curX = crossX;
      curY = crossY;
      curL = crossL;

      const int cnt_n1n2 = cnt;

      const int E1x = gridsX[0];
      const int E1y = gridsY[0];
      const int E2x = gridsX.back();
      const int E2y = gridsY.back();

      int headRoom = 0;
      int origL = gridsL[0];

      while (headRoom < gridsX.size() && gridsX[headRoom] == E1x
             && gridsY[headRoom] == E1y) {
        headRoom++;
      }
      if (headRoom > 0) {
        headRoom--;
      }

      int lastL = gridsL[headRoom];

      // change the tree structure according to the new routing for the tree
      // edge find E1 and E2, and the endpoints of the edges they are on

      const int edge_n1n2 = edgeID;
      // (1) consider subtree1
      if (n1 < num_terminals && (E1x != n1x || E1y != n1y)) {
        // split neighbor edge and return id new node
        n1 = splitEdge(treeedges, treenodes, n2, n1, edgeID);
        // calculate TreeNode variables for new node
        setTreeNodesVariables(netID);
      }
      if (n1 >= num_terminals && (E1x != n1x || E1y != n1y))
      // n1 is not a pin and E1!=n1, then make change to subtree1,
      // otherwise, no change to subtree1
      {
        n1Shift = true;
        const int corE1 = corr_edge_3D[origL][E1y][E1x];

        const int endpt1 = treeedges[corE1].n1;
        const int endpt2 = treeedges[corE1].n2;

        // find A1, A2 and edge_n1A1, edge_n1A2
        int edge_n1A1, edge_n1A2;
        int A1, A2;
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
          updateRouteType13D(netID,
                             treenodes,
                             n1,
                             A1,
                             A2,
                             E1x,
                             E1y,
                             treeedges,
                             edge_n1A1,
                             edge_n1A2);

          // update position for n1

          // treenodes[n1].l = E1l;
          treenodes[n1].assigned = true;
        }     // if E1 is on (n1, A1) or (n1, A2)
        else  // E1 is not on (n1, A1) or (n1, A2), but on (C1, C2)
        {
          const int C1 = endpt1;
          const int C2 = endpt2;
          const int edge_C1C2 = corr_edge_3D[origL][E1y][E1x];

          // update route for edge (n1, C1), (n1, C2) and (A1, A2)
          updateRouteType23D(netID,
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
          treenodes[n1].assigned = true;
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
      else {
        newUpdateNodeLayers(treenodes, edge_n1n2, n1a, lastL);
      }

      origL = gridsL[cnt_n1n2 - 1];
      int tailRoom = cnt_n1n2 - 1;

      while (tailRoom > 0 && gridsX[tailRoom] == E2x
             && gridsY[tailRoom] == E2y) {
        tailRoom--;
      }
      if (tailRoom < cnt_n1n2 - 1) {
        tailRoom++;
      }

      lastL = gridsL[tailRoom];

      // (2) consider subtree2
      if (n2 < num_terminals && (E2x != n2x || E2y != n2y)) {
        // split neighbor edge and return id new node
        n2 = splitEdge(treeedges, treenodes, n1, n2, edgeID);
        // calculate TreeNode variables for new node
        setTreeNodesVariables(netID);
      }
      if (n2 >= num_terminals && (E2x != n2x || E2y != n2y))
      // n2 is not a pin and E2!=n2, then make change to subtree2,
      // otherwise, no change to subtree2
      {
        // find the endpoints of the edge E1 is on

        n2Shift = true;
        const int corE2 = corr_edge_3D[origL][E2y][E2x];
        const int endpt1 = treeedges[corE2].n1;
        const int endpt2 = treeedges[corE2].n2;

        // find B1, B2
        int edge_n2B1, edge_n2B2;
        int B1, B2;
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
          updateRouteType13D(netID,
                             treenodes,
                             n2,
                             B1,
                             B2,
                             E2x,
                             E2y,
                             treeedges,
                             edge_n2B1,
                             edge_n2B2);

          // update position for n2
          treenodes[n2].assigned = true;
        }     // if E2 is on (n2, B1) or (n2, B2)
        else  // E2 is not on (n2, B1) or (n2, B2), but on (d1_3D, d2_3D)
        {
          const int D1 = endpt1;
          const int D2 = endpt2;
          const int edge_D1D2 = corr_edge_3D[origL][E2y][E2x];

          // update route for edge (n2, d1_3D), (n2, d2_3D) and (B1, B2)
          updateRouteType23D(netID,
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
          treenodes[n2].assigned = true;
          // update 3 edges (n2, B1)->(d1_3D, n2), (n2, B2)->(n2, d2_3D),
          // (d1_3D, d2_3D)->(B1, B2)
          const int edge_n2D1 = edge_n2B1;
          treeedges[edge_n2D1].n1 = D1;
          treeedges[edge_n2D1].n2 = n2;
          const int edge_n2D2 = edge_n2B2;
          treeedges[edge_n2D2].n1 = n2;
          treeedges[edge_n2D2].n2 = D2;
          const int edge_B1B2 = edge_D1D2;
          treeedges[edge_B1B2].n1 = B1;
          treeedges[edge_B1B2].n2 = B2;
          // update nbr and edge for 5 nodes n2, B1, B2, d1_3D, d2_3D
          // n1's nbr (n1, B1, B2)->(n1, d1_3D, d2_3D)
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
        }     // else E2 is not on (n2, B1) or (n2, B2), but on (d1_3D,
              // d2_3D)
      } else  // n2 is not a pin and E2!=n2
      {
        newUpdateNodeLayers(treenodes, edge_n1n2, n2a, lastL);
      }

      const int newcnt_n1n2 = tailRoom - headRoom + 1;

      // update route for edge (n1, n2) and edge usage
      if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
        treeedges[edge_n1n2].route.gridsX.clear();
        treeedges[edge_n1n2].route.gridsY.clear();
        treeedges[edge_n1n2].route.gridsL.clear();
      }

      // avoid resizing vector with negative value.
      // this may happen when all elements of gridsX and gridsY are the
      // same.
      if (newcnt_n1n2 > 0) {
        treeedges[edge_n1n2].route.gridsX.resize(newcnt_n1n2, 0);
        treeedges[edge_n1n2].route.gridsY.resize(newcnt_n1n2, 0);
        treeedges[edge_n1n2].route.gridsL.resize(newcnt_n1n2, 0);
      }
      treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
      treeedges[edge_n1n2].route.routelen = newcnt_n1n2 - 1;
      treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y - E2y);

      int j = headRoom;
      for (int i = 0; i < newcnt_n1n2; i++) {
        treeedges[edge_n1n2].route.gridsX[i] = gridsX[j];
        treeedges[edge_n1n2].route.gridsY[i] = gridsY[j];
        treeedges[edge_n1n2].route.gridsL[i] = gridsL[j];
        j++;
      }

      // update edge usage
      for (int i = headRoom; i < tailRoom; i++) {
        if (gridsL[i] == gridsL[i + 1]) {
          if (gridsX[i] == gridsX[i + 1])  // a vertical edge
          {
            const int min_y = std::min(gridsY[i], gridsY[i + 1]);
            v_edges_[min_y][gridsX[i]].usage += net->getEdgeCost();
            v_used_ggrid_.insert(std::make_pair(min_y, gridsX[i]));
            v_edges_3D_[gridsL[i]][min_y][gridsX[i]].usage
                += net->getLayerEdgeCost(gridsL[i]);
          } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
          {
            const int min_x = std::min(gridsX[i], gridsX[i + 1]);
            h_edges_[gridsY[i]][min_x].usage += net->getEdgeCost();
            h_used_ggrid_.insert(std::make_pair(gridsY[i], min_x));
            h_edges_3D_[gridsL[i]][gridsY[i]][min_x].usage
                += net->getLayerEdgeCost(gridsL[i]);
          }
        }
      }

      if (!n1Shift && !n2Shift) {
        continue;
      }
      setTreeNodesVariables(netID);
    }
  }
}

}  // namespace grt
