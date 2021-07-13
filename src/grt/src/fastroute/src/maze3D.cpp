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
#include <time.h>

#include <algorithm>

#include "DataType.h"
#include "FastRoute.h"
#include "flute.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

#define PARENT(i) (i - 1) / 2
#define LEFT(i) 2 * i + 1
#define RIGHT(i) 2 * i + 2

// non recursive version of heapify-
static void heapify3D(int** array, int heapSize, int i)
{
  int l, r, smallest;
  int* tmp;
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

static void updateHeap3D(int** array, int arrayLen, int i)
{
  int parent;
  int* tmpi;

  tmpi = array[i];
  while (i > 0 && *(array[PARENT(i)]) > *tmpi) {
    parent = PARENT(i);
    array[i] = array[parent];
    i = parent;
  }
  array[i] = tmpi;
}

// extract the entry with minimum distance from Priority queue
static void extractMin3D(int** array, int arrayLen)
{
  array[0] = array[arrayLen - 1];
  heapify3D(array, arrayLen - 1, 0);
}

void FastRouteCore::setupHeap3D(int netID,
                 int edgeID,
                 int* heapLen1,
                 int* heapLen2,
                 int regionX1,
                 int regionX2,
                 int regionY1,
                 int regionY2)
{
  int nt, nbr, nbrX, nbrY, cur, edge;
  int x_grid, y_grid, l_grid, heapcnt;
  int queuehead, queuetail;
  Route* route;

  TreeEdge* treeedges = sttrees_[netID].edges;
  TreeNode* treenodes = sttrees_[netID].nodes;

  int d = sttrees_[netID].deg;
  // TODO: check this size
  int numNodes = 2 * d - 2;
  int heapVisited[numNodes];
  int heapQueue[numNodes];

  // TODO: check this size
  int n1 = treeedges[edgeID].n1;
  int n2 = treeedges[edgeID].n2;
  int x1 = treenodes[n1].x;
  int y1 = treenodes[n1].y;
  int x2 = treenodes[n2].x;
  int y2 = treenodes[n2].y;

  if (d == 2)  // 2-pin net
  {
    d1_3D_[0][y1][x1] = 0;
    directions_3D_[0][y1][x1] = ORIGIN;
    heap1_3D_[0] = &d1_3D_[0][y1][x1];
    *heapLen1 = 1;
    d2_3D_[0][y2][x2] = 0;
    directions_3D_[0][y2][x2] = ORIGIN;
    heap2_3D_[0] = &d2_3D_[0][y2][x2];
    *heapLen2 = 1;
  } else  // net with more than 2 pins
  {
    for (int i = regionY1; i <= regionY2; i++) {
      for (int j = regionX1; j <= regionX2; j++) {
        in_region_[i][j] = true;
      }
    }

    for (int i = 0; i < numNodes; i++)
      heapVisited[i] = false;

    // find all the grids on tree edges in subtree t1 (connecting to n1) and put
    // them into heap1_3D_
    if (n1 < d)  // n1 is a Pin node
    {
      // just need to put n1 itself into heap1_3D_
      heapcnt = 0;

      nt = treenodes[n1].stackAlias;

      for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
        d1_3D_[l][y1][x1] = 0;
        heap1_3D_[heapcnt] = &d1_3D_[l][y1][x1];
        directions_3D_[l][y1][x1] = ORIGIN;
        heapVisited[n1] = true;
        heapcnt++;
      }
      *heapLen1 = heapcnt;

    } else  // n1 is a Steiner node
    {
      heapcnt = 0;
      queuehead = queuetail = 0;

      nt = treenodes[n1].stackAlias;

      // add n1 into heap1_3D_
      for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
        d1_3D_[l][y1][x1] = 0;
        directions_3D_[l][y1][x1] = ORIGIN;
        heap1_3D_[heapcnt] = &d1_3D_[l][y1][x1];
        heapVisited[n1] = true;
        heapcnt++;
      }

      // add n1 into the heapQueue
      heapQueue[queuetail] = n1;
      queuetail++;

      // loop to find all the edges in subtree t1
      while (queuetail > queuehead) {
        // get cur node from the queuehead
        cur = heapQueue[queuehead];
        queuehead++;
        heapVisited[cur] = true;
        if (cur >= d)  // cur node is a Steiner node
        {
          for (int i = 0; i < 3; i++) {
            nbr = treenodes[cur].nbr[i];
            edge = treenodes[cur].edge[i];
            if (nbr != n2)  // not n2
            {
              if (heapVisited[nbr] == false) {
                // put all the grids on the two adjacent tree edges into heap1_3D_
                if (treeedges[edge].route.routelen > 0)  // not a degraded edge
                {
                  // put nbr into heap1_3D_ if in enlarged region
                  if (in_region_[treenodes[nbr].y][treenodes[nbr].x]) {
                    nbrX = treenodes[nbr].x;
                    nbrY = treenodes[nbr].y;
                    nt = treenodes[nbr].stackAlias;
                    for (int l = treenodes[nt].botL; l <= treenodes[nt].topL;
                         l++) {
                      d1_3D_[l][nbrY][nbrX] = 0;
                      directions_3D_[l][nbrY][nbrX] = ORIGIN;
                      heap1_3D_[heapcnt] = &d1_3D_[l][nbrY][nbrX];
                      heapcnt++;
                      corr_edge_3D_[l][nbrY][nbrX] = edge;
                    }
                  }

                  // the coordinates of two end nodes of the edge

                  route = &(treeedges[edge].route);
                  if (route->type == RouteType::MazeRoute) {
                    for (int j = 1; j < route->routelen;
                         j++)  // don't put edge_n1 and edge_n2 into heap1_3D_
                    {
                      x_grid = route->gridsX[j];
                      y_grid = route->gridsY[j];
                      l_grid = route->gridsL[j];

                      if (in_region_[y_grid][x_grid]) {
                        d1_3D_[l_grid][y_grid][x_grid] = 0;
                        heap1_3D_[heapcnt] = &d1_3D_[l_grid][y_grid][x_grid];
                        directions_3D_[l_grid][y_grid][x_grid] = ORIGIN;
                        heapcnt++;
                        corr_edge_3D_[l_grid][y_grid][x_grid] = edge;
                      }
                    }

                  }  // if MazeRoute
                }    // if not a degraded edge (len>0)

                // add the neighbor of cur node into heapQueue
                heapQueue[queuetail] = nbr;
                queuetail++;
              }             // if the node is not heapVisited
            }               // if nbr!=n2
          }                 // loop i (3 neigbors for cur node)
        }                   // if cur node is a Steiner nodes
      }                     // while heapQueue is not empty
      *heapLen1 = heapcnt;  // record the length of heap1_3D_
    }                       // else n1 is not a Pin node

    // find all the grids on subtree t2 (connect to n2) and put them into
    // heap2_3D_ find all the grids on tree edges in subtree t2 (connecting to n2)
    // and put them into heap2_3D_
    if (n2 < d)  // n2 is a Pin node
    {
      nt = treenodes[n2].stackAlias;
      //*heapLen2 = 0;
      heapcnt = 0;

      for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
        // just need to put n1 itself into heap1_3D_
        d2_3D_[l][y2][x2] = 0;
        directions_3D_[l][y2][x2] = ORIGIN;
        heap2_3D_[heapcnt] = &d2_3D_[l][y2][x2];
        heapVisited[n2] = true;
        //*heapLen2 += 1;
        heapcnt++;
      }
      *heapLen2 = heapcnt;
    } else  // n2 is a Steiner node
    {
      heapcnt = 0;
      queuehead = queuetail = 0;

      nt = treenodes[n2].stackAlias;
      // add n2 into heap2_3D_
      for (int l = treenodes[nt].botL; l <= treenodes[nt].topL; l++) {
        d2_3D_[l][y2][x2] = 0;
        directions_3D_[l][y2][x2] = ORIGIN;
        heap2_3D_[heapcnt] = &d2_3D_[l][y2][x2];
        heapcnt++;
      }
      heapVisited[n2] = true;

      // add n2 into the heapQueue
      heapQueue[queuetail] = n2;
      queuetail++;

      // loop to find all the edges in subtree t2
      while (queuetail > queuehead) {
        // get cur node form queuehead
        cur = heapQueue[queuehead];
        heapVisited[cur] = true;
        queuehead++;

        if (cur >= d)  // cur node is a Steiner node
        {
          for (int i = 0; i < 3; i++) {
            nbr = treenodes[cur].nbr[i];
            edge = treenodes[cur].edge[i];
            if (nbr != n1)  // not n1
            {
              if (heapVisited[nbr] == false) {
                // put all the grids on the two adjacent tree edges into heap2_3D_
                if (treeedges[edge].route.routelen > 0)  // not a degraded edge
                {
                  // put nbr into heap2_3D_
                  if (in_region_[treenodes[nbr].y][treenodes[nbr].x]) {
                    nbrX = treenodes[nbr].x;
                    nbrY = treenodes[nbr].y;
                    nt = treenodes[nbr].stackAlias;
                    for (int l = treenodes[nt].botL; l <= treenodes[nt].topL;
                         l++) {
                      // nbrL = treenodes[nbr].l;

                      d2_3D_[l][nbrY][nbrX] = 0;
                      directions_3D_[l][nbrY][nbrX] = ORIGIN;
                      heap2_3D_[heapcnt] = &d2_3D_[l][nbrY][nbrX];
                      heapcnt++;
                      corr_edge_3D_[l][nbrY][nbrX] = edge;
                    }
                  }

                  // the coordinates of two end nodes of the edge

                  route = &(treeedges[edge].route);
                  if (route->type == RouteType::MazeRoute) {
                    for (int j = 1; j < route->routelen;
                         j++)  // don't put edge_n1 and edge_n2 into heap2_3D_
                    {
                      x_grid = route->gridsX[j];
                      y_grid = route->gridsY[j];
                      l_grid = route->gridsL[j];
                      if (in_region_[y_grid][x_grid]) {
                        d2_3D_[l_grid][y_grid][x_grid] = 0;
                        directions_3D_[l_grid][y_grid][x_grid] = ORIGIN;
                        heap2_3D_[heapcnt] = &d2_3D_[l_grid][y_grid][x_grid];
                        heapcnt++;

                        corr_edge_3D_[l_grid][y_grid][x_grid] = edge;
                      }
                    }

                  }  // if MazeRoute
                }    // if the edge is not degraded (len>0)

                // add the neighbor of cur node into heapQueue
                heapQueue[queuetail] = nbr;
                queuetail++;
              }             // if the node is not heapVisited
            }               // if nbr!=n1
          }                 // loop i (3 neigbors for cur node)
        }                   // if cur node is a Steiner nodes
      }                     // while heapQueue is not empty
      *heapLen2 = heapcnt;  // record the length of heap2_3D_
    }                       // else n2 is not a Pin node

    for (int i = regionY1; i <= regionY2; i++) {
      for (int j = regionX1; j <= regionX2; j++) {
        in_region_[i][j] = false;
      }
    }
  }  // net with more than two pins
}

