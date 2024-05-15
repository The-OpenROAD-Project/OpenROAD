///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "bus_synthesis.h"

#include <ortools/linear_solver/linear_solver.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "object.h"
#include "utl/Logger.h"

namespace mpl2 {
using utl::MPL;

using operations_research::MPConstraint;
using operations_research::MPObjective;
using operations_research::MPSolver;
using operations_research::MPVariable;

///////////////////////////////////////////////////////////////////////
// Utility Functions

// Get vertices in a given segement
// We consider start terminal and end terminal
static void getVerticesInSegment(const std::vector<float>& grid,
                                 const float start_point,
                                 const float end_point,
                                 int& start_idx,
                                 int& end_idx)
{
  start_idx = 0;
  end_idx = 0;
  if (grid.empty() || start_point > end_point) {
    return;
  }
  // calculate start_idx
  while (start_idx < grid.size() && grid[start_idx] < start_point) {
    start_idx++;
  }
  // calculate end_idx
  while (end_idx < grid.size() && grid[end_idx] <= end_point) {
    end_idx++;
  }
}

// Get vertices within a given rectangle
// Calculate the start index and end index in the grid
static void getVerticesInRect(const std::vector<float>& x_grid,
                              const std::vector<float>& y_grid,
                              const Rect& rect,
                              int& x_start,
                              int& x_end,
                              int& y_start,
                              int& y_end)
{
  getVerticesInSegment(x_grid, rect.xMin(), rect.xMax(), x_start, x_end);
  getVerticesInSegment(y_grid, rect.yMin(), rect.yMax(), y_start, y_end);
}

//////////////////////////////////////////////////////////////
// Class Graph
Graph::Graph(int num_vertices, float congestion_weight)
    : adj_(num_vertices), congestion_weight_(congestion_weight)
{
}

// Add an edge to the adjacency matrix
void Graph::addEdge(int src, int dest, float weight, Edge* edge_ptr)
{
  adj_[src].push_back(Arrow{dest, weight, edge_ptr});
  // adj_[dest].push_back(Arrow{src, weight, edge_ptr});
}

// Define the comparator for VetexDist object, so VertexDist object can be
// used in priority_queue
class VertexDistComparator
{
 public:
  bool operator()(const VertexDist& x, const VertexDist& y)
  {
    return x.dist > y.dist;
  }
};

// Find the shortest paths relative to root vertex based on priority queue
// We store the paths in the format of parent vertices
// If we want to get real pathes, we need to traverse back the parent vertices
void Graph::calShortPathParentVertices(int root)
{
  // store the parent vertices for each vertex in the shortest paths
  // for example, there are two paths from root to dest
  // path1: root -> A -> dest
  // path2: root -> B -> dest
  // then dest vertex has two parents:  A and B
  // So the parent of each vertex is a vector instead of some vertex
  std::vector<std::vector<int>> parent(adj_.size());
  // initialization
  // set the dist to infinity, store the dist of each vertex related to root
  // vertex
  std::vector<float> dist(adj_.size(), std::numeric_limits<float>::max());
  // set all the vertices unvisited
  std::vector<bool> visited(adj_.size(), false);
  // initialize empty wavefront
  // the class is VertexDist (vertex, dist to src)
  // std::vector<VertexDist> is the class container
  // VertexDistComparator is the comparator, the first lement is
  // the greatest one (with shortest distance to src)
  std::priority_queue<VertexDist, std::vector<VertexDist>, VertexDistComparator>
      wavefront;
  // initialize root vertex
  parent[root] = {-1};  // set the parent of root vertex to { -1 }
  dist[root] = 0.0;
  wavefront.push(VertexDist{root, dist[root]});
  // Forward propagation
  while (!wavefront.empty()) {
    VertexDist vertex_dist = wavefront.top();
    wavefront.pop();
    // check if the vertex has been visited
    // we may have a vertex with different distances in the wavefront
    // only the shortest distance of the vertex should be used.
    if (visited[vertex_dist.vertex]) {
      continue;
    }
    // mark current vertex as visited
    visited[vertex_dist.vertex] = true;
    for (const auto& edge : adj_[vertex_dist.vertex]) {
      if (dist[edge.dest] > dist[vertex_dist.vertex] + edge.weight) {
        dist[edge.dest] = dist[vertex_dist.vertex] + edge.weight;
        parent[edge.dest].clear();
        parent[edge.dest].push_back(vertex_dist.vertex);
        wavefront.push(VertexDist{edge.dest, dist[edge.dest]});
      } else if (dist[edge.dest] == dist[vertex_dist.vertex] + edge.weight) {
        parent[edge.dest].push_back(vertex_dist.vertex);
      }
    }                                  // done edge traversal
  }                                    // done forward propagation
  parents_[root] = std::move(parent);  // update parents map
};

// Find real paths between root vertex and target vertex
// by traversing back the parent vertices in a recursive manner
// Similar to DFS (not exactly DFS)
void Graph::calShortPaths(
    // all paths between root vertex and target vertex
    std::vector<std::vector<int>>& paths,
    // current path between root vertex and target vertex
    std::vector<int>& path,
    // vector of parent vertices for root vertex
    std::vector<std::vector<int>>& parent_vertices,
    // current parent vertex
    int parent)
{
  if (paths.size() >= max_num_path_) {
    return;
  }

  // Base case
  if (parent == -1) {
    paths.push_back(path);
    return;
  }

  // Recursive case
  for (const auto& ancestor : parent_vertices[parent]) {
    path.push_back(parent);
    // This step is necessary to avoid loops caused by the edge with zero weight
    if (std::find(path.begin(), path.end(), ancestor) == path.end()) {
      calShortPaths(paths, path, parent_vertices, ancestor);
    }
    path.pop_back();
  }
}

// Calculate shortest edge paths
void Graph::calEdgePaths(
    // shortest paths, path = { vertex_id }
    std::vector<std::vector<int>>& paths,
    // shortest boundary edge paths
    std::vector<std::vector<int>>& edge_paths,
    // length of shortest paths
    float& hpwl)
{
  // map each edge in the adjacency matrix to edge_ptr
  std::vector<std::map<int, Edge*>> adj_map(adj_.size());
  for (int i = 0; i < adj_.size(); i++) {
    for (auto& arrow : adj_[i]) {
      adj_map[i][arrow.dest] = arrow.edge_ptr;
    }
  }
  // use sum(edge_id * edge_id) as hash value for each path
  std::set<int> path_hash_set;
  float distance = 0.0;
  std::vector<int> edge_path;
  for (const auto& path : paths) {
    // convert path to edge_path
    edge_path.clear();
    int hash_value = 0;
    for (int i = 0; i < path.size() - 1; i++) {
      Edge* edge_ptr = adj_map[path[i]][path[i + 1]];
      distance += edge_ptr->length * (1 - congestion_weight_);
      distance += edge_ptr->length_w * congestion_weight_;
      if (!edge_ptr->internal) {
        hash_value += edge_ptr->edge_id * edge_ptr->edge_id;
        edge_path.push_back(edge_ptr->edge_id);
      }
    }
    // add edge_path to edge_paths
    if (path_hash_set.find(hash_value) == path_hash_set.end()) {
      hpwl = distance;
      edge_paths.push_back(edge_path);
      path_hash_set.insert(hash_value);
    }  // done edge_path
  }    // done edge_paths
}

// Calculate shortest pathes in terms of boundary edges
void Graph::calNetEdgePaths(int src,
                            int target,
                            BundledNet& net,
                            utl::Logger* logger)
{
  debugPrint(logger, MPL, "bus_planning", 1, "Enter CalNetEdgePaths");
  // check if the parent vertices have been calculated
  if (parents_.find(src) == parents_.end()) {
    calShortPathParentVertices(src);  // calculate parent vertices
  }
  debugPrint(
      logger, MPL, "bus_planning", 1, "Finish CalShortPathParentVertices");
  // initialize an empty path
  std::vector<int> path;
  std::vector<std::vector<int>> paths;  // paths in vertex id
  calShortPaths(paths, path, parents_[src],
                target);  // pathes in vertex id
  debugPrint(logger, MPL, "bus_planning", 1, "Finish CalShortPaths");
  calEdgePaths(paths, net.edge_paths, net.hpwl);  // pathes in edges
  debugPrint(logger, MPL, "bus_planning", 1, "Finish CalEdgePaths");
}

///////////////////////////////////////////////////////////////////////////////////
// Top level functions
void createGraph(std::vector<SoftMacro>& soft_macros,     // placed soft macros
                 std::vector<int>& soft_macro_vertex_id,  // store the vertex id
                                                          // for each soft macro
                 std::vector<Edge>& edge_list,  // edge_list and vertex_list
                                                // are all empty list
                 std::vector<Vertex>& vertex_list,
                 utl::Logger* logger)
{
  // first use the boundaries of clusters to define the center of empty spaces
  // Then use the center of empty spaces and center of clusters to define hanan
  // grid
  std::set<float> x_bound_point;
  std::set<float> y_bound_point;
  std::set<float> x_hanan_point;
  std::set<float> y_hanan_point;
  for (const auto& soft_macro : soft_macros) {
    x_bound_point.insert(std::round(soft_macro.getX()));
    y_bound_point.insert(std::round(soft_macro.getY()));
    x_hanan_point.insert(
        std::round(soft_macro.getX() + soft_macro.getWidth() / 2.0));
    y_hanan_point.insert(
        std::round(soft_macro.getY() + soft_macro.getHeight() / 2.0));
    x_bound_point.insert(std::round(soft_macro.getX() + soft_macro.getWidth()));
    y_bound_point.insert(
        std::round(soft_macro.getY() + soft_macro.getHeight()));
  }
  auto it = x_bound_point.begin();
  while (it != x_bound_point.end()) {
    float midpoint = *it;
    it++;
    if (it != x_bound_point.end()) {
      midpoint = std::round((midpoint + *it) / 2.0);
      x_hanan_point.insert(midpoint);
    }
  }
  it = y_bound_point.begin();
  while (it != y_bound_point.end()) {
    float midpoint = *it;
    it++;
    if (it != y_bound_point.end()) {
      midpoint = std::round((midpoint + *it) / 2.0);
      y_hanan_point.insert(midpoint);
    }
  }
  std::vector<float> x_grid(x_hanan_point.begin(), x_hanan_point.end());
  std::vector<float> y_grid(y_hanan_point.begin(), y_hanan_point.end());
  // create vertex list based on the hanan grids in a row-based manner
  // each grid point cooresponds to a vertex
  // we assign weight to all the vertices
  // the weight of each vertex is the macro utilization of the cluster
  // (softmacro) to which it belongs to
  for (auto y : y_grid) {
    for (auto x : x_grid) {
      vertex_list.emplace_back(vertex_list.size(), Point(x, y));
    }
  }
  // initialize the macro_id and macro util for each vertex
  for (auto& vertex : vertex_list) {
    vertex.weight = 0.0;   // weight is the macro util
    vertex.macro_id = -1;  // macro_id
  }

  debugPrint(logger, MPL, "bus_planning", 1, "Finish Creating vertex list");

  debugPrint(logger, MPL, "bus_planning", 1, "x_grid:  ");

  for (auto& x : x_grid) {
    debugPrint(logger, MPL, "bus_planning", 1, " {} ", x);
  }
  debugPrint(logger, MPL, "bus_planning", 1, "\n");
  debugPrint(logger, MPL, "bus_planning", 1, "y_grid:  ");
  for (auto& y : y_grid) {
    debugPrint(logger, MPL, "bus_planning", 1, " {} ", y);
  }
  debugPrint(logger, MPL, "bus_planning", 1, "\n");

  int macro_id = 0;
  for (const auto& soft_macro : soft_macros) {
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "vertices in macro : {}",
               soft_macro.getName());
    const float lx = std::round(soft_macro.getX());
    const float ly = std::round(soft_macro.getY());
    const float ux = std::round(soft_macro.getX() + soft_macro.getWidth());
    const float uy = std::round(soft_macro.getY() + soft_macro.getHeight());
    const float cx
        = std::round(soft_macro.getX() + soft_macro.getWidth() / 2.0);
    const float cy
        = std::round(soft_macro.getY() + soft_macro.getHeight() / 2.0);
    Rect rect(lx, ly, ux, uy);
    // calculate the macro utilization of the soft macro
    float macro_util = soft_macro.getMacroUtil();
    // find the vertices within the soft macro
    int x_start = -1;
    int y_start = -1;
    int x_end = -1;
    int y_end = -1;
    getVerticesInRect(x_grid, y_grid, rect, x_start, x_end, y_start, y_end);
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "x_start :  {} x_end: {} y_start: {} y_end: {}",
               x_start,
               x_end,
               y_start,
               y_end);
    debugPrint(logger, MPL, "bus_planning", 1, "lx  :  {} ux: {}", cx, cy);
    debugPrint(logger, MPL, "bus_planning", 1, "ly  :  {} uy: {}", cx, cy);
    debugPrint(logger, MPL, "bus_planning", 1, "cx  :  {} cy: {}", cx, cy);

