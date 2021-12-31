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

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <vector>

namespace dpo {

class Graph {
 public:
  Graph(int v) : m_v(v) {
    m_adj.resize(m_v);

    m_color.resize(m_v);
    std::fill(m_color.begin(), m_color.end(), -1);
    m_ncolors = 0;
  }
  virtual ~Graph() {}
  void addEdge(int u, int v) {
    m_adj[u].push_back(v);
    m_adj[v].push_back(u);
  }
  void removeDuplicates() {
    for (int i = 0; i < m_v; i++) {
      std::sort(m_adj[i].begin(), m_adj[i].end());
      m_adj[i].erase(std::unique(m_adj[i].begin(), m_adj[i].end()),
                     m_adj[i].end());
    }
  }

  void greedyColoring() {
    m_color.resize(m_v);
    std::fill(m_color.begin(), m_color.end(), -1);
    m_color[0] = 0;  // first node gets first color.

    m_ncolors = 1;

    std::vector<int> avail(m_v, -1);

    // Do subsequent nodes.
    for (int v = 1; v < m_v; v++) {
      // Determine which colors cannot be used.  Pick the smallest
      // color which can be used.
      for (int i = 0; i < m_adj[v].size(); i++) {
        int u = m_adj[v][i];
        if (m_color[u] != -1) {
          // Node "u" has a color.  So, it is not available to "v".
          avail[m_color[u]] = v;  // Marking "avail[color]" with a "v" means it
                                  // is not available for node v.
        }
      }

      for (int cr = 0; cr < m_v; cr++) {
        if (avail[cr] != v) {
          m_color[v] = cr;
          m_ncolors = std::max(m_ncolors, cr + 1);
          break;
        }
      }
    }
  }

  int getColor(int i) const { return m_color[i]; }
  int getNColors() const { return m_ncolors; }

 private:
  int m_v;
  std::vector<std::vector<int>> m_adj;
  std::vector<int> m_color;
  int m_ncolors;
};

}  // namespace dpo
