#include "bus_synthesis.h"

#include <iostream>
#include <queue>
#include <vector>
#include <limits>
#include <map>
#include <algorithm>
#include <iterator>
#include <set>
#include <absl/flags/flag.h>
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <ortools/base/commandlineflags.h>
#include <ortools/base/init_google.h>
#include <ortools/base/logging.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/linear_solver/linear_solver.pb.h>
#include "object.h"

#ifdef USE_CPLEX
#include "ilcplex/cplex.h"
#include "ilcplex/ilocplex.h"
#endif

namespace mpl {

using operations_research::MPSolver;
using operations_research::MPObjective;
using operations_research::MPConstraint;
using operations_research::MPVariable;

//////////////////////////////////////////////////////////////
// Class Graph
// Constructors
Graph::Graph(int num_vertices, float congestion_weight)
  : adj_(num_vertices), congestion_weight_(congestion_weight)  {   }

// Add an edge to the adjacency matrix
void Graph::AddEdge(int src, int dest, float weight, Edge* edge_ptr) 
{
  adj_[src].push_back(Arrow{dest, weight, edge_ptr});
  //adj_[dest].push_back(Arrow{src, weight, edge_ptr});
}

// Find the shortest paths relative to root vertex based on priority queue 
// We store the paths in the format of parent vertices
// If we want to get real pathes, we need to traverse back the parent vertices
void Graph::CalShortPathParentVertices(int root) 
{
  // store the parent vertices for each vertex in the shortest paths
  // for example, there are two paths from root to dest
  // path1: root -> A -> dest
  // path2: root -> B -> dest
  // then dest vertex has two parents:  A and B 
  // So the parent of each vertex is a vector instead of some vertex
  std::vector<std::vector<int> > parent(adj_.size());
  // initialization
  // set the dist to infinity, store the dist of each vertex related to root vertex
  std::vector<float> dist(adj_.size(), std::numeric_limits<float>::max());
  // set all the vertices unvisited
  std::vector<bool> visited(adj_.size(), false);
  // initialize empty wavefront 
  // the class is VertexDist (vertex, dist to src)
  // std::vector<VertexDist> is the class container
  // VertexDistComparator is the comparator, the first lement is
  // the greatest one (with shortest distance to src)
  std::priority_queue<VertexDist, std::vector<VertexDist>, 
                      VertexDistComparator> wavefront;
  // initialize root vertex
  parent[root] = { -1 };  // set the parent of root vertex to { -1 }
  dist[root] = 0.0;
  wavefront.push(VertexDist{root, dist[root]});
  // Forward propagation
  while (wavefront.empty() == false) {
    VertexDist vertex_dist = wavefront.top();
    wavefront.pop();
    // check if the vertex has been visited
    // we may have a vertex with different distances in the wavefront   
    // only the shortest distance of the vertex should be used.
    if (visited[vertex_dist.vertex] == true)
      continue;
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
    } // done edge traversal
  } // done forward propagation
  parents_[root] = parent; // update parents map
};


// Find real paths between root vertex and target vertex
// by traversing back the parent vertices in a recursive manner
// Similar to DFS (not exactly DFS)
void Graph::CalShortPaths(
     // all paths between root vertex and target vertex
     std::vector<std::vector<int> >& paths, 
     // current path between root vertex and target vertex
     std::vector<int>& path,
     // vector of parent vertices for root vertex
     std::vector<std::vector<int> >& parent_vertices,
     // current parent vertex
     int parent)
{
  if (paths.size() >= max_num_path_)
    return;

  // Base case
  if (parent == -1) {
    paths.push_back(path);
    return;
  }

  // Recursive case
  for (const auto& ancestor : parent_vertices[parent]) {
    path.push_back(parent);
    // This step is necessary to avoid loops caused by the edge with zero weight
    if (std::find(path.begin(), path.end(), ancestor) == path.end())
      CalShortPaths(paths, path, parent_vertices, ancestor);     
    path.pop_back();    
  }
}


