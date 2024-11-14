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
#pragma once
#include <cfloat>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <random>
#include <vector>

namespace mpl2 {

class ModularityVertexPartition;

std::vector<size_t> range(size_t n);

bool orderCSize(const size_t* A, const size_t* B);

void shuffle(std::vector<size_t>& v);

class Exception : public std::exception
{
 public:
  Exception(const char* str) { this->str = str; }

  virtual const char* what() const throw() { return this->str; }

 private:
  const char* str;
};

/**
 * @brief Hypergraph hyperedges_ stands for the adjacency list of the
 * hypergraph. hyperedge_weights_ stands for the weights of the hyperedges.
 * vertex_weights_ stands for the weights of the vertices.
 */
struct HyperGraphForLeidenAlgorithm
{
  std::vector<std::vector<int>> hyperedges_;
  std::vector<float> hyperedge_weights_;
  std::vector<float> vertex_weights_;
  size_t num_vertices_;
};

/**
 * @brief Graph if edges_[i] = j stands the node i is connected to j in  the
 * graph. edge_weights_ stands for the weights of the edges. vertex_weights_
 * stands for the weights of the vertices.
 */
class GraphForLeidenAlgorithm
{
 private:
  size_t num_vertices_;  // Number of vertices
  std::vector<std::map<int, float>>
      adjacency_list_;  // Map from neighbor to edge weight
  double _total_weight;

 public:
  std::vector<float> vertex_weights_;  // Vertex weights

  // Constructor
  GraphForLeidenAlgorithm(size_t num_vertices)
      : num_vertices_(num_vertices),
        adjacency_list_(num_vertices),
        vertex_weights_(num_vertices, 1.0f)  // Default vertex weight is 1.0
  {
  }

  // Add an edge between two vertices with a weight
  void addEdge(int v1, int v2, float weight)
  {
    adjacency_list_[v1][v2] = weight;
    adjacency_list_[v2][v1] = weight;  // For undirected graph
  }

  // Get the neighbors of a vertex
  std::vector<size_t> get_neighbours(size_t vertex) const
  {
    std::vector<size_t> neighbor_list;
    for (const auto& pair : adjacency_list_[vertex]) {
      neighbor_list.push_back(pair.first);
    }
    return neighbor_list;
  }

  // Get the neighbor edges of a vertex
  std::vector<double> get_neighbour_edges(int vertex) const
  {
    std::vector<double> neighbour_edges;
    for (const auto& pair : adjacency_list_[vertex]) {
      neighbour_edges.push_back(pair.second);
    }
    return neighbour_edges;
  }

  float getVertexWeight(int vertex) const { return vertex_weights_[vertex]; }

  float getEdgeWeight(int v1, int v2) const
  {
    auto it = adjacency_list_[v1].find(v2);
    return (it != adjacency_list_[v1].end()) ? it->second : 0.0f;
  }

  // Get the number of vertices
  inline size_t numVertices() const { return num_vertices_; }
  bool calculateTotalweight()
  {
    double total_weight = 0.0;
    for (size_t i = 0; i < num_vertices_; i++) {
      for (const auto& pair : adjacency_list_[i]) {
        total_weight += pair.second;
      }
    }
    this->_total_weight = total_weight;
    return true;
  }
  inline double total_weight() { return this->_total_weight; };
  // esstimate edges count
  double possible_edges() const
  {
    return this->possible_edges(this->numVertices());
  }
  double possible_edges(double n) const { return n * (n - 1); }

  GraphForLeidenAlgorithm* collapse_graph(ModularityVertexPartition* partition);
};

}  // namespace mpl2
