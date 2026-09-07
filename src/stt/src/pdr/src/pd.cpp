// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "stt/pd.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <limits>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/heap/d_ary_heap.hpp"
#include "lemon/core.h"
#include "lemon/list_graph.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace pdr {

using lemon::INVALID;
using lemon::ListGraph;
using odb::Point;
using std::vector;
using stt::Tree;
using utl::Logger;

/////////// Nearest Neighbors

// This is the method in "Prim-Dijkstra Revisited" section 4.
// The key idea is: "We say that vi is a neighbor of vj if the smallest
// bounding box containing vi and vj contains no other nodes."

// Below implements the double monotone chains algorithm described in
// "Provably Optimal Planar Pareto Nearest Neighbor Search with Double
// Monotone Chains" (Guo et al, DATE'26).
// This is output-optimal, 39x faster than brute force and 1.3x faster
// than the lossy Guibas-Stolfi algorithm.

using Neighbors = std::vector<int>;
static vector<Neighbors> get_nearest_neighbors(const vector<Point>& pts)
{
  thread_local static vector<int> data;
  data.clear();

  const size_t pt_count = pts.size();

  vector<Neighbors> neighbors(pt_count);

  data.resize(pt_count * 6);
  std::fill_n(data.begin(), pt_count * 4, -1);
  int *yprev_w_smallx = &data[0];
  int *ynext_w_smallx = &data[pt_count];
  int *curx_yprev_w_largex = &data[pt_count * 2];
  int *curx_ynext_w_largex = &data[pt_count * 3];
  int *sorted_x = &data[pt_count * 4];
  int *sorted_y = &data[pt_count * 5];

  std::iota(sorted_x, sorted_x + pt_count, 0);
  std::iota(sorted_y, sorted_y + pt_count, 0);
  std::sort(sorted_x, sorted_x + pt_count, [&pts] (int i, int j) {
    return pts[i].getX() < pts[j].getX();
  });
  std::sort(sorted_y, sorted_y + pt_count, [&pts] (int i, int j) {
    return pts[i].getY() < pts[j].getY();
  });

  // x left to right
  // first compute "the previous/next (order by y) i that has smaller x"
  for (int syi = 1; syi < pt_count; ++syi) {
    int i = sorted_y[syi];
    int xi = pts[i].getX();
    int j = sorted_y[syi - 1];
    while ((~j) && pts[j].getX() > xi) {
      j = yprev_w_smallx[j];
    }
    yprev_w_smallx[i] = j;
  }
  for (int syi = pt_count - 2; syi >= 0; --syi) {
    int i = sorted_y[syi];
    int xi = pts[i].getX();
    int j = sorted_y[syi + 1];
    while ((~j) && pts[j].getX() > xi) {
      j = ynext_w_smallx[j];
    }
    ynext_w_smallx[i] = j;
  }
  // next, add nodes one by one with increasing x.
  // for currently encountered nodes, maintain next/prev larger x.
  for (int sxi = 0; sxi < pt_count; ++sxi) {
    int i = sorted_x[sxi], j;
    // upper left
    j = ynext_w_smallx[i];
    while (~j) {
      neighbors[j].push_back(i);
      neighbors[i].push_back(j);
      curx_yprev_w_largex[j] = i;
      j = curx_ynext_w_largex[j];
    }
    // lower left
    j = yprev_w_smallx[i];
    while (~j) {
      neighbors[j].push_back(i);
      neighbors[i].push_back(j);
      curx_ynext_w_largex[j] = i;
      j = curx_yprev_w_largex[j];
    }
  }

  return neighbors;
}

/////////// Minimum Spanning Tree per PD costing

// A potential edge in the minimum spanning tree (MST).  These are stored
// in the heap during the PD search.
struct SearchEdge
{
  float weight;            // the heap key holding the PD cost of this edge
  int path_length;         // distance from the driver to node
  ListGraph::Node parent;  // may be updated during the search
  const ListGraph::Node node;
};

