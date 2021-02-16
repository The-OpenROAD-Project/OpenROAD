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

#include "node.h"

namespace PD {

using std::ostream;

class Edge
{
 public:
  int idx;
  int head;
  int tail;
  int best_shape;        // 0 = lower L, 1 = upper L
  int final_best_shape;  // 0 = lower L, 1 = upper L
  int best_ov;
  unsigned lower_ov, upper_ov;
  vector<int> upper_best_config, lower_best_config;
  unsigned lower_idx_of_cn_x, lower_idx_of_cn_y;
  unsigned upper_idx_of_cn_x, upper_idx_of_cn_y;

  vector<Node> STNodes;
  vector<Node> lower_sps_to_be_added_x, lower_sps_to_be_added_y;
  vector<Node> upper_sps_to_be_added_x, upper_sps_to_be_added_y;

  Edge(){};
  Edge(int _idx, int _head, int _tail)
  {
    idx = _idx;
    head = _head;
    tail = _tail;
    lower_ov = 0;
    upper_ov = 0;
    best_ov = 0;
    best_shape = 5;
    final_best_shape = 5;
    lower_idx_of_cn_x = 9999999;
    lower_idx_of_cn_y = 9999999;
    upper_idx_of_cn_x = 9999999;
    upper_idx_of_cn_y = 9999999;
  };

  ~Edge() { STNodes.clear(); }

  friend ostream& operator<<(ostream& os, const Edge& n)
  {
    os << n.idx << "(" << n.head << ", " << n.tail
       << ") edgeShape: " << n.best_shape;
    os << "  Steiner: ";
    for (unsigned i = 0; i < n.STNodes.size(); ++i) {
      os << " (" << n.STNodes[i].x << " " << n.STNodes[i].y << ") Child: ";
      for (unsigned j = 0; j < n.STNodes[i].sp_chil.size(); ++j) {
        os << n.STNodes[i].sp_chil[j] << " ";
      }
      os << "/";
    }
    if (n.best_shape == 0) {
      for (unsigned i = 0; i < n.lower_sps_to_be_added_x.size(); ++i) {
        os << " (" << n.lower_sps_to_be_added_x[i].x << " "
           << n.lower_sps_to_be_added_x[i].y << ") Child: ";
        for (unsigned j = 0; j < n.lower_sps_to_be_added_x[i].sp_chil.size();
             ++j) {
          os << n.lower_sps_to_be_added_x[i].sp_chil[j] << " ";
        }
        os << "/";
      }
    } else {
      for (unsigned i = 0; i < n.upper_sps_to_be_added_x.size(); ++i) {
        os << " (" << n.upper_sps_to_be_added_x[i].x << " "
           << n.upper_sps_to_be_added_x[i].y << ") Child: ";
        for (unsigned j = 0; j < n.upper_sps_to_be_added_x[i].sp_chil.size();
             ++j) {
          os << n.upper_sps_to_be_added_x[i].sp_chil[j] << " ";
        }
        os << "/";
      }
    }
    return os;
  }
};

}  // namespace PD
