// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbWireGraph.h"

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "utl/Logger.h"

namespace odb {

static void addObject(dbWireEncoder& encoder, dbObject* obj)
{
  dbObjectType type = obj->getObjectType();

  if (type == dbITermObj) {
    encoder.addITerm((dbITerm*) obj);
  } else if (type == dbBTermObj) {
    encoder.addBTerm((dbBTerm*) obj);
  }
}

void dbWireGraph::clear()
{
  node_iterator itr;

  for (itr = begin_nodes(); itr != end_nodes();) {
    itr = deleteNode(itr);
  }

  junction_map_.clear();
}

dbWireGraph::Node* dbWireGraph::createNode(int x, int y, dbTechLayer* l)
{
  Node* n = new Node(x, y, l);
  assert(n);
  nodes_.push_back(n);
  return n;
}

dbWireGraph::Via* dbWireGraph::createVia(Node* src,
                                         Node* tgt,
                                         dbVia* via,
                                         dbWireType::Value type,
                                         dbTechLayerRule* rule)
{
  assert(tgt->in_edge_ == nullptr);
  assert((src->x_ == tgt->x_) && (src->y_ == tgt->y_)
         && "via coordinates are skewed");

  Via* v = new Via(via, type, rule);
  v->src_ = src;
  v->tgt_ = tgt;
  tgt->in_edge_ = v;
  src->out_edges_.push_back(v);
  edges_.push_back(v);
  return v;
}

dbWireGraph::TechVia* dbWireGraph::createTechVia(Node* src,
                                                 Node* tgt,
                                                 dbTechVia* via,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule* rule)
{
  assert(tgt->in_edge_ == nullptr);
  assert((src->x_ == tgt->x_) && (src->y_ == tgt->y_)
         && "via coordinates are skewed");

  TechVia* v = new TechVia(via, type, rule);
  v->src_ = src;
  v->tgt_ = tgt;
  tgt->in_edge_ = v;
  src->out_edges_.push_back(v);
  edges_.push_back(v);
  return v;
}

dbWireGraph::Segment* dbWireGraph::createSegment(Node* src,
                                                 Node* tgt,
                                                 EndStyle src_style,
                                                 EndStyle tgt_style,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule* rule)
{
  assert(tgt->in_edge_ == nullptr);
  assert(src->layer_ == tgt->layer_);
  assert((src->x_ == tgt->x_ || src->y_ == tgt->y_) && "non-orthognal segment");

  Segment* s = new Segment(src_style, tgt_style, type, rule);

  s->src_ = src;
  s->tgt_ = tgt;
  tgt->in_edge_ = s;
  src->out_edges_.push_back(s);
  edges_.push_back(s);
  return s;
}

dbWireGraph::Short* dbWireGraph::createShort(Node* src,
                                             Node* tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule* rule)
{
  assert(tgt->in_edge_ == nullptr);

  Short* s = new Short(type, rule);
  s->src_ = src;
  s->tgt_ = tgt;
  tgt->in_edge_ = s;
  src->out_edges_.push_back(s);
  edges_.push_back(s);
  return s;
}

dbWireGraph::VWire* dbWireGraph::createVWire(Node* src,
                                             Node* tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule* rule)
{
  assert(tgt->in_edge_ == nullptr);

  VWire* s = new VWire(type, rule);
  s->src_ = src;
  s->tgt_ = tgt;
  tgt->in_edge_ = s;
  src->out_edges_.push_back(s);
  edges_.push_back(s);
  return s;
}

void dbWireGraph::deleteNode(Node* n)
{
  Node::edge_iterator eitr;

  for (eitr = n->begin(); eitr != n->end();) {
    Edge* e = *eitr;
    eitr = n->out_edges_.remove(e);
    e->tgt_->in_edge_ = nullptr;
    edges_.remove(e);
    delete e;
  }

  if (n->in_edge_) {
    n->in_edge_->src_->out_edges_.remove(n->in_edge_);
    edges_.remove(n->in_edge_);
  }

  nodes_.remove(n);
  delete n;
}

dbWireGraph::node_iterator dbWireGraph::deleteNode(node_iterator itr)
{
  Node* n = *itr;
  node_iterator next = ++itr;
  deleteNode(n);
  return next;
}

void dbWireGraph::deleteEdge(Edge* e)
{
  e->src_->out_edges_.remove(e);
  e->tgt_->in_edge_ = nullptr;
  edges_.remove(e);
  delete e;
}

dbWireGraph::edge_iterator dbWireGraph::deleteEdge(edge_iterator itr)
{
  Edge* e = *itr;
  edge_iterator next = ++itr;
  deleteEdge(e);
  return next;
}

dbWireGraph::Edge* dbWireGraph::getEdge(uint32_t shape_id)
{
  assert(shape_id < junction_map_.size());
  Node* n = junction_map_[shape_id];
  assert(n->in_edge_);
  return n->in_edge_;
}

void dbWireGraph::decode(dbWire* wire)
{
  clear();

  _dbWire* w = (_dbWire*) wire;
  junction_map_.resize(w->opcodes_.size(), nullptr);

  dbTechLayer* cur_layer = nullptr;
  dbTechLayerRule* cur_rule = nullptr;
  dbWireType::Value cur_type = dbWireType::NONE;
  Node* prev = nullptr;
  EndStyle prev_style;
  int jct_id = -1;
  int short_id = -1;
  int vwire_id = -1;

  dbWireDecoder decoder;
  decoder.begin(wire);
  bool done = false;

  while (!done) {
    switch (decoder.next()) {
      case dbWireDecoder::PATH: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::JUNCTION: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        jct_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::SHORT: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        short_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::VWIRE: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        vwire_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::POINT: {
        int x, y;
        decoder.getPoint(x, y);

        // The decoder always outputs the point of the junction.
        if (jct_id != -1) {
          prev_style.setExtended();
          prev = junction_map_[jct_id];
          jct_id = -1;
        }

        else if (prev == nullptr) {
          prev_style.setExtended();
          prev = createNode(x, y, cur_layer);
          junction_map_[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = junction_map_[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = junction_map_[vwire_id];
            createVWire(s, prev, cur_type, cur_rule);
            vwire_id = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->xy(prev_x, prev_y);

          // And check for the colinear cancelation of a extension.
          if ((x == prev_x) && (y == prev_y)) {
            if (prev_style.getType() == EndStyle::VARIABLE) {
              prev_style.setExtended();
              junction_map_[decoder.getJunctionId()] = prev;
              break;
            }
          }

          Node* cur = createNode(x, y, cur_layer);
          junction_map_[decoder.getJunctionId()] = cur;
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
          prev = junction_map_[jct_id];
          jct_id = -1;
        }

        else if (prev == nullptr) {
          prev_style.setVariable(ext);
          prev = createNode(x, y, cur_layer);
          junction_map_[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = junction_map_[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = junction_map_[vwire_id];
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
            junction_map_[decoder.getJunctionId()] = prev;
          } else {
            EndStyle cur_style;
            cur_style.setVariable(ext);
            Node* cur = createNode(x, y, cur_layer);
            junction_map_[decoder.getJunctionId()] = cur;
            createSegment(prev, cur, prev_style, cur_style, cur_type, cur_rule);
            prev = cur;
            prev_style = cur_style;
          }
        }
        break;
      }

      case dbWireDecoder::VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur = createNode(x, y, cur_layer);
        junction_map_[decoder.getJunctionId()] = cur;
        createVia(prev, cur, decoder.getVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur = createNode(x, y, cur_layer);
        junction_map_[decoder.getJunctionId()] = cur;
        createTechVia(prev, cur, decoder.getTechVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::ITERM: {
        if (!prev) {
          w->getLogger()->error(
              utl::ODB, 1117, "ITerm found without previous element");
        }
        if (prev->object_) {
          assert(prev->object_ == (dbObject*) decoder.getITerm());
        }
        prev->object_ = (dbObject*) decoder.getITerm();
        break;
      }

      case dbWireDecoder::BTERM: {
        if (!prev) {
          w->getLogger()->error(
              utl::ODB, 1116, "BTerm found without previous element");
        }
        if (prev->object_) {
          assert(prev->object_ == (dbObject*) decoder.getBTerm());
        }
        prev->object_ = (dbObject*) decoder.getBTerm();
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

  node_iterator itr;
  std::vector<Edge*> path;

  for (itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
    Node* n = *itr;
    n->jct_id_ = -1;
  }

  for (itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
    Node* n = *itr;

    if (n->in_edge_ == nullptr) {
      path.clear();
      encodePath(encoder, path, n, dbWireType::NONE, nullptr);
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

void dbWireGraph::encodePath(dbWireEncoder& encoder,
                             std::vector<Edge*>& path,
                             Node* src,
                             dbWireType::Value cur_type,
                             dbTechLayerRule* cur_rule)
{
  if (src->out_edges_.empty()) {
    encodePath(encoder, path);
    return;
  }

  // src->_out_edges.sort( EdgeCmp() );
  Node::edge_iterator itr;
  bool has_shorts_or_vwires = false;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT)
        || (e->type() == Edge::VWIRE))  // do shorts/vwires on second pass
    {
      has_shorts_or_vwires = true;
      continue;
    }

    bool type_or_rule_changed = false;

    if (e->non_default_rule_ != cur_rule) {
      cur_rule = e->non_default_rule_;
      type_or_rule_changed = true;
    }

    if (e->wire_type_ != cur_type) {
      cur_type = e->wire_type_;
      type_or_rule_changed = true;
    }

    if (type_or_rule_changed && (!path.empty())) {
      encodePath(encoder, path);
    }

    path.push_back(e);
    encodePath(encoder, path, e->tgt_, cur_type, cur_rule);
  }

  // Handle the case where there was only a short branch(es) from the src node:
  if (!path.empty()) {
    encodePath(encoder, path);
  }

  if (!has_shorts_or_vwires) {
    return;
  }

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT) || (e->type() == Edge::VWIRE)) {
      std::vector<Edge*> new_path;
      new_path.push_back(e);
      encodePath(
          encoder, new_path, e->tgt_, e->wire_type_, e->non_default_rule_);
    }
  }
}

void dbWireGraph::encodePath(dbWireEncoder& encoder, std::vector<Edge*>& path)
{
  std::vector<Edge*>::iterator itr = path.begin();

  if (itr == path.end()) {
    return;
  }

  Edge* e = *itr;
  EndStyle prev_style;

  bool is_short_or_vwire_path = false;

  switch (e->type_) {
    case Edge::SEGMENT: {
      Segment* s = (Segment*) e;

      if (e->src_->jct_id_ == -1) {
        if (e->non_default_rule_ == nullptr) {
          encoder.newPath(e->src_->layer_, e->wire_type_);
        } else {
          encoder.newPath(e->src_->layer_, e->wire_type_, e->non_default_rule_);
        }

        if (s->src_style_.getType() == EndStyle::EXTENDED) {
          e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);
        } else {
          e->src_->jct_id_ = encoder.addPoint(
              e->src_->x_, e->src_->y_, s->src_style_.getExt());
        }

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      } else {
        if (e->non_default_rule_ == nullptr) {
          if (s->src_style_.getType() == EndStyle::EXTENDED) {
            encoder.newPath(e->src_->jct_id_, e->wire_type_);
          } else {
            encoder.newPathExt(
                e->src_->jct_id_, s->src_style_.getExt(), e->wire_type_);
          }
        } else {
          if (s->src_style_.getType() == EndStyle::EXTENDED) {
            encoder.newPath(
                e->src_->jct_id_, e->wire_type_, e->non_default_rule_);
          } else {
            encoder.newPathExt(e->src_->jct_id_,
                               s->src_style_.getExt(),
                               e->wire_type_,
                               e->non_default_rule_);
          }
        }

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      }

      if (s->tgt_style_.getType() == EndStyle::EXTENDED) {
        e->tgt_->jct_id_ = encoder.addPoint(e->tgt_->x_, e->tgt_->y_);
      } else {
        e->tgt_->jct_id_ = encoder.addPoint(
            e->tgt_->x_, e->tgt_->y_, s->tgt_style_.getExt());
      }

      if (e->tgt_->object_ != nullptr) {
        addObject(encoder, e->tgt_->object_);
      }

      prev_style = s->tgt_style_;
      break;
    }

    case Edge::TECH_VIA: {
      if (e->src_->jct_id_ == -1) {
        if (e->non_default_rule_ == nullptr) {
          encoder.newPath(e->src_->layer_, e->wire_type_);
        } else {
          encoder.newPath(e->src_->layer_, e->wire_type_, e->non_default_rule_);
        }

        e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      } else {
        if (e->non_default_rule_ == nullptr) {
          encoder.newPath(e->src_->jct_id_, e->wire_type_);
        } else {
          encoder.newPath(
              e->src_->jct_id_, e->wire_type_, e->non_default_rule_);
        }

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      }

      TechVia* v = (TechVia*) e;
      e->tgt_->jct_id_ = encoder.addTechVia(v->via_);

      if (e->tgt_->object_ != nullptr) {
        addObject(encoder, e->tgt_->object_);
      }
      break;
    }

    case Edge::VIA: {
      if (e->src_->jct_id_ == -1) {
        if (e->non_default_rule_ == nullptr) {
          encoder.newPath(e->src_->layer_, e->wire_type_);
        } else {
          encoder.newPath(e->src_->layer_, e->wire_type_, e->non_default_rule_);
        }

        e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      } else {
        if (e->non_default_rule_ == nullptr) {
          encoder.newPath(e->src_->jct_id_, e->wire_type_);
        } else {
          encoder.newPath(
              e->src_->jct_id_, e->wire_type_, e->non_default_rule_);
        }

        if (e->src_->object_ != nullptr) {
          addObject(encoder, e->src_->object_);
        }
      }

      Via* v = (Via*) e;
      e->tgt_->jct_id_ = encoder.addVia(v->via_);

      if (e->tgt_->object_ != nullptr) {
        addObject(encoder, e->tgt_->object_);
      }
      break;
    }

    case Edge::SHORT: {
      assert(e->src_->jct_id_ != -1);

      if (e->non_default_rule_ == nullptr) {
        encoder.newPathShort(e->src_->jct_id_, e->src_->layer_, e->wire_type_);
      } else {
        encoder.newPathShort(e->src_->jct_id_,
                             e->src_->layer_,
                             e->wire_type_,
                             e->non_default_rule_);
      }

      is_short_or_vwire_path = true;
      break;
    }

    case Edge::VWIRE: {
      assert(e->src_->jct_id_ != -1);

      if (e->non_default_rule_ == nullptr) {
        encoder.newPathVirtualWire(
            e->src_->jct_id_, e->src_->layer_, e->wire_type_);
      } else {
        encoder.newPathVirtualWire(e->src_->jct_id_,
                                   e->src_->layer_,
                                   e->wire_type_,
                                   e->non_default_rule_);
      }

      is_short_or_vwire_path = true;
      break;
    }
  }

  for (++itr; itr != path.end(); ++itr) {
    e = *itr;

    switch (e->type_) {
      case Edge::SEGMENT: {
        Segment* s = (Segment*) e;

        if (is_short_or_vwire_path) {
          if (s->src_style_.getType() == EndStyle::EXTENDED) {
            e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);
          } else {
            e->src_->jct_id_ = encoder.addPoint(
                e->src_->x_, e->src_->y_, s->src_style_.getExt());
          }

          if (e->src_->object_ != nullptr) {
            addObject(encoder, e->src_->object_);
          }
          is_short_or_vwire_path = false;
        }

        else if (prev_style.getType() != s->src_style_.getType()) {
          // reset: default ext
          if (prev_style.getType() == EndStyle::VARIABLE
              && s->src_style_.getType() == EndStyle::EXTENDED) {
            encoder.addPoint(e->src_->x_, e->src_->y_);

            // reset: variable ext
          } else if (prev_style.getType() == EndStyle::EXTENDED
                     && s->src_style_.getType() == EndStyle::VARIABLE) {
            encoder.addPoint(e->src_->x_, e->src_->y_, s->src_style_.getExt());

            // Reset: variable ext
          } else if (prev_style.getType() == EndStyle::VARIABLE
                     && s->src_style_.getType() == EndStyle::VARIABLE) {
            encoder.addPoint(e->src_->x_, e->src_->y_, s->src_style_.getExt());
          }
        }

        assert(e->src_->jct_id_ != -1);

        if (s->tgt_style_.getType() == EndStyle::EXTENDED) {
          e->tgt_->jct_id_ = encoder.addPoint(e->tgt_->x_, e->tgt_->y_);
        } else {
          e->tgt_->jct_id_ = encoder.addPoint(
              e->tgt_->x_, e->tgt_->y_, s->tgt_style_.getExt());
        }

        if (e->tgt_->object_ != nullptr) {
          addObject(encoder, e->tgt_->object_);
        }
        prev_style = s->tgt_style_;
        break;
      }

      case Edge::TECH_VIA: {
        if (is_short_or_vwire_path) {
          e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);
          is_short_or_vwire_path = false;

          if (e->src_->object_ != nullptr) {
            addObject(encoder, e->src_->object_);
          }
        }

        assert(e->src_->jct_id_ != -1);

        TechVia* v = (TechVia*) e;
        e->tgt_->jct_id_ = encoder.addTechVia(v->via_);

        if (e->tgt_->object_ != nullptr) {
          addObject(encoder, e->tgt_->object_);
        }
        break;
      }

      case Edge::VIA: {
        if (is_short_or_vwire_path) {
          e->src_->jct_id_ = encoder.addPoint(e->src_->x_, e->src_->y_);
          is_short_or_vwire_path = false;

          if (e->src_->object_ != nullptr) {
            addObject(encoder, e->src_->object_);
          }
        }

        assert(e->src_->jct_id_ != -1);

        Via* v = (Via*) e;
        e->tgt_->jct_id_ = encoder.addVia(v->via_);

        if (e->tgt_->object_ != nullptr) {
          addObject(encoder, e->tgt_->object_);
        }
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

void dbWireGraph::dump(utl::Logger* logger)
{
  std::map<Node*, int> node2index;
  int index = 0;
  for (auto it = begin_nodes(); it != end_nodes(); ++it) {
    auto node = *it;
    node2index[node] = index;
    int x;
    int y;
    node->xy(x, y);
    std::string obj_name("-");
    if (dbObject* object = node->object()) {
      if (object->getObjectType() == dbITermObj) {
        obj_name = static_cast<dbITerm*>(object)->getName();
      } else if (object->getObjectType() == dbBTermObj) {
        obj_name = static_cast<dbBTerm*>(object)->getName();
      } else {
        obj_name = "<unknown>";
      }
    }
    logger->report("Node {:2}: ({}, {}) {} (obj {})",
                   index++,
                   x,
                   y,
                   node->layer()->getName(),
                   obj_name);
  }

  index = 0;
  for (auto it = begin_edges(); it != end_edges(); ++it) {
    auto edge = *it;
    std::string type;
    switch (edge->type()) {
      case Edge::SEGMENT:
        type = "SEGMENT";
        break;
      case Edge::TECH_VIA:
        type = "TECH_VIA";
        break;
      case Edge::VIA:
        type = "VIA";
        break;
      case Edge::SHORT:
        type = "SHORT";
        break;
      case Edge::VWIRE:
        type = "VWIRE";
        break;
    }
    logger->report("Edge {:2}: {:8} {:2} -> {:2}",
                   index++,
                   type,
                   node2index.at(edge->source()),
                   node2index.at(edge->target()));
  }
}

}  // namespace odb