void FastRouteCore::newUpdateNodeLayers(TreeNode* treenodes, int edgeID, int n1, int lastL)
{
  int con;

  con = treenodes[n1].conCNT;

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

int FastRouteCore::copyGrids3D(TreeNode* treenodes,
                int n1,
                int n2,
                TreeEdge* treeedges,
                int edge_n1n2,
                int gridsX_n1n2[],
                int gridsY_n1n2[],
                int gridsL_n1n2[])
{
  int i, cnt;
  int n1x, n1y, n1l;

  n1x = treenodes[n1].x;
  n1y = treenodes[n1].y;
  n1l = treenodes[n1].botL;

  cnt = 0;
  if (treeedges[edge_n1n2].n1 == n1)  // n1 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.routelen > 0) {
      for (i = 0; i <= treeedges[edge_n1n2].route.routelen; i++) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        gridsL_n1n2[cnt] = treeedges[edge_n1n2].route.gridsL[i];
        cnt++;
      }
    }  // MazeRoute
    else
    // NoRoute
    {
      fflush(stdout);
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      gridsL_n1n2[cnt] = n1l;
      cnt++;
    }
  }     // if n1 is the first node of (n1, n2)
  else  // n2 is the first node of (n1, n2)
  {
    if (treeedges[edge_n1n2].route.routelen > 0) {
      for (i = treeedges[edge_n1n2].route.routelen; i >= 0; i--) {
        gridsX_n1n2[cnt] = treeedges[edge_n1n2].route.gridsX[i];
        gridsY_n1n2[cnt] = treeedges[edge_n1n2].route.gridsY[i];
        gridsL_n1n2[cnt] = treeedges[edge_n1n2].route.gridsL[i];
        cnt++;
      }
    }     // MazeRoute
    else  // NoRoute
    {
      gridsX_n1n2[cnt] = n1x;
      gridsY_n1n2[cnt] = n1y;
      gridsL_n1n2[cnt] = n1l;
      cnt++;
    }  // MazeRoute
  }

  return (cnt);
}

