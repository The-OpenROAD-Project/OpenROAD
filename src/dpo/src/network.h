///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

////////////////////////////////////////////////////////////////////////////////
// File: network.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "architecture.h"
#include "orientation.h"
#include "rectangle.h"
#include "symmetry.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Node;
class Edge;
class Pin;
class Network;
class Architecture;

const unsigned NodeType_UNKNOWN = 0x00000000;
const unsigned NodeType_CELL = 0x00000001;
const unsigned NodeType_TERMINAL = 0x00000002;
const unsigned NodeType_TERMINAL_NI = 0x00000004;
const unsigned NodeType_MACROCELL = 0x00000008;
const unsigned NodeType_FILLER = 0x00000010;
const unsigned NodeType_SHAPE = 0x00000020;

const unsigned NodeAttributes_EMPTY = 0x00000000;

const unsigned NodeFixed_NOT_FIXED = 0x00000000;
const unsigned NodeFixed_FIXED_X = 0x00000001;
const unsigned NodeFixed_FIXED_Y = 0x00000002;
const unsigned NodeFixed_FIXED_XY = 0x00000003;  // FIXED_X and FIXED_Y.

const int EDGETYPE_DEFAULT = 0;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Node {
 public:
  Node();
  virtual ~Node();

  int getId() const { return m_id; }
  void setId(int id) { m_id = id; }

  void setHeight(int h) { m_h = h; }
  int getHeight() const { return m_h; }

  void setWidth(int w) { m_w = w; }
  int getWidth() const { return m_w; }

  int getArea() const { return m_w * m_h; }

  void setLeft(int left) { m_left = left; }
  int getLeft() const { return m_left; }
  int getRight() const { return m_left + m_w; }

  void setBottom(int bottom) { m_bottom = bottom; }
  int getBottom() const { return m_bottom; }
  int getTop() const { return m_bottom + m_h; }

  void setOrigLeft(double left) { m_origLeft = left; }
  double getOrigLeft() const { return m_origLeft; }
  void setOrigBottom(double bottom) { m_origBottom = bottom; }
  double getOrigBottom() const { return m_origBottom; }

  void setFixed(unsigned fixed) { m_fixed = fixed; }
  unsigned getFixed() const { return m_fixed; }

  void setType(unsigned type) { m_type = type; }
  unsigned getType() const { return m_type; }

  void setRegionId(int id) { m_regionId = id; }
  int getRegionId() const { return m_regionId; }

  void setCurrOrient(unsigned orient) { m_currentOrient = orient; }
  unsigned getCurrOrient() const { return m_currentOrient; }
  void setAvailOrient(unsigned avail) { m_availOrient = avail; }
  unsigned getAvailOrient() const { return m_availOrient; }
  bool adjustCurrOrient(unsigned newOrient);

  void setAttributes(unsigned attributes) { m_attributes = attributes; }
  unsigned getAttributes() const { return m_attributes; }
  void addAttribute(unsigned attribute) { m_attributes |= attribute; }
  void remAttribute(unsigned attribute) { m_attributes &= ~attribute; }

  bool isTerminal() const { return (m_type == NodeType_TERMINAL); }
  bool isTerminalNI() const { return (m_type == NodeType_TERMINAL_NI); }
  bool isFiller() const { return (m_type == NodeType_FILLER); }
  bool isShape() const { return (m_type == NodeType_SHAPE); }
  bool isFixed() const { return (m_fixed != NodeFixed_NOT_FIXED); }

  void setLeftEdgeType(int etl) { m_etl = etl; }
  int getLeftEdgeType() const { return m_etl; }

  void setRightEdgeType(int etr) { m_etr = etr; }
  int getRightEdgeType() const { return m_etr; }

  void swapEdgeTypes() { std::swap<int>(m_etl, m_etr); }

  void setBottomPower(int bot) { m_powerBot = bot; }
  int getBottomPower() const { return m_powerBot; }

  void setTopPower(int top) { m_powerTop = top; }
  int getTopPower() const { return m_powerTop; }

  int getNumPins() const { return (int)m_pins.size(); }
  const std::vector<Pin*>& getPins() { return m_pins; }

  void setIsDefinedByShapes(bool val = false) { m_isDefinedByShapes = val; }
  bool isDefinedByShapes() const { return m_isDefinedByShapes; }

 protected:
  // Id.
  int m_id;
  // Current position; bottom corner.
  int m_left;
  int m_bottom;
  // Original position.  Stored as double still.
  double m_origLeft;
  double m_origBottom;
  // Width and height.
  int m_w;
  int m_h;
  // Type.
  unsigned m_type;
  // Fixed or not fixed.
  unsigned m_fixed;
  // Place for attributes.
  unsigned m_attributes;
  // For edge types and spacing tables.
  int m_etl;
  int m_etr;
  // For power.
  int m_powerTop;
  int m_powerBot;
  // Regions.
  int m_regionId;
  // Orientations.
  unsigned m_currentOrient;
  unsigned m_availOrient;
  // Pins.
  std::vector<Pin*> m_pins;
  // Shapes.  Legacy from bookshelf in which
  // some fixed macros are not rectangles
  // and are defined by sub-rectanges (shapes).
  bool m_isDefinedByShapes;

  friend class Network;
};

