// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <vector>

#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odbDList.h"
#include "utl/Logger.h"

namespace odb {

class dbWire;
class dbVia;
class dbTechVia;
class dbObject;
class dbTechLayer;
class dbTechLayerRule;
class dbWireEncoder;

//
// dbWireGraph - This class is used to represent an encoded dbWire as a graph.
//
//    The graph is directed and acyclic (DAG).
//    Each node has one incoming edge and 1 or more outgoing edges.
//    Nodes which have no in-edge are root nodes.
//    Nodes which have no out-edges are leaf nodes.
//
//     o------>o------>o---->o
//             |
//             +----->o----->o
//                    |
//                    +----->o
//
//   The graph may contains a forest of trees.
//
//   This graph may be edited and encoded back to the wire.
//
//   Patch wires (rects) are not exposed in the graph.
//
//
class dbWireGraph
{
 public:
  // EndStyle - Defines the end style of a segment
  class EndStyle
  {
   public:
    enum Type
    {
      EXTENDED,  // segment extended 1/2 path-width
      VARIABLE   // segment extended variable amount
    };

    Type getType() const { return type_; }
    int getExt() const { return ext_; }

    void setExtended()
    {
      type_ = EXTENDED;
      ext_ = 0;
    }

    void setVariable(int ext)
    {
      type_ = VARIABLE;
      ext_ = ext;
    }

    bool operator==(const EndStyle& s) const
    {
      return (type_ == s.type_) && (ext_ == s.ext_);
    }

    bool operator!=(const EndStyle& s) const
    {
      return (type_ != s.type_) || (ext_ != s.ext_);
    }

   private:
    Type type_{EXTENDED};
    int ext_{0};
  };

  class Node;

  // Edge - Represents wire segments, vias, or shorts in the graph.
  class Edge
  {
   public:
    enum Type
    {
      SEGMENT,
      TECH_VIA,
      VIA,
      SHORT,
      VWIRE
    };

    virtual ~Edge() = default;

    Type type() const { return type_; }
    Node* source() const { return src_; }
    Node* target() const { return tgt_; }
    dbWireType::Value wireType() const { return wire_type_; }
    dbTechLayerRule* nonDefaultRule() const { return non_default_rule_; }

   protected:
    Edge(Type type, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : type_(type), wire_type_(wire_type), non_default_rule_(rule)
    {
    }

   private:
    static DListEntry<Edge>* edgeEntry(Edge* edge)
    {
      return &edge->edge_entry_;
    }

    static DListEntry<Edge>* outEdgeEntry(Edge* edge)
    {
      return &edge->_out_edge_entry_;
    }

    const Type type_;
    Node* src_{nullptr};
    Node* tgt_{nullptr};
    dbWireType::Value wire_type_;
    dbTechLayerRule* non_default_rule_;
    DListEntry<Edge> edge_entry_;
    DListEntry<Edge> _out_edge_entry_;

    friend class dbWireGraph;
  };

  // Node - A Node represents a point "(x y layer)" in the graph.
  class Node
  {
   public:
    using edge_iterator = DList<Edge, &Edge::outEdgeEntry>::iterator;

    Node(int x, int y, dbTechLayer* layer) : x_(x), y_(y), layer_(layer) {}

    void xy(int& x, int& y) const
    {
      x = x_;
      y = y_;
    }
    Point point() const { return {x_, y_}; }
    dbTechLayer* layer() const { return layer_; }
    Edge* in_edge() const { return in_edge_; }
    edge_iterator begin() { return out_edges_.begin(); }
    edge_iterator end() { return out_edges_.end(); }
    dbObject* object() const { return object_; }
    void setObject(dbObject* obj) { object_ = obj; }

   private:
    static DListEntry<Node>* nodeEntry(Node* node)
    {
      return &node->node_entry_;
    }

    int x_;
    int y_;
    int jct_id_{-1};
    dbTechLayer* layer_;
    Edge* in_edge_{nullptr};
    dbObject* object_{nullptr};
    DList<Edge, &Edge::outEdgeEntry> out_edges_;
    DListEntry<Node> node_entry_;

    friend class dbWireGraph;
  };

  // VIA edge
  class Via : public Edge
  {
   public:
    Via(dbVia* via, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(VIA, wire_type, rule), via_(via)
    {
    }

    dbVia* via() const { return via_; }

   private:
    dbVia* via_;
    friend class dbWireGraph;
  };

  // TECH-VIA edge
  class TechVia : public Edge
  {
   public:
    TechVia(dbTechVia* via, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(TECH_VIA, wire_type, rule), via_(via)
    {
    }

    dbTechVia* via() const { return via_; }

   private:
    dbTechVia* via_;
    friend class dbWireGraph;
  };

