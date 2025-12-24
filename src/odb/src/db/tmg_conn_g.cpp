// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tmg_conn_g.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "dbCommon.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "tmg_conn.h"
#include "utl/Logger.h"

namespace odb {

tmg_conn_graph::tmg_conn_graph()
{
  ptNmax_ = 1024;
  shortNmax_ = 1024;
  eNmax_ = 1024;
  ptV_ = (tcg_pt*) safe_malloc(ptNmax_ * sizeof(tcg_pt));
  path_vis_ = (int*) safe_malloc(ptNmax_ * sizeof(int));
  eV_ = (tcg_edge*) safe_malloc(2UL * ptNmax_ * sizeof(tcg_edge));
  stackV_ = (tcg_edge**) safe_malloc(shortNmax_ * sizeof(tcg_edge*));
}

tmg_conn_graph::~tmg_conn_graph()
{
  free(ptV_);
  free(path_vis_);
  free(eV_);
  free(static_cast<void*>(stackV_));
}

void tmg_conn_graph::init(const int ptN, const int shortN)
{
  if (ptN > ptNmax_) {
    ptNmax_ = 2 * ptN;
    free(ptV_);
    ptV_ = (tcg_pt*) safe_malloc(ptNmax_ * sizeof(tcg_pt));
    free(path_vis_);
    path_vis_ = (int*) safe_malloc(ptNmax_ * sizeof(int));
  }
  if (shortN > shortNmax_) {
    shortNmax_ = 2 * shortN;
    free(static_cast<void*>(stackV_));
    stackV_ = (tcg_edge**) safe_malloc(shortNmax_ * sizeof(tcg_edge*));
  }
  if (4 * ptN + 2 * shortN > eNmax_) {
    eNmax_ *= 2;
    eNmax_ = std::max((4 * ptN) + (2 * shortN), eNmax_);
    free(eV_);
    eV_ = (tcg_edge*) safe_malloc(eNmax_ * sizeof(tcg_edge));
  }
  eN_ = 0;
  for (int j = 0; j < ptN; j++) {
    ptV_[j].edges = nullptr;
  }
  ptN_ = ptN;
}

tcg_edge* tmg_conn_graph::newEdge(const tmg_conn* conn,
                                  const int fr,
                                  const int to)
{
  tcg_edge* e = eV_ + eN_++;
  e->k = -1;
  e->skip = false;
  const int ndx = conn->pt(to).x;
  const int ndy = conn->pt(to).y;
  tcg_edge* prev_edge = nullptr;
  tcg_edge* edge = ptV_[fr].edges;
  while (edge && !edge->s && ndx > conn->pt(edge->to).x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && !edge->s && ndx == conn->pt(edge->to).x
         && ndy > conn->pt(edge->to).y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    ptV_[fr].edges = e;
  }
  return e;
}

tcg_edge* tmg_conn_graph::newShortEdge(const tmg_conn* conn,
                                       const int fr,
                                       const int to)
{
  tcg_edge* e = eV_ + eN_++;
  e->k = -1;
  e->skip = false;
  const int ned = conn->ptDist(fr, to);
  const int ndx = conn->pt(to).x;
  const int ndy = conn->pt(to).y;
  tcg_edge* prev_edge = nullptr;
  tcg_edge* edge = ptV_[fr].edges;
  while (edge && ned > conn->ptDist(edge->fr, edge->to)) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->ptDist(edge->fr, edge->to)
         && ndx > conn->pt(edge->to).x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->ptDist(edge->fr, edge->to)
         && ndx == conn->pt(edge->to).x && ndy > conn->pt(edge->to).y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    ptV_[fr].edges = e;
  }
  return e;
}

void tmg_conn_graph::clearVisited()
{
  for (int j = 0; j < eN_; j++) {
    eV_[j].visited = false;
  }
  for (int j = 0; j < ptN_; j++) {
    ptV_[j].visited = 0;
  }
}

void tmg_conn_graph::getEdgeRefCoord(const tmg_conn* conn,
                                     tcg_edge* pe,
                                     int& rx,
                                     int& ry)
{
  rx = conn->pt(pe->to).x;
  ry = conn->pt(pe->to).y;
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
  rx = conn->pt(se->to).x;
  ry = conn->pt(se->to).y;
}

bool tmg_conn_graph::isBadShort(tcg_edge* pe, const tmg_conn* conn)
{
  if (pe->s == nullptr) {
    return false;
  }
  const tmg_rcpt& from = conn->pt(pe->fr);
  const tmg_rcpt& to = conn->pt(pe->to);
  return from.x != to.x || from.y != to.y;
}

