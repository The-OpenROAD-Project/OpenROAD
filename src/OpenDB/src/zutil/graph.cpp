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
#include "graph.h"

#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "dbLogger.h"

#define MAXIT 1000
#define LARGE 2000000000
int cmpwt(const void* a, const void* b)
{
  Edge* ae = *(Edge**) a;
  Edge* be = *(Edge**) b;
  if (ae->wt < be->wt)
    return -1;
  if (ae->wt > be->wt)
    return 1;
  return 0;
}
Queue::Queue(int size)
{
  _front = 0;
  _back = -1;
  _size = size;
  if (size)
    _qu = (int*) malloc(size * sizeof(int));
}
void Queue::set_size(int size)
{
  _size = size;
  if (_qu)
    free(_qu);
  _qu = (int*) malloc(size * sizeof(int));
}
Queue::~Queue(void)
{
  if (_qu)
    free(_qu);
}

void Queue::reset(void)
{
  _front = 0;
  _back = -1;
}

int Queue::is_empty()
{
  return (_back < _front);
}

void Queue::insert(int el)
{
  assert(_back < _size);
  _qu[++_back] = el;
}
int Queue::remove(void)
{
  assert(_back >= _front);
  return _qu[_front++];
}
void Graph::sort_neighbor_weights(void)
{
  int i;
  for (i = 0; i < _neighbors.n(); i++) {
    Darr<Edge*>* ne = _neighbors.get(i);
    ne->dsort(cmpwt);
  }
}
Graph::Graph(int n)
{
  _num_left = n;
  _num_vert = n;
  _match = (int*) malloc(n * sizeof(int));
  int i;
  for (i = 0; i < n; i++) {
    Darr<Edge*>* da = new Darr<Edge*>;
    _neighbors.insert(da);
    _degree.insert(0);
    _match[i] = -1;
  }
}
void Graph::set_num_left(int n = 0)
{
  assert(_num_left == 0);
  _num_left = n;
  _num_vert = n;
  _match = (int*) malloc(n * sizeof(int));
  int i;
  for (i = 0; i < n; i++) {
    Darr<Edge*>* da = new Darr<Edge*>;
    _neighbors.insert(da);
    _degree.insert(0);
    _match[i] = -1;
  }
}
int Graph::get_num_left(void)
{
  return _num_left;
}
void Graph::add_right_vertex(int thresh)
{
  Darr<Edge*>* neigh = new Darr<Edge*>;
  _num_vert++;
  _neighbors.insert(neigh);
  _degree.insert(0);
  _thresh.insert(thresh);
}
int Graph::get_thresh(int vertex)
{
  if (vertex < _num_left)
    return 1;
  return (_thresh.get(vertex - _num_left));
}
void Graph::add_edge(int left, int right, long wt)
{
  Keyval<int, int> kv;
  kv.key = left;
  kv.val = right;
  Edge* ed;
  if (_edge_table.find(kv, ed)) {
    ed->wt = wt;
    return;
  }
  ed = (Edge*) malloc(sizeof(Edge));
  ed->left = left;
  ed->right = right;
  ed->wt = wt;
  _edges.insert(ed);
  int m = right;
  Darr<Edge*>* r = _neighbors.get(m);
  _edge_table.insert(kv, ed);
  r->insert(ed);
  int dr = _degree.get(m);
  _degree.set(m, (dr + 1));
  Darr<Edge*>* l = _neighbors.get(left);
  dr = _degree.get(left);
  _degree.set(left, (dr + 1));
  l->insert(ed);
}
Darr<Edge*>* Graph::get_neighbors(int vertex)
{
  return _neighbors.get(vertex);
}
void Graph::get_neighbor(int vertex, int i, int& neigh, long& wt)
{
  Darr<Edge*>* ne = _neighbors.get(vertex);
  assert(i < ne->n());
  Edge* ed = ne->get(i);
  if (vertex < _num_left)
    neigh = ed->right;
  else
    neigh = ed->left;
  wt = ed->wt;
}
int Graph::get_degree(int vertex)
{
  return (_degree.get(vertex));
}
Darr<Edge*> Graph::get_all_edges(void)
{
  return _edges;
}
int Graph::get_num_vertices(void)
{
  return _num_vert;
}
Graph::~Graph()
{
  int i;
  for (i = 0; i < _edges.n(); i++) {
    free(_edges.get(i));
  }
  for (i = 0; i < _neighbors.n(); i++) {
    delete _neighbors.get(i);
  }
  free(_match);
}
int Graph::get_matched_vertex(int i)
{
  assert(i < _num_left);
  return _match[i];
}
void Graph::print()
{
  int i;
  for (i = 0; i < _edges.n(); i++) {
    int left = _edges.get(i)->left;
    int right = _edges.get(i)->right;
    int wt = _edges.get(i)->wt;
    odb::notice(0, "left=%d, right=%d, wt=%d\n", left, right, wt);
  }
}
void Graph::find_connected_components(Darr<Darr<Edge*>*>& ed)
{
  int* vis = (int*) malloc(_num_vert * sizeof(int));
  int i;
  for (i = 0; i < _num_vert; i++)
    vis[i] = -1;
  int k = 0;
  Hash<Edge*, int> edge_tab;
  for (i = 0; i < _num_left; i++) {
    if (vis[i] != -1)
      continue;
    Darr<Edge*>* edges = new Darr<Edge*>;
    ed.insert(edges);
    dfs(i, vis, k, edges, edge_tab);
    k++;
  }
  free(vis);
  // odb::notice(0,"The number of connected components is %d\n", ed.n());
}

