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
    pt.visited = 0;
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
  // sibling
  // avoid resetting loop node
  if (pt(e->to).visited > pt(e->from).visited) {
    pt(e->to).visited = 1;
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
    pt(e->to).visited = 1;
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

void tmg_conn::relocateShorts()
{
  connection_graph_->relocateShorts(this);
}

void tmg_conn::removeShortLoops()
{
  if (!connection_graph_) {
    connection_graph_ = std::make_unique<ConnectionGraph>();
  }
  connection_graph_->init(wire_points_.size(), shorts_.size());
  std::vector<ConnectionGraph::Point>& pgV = connection_graph_->points_;

  // setup paths
  int npath = -1;
  for (size_t j = 0; j < wire_sections_.size(); j++) {
    if (j == 0 || wire_sections_[j].from_idx != wire_sections_[j - 1].to_idx) {
      ++npath;
    }
    pgV[wire_sections_[j].from_idx].path_index = npath;
    pgV[wire_sections_[j].to_idx].path_index = npath;
  }
  npath++;

  // remove shorts to same path
  for (Short& s : shorts_) {
    if (s.skip) {
      continue;
    }
    if (pgV[s.i0].path_index == pgV[s.i1].path_index) {
      s.skip = true;
    }
  }

  for (Short& s : shorts_) {
    if (s.skip) {
      continue;
    }
    ConnectionGraph::Edge* e;
    for (e = pgV[s.i0].first_edge; e; e = e->next) {
      if (e->to == s.i1) {
        break;
      }
    }
    if (e) {
      s.skip = true;
      continue;
    }
    e = connection_graph_->newShortEdge(this, s.i0, s.i1);
    ConnectionGraph::Edge* e2
        = connection_graph_->newShortEdge(this, s.i1, s.i0);
    e->wire_short = &s;
    e2->wire_short = &s;
    e->reverse = e2;
    e2->reverse = e;
    e->from = s.i0;
    e->to = s.i1;
    e2->from = s.i1;
    e2->to = s.i0;
    e->visited = false;
    e2->visited = false;
  }

  for (int j = 0; j < wire_points_.size(); j++) {
    pgV[j].visited = 0;
  }

  // remove all short loops
  connection_graph_->clearVisited();

  for (int jstart = 0; jstart < wire_points_.size(); jstart++) {
    ConnectionGraph::Edge* e = connection_graph_->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      ConnectionGraph::Point* pg = &pgV[e->to];
      if (pg->visited) {
        e->skip = true;
        e->reverse->skip = true;
        e->wire_short->skip = true;
        e = connection_graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + connection_graph_->descent_edges_.size();
        e = connection_graph_->getNextEdge(true);
      }
    }
  }

  // count components, and remaining loops
  connection_graph_->clearVisited();
  for (int jstart = 0; jstart < wire_points_.size(); jstart++) {
    ConnectionGraph::Edge* e = connection_graph_->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      ConnectionGraph::Point* pg = &pgV[e->to];
      if (pg->visited) {
        e = connection_graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + connection_graph_->descent_edges_.size();
        e = connection_graph_->getNextEdge(true);
      }
    }
  }
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

