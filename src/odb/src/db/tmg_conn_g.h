// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <deque>
#include <vector>

namespace odb {

class tmg_conn;
struct Short;

class ConnectionGraph
{
 public:
  struct Edge
  {
    int from{-1};
    int to{-1};
    Edge* next{nullptr};

    // The same edge in the other direction. Every connection between
    // two points is stored twice in the graph, once per direction, so
    // a walk can leave a point along any of its edges.
    Edge* reverse{nullptr};

    // An edge is either a path or a short:
    // If wire_short is nullptr and wire_section_index is != -1,
    // the edge is a path. The opposite for a short.
    Short* wire_short{nullptr};
    int wire_section_index{-1};

    // Note that when an edge is marked as visited, it's reverse version is
    // also marked as visited.
    bool visited{false};

    // When an edge is removed from the graph, the latter is not rebuilt.
    // Instead, that edge is marked as "skip" and the walkers will ignore
    // it when traversing.
    bool skip{false};
  };

  struct Point
  {
    int path_index{-1};

    // Head of the chain of edges that leave this point.
    Edge* first_edge{nullptr};

    // Visit state.
    // 0:     This point was not yet reached.
    // 1:     This point was reached, but is no longer on the current descent.
    // 2 + k: This point is on the current descent, and descent_edges_[k] is
    //        the edge that leaves it.
    int visited{0};
  };

  void init(int ptN, int shortN);
  Edge* newEdge(const tmg_conn* conn, int fr, int to);
  Edge* newShortEdge(const tmg_conn* conn, int fr, int to);
  Edge* getNextEdge(bool ok_to_descend);
  Edge* getFirstEdge(int jstart);
  Edge* getFirstNonShortEdge(int& jstart);
  void addEdges(const tmg_conn* conn, int i0, int i1, int k);
  void clearVisited();
  void relocateShorts(tmg_conn* conn);
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);
  Point& pt(const int index) { return points_[index]; }
  const Point& pt(const int index) const { return points_[index]; }

  // TODO: make private
  std::vector<Point> points_;
  std::vector<Edge*> descent_edges_;

 private:
  void getEdgeRefCoord(const tmg_conn* conn, Edge* pe, int& rx, int& ry);
  bool isBadShort(Edge* pe, const tmg_conn* conn);

  std::deque<Edge> edges_;

  Edge* next_edge_{nullptr};
};

}  // namespace odb
