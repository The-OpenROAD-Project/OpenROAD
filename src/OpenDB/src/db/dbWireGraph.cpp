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

#include "dbWireGraph.h"

#include "dbWire.h"
#include "dbWireCodec.h"

namespace odb {

static void addObject(dbWireEncoder& encoder, dbObject* obj)
{
  dbObjectType type = obj->getObjectType();

  if (type == dbITermObj) {
    encoder.addITerm((dbITerm*) obj);

  }

  else if (type == dbBTermObj) {
    encoder.addBTerm((dbBTerm*) obj);
  }
}

dbWireGraph::dbWireGraph()
{
}

dbWireGraph::~dbWireGraph()
{
}

void dbWireGraph::clear()
{
  node_iterator itr;

  for (itr = begin_nodes(); itr != end_nodes();)
    itr = deleteNode(itr);

  _junction_map.clear();
}

dbWireGraph::Node* dbWireGraph::createNode(int x, int y, dbTechLayer* l)
{
  Node* n = new Node(x, y, l);
  assert(n);
  _nodes.push_back(n);
  return n;
}

dbWireGraph::Via* dbWireGraph::createVia(Node*             src,
                                         Node*             tgt,
                                         dbVia*            via,
                                         dbWireType::Value type,
                                         dbTechLayerRule*  rule)
{
  assert(tgt->_in_edge == NULL);
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");

  Via* v        = new Via(via, type, rule);
  v->_src       = src;
  v->_tgt       = tgt;
  tgt->_in_edge = v;
  src->_out_edges.push_back(v);
  _edges.push_back(v);
  return v;
}

dbWireGraph::TechVia* dbWireGraph::createTechVia(Node*             src,
                                                 Node*             tgt,
                                                 dbTechVia*        via,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule*  rule)
{
  assert(tgt->_in_edge == NULL);
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");

  TechVia* v    = new TechVia(via, type, rule);
  v->_src       = src;
  v->_tgt       = tgt;
  tgt->_in_edge = v;
  src->_out_edges.push_back(v);
  _edges.push_back(v);
  return v;
}

dbWireGraph::Segment* dbWireGraph::createSegment(Node*             src,
                                                 Node*             tgt,
                                                 EndStyle          src_style,
                                                 EndStyle          tgt_style,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule*  rule)
{
  assert(tgt->_in_edge == NULL);
  assert(src->_layer == tgt->_layer);
  assert((src->_x == tgt->_x || src->_y == tgt->_y) && "non-orthognal segment");

  Segment* s = new Segment(src_style, tgt_style, type, rule);

  s->_src       = src;
  s->_tgt       = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

dbWireGraph::Short* dbWireGraph::createShort(Node*             src,
                                             Node*             tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule*  rule)
{
  assert(tgt->_in_edge == NULL);

  Short* s      = new Short(type, rule);
  s->_src       = src;
  s->_tgt       = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

dbWireGraph::VWire* dbWireGraph::createVWire(Node*             src,
                                             Node*             tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule*  rule)
{
  assert(tgt->_in_edge == NULL);

  VWire* s      = new VWire(type, rule);
  s->_src       = src;
  s->_tgt       = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

void dbWireGraph::deleteNode(Node* n)
{
  Node::edge_iterator eitr;

  for (eitr = n->begin(); eitr != n->end();) {
    Edge* e           = *eitr;
    eitr              = n->_out_edges.remove(e);
    e->_tgt->_in_edge = NULL;
    _edges.remove(e);
    delete e;
  }

  if (n->_in_edge) {
    n->_in_edge->_src->_out_edges.remove(n->_in_edge);
    _edges.remove(n->_in_edge);
  }

  _nodes.remove(n);
  delete n;
}

dbWireGraph::node_iterator dbWireGraph::deleteNode(node_iterator itr)
{
  Node*         n    = *itr;
  node_iterator next = ++itr;
  deleteNode(n);
  return next;
}

void dbWireGraph::deleteEdge(Edge* e)
{
  e->_src->_out_edges.remove(e);
  e->_tgt->_in_edge = NULL;
  _edges.remove(e);
  delete e;
}

dbWireGraph::edge_iterator dbWireGraph::deleteEdge(edge_iterator itr)
{
  Edge*         e    = *itr;
  edge_iterator next = ++itr;
  deleteEdge(e);
  return next;
}

dbWireGraph::Edge* dbWireGraph::getEdge(uint shape_id)
{
  assert(shape_id < _junction_map.size());
  Node* n = _junction_map[shape_id];
  assert(n->_in_edge);
  return n->_in_edge;
}

void dbWireGraph::decode(dbWire* wire)
{
  clear();

  _dbWire* w = (_dbWire*) wire;
  _junction_map.resize(w->_opcodes.size(), NULL);

  dbTechLayer*      cur_layer = NULL;
  dbTechLayerRule*  cur_rule  = NULL;
  dbWireType::Value cur_type  = dbWireType::NONE;
  Node*             prev      = NULL;
  EndStyle          prev_style;
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
          prev   = _junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == NULL) {
          prev_style.setExtended();
          prev                                   = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = _junction_map[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = _junction_map[vwire_id];
            createVWire(s, prev, cur_type, cur_rule);
            vwire_id = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->xy(prev_x, prev_y);

          // And check for the colinear cancelation of a extension.
          if ((x == prev_x) && (y == prev_y)) {
            if (prev_style._type == EndStyle::VARIABLE) {
              prev_style.setExtended();
              _junction_map[decoder.getJunctionId()] = prev;
              break;
            }
          }

          Node* cur                              = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = cur;
          createSegment(prev, cur, prev_style, EndStyle(), cur_type, cur_rule);
          prev = cur;
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
          prev   = _junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == NULL) {
          prev_style.setVariable(ext);
          prev                                   = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = _junction_map[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = _junction_map[vwire_id];
            createVWire(s, prev, cur_type, cur_rule);
            vwire_id = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->xy(prev_x, prev_y);

          // A colinear point with an extenstion begins a new path segment
          if ((x == prev_x) && (y == prev_y)) {
            prev_style.setVariable(ext);
            _junction_map[decoder.getJunctionId()] = prev;
          } else {
            EndStyle cur_style;
            cur_style.setVariable(ext);
            Node* cur = createNode(x, y, cur_layer);
            _junction_map[decoder.getJunctionId()] = cur;
            createSegment(prev, cur, prev_style, cur_style, cur_type, cur_rule);
            prev       = cur;
            prev_style = cur_style;
          }
        }
        break;
      }

      case dbWireDecoder::VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur                              = createNode(x, y, cur_layer);
        _junction_map[decoder.getJunctionId()] = cur;
        createVia(prev, cur, decoder.getVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur                              = createNode(x, y, cur_layer);
        _junction_map[decoder.getJunctionId()] = cur;
        createTechVia(prev, cur, decoder.getTechVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::ITERM: {
#ifndef NDEBUG
        if (prev->_object)
          assert(prev->_object == (dbObject*) decoder.getITerm());
        else
          assert(prev->_object == NULL);
#endif
        prev->_object = (dbObject*) decoder.getITerm();
        break;
      }

      case dbWireDecoder::BTERM: {
#ifndef NDEBUG
        if (prev->_object)
          assert(prev->_object == (dbObject*) decoder.getBTerm());
        else
          assert(prev->_object == NULL);
#endif
        prev->_object = (dbObject*) decoder.getBTerm();
        break;
      }

      case dbWireDecoder::RECT:
        // ignored
        break;

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

void dbWireGraph::encode(dbWire* wire)
{
  dbWireEncoder encoder;

  encoder.begin(wire);

  node_iterator      itr;
  std::vector<Edge*> path;

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    Node* n    = *itr;
    n->_jct_id = -1;
  }

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    Node* n = *itr;

    if (n->_in_edge == NULL) {
      path.clear();
      encodePath(encoder, path, n, dbWireType::NONE, NULL);
    }
  }

  encoder.end();
}

class EdgeCmp
{
 public:
  int operator()(dbWireGraph::Edge& lhs, dbWireGraph::Edge& rhs)
  {
    return lhs.type() < rhs.type();
  }
};

void dbWireGraph::encodePath(dbWireEncoder&      encoder,
                             std::vector<Edge*>& path,
                             Node*               src,
                             dbWireType::Value   cur_type,
                             dbTechLayerRule*    cur_rule)
{
  if (src->_out_edges.empty()) {
    encodePath(encoder, path);
    return;
  }

  // src->_out_edges.sort( EdgeCmp() );
  Node::edge_iterator itr;
  bool                has_shorts_or_vwires = false;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT)
        || (e->type() == Edge::VWIRE))  // do shorts/vwires on second pass
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
      encodePath(encoder, path);

    path.push_back(e);
    encodePath(encoder, path, e->_tgt, cur_type, cur_rule);
  }

  // Handle the case where there was only a short branch(es) from the src node:
  if (!path.empty())
    encodePath(encoder, path);

  if (!has_shorts_or_vwires)
    return;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT) || (e->type() == Edge::VWIRE)) {
      std::vector<Edge*> new_path;
      new_path.push_back(e);
      encodePath(
          encoder, new_path, e->_tgt, e->_wire_type, e->_non_default_rule);
    }
  }
}

