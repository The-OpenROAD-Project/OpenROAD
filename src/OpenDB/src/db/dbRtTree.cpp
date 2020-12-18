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

#include "dbRtTree.h"

#include "db.h"
#include "dbWire.h"
#include "dbWireCodec.h"
#include "odbAllocator.h"

namespace odb {

// object allocators
static Allocator<dbRtNode>    g_node_alloc;
static Allocator<dbRtSegment> g_segment_alloc;
static Allocator<dbRtTechVia> g_tech_via_alloc;
static Allocator<dbRtVia>     g_via_alloc;
static Allocator<dbRtShort>   g_short_alloc;
static Allocator<dbRtVWire>   g_vwire_alloc;

// object "edge" destroyers
class ObjectDestroy
{
 public:
  virtual void destroyObject(void* obj) = 0;
};

class DestroySegment : public ObjectDestroy
{
 public:
  void destroyObject(void* obj) { g_segment_alloc.destroy((dbRtSegment*) obj); }

  static DestroySegment singleton;
};

class DestroyTechVia : public ObjectDestroy
{
 public:
  void destroyObject(void* obj)
  {
    g_tech_via_alloc.destroy((dbRtTechVia*) obj);
  }

  static DestroyTechVia singleton;
};

class DestroyVia : public ObjectDestroy
{
 public:
  void destroyObject(void* obj) { g_via_alloc.destroy((dbRtVia*) obj); }

  static DestroyVia singleton;
};

class DestroyShort : public ObjectDestroy
{
 public:
  void destroyObject(void* obj) { g_short_alloc.destroy((dbRtShort*) obj); }

  static DestroyShort singleton;
};

class DestroyVWire : public ObjectDestroy
{
 public:
  void destroyObject(void* obj) { g_vwire_alloc.destroy((dbRtVWire*) obj); }