// Calculate shortest edge paths
void Graph::CalEdgePaths(
        // shortest paths, path = { vertex_id }
        std::vector<std::vector<int> >& paths, 
        // shortest boundary edge paths
        std::vector<std::vector<int> >& edge_paths,
        // length of shortest paths
        float& HPWL)
{
  // map each edge in the adjacency matrix to edge_ptr
  std::vector<std::map<int, Edge*> > adj_map(adj_.size());   
  for (int i = 0; i < adj_.size(); i++)
    for (auto& arrow : adj_[i]) 
      adj_map[i][arrow.dest] = arrow.edge_ptr;
  // use sum(edge_id * edge_id) as hash value for each path   
  std::set<int>  path_hash_set;
  float distance = 0.0;
  std::vector<int> edge_path;
  int hash_value = 0;
  for (const auto& path : paths) {
    // convert path to edge_path
    edge_path.clear();
    hash_value = 0;
    for (int i = 0; i < path.size() - 1; i++) {
      Edge* edge_ptr = adj_map[path[i]][path[i+1]];
      distance += edge_ptr->length * (1 - congestion_weight_);
      distance += edge_ptr->length_w * congestion_weight_;
      if (edge_ptr->internal == false) {
        hash_value += edge_ptr->edge_id * edge_ptr->edge_id;
        edge_path.push_back(edge_ptr->edge_id);
      }
    }
    // add edge_path to edge_paths
    if (path_hash_set.find(hash_value) == path_hash_set.end()) {
      HPWL = distance;
      edge_paths.push_back(edge_path);
      path_hash_set.insert(hash_value);
    } // done edge_path
  } // done edge_paths
}