void Graph::dfs(int i,
                int* vis,
                int k,
                Darr<Edge*>* edg,
                Hash<Edge*, int>& edge_tab)
{
  if (vis[i] != -1)
    return;
  vis[i] = k;
  Darr<Edge*>* edges = _neighbors.get(i);
  int j;
  for (j = 0; j < edges->n(); j++) {
    int vvv;
    if (!edge_tab.find(edges->get(j), vvv)) {
      edg->insert(edges->get(j));
      edge_tab.insert(edges->get(j), 1);
    }
    int l;
    if (i < _num_left)
      l = edges->get(j)->right;
    else
      l = edges->get(j)->left;
    dfs(l, vis, k, edg, edge_tab);
  }
}

static void relax(int u, int v, long wt, long* dist, int* pred)
{
  if (dist[u] >= LARGE)
    return;
  int d = dist[u] + wt;
  if (d < dist[v]) {
    dist[v] = d;
    pred[v] = u;
  }
  return;
}
static int cycle_rec(int i, int* pred, int* vis, int l, int& k)
{
  if (vis[i] == l) {
    k = i;
    return 1;
  }
  if (vis[i])
    return 0;
  if (pred[i] == i)
    return 0;
  vis[i] = l;
  return cycle_rec(pred[i], pred, vis, l, k);
}
int Graph::_negative_cycle(long* dist, int* pred, int* vis, int* rightd, int& k)
{
  int i;
  int n = _num_vert;
  for (i = 0; i < n; i++)
    vis[i] = 0;
  int l = 0;
  for (i = 0; i < n; i++) {
    if (vis[i])
      continue;
    l++;
    if (cycle_rec(i, pred, vis, l, k)) {
      if (k < _num_left)
        k = pred[k];
      return 1;
    }
  }
  for (i = _num_left; i < n; i++) {
    if ((dist[i] < 0) && (rightd[i - _num_left] < _thresh.get(i - _num_left))) {
      k = i;
      return 1;
    }
  }
  return 0;
}

