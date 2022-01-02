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
#include <vector>
#include <unordered_map>
#include "architecture.h"
#include "orientation.h"
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

  void setHeight(double h) { m_h = h; }
  double getHeight() const { return m_h; }

  void setWidth(double w) { m_w = w; }
  double getWidth() const { return m_w; }

  double getArea() const { return m_w * m_h; }

  void setX(double x) { m_x = x; }
  double getX() const { return m_x; }

  void setY(double y) { m_y = y; }
  double getY() const { return m_y; }

  void setOrigX(double x) { m_origX = x; }
  double getOrigX() const { return m_origX; }

  void setOrigY(double y) { m_origY = y; }
  double getOrigY() const { return m_origY; }

  void setFixed(unsigned fixed) { m_fixed = fixed; }
  unsigned getFixed() const { return m_fixed; }

  void setType(unsigned type) { m_type = type; }
  unsigned getType() const { return m_type; }

  void setRegionId(int id) { m_regionId = id; }
  int getRegionId() const { return m_regionId; }

  void setCurrOrient(unsigned orient) { m_currentOrient = orient; }
  unsigned getCurrOrient() const { return m_currentOrient; }
  void setAvailOrient(unsigned avail) { m_availOrient = avail; }
  unsigned getAvailOrient( void ) const { return m_availOrient; }

  void setAttributes(unsigned attributes) { m_attributes = attributes; }
  unsigned getAttributes() const { return m_attributes; }
  void addAttribute(unsigned attribute) { m_attributes |= attribute; }
  void remAttribute(unsigned attribute) { m_attributes &= ~attribute; }

  bool isTerminal() const {
    return (m_type == NodeType_TERMINAL);
  }
  bool isTerminalNI() const {
    return (m_type == NodeType_TERMINAL_NI);
  }
  bool isFiller() const { 
    return (m_type == NodeType_FILLER);
  }
  bool isShape() const {
    return (m_type == NodeType_SHAPE);
  }
  bool isFixed() const {
    return (m_fixed != NodeFixed_NOT_FIXED);
  }

  void setLeftEdgeType(int etl) { m_etl = etl; }
  int getLeftEdgeType() const { return m_etl; }

  void setRightEdgeType(int etr) { m_etr = etr; }
  int getRightEdgeType() const { return m_etr; }

  void swapEdgeTypes() { std::swap<int>(m_etl, m_etr); }

  void setBottomPower(int bot) { m_powerBot = bot; }
  int getBottomPower() const { return m_powerBot; }

  void setTopPower(int top) { m_powerTop = top; }
  int getTopPower() const { return m_powerTop; }

  const std::vector<Pin*>& getPins() { return m_pins; }

 protected:
  // Id.
  int m_id;
  // Current position.
  double m_x;
  double m_y;
  // Original position.
  double m_origX;
  double m_origY;
  // Width and height.
  double m_w;
  double m_h;
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

  friend class Network;
};

class Edge {
 public:
  Edge();
  virtual ~Edge();

  int getId() const { return m_id; }
  void setId(int id) { m_id = id; }

  void setNdr( int ndr ) { m_ndr = ndr; }
  int getNdr( void ) const { return m_ndr; }

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

  void setPinWidth(double width) { m_pinW = width; }
  double getPinWidth() const { return m_pinW; }

  void setPinHeight(double height) { m_pinH = height; }
  double getPinHeight() const { return m_pinH; }

 protected:
  // Pin width and height.
  double m_pinW; 
  double m_pinH;   
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
  struct compareNodesByW {
    bool operator()(Node* ndi, Node* ndj) {
      if (ndi->getWidth() == ndj->getWidth()) {
        return ndi->getId() < ndj->getId();
      }
      return ndi->getWidth() < ndj->getWidth();
    }
  };

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
  class Shape {
   public:
    Shape() : m_llx(0.0), m_lly(0.0), m_w(0.0), m_h(0.0) { ; }
    Shape(double llx, double lly, double w, double h)
        : m_llx(llx), m_lly(lly), m_w(w), m_h(h) {
      ;
    }

   public:
    double m_llx;
    double m_lly;
    double m_w;
    double m_h;
  };

 public:
  Network();
  virtual ~Network();

  size_t getNumNodes() const { return m_nodes.size(); }
  Node* getNode(int i) { return &(m_nodes[i]); }
  void setNodeName(int i, std::string& name) { m_nodeNames[i] = name; }
  void setNodeName(int i, const char* name) { m_nodeNames[i] = name; }
  std::string& getNodeName(int i) { return m_nodeNames[i]; }

  size_t getNumEdges() const { return m_edges.size(); }
  Edge* getEdge(int i) { return &(m_edges[i]); }
  void setEdgeName(int i, std::string& name) { m_edgeNames[i] = name; }
  void setEdgeName(int i, const char* name) { m_edgeNames[i] = name; }
  std::string& getEdgeName(int i) { return m_edgeNames[i]; }

  // For building only.
  void resizeNodes(int nNodes) {
    m_nodes.resize(nNodes);
    m_shapes.resize(nNodes);
  }
  void resizeEdges(int nEdges) {
    m_edges.resize(nEdges);
  }

  // For creating and adding pins.
  Pin* createAndAddPin(Node* nd, Edge* ed);
  int getNumPins() const { return m_pins.size(); }

  size_t getNumFillerNodes() const { return m_filler.size(); }
  Node* getFillerNode(int i) const { return m_filler[i]; }
  void deleteFillerNodes();
  Node* createAndAddFillerNode(double x, double y, double width, double height);

  bool hasShapes(Node* ndi) const {
    if (ndi->getId() < m_shapes.size()) {
      return (m_shapes[ndi->getId()].size() != 0);
    }
    return false;
  }
  int getNumShapes(Node* ndi) const { 
    return (ndi->getId() < m_shapes.size())
        ? m_shapes[ndi->getId()].size()
        : 0;
  }
  Node* getShape(Node* ndi, int i) {
    return m_shapes[ndi->getId()][i];
  }
  Node* createAndAddShapeNode(Node* ndi,
    double x, double y, double width, double height);

 protected:
  // For creating and adding pins. 
  Pin* createAndAddPin();

  std::vector<Edge> m_edges;  // The edges in the netlist...
  std::unordered_map<int,std::string> m_edgeNames; // Names of edges...
  std::vector<Node> m_nodes;  // The nodes in the netlist...
  std::unordered_map<int,std::string> m_nodeNames; // Names of nodes...
  std::vector<Pin*> m_pins; // The pins in the network...

  std::vector<Node*> m_filler; // For filler...

  // Shapes for non-rectangular nodes...
  std::vector<std::vector<Node*> > m_shapes;
};

}  // namespace dpo
