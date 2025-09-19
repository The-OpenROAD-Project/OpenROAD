// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <vector>

namespace dpl {

class ColorGraph
{
 public:
  explicit ColorGraph(int num_nodes);
  void addEdge(int u, int v);
  void greedyColoring();

  int getColor(const int node_id) const { return color_[node_id]; }
  int getNumColors() const { return num_colors_; }

 private:
  void removeDuplicateEdges();
  int getNumNodes() const { return adj_.size(); }

  // An adjacency matrix: node id -> adjacent node ids
  std::vector<std::vector<int>> adj_;
  // Computed color for each node id
  std::vector<int> color_;
  // The total number of colors needed to color the graph
  int num_colors_;
};

}  // namespace dpl
