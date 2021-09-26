///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ostream>
#include <vector>

#include "pdrevII.h"

namespace pdr {

using std::ostream;
using std::vector;

class Node
{
public:
  Node(int _idx, int _x, int _y);
  void report(ostream& os, int level) const;

  int idx;
  int x;
  int y;

  int parent;       // parent's node index
  vector<int> children;  // immediate children's indices
  int size_of_chi;
  int min_dist;  // source to sink manhattan dist
  int path_length;
  int detcost_edgePToNode;  // Detour cost of edge, from parent to node
  int detcost_edgeNodeToP;  // Detour cost of edge, from node to parent
  int cost_edgeToP;         // Cost of edge to parent
  int src_to_sink_dist;  // source to sink distance tranversing from the node to
                         // the source through the tree
  int K_t;               // No. of downstream sinks
  int level;        // Level in tree

  bool conn_to_par;
  vector<int> sp_chil;
  int idx_of_cn_x, idx_of_cn_y;
  int det_cost_node;  // Detour cost of the node j = Sum of all the detour costs
                      // of the edges from the source to the node j
  int maxPLToChild;

  friend ostream& operator<<(ostream& os, const Node& n);

#ifdef PDREVII
  // Segregated PDrevII code
  vector<int> N;
  vector<int> S;
  vector<int> E;
  vector<int> W;
  vector<int> nn_edge_detcost;  // Detour cost of the edges to the nearest neighbours
  vector<vector<int>> swap_space;
#endif
};

class Node1
{
public:
  Node1(int _idx, int _x, int _y);

  int idx;
  int x;
  int y;
};

}  // namespace
