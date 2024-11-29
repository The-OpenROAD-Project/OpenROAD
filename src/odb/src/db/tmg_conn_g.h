///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
  void getEdgeRefCoord(tmg_conn* conn, tcg_edge* pe, int& rx, int& ry);
  bool isBadShort(tcg_edge* pe, tmg_conn* conn);
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);

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
