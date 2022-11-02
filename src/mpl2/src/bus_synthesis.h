// ************************************************
// Route important buses based on current layout
// ************************************************
#ifndef BUS_SYNTHESIS_H__
#define BUS_SYNTHESIS_H__

#include <map>
#include <vector>

#include "object.h"

namespace mpl {

typedef std::pair<float, float> Point;

// Each point in the hanan grid is represented by a vertex (with no size)
// And each bundled IO pin is represented by a vertex
struct Vertex
{
  int vertex_id = -1;  // vertex_id of current vertex
  int macro_id
      = -1;  // soft_macro_id of the SoftMacro which the vertex belongs to
  bool disable_v_edge
      = false;  // disable the vertical connection of this vertex
  bool disable_h_edge
      = false;  // disable the horizontal connection of this vertex
  Point pos;    // position of the vertex
  float weight
      = 0.0;  // the weight of the vertex : macro utilization of the SoftMacro
              // which the vertex belongs to.
              // For bundled IO pin, we set the macro utilization to be zero
  Vertex() {}
  Vertex(int vertex_id, Point pos) : vertex_id(vertex_id), pos(pos) {}
};

struct VertexDist
{
  int vertex;  // vertex_id of current vertex
  float dist;  // shortest distance (currently) between vertex and root vertex
  VertexDist(){};
  VertexDist(int vertex, float dist) : vertex(vertex), dist(dist) {}
};

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

// The connection between two vertices is represented by an edge
struct Edge
{
  int edge_id;                    // edge id of current edge
  std::pair<int, int> terminals;  // the vertex_id of two terminal vertices
  bool direction;                 // True for horizontal and False for vertical
  bool internal = true;  // True for edge within one SoftMacro otherwise false
  PinAccess pin_access
      = NONE;            // pin_access for internal == false (for src vertex)
  float length = 0.0;    // the length of edge
  float length_w = 0.0;  // weighted length : weight * length
  float weight = 0.0;    // the largeest macro utilization of terminals
  float num_nets = 0.0;  // num_nets passing through this edge
  Edge() {}
  Edge(int edge_id) : edge_id(edge_id) {}
  // create a reverse edge by changing the order of terminals
  Edge(int edge_id, Edge& edge)
  {
    this->edge_id = edge_id;
    this->terminals
        = std::pair<int, int>(edge.terminals.second, edge.terminals.first);
    this->internal = edge.internal;
    this->pin_access = Opposite(edge.pin_access);
    this->length = edge.length;
    this->length_w = edge.length_w;
    this->weight = edge.weight;
    this->num_nets = edge.num_nets;
  }

  void Print()
  {
    std::cout << "***********************************" << std::endl;
    std::cout << "edge_id = " << edge_id << std::endl;
    std::cout << "terminals =   " << terminals.first << "  " << terminals.second
              << std::endl;
    std::cout << "direction =   " << direction << std::endl;
    std::cout << "pin_access =  " << to_string(pin_access) << std::endl;
  }
};

// We use Arrow object in the adjacency matrix to represent the grid graph
struct Arrow
{
  int dest;                  // src -> dest (destination)
  float weight;              // weight must be nonnegative (or cost)
  Edge* edge_ptr = nullptr;  // the pointer of corresponding edge
  Arrow(){};
  Arrow(int dest, float weight, Edge* edge_ptr)
      : dest(dest), weight(weight), edge_ptr(edge_ptr)
  {
  }
};

// Grid graph for the clustered netlist
// Note that the graph is a connected undirected graph
class Graph
{
 public:
  Graph() {}
  Graph(int num_vertices, float congestion_weight);
  void AddEdge(int src, int dest, float weight, Edge* edge_ptr);
  // Calculate shortest pathes in terms of boundary edges
  void CalNetEdgePaths(int src, int target, BundledNet& net);
  bool IsConnected() const;  // check the GFS is connected
 private:
  std::vector<std::vector<Arrow>> adj_;  // adjacency matrix
  int max_num_path_
      = 10;  // limit the maximum number of candidate paths to reduce runtime
  float congestion_weight_ = 1.0;
  // store the parent vertices for each vertex in the shortest paths
  // for example, there are two paths from root to dest
  // path1: root -> A -> dest
  // path2: root -> B -> dest
  // then dest vertex has two parents:  A and B
  // So the parent of each vertex is a vector instead of some vertex
  std::map<int, std::vector<std::vector<int>>> parents_;
  // Find the shortest paths relative to root vertex based on priority queue
  // We store the paths in the format of parent vertices
  // If we want to get real pathes, we need to traverse back the parent vertices
  void CalShortPathParentVertices(int root);
  // Find real paths between root vertex and target vertex
  // by traversing back the parent vertices in a recursive manner
  // Similar to DFS (not exactly DFS)
  void CalShortPaths(
      // all paths between root vertex and target vertex
      std::vector<std::vector<int>>& paths,
      // current path between root vertex and target vertex
      std::vector<int>& path,
      // vector of parent vertices for root vertex
      std::vector<std::vector<int>>& parent_vertices,
      // current parent vertex
      int parent);
  // Calculate shortest edge paths
  void CalEdgePaths(
      // shortest paths, path = { vertex_id }
      std::vector<std::vector<int>>& paths,
      // shortest boundary edge paths
      std::vector<std::vector<int>>& edge_paths,
      // length of shortest paths
      float& HPWL);
};

// Get vertices in a given segement
// We consider start terminal and end terminal
void GetVerticesInSegment(const std::vector<float>& grid,
                          const float start_point,
                          const float end_point,
                          int& start_idx,
                          int& end_idx);

// Get vertices within a given rectangle
// Calculate the start index and end index in the grid
void GetVerticesInRect(const std::vector<float>& x_grid,
                       const std::vector<float>& y_grid,
                       const Rect& rect,
                       int& x_start,
                       int& x_end,
                       int& y_start,
                       int& y_end);

void CreateGraph(std::vector<SoftMacro>& soft_macros,     // placed soft macros
                 std::vector<int>& soft_macro_vertex_id,  // store the vertex id
                                                          // for each soft macro
                 std::vector<Edge>&
                     edge_list,  // edge_list and vertex_list are all empty list
                 std::vector<Vertex>& vertex_list);

// Calculate the paths for global buses with ILP
// congestion_weight : the cost for each edge is
// (1 - congestion_weight) * length + congestion_weight * length_w
bool CalNetPaths(std::vector<SoftMacro>& soft_macros,     // placed soft macros
                 std::vector<int>& soft_macro_vertex_id,  // store the vertex id
                                                          // for each soft macro
                 std::vector<Edge>& edge_list,
                 std::vector<Vertex>& vertex_list,
                 std::vector<BundledNet>& nets,
                 // parameters
                 float congestion_weight);

}  // namespace mpl

#endif  // BUS_SYNTHESIS_H__