void FastRouteCore::updateRouteType13D(int netID,
                        TreeNode* treenodes,
                        int n1,
                        int A1,
                        int A2,
                        int E1x,
                        int E1y,
                        TreeEdge* treeedges,
                        int edge_n1A1,
                        int edge_n1A2)
{
  int i, l, cnt, A1x, A1y, A2x, A2y;
  int cnt_n1A1, cnt_n1A2, E1_pos1, E1_pos2;
  int gridsX_n1A1[MAXLEN], gridsY_n1A1[MAXLEN], gridsL_n1A1[MAXLEN],
      gridsX_n1A2[MAXLEN], gridsY_n1A2[MAXLEN], gridsL_n1A2[MAXLEN];

  A1x = treenodes[A1].x;
  A1y = treenodes[A1].y;
  A2x = treenodes[A2].x;
  A2y = treenodes[A2].y;

  // copy all the grids on (n1, A1) and (n2, A2) to tmp arrays, and keep the
  // grids order A1->n1->A2 copy (n1, A1)
  cnt_n1A1 = copyGrids3D(treenodes,
                         A1,
                         n1,
                         treeedges,
                         edge_n1A1,
                         gridsX_n1A1,
                         gridsY_n1A1,
                         gridsL_n1A1);

  // copy (n1, A2)
  cnt_n1A2 = copyGrids3D(treenodes,
                         n1,
                         A2,
                         treeedges,
                         edge_n1A2,
                         gridsX_n1A2,
                         gridsY_n1A2,
                         gridsL_n1A2);

  if (cnt_n1A1 == 1) {
    logger_->error(GRT, 187, "In 3D maze routing, type 1 node shift, cnt_n1A1 is 1.");
  }

  E1_pos1 = -1;
  for (i = 0; i < cnt_n1A1; i++) {
    if (gridsX_n1A1[i] == E1x && gridsY_n1A1[i] == E1y)  // reach the E1
    {
      E1_pos1 = i;
      break;
    }
  }

  if (E1_pos1 == -1) {
    logger_->error(GRT, 171, "Invalid index for position ({}, {}).", E1x, E1y);
  }

  for (i = cnt_n1A1 - 1; i >= 0; i--) {
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
    free(treeedges[edge_n1A1].route.gridsX);
    free(treeedges[edge_n1A1].route.gridsY);
    free(treeedges[edge_n1A1].route.gridsL);
  }
  treeedges[edge_n1A1].route.gridsX
      = (short*) calloc((E1_pos1 + 1), sizeof(short));
  treeedges[edge_n1A1].route.gridsY
      = (short*) calloc((E1_pos1 + 1), sizeof(short));
  treeedges[edge_n1A1].route.gridsL
      = (short*) calloc((E1_pos1 + 1), sizeof(short));

  if (A1x <= E1x) {
    cnt = 0;
    for (i = 0; i <= E1_pos1; i++) {
      treeedges[edge_n1A1].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A1].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A1].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    treeedges[edge_n1A1].n1 = A1;
    treeedges[edge_n1A1].n2 = n1;
  } else {
    cnt = 0;
    for (i = E1_pos1; i >= 0; i--) {
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
      && treeedges[edge_n1A2].route.routelen
             > 0)  // if originally allocated, free them first
  {
    free(treeedges[edge_n1A2].route.gridsX);
    free(treeedges[edge_n1A2].route.gridsY);
    free(treeedges[edge_n1A2].route.gridsL);
  }

  if (cnt_n1A2 > 1) {
    treeedges[edge_n1A2].route.gridsX
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
                           + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0])),
                          sizeof(short));
    treeedges[edge_n1A2].route.gridsY
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
                           + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0])),
                          sizeof(short));
    treeedges[edge_n1A2].route.gridsL
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1
                           + abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0])),
                          sizeof(short));
  } else {
    treeedges[edge_n1A2].route.gridsX
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1), sizeof(short));
    treeedges[edge_n1A2].route.gridsY
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1), sizeof(short));
    treeedges[edge_n1A2].route.gridsL
        = (short*) calloc((cnt_n1A1 + cnt_n1A2 - E1_pos2 - 1), sizeof(short));
  }

  if (E1x <= A2x) {
    cnt = 0;
    for (i = E1_pos2; i < cnt_n1A1; i++) {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A1[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A1[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A1[i];
      cnt++;
    }
    if (cnt_n1A2 > 1) {
      if (gridsL_n1A1[cnt_n1A1 - 1] > gridsL_n1A2[0]) {
        for (l = gridsL_n1A1[cnt_n1A1 - 1] - 1; l >= gridsL_n1A2[0]; l--) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      } else if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (l = gridsL_n1A1[cnt_n1A1 - 1] + 1; l <= gridsL_n1A2[0]; l++) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      }
    }

    for (i = 1; i < cnt_n1A2; i++)  // 0 is n1 again, so no repeat
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
    for (i = cnt_n1A2 - 1; i >= 1; i--)  // 0 is n1 again, so no repeat
    {
      treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      treeedges[edge_n1A2].route.gridsL[cnt] = gridsL_n1A2[i];
      cnt++;
    }

    if (cnt_n1A2 > 1) {
      if (gridsL_n1A1[cnt_n1A1 - 1] > gridsL_n1A2[0]) {
        for (l = gridsL_n1A2[0]; l < gridsL_n1A1[cnt_n1A1 - 1]; l++) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      } else if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (l = gridsL_n1A2[0]; l > gridsL_n1A1[cnt_n1A1 - 1]; l--) {
          treeedges[edge_n1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_n1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_n1A2].route.gridsL[cnt] = l;
          cnt++;
        }
      }
    }
    for (i = cnt_n1A1 - 1; i >= E1_pos2; i--) {
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
                        TreeNode* treenodes,
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
  int i, cnt, A1x, A1y, A2x, A2y, C1x, C1y, C2x, C2y, extraLen, startIND;
  int edge_n1C1, edge_n1C2, edge_A1A2;
  int cnt_n1A1, cnt_n1A2, cnt_C1C2, E1_pos1, E1_pos2;
  int len_A1A2, len_n1C1, len_n1C2;
  int gridsX_n1A1[MAXLEN], gridsY_n1A1[MAXLEN], gridsL_n1A1[MAXLEN];
  int gridsX_n1A2[MAXLEN], gridsY_n1A2[MAXLEN], gridsL_n1A2[MAXLEN];
  int gridsX_C1C2[MAXLEN], gridsY_C1C2[MAXLEN], gridsL_C1C2[MAXLEN];

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

  // copy (A1, n1)
  cnt_n1A1 = copyGrids3D(treenodes,
                         A1,
                         n1,
                         treeedges,
                         edge_n1A1,
                         gridsX_n1A1,
                         gridsY_n1A1,
                         gridsL_n1A1);

  // copy (n1, A2)
  cnt_n1A2 = copyGrids3D(treenodes,
                         n1,
                         A2,
                         treeedges,
                         edge_n1A2,
                         gridsX_n1A2,
                         gridsY_n1A2,
                         gridsL_n1A2);

  // copy all the grids on (C1, C2) to gridsX_C1C2[] and gridsY_C1C2[]
  cnt_C1C2 = copyGrids3D(treenodes,
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
    free(treeedges[edge_A1A2].route.gridsX);
    free(treeedges[edge_A1A2].route.gridsY);
    free(treeedges[edge_A1A2].route.gridsL);
  }
  len_A1A2 = cnt_n1A1 + cnt_n1A2 - 1;

  if (len_A1A2 == 1) {
    treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
    treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);
  } else {
    extraLen = 0;
    if (cnt_n1A1 > 1 && cnt_n1A2 > 1) {
      extraLen = abs(gridsL_n1A1[cnt_n1A1 - 1] - gridsL_n1A2[0]);
      len_A1A2 += extraLen;
    }
    treeedges[edge_A1A2].route.gridsX
        = (short*) calloc(len_A1A2, sizeof(short));
    treeedges[edge_A1A2].route.gridsY
        = (short*) calloc(len_A1A2, sizeof(short));
    treeedges[edge_A1A2].route.gridsL
        = (short*) calloc(len_A1A2, sizeof(short));
    treeedges[edge_A1A2].route.routelen = len_A1A2 - 1;
    treeedges[edge_A1A2].len = abs(A1x - A2x) + abs(A1y - A2y);

    cnt = 0;
    startIND = 0;

    if (cnt_n1A1 > 1) {
      startIND = 1;
      for (i = 0; i < cnt_n1A1; i++) {
        treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A1[i];
        treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A1[i];
        treeedges[edge_A1A2].route.gridsL[cnt] = gridsL_n1A1[i];
        cnt++;
      }
    }

    if (extraLen > 0) {
      if (gridsL_n1A1[cnt_n1A1 - 1] < gridsL_n1A2[0]) {
        for (i = gridsL_n1A1[cnt_n1A1 - 1] + 1; i <= gridsL_n1A2[0]; i++) {
          treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_A1A2].route.gridsL[cnt] = i;
          cnt++;
        }
      } else {
        for (i = gridsL_n1A1[cnt_n1A1 - 1] - 1; i >= gridsL_n1A2[1]; i--) {
          treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[0];
          treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[0];
          treeedges[edge_A1A2].route.gridsL[cnt] = i;
          cnt++;
        }
      }
    }

    for (i = startIND; i < cnt_n1A2; i++)  // do not repeat point n1
    {
      treeedges[edge_A1A2].route.gridsX[cnt] = gridsX_n1A2[i];
      treeedges[edge_A1A2].route.gridsY[cnt] = gridsY_n1A2[i];
      treeedges[edge_A1A2].route.gridsL[cnt] = gridsL_n1A2[i];
      cnt++;
    }
  }

  if (cnt_C1C2 == 1) {
    logger_->warn(GRT, 184, "Shift to 0 length edge, type2.");
  }

  // find the index of E1 in (C1, C2)
  for (i = 0; i < cnt_C1C2; i++) {
    if (gridsX_C1C2[i] == E1x && gridsY_C1C2[i] == E1y) {
      E1_pos1 = i;
      break;
    }
  }

  E1_pos2 = -1;

  for (i = cnt_C1C2 - 1; i >= 0; i--) {
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
    free(treeedges[edge_n1C1].route.gridsX);
    free(treeedges[edge_n1C1].route.gridsY);
    free(treeedges[edge_n1C1].route.gridsL);
  }
  len_n1C1 = E1_pos1 + 1;

  treeedges[edge_n1C1].route.gridsX = (short*) calloc(len_n1C1, sizeof(short));
  treeedges[edge_n1C1].route.gridsY = (short*) calloc(len_n1C1, sizeof(short));
  treeedges[edge_n1C1].route.gridsL = (short*) calloc(len_n1C1, sizeof(short));
  treeedges[edge_n1C1].route.routelen = len_n1C1 - 1;
  treeedges[edge_n1C1].len = abs(C1x - E1x) + abs(C1y - E1y);

  if (treeedges[edge_n1C2].route.type == RouteType::MazeRoute
      && treeedges[edge_n1C2].route.routelen > 0) {
    free(treeedges[edge_n1C2].route.gridsX);
    free(treeedges[edge_n1C2].route.gridsY);
    free(treeedges[edge_n1C2].route.gridsL);
  }
  len_n1C2 = cnt_C1C2 - E1_pos2;

  treeedges[edge_n1C2].route.gridsX = (short*) calloc(len_n1C2, sizeof(short));
  treeedges[edge_n1C2].route.gridsY = (short*) calloc(len_n1C2, sizeof(short));
  treeedges[edge_n1C2].route.gridsL = (short*) calloc(len_n1C2, sizeof(short));
  treeedges[edge_n1C2].route.routelen = len_n1C2 - 1;
  treeedges[edge_n1C2].len = abs(C2x - E1x) + abs(C2y - E1y);

  // split original (C1, C2) to (C1, n1) and (n1, C2)
  cnt = 0;
  for (i = 0; i <= E1_pos1; i++) {
    treeedges[edge_n1C1].route.gridsX[i] = gridsX_C1C2[i];
    treeedges[edge_n1C1].route.gridsY[i] = gridsY_C1C2[i];
    treeedges[edge_n1C1].route.gridsL[i] = gridsL_C1C2[i];
    cnt++;
  }

  cnt = 0;
  for (i = E1_pos2; i < cnt_C1C2; i++) {
    treeedges[edge_n1C2].route.gridsX[cnt] = gridsX_C1C2[i];
    treeedges[edge_n1C2].route.gridsY[cnt] = gridsY_C1C2[i];
    treeedges[edge_n1C2].route.gridsL[cnt] = gridsL_C1C2[i];
    cnt++;
  }
}

