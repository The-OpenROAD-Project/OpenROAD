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

#include <vector>

#include "odbDList.h"
#include "dbRtEdge.h"
#include "geom.h"
#include "odb.h"

namespace odb {

class dbObject;
class dbTechLayer;
class dbRtTree;
class dbRtEdge;
class dbWireEncoder;

class dbRtNodeEdgeIterator
{
  dbRtNode* _node;
  dbRtEdge* _cur;

  void incr() { _cur = _cur->next(_node); }

 public:
  dbRtNodeEdgeIterator()
  {
    _node = NULL;
    _cur  = NULL;
  }
  dbRtNodeEdgeIterator(dbRtNode* node, dbRtEdge* cur)
  {
    _node = node;
    _cur  = cur;
  }
  dbRtNodeEdgeIterator(const dbRtNodeEdgeIterator& i)
  {
    _node = i._node;
    _cur  = i._cur;
  }
  dbRtNodeEdgeIterator& operator=(const dbRtNodeEdgeIterator& i)
  {
    _node = i._node;
    _cur  = i._cur;
    return *this;
  }

  bool operator==(const dbRtNodeEdgeIterator& i) const
  {
    return _cur == i._cur;
  }
  bool operator!=(const dbRtNodeEdgeIterator& i) const
  {
    return _cur != i._cur;
  }
  dbRtEdge*             operator*() { return _cur; }
  dbRtNodeEdgeIterator& operator++()
  {
    incr();
    return *this;
  }
  dbRtNodeEdgeIterator operator++(int)
  {
    dbRtNodeEdgeIterator i = *this;
    incr();
    return i;
  }
};

// Node - A Node represents a physical point in the graph.
class dbRtNode
{
 private:
  int                    _x;
  int                    _y;
  int                    _jct_id;
  dbTechLayer*           _layer;
  std::vector<dbObject*> _objects;
  dbRtTree*              _rt_tree;
  dbRtEdge*              _head;
  dbRtEdge*              _tail;
  DListEntry<dbRtNode>   _rt_node;
  bool                   _visited;

  void add_edge(dbRtEdge* edge)
  {
    assert(edge->_src == this || edge->_tgt == this);

    if (_head == NULL) {
      _head = _tail    = edge;
      edge->next(this) = NULL;
      edge->prev(this) = NULL;
    } else {
      edge->prev(this)  = _tail;
      _tail->next(this) = edge;
      edge->next(this)  = NULL;
      _tail             = edge;
    }
  }

  void remove_edge(dbRtEdge* edge)
  {
    dbRtEdge* n = edge->next(this);
    dbRtEdge* p = edge->prev(this);

    if (p)
      p->next(this) = n;
    else
      _head = n;

    if (n)
      n->prev(this) = p;
    else
      _tail = p;
  }

 public:
  typedef dbRtNodeEdgeIterator             edge_iterator;
  typedef std::vector<dbObject*>::iterator object_iterator;

  dbRtNode(int x, int y, dbTechLayer* layer)
      : _x(x),
        _y(y),
        _jct_id(-1),
        _layer(layer),
        _rt_tree(NULL),
        _head(NULL),
        _tail(NULL),
        _visited(false)
  {
  }

  // Set the x/y location of this node
  void setPoint(int x, int y)
  {
    _x = x;
    _y = y;
  }

  // Set the x/y location of this node
  void setPoint(Point& p)
  {
    _x = p.x();
    _y = p.y();
  }

  // Set the x/y location of this node
  void getPoint(int& x, int& y) const
  {
    x = _x;
    y = _y;
  }

  // Set the x/y location of this node
  void getPoint(Point& p) const { p = Point(_x, _y); }

  // Get the x-coordinate of this point
  int x() const { return _x; }

  // Get the x-coordinate of this point
  int y() const { return _y; }

  // Set the layer of this node
  void setLayer(dbTechLayer* layer) { _layer = layer; }

  // Get the layer of this node
  dbTechLayer* getLayer() const { return _layer; }

  // begin iterating edges of this node
  edge_iterator begin() { return edge_iterator(this, _head); }

  // end iterating edges of this node
  edge_iterator end() { return edge_iterator(this, NULL); }

  // Add database object to this node
  void addObject(dbObject* object) { _objects.push_back(object); }

  // Get the database object assigned to this node
  void getObjects(std::vector<dbObject*>& objects) const { objects = _objects; }

  // Get the value of the visited flag.
  bool isVisited() const { return _visited; }

  // Set the value of the visited flag.
  void setVisited(bool value) { _visited = value; }

  // returns true if this node has no edges
  bool isOrphan() const { return _head == NULL; }

  // returns true if this node is not an orphan and has a single edge
  bool isLeaf() const { return (_head != NULL) && (_head == _tail); }

  object_iterator begin_objects() { return _objects.begin(); }
  object_iterator end_objects() { return _objects.end(); }

  static DListEntry<dbRtNode>* rtNode(dbRtNode* node)
  {
    return &node->_rt_node;
  }

  friend class dbRtTree;
  friend class dbRtEdge;
  friend class dbRtSegment;
  friend class dbRtVia;
  friend class dbRtTechVia;
  friend class dbRtShort;
  friend class dbRtVWire;
};

}  // namespace odb
