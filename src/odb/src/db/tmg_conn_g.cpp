// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tmg_conn_g.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "tmg_conn.h"

namespace odb {

void ConnectionGraph::init(const int ptN, const int shortN)
{
  points_.resize(ptN);
  for (ConnectionGraph::Point& pt : points_) {
    pt.first_edge = nullptr;
  }

  descent_edges_.clear();
  descent_edges_.reserve(2ul * shortN);

  edges_.clear();
}

ConnectionGraph::Edge* ConnectionGraph::newEdge(const tmg_conn* conn,
                                                const int fr,
                                                const int to)
{
  ConnectionGraph::Edge* e = &edges_.emplace_back();
  e->wire_section_index = -1;
  e->skip = false;
  const int ndx = conn->wirePoint(to).x;
  const int ndy = conn->wirePoint(to).y;
  ConnectionGraph::Edge* prev_edge = nullptr;
  ConnectionGraph::Edge* edge = points_[fr].first_edge;
  while (edge && !edge->wire_short && ndx > conn->wirePoint(edge->to).x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && !edge->wire_short && ndx == conn->wirePoint(edge->to).x
         && ndy > conn->wirePoint(edge->to).y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    points_[fr].first_edge = e;
  }
  return e;
}

ConnectionGraph::Edge* ConnectionGraph::newShortEdge(const tmg_conn* conn,
                                                     const int fr,
                                                     const int to)
{
  ConnectionGraph::Edge* e = &edges_.emplace_back();
  e->wire_section_index = -1;
  e->skip = false;
  const int ned = conn->distance(fr, to);
  const int ndx = conn->wirePoint(to).x;
  const int ndy = conn->wirePoint(to).y;
  ConnectionGraph::Edge* prev_edge = nullptr;
  ConnectionGraph::Edge* edge = points_[fr].first_edge;
  while (edge && ned > conn->distance(edge->from, edge->to)) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->distance(edge->from, edge->to)
         && ndx > conn->wirePoint(edge->to).x) {
    prev_edge = edge;
    edge = edge->next;
  }
  while (edge && ned == conn->distance(edge->from, edge->to)
         && ndx == conn->wirePoint(edge->to).x
         && ndy > conn->wirePoint(edge->to).y) {
    prev_edge = edge;
    edge = edge->next;
  }
  e->next = edge;
  if (prev_edge) {
    prev_edge->next = e;
  } else {
    points_[fr].first_edge = e;
  }
  return e;
}

void ConnectionGraph::clearVisited()
{
  for (ConnectionGraph::Edge& edge : edges_) {
    edge.visited = false;
  }
  for (ConnectionGraph::Point& pt : points_) {
    pt.visited = false;
    pt.descent_edge_index = -1;
  }
}

void ConnectionGraph::getEdgeRefCoord(const tmg_conn* conn,
                                      ConnectionGraph::Edge* pe,
                                      int& rx,
                                      int& ry)
{
  rx = conn->wirePoint(pe->to).x;
  ry = conn->wirePoint(pe->to).y;
  if (pe->wire_short == nullptr) {
    return;
  }
  ConnectionGraph::Edge* se = pt(pe->to).first_edge;
  while (se && se->wire_short) {
    se = se->next;
  }
  if (se == nullptr) {
    return;
  }
  rx = conn->wirePoint(se->to).x;
  ry = conn->wirePoint(se->to).y;
}

bool ConnectionGraph::isBadShort(ConnectionGraph::Edge* pe,
                                 const tmg_conn* conn)
{
  if (pe->wire_short == nullptr) {
    return false;
  }
  const WirePoint& from = conn->wirePoint(pe->from);
  const WirePoint& to = conn->wirePoint(pe->to);
  return from.x != to.x || from.y != to.y;
}

