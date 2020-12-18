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
#include "dbRtNode.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

class dbWire;
class dbVia;
class dbTechVia;
class dbObject;
class dbTechLayer;
class dbTechLayerRule;
class dbWireEncoder;

class dbRtEdge;
class dbRtNode;
class dbRtSegment;
class dbRtTechVia;
class dbRtBlockVia;
class dbRtShort;
class dbRtVWire;

//
// dbRtTree - This class is used to represent an encoded dbWire as a graph.
//
//     The graph is undirected and cannot contain cycles.
//
//   Patch wires (rects) are not exposed in the tree.
//
class dbRtTree
{
 public:
  dbRtTree();
  ~dbRtTree();

  // Clear this graph.
  void clear();

  // Decode a dbWire into this graph. Overwrites previous graph.
  void decode(dbWire* wire, bool decode_bterm_iterms = true);

  // Encode this graph into the dbWire. The contents of the dbWire are over
  // written.
  void encode(dbWire* wire, bool encode_bterm_iterms = true);

  // Get the edge of this shape_id.
  // PRECONDITION: The "target" node of this edge must exist (do not delete this
  // node!).
  dbRtEdge* getEdge(uint shape_id);

  // Node Create Method
  dbRtNode* createNode(int x, int y, dbTechLayer* l);

  // Create a via edge.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  //    Returns NULL, if the src and tgt layers are not the respective upper and
  //    lower layers of this via.
  dbRtVia* createVia(dbRtNode*         src,
                     dbRtNode*         tgt,
                     dbVia*            via,
                     dbWireType::Value type = dbWireType::NONE,
                     dbTechLayerRule*  rule = NULL);

  // Create a tech-via edge.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  //    Returns NULL, if the src and tgt layers are not the respective upper and
  //    lower layers of this via.
  dbRtTechVia* createTechVia(dbRtNode*         src,
                             dbRtNode*         tgt,
                             dbTechVia*        via,
                             dbWireType::Value type = dbWireType::NONE,
                             dbTechLayerRule*  rule = NULL);

  // Create a segment edge with the default (EXTENDED) end style.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  //    Returns NULL, if the src and tgt layer are not the same.
  //    Returns NULL, if the src and tgt point do not form an orthogonal
  //    segment.
  dbRtSegment* createSegment(dbRtNode*         src,
                             dbRtNode*         tgt,
                             dbWireType::Value type = dbWireType::NONE,
                             dbTechLayerRule*  rule = NULL);

  // Create a segment edge.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  //    Returns NULL, if the src and tgt layer are not the same.
  //    Returns NULL, if the src and tgt point do not form an orthogonal
  //    segment.
  dbRtSegment* createSegment(dbRtNode*         src,
                             dbRtNode*         tgt,
                             dbRtEndStyle      src_style,
                             dbRtEndStyle      tgt_style,
                             dbWireType::Value type = dbWireType::NONE,
                             dbTechLayerRule*  rule = NULL);

  // Create a short edge.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  dbRtShort* createShort(dbRtNode*         src,
                         dbRtNode*         tgt,
                         dbWireType::Value type = dbWireType::NONE,
                         dbTechLayerRule*  rule = NULL);

  // Create a vwire edge.
  //    Returns NULL, if the "tgt" node already has an in-edge.
  dbRtVWire* createVWire(dbRtNode*         src,
                         dbRtNode*         tgt,
                         dbWireType::Value type = dbWireType::NONE,
                         dbTechLayerRule*  rule = NULL);

  // Node iterator
  typedef DList<dbRtNode, &dbRtNode::rtNode>::iterator node_iterator;

  node_iterator begin_nodes() { return _nodes.begin(); }
  node_iterator end_nodes() { return _nodes.end(); }

  // Edge iterator
  typedef DList<dbRtEdge, &dbRtEdge::rtEdge>::iterator edge_iterator;
  edge_iterator begin_edges() { return _edges.begin(); }
  edge_iterator end_edges() { return _edges.end(); }

  // Delete the specified node
  void deleteNode(dbRtNode* n);

  // Delete the specified edge, does not delete orphan nodes
  void deleteEdge(dbRtEdge* e);

  // Delete the specified edge, does not delete orphan nodes
  edge_iterator deleteEdge(edge_iterator itr)
  {
    dbRtEdge* e = *itr;
    ++itr;
    deleteEdge(e);
    return itr;
  }

  // Delete the specified edge, optionally deletes orphan nodes
  void deleteEdge(dbRtEdge* e, bool destroy_orphan_nodes);

  // Delete the specified edge, optionally deletes orphan nodes
  edge_iterator deleteEdge(edge_iterator itr, bool destroy_orphan_nodes)
  {
    dbRtEdge* e = *itr;
    ++itr;
    deleteEdge(e, destroy_orphan_nodes);
    return itr;
  }

  // Make a copy of this graph
  dbRtTree* duplicate();

  // Move the tree "T" into this tree
  // All the nodes/edges of "T" will be moved to this tree.
  void move(dbRtTree* T);

  // Copy the tree "T" into this tree
  // All the nodes/edges of "T" will be copied to this tree.
  void copy(dbRtTree* T);

  // Make a duplicate copy of this node (does not duplicate edges) in the
  // specified tree
  static dbRtNode* duplicate(dbRtTree* G,
                             dbRtNode* node,
                             bool      copy_objects = true);

  // Make a duplicate copy of this edge between src and target in the specified
  // tree
  static dbRtEdge* duplicate(dbRtTree* G,
                             dbRtEdge* edge,
                             dbRtNode* src,
                             dbRtNode* tgt);

 private:
  void encodePath(dbWireEncoder&          encoder,
                  std::vector<dbRtEdge*>& path,
                  dbRtNode*               src,
                  dbWireType::Value       cur_type,
                  dbTechLayerRule*        cur_rule,
                  bool                    encode_bterms_iterms);
  void encodePath(dbWireEncoder&          encoder,
                  std::vector<dbRtEdge*>& path,
                  bool                    encode_bterms_iterms);
  void add_node(dbRtNode* node);
  void add_edge(dbRtEdge* edge);
  void remove_node(dbRtNode* node);
  void remove_edge(dbRtEdge* edge);
  void addObjects(dbWireEncoder& encoder, dbRtNode* node);

  static void copyNode(dbRtTree* G,
                       dbRtNode* node,
                       dbRtNode* src,
                       bool      copy_edge_map);
  static void copyEdge(dbRtTree* G,
                       dbRtNode* src,
                       dbRtNode* tgt,
                       dbRtEdge* edge,
                       bool      copy_edge_map);

  std::vector<dbRtEdge*>             _edge_map;
  DList<dbRtNode, &dbRtNode::rtNode> _nodes;
  DList<dbRtEdge, &dbRtEdge::rtEdge> _edges;
};

inline dbRtSegment* dbRtTree::createSegment(dbRtNode*         src,
                                            dbRtNode*         tgt,
                                            dbWireType::Value type,
                                            dbTechLayerRule*  rule)
{
  return createSegment(src, tgt, dbRtEndStyle(), dbRtEndStyle(), type, rule);
}

}  // namespace odb
