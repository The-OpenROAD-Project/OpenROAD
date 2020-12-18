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

/*-------------------------------------------------------------
////	AUTHOR: SANJEEV MAHAJAN
---------------------------------------------------------------*/
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "hash.h"
#define MAXIT 1000
#define LARGE 2000000000
enum Matchkind
{
  OPT = 0,
  NOOPT
};
typedef struct _Edge
{
  int  left;
  int  right;
  long wt;
} Edge;
class Queue
{
 public:
  Queue(int size = 0);
  void insert(int el);
  int  remove(void);
  void set_size(int size);
  void reset();
  int  is_empty();
  ~Queue(void);

 private:
  int* _qu;
  int  _size;
  int  _front;
  int  _back;
};

class Graph
{
 public:
  Graph(int n = 0);
  void         add_edge(int left, int right, long weight);
  void         set_num_left(int);
  int          get_num_left(void);
  Darr<Edge*>* get_neighbors(int vertex);
  int          get_degree(int vertex);
  Darr<Edge*>  get_all_edges();
  int          get_num_vertices();
  void         get_neighbor(int vertex, int i, int& neigh, long& wt);
  void         add_right_vertex(int thresh = 1);
  ~Graph(void);
  void sort_neighbor_weights(void);
  int  get_thresh(int vertex);
  int  find_matching(Matchkind kind = OPT);
  int  get_matched_vertex(int i);
  void print();
  long matchwt();
  void find_connected_components(Darr<Darr<Edge*>*>& ed);
  void dfs(int               i,
           int*              vis,
           int               k,
           Darr<Edge*>*      edg,
           Hash<Edge*, int>& edge_tab);
  int  find_hall_set(Darr<Darr<int>*>& hall);

 private:
  Darr<Darr<Edge*>*> _neighbors;
  int                _num_left;
  int                _num_vert;
  Darr<int>          _degree;
  Darr<Edge*>        _edges;
  HashP<int, Edge*>  _edge_table;
  Darr<int>          _thresh;
  int*               _match;
  int  _negative_cycle(long* dist, int* pred, int* vis, int* rightd, int& k);
  int  _bellman_ford(Darr<Edge*>* edges,
                     long*        dist,
                     int*         pred,
                     int*         vis,
                     int*         rightd,
                     int          factor,
                     int&         k);
  void _dfs_hall(int i, int* vis, int* par, int k);
};

