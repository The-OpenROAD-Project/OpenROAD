// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tmg_conn_g.h"

#include <cstdio>
#include <cstdlib>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "tmg_conn.h"
#include "utl/Logger.h"

namespace odb {

tmg_conn_graph::tmg_conn_graph()
{
  _ptNmax = 1024;
  _shortNmax = 1024;
  _eNmax = 1024;
  _ptV = (tcg_pt*) malloc(_ptNmax * sizeof(tcg_pt));
  _path_vis = (int*) malloc(_ptNmax * sizeof(int));
  _eV = (tcg_edge*) malloc(2UL * _ptNmax * sizeof(tcg_edge));
  _stackV = (tcg_edge**) malloc(_shortNmax * sizeof(tcg_edge*));
}

tmg_conn_graph::~tmg_conn_graph()
{
  free(_ptV);
  free(_path_vis);
  free(_eV);
  free(_stackV);
}

void tmg_conn_graph::init(const int ptN, const int shortN)
{
  if (ptN > _ptNmax) {
    _ptNmax = 2 * ptN;
    free(_ptV);
    _ptV = (tcg_pt*) malloc(_ptNmax * sizeof(tcg_pt));
    free(_path_vis);
    _path_vis = (int*) malloc(_ptNmax * sizeof(int));
  }
  if (shortN > _shortNmax) {
    _shortNmax = 2 * shortN;
    free(_stackV);
    _stackV = (tcg_edge**) malloc(_shortNmax * sizeof(tcg_edge*));
  }
  if (4 * ptN + 2 * shortN > _eNmax) {
    _eNmax *= 2;
    if (4 * ptN + 2 * shortN > _eNmax) {
      _eNmax = 4 * ptN + 2 * shortN;
    }
    free(_eV);
    _eV = (tcg_edge*) malloc(_eNmax * sizeof(tcg_edge));
  }
  _eN = 0;
  for (int j = 0; j < ptN; j++) {
    _ptV[j].edges = nullptr;
  }
  _ptN = ptN;
}

tcg_edge* tmg_conn_graph::newEdge(const tmg_conn* conn,
                                  const int fr,
                                  const int to)
{
  tcg_edge* e = _eV + _eN++;
  e->k = -1;
  e->skip = false;
  const int ndx = conn->pt(to)._x;
  const int ndy = conn->pt(to)._y;
  tcg_edge* prev_edge = nullptr;
  tcg_edge* edge = _ptV[fr].edges;
  while (edge && !edge->s && ndx > conn->pt(edge->to)._x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && !edge->s && ndx == conn->pt(edge->to)._x
         && ndy > conn->pt(edge->to)._y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    _ptV[fr].edges = e;
  }
  return e;
}

tcg_edge* tmg_conn_graph::newShortEdge(const tmg_conn* conn,
                                       const int fr,
                                       const int to)
{
  tcg_edge* e = _eV + _eN++;
  e->k = -1;
  e->skip = false;
  const int ned = conn->ptDist(fr, to);
  const int ndx = conn->pt(to)._x;
  const int ndy = conn->pt(to)._y;
  tcg_edge* prev_edge = nullptr;
  tcg_edge* edge = _ptV[fr].edges;
  while (edge && ned > conn->ptDist(edge->fr, edge->to)) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->ptDist(edge->fr, edge->to)
         && ndx > conn->pt(edge->to)._x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->ptDist(edge->fr, edge->to)
         && ndx == conn->pt(edge->to)._x && ndy > conn->pt(edge->to)._y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    _ptV[fr].edges = e;
  }
  return e;
}

void tmg_conn_graph::clearVisited()
{
  for (int j = 0; j < _eN; j++) {
    _eV[j].visited = false;
  }
  for (int j = 0; j < _ptN; j++) {
    _ptV[j].visited = 0;
  }
}