void dbWireGraph::encodePath(dbWireEncoder& encoder, std::vector<Edge*>& path)
{
  std::vector<Edge*>::iterator itr = path.begin();

  if (itr == path.end())
    return;

  Edge*    e = *itr;
  EndStyle prev_style;

  bool is_short_or_vwire_path = false;

  switch (e->_type) {
    case Edge::SEGMENT: {
      Segment* s = (Segment*) e;

      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        if (s->_src_style._type == EndStyle::EXTENDED)
          e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
        else
          e->_src->_jct_id
              = encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      } else {
        if (e->_non_default_rule == NULL) {
          if (s->_src_style._type == EndStyle::EXTENDED)
            encoder.newPath(e->_src->_jct_id, e->_wire_type);
          else
            encoder.newPathExt(
                e->_src->_jct_id, s->_src_style._ext, e->_wire_type);
        } else {
          if (s->_src_style._type == EndStyle::EXTENDED)
            encoder.newPath(
                e->_src->_jct_id, e->_wire_type, e->_non_default_rule);
          else
            encoder.newPathExt(e->_src->_jct_id,
                               s->_src_style._ext,
                               e->_wire_type,
                               e->_non_default_rule);
        }

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      }

      if (s->_tgt_style._type == EndStyle::EXTENDED)
        e->_tgt->_jct_id = encoder.addPoint(e->_tgt->_x, e->_tgt->_y);
      else
        e->_tgt->_jct_id
            = encoder.addPoint(e->_tgt->_x, e->_tgt->_y, s->_tgt_style._ext);

      if (e->_tgt->_object != NULL)
        addObject(encoder, e->_tgt->_object);

      prev_style = s->_tgt_style;
      break;
    }

    case Edge::TECH_VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      } else {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        else
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      }

      TechVia* v       = (TechVia*) e;
      e->_tgt->_jct_id = encoder.addTechVia(v->_via);

      if (e->_tgt->_object != NULL)
        addObject(encoder, e->_tgt->_object);
      break;
    }

    case Edge::VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_layer, e->_wire_type);
        else
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      } else {
        if (e->_non_default_rule == NULL)
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        else
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);

        if (e->_src->_object != NULL)
          addObject(encoder, e->_src->_object);
      }

      Via* v           = (Via*) e;
      e->_tgt->_jct_id = encoder.addVia(v->_via);

      if (e->_tgt->_object != NULL)
        addObject(encoder, e->_tgt->_object);
      break;
    }

    case Edge::SHORT: {
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

    case Edge::VWIRE: {
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
      case Edge::SEGMENT: {
        Segment* s = (Segment*) e;

        if (is_short_or_vwire_path) {
          if (s->_src_style._type == EndStyle::EXTENDED)
            e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
          else
            e->_src->_jct_id = encoder.addPoint(
                e->_src->_x, e->_src->_y, s->_src_style._ext);

          if (e->_src->_object != NULL)
            addObject(encoder, e->_src->_object);
          is_short_or_vwire_path = false;
        }

        else if (prev_style._type != s->_src_style._type) {
          // reset: default ext
          if (prev_style._type == EndStyle::VARIABLE
              && s->_src_style._type == EndStyle::EXTENDED)
            encoder.addPoint(e->_src->_x, e->_src->_y);

          // reset: variable ext
          else if (prev_style._type == EndStyle::EXTENDED
                   && s->_src_style._type == EndStyle::VARIABLE)
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);

          // Reset: variable ext
          else if (prev_style._type == EndStyle::VARIABLE
                   && s->_src_style._type == EndStyle::VARIABLE)
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style._ext);
        }

        assert(e->_src->_jct_id != -1);

        if (s->_tgt_style._type == EndStyle::EXTENDED)
          e->_tgt->_jct_id = encoder.addPoint(e->_tgt->_x, e->_tgt->_y);
        else
          e->_tgt->_jct_id
              = encoder.addPoint(e->_tgt->_x, e->_tgt->_y, s->_tgt_style._ext);

        if (e->_tgt->_object != NULL)
          addObject(encoder, e->_tgt->_object);
        prev_style = s->_tgt_style;
        break;
      }

      case Edge::TECH_VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id       = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;

          if (e->_src->_object != NULL)
            addObject(encoder, e->_src->_object);
        }

        assert(e->_src->_jct_id != -1);

        TechVia* v       = (TechVia*) e;
        e->_tgt->_jct_id = encoder.addTechVia(v->_via);

        if (e->_tgt->_object != NULL)
          addObject(encoder, e->_tgt->_object);
        break;
      }

      case Edge::VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id       = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;

          if (e->_src->_object != NULL)
            addObject(encoder, e->_src->_object);
        }

        assert(e->_src->_jct_id != -1);

        Via* v           = (Via*) e;
        e->_tgt->_jct_id = encoder.addVia(v->_via);

        if (e->_tgt->_object != NULL)
          addObject(encoder, e->_tgt->_object);
        break;
      }

      case Edge::SHORT:
      case Edge::VWIRE: {
        // there should never be a short edge in the middle of a path
        assert(0);
        break;
      }
    }
  }

  path.clear();
}

}  // namespace odb