  static DestroyVWire singleton;
};

DestroySegment DestroySegment::singleton;
DestroyTechVia DestroyTechVia::singleton;
DestroyVia     DestroyVia::singleton;
DestroyShort   DestroyShort::singleton;
DestroyVWire   DestroyVWire::singleton;

// allocators
static ObjectDestroy* g_edge_destroyers[5] = {
    &DestroySegment::singleton,
    &DestroyTechVia::singleton,
    &DestroyVia::singleton,
    &DestroyShort::singleton,
    &DestroyVWire::singleton,
};

//
// this code uses a lookup table to find the correct allocator
//
inline void destroyEdge(dbRtEdge* edge)
{
  g_edge_destroyers[edge->getType()]->destroyObject(edge);
}

void dbRtTree::addObjects(dbWireEncoder& encoder, dbRtNode* node)
{
  std::vector<dbObject*>::iterator itr;

  for (itr = node->_objects.begin(); itr != node->_objects.end(); ++itr) {
    dbObject*    obj  = *itr;
    dbObjectType type = obj->getObjectType();

    if (type == dbITermObj) {
      encoder.addITerm((dbITerm*) obj);

    }

    else if (type == dbBTermObj) {
      encoder.addBTerm((dbBTerm*) obj);
    }
  }
}

void dbRtVia::getBBox(Rect& bbox)
{
  int    x   = _src->_x;
  int    y   = _src->_y;
  dbBox* box = _via->getBBox();

  if (box == NULL) {
    bbox.reset(0, 0, 0, 0);
    return;
  }

  Rect b;
  box->getBox(b);
  int xmin = b.xMin() + x;
  int ymin = b.yMin() + y;
  int xmax = b.xMax() + x;
  int ymax = b.yMax() + y;
  bbox.reset(xmin, ymin, xmax, ymax);
}

void dbRtTechVia::getBBox(Rect& bbox)
{
  int    x   = _src->_x;
  int    y   = _src->_y;
  dbBox* box = _via->getBBox();

  if (box == NULL) {
    bbox.reset(0, 0, 0, 0);
    return;
  }

  Rect b;
  box->getBox(b);
  int xmin = b.xMin() + x;
  int ymin = b.yMin() + y;
  int xmax = b.xMax() + x;
  int ymax = b.yMax() + y;
  bbox.reset(xmin, ymin, xmax, ymax);
}

int dbRtSegment::getWidth() const
{
  if (_non_default_rule)
    return _non_default_rule->getWidth();

  return _src->_layer->getWidth();
}

void dbRtSegment::getBBox(Rect& bbox)
{
  int src_x = _src->_x;
  int src_y = _src->_y;
  int tgt_x = _tgt->_x;
  int tgt_y = _tgt->_y;
  int dw    = getWidth() >> 1;
  int x1, x2, y1, y2;

  if (src_x == tgt_x)  // vert. path
  {
    x1 = src_x - dw;
    x2 = src_x + dw;

    if (src_y > tgt_y) {
      if (_tgt_style.getType() == dbRtEndStyle::VARIABLE)
        y1 = tgt_y - _tgt_style.getExt();
      else
        y1 = tgt_y - dw;

      if (_src_style.getType() == dbRtEndStyle::VARIABLE)
        y2 = src_y + _src_style.getExt();
      else
        y2 = src_y + dw;
    } else if (src_y < tgt_y) {
      if (_src_style.getType() == dbRtEndStyle::VARIABLE)
        y1 = src_y - _src_style.getExt();
      else
        y1 = src_y - dw;

      if (_tgt_style.getType() == dbRtEndStyle::VARIABLE)
        y2 = tgt_y + _tgt_style.getExt();
      else
        y2 = tgt_y + dw;
    } else {
      y1 = src_y - dw;
      y2 = src_y + dw;
    }
  } else {
    ZASSERT(src_y == tgt_y);  // horiz. path
    y1 = src_y - dw;
    y2 = src_y + dw;

    if (src_x > tgt_x) {
      if (_tgt_style.getType() == dbRtEndStyle::VARIABLE)
        x1 = tgt_x - _tgt_style.getExt();
      else
        x1 = tgt_x - dw;

      if (_src_style.getType() == dbRtEndStyle::VARIABLE)
        x2 = src_x + _src_style.getExt();
      else
        x2 = src_x + dw;
    } else if (src_x < tgt_x) {
      if (_src_style.getType() == dbRtEndStyle::VARIABLE)
        x1 = src_x - _src_style.getExt();
      else
        x1 = src_x - dw;

      if (_tgt_style.getType() == dbRtEndStyle::VARIABLE)
        x2 = tgt_x + _tgt_style.getExt();
      else
        x2 = tgt_x + dw;
    } else {
      x1 = src_x - dw;
      x2 = src_x + dw;
    }
  }

  bbox.reset(x1, y1, x2, y2);
}

void dbRtShort::getBBox(Rect& bbox)
{
  bbox.init(_src->_x, _src->_y, _tgt->_x, _tgt->_y);
}

void dbRtVWire::getBBox(Rect& bbox)
{
  bbox.init(_src->_x, _src->_y, _tgt->_x, _tgt->_y);
}

dbRtTree::dbRtTree()
{
}

dbRtTree::~dbRtTree()
{
  node_iterator nitr;

  for (nitr = _nodes.begin(); nitr != _nodes.end(); ++nitr)
    g_node_alloc.destroy(*nitr);

  edge_iterator eitr;

  for (eitr = _edges.begin(); eitr != _edges.end(); ++eitr)
    destroyEdge(*eitr);
}

void dbRtTree::add_node(dbRtNode* node)
{
  _nodes.push_back(node);
}
void dbRtTree::add_edge(dbRtEdge* edge)
{
  _edges.push_back(edge);
}
void dbRtTree::remove_node(dbRtNode* node)
{
  _nodes.remove(node);
}
void dbRtTree::remove_edge(dbRtEdge* edge)
{
  if (edge->_shape_id != -1)
    _edge_map[edge->_shape_id] = 0;

  _edges.remove(edge);
}

void dbRtTree::clear()
{
  node_iterator nitr;

  for (nitr = _nodes.begin(); nitr != _nodes.end(); ++nitr)
    g_node_alloc.destroy(*nitr);

  edge_iterator eitr;

  for (eitr = _edges.begin(); eitr != _edges.end(); ++eitr)
    destroyEdge(*eitr);

  _edges.clear();
  _nodes.clear();
  _edge_map.clear();
}

dbRtNode* dbRtTree::createNode(int x, int y, dbTechLayer* l)
{
  dbRtNode* n = new (g_node_alloc.malloc()) dbRtNode(x, y, l);
  assert(n);
  n->_rt_tree = this;
  add_node(n);
  return n;
}

dbRtVia* dbRtTree::createVia(dbRtNode*         src,
                             dbRtNode*         tgt,
                             dbVia*            via,
                             dbWireType::Value type,
                             dbTechLayerRule*  rule)
{
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");
  assert((src->_rt_tree == this) && (tgt->_rt_tree == this));

  dbRtVia* v  = new (g_via_alloc.malloc()) dbRtVia(via, type, rule);
  v->_src     = src;
  v->_tgt     = tgt;
  v->_rt_tree = this;
  tgt->add_edge(v);
  src->add_edge(v);
  add_edge(v);
  return v;
}

dbRtTechVia* dbRtTree::createTechVia(dbRtNode*         src,
                                     dbRtNode*         tgt,
                                     dbTechVia*        via,
                                     dbWireType::Value type,
                                     dbTechLayerRule*  rule)
{
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");
  assert((src->_rt_tree == this) && (tgt->_rt_tree == this));

  dbRtTechVia* v = new (g_tech_via_alloc.malloc()) dbRtTechVia(via, type, rule);
  v->_src        = src;
  v->_tgt        = tgt;
  v->_rt_tree    = this;
  tgt->add_edge(v);
  src->add_edge(v);
  add_edge(v);
  return v;
}

dbRtSegment* dbRtTree::createSegment(dbRtNode*         src,
                                     dbRtNode*         tgt,
                                     dbRtEndStyle      src_style,
                                     dbRtEndStyle      tgt_style,
                                     dbWireType::Value type,
                                     dbTechLayerRule*  rule)
{
  assert(src->_layer == tgt->_layer);
  assert((src->_x == tgt->_x || src->_y == tgt->_y) && "non-orthognal segment");
  assert((src->_rt_tree == this) && (tgt->_rt_tree == this));

  dbRtSegment* s = new (g_segment_alloc.malloc())
      dbRtSegment(src_style, tgt_style, type, rule);

  s->_src     = src;
  s->_tgt     = tgt;
  s->_rt_tree = this;
  tgt->add_edge(s);
  src->add_edge(s);
  add_edge(s);
  return s;
}

dbRtShort* dbRtTree::createShort(dbRtNode*         src,
                                 dbRtNode*         tgt,
                                 dbWireType::Value type,
                                 dbTechLayerRule*  rule)
{
  assert((src->_rt_tree == this) && (tgt->_rt_tree == this));

  dbRtShort* s = new (g_short_alloc.malloc()) dbRtShort(type, rule);
  s->_src      = src;
  s->_tgt      = tgt;
  s->_rt_tree  = this;
  tgt->add_edge(s);
  src->add_edge(s);
  add_edge(s);
  return s;
}

dbRtVWire* dbRtTree::createVWire(dbRtNode*         src,
                                 dbRtNode*         tgt,
                                 dbWireType::Value type,
                                 dbTechLayerRule*  rule)
{
  assert((src->_rt_tree == this) && (tgt->_rt_tree == this));

  dbRtVWire* s = new (g_vwire_alloc.malloc()) dbRtVWire(type, rule);
  s->_src      = src;
  s->_tgt      = tgt;
  s->_rt_tree  = this;
  tgt->add_edge(s);
  src->add_edge(s);
  add_edge(s);
  return s;
}

void dbRtTree::deleteNode(dbRtNode* n)
{
  assert((n->_rt_tree == this));

  dbRtNode::edge_iterator eitr;

  for (eitr = n->begin(); eitr != n->end(); eitr = n->begin()) {
    dbRtEdge* e = *eitr;
    n->remove_edge(e);
    e->opposite(n)->remove_edge(e);
    remove_edge(e);
    destroyEdge(e);
  }

  remove_node(n);
  g_node_alloc.destroy(n);
}

void dbRtTree::deleteEdge(dbRtEdge* e)
{
  assert((e->_rt_tree == this));
  e->_src->remove_edge(e);
  e->_tgt->remove_edge(e);
  remove_edge(e);
  destroyEdge(e);
}

void dbRtTree::deleteEdge(dbRtEdge* e, bool destroy_orphan_nodes)
{
  assert((e->_rt_tree == this));
  e->_src->remove_edge(e);
  e->_tgt->remove_edge(e);
  remove_edge(e);

  if (destroy_orphan_nodes) {
    if (e->_src->_head == NULL) {
      remove_node(e->_src);
      g_node_alloc.destroy(e->_src);
    }

    if (e->_tgt->_head == NULL) {
      remove_node(e->_tgt);
      g_node_alloc.destroy(e->_tgt);
    }
  }

  destroyEdge(e);
}

dbRtEdge* dbRtTree::getEdge(uint shape_id)
{
  assert(shape_id < _edge_map.size());
  dbRtEdge* e = _edge_map[shape_id];
  return e;
}

void dbRtTree::decode(dbWire* wire, bool decode_bterms_iterms)
{
  clear();
  _dbWire* w = (_dbWire*) wire;

  std::vector<dbRtNode*> junction_map;
  junction_map.resize(w->_opcodes.size(), NULL);
  _edge_map.resize(w->_opcodes.size(), NULL);

  dbTechLayer*      cur_layer = NULL;
  dbTechLayerRule*  cur_rule  = NULL;
  dbWireType::Value cur_type  = dbWireType::NONE;
  dbRtNode*         prev      = NULL;
  dbRtEndStyle      prev_style;
  int               jct_id   = -1;
  int               short_id = -1;
  int               vwire_id = -1;

  dbWireDecoder decoder;
  decoder.begin(wire);
  bool done = false;

  while (!done) {
    switch (decoder.next()) {
      case dbWireDecoder::PATH: {
        cur_layer = decoder.getLayer();
        cur_type  = decoder.getWireType();
        prev      = NULL;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::JUNCTION: {
        cur_layer = decoder.getLayer();
        cur_type  = decoder.getWireType();
        jct_id    = decoder.getJunctionValue();
        prev      = NULL;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::SHORT: {
        cur_layer = decoder.getLayer();
        cur_type  = decoder.getWireType();
        short_id  = decoder.getJunctionValue();
        prev      = NULL;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::VWIRE: {
        cur_layer = decoder.getLayer();
        cur_type  = decoder.getWireType();
        vwire_id  = decoder.getJunctionValue();
        prev      = NULL;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::POINT: {
        int x, y;
        decoder.getPoint(x, y);

        // The decoder always outputs the point of the junction.
        if (jct_id != -1) {
          prev_style.setExtended();
          prev   = junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == NULL) {
          prev_style.setExtended();
          prev                                  = createNode(x, y, cur_layer);
          junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            dbRtNode* s = junction_map[short_id];
            dbRtEdge* e = createShort(s, prev, cur_type, cur_rule);
            _edge_map[decoder.getJunctionId()] = e;
            e->_shape_id                       = decoder.getJunctionId();
            short_id                           = -1;
          }

          if (vwire_id != -1) {
            dbRtNode* s = junction_map[vwire_id];
            dbRtEdge* e = createVWire(s, prev, cur_type, cur_rule);
            _edge_map[decoder.getJunctionId()] = e;
            e->_shape_id                       = decoder.getJunctionId();
            vwire_id                           = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->getPoint(prev_x, prev_y);

          // And check for the colinear cancelation of a extension.
          if ((x == prev_x) && (y == prev_y)) {
            if (prev_style._type == dbRtEndStyle::VARIABLE) {
              prev_style.setExtended();
              junction_map[decoder.getJunctionId()] = prev;
              break;
            }
          }

          dbRtNode* cur                         = createNode(x, y, cur_layer);
          junction_map[decoder.getJunctionId()] = cur;
          dbRtSegment* s                        = createSegment(
              prev, cur, prev_style, dbRtEndStyle(), cur_type, cur_rule);
          _edge_map[decoder.getJunctionId()] = s;
          s->_shape_id                       = decoder.getJunctionId();
          s->_property                       = decoder.getProperty();
          prev                               = cur;
          prev_style.setExtended();
        }

        break;
      }

      case dbWireDecoder::POINT_EXT: {
        int x, y, ext;
        decoder.getPoint(x, y, ext);

        // The decoder always outputs the point of the junction.
        if (jct_id != -1) {
          prev_style.setVariable(ext);
          prev   = junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == NULL) {
          prev_style.setVariable(ext);
          prev                                  = createNode(x, y, cur_layer);
          junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            dbRtNode* s = junction_map[short_id];
            dbRtEdge* e = createShort(s, prev, cur_type, cur_rule);
            _edge_map[decoder.getJunctionId()] = e;
            e->_shape_id                       = decoder.getJunctionId();
            short_id                           = -1;
          }

          if (vwire_id != -1) {
            dbRtNode* s = junction_map[vwire_id];
            dbRtEdge* e = createVWire(s, prev, cur_type, cur_rule);
            _edge_map[decoder.getJunctionId()] = e;
            e->_shape_id                       = decoder.getJunctionId();
            vwire_id                           = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->getPoint(prev_x, prev_y);

          // A colinear point with an extenstion begins a new path segment
          if ((x == prev_x) && (y == prev_y)) {
            prev_style.setVariable(ext);
            junction_map[decoder.getJunctionId()] = prev;
          } else {
            dbRtEndStyle cur_style;
            cur_style.setVariable(ext);
            dbRtNode* cur                         = createNode(x, y, cur_layer);
            junction_map[decoder.getJunctionId()] = cur;
            dbRtSegment* s                        = createSegment(
                prev, cur, prev_style, cur_style, cur_type, cur_rule);
            _edge_map[decoder.getJunctionId()] = s;
            s->_shape_id                       = decoder.getJunctionId();
            s->_property                       = decoder.getProperty();
            prev                               = cur;
            prev_style                         = cur_style;
          }
        }
        break;
      }

      case dbWireDecoder::VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->getPoint(x, y);
        dbRtNode* cur                         = createNode(x, y, cur_layer);
        junction_map[decoder.getJunctionId()] = cur;
        dbRtVia* v = createVia(prev, cur, decoder.getVia(), cur_type, cur_rule);
        _edge_map[decoder.getJunctionId()] = v;
        v->_shape_id                       = decoder.getJunctionId();
        prev                               = cur;
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->getPoint(x, y);
        dbRtNode* cur                         = createNode(x, y, cur_layer);
        junction_map[decoder.getJunctionId()] = cur;
        dbRtTechVia* v                        = createTechVia(
            prev, cur, decoder.getTechVia(), cur_type, cur_rule);
        _edge_map[decoder.getJunctionId()] = v;
        v->_shape_id                       = decoder.getJunctionId();
        prev                               = cur;
        break;
      }

      case dbWireDecoder::RECT: {
        // ignored
        break;
      }
      case dbWireDecoder::ITERM: {
        if (decode_bterms_iterms)
          prev->addObject((dbObject*) decoder.getITerm());
        break;
      }

      case dbWireDecoder::BTERM: {
        if (decode_bterms_iterms)
          prev->addObject((dbObject*) decoder.getBTerm());
        break;
      }

      case dbWireDecoder::RULE: {
        cur_rule = decoder.getRule();
        break;
      }

      case dbWireDecoder::END_DECODE: {
        done = true;
        break;
      }
    }
  }
}

void dbRtTree::encode(dbWire* wire, bool encode_bterms_iterms)
{
  dbWireEncoder encoder;

  encoder.begin(wire);

  node_iterator          itr;
  std::vector<dbRtEdge*> path;
  path.reserve(1024);

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    dbRtNode* n = *itr;
    n->_jct_id  = -1;
    n->_visited = false;
  }

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    dbRtNode* n = *itr;

    // Node visted
    if (n->_visited)
      continue;

    // Orphan node
    if (n->isOrphan())
      continue;

    // Encode path from leaf node
    if (n->isLeaf()) {
      path.clear();
      encodePath(
          encoder, path, n, dbWireType::NONE, NULL, encode_bterms_iterms);
    }
  }

  encoder.end();
}

void dbRtTree::encodePath(dbWireEncoder&          encoder,
                          std::vector<dbRtEdge*>& path,
                          dbRtNode*               src,
                          dbWireType::Value       cur_type,
                          dbTechLayerRule*        cur_rule,
                          bool                    encode_bterms_iterms)
{
  src->_visited = true;
  assert(!src->isOrphan());
  // if( src->isOrphan() )
  //{
  //    encodePath( encoder, path );
  //    return;
  //}

  dbRtNode::edge_iterator itr;
  bool                    has_shorts_or_vwires = false;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    dbRtEdge* e   = *itr;
    dbRtNode* tgt = e->opposite(src);

    if (tgt->_visited)  // incoming edge
      continue;

    if ((e->getType() == dbRtEdge::SHORT)
        || (e->getType()
            == dbRtEdge::VWIRE))  // do shorts/vwires on second pass
    {
      has_shorts_or_vwires = true;
      continue;
    }

    bool type_or_rule_changed = false;

    if (e->_non_default_rule != cur_rule) {
      cur_rule             = e->_non_default_rule;
      type_or_rule_changed = true;
    }

    if (e->_wire_type != cur_type) {
      cur_type             = e->_wire_type;
      type_or_rule_changed = true;
    }

    if (type_or_rule_changed && (!path.empty()))
      encodePath(encoder, path, encode_bterms_iterms);

    path.push_back(e);
    encodePath(encoder, path, tgt, cur_type, cur_rule, encode_bterms_iterms);
  }

  // Handle the case where there was only a short branch(es) from the src node:
  if (!path.empty())
    encodePath(encoder, path, encode_bterms_iterms);

  if (!has_shorts_or_vwires)
    return;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    dbRtEdge* e   = *itr;
    dbRtNode* tgt = e->opposite(src);

    if (tgt->_visited)  // incoming edge
      continue;

    if ((e->getType() == dbRtEdge::SHORT)
        || (e->getType() == dbRtEdge::VWIRE)) {
      std::vector<dbRtEdge*> new_path;
      new_path.push_back(e);
      encodePath(encoder,
                 new_path,
                 tgt,
                 e->_wire_type,
                 e->_non_default_rule,
                 encode_bterms_iterms);
    }
  }
}

