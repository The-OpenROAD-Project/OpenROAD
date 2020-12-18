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

#include <algorithm>
#include <list>
#include <vector>

#include "geom.h"

namespace odb {

class PolyDecomp
{
  enum Side
  {
    LEFT,
    RIGHT
  };

  class Node;

  struct Edge
  {
    Node* _src;
    Node* _tgt;
    Side  _side;
    Edge() {}
    bool inside_y(int y);
    bool contains_y(int y);
  };

  struct Node
  {
    Point _p;
    Edge*    _in_edge;
    Edge*    _out_edge;
    Node() {}
    int x() { return _p.getX(); };
    int y() { return _p.getY(); };
  };

  struct NodeCompare
  {
    bool operator()(Node* n0, Node* n1)
    {
      if (n0->y() < n1->y())
        return true;

      if (n0->y() > n1->y())
        return false;

      return n0->x() < n1->x();
    }
  };

  std::vector<Edge*> _edges;
  std::vector<Node*> _nodes;
  std::list<Edge*>   _active_edges;
  std::vector<Node*> _active_nodes;

  Node* new_node(Point p);
  Edge* new_edge(Node* src, Node* tgt, Side side);
  void  clear();
  void  add_edges(std::vector<Node*>::iterator& itr, int scanline);
  void  insert_edge(Edge* e, std::list<Edge*>::iterator& aeitr);
  void  scan_edges(int scanline, std::vector<Rect>& rects);
  void  purge_edges(int scanline);

 public:
  PolyDecomp();
  ~PolyDecomp();

