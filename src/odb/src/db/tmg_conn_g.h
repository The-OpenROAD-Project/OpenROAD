// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace odb {

class tmg_conn;
struct tmg_rcshort;

struct tcg_edge
{
  tcg_edge* next;
  tcg_edge* reverse;
  tmg_rcshort* s;
  int fr;
  int to;
  int k;  // index to _rcV
  bool visited;
  bool skip;
};

struct tcg_pt
{
  tcg_edge* edges;
  int ipath;
  int visited;  // 1= from another descent, 2+k= _stackV[k]->fr
};

class tmg_conn_graph
{
 public:
  tmg_conn_graph();
  ~tmg_conn_graph();
  void init(int ptN, int shortN);
  tcg_edge* newEdge(const tmg_conn* conn, int fr, int to);
  tcg_edge* newShortEdge(const tmg_conn* conn, int fr, int to);
  tcg_edge* getNextEdge(bool ok_to_descend);
  tcg_edge* getFirstEdge(int jstart);
  tcg_edge* getFirstNonShortEdge(int& jstart);
  void addEdges(const tmg_conn* conn, int i0, int i1, int k);
  void clearVisited();
  void relocateShorts(tmg_conn* conn);
  void getEdgeRefCoord(const tmg_conn* conn, tcg_edge* pe, int& rx, int& ry);
  bool isBadShort(tcg_edge* pe, const tmg_conn* conn);
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);
  tcg_pt& pt(const int index) { return _ptV[index]; }
  const tcg_pt& pt(const int index) const { return _ptV[index]; }

 public:
  tcg_pt* _ptV;
  int _ptN;
  int* _path_vis;
  tcg_edge** _stackV;
  int _stackN;

 private:
  tcg_edge* _e;
  int _ptNmax;
  int _shortNmax;
  int _eNmax;
  tcg_edge* _eV;
  int _eN;
};

}  // namespace odb