void tmg_conn_graph::getEdgeRefCoord(const tmg_conn* conn,
                                     tcg_edge* pe,
                                     int& rx,
                                     int& ry)
{
  rx = conn->pt(pe->to)._x;
  ry = conn->pt(pe->to)._y;
  if (pe->s == nullptr) {
    return;
  }
  tcg_edge* se = pt(pe->to).edges;
  while (se && se->s) {
    se = se->next;
  }
  if (se == nullptr) {
    return;
  }
  rx = conn->pt(se->to)._x;
  ry = conn->pt(se->to)._y;
}

bool tmg_conn_graph::isBadShort(tcg_edge* pe, const tmg_conn* conn)
{
  if (pe->s == nullptr) {
    return false;
  }
  const tmg_rcpt& from = conn->pt(pe->fr);
  const tmg_rcpt& to = conn->pt(pe->to);
  return from._x != to._x || from._y != to._y;
}

void tmg_conn_graph::relocateShorts(tmg_conn* conn)
{
  for (int jp = 0; jp < _ptN; jp++) {
    tcg_edge* pe = pt(jp).edges;
    if (pe == nullptr || pe->next == nullptr) {
      continue;
    }
    bool needAdjust = true;
    while (needAdjust) {
      needAdjust = false;
      int r1x, r1y;
      bool firstCheck = true;
      tcg_edge* pppe = nullptr;
      tcg_edge* ppe = nullptr;
      for (pe = pt(jp).edges; pe != nullptr; pe = pe->next) {
        if (ppe == nullptr) {
          ppe = pe;
          continue;
        }
        if (firstCheck) {
          getEdgeRefCoord(conn, ppe, r1x, r1y);
        }
        firstCheck = false;
        int r2x;
        int r2y;
        getEdgeRefCoord(conn, pe, r2x, r2y);
        if ((pe->s == nullptr && ppe->s == nullptr) || isBadShort(pe, conn)
            || isBadShort(ppe, conn)) {
          pppe = ppe;
          ppe = pe;
          r1x = r2x;
          r1y = r2y;
          continue;
        }
        if (r1x > r2x || (r1x == r2x && r1y > r2y)) {
          needAdjust = true;
          tcg_edge* last = pe->next;
          if (pppe) {
            pppe->next = pe;
          } else {
            pt(jp).edges = pe;
          }
          pppe = pe;
          pe->next = ppe;
          ppe->next = last;
          pe = ppe;
        } else {
          pppe = ppe;
          ppe = pe;
          r1x = r2x;
          r1y = r2y;
        }
      }
    }
  }
  // re-assign "skip".
  for (int jp = 0; jp < _ptN; jp++) {
    tcg_edge* skipe = nullptr;
    int noshortn = 0;
    int shortn = 0;
    tcg_edge* plast = nullptr;
    tcg_edge* last = nullptr;
    for (tcg_edge* pe = pt(jp).edges; pe != nullptr; pe = pe->next) {
      if (!pe->s) {
        noshortn++;
        continue;
      }
      shortn++;
      if (isBadShort(pe, conn)) {
        continue;  // bad short
      }
      if (pe->skip) {
        if (!skipe) {
          skipe = pe;
        }
      }
      plast = last;
      last = pe;
    }
    if (!skipe) {
      continue;  // no need to adjust skip
    }
    if (noshortn <= 1) {
      continue;  // adjust only the long (main) branch
    }
    if (shortn <= 1) {
      continue;  //  bp. because skipe != nullptr
    }
    if (!plast) {
      continue;  // may happen with bad short    wfs 6-27-06
    }
    // plast->to and last->to is the short pair to skip
    // do skip new pair;
    tcg_edge* nse = pt(plast->to).edges;
    while (nse != nullptr && nse->to != last->to) {
      nse = nse->next;
    }
    if (nse && nse->s) {
      nse->s->_skip = true;
      nse->skip = true;
      nse->reverse->skip = true;
    } else {
      return;
    }
    // unskip skipe
    skipe->s->_skip = false;
    skipe->skip = false;
    skipe->reverse->skip = false;
  }
}

