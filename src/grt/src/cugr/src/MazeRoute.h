#pragma once
#include "CUGR.h"
#include "GRNet.h"
#include "GridGraph.h"
#include "PatternRoute.h"

namespace grt {

struct SparseGrid
{
  PointT<int> interval;
  PointT<int> offset;

  SparseGrid(int x_interval, int y_interval, int x_offset, int y_offset)
      : interval(x_interval, y_interval), offset(x_offset, y_offset)
  {
  }

  void step()
  {
    offset.x = (offset.x + 1) % interval.x;
    offset.y = (offset.y + 1) % interval.y;
  }

  void reset(int x_interval, int y_interval)
  {
    interval.x = x_interval;
    interval.y = y_interval;
    step();
  }
};

class SparseGraph
{
 public:
  SparseGraph(GRNet* net, const GridGraph* graph)
      : net_(net), grid_graph_(graph)
  {
  }

  void init(GridGraphView<CostT>& wire_cost_view, SparseGrid& grid);
  int getNumVertices() const { return vertices_.size(); }
  int getNumPseudoPins() const { return pseudo_pins_.size(); }
  std::pair<PointT<int>, IntervalT<int>> getPseudoPin(int pin_index) const
  {
    return pseudo_pins_[pin_index];
  }

  int getPinVertex(const int pin_index) const { return pin_vertex_[pin_index]; }
  int getVertexPin(const int vertex) const
  {
    auto it = vertex_pin_.find(vertex);
    return it == vertex_pin_.end() ? -1 : it->second;
  }

  int getNextVertex(const int vertex, const int edge_index) const
  {
    return edges_[vertex][edge_index];
  }

  CostT getEdgeCost(const int vertex, const int edge_index) const
  {
    return costs_[vertex][edge_index];
  }

  GRPoint getPoint(const int vertex) const { return vertices_[vertex]; }

 private:
  inline int getVertexIndex(int direction, int xi, int yi) const
  {
    return direction * xs_.size() * ys_.size() + yi * xs_.size() + xi;
  }

  GRNet* net_;
  const GridGraph* grid_graph_;

  std::vector<std::pair<PointT<int>, IntervalT<int>>> pseudo_pins_;

  std::vector<int> xs_;
  std::vector<int> ys_;

  std::vector<GRPoint> vertices_;
  std::vector<std::array<int, 3>> edges_;
  std::vector<std::array<CostT, 3>> costs_;
  // std::unordered_map<int, std::vector<int>> vertexPins;
  // std::vector<std::vector<int>> pinVertices;
  std::unordered_map<int, int> vertex_pin_;
  std::vector<int> pin_vertex_;
};

struct Solution
{
  CostT cost;
  int vertex;
  std::shared_ptr<Solution> prev;

  Solution(CostT c, int v, const std::shared_ptr<Solution>& p)
      : cost(c), vertex(v), prev(p)
  {
  }
};

class MazeRoute
{
 public:
  MazeRoute(GRNet* net, const GridGraph* graph)
      : net_(net), grid_graph_(graph), graph_(net, graph)
  {
  }

  void run();
  void constructSparsifiedGraph(GridGraphView<CostT>& wire_cost_view,
                                SparseGrid& grid)
  {
    graph_.init(wire_cost_view, grid);
  }
  std::shared_ptr<SteinerTreeNode> getSteinerTree() const;

 private:
  GRNet* net_;
  const GridGraph* grid_graph_;
  SparseGraph graph_;

  std::vector<std::shared_ptr<Solution>> solutions_;
};

}  // namespace grt
