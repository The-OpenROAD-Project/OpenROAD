///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

#pragma once

#include <vector>

namespace dpo {

class Graph
{
 public:
  Graph(int v) : v_(v)
  {
    adj_.resize(v_);

    color_.resize(v_);
    std::fill(color_.begin(), color_.end(), -1);
    ncolors_ = 0;
  }

  void addEdge(int u, int v)
  {
    adj_[u].push_back(v);
    adj_[v].push_back(u);
  }

  void removeDuplicates()
  {
    for (int i = 0; i < v_; i++) {
      std::sort(adj_[i].begin(), adj_[i].end());
      adj_[i].erase(std::unique(adj_[i].begin(), adj_[i].end()), adj_[i].end());
    }
  }

  void greedyColoring()
  {
    color_.resize(v_);
    std::fill(color_.begin(), color_.end(), -1);
    color_[0] = 0;  // first node gets first color.

    ncolors_ = 1;

    std::vector<int> avail(v_, -1);

    // Do subsequent nodes.
    for (int v = 1; v < v_; v++) {
      // Determine which colors cannot be used.  Pick the smallest
      // color which can be used.
      for (int i = 0; i < adj_[v].size(); i++) {
        int u = adj_[v][i];
        if (color_[u] != -1) {
          // Node "u" has a color.  So, it is not available to "v".
          avail[color_[u]] = v;  // Marking "avail[color]" with a "v" means it
                                 // is not available for node v.
        }
      }

      for (int cr = 0; cr < v_; cr++) {
        if (avail[cr] != v) {
          color_[v] = cr;
          ncolors_ = std::max(ncolors_, cr + 1);
          break;
        }
      }
    }
  }

  int getColor(int i) const { return color_[i]; }
  int getNColors() const { return ncolors_; }

 private:
  int v_;
  std::vector<std::vector<int>> adj_;
  std::vector<int> color_;
  int ncolors_;
};

}  // namespace dpo