tcg_edge* tmg_conn_graph::getFirstNonShortEdge(int& jstart)
{
  if (pt(jstart).visited || !pt(jstart).edges) {
    return nullptr;
  }
  tcg_edge* e = pt(jstart).edges;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (!e) {
    return nullptr;
  }
  uint loops = 16;
  while (loops && e->s) {
    jstart = jstart == e->s->_i0 ? e->s->_i1 : e->s->_i0;
    e = pt(jstart).edges;
    loops--;
  }
  if (loops == 0) {
    e = nullptr;
  }
  if (!e) {
    _stackV[0] = nullptr;
    _stackN = 0;
    return nullptr;
  }
  _stackV[0] = e;
  _stackN = 1;
  return e;
}

tcg_edge* tmg_conn_graph::getFirstEdge(const int jstart)
{
  if (pt(jstart).visited || !pt(jstart).edges) {
    return nullptr;
  }
  tcg_edge* e = pt(jstart).edges;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (!e) {
    return nullptr;
  }
  _stackV[0] = e;
  _stackN = 1;
  return e;
}

tcg_edge* tmg_conn_graph::getNextEdge(const bool ok_to_descend)
{
  tcg_edge* e = _stackV[_stackN - 1];

  if (ok_to_descend) {
    tcg_edge* e2 = pt(e->to).edges;
    while (e2 && (e2->visited || e2->skip)) {
      e2 = e2->next;
    }
    if (e2) {
      if (_stackN >= _shortNmax) {
        _shortNmax = _shortNmax * 2;
        _stackV = (tcg_edge**) realloc(_stackV, _shortNmax * sizeof(tcg_edge*));
      }
      _stackV[_stackN++] = e2;
      return e2;
    }
  }
  // sibling
  // avoid resetting loop node
  if (pt(e->to).visited > pt(e->fr).visited) {
    pt(e->to).visited = 1;
  }
  e = e->next;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (e) {
    _stackV[_stackN - 1] = e;
    return e;
  }
  // ascend
  while (--_stackN > 0) {
    e = _stackV[_stackN - 1];
    pt(e->to).visited = 1;
    e = e->next;
    while (e && (e->visited || e->skip)) {
      e = e->next;
    }
    if (e) {
      _stackV[_stackN - 1] = e;
      return e;
    }
  }
  return nullptr;
}

void tmg_conn::relocateShorts()
{
  _graph->relocateShorts(this);
}