void dbRtTree::encodePath(dbWireEncoder&          encoder,
                          std::vector<dbRtEdge*>& path,
                          bool                    encode_bterms_iterms)
{
  std::vector<dbRtEdge*>::iterator itr = path.begin();

  if (itr == path.end())
    return;

  dbRtEdge*    e = *itr;
  dbRtEndStyle prev_style;

  bool is_short_or_vwire_path = false;

  switch (e->_type) {
    case dbRtEdge::SEGMENT: {
      dbRtSegment* s = (dbRtSegment*) e;

      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        if (s->_src_style._type == dbRtEndStyle::EXTENDED)
          e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
        else
          e->_src->_jct_id
              = encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      } else {
        if (e->_non_default_rule == NULL) {
          if (s->_src_style._type == dbRtEndStyle::EXTENDED)
            encoder.newPath(e->_src->_jct_id, e->_wire_type);
          else
            encoder.newPathExt(
                e->_src->_jct_id, s->_src_style._ext, e->_wire_type);
        } else {
          if (s->_src_style._type == dbRtEndStyle::EXTENDED)
            encoder.newPath(
                e->_src->_jct_id, e->_wire_type, e->_non_default_rule);
          else
            encoder.newPathExt(e->_src->_jct_id,
                               s->_src_style._ext,
                               e->_wire_type,
                               e->_non_default_rule);
        }

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      }

      if (s->_tgt_style._type == dbRtEndStyle::EXTENDED)
        e->_tgt->_jct_id
            = encoder.addPoint(e->_tgt->_x, e->_tgt->_y, e->_property);
      else
        e->_tgt->_jct_id = encoder.addPoint(
            e->_tgt->_x, e->_tgt->_y, s->_tgt_style._ext, e->_property);

      if (encode_bterms_iterms)
        addObjects(encoder, e->_tgt);
      prev_style = s->_tgt_style;
      break;
    }

    case dbRtEdge::TECH_VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      } else {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        else
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      }

      dbRtTechVia* v   = (dbRtTechVia*) e;
      e->_tgt->_jct_id = encoder.addTechVia(v->_via);
      if (encode_bterms_iterms)
        addObjects(encoder, e->_tgt);
      break;
    }

    case dbRtEdge::VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      } else {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        else
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_src);
      }

      dbRtVia* v       = (dbRtVia*) e;
      e->_tgt->_jct_id = encoder.addVia(v->_via);
      if (encode_bterms_iterms)
        addObjects(encoder, e->_tgt);
      break;
    }

    case dbRtEdge::SHORT: {
      assert(e->_src->_jct_id != -1);

      if (e->_non_default_rule == NULL)
        encoder.newPathShort(e->_src->_jct_id, e->_src->_layer, e->_wire_type);
      else
        encoder.newPathShort(e->_src->_jct_id,
                             e->_src->_layer,
                             e->_wire_type,
                             e->_non_default_rule);

      is_short_or_vwire_path = true;
      break;
    }

    case dbRtEdge::VWIRE: {
      assert(e->_src->_jct_id != -1);

      if (e->_non_default_rule == NULL)
        encoder.newPathVirtualWire(
            e->_src->_jct_id, e->_src->_layer, e->_wire_type);
      else
        encoder.newPathVirtualWire(e->_src->_jct_id,
                                   e->_src->_layer,
                                   e->_wire_type,
                                   e->_non_default_rule);

      is_short_or_vwire_path = true;
      break;
    }
  }

  for (++itr; itr != path.end(); ++itr) {
    e = *itr;

    switch (e->_type) {
      case dbRtEdge::SEGMENT: {
        dbRtSegment* s = (dbRtSegment*) e;

        if (is_short_or_vwire_path) {
          if (s->_src_style._type == dbRtEndStyle::EXTENDED)
            e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
          else
            e->_src->_jct_id = encoder.addPoint(
                e->_src->_x, e->_src->_y, s->_src_style._ext);

          if (encode_bterms_iterms)
            addObjects(encoder, e->_src);
          is_short_or_vwire_path = false;
        }

        else if (prev_style._type != s->_src_style._type) {
          // reset: default ext
          if (prev_style._type == dbRtEndStyle::VARIABLE
              && s->_src_style._type == dbRtEndStyle::EXTENDED)
            encoder.addPoint(e->_src->_x, e->_src->_y);

          // reset: variable ext
          else if (prev_style._type == dbRtEndStyle::EXTENDED
                   && s->_src_style._type == dbRtEndStyle::VARIABLE)
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);

          // Reset: variable ext
          else if (prev_style._type == dbRtEndStyle::VARIABLE
                   && s->_src_style._type == dbRtEndStyle::VARIABLE)
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);
        }

        assert(e->_src->_jct_id != -1);

        if (s->_tgt_style._type == dbRtEndStyle::EXTENDED)
          e->_tgt->_jct_id
              = encoder.addPoint(e->_tgt->_x, e->_tgt->_y, e->_property);
        else
          e->_tgt->_jct_id = encoder.addPoint(
              e->_tgt->_x, e->_tgt->_y, s->_tgt_style._ext, e->_property);

        if (encode_bterms_iterms)
          addObjects(encoder, e->_tgt);
        prev_style = s->_tgt_style;
        break;
      }

      case dbRtEdge::TECH_VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id       = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;
          if (encode_bterms_iterms)
            addObjects(encoder, e->_src);
        }

        assert(e->_src->_jct_id != -1);

        dbRtTechVia* v   = (dbRtTechVia*) e;
        e->_tgt->_jct_id = encoder.addTechVia(v->_via);
        if (encode_bterms_iterms)
          addObjects(encoder, e->_tgt);
        break;
      }

      case dbRtEdge::VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id       = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;
          if (encode_bterms_iterms)
            addObjects(encoder, e->_src);
        }

        assert(e->_src->_jct_id != -1);

        dbRtVia* v       = (dbRtVia*) e;
        e->_tgt->_jct_id = encoder.addVia(v->_via);
        if (encode_bterms_iterms)
          addObjects(encoder, e->_tgt);
        break;
      }

      case dbRtEdge::SHORT:
      case dbRtEdge::VWIRE: {
        // there should never be a short edge in the middle of a path
        assert(0);
        break;
      }
    }
  }

  path.clear();
}