// Boost provides a max heap but we need a min heap so this reverses
// the comparison to achieve that.
struct CmpEdge
{
  bool operator()(const SearchEdge& lhs, const SearchEdge& rhs) const
  {
    return std::tie(lhs.weight, lhs.parent, lhs.node)
           > std::tie(rhs.weight, rhs.parent, rhs.node);
  }
};

static void buildSpanningTree(const ListGraph::NodeMap<Point>& node_point,
                              const ListGraph::Node& driver_node,
                              const float alpha,
                              const vector<Neighbors>& nn,
                              ListGraph& graph)
{
  using Heap = boost::heap::d_ary_heap<SearchEdge,
                                       boost::heap::arity<2>,
                                       boost::heap::mutable_<true>,
                                       boost::heap::compare<CmpEdge>>;

  const int num_nodes = graph.maxNodeId() + 1;
  int num_visited = 0;
  ListGraph::NodeMap<bool> visited(graph);

  // Handles allow you to reference a heap entry for the node even
  // as the heap is updated.
  vector<Heap::handle_type> handles(num_nodes);

  Heap heap;
  handles[ListGraph::id(driver_node)] = heap.push({0, 0, INVALID, driver_node});

  while (!heap.empty()) {
    const SearchEdge edge = heap.top();
    heap.pop();

    visited[edge.node] = true;
    ++num_visited;
    if (edge.parent != INVALID) {  // skip root node
      graph.addEdge(edge.parent, edge.node);
    }

    if (num_visited == num_nodes) {
      break;  // all nodes are in the graph so we are done
    }

    // Add the neareset neighbors to the heap
    for (const int neighbor : nn[ListGraph::id(edge.node)]) {
      auto neighbor_node = ListGraph::nodeFromId(neighbor);
      if (visited[neighbor_node]) {
        continue;  // already in the graph
      }

      const int edge_length = Point::manhattanDistance(
          node_point[neighbor_node], node_point[edge.node]);
      const int neighbor_path_length = edge_length + edge.path_length;
      const float neighbor_weight = edge_length + alpha * edge.path_length;

      auto& handle = handles[neighbor];
      if (handle == Heap::handle_type()) {  // not in the heap
        handles[neighbor] = heap.push(
            {neighbor_weight, neighbor_path_length, edge.node, neighbor_node});
      } else if (neighbor_weight <= (*handle).weight) {
        (*handle).weight = neighbor_weight;
        (*handle).path_length = neighbor_path_length;
        (*handle).parent = edge.node;
        heap.increase(handle);
      }
    }
  }
}

/////////// Steinerize the Spanning Tree

// A possible Steiner node
struct CandidateSteiner
{
  int gain;  // wire length improvement by adding this node
  ListGraph::Node node;
  ListGraph::Edge edge1;
  ListGraph::Edge edge2;
  Point steiner_point;
};

struct CmpCandidate
{
  bool operator()(const CandidateSteiner& lhs,
                  const CandidateSteiner& rhs) const
  {
    // Gain is key, the steiner_point is just an arbitrary tie breaker.
    return std::make_tuple(
               lhs.gain, lhs.steiner_point.getX(), lhs.steiner_point.getY())
           < std::make_tuple(
               rhs.gain, rhs.steiner_point.getX(), rhs.steiner_point.getY());
  }
};