  // Wire segment
  class Segment : public Edge
  {
   public:
    Segment(EndStyle src,
            EndStyle tgt,
            dbWireType::Value wire_type,
            dbTechLayerRule* rule)
        : Edge(SEGMENT, wire_type, rule), src_style_(src), tgt_style_(tgt)
    {
    }

   private:
    EndStyle src_style_;
    EndStyle tgt_style_;
    friend class dbWireGraph;
  };

  // Virtual short to node
  class Short : public Edge
  {
   public:
    Short(dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(SHORT, wire_type, rule)
    {
    }

    friend class dbWireGraph;
  };

  // Virtual wire between nodes
  class VWire : public Edge
  {
   public:
    VWire(dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(VWIRE, wire_type, rule)
    {
    }

    friend class dbWireGraph;
  };

  dbWireGraph() = default;

  // Clear this graph.
  void clear();

  // Decode a dbWire into this graph. Overwrites previous graph.
  void decode(dbWire* wire);

  // Encode this graph into the dbWire. The contents of the dbWire are over
  // written.
  void encode(dbWire* wire);

  // Get the edge of this shape_id.
  // PRECONDITION: The "target" node of this edge must exist (do not delete this
  // node!).
  Edge* getEdge(uint32_t shape_id);

  // Node Create Method
  Node* createNode(int x, int y, dbTechLayer* l);

  // Create a via edge.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  //    Returns nullptr, if the src and tgt layers are not the respective upper
  //    and lower layers of this via.
  Via* createVia(Node* src,
                 Node* tgt,
                 dbVia* via,
                 dbWireType::Value type = dbWireType::NONE,
                 dbTechLayerRule* rule = nullptr);

  // Create a tech-via edge.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  //    Returns nullptr, if the src and tgt layers are not the respective upper
  //    and lower layers of this via.
  TechVia* createTechVia(Node* src,
                         Node* tgt,
                         dbTechVia* via,
                         dbWireType::Value type = dbWireType::NONE,
                         dbTechLayerRule* rule = nullptr);

  // Create a segment edge with the default (EXTENDED) end style.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  //    Returns nullptr, if the src and tgt layer are not the same.
  //    Returns nullptr, if the src and tgt point do not form an orthogonal
  //    segment.
  Segment* createSegment(Node* src,
                         Node* tgt,
                         dbWireType::Value type = dbWireType::NONE,
                         dbTechLayerRule* rule = nullptr);

  // Create a segment edge.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  //    Returns nullptr, if the src and tgt layer are not the same.
  //    Returns nullptr, if the src and tgt point do not form an orthogonal
  //    segment.
  Segment* createSegment(Node* src,
                         Node* tgt,
                         EndStyle src_style,
                         EndStyle tgt_style,
                         dbWireType::Value type = dbWireType::NONE,
                         dbTechLayerRule* rule = nullptr);

  // Create a short edge.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  Short* createShort(Node* src,
                     Node* tgt,
                     dbWireType::Value type = dbWireType::NONE,
                     dbTechLayerRule* rule = nullptr);

  // Create a vwire edge.
  //    Returns nullptr, if the "tgt" node already has an in-edge.
  VWire* createVWire(Node* src,
                     Node* tgt,
                     dbWireType::Value type = dbWireType::NONE,
                     dbTechLayerRule* rule = nullptr);

  // Edge iterator
  using node_iterator = DList<Node, &Node::nodeEntry>::iterator;
  node_iterator begin_nodes() { return nodes_.begin(); }
  node_iterator end_nodes() { return nodes_.end(); }

  // Node iterator
  using edge_iterator = DList<Edge, &Edge::edgeEntry>::iterator;
  edge_iterator begin_edges() { return edges_.begin(); }
  edge_iterator end_edges() { return edges_.end(); }

  // Delete Node
  // The deletion of a node WILL delete the connected edges.
  void deleteNode(Node* n);
  node_iterator deleteNode(node_iterator itr);

  // Delete Edge
  // The deletion of a edge WILL NOT delete the src/tgt node.
  void deleteEdge(Edge* e);
  edge_iterator deleteEdge(edge_iterator itr);

  void dump(utl::Logger* logger);

 private:
  void encodePath(dbWireEncoder& encoder,
                  std::vector<Edge*>& path,
                  Node* src,
                  dbWireType::Value cur_type,
                  dbTechLayerRule* cur_rule);
  void encodePath(dbWireEncoder& encoder, std::vector<Edge*>& path);

  std::vector<Node*> junction_map_;
  DList<Node, &Node::nodeEntry> nodes_;
  DList<Edge, &Edge::edgeEntry> edges_;
};

inline dbWireGraph::Segment* dbWireGraph::createSegment(Node* src,
                                                        Node* tgt,
                                                        dbWireType::Value type,
                                                        dbTechLayerRule* rule)
{
  return createSegment(src, tgt, EndStyle(), EndStyle(), type, rule);
}

}  // namespace odb
