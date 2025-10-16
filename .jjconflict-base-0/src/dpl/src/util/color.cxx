// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "color.h"

#include <algorithm>
#include <vector>

namespace dpl {

ColorGraph::ColorGraph(const int num_nodes)
{
  adj_.resize(num_nodes);

  color_.resize(num_nodes);
  std::fill(color_.begin(), color_.end(), -1);
  num_colors_ = 0;
}

void ColorGraph::addEdge(const int u, const int v)
{
  adj_[u].push_back(v);
  adj_[v].push_back(u);
}

void ColorGraph::removeDuplicateEdges()
{
  for (auto& adj : adj_) {
    std::sort(adj.begin(), adj.end());
    adj.erase(std::unique(adj.begin(), adj.end()), adj.end());
  }
}

void ColorGraph::greedyColoring()
{
  removeDuplicateEdges();
  std::fill(color_.begin(), color_.end(), -1);
  color_[0] = 0;  // first node gets first color.

  num_colors_ = 1;

  std::vector<int> avail(getNumNodes(), -1);

  // Do subsequent nodes.
  for (int v = 1; v < getNumNodes(); v++) {
    // Determine which colors cannot be used.  Pick the smallest
    // color which can be used.
    for (const int u : adj_[v]) {
      if (color_[u] != -1) {
        // Node "u" has a color.  So, it is not available to "v".
        avail[color_[u]] = v;  // Marking "avail[color]" with a "v" means it
                               // is not available for node v.
      }
    }

    for (int cr = 0; cr < getNumNodes(); cr++) {
      if (avail[cr] != v) {
        color_[v] = cr;
        num_colors_ = std::max(num_colors_, cr + 1);
        break;
      }
    }
  }
}

}  // namespace dpl
