///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "leidenInterface.h"
#include "ModularityVertexPartition.h"

#include <algorithm>
#include <iostream>
#include <queue>

namespace mpl2 {

std::vector<size_t> range(size_t n)
{
  std::vector<size_t> range_vec(n);
  for (size_t i = 0; i < n; i++)
    range_vec[i] = i;
  return range_vec;
}

bool orderCSize(const size_t* A, const size_t* B)
{
  if (A[1] == B[1]) {
    if (A[2] == B[2])
      return A[0] < B[0];
    else
      return A[2] > B[2];
  } else
    return A[1] > B[1];
}

void shuffle(std::vector<size_t>& v)
{
  std::random_device rd;
  std::mt19937 rng(rd());
  size_t n = v.size();
  if (n > 0)
  {
    for (size_t idx = n - 1; idx > 0; idx--)
    {
      std::uniform_int_distribution<size_t> dist(0, idx);
      size_t rand_idx = dist(rng);
      std::swap(v[idx], v[rand_idx]);
    }
  }
}

/****************************************************************************
  Creates a graph with communities as node and links as weights between
  communities.

  The weight of the edges in the new graph is simply the sum of the weight
  of the edges between the communities. The self weight of a node (i.e. the
  weight of its self loop) is the internal weight of a community. The size
  of a node in the new graph is simply the size of the community in the old
  graph.
*****************************************************************************/

GraphForLeidenAlgorithm* GraphForLeidenAlgorithm::collapse_graph(
    ModularityVertexPartition* partition)
{
  // Get the number of collapsed communities (vertices in the collapsed graph)
  size_t n_collapsed = partition->n_communities();
  std::vector<std::vector<size_t>> community_memberships
      = partition->get_communities();

  // Container for collapsed weights and total weight in the collapsed graph
  std::vector<std::map<int, float>> collapsed_adjacency_list(n_collapsed);
  std::vector<double> collapsed_weights;
  double total_collapsed_weight = 0.0;

  // To store temporary edge weights to communities
  std::vector<double> edge_weight_to_community(n_collapsed, 0.0);
  std::vector<bool> neighbour_comm_added(n_collapsed, false);

  // Iterate over each community (which corresponds to a node in the collapsed
  // graph)
  for (size_t v_comm = 0; v_comm < n_collapsed; ++v_comm) {
    std::vector<size_t> neighbour_communities;

    // Iterate over the nodes within this community
    for (size_t v : community_memberships[v_comm]) {
      // For each node, get its neighbors and their corresponding edges
      for (int u : this->get_neighbours(v)) {
        size_t u_comm = partition->membership(u);
        float w = this->getEdgeWeight(v, u);

        // If the graph is undirected and this is a self-loop, we need to divide
        // the weight
        if (v == u) {
          w /= 2.0;
        }

        // Add the edge weight to the collapsed community
        if (!neighbour_comm_added[u_comm]) {
          neighbour_comm_added[u_comm] = true;
          neighbour_communities.push_back(u_comm);
        }
        edge_weight_to_community[u_comm] += w;
      }
    }

    // For each neighboring community, add the collapsed edge
    for (size_t u_comm : neighbour_communities) {
      // Add the edge between the collapsed communities
      collapsed_adjacency_list[v_comm][u_comm]
          += edge_weight_to_community[u_comm];
      collapsed_weights.push_back(edge_weight_to_community[u_comm]);
      total_collapsed_weight += edge_weight_to_community[u_comm];

      // Reset the temporary edge weights and markers
      edge_weight_to_community[u_comm] = 0.0;
      neighbour_comm_added[u_comm] = false;
    }
  }

  // Create the collapsed graph with the new adjacency list
  GraphForLeidenAlgorithm* collapsed_graph
      = new GraphForLeidenAlgorithm(n_collapsed);
  collapsed_graph->_total_weight = total_collapsed_weight;

  // Copy the edges and weights into the new graph
  for (size_t v_comm = 0; v_comm < n_collapsed; ++v_comm) {
    for (const auto& pair : collapsed_adjacency_list[v_comm]) {
      collapsed_graph->addEdge(v_comm, pair.first, pair.second);
    }
  }

  // Set vertex weights for the collapsed graph (based on the size of the
  // original communities)
  for (size_t c = 0; c < n_collapsed; ++c) {
    collapsed_graph->vertex_weights_[c] = partition->csize(c);
  }

  return collapsed_graph;
}

}  // namespace mpl2