void tmg_conn_graph::relocateShorts(tmg_conn* conn)
{
  for (int jp = 0; jp < ptN_; jp++) {
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
  for (int jp = 0; jp < ptN_; jp++) {
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
      nse->s->skip = true;
      nse->skip = true;
      nse->reverse->skip = true;
    } else {
      return;
    }
    // unskip skipe
    skipe->s->skip = false;
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
  int loops = 16;
  while (loops && e->s) {
    jstart = jstart == e->s->i0 ? e->s->i1 : e->s->i0;
    e = pt(jstart).edges;
    loops--;
  }
  if (loops == 0) {
    e = nullptr;
  }
  if (!e) {
    stackV_[0] = nullptr;
    stackN_ = 0;
    return nullptr;
  }
  stackV_[0] = e;
  stackN_ = 1;
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
  stackV_[0] = e;
  stackN_ = 1;
  return e;
}

tcg_edge* tmg_conn_graph::getNextEdge(const bool ok_to_descend)
{
  tcg_edge* e = stackV_[stackN_ - 1];

  if (ok_to_descend) {
    tcg_edge* e2 = pt(e->to).edges;
    while (e2 && (e2->visited || e2->skip)) {
      e2 = e2->next;
    }
    if (e2) {
      if (stackN_ >= shortNmax_) {
        shortNmax_ = shortNmax_ * 2;
        stackV_ = (tcg_edge**) realloc(static_cast<void*>(stackV_),
                                       shortNmax_ * sizeof(tcg_edge*));
      }
      stackV_[stackN_++] = e2;
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
    stackV_[stackN_ - 1] = e;
    return e;
  }
  // ascend
  while (--stackN_ > 0) {
    e = stackV_[stackN_ - 1];
    pt(e->to).visited = 1;
    e = e->next;
    while (e && (e->visited || e->skip)) {
      e = e->next;
    }
    if (e) {
      stackV_[stackN_ - 1] = e;
      return e;
    }
  }
  return nullptr;
}

void tmg_conn::relocateShorts()
{
  graph_->relocateShorts(this);
}

void tmg_conn::removeShortLoops()
{
  if (!graph_) {
    graph_ = std::make_unique<tmg_conn_graph>();
  }
  graph_->init(ptV_.size(), shortV_.size());
  tcg_pt* pgV = graph_->ptV_;

  // setup paths
  int npath = -1;
  for (size_t j = 0; j < rcV_.size(); j++) {
    if (j == 0 || rcV_[j].from_idx != rcV_[j - 1].to_idx) {
      ++npath;
    }
    pgV[rcV_[j].from_idx].ipath = npath;
    pgV[rcV_[j].to_idx].ipath = npath;
  }
  npath++;

  // remove shorts to same path
  for (tmg_rcshort& s : shortV_) {
    if (s.skip) {
      continue;
    }
    if (pgV[s.i0].ipath == pgV[s.i1].ipath) {
      s.skip = true;
    }
  }

  for (tmg_rcshort& s : shortV_) {
    if (s.skip) {
      continue;
    }
    tcg_edge* e;
    for (e = pgV[s.i0].edges; e; e = e->next) {
      if (e->to == s.i1) {
        break;
      }
    }
    if (e) {
      s.skip = true;
      continue;
    }
    e = graph_->newShortEdge(this, s.i0, s.i1);
    tcg_edge* e2 = graph_->newShortEdge(this, s.i1, s.i0);
    e->s = &s;
    e2->s = &s;
    e->reverse = e2;
    e2->reverse = e;
    e->fr = s.i0;
    e->to = s.i1;
    e2->fr = s.i1;
    e2->to = s.i0;
    e->visited = false;
    e2->visited = false;
  }

  for (int j = 0; j < ptV_.size(); j++) {
    pgV[j].visited = 0;
  }

  // remove all short loops
  int* path_vis = graph_->path_vis_;
  graph_->clearVisited();
  for (int j = 0; j < npath; j++) {
    path_vis[j] = 0;
  }

  for (int jstart = 0; jstart < ptV_.size(); jstart++) {
    tcg_edge* e = graph_->getFirstEdge(jstart);
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
        e->s->skip = true;
        e = graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + graph_->stackN_;
        e = graph_->getNextEdge(true);
      }
    }
  }

  // count components, and remaining loops
  graph_->clearVisited();
  for (int jstart = 0; jstart < ptV_.size(); jstart++) {
    tcg_edge* e = graph_->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      tcg_pt* pg = pgV + e->to;
      if (pg->visited) {
        e = graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + graph_->stackN_;
        e = graph_->getNextEdge(true);
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
  if (rcV_.empty()) {
    return;
  }
  // add all path edges
  for (size_t j = 0; j < rcV_.size(); j++) {
    graph_->addEdges(this, rcV_[j].from_idx, rcV_[j].to_idx, j);
  }

  // remove loops that have shorts by removing
  // the short with max distance
  // if no shorts, allow the loop to stay;
  // we do not expect any router to have a pure path loop

  tcg_pt* pgV = graph_->ptV_;

  bool done = false;
  while (!done) {
    int loop_removed = 0;
    done = true;
    graph_->clearVisited();
    for (int jstart = 0; jstart < ptV_.size(); jstart++) {
      tcg_edge* e = graph_->getFirstEdge(jstart);
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
          for (; k < graph_->stackN_; k++) {
            tcg_edge* eloop = graph_->stackV_[k];
            if (!eloop->s) {
              continue;
            }
            const int dist = abs(pt(eloop->fr).x - pt(eloop->to).x)
                             + abs(pt(eloop->fr).y - pt(eloop->to).y);
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
            emax->s->skip = true;
            loop_removed++;
            done = false;
            if (max_k + 1 < graph_->stackN_) {
              int k2;
              for (k2 = max_k + 1; k2 < graph_->stackN_ - 1; k2++) {
                pgV[graph_->stackV_[k2]->to].visited = 1;
              }
              graph_->stackN_ = max_k + 1;
            }
          }
        }
        if (pg->visited) {
          e = graph_->getNextEdge(false);
        } else {
          pg->visited = 2 + graph_->stackN_;
          e = graph_->getNextEdge(true);
        }
      }
    }
    if (!loop_removed) {
      break;
    }
  }

  // report all remaining loops, and count components
  graph_->clearVisited();
  for (int jstart = 0; jstart < ptV_.size(); jstart++) {
    tcg_edge* e = graph_->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      tcg_pt* pg = pgV + e->to;
      if (pg->visited) {
        e = graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + graph_->stackN_;
        e = graph_->getNextEdge(true);
      }
    }
  }
}