void tmg_conn::removeShortLoops()
{
  if (!_graph) {
    _graph = std::make_unique<tmg_conn_graph>();
  }
  _graph->init(_ptV.size(), _shortV.size());
  tcg_pt* pgV = _graph->_ptV;

  // setup paths
  int npath = -1;
  for (size_t j = 0; j < _rcV.size(); j++) {
    if (j == 0 || _rcV[j]._from_idx != _rcV[j - 1]._to_idx) {
      ++npath;
    }
    pgV[_rcV[j]._from_idx].ipath = npath;
    pgV[_rcV[j]._to_idx].ipath = npath;
  }
  npath++;

  // remove shorts to same path
  for (int j = 0; j < _shortV.size(); j++) {
    tmg_rcshort* s = &_shortV[j];
    if (s->_skip) {
      continue;
    }
    if (pgV[s->_i0].ipath == pgV[s->_i1].ipath) {
      s->_skip = true;
    }
  }

  for (int j = 0; j < _shortV.size(); j++) {
    tmg_rcshort* s = &_shortV[j];
    if (s->_skip) {
      continue;
    }
    tcg_edge* e;
    for (e = pgV[s->_i0].edges; e; e = e->next) {
      if (e->to == s->_i1) {
        break;
      }
    }
    if (e) {
      s->_skip = true;
      continue;
    }
    e = _graph->newShortEdge(this, s->_i0, s->_i1);
    tcg_edge* e2 = _graph->newShortEdge(this, s->_i1, s->_i0);
    e->s = s;
    e2->s = s;
    e->reverse = e2;
    e2->reverse = e;
    e->fr = s->_i0;
    e->to = s->_i1;
    e2->fr = s->_i1;
    e2->to = s->_i0;
    e->visited = false;
    e2->visited = false;
  }

  for (int j = 0; j < _ptV.size(); j++) {
    pgV[j].visited = 0;
  }

  // remove all short loops
  int* path_vis = _graph->_path_vis;
  _graph->clearVisited();
  for (int j = 0; j < npath; j++) {
    path_vis[j] = 0;
  }

  for (int jstart = 0; jstart < _ptV.size(); jstart++) {
    tcg_edge* e = _graph->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      tcg_pt* pg = pgV + e->to;
      if (pg->visited) {
        e->skip = true;
        e->reverse->skip = true;
        e->s->_skip = true;
        e = _graph->getNextEdge(false);
      } else {
        pg->visited = 2 + _graph->_stackN;
        e = _graph->getNextEdge(true);
      }
    }
  }

  // count components, and remaining loops
  _graph->clearVisited();
  for (int jstart = 0; jstart < _ptV.size(); jstart++) {
    tcg_edge* e = _graph->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      tcg_pt* pg = pgV + e->to;
      if (pg->visited) {
        e = _graph->getNextEdge(false);
      } else {
        pg->visited = 2 + _graph->_stackN;
        e = _graph->getNextEdge(true);
      }
    }
  }
}

void tmg_conn_graph::addEdges(const tmg_conn* conn,
                              const int i0,
                              const int i1,
                              const int k)
{
  tcg_edge* e = newEdge(conn, i0, i1);
  tcg_edge* e2 = newEdge(conn, i1, i0);

  e->s = nullptr;
  e->reverse = e2;
  e->fr = i0;
  e->to = i1;
  e->k = k;
  e->visited = false;
  e->skip = false;

  e2->s = nullptr;
  e2->reverse = e;
  e2->fr = i1;
  e2->to = i0;
  e2->k = k;
  e2->visited = false;
  e2->skip = false;
}

void tmg_conn::removeWireLoops()
{
  removeShortLoops();

  // loops involving only shorts have already been handled
  if (_rcV.empty()) {
    return;
  }
  // add all path edges
  for (size_t j = 0; j < _rcV.size(); j++) {
    _graph->addEdges(this, _rcV[j]._from_idx, _rcV[j]._to_idx, j);
  }

  // remove loops that have shorts by removing
  // the short with max distance
  // if no shorts, allow the loop to stay;
  // we do not expect any router to have a pure path loop

  tcg_pt* pgV = _graph->_ptV;

  bool done = false;
  while (!done) {
    int loop_removed = 0;
    done = true;
    _graph->clearVisited();
    for (int jstart = 0; jstart < _ptV.size(); jstart++) {
      tcg_edge* e = _graph->getFirstEdge(jstart);
      if (!e) {
        continue;
      }
      pgV[jstart].visited = 2;
      while (e) {
        e->visited = true;
        e->reverse->visited = true;
        tcg_pt* pg = pgV + e->to;
        if (pg->visited == 1) {
          done = false;
        } else if (pg->visited) {
          int k = pg->visited - 2;
          int max_dist = 0;
          int max_k = 0;
          tcg_edge* emax = nullptr;
          for (; k < _graph->_stackN; k++) {
            tcg_edge* eloop = _graph->_stackV[k];
            if (!eloop->s) {
              continue;
            }
            const int dist = abs(pt(eloop->fr)._x - pt(eloop->to)._x)
                             + abs(pt(eloop->fr)._y - pt(eloop->to)._y);
            if (dist >= max_dist) {
              max_dist = dist;
              max_k = k;
              emax = eloop;
            }
          }
          if (!emax) {
            done = false;
          } else {
            emax->skip = true;
            emax->reverse->skip = true;
            emax->s->_skip = true;
            loop_removed++;
            done = false;
            if (max_k + 1 < _graph->_stackN) {
              int k2;
              for (k2 = max_k + 1; k2 < _graph->_stackN - 1; k2++) {
                pgV[_graph->_stackV[k2]->to].visited = 1;
              }
              _graph->_stackN = max_k + 1;
            }
          }
        }
        if (pg->visited) {
          e = _graph->getNextEdge(false);
        } else {
          pg->visited = 2 + _graph->_stackN;
          e = _graph->getNextEdge(true);
        }
      }
    }
    if (!loop_removed) {
      break;
    }
  }

  // report all remaining loops, and count components
  _graph->clearVisited();
  for (int jstart = 0; jstart < _ptV.size(); jstart++) {
    tcg_edge* e = _graph->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      tcg_pt* pg = pgV + e->to;
      if (pg->visited) {
        e = _graph->getNextEdge(false);
      } else {
        pg->visited = 2 + _graph->_stackN;
        e = _graph->getNextEdge(true);
      }
    }
  }
}