// Compute the best pair of edges connected to a node that can be
// improved by adding a Steiner node.
static CandidateSteiner best_steiner_for_node(
    const ListGraph& graph,
    const ListGraph::Node& node,
    const ListGraph::NodeMap<Point>& node_point)
{
  const Point pt_node = node_point[node];

  CandidateSteiner best;
  best.gain = 0;

  // Loop through all edge pairs.  N^2 but N is small.
  for (ListGraph::IncEdgeIt edge1(graph, node); edge1 != INVALID; ++edge1) {
    auto n1 = graph.runningNode(edge1);
    const Point pt1 = node_point[n1];
    ListGraph::IncEdgeIt edge2(edge1);
    for (++edge2; edge2 != INVALID; ++edge2) {
      auto n2 = graph.runningNode(edge2);
      const Point pt2 = node_point[n2];

      Point pt_steiner = pt_node;
      if (std::min(pt1.getX(), pt2.getX()) > pt_node.getX()) {
        // both right of node
        pt_steiner.setX(std::min(pt1.getX(), pt2.getX()));
      } else if (std::max(pt1.getX(), pt2.getX()) < pt_node.getX()) {
        // both left of node
        pt_steiner.setX(std::max(pt1.getX(), pt2.getX()));
      }

      if (std::min(pt1.getY(), pt2.getY()) > pt_node.getY()) {
        // both above node
        pt_steiner.setY(std::min(pt1.getY(), pt2.getY()));
      } else if (std::max(pt1.getY(), pt2.getY()) < pt_node.getY()) {
        // both below node
        pt_steiner.setY(std::max(pt1.getY(), pt2.getY()));
      }

      const int gain = Point::manhattanDistance(pt_steiner, pt_node);
      if (gain > best.gain) {
        best = {gain, node, edge1, edge2, pt_steiner};
      }
    }
  }
  return best;
}

// Steinerize by looking at all adjacent edge pairs and finding the one
// with maximum improvement by adding a Steiner point.  Repeat until
// no further improvement can be found.  This idea comes from footnote
// 1 in "Prim-Dijkstra tradeoffs for improved performance-driven
// routing tree design".
static void steinerize(ListGraph& graph, ListGraph::NodeMap<Point>& node_point)
{
  using Heap = boost::heap::d_ary_heap<CandidateSteiner,
                                       boost::heap::arity<2>,
                                       boost::heap::mutable_<true>,
                                       boost::heap::compare<CmpCandidate>>;
  Heap heap;
  std::map<ListGraph::Node, Heap::handle_type> handles;

  // Setup the heap with all the candidates
  for (ListGraph::NodeIt node(graph); node != INVALID; ++node) {
    handles[node] = heap.push(best_steiner_for_node(graph, node, node_point));
  }

  while (!heap.empty()) {
    const CandidateSteiner best = heap.top();
    heap.pop();

    if (best.gain == 0) {
      break;
    }

    auto opp1 = graph.oppositeNode(best.node, best.edge1);
    auto opp2 = graph.oppositeNode(best.node, best.edge2);

    bool new_node = false;
    ListGraph::Node steiner_node;
    if (best.steiner_point == node_point[opp1]) {
      steiner_node = opp1;
    } else if (best.steiner_point == node_point[opp2]) {
      steiner_node = opp2;
    } else {
      steiner_node = graph.addNode();
      node_point[steiner_node] = best.steiner_point;
      new_node = true;
    }

    if (steiner_node != opp1) {
      if (graph.u(best.edge1) == best.node) {
        graph.changeU(best.edge1, steiner_node);
      } else {
        graph.changeV(best.edge1, steiner_node);
      }
    }

    if (steiner_node != opp2) {
      if (graph.u(best.edge2) == best.node) {
        graph.changeU(best.edge2, steiner_node);
      } else {
        graph.changeV(best.edge2, steiner_node);
      }
    }

    if (new_node) {
      // Add to the heap
      graph.addEdge(steiner_node, best.node);
      handles[steiner_node]
          = heap.push(best_steiner_for_node(graph, steiner_node, node_point));
    }

    // Find the new best candidate for the updated nodes
    handles[best.node]
        = heap.push(best_steiner_for_node(graph, best.node, node_point));

    for (const auto& node : {opp1, opp2}) {
      *handles[node] = best_steiner_for_node(graph, node, node_point);
      heap.update(handles[node]);
    }
  }
}

/////////// Convert to Flute-style Tree