class Edge {
 public:
  Edge();
  virtual ~Edge();

  int getId() const { return m_id; }
  void setId(int id) { m_id = id; }

  void setNdr(int ndr) { m_ndr = ndr; }
  int getNdr() const { return m_ndr; }

  int getNumPins() const { return (int)m_pins.size(); }
  const std::vector<Pin*>& getPins() { return m_pins; }

 protected:
  // Id.
  int m_id;
  // Refer to routing rule stored elsewhere.
  int m_ndr;
  // Pins.
  std::vector<Pin*> m_pins;

  friend class Network;
};

class Pin {
 public:
  enum Direction { Dir_IN, Dir_OUT, Dir_INOUT, Dir_UNKNOWN };

 public:
  Pin();
  virtual ~Pin();

  void setDirection(int dir) { m_dir = dir; }
  int getDirection() const { return m_dir; }

  Node* getNode() const { return m_node; }
  Edge* getEdge() const { return m_edge; }

  void setOffsetX(double offsetX) { m_offsetX = offsetX; }
  double getOffsetX() const { return m_offsetX; }

  void setOffsetY(double offsetY) { m_offsetY = offsetY; }
  double getOffsetY() const { return m_offsetY; }

  void setPinLayer(int layer) { m_pinLayer = layer; }
  int getPinLayer() const { return m_pinLayer; }

  void setPinWidth(double width) { m_pinWidth = width; }
  double getPinWidth() const { return m_pinWidth; }

  void setPinHeight(double height) { m_pinHeight = height; }
  double getPinHeight() const { return m_pinHeight; }

 protected:
  // Pin width and height.
  double m_pinWidth;
  double m_pinHeight;
  // Direction.
  int m_dir;
  // Layer.
  int m_pinLayer;
  // Node and edge for pin.
  Node* m_node;
  Edge* m_edge;
  // Offsets from cell center.
  double m_offsetX;
  double m_offsetY;

  friend class Network;
};

class Network {
 public:
  struct comparePinsByNodeId {
    bool operator()(const Pin* a, const Pin* b) {
      return a->getNode()->getId() < b->getNode()->getId();
    }
  };

  class comparePinsByEdgeId {
   public:
    comparePinsByEdgeId() : m_nw(0) {}
    comparePinsByEdgeId(Network* nw) : m_nw(nw) {}
    bool operator()(const Pin* a, const Pin* b) {
      return a->getEdge()->getId() < b->getEdge()->getId();
    }
    Network* m_nw;
  };

  class comparePinsByOffset {
   public:
    comparePinsByOffset() : m_nw(0) {}
    comparePinsByOffset(Network* nw) : m_nw(nw) {}
    bool operator()(const Pin* a, const Pin* b) {
      if (a->getOffsetX() == b->getOffsetX()) {
        return a->getOffsetY() < b->getOffsetY();
      }
      return a->getOffsetX() < b->getOffsetX();
    }
    Network* m_nw;
  };

 public:
 public:
  Network();
  virtual ~Network();

  int getNumNodes() const { return (int)m_nodes.size(); }
  Node* getNode(int i) { return m_nodes[i]; }
  void setNodeName(int i, std::string& name) { m_nodeNames[i] = name; }
  void setNodeName(int i, const char* name) { m_nodeNames[i] = name; }
  std::string& getNodeName(int i) { return m_nodeNames[i]; }

  int getNumEdges() const { return (int)m_edges.size(); }
  Edge* getEdge(int i) { return m_edges[i]; }
  void setEdgeName(int i, std::string& name) { m_edgeNames[i] = name; }
  void setEdgeName(int i, const char* name) { m_edgeNames[i] = name; }
  std::string& getEdgeName(int i) { return m_edgeNames[i]; }

  int getNumPins() const { return (int)m_pins.size(); }

  // For creating and adding pins.
  Pin* createAndAddPin(Node* nd, Edge* ed);

  // For creating and adding cells.
  Node* createAndAddNode();  // Network cells.
  Node* createAndAddShapeNode(
      int left, int bottom, int width,
      int height);  // Extras for non-rectangular shapes.
  Node* createAndAddFillerNode(
      int left, int bottom, int width,
      int height);  // Extras to block space.

  // For creating and adding edges.
  Edge* createAndAddEdge();

 protected:
  Pin* createAndAddPin();

 protected:
  std::vector<Edge*> m_edges;  // The edges in the netlist...
  std::unordered_map<int, std::string> m_edgeNames;  // Names of edges...
  std::vector<Node*> m_nodes;  // The nodes in the netlist...
  std::unordered_map<int, std::string> m_nodeNames;  // Names of nodes...
  std::vector<Pin*> m_pins;  // The pins in the network...
};

}  // namespace dpo