dbRtTree* dbRtTree::duplicate()
{
  dbRtTree* G = new dbRtTree();
  G->_edge_map.resize(_edge_map.size(), NULL);

  node_iterator itr;

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    dbRtNode* n = *itr;
    n->_visited = false;  // not visited
  }

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    dbRtNode* n = *itr;

    if (n->_visited == false) {
      dbRtNode* src = G->createNode(n->_x, n->_y, n->_layer);
      copyNode(G, n, src, true);
    }
  }

  return G;
}

dbRtNode* dbRtTree::duplicate(dbRtTree* G, dbRtNode* other, bool copy_objs)
{
  dbRtNode* n = G->createNode(other->_x, other->_y, other->_layer);

  if (copy_objs)
    n->_objects = other->_objects;
  return n;
}

dbRtEdge* dbRtTree::duplicate(dbRtTree* G,
                              dbRtEdge* edge,
                              dbRtNode* src,
                              dbRtNode* tgt)
{
  dbRtEdge* e = NULL;

  switch (edge->_type) {
    case dbRtEdge::SEGMENT: {
      dbRtSegment* s = (dbRtSegment*) edge;
      e              = G->createSegment(src,
                           tgt,
                           s->_src_style,
                           s->_tgt_style,
                           s->_wire_type,
                           s->_non_default_rule);
      break;
    }

    case dbRtEdge::TECH_VIA: {
      dbRtTechVia* v = (dbRtTechVia*) edge;
      e              = G->createTechVia(
          src, tgt, v->_via, v->_wire_type, v->_non_default_rule);
      break;
    }

    case dbRtEdge::VIA: {
      dbRtVia* v = (dbRtVia*) edge;
      e = G->createVia(src, tgt, v->_via, v->_wire_type, v->_non_default_rule);
      break;
    }

    case dbRtEdge::SHORT: {
      dbRtShort* s = (dbRtShort*) edge;
      e = G->createShort(src, tgt, s->_wire_type, s->_non_default_rule);
      break;
    }

    case dbRtEdge::VWIRE: {
      dbRtVWire* w = (dbRtVWire*) edge;
      e = G->createVWire(src, tgt, w->_wire_type, w->_non_default_rule);
      break;
    }
  }

  return e;
}