// Here we also build the connection graph.
void tmg_conn::removeWireLoops()
{
  removeShortLoops();

  // loops involving only shorts have already been handled
  if (wire_sections_.empty()) {
    return;
  }
  // add all path edges
  for (size_t j = 0; j < wire_sections_.size(); j++) {
    connection_graph_->addEdges(
        this, wire_sections_[j].from_idx, wire_sections_[j].to_idx, j);
  }

  // remove loops that have shorts by removing
  // the short with max distance
  // if no shorts, allow the loop to stay;
  // we do not expect any router to have a pure path loop

  std::vector<ConnectionGraph::Point>& pgV = connection_graph_->points_;

  bool done = false;
  while (!done) {
    int loop_removed = 0;
    done = true;
    connection_graph_->clearVisited();
    for (int jstart = 0; jstart < wire_points_.size(); jstart++) {
      ConnectionGraph::Edge* e = connection_graph_->getFirstEdge(jstart);
      if (!e) {
        continue;
      }
      pgV[jstart].visited = 2;
      while (e) {
        e->visited = true;
        e->reverse->visited = true;
        ConnectionGraph::Point* pg = &pgV[e->to];
        if (pg->visited == 1) {
          done = false;
        } else if (pg->visited) {
          int k = pg->visited - 2;
          int max_dist = 0;
          int max_k = 0;
          ConnectionGraph::Edge* emax = nullptr;
          for (; k < connection_graph_->descent_edges_.size(); k++) {
            ConnectionGraph::Edge* eloop = connection_graph_->descent_edges_[k];
            if (!eloop->wire_short) {
              continue;
            }
            const int dist
                = abs(wirePoint(eloop->from).x - wirePoint(eloop->to).x)
                  + abs(wirePoint(eloop->from).y - wirePoint(eloop->to).y);
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
            emax->wire_short->skip = true;
            loop_removed++;
            done = false;
            if (max_k + 1 < connection_graph_->descent_edges_.size()) {
              int k2;
              for (k2 = max_k + 1;
                   k2 < connection_graph_->descent_edges_.size() - 1;
                   k2++) {
                pgV[connection_graph_->descent_edges_[k2]->to].visited = 1;
              }
              connection_graph_->descent_edges_.resize(max_k + 1);
            }
          }
        }
        if (pg->visited) {
          e = connection_graph_->getNextEdge(false);
        } else {
          pg->visited = 2 + connection_graph_->descent_edges_.size();
          e = connection_graph_->getNextEdge(true);
        }
      }
    }
    if (!loop_removed) {
      break;
    }
  }

  // report all remaining loops, and count components
  connection_graph_->clearVisited();
  for (int jstart = 0; jstart < wire_points_.size(); jstart++) {
    ConnectionGraph::Edge* e = connection_graph_->getFirstEdge(jstart);
    if (!e) {
      continue;
    }
    pgV[jstart].visited = 2;
    while (e) {
      e->visited = true;
      e->reverse->visited = true;
      ConnectionGraph::Point* pg = &pgV[e->to];
      if (pg->visited) {
        e = connection_graph_->getNextEdge(false);
      } else {
        pg->visited = 2 + connection_graph_->descent_edges_.size();
        e = connection_graph_->getNextEdge(true);
      }
    }
  }
}

void tmg_conn::dfsClear()
{
  connection_graph_->clearVisited();
}

bool ConnectionGraph::dfsStart(int& j)
{
  next_edge_ = getFirstNonShortEdge(j);
  return next_edge_ != nullptr;
}

bool tmg_conn::dfsStart(int& j)
{
  return connection_graph_->dfsStart(j);
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
  pgV[e->from].visited = 1;
  if (pgV[e->to].visited) {
    *is_loop = true;
    next_edge_ = getNextEdge(false);
  } else {
    *is_loop = false;
    pgV[e->to].visited = 1;
    next_edge_ = getNextEdge(true);
  }
  return true;
}

bool tmg_conn::dfsNext(int* from,
                       int* to,
                       int* k,
                       bool* is_short,
                       bool* is_loop)
{
  return connection_graph_->dfsNext(from, to, k, is_short, is_loop);
}

int tmg_conn::isVisited(int j) const
{
  return connection_graph_->pt(j).visited;
}

void tmg_conn::checkVisited()
{
  std::vector<ConnectionGraph::Point>& pgV = connection_graph_->points_;
  for (int j = 0; j < wire_points_.size(); j++) {
    if (!pgV[j].visited) {
      connected_ = false;
      break;
    }
  }
}

int tmg_conn::getDisconnectedStart()
{
  for (int j = 0; j < wire_points_.size(); j++) {
    if (!connection_graph_->pt(j).visited) {
      if (connection_graph_->pt(j).first_edge
          && !connection_graph_->pt(j).first_edge->next) {
        return j;
      }
    }
  }
  for (int j = 0; j < wire_points_.size(); j++) {
    if (!connection_graph_->pt(j).visited) {
      if (connection_graph_->pt(j).first_edge) {
        return j;
      }
    }
  }
  return -1;
}

void tmg_conn::copyWireIdToVisitedShorts(const int j)
{
  // copy wirePoint(j)._dbwire_id to visited points shorted to j
  const int wire_id = wirePoint(j).dbwire_id;
  WirePoint* x0 = &wirePoint(j);
  for (WirePoint* x = x0->sring; x && x != x0; x = x->sring) {
    if (x->dbwire_id < 0
        && connection_graph_->pt(x - wire_points_.data()).visited) {
      x->dbwire_id = wire_id;
    }
  }
}

}  // namespace odb