void FastRouteCore::mazeRouteMSMDOrder3D(int expand, int ripupTHlb, int ripupTHub, int layerOrientation)
{
  short* gridsLtmp;
  int netID, enlarge, endIND;
  std::vector<bool> pop_heap2_3D_;

  int i, j, k, deg, n1, n2, n1x, n1y, n2x, n2y, ymin, ymax, xmin, xmax, curX,
      curY, curL, crossX, crossY, crossL, tmpX, tmpY, tmpL, tmpi, min_x, min_y,
      *dtmp;
  int regionX1, regionX2, regionY1, regionY2, routeLen;
  int heapLen1, heapLen2, ind, ind1, grid;
  float tmp;
  TreeEdge *treeedges, *treeedge;
  TreeNode* treenodes;

  int endpt1, endpt2, A1, A2, B1, B2, C1, C2, cnt, cnt_n1n2, remd;
  int edge_n1n2, edge_n1A1, edge_n1A2, edge_n1C1, edge_n1C2, edge_A1A2,
      edge_C1C2;
  int edge_n2B1, edge_n2B2, edge_n2D1, edge_n2D2, edge_B1B2, edge_D1D2, D1, D2;
  int E1x, E1y, E2x, E2y, range, corE1, corE2, edgeID;

  bool Horizontal, n1Shift, n2Shift, redundant;
  int lastL, origL, headRoom, tailRoom, newcnt_n1n2, numpoints, d, n1a, n2a,
      connectionCNT;
  int origEng, orderIndex;

  directions_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  corr_edge_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);
  pr_3D_.resize(boost::extents[num_layers_][y_grid_][x_grid_]);

  pop_heap2_3D_.resize(num_layers_ * y_range_ * x_range_);

  // allocate memory for priority queue
  heap1_3D_ = new int*[y_grid_ * x_grid_ * num_layers_];
  heap2_3D_ = new short*[y_grid_ * x_grid_ * num_layers_];

  for (i = 0; i < y_grid_; i++) {
    for (j = 0; j < x_grid_; j++) {
      in_region_[i][j] = false;
    }
  }

  range = y_range_ * x_range_ * num_layers_;
  for (i = 0; i < range; i++) {
    pop_heap2_3D_[i] = false;
  }

  endIND = num_valid_nets_ * 0.9;

  for (orderIndex = 0; orderIndex < endIND; orderIndex++) {
    netID = tree_order_pv_[orderIndex].treeIndex;
    FrNet* net = nets_[netID];

    std::vector<int> edge_cost_per_layer = nets_[netID]->edge_cost_per_layer;

    /* TODO:  <14-08-19, uncomment this to reproduce ispd18_test6> */
    /* if (netID == 53757) { */
    /*         continue; */
    /* } */

    enlarge = expand;
    deg = sttrees_[netID].deg;
    treeedges = sttrees_[netID].edges;
    treenodes = sttrees_[netID].nodes;
    origEng = enlarge;

    for (edgeID = 0; edgeID < 2 * deg - 3; edgeID++) {
      treeedge = &(treeedges[edgeID]);

      if (treeedge->len < ripupTHub && treeedge->len > ripupTHlb) {
        n1 = treeedge->n1;
        n2 = treeedge->n2;
        n1x = treenodes[n1].x;
        n1y = treenodes[n1].y;
        n2x = treenodes[n2].x;
        n2y = treenodes[n2].y;
        routeLen = treeedges[edgeID].route.routelen;

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

        // ripup the routing for the edge
        if (newRipup3DType3(netID, edgeID)) {
          enlarge = std::min(origEng, treeedge->route.routelen);

          regionX1 = std::max(0, xmin - enlarge);
          regionX2 = std::min(x_grid_ - 1, xmax + enlarge);
          regionY1 = std::max(0, ymin - enlarge);
          regionY2 = std::min(y_grid_ - 1, ymax + enlarge);

          n1Shift = false;
          n2Shift = false;
          n1a = treeedge->n1a;
          n2a = treeedge->n2a;

          // initialize pop_heap1_3D_[] and pop_heap2_3D_[] as false (for detecting
          // the shortest path is found or not)

          for (k = 0; k < num_layers_; k++) {
            for (i = regionY1; i <= regionY2; i++) {
              for (j = regionX1; j <= regionX2; j++) {
                d1_3D_[k][i][j] = BIG_INT;
                d2_3D_[k][i][j] = 256;
              }
            }
          }

          // setup heap1_3D_, heap2_3D_ and initialize d1_3D_[][] and d2_3D_[][] for all
          // the grids on the two subtrees
          setupHeap3D(netID,
                      edgeID,
                      &heapLen1,
                      &heapLen2,
                      regionX1,
                      regionX2,
                      regionY1,
                      regionY2);

          // while loop to find shortest path
          ind1 = (heap1_3D_[0] - &d1_3D_[0][0][0]);

          for (i = 0; i < heapLen2; i++)
            pop_heap2_3D_[heap2_3D_[i] - &d2_3D_[0][0][0]] = true;

          while (pop_heap2_3D_[ind1]
                 == false)  // stop until the grid position been popped out from
                            // both heap1_3D_ and heap2_3D_
          {
            // relax all the adjacent grids within the enlarged region for
            // source subtree
            curL = ind1 / (grid_hv_);
            remd = ind1 % (grid_hv_);
            curX = remd % x_range_;
            curY = remd / x_range_;

            if (heapLen1 < 1) {
              logger_->error(GRT, 183, "Heap underflow.");
            }
            extractMin3D(heap1_3D_, heapLen1);
            // pop_heap1_3D_[ind1] = true;
            heapLen1--;

            if (((curL % 2) - layerOrientation) == 0) {
              Horizontal = true;
            } else {
              Horizontal = false;
            }

            if (Horizontal) {
              // left
              if (curX > regionX1 && directions_3D_[curL][curY][curX] != EAST) {
                grid = grid_hs_[curL] + curY * (x_grid_ - 1) + curX - 1;
                tmp = d1_3D_[curL][curY][curX] + 1;
                if (h_edges_3D_[grid].usage < h_edges_3D_[grid].cap &&
                    curL >= net->minLayer && curL <= net->maxLayer) {
                  tmpX = curX - 1;  // the left neighbor

                  if (d1_3D_[curL][curY][tmpX]
                      >= BIG_INT)  // left neighbor not been put into heap1_3D_
                  {
                    d1_3D_[curL][curY][tmpX] = tmp;
                    pr_3D_[curL][curY][tmpX].l = curL;
                    pr_3D_[curL][curY][tmpX].x = curX;
                    pr_3D_[curL][curY][tmpX].y = curY;
                    directions_3D_[curL][curY][tmpX] = WEST;
                    heap1_3D_[heapLen1] = &d1_3D_[curL][curY][tmpX];
                    heapLen1++;
                    updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
                  } else if (d1_3D_[curL][curY][tmpX]
                             > tmp)  // left neighbor been put into heap1_3D_ but
                                     // needs update
                  {
                    d1_3D_[curL][curY][tmpX] = tmp;
                    pr_3D_[curL][curY][tmpX].l = curL;
                    pr_3D_[curL][curY][tmpX].x = curX;
                    pr_3D_[curL][curY][tmpX].y = curY;
                    directions_3D_[curL][curY][tmpX] = WEST;
                    dtmp = &d1_3D_[curL][curY][tmpX];
                    ind = 0;
                    while (heap1_3D_[ind] != dtmp)
                      ind++;
                    updateHeap3D(heap1_3D_, heapLen1, ind);
                  }
                }
              }
              // right
              if (Horizontal && curX < regionX2
                  && directions_3D_[curL][curY][curX] != WEST) {
                grid = grid_hs_[curL] + curY * (x_grid_ - 1) + curX;

                tmp = d1_3D_[curL][curY][curX] + 1;
                tmpX = curX + 1;  // the right neighbor

                if (h_edges_3D_[grid].usage < h_edges_3D_[grid].cap &&
                    curL >= net->minLayer && curL <= net->maxLayer) {
                  if (d1_3D_[curL][curY][tmpX]
                      >= BIG_INT)  // right neighbor not been put into heap1_3D_
                  {
                    d1_3D_[curL][curY][tmpX] = tmp;
                    pr_3D_[curL][curY][tmpX].l = curL;
                    pr_3D_[curL][curY][tmpX].x = curX;
                    pr_3D_[curL][curY][tmpX].y = curY;
                    directions_3D_[curL][curY][tmpX] = EAST;
                    heap1_3D_[heapLen1] = &d1_3D_[curL][curY][tmpX];
                    heapLen1++;
                    updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
                  } else if (d1_3D_[curL][curY][tmpX]
                             > tmp)  // right neighbor been put into heap1_3D_ but
                                     // needs update
                  {
                    d1_3D_[curL][curY][tmpX] = tmp;
                    pr_3D_[curL][curY][tmpX].l = curL;
                    pr_3D_[curL][curY][tmpX].x = curX;
                    pr_3D_[curL][curY][tmpX].y = curY;
                    directions_3D_[curL][curY][tmpX] = EAST;
                    dtmp = &d1_3D_[curL][curY][tmpX];
                    ind = 0;
                    while (heap1_3D_[ind] != dtmp)
                      ind++;
                    updateHeap3D(heap1_3D_, heapLen1, ind);
                  }
                }
              }
            } else {
              // bottom
              if (!Horizontal && curY > regionY1
                  && directions_3D_[curL][curY][curX] != SOUTH) {
                grid = grid_vs_[curL] + (curY - 1) * x_grid_ + curX;
                tmp = d1_3D_[curL][curY][curX] + 1;
                tmpY = curY - 1;  // the bottom neighbor
                if (v_edges_3D_[grid].usage < v_edges_3D_[grid].cap &&
                    curL >= net->minLayer && curL <= net->maxLayer) {
                  if (d1_3D_[curL][tmpY][curX]
                      >= BIG_INT)  // bottom neighbor not been put into heap1_3D_
                  {
                    d1_3D_[curL][tmpY][curX] = tmp;
                    pr_3D_[curL][tmpY][curX].l = curL;
                    pr_3D_[curL][tmpY][curX].x = curX;
                    pr_3D_[curL][tmpY][curX].y = curY;
                    directions_3D_[curL][tmpY][curX] = NORTH;
                    heap1_3D_[heapLen1] = &d1_3D_[curL][tmpY][curX];
                    heapLen1++;
                    updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
                  } else if (d1_3D_[curL][tmpY][curX]
                             > tmp)  // bottom neighbor been put into heap1_3D_
                                     // but needs update
                  {
                    d1_3D_[curL][tmpY][curX] = tmp;
                    pr_3D_[curL][tmpY][curX].l = curL;
                    pr_3D_[curL][tmpY][curX].x = curX;
                    pr_3D_[curL][tmpY][curX].y = curY;
                    directions_3D_[curL][tmpY][curX] = NORTH;
                    dtmp = &d1_3D_[curL][tmpY][curX];
                    ind = 0;
                    while (heap1_3D_[ind] != dtmp)
                      ind++;
                    updateHeap3D(heap1_3D_, heapLen1, ind);
                  }
                }
              }
              // top
              if (!Horizontal && curY < regionY2
                  && directions_3D_[curL][curY][curX] != NORTH) {
                grid = grid_vs_[curL] + curY * x_grid_ + curX;
                tmp = d1_3D_[curL][curY][curX] + 1;
                tmpY = curY + 1;  // the top neighbor
                if (v_edges_3D_[grid].usage < v_edges_3D_[grid].cap &&
                    curL >= net->minLayer && curL <= net->maxLayer) {
                  if (d1_3D_[curL][tmpY][curX]
                      >= BIG_INT)  // top neighbor not been put into heap1_3D_
                  {
                    d1_3D_[curL][tmpY][curX] = tmp;
                    pr_3D_[curL][tmpY][curX].l = curL;
                    pr_3D_[curL][tmpY][curX].x = curX;
                    pr_3D_[curL][tmpY][curX].y = curY;
                    directions_3D_[curL][tmpY][curX] = SOUTH;
                    heap1_3D_[heapLen1] = &d1_3D_[curL][tmpY][curX];
                    heapLen1++;
                    updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
                  } else if (d1_3D_[curL][tmpY][curX]
                             > tmp)  // top neighbor been put into heap1_3D_ but
                                     // needs update
                  {
                    d1_3D_[curL][tmpY][curX] = tmp;
                    pr_3D_[curL][tmpY][curX].l = curL;
                    pr_3D_[curL][tmpY][curX].x = curX;
                    pr_3D_[curL][tmpY][curX].y = curY;
                    directions_3D_[curL][tmpY][curX] = SOUTH;
                    dtmp = &d1_3D_[curL][tmpY][curX];
                    ind = 0;
                    while (heap1_3D_[ind] != dtmp)
                      ind++;
                    updateHeap3D(heap1_3D_, heapLen1, ind);
                  }
                }
              }
            }

            // down
            if (curL > 0 && directions_3D_[curL][curY][curX] != UP) {
              tmp = d1_3D_[curL][curY][curX] + via_cost_;
              tmpL = curL - 1;  // the bottom neighbor

              if (d1_3D_[tmpL][curY][curX]
                  >= BIG_INT)  // bottom neighbor not been put into heap1_3D_
              {
                d1_3D_[tmpL][curY][curX] = tmp;
                pr_3D_[tmpL][curY][curX].l = curL;
                pr_3D_[tmpL][curY][curX].x = curX;
                pr_3D_[tmpL][curY][curX].y = curY;
                directions_3D_[tmpL][curY][curX] = DOWN;
                heap1_3D_[heapLen1] = &d1_3D_[tmpL][curY][curX];
                heapLen1++;
                updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
              } else if (d1_3D_[tmpL][curY][curX]
                         > tmp)  // bottom neighbor been put into heap1_3D_ but
                                 // needs update
              {
                d1_3D_[tmpL][curY][curX] = tmp;
                pr_3D_[tmpL][curY][curX].l = curL;
                pr_3D_[tmpL][curY][curX].x = curX;
                pr_3D_[tmpL][curY][curX].y = curY;
                directions_3D_[tmpL][curY][curX] = DOWN;
                dtmp = &d1_3D_[tmpL][curY][curX];
                ind = 0;
                while (heap1_3D_[ind] != dtmp)
                  ind++;
                updateHeap3D(heap1_3D_, heapLen1, ind);
              }
            }

            // up
            if (curL < num_layers_ - 1
                && directions_3D_[curL][curY][curX] != DOWN) {
              tmp = d1_3D_[curL][curY][curX] + via_cost_;
              tmpL = curL + 1;  // the bottom neighbor
              if (d1_3D_[tmpL][curY][curX]
                  >= BIG_INT)  // bottom neighbor not been put into heap1_3D_
              {
                d1_3D_[tmpL][curY][curX] = tmp;
                pr_3D_[tmpL][curY][curX].l = curL;
                pr_3D_[tmpL][curY][curX].x = curX;
                pr_3D_[tmpL][curY][curX].y = curY;
                directions_3D_[tmpL][curY][curX] = UP;
                heap1_3D_[heapLen1] = &d1_3D_[tmpL][curY][curX];
                heapLen1++;
                updateHeap3D(heap1_3D_, heapLen1, heapLen1 - 1);
              } else if (d1_3D_[tmpL][curY][curX]
                         > tmp)  // bottom neighbor been put into heap1_3D_ but
                                 // needs update
              {
                d1_3D_[tmpL][curY][curX] = tmp;
                pr_3D_[tmpL][curY][curX].l = curL;
                pr_3D_[tmpL][curY][curX].x = curX;
                pr_3D_[tmpL][curY][curX].y = curY;
                directions_3D_[tmpL][curY][curX] = UP;
                dtmp = &d1_3D_[tmpL][curY][curX];
                ind = 0;
                while (heap1_3D_[ind] != dtmp)
                  ind++;
                updateHeap3D(heap1_3D_, heapLen1, ind);
              }
            }

            // update ind1 for next loop
            ind1 = (heap1_3D_[0] - &d1_3D_[0][0][0]);
          }  // while loop

          for (i = 0; i < heapLen2; i++)
            pop_heap2_3D_[heap2_3D_[i] - &d2_3D_[0][0][0]] = false;

          // get the new route for the edge and store it in gridsX[] and
          // gridsY[] temporarily

          crossL = ind1 / (grid_hv_);
          crossX = (ind1 % (grid_hv_)) % x_range_;
          crossY = (ind1 % (grid_hv_)) / x_range_;

          cnt = 0;
          curX = crossX;
          curY = crossY;
          curL = crossL;

          if (d1_3D_[curL][curY][curX] == 0) {
            recoverEdge(netID, edgeID);
            break;
          }

          std::vector<int> tmp_gridsX, tmp_gridsY, tmp_gridsL;

          while (d1_3D_[curL][curY][curX] != 0)  // loop until reach subtree1
          {
            tmpL = pr_3D_[curL][curY][curX].l;
            tmpX = pr_3D_[curL][curY][curX].x;
            tmpY = pr_3D_[curL][curY][curX].y;
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

          cnt_n1n2 = cnt;

          E1x = gridsX[0];
          E1y = gridsY[0];
          E2x = gridsX.back();
          E2y = gridsY.back();

          headRoom = 0;
          origL = gridsL[0];

          while (headRoom < gridsX.size() && gridsX[headRoom] == E1x
                 && gridsY[headRoom] == E1y) {
            headRoom++;
          }
          if (headRoom > 0) {
            headRoom--;
          }

          lastL = gridsL[headRoom];

          // change the tree structure according to the new routing for the tree
          // edge find E1 and E2, and the endpoints of the edges they are on

          edge_n1n2 = edgeID;
          // (1) consider subtree1
          if (n1 >= deg && (E1x != n1x || E1y != n1y))
          // n1 is not a pin and E1!=n1, then make change to subtree1,
          // otherwise, no change to subtree1
          {
            n1Shift = true;
            corE1 = corr_edge_3D_[origL][E1y][E1x];

            endpt1 = treeedges[corE1].n1;
            endpt2 = treeedges[corE1].n2;

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
              C1 = endpt1;
              C2 = endpt2;
              edge_C1C2 = corr_edge_3D_[origL][E1y][E1x];

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
          else {
            newUpdateNodeLayers(treenodes, edge_n1n2, n1a, lastL);
          }

          origL = gridsL[cnt_n1n2 - 1];
          tailRoom = cnt_n1n2 - 1;

          while (tailRoom > 0 && gridsX[tailRoom] == E2x
                 && gridsY[tailRoom] == E2y) {
            tailRoom--;
          }
          if (tailRoom < cnt_n1n2 - 1) {
            tailRoom++;
          }

          lastL = gridsL[tailRoom];

          // (2) consider subtree2
          if (n2 >= deg && (E2x != n2x || E2y != n2y))
          // n2 is not a pin and E2!=n2, then make change to subtree2,
          // otherwise, no change to subtree2
          {
            // find the endpoints of the edge E1 is on

            n2Shift = true;
            corE2 = corr_edge_3D_[origL][E2y][E2x];
            endpt1 = treeedges[corE2].n1;
            endpt2 = treeedges[corE2].n2;

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
            else  // E2 is not on (n2, B1) or (n2, B2), but on (d1_3D_, d2_3D_)
            {
              D1 = endpt1;
              D2 = endpt2;
              edge_D1D2 = corr_edge_3D_[origL][E2y][E2x];

              // update route for edge (n2, d1_3D_), (n2, d2_3D_) and (B1, B2)
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
              // update 3 edges (n2, B1)->(d1_3D_, n2), (n2, B2)->(n2, d2_3D_),
              // (d1_3D_, d2_3D_)->(B1, B2)
              edge_n2D1 = edge_n2B1;
              treeedges[edge_n2D1].n1 = D1;
              treeedges[edge_n2D1].n2 = n2;
              edge_n2D2 = edge_n2B2;
              treeedges[edge_n2D2].n1 = n2;
              treeedges[edge_n2D2].n2 = D2;
              edge_B1B2 = edge_D1D2;
              treeedges[edge_B1B2].n1 = B1;
              treeedges[edge_B1B2].n2 = B2;
              // update nbr and edge for 5 nodes n2, B1, B2, d1_3D_, d2_3D_
              // n1's nbr (n1, B1, B2)->(n1, d1_3D_, d2_3D_)
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
            }     // else E2 is not on (n2, B1) or (n2, B2), but on (d1_3D_, d2_3D_)
          } else  // n2 is not a pin and E2!=n2
          {
            newUpdateNodeLayers(treenodes, edge_n1n2, n2a, lastL);
          }

          newcnt_n1n2 = tailRoom - headRoom + 1;

          // update route for edge (n1, n2) and edge usage
          if (treeedges[edge_n1n2].route.type == RouteType::MazeRoute) {
            free(treeedges[edge_n1n2].route.gridsX);
            free(treeedges[edge_n1n2].route.gridsY);
            free(treeedges[edge_n1n2].route.gridsL);
          }

          treeedges[edge_n1n2].route.gridsX
              = (short*) calloc(newcnt_n1n2, sizeof(short));
          treeedges[edge_n1n2].route.gridsY
              = (short*) calloc(newcnt_n1n2, sizeof(short));
          treeedges[edge_n1n2].route.gridsL
              = (short*) calloc(newcnt_n1n2, sizeof(short));
          treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
          treeedges[edge_n1n2].route.routelen = newcnt_n1n2 - 1;
          treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y- E2y);

          j = headRoom;
          for (i = 0; i < newcnt_n1n2; i++) {
            treeedges[edge_n1n2].route.gridsX[i] = gridsX[j];
            treeedges[edge_n1n2].route.gridsY[i] = gridsY[j];
            treeedges[edge_n1n2].route.gridsL[i] = gridsL[j];
            j++;
          }

          // update edge usage
          for (i = headRoom; i < tailRoom; i++) {
            if (gridsL[i] == gridsL[i + 1]) {
              if (gridsX[i] == gridsX[i + 1])  // a vertical edge
              {
                min_y = std::min(gridsY[i], gridsY[i + 1]);
                v_edges_3D_[gridsL[i] * grid_v_ + min_y * x_grid_ + gridsX[i]].usage
                    += edge_cost_per_layer[gridsL[i]];
              } else  /// if(gridsY[i]==gridsY[i+1])// a horizontal edge
              {
                min_x = std::min(gridsX[i], gridsX[i + 1]);
                h_edges_3D_[gridsL[i] * grid_h_ + gridsY[i] * (x_grid_ - 1) + min_x]
                    .usage
                    += edge_cost_per_layer[gridsL[i]];
              }
            }
          }

          if (n1Shift || n2Shift) {
            // re statis the node overlap
            numpoints = 0;

            for (d = 0; d < 2 * deg - 2; d++) {
              treenodes[d].topL = -1;
              treenodes[d].botL = num_layers_;
              treenodes[d].assigned = false;
              treenodes[d].stackAlias = d;
              treenodes[d].conCNT = 0;
              treenodes[d].hID = BIG_INT;
              treenodes[d].lID = BIG_INT;
              treenodes[d].status = 0;

              if (d < deg) {
                treenodes[d].botL = treenodes[d].topL = 0;
                // treenodes[d].l = 0;
                treenodes[d].assigned = true;
                treenodes[d].status = 1;

                xcor_[numpoints] = treenodes[d].x;
                ycor_[numpoints] = treenodes[d].y;
                dcor_[numpoints] = d;
                numpoints++;
              } else {
                redundant = false;
                for (k = 0; k < numpoints; k++) {
                  if ((treenodes[d].x == xcor_[k])
                      && (treenodes[d].y == ycor_[k])) {
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
            }  // numerating for nodes
            for (k = 0; k < 2 * deg - 3; k++) {
              treeedge = &(treeedges[k]);

              if (treeedge->len > 0) {
                routeLen = treeedge->route.routelen;

                n1 = treeedge->n1;
                n2 = treeedge->n2;
                gridsLtmp = treeedge->route.gridsL;

                n1a = treenodes[n1].stackAlias;

                n2a = treenodes[n2].stackAlias;

                treeedge->n1a = n1a;
                treeedge->n2a = n2a;

                connectionCNT = treenodes[n1a].conCNT;
                treenodes[n1a].heights[connectionCNT] = gridsLtmp[0];
                treenodes[n1a].eID[connectionCNT] = k;
                treenodes[n1a].conCNT++;

                if (gridsLtmp[0] > treenodes[n1a].topL) {
                  treenodes[n1a].hID = k;
                  treenodes[n1a].topL = gridsLtmp[0];
                }
                if (gridsLtmp[0] < treenodes[n1a].botL) {
                  treenodes[n1a].lID = k;
                  treenodes[n1a].botL = gridsLtmp[0];
                }

                treenodes[n1a].assigned = true;

                connectionCNT = treenodes[n2a].conCNT;
                treenodes[n2a].heights[connectionCNT] = gridsLtmp[routeLen];
                treenodes[n2a].eID[connectionCNT] = k;
                treenodes[n2a].conCNT++;
                if (gridsLtmp[routeLen] > treenodes[n2a].topL) {
                  treenodes[n2a].hID = k;
                  treenodes[n2a].topL = gridsLtmp[routeLen];
                }
                if (gridsLtmp[routeLen] < treenodes[n2a].botL) {
                  treenodes[n2a].lID = k;
                  treenodes[n2a].botL = gridsLtmp[routeLen];
                }

                treenodes[n2a].assigned = true;

              }  // edge len > 0

            }  // eunmerating edges
          }  // if shift1 and shift2
        }
      }
    }
  }

  directions_3D_.resize(boost::extents[0][0][0]);
  corr_edge_3D_.resize(boost::extents[0][0][0]);
  pr_3D_.resize(boost::extents[0][0][0]);

  delete[] heap1_3D_;
  delete[] heap2_3D_;
}

}  // namespace grt
