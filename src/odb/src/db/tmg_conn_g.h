// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <deque>
#include <vector>

namespace odb {

class tmg_conn;
struct Short;

struct tcg_edge
{
  int from;
  int to;
  tcg_edge* next;

  // The same edge in the other direction. Every connection between
  // two points is stored twice in the graph, once per direction, so
  // a walk can leave a point along any of its edges.
  tcg_edge* reverse;

  // An edge is either a path or a short:
  // If wire_short is nullptr and wire_section_index is != -1,
  // the edge is a path. The opposite for a short.
  Short* wire_short;
  int wire_section_index;

  // When this edge is marked as visited, it's reverse version is also
  // marked as visited.
  bool visited;

  // When an edge is removed from the graph, the latter is not rebuilt.
  // Instead, that edge is marked as "skip" and the walkers will ignore
  // it when traversing. I.e., this edge was removed from the graph.
  bool skip;
};

struct tcg_pt
{
  tcg_edge* first_edge; // Head of the chain of edges that leave this point.
  int path_index;
  int visited;  // 1= from another descent, 2+k= _stackV[k]->fr
};

class ConnectionGraph
{
 public:
  void init(int ptN, int shortN);
  tcg_edge* newEdge(const tmg_conn* conn, int fr, int to);
  tcg_edge* newShortEdge(const tmg_conn* conn, int fr, int to);
  tcg_edge* getNextEdge(bool ok_to_descend);
  tcg_edge* getFirstEdge(int jstart);
  tcg_edge* getFirstNonShortEdge(int& jstart);
  void addEdges(const tmg_conn* conn, int i0, int i1, int k);
  void clearVisited();
  void relocateShorts(tmg_conn* conn);
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);
  tcg_pt& pt(const int index) { return ptV_[index]; }
  const tcg_pt& pt(const int index) const { return ptV_[index]; }

  // TODO: make private
  std::vector<tcg_pt> ptV_;
  std::vector<tcg_edge*> stackV_;

 private:
  void getEdgeRefCoord(const tmg_conn* conn, tcg_edge* pe, int& rx, int& ry);
  bool isBadShort(tcg_edge* pe, const tmg_conn* conn);

  tcg_edge* e_;
  std::deque<tcg_edge> eV_;
};

}  // namespace odb