    bool test_flag = false;

    // set the weight for vertices within soft macros
    for (int y_idx = y_start; y_idx < y_end; y_idx++) {
      for (int x_idx = x_start; x_idx < x_end; x_idx++) {
        const int vertex_id = y_idx * x_grid.size() + x_idx;
        vertex_list[vertex_id].weight = macro_util;
        vertex_list[vertex_id].macro_id = macro_id;
        // if the grid point is the center of a SoftMacro
        if (x_grid[x_idx] == cx && y_grid[y_idx] == cy) {
          soft_macro_vertex_id.push_back(vertex_id);
          test_flag = true;
        }
        // if the grid point is on the left or right boundry
        if (x_grid[x_idx] == lx || x_grid[x_idx] == ux) {
          vertex_list[vertex_id].disable_v_edge = true;
        }
        // if the grid point is on the top or bottom boundry
        if (y_grid[y_idx] == ly || y_grid[y_idx] == uy) {
          vertex_list[vertex_id].disable_h_edge = true;
        }
      }
    }
    if (!test_flag) {
      logger->report("Error\n\n");
    }

    // increase macro id
    macro_id++;
    if (soft_macro.getArea() <= 0.0) {
      debugPrint(logger,
                 MPL,
                 "bus_planning",
                 1,
                 "macro_id : {}  {}",
                 macro_id,
                 soft_macro_vertex_id.size());
    }
  }

  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "soft_macro_vertex_id.size() : {} soft_macros.size(): {}",
             soft_macro_vertex_id.size(),
             soft_macros.size());
  debugPrint(logger, MPL, "bus_planning", 1, "Finish macro_id assignment");
  // print vertex id
  for (int i = 0; i < soft_macros.size(); i++) {
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "macro_id : {} vertex_id: {} macro_id: {}",
               i,
               soft_macro_vertex_id[i],
               vertex_list[soft_macro_vertex_id[i]].vertex_id);
  }

  // add all the edges between grids (undirected)
  // add all the horizontal edges
  for (int y_idx = 0; y_idx < y_grid.size(); y_idx++) {
    for (int x_idx = 0; x_idx < x_grid.size() - 1; x_idx++) {
      const int src = y_idx * x_grid.size() + x_idx;
      const int target = src + 1;
      if (vertex_list[src].disable_h_edge || vertex_list[target].disable_h_edge
          || vertex_list[src].disable_v_edge
          || vertex_list[target].disable_v_edge) {
        continue;
      }
      Edge edge(edge_list.size());  // create an edge with edge id
      edge.terminals = std::pair<int, int>(src, target);
      edge.direction = true;  // true means horizontal edge
      edge.length = x_grid[x_idx + 1] - x_grid[x_idx];
      // calculate edge type (internal or not)
      // and weighted length
      const int& src_macro_id = vertex_list[src].macro_id;
      const int& target_macro_id = vertex_list[target].macro_id;
      if (src_macro_id == target_macro_id) {
        // this is an internal edge
        edge.internal = true;
        edge.length_w = vertex_list[src].weight * edge.length;
      } else {
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getX() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[target_macro_id].getX());
        } else if (target_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[src_macro_id].getX()
                   + soft_macros[src_macro_id].getWidth() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[src_macro_id].getX()
                    - soft_macros[src_macro_id].getWidth());
        } else {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getX() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[target_macro_id].getX());
        }
      }
      edge_list.push_back(edge);
    }
  }
  // add the vertical edges
  for (int x_idx = 0; x_idx < x_grid.size(); x_idx++) {
    for (int y_idx = 0; y_idx < y_grid.size() - 1; y_idx++) {
      const int src = y_idx * x_grid.size() + x_idx;
      const int target = src + x_grid.size();
      if (vertex_list[src].disable_h_edge || vertex_list[target].disable_h_edge
          || vertex_list[src].disable_v_edge
          || vertex_list[target].disable_v_edge) {
        continue;
      }
      Edge edge(edge_list.size());  // create an edge with edge id
      edge.terminals = std::pair<int, int>(src, target);
      edge.direction = false;  // false means vertical edge
      edge.length = y_grid[y_idx + 1] - y_grid[y_idx];
      // calculate edge type (internal or not)
      // and weighted length
      const int& src_macro_id = vertex_list[src].macro_id;
      const int& target_macro_id = vertex_list[target].macro_id;
      if (src_macro_id == target_macro_id) {
        // this is an internal edge
        edge.internal = true;
        edge.length_w = vertex_list[src].weight * edge.length;
      } else {
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getY() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[target_macro_id].getY());
        } else if (target_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[src_macro_id].getY()
                   + soft_macros[src_macro_id].getHeight() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[src_macro_id].getY()
                    - soft_macros[src_macro_id].getHeight());
        } else {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getY() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[target_macro_id].getY());
        }
      }
      edge_list.push_back(edge);
    }
  }

  debugPrint(logger, MPL, "bus_planning", 1, "finish edge list");
  // handle the vertices on left or right boundaries
  for (int y_idx = 0; y_idx < y_grid.size(); y_idx++) {
    for (int x_idx = 1; x_idx < x_grid.size() - 1; x_idx++) {
      auto& vertex = vertex_list[y_idx * x_grid.size() + x_idx];
      if (!vertex.disable_v_edge && !vertex.disable_h_edge) {
        continue;
      }
      if (vertex.disable_v_edge && vertex.disable_h_edge) {
        continue;
      }
      if (vertex.disable_v_edge) {
        const int src = vertex.vertex_id - 1;
        const int target = vertex.vertex_id + 1;
        Edge edge(edge_list.size());  // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = true;  // true means horizontal edge
        edge.length = x_grid[x_idx + 1] - x_grid[x_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length

        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "src: {}, target: {}, vertex_list size: {}",
                   src,
                   target,
                   vertex_list.size());

        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "src_macro_id: {} target_macro_id: {} num soft macros: {}",
                   src_macro_id,
                   target_macro_id,
                   soft_macros.size());
        // this is an edge crossing boundaries
        edge.internal = false;
        // exception handling.  Later we should find better way to handle this.
        // [20221202]
        if (src_macro_id == -1 && target_macro_id == -1) {
          edge.length_w = edge.length;
          continue;
        }

        if (src_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getX() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[target_macro_id].getX());
        } else if (target_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[src_macro_id].getX()
                   + soft_macros[src_macro_id].getWidth() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[src_macro_id].getX()
                    - soft_macros[src_macro_id].getWidth());
        } else {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getX() - x_grid[x_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (x_grid[x_idx + 1] - soft_macros[target_macro_id].getX());
        }
        edge_list.push_back(edge);
      }
    }
  }

  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "finish boundary edges (left and right boundaries)");

  // handle the vertices on top or bottom boundaries
  for (int y_idx = 1; y_idx < y_grid.size() - 1; y_idx++) {
    for (int x_idx = 0; x_idx < x_grid.size(); x_idx++) {
      auto& vertex = vertex_list[y_idx * x_grid.size() + x_idx];
      if (!vertex.disable_v_edge && !vertex.disable_h_edge) {
        continue;
      }
      if (vertex.disable_v_edge && vertex.disable_h_edge) {
        continue;
      }
      if (vertex.disable_h_edge) {
        const int src = vertex.vertex_id - x_grid.size();
        const int target = vertex.vertex_id + x_grid.size();
        Edge edge(edge_list.size());  // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = false;  // false means vertical edge
        edge.length = y_grid[y_idx + 1] - y_grid[y_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "src: {} target: {} vertex_list size: {}",
                   src,
                   target,
                   vertex_list.size());
        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "src_macro_id: {}, target_macro_id: {}, num soft macros: {}",
                   src_macro_id,
                   target_macro_id,
                   soft_macros.size());
        // this is an edge crossing boundaries
        edge.internal = false;
        // exception handling.  Later we should find better way to handle this.
        // [20221202]
        if (src_macro_id == -1 && target_macro_id == -1) {
          edge.length_w = edge.length;
          continue;
        }

        if (src_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getY() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[target_macro_id].getY());
        } else if (target_macro_id == -1) {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[src_macro_id].getY()
                   + soft_macros[src_macro_id].getHeight() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[src_macro_id].getY()
                    - soft_macros[src_macro_id].getHeight());
        } else {
          edge.length_w
              = vertex_list[src].weight
                * (soft_macros[target_macro_id].getY() - y_grid[y_idx]);
          edge.length_w
              += vertex_list[target].weight
                 * (y_grid[y_idx + 1] - soft_macros[target_macro_id].getY());
        }
        edge_list.push_back(edge);
      }
    }
  }

  debugPrint(logger, MPL, "bus_planning", 1, "finish boundary edges");

  // handle all the IO cluster
  for (int i = 0; i < soft_macros.size(); i++) {
    if (soft_macros[i].getArea() > 0.0) {
      continue;
    }
    auto& vertex = vertex_list[soft_macro_vertex_id[i]];
    vertex.macro_id = i;  // update the macro id
    debugPrint(
        logger, MPL, "bus_planning", 1, "macro_id : {}", vertex.macro_id);
    std::set<int> neighbors;
    // add horizontal edges
    if (vertex.pos.first == *(x_grid.begin())) {
      // left boundary
      neighbors.insert(vertex.vertex_id - x_grid.size() + 1);
      neighbors.insert(vertex.vertex_id + x_grid.size() + 1);
      neighbors.insert(vertex.vertex_id + 1);
    } else if (vertex.pos.first == *(std::prev(x_grid.end(), 1))) {
      // right boundary
      neighbors.insert(vertex.vertex_id - x_grid.size() - 1);
      neighbors.insert(vertex.vertex_id + x_grid.size() - 1);
      neighbors.insert(vertex.vertex_id - 1);
    }
    debugPrint(logger, MPL, "bus_planning", 1, "step1");
    for (auto& neighbor : neighbors) {
      if (neighbor >= vertex_list.size()) {
        continue;
      }
      auto& n_vertex = vertex_list[neighbor];
      if (n_vertex.disable_v_edge || n_vertex.disable_h_edge) {
        continue;
      }
      Edge edge(edge_list.size());  // create an edge with edge id
      edge.terminals
          = std::pair<int, int>(vertex.vertex_id, n_vertex.vertex_id);
      edge.direction = true;  // false means horizontal edge
      edge.length = std::abs(vertex.pos.first - n_vertex.pos.first);
      edge.internal = false;
      edge.length_w = n_vertex.weight * edge.length;
      edge_list.push_back(edge);
    }
    debugPrint(logger, MPL, "bus_planning", 1, "step2");
    // add vertical edges
    neighbors.clear();
    if (vertex.pos.second == *(y_grid.begin())) {
      // bottom boundary
      neighbors.insert(vertex.vertex_id + x_grid.size() - 1);
      neighbors.insert(vertex.vertex_id + x_grid.size() + 1);
      neighbors.insert(vertex.vertex_id + x_grid.size());
    } else if (vertex.pos.second == *(std::prev(y_grid.end(), 1))) {
      // top boundary
      neighbors.insert(vertex.vertex_id - x_grid.size() - 1);
      neighbors.insert(vertex.vertex_id - x_grid.size() + 1);
      neighbors.insert(vertex.vertex_id - x_grid.size() - 1);
    }
    debugPrint(logger, MPL, "bus_planning", 1, "step3");
    for (auto& neighbor : neighbors) {
      if (neighbor >= vertex_list.size()) {
        continue;
      }
      auto& n_vertex = vertex_list[neighbor];
      if (n_vertex.disable_v_edge || n_vertex.disable_h_edge) {
        continue;
      }
      Edge edge(edge_list.size());  // create an edge with edge id
      edge.terminals
          = std::pair<int, int>(vertex.vertex_id, n_vertex.vertex_id);
      edge.direction = false;  // false means vertical edge
      edge.length = std::abs(vertex.pos.second - n_vertex.pos.second);
      edge.internal = false;
      edge.length_w = n_vertex.weight * edge.length;
      edge_list.push_back(edge);
    }
    debugPrint(logger, MPL, "bus_planning", 1, "step4");
  }

  debugPrint(logger, MPL, "bus_planning", 1, "finish io cluster related edges");

  // update edge weight and pin access
  for (auto& edge : edge_list) {
    edge.weight = std::max(vertex_list[edge.terminals.first].weight,
                           vertex_list[edge.terminals.second].weight);
    // for the edge crossing soft macros
    if (!edge.internal) {
      if (edge.direction) {  // horizontal
        edge.pin_access = R;
      } else {
        edge.pin_access = T;
      }
    }  // update crossing edge
  }    // update edge weight

  int num_edges = edge_list.size();
  for (int i = 0; i < num_edges; i++) {
    edge_list.emplace_back(edge_list.size(), edge_list[i]);
  }

  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "\n****************************************");
  debugPrint(
      logger, MPL, "bus_planning", 1, "macro_id,  macro,  vertex_id, macro_id");
  for (int i = 0; i < soft_macros.size(); i++) {
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "i:  {}  {}  {} {}",
               i,
               soft_macros[i].getName(),
               soft_macro_vertex_id[i],
               vertex_list[soft_macro_vertex_id[i]].macro_id);
  }
  debugPrint(logger, MPL, "bus_planning", 1, "exiting create graph");
}

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
                 utl::Logger* logger)
{
  // create vertex_list and edge_list
  createGraph(
      soft_macros, soft_macro_vertex_id, edge_list, vertex_list, logger);
  // create graph based on vertex list and edge list
  Graph graph(vertex_list.size(), congestion_weight);
  for (auto& edge : edge_list) {
    float weight = edge.length * (1 - congestion_weight)
                   + edge.length_w * congestion_weight;  // cal edge weight
    if (weight <= 0.0) {
      debugPrint(logger,
                 MPL,
                 "bus_planning",
                 1,
                 "warning weight < 0 - length: {} length_w: {}",
                 edge.length,
                 edge.length_w);
    }
    graph.addEdge(edge.terminals.first, edge.terminals.second, weight, &edge);
  }
  // Find all the shortest paths based on graph
  int num_paths = 0;
  // map each candidate path to its related net
  std::map<int, int> path_net_map;       // <path_id, net_id>
  std::map<int, int> path_net_path_map;  // <path_id, path_id>
  int net_id = 0;
  for (auto& net : nets) {
    // calculate candidate paths
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "calculate the path for net: {} . {}",
               net.terminals.first,
               net.terminals.second);
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "cluster :  {}   {}",
               soft_macros[net.terminals.first].getName(),
               soft_macros[net.terminals.second].getName());
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "{}  {}",
               soft_macro_vertex_id[net.terminals.first],
               soft_macro_vertex_id[net.terminals.second]);
    graph.calNetEdgePaths(soft_macro_vertex_id[net.terminals.second],
                          soft_macro_vertex_id[net.terminals.first],
                          net,
                          logger);
    // update path id
    int path_id = 0;
    for (const auto& edge_path : net.edge_paths) {
      // here the edge paths only include edges crossing soft macros (IOs)
      debugPrint(logger, MPL, "bus_planning", 1, "path :  ");
      for (auto& edge_id : edge_path) {
        debugPrint(logger, MPL, "bus_planning", 1, "\t{} ", edge_id);
      }
      path_net_path_map[num_paths] = path_id++;
      path_net_map[num_paths++] = net_id;
    }
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "number candidate paths is {}",
               net.edge_paths.size());
    net_id++;
  }

  debugPrint(logger, MPL, "bus_planning", 1, "\nAll the candidate paths");
  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "Total number of candidate paths : {}",
             num_paths);
  for (auto& net : nets) {
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "---------------------------------------");
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "src :  {}  target {}",
               net.terminals.first,
               net.terminals.second);
    for (auto& edge_path : net.edge_paths) {
      debugPrint(logger, MPL, "bus_planning", 1, "path :  ");
      for (auto& edge_id : edge_path) {
        debugPrint(logger, MPL, "bus_planning", 1, " {}  ", edge_id);
      }
      debugPrint(logger, MPL, "bus_planning", 1, "\n");
    }
  }

  // Google OR-TOOLS for SCIP Implementation
  // create the ILP solver with the SCIP backend
  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));
  if (!solver) {
    logger->report("Error ! SCIP solver unavailable!");
    return false;
  }

  // For each path, define a variable x
  std::vector<const MPVariable*> x(num_paths);
  for (int i = 0; i < num_paths; i++) {
    x[i] = solver->MakeIntVar(0.0, 1.0, "");
  }

  // For each edge, define a variable y
  std::vector<const MPVariable*> y(edge_list.size());
  for (int i = 0; i < edge_list.size(); i++) {
    y[i] = solver->MakeIntVar(0.0, 1.0, "");
  }
  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "Number of variables = {}",
             solver->NumVariables());
  const double infinity = solver->infinity();

  // add constraints
  int x_id = 0;
  for (auto& net : nets) {
    // need take a detail look [fix]
    if (net.edge_paths.empty()) {
      continue;
    }
    MPConstraint* net_c = solver->MakeRowConstraint(1, 1, "");
    for (auto& edge_path : net.edge_paths) {
      for (auto& edge_id : edge_path) {
        MPConstraint* edge_c = solver->MakeRowConstraint(0, infinity, "");
        edge_c->SetCoefficient(x[x_id], -1);
        edge_c->SetCoefficient(y[edge_id], 1);
      }
      net_c->SetCoefficient(x[x_id++], 1);
    }
  }
  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "Number of constraints = {}",
             solver->NumConstraints());

  // Create the objective function
  MPObjective* const objective = solver->MutableObjective();
  for (int i = 0; i < edge_list.size(); i++) {
    objective->SetCoefficient(y[i], edge_list[i].weight);
  }

  objective->SetMinimization();
  const MPSolver::ResultStatus result_status = solver->Solve();
  // Check that the problem has an optimal solution.
  if (result_status != MPSolver::OPTIMAL) {
    return false;  // The problem does not have an optimal solution;
  }
  debugPrint(logger, MPL, "bus_planning", 1, "Soluton : ");
  debugPrint(logger,
             MPL,
             "bus_planning",
             1,
             "Optimal objective value = {}",
             objective->Value());

  // Generate the solution and check which edge get selected
  for (int i = 0; i < num_paths; i++) {
    if (x[i]->solution_value() == 0) {
      continue;
    }

    debugPrint(logger, MPL, "bus_planning", 1, "working on path {}", i);
    auto target_cluster
        = soft_macros[nets[path_net_map[i]].terminals.second].getCluster();
    Boundary src_pin = NONE;
    Cluster* pre_cluster = nullptr;
    int last_edge_id = -1;
    const float net_weight = nets[path_net_map[i]].weight;
    const int src_cluster_id = nets[path_net_map[i]].src_cluster_id;
    const int target_cluster_id = nets[path_net_map[i]].target_cluster_id;
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "src_cluster_id : {} target_cluster_id: {} ",
               src_cluster_id,
               target_cluster_id);
    debugPrint(logger,
               MPL,
               "bus_planning",
               1,
               "src_macro_id : {}  target_macro_id {}",
               nets[path_net_map[i]].terminals.first,
               nets[path_net_map[i]].terminals.second);

    for (auto& edge_id :
         nets[path_net_map[i]].edge_paths[path_net_path_map[i]]) {
      auto& edge = edge_list[edge_id];
      debugPrint(logger,
                 MPL,
                 "bus_planning",
                 1,
                 "edge_terminals : {}  {}",
                 edge.terminals.first,
                 edge.terminals.second);
      debugPrint(logger,
                 MPL,
                 "bus_planning",
                 1,
                 "edge_terminals_macro_id : {}  {} ",
                 vertex_list[edge.terminals.first].macro_id,
                 vertex_list[edge.terminals.second].macro_id);
      last_edge_id = edge_id;
      Cluster* start_cluster = nullptr;
      if (vertex_list[edge.terminals.first].macro_id != -1) {
        start_cluster = soft_macros[vertex_list[edge.terminals.first].macro_id]
                            .getCluster();
        debugPrint(
            logger,
            MPL,
            "bus_planning",
            1,
            "start_name : {}",
            soft_macros[vertex_list[edge.terminals.first].macro_id].getName());
      }
      if (start_cluster != nullptr) {
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "start_cluster_id : {}",
                   start_cluster->getId());
      }
      Cluster* end_cluster = nullptr;
      if (vertex_list[edge.terminals.second].macro_id != -1) {
        end_cluster = soft_macros[vertex_list[edge.terminals.second].macro_id]
                          .getCluster();
        debugPrint(
            logger,
            MPL,
            "bus_planning",
            1,
            "end_name : {}",
            soft_macros[vertex_list[edge.terminals.second].macro_id].getName());
      }
      if (end_cluster != nullptr) {
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "end_cluster_id : {}",
                   end_cluster->getId());
      }

      if (start_cluster == nullptr && end_cluster == nullptr) {
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "bus planning - error condition nullptr");
      } else if (start_cluster != nullptr && end_cluster != nullptr) {
        if (start_cluster->getId() == src_cluster_id) {
          start_cluster->setPinAccess(
              target_cluster_id, edge.pin_access, net_weight);
          src_pin = opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else if (end_cluster->getId() == src_cluster_id) {
          end_cluster->setPinAccess(
              target_cluster_id, opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else {
          if (start_cluster != pre_cluster && end_cluster != pre_cluster) {
            debugPrint(logger,
                       MPL,
                       "bus_planning",
                       1,
                       "bus planning - error condition pre_cluster");
          } else if (start_cluster == pre_cluster) {
            start_cluster->addBoundaryConnection(
                src_pin, edge.pin_access, net_weight);
            src_pin = opposite(edge.pin_access);
            pre_cluster = end_cluster;
          } else {
            end_cluster->addBoundaryConnection(
                src_pin, opposite(edge.pin_access), net_weight);
            src_pin = edge.pin_access;
            pre_cluster = start_cluster;
          }
        }
      } else if (start_cluster != nullptr) {
        if (start_cluster->getId() == src_cluster_id) {
          start_cluster->setPinAccess(
              target_cluster_id, edge.pin_access, net_weight);
          src_pin = opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else if (start_cluster != pre_cluster) {
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else {
          start_cluster->addBoundaryConnection(
              src_pin, edge.pin_access, net_weight);
          src_pin = opposite(edge.pin_access);
          pre_cluster = end_cluster;
        }
      } else {
        if (end_cluster->getId() == src_cluster_id) {
          end_cluster->setPinAccess(
              target_cluster_id, opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else if (end_cluster != pre_cluster) {
          src_pin = opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else {
          end_cluster->addBoundaryConnection(
              src_pin, opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        }
      }
    }
    if (target_cluster != nullptr && last_edge_id >= 0) {
      auto& edge = edge_list[last_edge_id];
      Cluster* start_cluster = nullptr;
      if (vertex_list[edge.terminals.first].macro_id != -1) {
        start_cluster = soft_macros[vertex_list[edge.terminals.first].macro_id]
                            .getCluster();
      }
      if (start_cluster != nullptr) {
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "start_cluster_id : {}",
                   start_cluster->getId());
      }
      Cluster* end_cluster = nullptr;
      if (vertex_list[edge.terminals.second].macro_id != -1) {
        end_cluster = soft_macros[vertex_list[edge.terminals.second].macro_id]
                          .getCluster();
      }
      if (end_cluster != nullptr) {
        debugPrint(logger,
                   MPL,
                   "bus_planning",
                   1,
                   "end_cluster_id : {}",
                   end_cluster->getId());
      }
      if (start_cluster == target_cluster) {
        target_cluster->setPinAccess(
            src_cluster_id, edge.pin_access, net_weight);
      } else if (end_cluster == target_cluster) {
        target_cluster->setPinAccess(
            src_cluster_id, opposite(edge.pin_access), net_weight);
      } else {
        logger->report("(3) Error ! This should not happen");
      }
    }
    debugPrint(logger, MPL, "bus_planning", 1, "finish path {}", i);
  }

  return true;
}

}  // namespace mpl2