void ConnectionGraph::relocateShorts(tmg_conn* conn)
{
  for (ConnectionGraph::Point& pt : points_) {
    ConnectionGraph::Edge* pe = pt.first_edge;
    if (pe == nullptr || pe->next == nullptr) {
      continue;
    }
    bool needAdjust = true;
    while (needAdjust) {
      needAdjust = false;
      int r1x, r1y;
      bool firstCheck = true;
      ConnectionGraph::Edge* pppe = nullptr;
      ConnectionGraph::Edge* ppe = nullptr;
      for (pe = pt.first_edge; pe != nullptr; pe = pe->next) {
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
        if ((pe->wire_short == nullptr && ppe->wire_short == nullptr)
            || isBadShort(pe, conn) || isBadShort(ppe, conn)) {
          pppe = ppe;
          ppe = pe;
          r1x = r2x;
          r1y = r2y;
          continue;
        }
        if (r1x > r2x || (r1x == r2x && r1y > r2y)) {
          needAdjust = true;
          ConnectionGraph::Edge* last = pe->next;
          if (pppe) {
            pppe->next = pe;
          } else {
            pt.first_edge = pe;
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
  for (ConnectionGraph::Point& pt : points_) {
    ConnectionGraph::Edge* skipe = nullptr;
    int noshortn = 0;
    int shortn = 0;
    ConnectionGraph::Edge* plast = nullptr;
    ConnectionGraph::Edge* last = nullptr;
    for (ConnectionGraph::Edge* pe = pt.first_edge; pe != nullptr;
         pe = pe->next) {
      if (!pe->wire_short) {
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
    ConnectionGraph::Edge* nse = points_[plast->to].first_edge;
    while (nse != nullptr && nse->to != last->to) {
      nse = nse->next;
    }
    if (nse && nse->wire_short) {
      nse->wire_short->skip = true;
      nse->skip = true;
      nse->reverse->skip = true;
    } else {
      return;
    }
    // unskip skipe
    skipe->wire_short->skip = false;
    skipe->skip = false;
    skipe->reverse->skip = false;
  }
}

ConnectionGraph::Edge* ConnectionGraph::getFirstNonShortEdge(int& jstart)
{
  if (pt(jstart).visited || !pt(jstart).first_edge) {
    return nullptr;
  }
  ConnectionGraph::Edge* e = pt(jstart).first_edge;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (!e) {
    return nullptr;
  }
  int loops = 16;
  while (loops && e->wire_short) {
    jstart
        = jstart == e->wire_short->i0 ? e->wire_short->i1 : e->wire_short->i0;
    e = pt(jstart).first_edge;
    loops--;
  }
  if (loops == 0) {
    e = nullptr;
  }
  descent_edges_.clear();
  if (!e) {
    return nullptr;
  }
  descent_edges_.push_back(e);
  return e;
}

ConnectionGraph::Edge* ConnectionGraph::getFirstEdge(const int jstart)
{
  if (pt(jstart).visited || !pt(jstart).first_edge) {
    return nullptr;
  }
  ConnectionGraph::Edge* e = pt(jstart).first_edge;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (!e) {
    return nullptr;
  }
  descent_edges_.emplace_back(e);
  return e;
}

ConnectionGraph::Edge* ConnectionGraph::getNextEdge(const bool ok_to_descend)
{
  ConnectionGraph::Edge* e = descent_edges_.back();

  if (ok_to_descend) {
    ConnectionGraph::Edge* e2 = pt(e->to).first_edge;
    while (e2 && (e2->visited || e2->skip)) {
      e2 = e2->next;
    }
    if (e2) {
      descent_edges_.emplace_back(e2);
      return e2;
    }
  }

  // A higher index means that the point is deeper in the descent.
  // Note that if any of the points is not on the descent i.e.,
  // index = -1, the check still holds.
  if (pt(e->to).descent_edge_index > pt(e->from).descent_edge_index) {
    pt(e->to).descent_edge_index = -1;
  }

  e = e->next;
  while (e && (e->visited || e->skip)) {
    e = e->next;
  }
  if (e) {
    descent_edges_.back() = e;
    return e;
  }
  // ascend
  descent_edges_.pop_back();
  while (!descent_edges_.empty()) {
    e = descent_edges_.back();
    pt(e->to).descent_edge_index = -1;
    e = e->next;
    while (e && (e->visited || e->skip)) {
      e = e->next;
    }
    if (e) {
      descent_edges_.back() = e;
      return e;
    }
    descent_edges_.pop_back();
  }
  return nullptr;
}

void ConnectionGraph::addEdges(const tmg_conn* conn,
                               const int i0,
                               const int i1,
                               const int k)
{
  ConnectionGraph::Edge* e = newEdge(conn, i0, i1);
  ConnectionGraph::Edge* e2 = newEdge(conn, i1, i0);

  e->wire_short = nullptr;
  e->reverse = e2;
  e->from = i0;
  e->to = i1;
  e->wire_section_index = k;
  e->visited = false;
  e->skip = false;

  e2->wire_short = nullptr;
  e2->reverse = e;
  e2->from = i1;
  e2->to = i0;
  e2->wire_section_index = k;
  e2->visited = false;
  e2->skip = false;
}

bool ConnectionGraph::dfsStart(int& j)
{
  next_edge_ = getFirstNonShortEdge(j);
  return next_edge_ != nullptr;
}

bool ConnectionGraph::dfsNext(int* from,
                              int* to,
                              int* k,
                              bool* is_short,
                              bool* is_loop)
{
  ConnectionGraph::Edge* e = next_edge_;
  std::vector<ConnectionGraph::Point>& pgV = points_;
  if (!e) {
    return false;
  }
  *from = e->from;
  *to = e->to;
  *k = e->wire_section_index;
  *is_short = (e->wire_short != nullptr);
  e->visited = true;
  e->reverse->visited = true;
  pgV[e->from].visited = true;
  if (pgV[e->to].visited) {
    *is_loop = true;
    next_edge_ = getNextEdge(false);
  } else {
    *is_loop = false;
    pgV[e->to].visited = true;
    next_edge_ = getNextEdge(true);
  }
  return true;
}

ConnectionGraph::Edge* ConnectionGraph::descentEdge(const int edge_index) const
{
  return descent_edges_[edge_index];
}

}  // namespace odb
