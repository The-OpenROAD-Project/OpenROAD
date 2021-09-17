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

#include "node.h"

namespace pdr {

using std::ostream;
using std::vector;

Node::Node(int _idx, int _x, int _y) :
  idx(_idx),
  x(_x),
  y(_y),
  maxPLToChild(0)
{
  parent = 0;
  min_dist = 0;
  path_length = 0;
  detcost_edgePToNode = -1;
  detcost_edgeNodeToP = -1;
  src_to_sink_dist = 0;
  K_t = 1;
  level = 0;
  conn_to_par = false;
  // magic number alert -cherry
  idx_of_cn_x = 105;
  idx_of_cn_y = 105;
};

void Node::report(ostream& os,
                  int level) const
{
  os << idx << " (" << x << ", " << y << ")";
  if (level > 1) {
    os << " parent: " << parent << " children: ";
    for (int i = 0; i < children.size(); ++i) {
      os << children[i] << " ";
    }
  }
  if (level > 2) {
#ifdef PDREVII
    os << " N: ";
    for (int i = 0; i < N.size(); ++i) {
      os << N[i] << " ";
    }
    os << " S: ";
    for (int i = 0; i < S.size(); ++i) {
      os << S[i] << " ";
    }
    os << " E: ";
    for (int i = 0; i < E.size(); ++i) {
      os << E[i] << " ";
    }
    os << " W: ";
    for (int i = 0; i < W.size(); ++i) {
      os << W[i] << " ";
    }
#endif
    os << "PL: " << src_to_sink_dist << " MaxPLToChild: " << maxPLToChild;
  }
}

ostream& operator<<(ostream& os, const Node& n)
{
  n.report(os, 1);
  return os;
}

Node1::Node1(int _idx, int _x, int _y)
{
  idx = _idx;
  x = _x;
  y = _y;
}

} // namespace
