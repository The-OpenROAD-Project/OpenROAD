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
    int from;
    int to;
    Edge* next;

    // The same edge in the other direction. Every connection between
    // two points is stored twice in the graph, once per direction, so
    // a walk can leave a point along any of its edges.
    Edge* reverse;

    // An edge is either a path or a short:
    // If wire_short is nullptr and wire_section_index is != -1,
    // the edge is a path. The opposite for a short.
    Short* wire_short;
    int wire_section_index;

    // Note that when an edge is marked as visited, it's reverse version is
    // also marked as visited.
    bool visited;

    // When an edge is removed from the graph, the latter is not rebuilt.
    // Instead, that edge is marked as "skip" and the walkers will ignore
    // it when traversing.
    bool skip;
  };

  struct Point
  {
    Edge* first_edge;  // Head of the chain of edges that leave this point.
    int path_index;
    int visited;  // 1= from another descent, 2+k= _stackV[k]->fr
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
  Point& pt(const int index) { return ptV_[index]; }
  const Point& pt(const int index) const { return ptV_[index]; }

  // TODO: make private
  std::vector<Point> ptV_;
  std::vector<Edge*> stackV_;

 private:
  void getEdgeRefCoord(const tmg_conn* conn, Edge* pe, int& rx, int& ry);
  bool isBadShort(Edge* pe, const tmg_conn* conn);

  Edge* e_;
  std::deque<Edge> eV_;
};

}  // namespace odb