void tmg_conn::dfsClear()
{
  _graph->clearVisited();
}

bool tmg_conn_graph::dfsStart(int& j)
{
  _e = getFirstNonShortEdge(j);
  if (!_e) {
    return false;
  }
  return true;
}

bool tmg_conn::dfsStart(int& j)
{
  return _graph->dfsStart(j);
}

bool tmg_conn_graph::dfsNext(int* from,
                             int* to,
                             int* k,
                             bool* is_short,
                             bool* is_loop)
{
  tcg_edge* e = _e;
  tcg_pt* pgV = _ptV;
  if (!e) {
    return false;
  }
  *from = e->fr;
  *to = e->to;
  *k = e->k;
  *is_short = (e->s ? true : false);
  e->visited = true;
  e->reverse->visited = true;
  pgV[e->fr].visited = 1;
  if (pgV[e->to].visited) {
    *is_loop = true;
    _e = getNextEdge(false);
  } else {
    *is_loop = false;
    pgV[e->to].visited = 1;
    _e = getNextEdge(true);
  }
  return true;
}

bool tmg_conn::dfsNext(int* from,
                       int* to,
                       int* k,
                       bool* is_short,
                       bool* is_loop)
{
  return _graph->dfsNext(from, to, k, is_short, is_loop);
}

int tmg_conn::isVisited(int j) const
{
  return _graph->pt(j).visited;
}

void tmg_conn::checkVisited()
{
  tcg_pt* pgV = _graph->_ptV;
  for (int j = 0; j < _ptV.size(); j++) {
    if (!pgV[j].visited) {
      _connected = false;
      break;
    }
  }
}

int tmg_conn::getDisconnectedStart()
{
  for (int j = 0; j < _ptV.size(); j++) {
    if (!_graph->pt(j).visited) {
      if (_graph->pt(j).edges && !_graph->pt(j).edges->next) {
        return j;
      }
    }
  }
  for (int j = 0; j < _ptV.size(); j++) {
    if (!_graph->pt(j).visited) {
      if (_graph->pt(j).edges) {
        return j;
      }
    }
  }
  return -1;
}

void tmg_conn::copyWireIdToVisitedShorts(const int j)
{
  // copy pt(j)._dbwire_id to visited points shorted to j
  const int wire_id = pt(j)._dbwire_id;
  tmg_rcpt* x0 = &pt(j);
  for (tmg_rcpt* x = x0->_sring; x && x != x0; x = x->_sring) {
    if (x->_dbwire_id < 0 && _graph->pt(x - _ptV.data()).visited) {
      x->_dbwire_id = wire_id;
    }
  }
}

}  // namespace odb