void dbRtTree::copyNode(dbRtTree* G,
                        dbRtNode* node,
                        dbRtNode* src,
                        bool      copy_edge_map)
{
  node->_visited = true;  // visited
  dbRtNode::edge_iterator itr;
  int                     visited_cnt = 0;

  for (itr = node->begin(); itr != node->end(); ++itr) {
    dbRtEdge* edge  = *itr;
    dbRtNode* other = edge->opposite(node);

    if (other->_visited) {
      ++visited_cnt;
      assert(visited_cnt <= 1);  // graph contains a cycle
    }

    else {
      dbRtNode* tgt = G->createNode(other->_x, other->_y, other->_layer);
      tgt->_objects = other->_objects;
      copyEdge(G, src, tgt, edge, copy_edge_map);
      copyNode(G, other, tgt, copy_edge_map);
    }
  }
}

void dbRtTree::copyEdge(dbRtTree* G,
                        dbRtNode* src,
                        dbRtNode* tgt,
                        dbRtEdge* edge,
                        bool      copy_edge_map)
{
  dbRtEdge* e = duplicate(G, edge, src, tgt);

  if (copy_edge_map && (edge->_shape_id != -1)) {
    G->_edge_map[edge->_shape_id] = e;
    e->_shape_id                  = edge->_shape_id;
  }
}