// Calculate shortest pathes in terms of boundary edges
void Graph::CalNetEdgePaths(int src, int target, BundledNet& net)
{
  std::cout << "Enter CalNetEdgePaths" << std::endl;
  // check if the parent vertices have been calculated
  if (parents_.find(src) == parents_.end())
    CalShortPathParentVertices(src); // calculate parent vertices
  std::cout << "Finish CalShortPathParentVertices" << std::endl;
  // initialize an empty path
  std::vector<int> path;
  std::vector<std::vector<int> > paths;  // paths in vertex id
  CalShortPaths(paths, path, parents_[src], 
                target); // pathes in vertex id
  std::cout << "Finish CalShortPaths" << std::endl;
  CalEdgePaths(paths, net.edge_paths, net.HPWL); // pathes in edges
  std::cout << "Finish CalEdgePaths" << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////
// Top level functions
void CreateGraph(std::vector<SoftMacro>& soft_macros, // placed soft macros
     std::vector<int>& soft_macro_vertex_id, // store the vertex id for each soft macro
     std::vector<Edge>& edge_list, // edge_list and vertex_list are all empty list
     std::vector<Vertex>& vertex_list)
{
  // first use the boundaries of clusters to define the center of empty spaces
  // Then use the center of empty spaces and center of clusters to define hanan grid
  std::set<float> x_bound_point;
  std::set<float> y_bound_point;
  std::set<float> x_hanan_point;
  std::set<float> y_hanan_point;
  for (const auto& soft_macro : soft_macros) {
    x_bound_point.insert( std::round(soft_macro.GetX()));
    y_bound_point.insert( std::round(soft_macro.GetY()));
    x_hanan_point.insert( std::round(soft_macro.GetX() + soft_macro.GetWidth() / 2.0));
    y_hanan_point.insert( std::round(soft_macro.GetY() + soft_macro.GetHeight() / 2.0));
    x_bound_point.insert( std::round(soft_macro.GetX() + soft_macro.GetWidth()));
    y_bound_point.insert( std::round(soft_macro.GetY() + soft_macro.GetHeight()));
  }
  auto it = x_bound_point.begin();
  while (it != x_bound_point.end()) {
    float midpoint = *it;
    it++;
    if (it != x_bound_point.end()) {
      midpoint =  std::round((midpoint + *it) / 2.0);
      x_hanan_point.insert(midpoint);
    }
  }
  it = y_bound_point.begin();
  while (it != y_bound_point.end()) {
    float midpoint = *it;
    it++;
    if (it != y_bound_point.end()) {
      midpoint =  std::round((midpoint + *it) / 2.0);
      y_hanan_point.insert(midpoint);
    }
  }
  std::vector<float> x_grid(x_hanan_point.begin(), x_hanan_point.end());
  std::vector<float> y_grid(y_hanan_point.begin(), y_hanan_point.end());
  // create vertex list based on the hanan grids in a row-based manner
  // each grid point cooresponds to a vertex
  // we assign weight to all the vertices
  // the weight of each vertex is the macro utilization of the cluster (softmacro)
  // to which it belongs to
  for (auto y : y_grid)
    for (auto x : x_grid)
      vertex_list.push_back(Vertex(vertex_list.size(), Point(x, y)));
  // initialize the macro_id and macro util for each vertex
  for (auto& vertex : vertex_list) {
    vertex.weight = 0.0; // weight is the macro util
    vertex.macro_id = -1; // macro_id 
  }
  
  std::cout <<"\n\n\n";
  std::cout << "Finish Creating vertex list" << std::endl;
   
  std::cout << "x_grid:  " << "  ";
  for (auto& x : x_grid)
    std::cout << x << "   ";
  std::cout << std::endl;
  std::cout << "y_grid:  " << "  ";
  for (auto& y : y_grid)
    std::cout << y << "   ";
  std::cout << std::endl;

  int macro_id = 0;
  for (const auto& soft_macro : soft_macros) {
    std::cout << "vertices in macro : " << soft_macro.GetName() << std::endl;
    const float lx =  std::round(soft_macro.GetX());
    const float ly =  std::round(soft_macro.GetY());
    const float ux =  std::round(soft_macro.GetX() + soft_macro.GetWidth());
    const float uy =  std::round(soft_macro.GetY() + soft_macro.GetHeight());
    const float cx =  std::round(soft_macro.GetX() + soft_macro.GetWidth() / 2.0);
    const float cy =  std::round(soft_macro.GetY() + soft_macro.GetHeight() / 2.0);
    Rect rect(lx, ly, ux, uy);
    // calculate the macro utilization of the soft macro
    float macro_util = soft_macro.GetMacroUtil();  
    // find the vertices within the soft macro
    int x_start = -1;
    int y_start = -1;
    int x_end   = -1;
    int y_end   = -1;
    GetVerticesInRect(x_grid, y_grid, rect, x_start, x_end, y_start, y_end);
    std::cout << "x_start :  " << x_start << "  ";
    std::cout << "x_end   :  " << x_end   << "  ";
    std::cout << "y_start :  " << y_start << "  ";
    std::cout << "y_end   :  " << y_end   << "  ";
    std::cout << std::endl;
    std::cout << "lx  :  " << cx << "  ux:  " << cy << std::endl;
    std::cout << "ly  :  " << cx << "  uy:  " << cy << std::endl;
    std::cout << "cx  :  " << cx << "  cy:  " << cy << std::endl;

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
        if (x_grid[x_idx] == lx || x_grid[x_idx] == ux)
          vertex_list[vertex_id].disable_v_edge = true;
        // if the grid point is on the top or bottom boundry
        if (y_grid[y_idx] == ly || y_grid[y_idx] == uy)
          vertex_list[vertex_id].disable_h_edge = true;
      }
    }
    if (test_flag == false)
      std::cout << "Error\n\n" << std::endl;
      
    // increase macro id
    macro_id++;
    if (soft_macro.GetArea() <= 0.0)
      std::cout << "macro_id : " << macro_id << "  " 
                << soft_macro_vertex_id.size() << std::endl;
  }

  std::cout << "soft_macro_vertex_id.size() : " << soft_macro_vertex_id.size() << " ";
  std::cout << "soft_macros.size() : " << soft_macros.size() << std::endl;
  std::cout << "Finish macro_id assignment" << std::endl;
  // print vertex id
  for (int i = 0; i < soft_macros.size(); i++) 
    std::cout << "macro_id : " << i << "  "
              << "vertex_id : " << soft_macro_vertex_id[i] << "  "
              << "macro_id : " << vertex_list[soft_macro_vertex_id[i]].vertex_id
              << std::endl;

  // add all the edges between grids (undirected)
  // add all the horizontal edges
  for (int y_idx = 0; y_idx < y_grid.size(); y_idx++) {
    for (int x_idx = 0; x_idx < x_grid.size() - 1; x_idx++) {
      const int src = y_idx * x_grid.size() + x_idx;
      const int target = src + 1;
      if (vertex_list[src].disable_h_edge == true ||
          vertex_list[target].disable_h_edge == true ||
          vertex_list[src].disable_v_edge == true ||
          vertex_list[target].disable_v_edge == true)
        continue;
      Edge edge(edge_list.size()); // create an edge with edge id
      edge.terminals = std::pair<int, int>(src, target);
      edge.direction = true; // true means horizontal edge
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
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetX() + soft_macros[src_macro_id].GetWidth()
                 - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                (x_grid[x_idx + 1] - soft_macros[src_macro_id].GetX()
                 - soft_macros[src_macro_id].GetWidth());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                 (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
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
      if (vertex_list[src].disable_h_edge == true ||
          vertex_list[target].disable_h_edge == true ||
          vertex_list[src].disable_v_edge == true ||
          vertex_list[target].disable_v_edge == true)
        continue;
      Edge edge(edge_list.size()); // create an edge with edge id
      edge.terminals = std::pair<int, int>(src, target);
      edge.direction = false; // false means vertical edge
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
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetY() + soft_macros[src_macro_id].GetHeight()
                 - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                (y_grid[y_idx + 1] - soft_macros[src_macro_id].GetY()
                 - soft_macros[src_macro_id].GetHeight());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                 (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        }
      }
      edge_list.push_back(edge);
    }
  }

  std::cout << "finish edge list" << std::endl;
  // handle the vertices on left or right boundaries
  for (int y_idx = 0; y_idx < y_grid.size(); y_idx++) {
    for (int x_idx = 1; x_idx < x_grid.size() - 1; x_idx++) {
      auto& vertex = vertex_list[y_idx * x_grid.size() + x_idx];
      if (vertex.disable_v_edge == false && vertex.disable_h_edge == false) {
        continue;
      } else if (vertex.disable_v_edge == true && vertex.disable_h_edge == true) {
        continue;
      } else if (vertex.disable_v_edge == true) {
        const int src = vertex.vertex_id - 1;
        const int target = vertex.vertex_id + 1;
        Edge edge(edge_list.size()); // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = true; // true means horizontal edge
        edge.length = x_grid[x_idx + 1] - x_grid[x_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length
        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetX() + soft_macros[src_macro_id].GetWidth()
                 - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                (x_grid[x_idx + 1] - soft_macros[src_macro_id].GetX()
                 - soft_macros[src_macro_id].GetWidth());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                 (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        }
        //edge.length_w = vertex_list[src].weight * 
        //    (soft_macros[target_macro_id].GetX() - x_grid[x_idx - 1]);
        //edge.length_w += vertex_list[target].weight *
        //    (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        edge_list.push_back(edge);
      }
    }
  }
  // handle the vertices on top or bottom boundaries
  for (int y_idx = 1; y_idx < y_grid.size() - 1; y_idx++) {
    for (int x_idx = 0; x_idx < x_grid.size(); x_idx++) {
      auto& vertex = vertex_list[y_idx * x_grid.size() + x_idx];
      if (vertex.disable_v_edge == false && vertex.disable_h_edge == false) {
        continue;
      } else if (vertex.disable_v_edge == true && vertex.disable_h_edge == true) {
        continue;
      } else if (vertex.disable_h_edge == true) {
        const int src = vertex.vertex_id - x_grid.size();
        const int target = vertex.vertex_id + x_grid.size();
        Edge edge(edge_list.size()); // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = false; // false means vertical edge
        edge.length = y_grid[y_idx + 1] - y_grid[y_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length
        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetY() + soft_macros[src_macro_id].GetHeight()
                 - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                (y_grid[y_idx + 1] - soft_macros[src_macro_id].GetY()
                 - soft_macros[src_macro_id].GetHeight());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                 (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        }
        //edge.length_w = vertex_list[src].weight * 
        //        (soft_macros[target_macro_id].GetY() - y_grid[y_idx - 1]);
        //edge.length_w += vertex_list[target].weight *
        //        (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        edge_list.push_back(edge);
      }
    }
  }
  
  /*
  // handle the vertex on the boundaries
  for (int y_idx = 1; y_idx < y_grid.size() - 1; y_idx++) {
    for (int x_idx = 1; x_idx < x_grid.size() - 1; x_idx++) {
      auto& vertex = vertex_list[y_idx * x_grid.size() + x_idx];
      if (vertex.disable_v_edge == false && vertex.disable_h_edge == false) {
        continue;
      } else if (vertex.disable_v_edge == true && vertex.disable_h_edge == true) {
        continue;
      } else if (vertex.disable_v_edge == true) {
        const int src = vertex.vertex_id - 1;
        const int target = vertex.vertex_id + 1;
        Edge edge(edge_list.size()); // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = true; // true means horizontal edge
        edge.length = x_grid[x_idx + 1] - x_grid[x_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length
        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetX() + soft_macros[src_macro_id].GetWidth()
                 - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                (x_grid[x_idx + 1] - soft_macros[src_macro_id].GetX()
                 - soft_macros[src_macro_id].GetWidth());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetX() - x_grid[x_idx]);
          edge.length_w += vertex_list[target].weight *
                 (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        }
        //edge.length_w = vertex_list[src].weight * 
        //    (soft_macros[target_macro_id].GetX() - x_grid[x_idx - 1]);
        //edge.length_w += vertex_list[target].weight *
        //    (x_grid[x_idx + 1] - soft_macros[target_macro_id].GetX());
        edge_list.push_back(edge);
      } else {
        const int src = vertex.vertex_id - x_grid.size();
        const int target = vertex.vertex_id + x_grid.size();
        Edge edge(edge_list.size()); // create an edge with edge id
        edge.terminals = std::pair<int, int>(src, target);
        edge.direction = false; // false means vertical edge
        edge.length = y_grid[y_idx + 1] - y_grid[y_idx - 1];
        // calculate edge type (internal or not)
        // and weighted length
        const int& src_macro_id = vertex_list[src].macro_id;
        const int& target_macro_id = vertex_list[target].macro_id;
        // this is an edge crossing boundaries
        edge.internal = false;
        if (src_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                      (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight * 
                      (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        } else if (target_macro_id == -1) {
          edge.length_w = vertex_list[src].weight * 
                (soft_macros[src_macro_id].GetY() + soft_macros[src_macro_id].GetHeight()
                 - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                (y_grid[y_idx + 1] - soft_macros[src_macro_id].GetY()
                 - soft_macros[src_macro_id].GetHeight());
        } else {
          edge.length_w = vertex_list[src].weight * 
                 (soft_macros[target_macro_id].GetY() - y_grid[y_idx]);
          edge.length_w += vertex_list[target].weight *
                 (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        }
        //edge.length_w = vertex_list[src].weight * 
        //        (soft_macros[target_macro_id].GetY() - y_grid[y_idx - 1]);
        //edge.length_w += vertex_list[target].weight *
        //        (y_grid[y_idx + 1] - soft_macros[target_macro_id].GetY());
        edge_list.push_back(edge);
      }
    }
  }
  */


  std::cout << "finish boundary edges" << std::endl;

  // handle all the IO cluster
  for (int i = 0; i < soft_macros.size(); i++) {
    if (soft_macros[i].GetArea() > 0.0)
      continue;
    auto& vertex = vertex_list[soft_macro_vertex_id[i]];
    vertex.macro_id = i; // update the macro id
    std::cout << "macro_id : " << vertex.macro_id << std::endl;
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
    std::cout << "step1" << std::endl;
    for (auto& neighbor : neighbors) {
      if (neighbor >= vertex_list.size())
        continue;
      auto& n_vertex = vertex_list[neighbor];
      if (n_vertex.disable_v_edge == true ||
          n_vertex.disable_h_edge == true)
        continue;
      Edge edge(edge_list.size()); // create an edge with edge id
      edge.terminals = std::pair<int, int>(vertex.vertex_id, n_vertex.vertex_id);
      edge.direction = true; // false means horizontal edge
      edge.length = std::abs(vertex.pos.first - n_vertex.pos.first);
      edge.internal = false;
      edge.length_w = n_vertex.weight * edge.length;
      edge_list.push_back(edge);
    }
    std::cout << "step2" << std::endl;
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
    std::cout << "step3" << std::endl;
    for (auto& neighbor : neighbors) {
      if (neighbor >= vertex_list.size())
        continue;
      auto& n_vertex = vertex_list[neighbor];
      if (n_vertex.disable_v_edge == true ||
          n_vertex.disable_h_edge == true)
        continue;
      Edge edge(edge_list.size()); // create an edge with edge id
      edge.terminals = std::pair<int, int>(vertex.vertex_id, n_vertex.vertex_id);
      edge.direction = false; // false means vertical edge
      edge.length = std::abs(vertex.pos.second - n_vertex.pos.second);
      edge.internal = false;
      edge.length_w = n_vertex.weight * edge.length;
      edge_list.push_back(edge);
    }
    std::cout << "step4" << std::endl;
  }

  std::cout << "finish io cluster related edges" << std::endl;


  // update edge weight and pin access
  for (auto& edge : edge_list)  {
    edge.weight = std::max(vertex_list[edge.terminals.first].weight, 
                           vertex_list[edge.terminals.second].weight);
    // for the edge crossing soft macros
    if (edge.internal == false) {
      if (edge.direction == true) { // horizontal
        edge.pin_access = R;
      } else {
        edge.pin_access = T;
      }
    } // update crossing edge
  } // update edge weight
 
  int num_edges = edge_list.size();
  for (int i = 0; i < num_edges; i++)
    edge_list.push_back(Edge(edge_list.size(), edge_list[i]));

  //for (auto& edge : edge_list)
  //  edge.Print();

  std::cout << "\n\n****************************************";
  std::cout << "macro_id,  macro,  vertex_id, macro_id" << std::endl;
  for (int i = 0; i < soft_macros.size(); i++)
    std::cout << "i:  " << i << "   "
              << soft_macros[i].GetName() << "    "
              << soft_macro_vertex_id[i]  << "    "
              << vertex_list[soft_macro_vertex_id[i]].macro_id << std::endl;
  std::cout << std::endl;
  std::cout << "exiting create graph" << std::endl;
}

// Calculate the paths for global buses with ILP
// congestion_weight : the cost for each edge is
// (1 - congestion_weight) * length + congestion_weight * length_w
bool CalNetPaths(std::vector<SoftMacro>& soft_macros, // placed soft macros
        std::vector<int>& soft_macro_vertex_id, // store the vertex id for each soft macro
        std::vector<Edge>& edge_list,
        std::vector<Vertex>& vertex_list,
        std::vector<BundledNet>& nets,
        // parameters
        float congestion_weight)
{
  // create vertex_list and edge_list
  CreateGraph(soft_macros, soft_macro_vertex_id, edge_list, vertex_list);
  // create graph based on vertex list and edge list
  Graph graph(vertex_list.size(), congestion_weight);
  for (auto& edge : edge_list) {
    float weight = edge.length * (1 - congestion_weight) 
                   + edge.length_w * congestion_weight; // cal edge weight
    if (weight <= 0.0) {
      std::cout << "warning ! "
                << "edge_length : " << edge.length << "  "
                << "edge.length_w : " << edge.length_w << "  "
                <<  std::endl;
    }
    graph.AddEdge(edge.terminals.first, edge.terminals.second, weight, &edge);
  }
  // Find all the shortest paths based on graph
  int num_paths = 0;
  // map each candidate path to its related net
  std::map<int, int> path_net_map; // <path_id, net_id>
  std::map<int, int> path_net_path_map; // <path_id, path_id>
  int net_id = 0;
  for (auto& net : nets) {
    // calculate candidate paths
    std::cout << "calculate the path for net " << net.terminals.first << "  "
              << "  -  " << net.terminals.second << std::endl;
    std::cout << "cluster :  " << soft_macros[net.terminals.first].GetName() << "   "
              << soft_macros[net.terminals.second].GetName() << std::endl;
    std::cout << soft_macro_vertex_id[net.terminals.first] << "   "
              << soft_macro_vertex_id[net.terminals.second] << std::endl;
    //graph.CalNetEdgePaths(soft_macro_vertex_id[net.terminals.first], 
    //       soft_macro_vertex_id[net.terminals.second], net);
    graph.CalNetEdgePaths(soft_macro_vertex_id[net.terminals.second], 
                  soft_macro_vertex_id[net.terminals.first], net);
    // update path id
    int path_id = 0;
    for (const auto& edge_path : net.edge_paths) {
      // here the edge paths only include edges crossing soft macros (IOs)
      path_net_path_map[num_paths] = path_id++;
      path_net_map[num_paths++] = net_id;
    }
    std::cout << "number candidate paths is " << net.edge_paths.size() << std::endl;
    net_id++;
  }

  std::cout << "\n\n\n" << std::endl;
  std::cout << "All the candidate paths" << std::endl;
  std::cout << "Total number of candidate paths : " << num_paths << std::endl;
  for (auto& net : nets) {
   std::cout << "*****************************************" << std::endl;
   std::cout << "src :  " << net.terminals.first << "   "
             << "target :  " << net.terminals.second << std::endl;
   for (auto& edge_path : net.edge_paths) {
    std::cout <<  "path :  ";
    for (auto& edge_id : edge_path)
      std::cout << edge_id << "   ";
    std::cout << std::endl;
   }
   std::cout << std::endl;
   std::cout << "\n\n";
  }


#ifdef USE_CPLEX
  // CPLEX Implementation (removed on 20221013 by Zhiang)
  /*
  // Cal ILP Solver to solve the ILP problem
  IloEnv myenv;   // environment object
  IloModel mymodel(myenv);  // model object
  // For each path, define a variable x
  IloNumVarArray x(myenv, num_paths, 0, 1, ILOINT);
  // For each edge, define a variable y
  IloNumVarArray y(myenv, edge_list.size(), 0, 1, ILOINT);
  // add constraints
  int x_id = 0;
  int num_constraints = 0;  // equal to number of nets
  for (auto& net : nets) {
    IloExpr expr(myenv); // empty expression
    std::cout << "old num_constraints : " << num_constraints << std::endl;
    for (auto& edge_path : net.edge_paths) {
      for (auto& edge_id : edge_path) {
        mymodel.add(x[x_id] <= y[edge_id]);
        num_constraints++;
      }
      expr += x[x_id++];
    }
    // need take a detail look [fix]
    if (net.edge_paths.size() == 0)
     continue;
    std::cout << "new num_constraints : " << num_constraints << std::endl;
    mymodel.add(expr == 1);
    expr.end(); // clear memory
    num_constraints++;
  }
  
  IloExpr obj_expr(myenv);   // empty expression
  for (int i = 0; i < edge_list.size(); i++)
    obj_expr += edge_list[i].weight * y[i]; // reduce the edges cross soft macros
  mymodel.add(IloMinimize(myenv, obj_expr));  // adding minimization objective
  obj_expr.end();  // clear memory
  // Model Solution
  IloCplex mycplex(myenv);
  mycplex.extract(mymodel);
  // solves model and stores whether or 
  // not it is feasible in an IloBool variable called "feasible
  IloBool feasible = mycplex.solve();
  // solves model and stores whether or 
  // not it is feasible in an IloBool variable called "feasible
  if (feasible != IloTrue) 
    return false; // Something wrong, no feasible solution
  std::cout << "\n\n Total number of paths : " << num_paths << std::endl;
  */

#endif // USE_CPLEX

  // Google OR-TOOLS for SCIP Implementation
  // create the ILP solver with the SCIP backend
  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));
  if (!solver) {
    std::cout << "Error ! SCIP solver unavailable!" << std::endl;
    return false;
  }

  // For each path, define a variable x
  std::vector<const MPVariable*> x(num_paths);
  for (int i = 0; i < num_paths; i++)
    x[i] = solver->MakeIntVar(0.0, 1.0, "");
  
  // For each edge, define a variable y
  std::vector<const MPVariable*> y(edge_list.size());
  for (int i = 0; i < edge_list.size(); i++)
    y[i] = solver->MakeIntVar(0.0, 1.0, "");
  std::cout  << "Number of variables = " << solver->NumVariables() << std::endl;
  const double infinity = solver->infinity();

  // add constraints
  int x_id = 0;
  for (auto& net : nets) {
    // need take a detail look [fix]
    if (net.edge_paths.size() == 0)
      continue;
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
  std::cout << "Number of constraints = " << solver->NumConstraints() << std::endl;
  
  // Create the objective function
  MPObjective* const objective = solver->MutableObjective();
  for (int i = 0; i < edge_list.size(); i++)
    objective->SetCoefficient(y[i], edge_list[i].weight);
  
  objective->SetMinimization();
  const MPSolver::ResultStatus result_status = solver->Solve();
  // Check that the problem has an optimal solution.
  if (result_status != MPSolver::OPTIMAL)
    return false;  // The problem does not have an optimal solution;
  std::cout << "Soluton : " << std::endl;
  std::cout << "Optimal objective value = " << objective->Value() << std::endl;
  
  // Generate the solution and check which edge get selected
  for (int i = 0; i < num_paths; i++) {
    //if (mycplex.getValue(x[i]) == 0) for cplex
    //  continue; 
    if (x[i]->solution_value() == 0)
      continue;

    std::cout << "working on path " << i << std::endl;
    auto target_cluster= soft_macros[nets[path_net_map[i]].terminals.second].GetCluster();
    PinAccess src_pin = NONE;
    Cluster* pre_cluster = nullptr;
    int last_edge_id = -1;
    const float net_weight = nets[path_net_map[i]].weight;
    const int src_cluster_id = nets[path_net_map[i]].src_cluster_id;
    const int target_cluster_id = nets[path_net_map[i]].target_cluster_id;
    std::cout << "src_cluster_id : " << src_cluster_id << "  "
              << "target_cluster_id : " << target_cluster_id << "  "
              << std::endl;
    std::cout << "src_macro_id : " << nets[path_net_map[i]].terminals.first << "  "
              << "target_macro_id : " << nets[path_net_map[i]].terminals.second << std::endl;
    //std::cout << "src_cluster_name : " 
    //  << soft_macros[nets[path_net_map[i]].terminals.first].GetCluster()->GetName()
    //  << std::endl;
    //std::cout << "target_cluster_name : " 
    //  << soft_macros[nets[path_net_map[i]].terminals.second].GetCluster()->GetName()
    //  << std::endl;    
    for (auto& edge_id : nets[path_net_map[i]].edge_paths[path_net_path_map[i]]) {
      auto& edge = edge_list[edge_id];
      std::cout << "edge_terminals : " << edge.terminals.first << "  "
                << edge.terminals.second << std::endl;
      std::cout << "edge_terminals_macro_id : " << vertex_list[edge.terminals.first].macro_id << "  "
                << vertex_list[edge.terminals.second].macro_id << std::endl;
      last_edge_id = edge_id;
      Cluster* start_cluster = nullptr;
      if (vertex_list[edge.terminals.first].macro_id != -1) {
        start_cluster = soft_macros[vertex_list[edge.terminals.first].macro_id].GetCluster();
        std::cout << "start_name : " 
                  << soft_macros[vertex_list[edge.terminals.first].macro_id].GetName()
                  << std::endl;
      }
      if (start_cluster != nullptr)
        std::cout << "start_cluster_id : " << start_cluster->GetId() << std::endl;
      Cluster* end_cluster = nullptr;
      if (vertex_list[edge.terminals.second].macro_id != -1) {
        end_cluster = soft_macros[vertex_list[edge.terminals.second].macro_id].GetCluster();
        std::cout << "end_name : " 
                  << soft_macros[vertex_list[edge.terminals.second].macro_id].GetName()
                  << std::endl;
      }
      if (end_cluster != nullptr)
        std::cout << "end_cluster_id : " << end_cluster->GetId() << std::endl;
     
      if (start_cluster == nullptr && end_cluster == nullptr) {
        std::cout << "(1) This should not happen" << std::endl;
      } else if (start_cluster != nullptr && end_cluster != nullptr) {
        if (start_cluster->GetId() == src_cluster_id) {
          start_cluster->SetPinAccess(target_cluster_id, edge.pin_access, net_weight);
          src_pin = Opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else if (end_cluster->GetId() == src_cluster_id) {
          end_cluster->SetPinAccess(target_cluster_id, Opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else {
          if (start_cluster != pre_cluster && end_cluster != pre_cluster) {
            std::cout << "(2) error ! This should not happen" << std::endl;
          } else if (start_cluster == pre_cluster) {
            start_cluster->AddBoundaryConnection(src_pin, edge.pin_access, net_weight);
            src_pin = Opposite(edge.pin_access);
            pre_cluster = end_cluster;
          } else {
            end_cluster->AddBoundaryConnection(src_pin, Opposite(edge.pin_access), net_weight);
            src_pin = edge.pin_access;
            pre_cluster = start_cluster;
          }
        }
      } else if (start_cluster != nullptr) {
        if (start_cluster->GetId() == src_cluster_id) {
          start_cluster->SetPinAccess(target_cluster_id, edge.pin_access, net_weight);
          src_pin = Opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else if (start_cluster != pre_cluster) {
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else {
          start_cluster->AddBoundaryConnection(src_pin, edge.pin_access, net_weight);
          src_pin = Opposite(edge.pin_access);
          pre_cluster = end_cluster;
        }
      } else {
        if (end_cluster->GetId() == src_cluster_id) {
          end_cluster->SetPinAccess(target_cluster_id, Opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster;
        } else if (end_cluster != pre_cluster) {
          src_pin = Opposite(edge.pin_access);
          pre_cluster = end_cluster;
        } else {
          end_cluster->AddBoundaryConnection(src_pin, Opposite(edge.pin_access), net_weight);
          src_pin = edge.pin_access;
          pre_cluster = start_cluster; 
        }
      }
    }
    if (target_cluster != nullptr && last_edge_id >= 0) {
      auto& edge = edge_list[last_edge_id];
      Cluster* start_cluster = nullptr;
      if (vertex_list[edge.terminals.first].macro_id != -1)
        start_cluster = soft_macros[vertex_list[edge.terminals.first].macro_id].GetCluster();
      if (start_cluster != nullptr)
        std::cout << "start_cluster_id : " << start_cluster->GetId() << std::endl;
      Cluster* end_cluster = nullptr;
      if (vertex_list[edge.terminals.second].macro_id != -1)
        end_cluster = soft_macros[vertex_list[edge.terminals.second].macro_id].GetCluster();
      if (end_cluster != nullptr)
        std::cout << "end_cluster_id : " << end_cluster->GetId() << std::endl;
      if (start_cluster == target_cluster) 
        target_cluster->SetPinAccess(src_cluster_id, edge.pin_access, net_weight);
      else if (end_cluster == target_cluster)
        target_cluster->SetPinAccess(src_cluster_id, Opposite(edge.pin_access), net_weight);
      else
        std::cout << "(3) Error ! This should not happen" << std::endl;
    }
    std::cout << "finish path " << i << std::endl;
  }
  
  // closing the model
  //mycplex.clear();
  //myenv.end();
  return true;
}


///////////////////////////////////////////////////////////////////////
// Utility Functions

// Get vertices within a given rectangle
// Calculate the start index and end index in the grid
void GetVerticesInRect(const std::vector<float>& x_grid,
                       const std::vector<float>& y_grid,
                       const Rect& rect,
                       int& x_start, int& x_end, int& y_start, int& y_end) 
{
  GetVerticesInSegment(x_grid, rect.xMin(), rect.xMax(), x_start, x_end);
  GetVerticesInSegment(y_grid, rect.yMin(), rect.yMax(), y_start, y_end);
}

// Get vertices in a given segement
// We consider start terminal and end terminal
void GetVerticesInSegment(const std::vector<float>& grid,
                          const float start_point,
                          const float end_point,
                          int& start_idx, int& end_idx)
{
  start_idx = 0;
  end_idx   = 0;
  if (grid.size() == 0 || start_point > end_point)
    return;
  // calculate start_idx
  while (start_idx < grid.size() &&
         grid[start_idx] < start_point)
    start_idx++;
  // calculate end_idx
  while (end_idx < grid.size() &&
         grid[end_idx] <= end_point)
    end_idx++;
}

}