  void decompose(const std::vector<Point>& points,
                 std::vector<Rect>&        rects);
};

inline bool PolyDecomp::Edge::contains_y(int y)
{
  int min_y = std::min(_src->y(), _tgt->y());
  int max_y = std::max(_src->y(), _tgt->y());
  return (min_y <= y) && (y <= max_y);
}

inline bool PolyDecomp::Edge::inside_y(int y)
{
  int min_y = std::min(_src->y(), _tgt->y());
  int max_y = std::max(_src->y(), _tgt->y());
  return (min_y < y) && (y < max_y);
}

PolyDecomp::PolyDecomp()
{
}

PolyDecomp::~PolyDecomp()
{
  clear();
}

void PolyDecomp::clear()
{
  std::vector<Edge*>::iterator eitr;

  for (eitr = _edges.begin(); eitr != _edges.end(); ++eitr)
    delete *eitr;

  std::vector<Node*>::iterator nitr;

  for (nitr = _nodes.begin(); nitr != _nodes.end(); ++nitr)
    delete *nitr;

  _edges.clear();
  _nodes.clear();
  _active_edges.clear();
  _active_nodes.clear();
}

PolyDecomp::Node* PolyDecomp::new_node(Point p)
{
  Node* n      = new Node();
  n->_p        = p;
  n->_in_edge  = NULL;
  n->_out_edge = NULL;
  _nodes.push_back(n);
  return n;
}

PolyDecomp::Edge* PolyDecomp::new_edge(Node* src, Node* tgt, Side side)
{
  Edge* e  = new Edge();
  e->_side = side;
  e->_src  = src;
  e->_tgt  = tgt;
  assert(src->_out_edge == NULL);
  src->_out_edge = e;
  assert(tgt->_in_edge == NULL);
  tgt->_in_edge = e;
  _edges.push_back(e);
  return e;
}

// assumes polygon points are in clockwise order
void PolyDecomp::decompose(const std::vector<Point>& points,
                           std::vector<Rect>&        rects)
{
  if (points.size() < 4)
    return;

  Node* u = new_node(points[0]);
  Node* w = u;
  _active_nodes.push_back(u);

  int n = points.size();

  // Create vertical edges for scanline intersection
  for (int i = 1; i < n; ++i) {
    Node* v = new_node(points[i]);
    _active_nodes.push_back(v);

    if (u->y() < v->y())
      new_edge(u, v, LEFT);

    else if (u->y() > v->y())
      new_edge(u, v, RIGHT);

    u = v;
  }

  if (u->y() < w->y())
    new_edge(u, w, LEFT);

  else if (u->y() > w->y())
    new_edge(u, w, RIGHT);

  std::sort(_active_nodes.begin(), _active_nodes.end(), NodeCompare());
  std::vector<Node*>::iterator itr      = _active_nodes.begin();
  int                          scanline = (*itr)->y();

  for (;;) {
    add_edges(itr, scanline);
    scan_edges(scanline, rects);

    if (itr == _active_nodes.end())
      break;

    ++itr;
    scanline = (*itr)->y();
    purge_edges(scanline);
  }

  clear();
}

void PolyDecomp::add_edges(std::vector<Node*>::iterator& itr, int scanline)
{
  std::list<Edge*>::iterator aeitr = _active_edges.begin();

  for (; itr != _active_nodes.end(); ++itr) {
    Node* n = *itr;

    if (n->y() != scanline) {
      --itr;
      break;
    }

    if (n->_in_edge)
      insert_edge(n->_in_edge, aeitr);

    if (n->_out_edge)
      insert_edge(n->_out_edge, aeitr);
  }
}

void PolyDecomp::insert_edge(Edge* e, std::list<Edge*>::iterator& aeitr)
{
  int x = e->_src->x();

  for (; aeitr != _active_edges.end(); ++aeitr) {
    Edge* ae = *aeitr;

    if (x < ae->_src->x()) {
      _active_edges.insert(aeitr, e);
      return;
    }
  }

  _active_edges.insert(aeitr, e);
}

void PolyDecomp::scan_edges(int scanline, std::vector<Rect>& rects)
{
  std::list<Edge*>::iterator itr = _active_edges.begin();
  std::list<Edge*>::iterator left_itr;
  std::list<Edge*>::iterator right_itr;

  for (;; ++itr) {
    Edge* left;

    for (;; ++itr) {
      if (itr == _active_edges.end())
        return;

      left = *itr;

      if ((left->_side == LEFT) && (left->_src->y() != scanline)) {
        left_itr = itr;
        break;
      }
    }

    Edge* right;

    for (++itr;; ++itr) {
      if (itr == _active_edges.end())
        return;

      right = *itr;

      if ((right->_side == RIGHT) && (right->_tgt->y() != scanline)) {
        right_itr = itr;
        break;
      }
    }

    if (left->inside_y(scanline) && right->inside_y(scanline))
      if ((++left_itr) == right_itr)
        continue;

    if (left->inside_y(scanline))  // split intersected edge
    {
      Node* u      = left->_src;
      Node* v      = new_node(Point(u->x(), scanline));
      u->_out_edge = NULL;
      v->_out_edge = left;
      left->_src   = v;
      left         = new_edge(u, v, LEFT);
    }

    if (right->inside_y(scanline))  // split intersected edge
    {
      Node* w     = right->_tgt;
      Node* v     = new_node(Point(w->x(), scanline));
      right->_tgt = v;
      v->_in_edge = right;
      w->_in_edge = NULL;
      right       = new_edge(v, w, RIGHT);
    }

    Rect r(left->_src->_p, right->_src->_p);
    rects.push_back(r);
  }
}

void PolyDecomp::purge_edges(int scanline)
{
  std::list<Edge*>::iterator itr = _active_edges.begin();

  while (itr != _active_edges.end()) {
    Edge* e = *itr;
    if (e->contains_y(scanline))
      ++itr;
    else
      itr = _active_edges.erase(itr);
  }
}

void decompose_polygon(const std::vector<Point>& points,
                       std::vector<Rect>&        rects)
{
  PolyDecomp decomp;
  decomp.decompose(points, rects);
}

// See "Orientation of a simple polygon" in
// https://en.wikipedia.org/wiki/Curve_orientation
// The a, b, c point names are used to match the wiki page
bool polygon_is_clockwise(const std::vector<Point>& P)
{
  if (P.size() < 3)
    return false;

  int n = P.size();

  // find a point on the convex hull of the polygon
  // Here we use the lowest-most in Y with lowest in X as a tie breaker
  int yMin = std::numeric_limits<int>::max();
  int xMin = std::numeric_limits<int>::max();
  int b = 0;  // the index of the point we are seeking
  for (int i = 0; i < n; ++i) {
    int x = P[i].x();
    int y = P[i].y();
    if (y < yMin || (y == yMin && x < xMin)) {
      b    = i;
      yMin = y;
      xMin = x;
    }
  }

  int a = b > 0 ? b - 1 : n - 1;  // previous pt to b
  int c = b < n - 1 ? b + 1 : 0;  // next pt to b

  double xa = P[a].getX();
  double ya = P[a].getY();

  double xb = P[b].getX();
  double yb = P[b].getY();

  double xc = P[c].getX();
  double yc = P[c].getY();

  double det = (xb * yc + xa * yb + ya * xc) - (ya * xb + yb * xc + xa * yc);
  return det < 0.0;
}

}  // namespace odb