void dbRtTree::move(dbRtTree* T)
{
  node_iterator nitr;

  for (nitr = T->_nodes.begin(); nitr != T->_nodes.end();
       nitr = T->_nodes.begin()) {
    dbRtNode* n = *nitr;
    T->_nodes.remove(n);
    _nodes.push_back(n);
    n->_rt_tree = this;
  }

  edge_iterator eitr;

  for (eitr = T->_edges.begin(); eitr != T->_edges.end();
       eitr = T->_edges.begin()) {
    dbRtEdge* e = *eitr;
    T->_edges.remove(e);
    _edges.push_back(e);
    e->_rt_tree  = this;
    e->_shape_id = -1;
  }

  T->_edge_map.clear();
}

void dbRtTree::copy(dbRtTree* T)
{
  node_iterator itr;

  for (itr = T->_nodes.begin(); itr != T->_nodes.end(); ++itr) {
    dbRtNode* n = *itr;
    n->_visited = false;  // not visited
  }

  for (itr = T->_nodes.begin(); itr != T->_nodes.end(); ++itr) {
    dbRtNode* n = *itr;

    if (n->_visited == false) {
      dbRtNode* src = createNode(n->_x, n->_y, n->_layer);
      copyNode(this, n, src, false);
    }
  }
}

}  // namespace odb
