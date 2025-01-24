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

#include "dbTypes.h"
#include "geom.h"
#include "odb.h"
#include "odbDList.h"

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
//   The graph may contains a fortest of trees.
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
    // End-Types:
    //     EXTENDED - segment extended 1/2 path-width
    //     VARIABLE - segment extended variable amount
    enum Type
    {
      EXTENDED,
      VARIABLE
    };

    void setExtended()
    {
      _type = EXTENDED;
      _ext = 0;
    }

    void setVariable(int ext)
    {
      _type = VARIABLE;
      _ext = ext;
    }

    bool operator==(const EndStyle& s) const
    {
      return (_type == s._type) && (_ext == s._ext);
    }

    bool operator!=(const EndStyle& s) const
    {
      return (_type != s._type) || (_ext != s._ext);
    }

   private:
    Type _type{EXTENDED};
    int _ext{0};

    friend class dbWireGraph;
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

    Edge(Type type, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : _type(type), _wire_type(wire_type), _non_default_rule(rule)
    {
    }

    virtual ~Edge() = default;

    Type type() const { return _type; }
    Node* source() const { return _src; }
    Node* target() const { return _tgt; }
    dbWireType::Value wireType() const { return _wire_type; }
    dbTechLayerRule* nonDefaultRule() const { return _non_default_rule; }

   private:
    static DListEntry<Edge>* edgeEntry(Edge* edge)
    {
      return &edge->_edge_entry;
    }

    static DListEntry<Edge>* outEdgeEntry(Edge* edge)
    {
      return &edge->_out_edge_entry;
    }

    const Type _type;
    Node* _src{nullptr};
    Node* _tgt{nullptr};
    dbWireType::Value _wire_type;
    dbTechLayerRule* _non_default_rule;
    DListEntry<Edge> _edge_entry;
    DListEntry<Edge> _out_edge_entry;

    friend class dbWireGraph;
  };

  // Node - A Node represents a point "(x y layer)" in the graph.
  class Node
  {
   public:
    using edge_iterator = DList<Edge, &Edge::outEdgeEntry>::iterator;

    Node(int x, int y, dbTechLayer* layer) : _x(x), _y(y), _layer(layer) {}

    void xy(int& x, int& y) const
    {
      x = _x;
      y = _y;
    }
    Point point() const { return {_x, _y}; }
    dbTechLayer* layer() const { return _layer; }
    Edge* in_edge() const { return _in_edge; }
    edge_iterator begin() { return _out_edges.begin(); }
    edge_iterator end() { return _out_edges.end(); }
    dbObject* object() const { return _object; }
    void setObject(dbObject* obj) { _object = obj; }

   private:
    static DListEntry<Node>* nodeEntry(Node* node)
    {
      return &node->_node_entry;
    }

    int _x;
    int _y;
    int _jct_id{-1};
    dbTechLayer* _layer;
    Edge* _in_edge{nullptr};
    dbObject* _object{nullptr};
    DList<Edge, &Edge::outEdgeEntry> _out_edges;
    DListEntry<Node> _node_entry;

    friend class dbWireGraph;
  };

  // VIA edge
  class Via : public Edge
  {
   public:
    Via(dbVia* via, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(VIA, wire_type, rule), _via(via)
    {
    }

    dbVia* via() const { return _via; }

   private:
    dbVia* _via;
    friend class dbWireGraph;
  };

  // TECH-VIA edge
  class TechVia : public Edge
  {
   public:
    TechVia(dbTechVia* via, dbWireType::Value wire_type, dbTechLayerRule* rule)
        : Edge(TECH_VIA, wire_type, rule), _via(via)
    {
    }

    dbTechVia* via() const { return _via; }

   private:
    dbTechVia* _via;
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
        : Edge(SEGMENT, wire_type, rule), _src_style(src), _tgt_style(tgt)
    {
    }

   private:
    EndStyle _src_style;
    EndStyle _tgt_style;
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

  dbWireGraph();
  ~dbWireGraph();

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
  Edge* getEdge(uint shape_id);

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
  node_iterator begin_nodes() { return _nodes.begin(); }
  node_iterator end_nodes() { return _nodes.end(); }

  // Node iterator
  using edge_iterator = DList<Edge, &Edge::edgeEntry>::iterator;
  edge_iterator begin_edges() { return _edges.begin(); }
  edge_iterator end_edges() { return _edges.end(); }

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

  std::vector<Node*> _junction_map;
  DList<Node, &Node::nodeEntry> _nodes;
  DList<Edge, &Edge::edgeEntry> _edges;
};

inline dbWireGraph::Segment* dbWireGraph::createSegment(Node* src,
                                                        Node* tgt,
                                                        dbWireType::Value type,
                                                        dbTechLayerRule* rule)
{
  return createSegment(src, tgt, EndStyle(), EndStyle(), type, rule);
}

}  // namespace odb
