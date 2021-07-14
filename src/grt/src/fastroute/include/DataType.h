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

#pragma once

#include <string>
#include <vector>

namespace odb {
class dbNet;
}

namespace grt {
typedef int DTYPE;

enum class RouteType
{
  NoRoute,
  LRoute,
  ZRoute,
  MazeRoute
};

typedef struct
{
  DTYPE x, y;
  int n;
} Branch;

typedef struct
{
  int deg;
  int totalDeg;
  DTYPE length;
  Branch* branch;
} Tree;

typedef struct
{
  bool xFirst;  // route x-direction first (only for L route)
  bool HVH;     // TRUE = HVH or false = VHV (only for Z route)
  bool maze;    // Whether this segment is routed by maze

  short x1, y1, x2, y2;  // coordinates of two endpoints
  int netID;             // the netID of the net this segment belonging to
  short Zpoint;          // The coordinates of Z point (x for HVH and y for VHV)
  short* route;          // array of H and V Edges to implement this Segment
  int numEdges;          // number of H and V Edges to implement this Segment
} Segment;               // A Segment is a 2-pin connection

typedef struct
{
  odb::dbNet* db_net;
  int numPins;  // number of pins in the net
  int deg;  // net degree (number of MazePoints connecting by the net, pins in
              // same MazePoints count only 1)
  std::vector<int> pinX;  // array of X coordinates of pins
  std::vector<int> pinY;  // array of Y coordinates of pins
  std::vector<int> pinL;  // array of L coordinates of pins
  float alpha;              // alpha for pdrev when routing clock nets
  bool is_clock;             // flag that indicates if net is a clock net
  int driver_idx;
  int edgeCost;
  int minLayer;
  int maxLayer;
  std::vector<int> edge_cost_per_layer;
} FrNet;                    // A Net is a set of connected MazePoints

const char* netName(FrNet* net);

typedef struct
{
  short congCNT;
  unsigned short cap;    // the capacity of the edge
  unsigned short usage;  // the usage of the edge
  unsigned short red;
  short last_usage;
  float est_usage;  // the estimated usage of the edge
} Edge;  // An Edge is the routing track holder between two adjacent MazePoints

typedef struct
{
  unsigned short cap;    // the capacity of the edge
  unsigned short usage;  // the usage of the edge
  unsigned short red;
} Edge3D;

typedef struct TNode
{
  bool assigned;

  short status;
  short conCNT;
  short botL, topL;
  // heights and eID arrays size were increased after using PD
  // to create the tree topologies.
  static constexpr int max_connections = 10;
  short heights[max_connections];
  int eID[max_connections];

  short x, y;     // position in the grid graph
  int nbr[3];   // three neighbors
  int edge[3];  // three adjacent edges
  int hID;
  int lID;
  int stackAlias;
} TreeNode;

typedef struct
{
  RouteType type;  // type of route: LRoute, ZRoute, MazeRoute
  bool xFirst;   // valid for LRoute, TRUE - the route is horizontal first (x1,
                 // y1) - (x2, y1) - (x2, y2), false (x1, y1) - (x1, y2) - (x2,
                 // y2)
  bool HVH;      // valid for ZRoute, TRUE - the route is HVH shape, false - VHV
                 // shape
  short Zpoint;  // valid for ZRoute, the position of turn point for Z-shape
  short*
      gridsX;  // valid for MazeRoute, a list of grids (n=routelen+1) the route
               // passes, (x1, y1) is the first one, but (x2, y2) is the lastone
  short*
      gridsY;  // valid for MazeRoute, a list of grids (n=routelen+1) the route
               // passes, (x1, y1) is the first one, but (x2, y2) is the lastone
  short* gridsL;  // n
  int routelen;   // valid for MazeRoute, the number of edges in the route
                  // Edge3D *edge;       // list of 3D edges the route go
                  // through;

} Route;

typedef struct
{
  bool assigned;

  int len;  // the Manhanttan Distance for two end nodes
  int n1, n1a;
  int n2, n2a;
  Route route;

} TreeEdge;

typedef struct
{
  int deg;
  TreeNode* nodes;  // the nodes (pin and Steiner nodes) in the tree
  TreeEdge* edges;  // the tree edges
} StTree;

typedef struct
{
  int treeIndex;
  int minX;
  float npv;  // net length over pin
} OrderNetPin;

typedef struct
{
  int length;
  int treeIndex;
  int xmin;
} OrderTree;

typedef struct
{
  short l;
  int x, y;
} parent3D;

typedef struct
{
  int length;
  int edgeID;
} OrderNetEdge;

typedef enum
{
  NORTH,
  EAST,
  SOUTH,
  WEST,
  ORIGIN,
  UP,
  DOWN
} dirctionT;
typedef enum
{
  NONE,
  HORI,
  VERT,
  BID
} viaST;

}  // namespace grt