int no_change(long* dist, long* predist, int n)
{
  int i;
  for (i = 0; i < n; i++) {
    if (dist[i] != predist[i])
      return 0;
  }
  return 1;
}
int Graph::_bellman_ford(Darr<Edge*>* edg,
                         long* dist,
                         int* pred,
                         int* vis,
                         int* rightd,
                         int factor,
                         int& k)
{
  int i;
  int n = _num_vert;
  long* predist = (long*) malloc(n * sizeof(long));
  for (i = 0; i < n; i++) {
    pred[i] = i;
  }
  for (i = _num_left; i < n; i++) {
    dist[i] = LARGE;
  }
  for (i = 0; i < _num_left; i++) {
    dist[i] = LARGE;
    dist[_match[i]] = 0;
  }
  int val = 0;
  for (i = 0; i < n; i++) {
    int j;
    int l;
    for (l = 0; l < n; l++)
      predist[l] = dist[l];
    for (j = 0; j < edg->n(); j++) {
      int left = edg->get(j)->left;
      int right = edg->get(j)->right;
      long wt = edg->get(j)->wt / factor;
      if (_match[left] == right)
        relax(right, left, -wt, dist, pred);
      else
        relax(left, right, wt, dist, pred);
    }
    if (no_change(dist, predist, n))
      break;
    val = _negative_cycle(dist, pred, vis, rightd, k);
    if (val)
      break;
  }
  free(predist);
  return val;
}
long Graph::matchwt()
{
  int i;
  long ww = 0;
  for (i = 0; i < _num_left; i++) {
#ifndef NDEBUG
    Keyval<int, int> kv;
    kv.key = i;
    kv.val = _match[i];
#endif
    assert(_match[i] != -1);
    Edge* ed;
    assert(_edge_table.find(kv, ed));
    ww += ed->wt;
  }
  return ww;
}
void Graph::_dfs_hall(int i, int* visited, int* parent, int l)
{
  if (visited[i] != -1) {
    if (visited[i] != l)
      parent[l] = visited[i];
    return;
  }
  visited[i] = l;
  Darr<Edge*>* edges = _neighbors.get(i);
  int j;
  if (i < _num_left) {
    for (j = 0; j < edges->n(); j++) {
      Edge* ed = edges->get(j);
      int k = ed->right;
      if (_match[i] == k)
        continue;
      _dfs_hall(k, visited, parent, l);
    }
    return;
  }
  for (j = 0; j < edges->n(); j++) {
    Edge* ed = edges->get(j);
    int k = ed->left;
    if (_match[k] != i)
      continue;
    _dfs_hall(k, visited, parent, l);
    break;
  }
}
int Graph::find_hall_set(Darr<Darr<int>*>& hall)
{
  int i;
  int* visited = (int*) malloc(_num_vert * sizeof(int));
  int k = 0;
  for (i = 0; i < _num_left; i++) {
    if (_match[i] != -1)
      continue;
    k++;
  }
  int* parent = (int*) malloc(_num_vert * sizeof(int));
  for (i = 0; i < _num_vert; i++)
    parent[i] = i;
  k = 0;
  for (i = 0; i < _num_vert; i++)
    visited[i] = -1;
  for (i = 0; i < _num_left; i++) {
    if (_match[i] != -1)
      continue;
    _dfs_hall(i, visited, parent, k);
    k++;
  }
  for (i = 0; i < _num_vert; i++) {
    if (visited[i] == -1)
      continue;
    int l = visited[i];
    while (l != parent[l])
      l = parent[l];
    visited[i] = l;
  }
  Hash<int, int> numc;
  for (i = 0; i < _num_vert; i++) {
    if (visited[i] == -1)
      continue;
    int vvv;
    if (numc.find(visited[i], vvv)) {
      Darr<int>* set = hall.get(vvv);
      set->insert(i);
      continue;
    }
    numc.insert(visited[i], hall.n());
    Darr<int>* set = new Darr<int>;
    hall.insert(set);
    set->insert(i);
  }
  // verification
  for (i = 0; i < hall.n(); i++) {
    Darr<int>* set = hall.get(i);
    int j;
    int numl = 0, numr = 0;
    for (j = 0; j < set->n(); j++) {
      if (set->get(j) < _num_left)
        numl++;
      else
        numr++;
    }
    if (numl <= numr) {
      odb::notice(0,
                  "%dth set is not a hall set. numl is %d and numr is %d\n",
                  i,
                  numl,
                  numr);
    }
  }
  free(visited);
  free(parent);
  return 1;
}
int Graph::find_matching(Matchkind kind)
{
  // this->print();
  int i;
  // for(i = 0; i<_num_left; i++) {
  // if(_degree.get(i) == 0)
  //  return 0;
  //}
  int noniso = 0;
  // odb::notice(0,"Numright is %d\n", _num_vert-_num_left);
  // odb::notice(0,"Number of edges is %d\n", _edges.n());
  for (i = _num_left; i < _num_vert; i++) {
    if (_degree.get(i) > 0)
      noniso += _thresh.get(i - _num_left);
  }
  // if(_num_left>noniso++)
  // return 0;
  int n = _num_vert;
  Queue qu(2 * _edges.n() + 2 * n + 1);
  int* pred = (int*) malloc(n * sizeof(int));
  int* visited = (int*) malloc(n * sizeof(int));
  Darr<int> chpr;
  Darr<int> chvis;

  // sort_neighbor_weights();
  int found = 1;
  int* rightd = (int*) malloc((n - _num_left) * sizeof(int));
  for (i = 0; i < n - _num_left; i++) {
    rightd[i] = 0;
  }
  for (i = 0; i < _num_left; i++) {
    if (_match[i] != -1)
      (rightd[_match[i] - _num_left])++;
  }
  for (i = 0; i < n; i++) {
    pred[i] = i;
    visited[i] = 0;
  }
  while (found) {
    found = 0;
    qu.reset();
    for (i = 0; i < _num_left; i++) {
      if (_match[i] == -1)
        qu.insert(i);
    }
    for (i = 0; i < chpr.n(); i++) {
      int m = chpr.get(i);
      pred[m] = m;
    }
    for (i = 0; i < chvis.n(); i++) {
      int m = chvis.get(i);
      visited[m] = 0;
    }
    chpr.reset();
    chvis.reset();
    int ne;
    while (!found) {
      if (qu.is_empty())
        break;
      int el = qu.remove();
      if (visited[el])
        continue;
      chvis.insert(el);
      visited[el] = 1;
      if (el < _num_left) {
        Darr<Edge*>* edges = _neighbors.get(el);
        for (i = 0; i < edges->n(); i++) {
          Edge* ed = edges->get(i);
          ne = ed->right;
          if (_match[el] == ne)
            continue;
          if (pred[ne] == ne)
            chpr.insert(ne);
          if (!visited[ne]) {
            pred[ne] = el;
            qu.insert(ne);
          }
          if (rightd[ne - _num_left] < _thresh.get(ne - _num_left)) {
            (rightd[ne - _num_left])++;
            found = 1;
            break;
          }
        }
        if (found)
          break;
        continue;
      }
      Darr<Edge*>* edges = _neighbors.get(el);
      for (i = 0; i < edges->n(); i++) {
        Edge* ed = edges->get(i);
        ne = ed->left;
        if (_match[ne] != el)
          continue;
        if (pred[ne] == ne)
          chpr.insert(ne);
        if (!visited[ne]) {
          pred[ne] = el;
          qu.insert(ne);
        }
      }
    }
    if (!found)
      break;
    int j = ne;
    while (pred[j] != j) {
      assert(j >= _num_left);
      _match[pred[j]] = j;
      j = pred[pred[j]];
    }
  }
  for (i = 0; i < _num_left; i++) {
    if (_match[i] == -1) {
      free(visited);
      free(pred);
      free(rightd);
      return 0;
    }
  }
  if (kind == NOOPT) {
    free(visited);
    free(pred);
    free(rightd);
    return 1;
  }
  long* dist = (long*) malloc(n * sizeof(long));
  int k;

  // long matw = LARGE;
  Darr<Darr<Edge*>*> ed;
  find_connected_components(ed);
  int ll;
  for (ll = 0; ll < ed.n(); ll++) {
    int factor = 1;
    int maxwt = 0;
    for (i = 0; i < ed.get(ll)->n(); i++) {
      if (maxwt < ed.get(ll)->get(i)->wt)
        maxwt = ed.get(ll)->get(i)->wt;
    }
    while (true) {
      if (10 * factor > maxwt)
        break;
      factor = 2 * factor;
    }

    while (true) {
      /*long nm = matchwt();
      assert(nm<matw); */
      int val
          = _bellman_ford(ed.get(ll), dist, pred, visited, rightd, factor, k);
      if (val) {
        int l = k;
        assert(k >= _num_left);
        while (pred[l] != l) {
          _match[pred[l]] = l;
          l = pred[pred[l]];
          if (l == k)
            break;
        }
        if (pred[l] == l) {
          (rightd[l - _num_left])--;
          (rightd[k - _num_left])++;
          /*assert(rightd[l-_num_left]>=0);
          assert(rightd[k-_num_left]<=1); */
        }
      } else {
        factor = factor / 2;
        if (factor <= 1000)
          break;
      }
    }  // while
  }    // for
  /*for(i = 0; i<_num_left; i++)
    odb::notice(0,"%d %d\n", i, _match[i]); */
  for (i = 0; i < ed.n(); i++)
    delete ed.get(i);
  free(visited);
  free(pred);
  free(dist);
  free(rightd);
  return 1;
}