void tmg_conn::dfsClear()
{
  graph_->clearVisited();
}

bool tmg_conn_graph::dfsStart(int& j)
{
  e_ = getFirstNonShortEdge(j);
  if (!e_) {
    return false;
  }
  return true;
}

bool tmg_conn::dfsStart(int& j)
{
  return graph_->dfsStart(j);
}

bool tmg_conn_graph::dfsNext(int* from,
                             int* to,
                             int* k,
                             bool* is_short,
                             bool* is_loop)
{
  tcg_edge* e = e_;
  tcg_pt* pgV = ptV_;
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
    e_ = getNextEdge(false);
  } else {
    *is_loop = false;
    pgV[e->to].visited = 1;
    e_ = getNextEdge(true);
  }
  return true;
}

bool tmg_conn::dfsNext(int* from,
                       int* to,
                       int* k,
                       bool* is_short,
                       bool* is_loop)
{
  return graph_->dfsNext(from, to, k, is_short, is_loop);
}

int tmg_conn::isVisited(int j) const
{
  return graph_->pt(j).visited;
}

void tmg_conn::checkVisited()
{
  tcg_pt* pgV = graph_->ptV_;
  for (int j = 0; j < ptV_.size(); j++) {
    if (!pgV[j].visited) {
      connected_ = false;
      break;
    }
  }
}

int tmg_conn::getDisconnectedStart()
{
  for (int j = 0; j < ptV_.size(); j++) {
    if (!graph_->pt(j).visited) {
      if (graph_->pt(j).edges && !graph_->pt(j).edges->next) {
        return j;
      }
    }
  }
  for (int j = 0; j < ptV_.size(); j++) {
    if (!graph_->pt(j).visited) {
      if (graph_->pt(j).edges) {
        return j;
      }
    }
  }
  return -1;
}

void tmg_conn::copyWireIdToVisitedShorts(const int j)
{
  // copy pt(j)._dbwire_id to visited points shorted to j
  const int wire_id = pt(j).dbwire_id;
  tmg_rcpt* x0 = &pt(j);
  for (tmg_rcpt* x = x0->sring; x && x != x0; x = x->sring) {
    if (x->dbwire_id < 0 && graph_->pt(x - ptV_.data()).visited) {
      x->dbwire_id = wire_id;
    }
  }
}

}  // namespace odb
