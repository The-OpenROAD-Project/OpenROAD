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

// ************************************************
// Route important buses based on current layout
// ************************************************

#include <map>
#include <vector>

#include "object.h"

namespace mpl2 {

using Point = std::pair<float, float>;

// Each point in the hanan grid is represented by a vertex (with no size)
// And each bundled IO pin is represented by a vertex
struct Vertex
{
  Vertex(int vertex_id, Point pos) : vertex_id(vertex_id), pos(pos) {}

  // vertex_id of current vertex
  int vertex_id = -1;

  // soft_macro_id of the SoftMacro which the vertex belongs to
  int macro_id = -1;

  // disable the vertical connection of this vertex
  bool disable_v_edge = false;

  // disable the horizontal connection of this vertex
  bool disable_h_edge = false;

  // position of the vertex
  Point pos;

  // The weight of the vertex : macro utilization of the SoftMacro
  // which the vertex belongs to.  For bundled IO pin, we set the
  // macro utilization to be zero
  float weight = 0.0;
};

struct VertexDist
{
  VertexDist(int vertex, float dist) : vertex(vertex), dist(dist) {}

  int vertex;  // vertex_id of current vertex
  float dist;  // shortest distance (currently) between vertex and root vertex
};

// The connection between two vertices is represented by an edge
struct Edge
{
  Edge(int edge_id) : edge_id(edge_id) {}
  // create a reverse edge by changing the order of terminals
  Edge(int edge_id, Edge& edge)
  {
    this->edge_id = edge_id;
    terminals = {edge.terminals.second, edge.terminals.first};
    internal = edge.internal;
    pin_access = opposite(edge.pin_access);
    length = edge.length;
    length_w = edge.length_w;
    weight = edge.weight;
    num_nets = edge.num_nets;
  }

  int edge_id;                    // edge id of current edge
  std::pair<int, int> terminals;  // the vertex_id of two terminal vertices
  bool direction = false;         // True for horizontal and False for vertical
  bool internal = true;  // True for edge within one SoftMacro otherwise false
  Boundary pin_access
      = NONE;            // pin_access for internal == false (for src vertex)
  float length = 0.0;    // the length of edge
  float length_w = 0.0;  // weighted length : weight * length
  float weight = 0.0;    // the largeest macro utilization of terminals
  float num_nets = 0.0;  // num_nets passing through this edge
};

// We use Arrow object in the adjacency matrix to represent the grid graph
struct Arrow
{
  Arrow(int dest, float weight, Edge* edge_ptr)
      : dest(dest), weight(weight), edge_ptr(edge_ptr)
  {
  }

  int dest;                  // src -> dest (destination)
  float weight;              // weight must be nonnegative (or cost)
  Edge* edge_ptr = nullptr;  // the pointer of corresponding edge
};

// Grid graph for the clustered netlist
// Note that the graph is a connected undirected graph
class Graph
{
 public:
  Graph(int num_vertices, float congestion_weight);

  void addEdge(int src, int dest, float weight, Edge* edge_ptr);

  // Calculate shortest pathes in terms of boundary edges
  void calNetEdgePaths(int src,
                       int target,
                       BundledNet& net,
                       utl::Logger* logger);

  bool isConnected() const;  // check the GFS is connected

 private:
  // Find the shortest paths relative to root vertex based on priority queue
  // We store the paths in the format of parent vertices
  // If we want to get real pathes, we need to traverse back the parent vertices
  void calShortPathParentVertices(int root);
  // Find real paths between root vertex and target vertex
  // by traversing back the parent vertices in a recursive manner
  // Similar to DFS (not exactly DFS)
  void calShortPaths(
      // all paths between root vertex and target vertex
      std::vector<std::vector<int>>& paths,
      // current path between root vertex and target vertex
      std::vector<int>& path,
      // vector of parent vertices for root vertex
      std::vector<std::vector<int>>& parent_vertices,
      // current parent vertex
      int parent);
  // Calculate shortest edge paths
  void calEdgePaths(
      // shortest paths, path = { vertex_id }
      std::vector<std::vector<int>>& paths,
      // shortest boundary edge paths
      std::vector<std::vector<int>>& edge_paths,
      // length of shortest paths
      float& hpwl);

  // store the parent vertices for each vertex in the shortest paths
  // for example, there are two paths from root to dest
  // path1: root -> A -> dest
  // path2: root -> B -> dest
  // then dest vertex has two parents:  A and B
  // So the parent of each vertex is a vector instead of some vertex
  std::map<int, std::vector<std::vector<int>>> parents_;
  std::vector<std::vector<Arrow>> adj_;  // adjacency matrix
  // limit the maximum number of candidate paths to reduce runtime
  int max_num_path_ = 10;
  float congestion_weight_ = 1.0;
};

// Calculate the paths for global buses with ILP
// congestion_weight : the cost for each edge is
// (1 - congestion_weight) * length + congestion_weight * length_w
bool calNetPaths(std::vector<SoftMacro>& soft_macros,     // placed soft macros
                 std::vector<int>& soft_macro_vertex_id,  // store the vertex id
                                                          // for each soft macro
                 std::vector<Edge>& edge_list,
                 std::vector<Vertex>& vertex_list,
                 std::vector<BundledNet>& nets,
                 // parameters
                 float congestion_weight,
                 utl::Logger* logger);

}  // namespace mpl2