// Flute only returns nodes with maximum degee of three.  grt assumes
// this property so we enfoce it here.  Any degree four node is split
// into two nodes with a zero length edge between them.  A very high
// degree node could be split more than once.
static void splitDegree4Nodes(ListGraph& graph,
                              ListGraph::NodeMap<Point>& node_point)
{
  std::deque<ListGraph::Node> to_process;
  for (ListGraph::NodeIt node(graph); node != INVALID; ++node) {
    to_process.push_back(node);
  }

  while (!to_process.empty()) {
    ListGraph::Node node = to_process.front();
    to_process.pop_front();
    int edge_cnt = 0;
    for (ListGraph::IncEdgeIt edge(graph, node); edge != INVALID; ++edge) {
      ++edge_cnt;
    }
    if (edge_cnt <= 3) {
      continue;
    }

    auto new_node = graph.addNode();
    node_point[new_node] = node_point[node];

    edge_cnt = 0;
    ListGraph::IncEdgeIt edge(graph, node);
    while (edge != INVALID) {
      if (++edge_cnt >= 3) {
        auto working_edge = edge;
        ++edge;
        if (graph.u(working_edge) == node) {
          graph.changeU(working_edge, new_node);
        } else {
          graph.changeV(working_edge, new_node);
        }
      } else {
        ++edge;
      }
    }
    graph.addEdge(node, new_node);
    to_process.push_back(new_node);
  }
}

static void makeTreeRecursive(const ListGraph& graph,
                              const ListGraph::Node& node,
                              const ListGraph::Node& parent,
                              const ListGraph::NodeMap<Point>& node_point,
                              Tree& tree)
{
  const int parent_id = ListGraph::id(parent);
  const int parent_x = node_point[parent].getX();
  const int parent_y = node_point[parent].getY();

  const int n = ListGraph::id(node);
  const int x = node_point[node].getX();
  const int y = node_point[node].getY();
  tree.branch[n] = {x, y, parent_id};
  tree.length += std::abs(x - parent_x) + std::abs(y - parent_y);

  for (ListGraph::IncEdgeIt edge(graph, node); edge != INVALID; ++edge) {
    auto child_node = graph.runningNode(edge);
    if (child_node != parent) {
      makeTreeRecursive(graph, child_node, node, node_point, tree);
    }
  }
}

static Tree makeTree(ListGraph& graph,
                     const int num_terminals,
                     const ListGraph::Node& driver_node,
                     ListGraph::NodeMap<Point>& node_point)
{
  Tree tree;
  tree.deg = num_terminals;
  tree.length = 0;
  // Flute-style needs a self edge on the driver node
  tree.branch.resize(graph.maxNodeId() + 1);

  makeTreeRecursive(graph, driver_node, driver_node, node_point, tree);

  return tree;
}

/////////// Entry Point

Tree primDijkstra(const vector<int>& x,
                  const vector<int>& y,
                  const int driver_index,
                  const float alpha,
                  Logger* logger)
{
  if (x.size() != y.size()) {
    logger->error(
        utl::STT, 8, "x size ({}) != y size ({})", x.size(), y.size());
  }

  if (x.empty()) {
    logger->error(utl::STT, 9, "Invalid request for an empty Steiner tree.");
  }

  ListGraph graph;
  ListGraph::NodeMap<Point> node_point(graph);  // Node -> location

  // convert x/y to points
  const int num_terminals = x.size();
  vector<Point> pts(num_terminals);
  for (int i = 0; i < num_terminals; ++i) {
    pts[i] = {x[i], y[i]};
    node_point[graph.addNode()] = pts[i];
  }

  const auto nn = get_nearest_neighbors(pts);

  auto driver_node = ListGraph::nodeFromId(driver_index);
  buildSpanningTree(node_point, driver_node, alpha, nn, graph);

  steinerize(graph, node_point);

  splitDegree4Nodes(graph, node_point);

  return makeTree(graph, num_terminals, driver_node, node_point);
}

}  // namespace pdr
