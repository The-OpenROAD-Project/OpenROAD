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
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <deque>
#include <string>

#include "network.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node::Node()
    : m_id(0),
      m_left(0),
      m_bottom(0),
      m_origLeft(0.0),
      m_origBottom(0.0),
      m_w(0.0),
      m_h(0.0),
      m_type(0),
      m_fixed(NodeFixed_NOT_FIXED),
      m_attributes(NodeAttributes_EMPTY),
      m_etl(EDGETYPE_DEFAULT),
      m_etr(EDGETYPE_DEFAULT),
      m_powerTop(dpo::RowPower_UNK),
      m_powerBot(dpo::RowPower_UNK),
      m_regionId(0),
      m_currentOrient(Orientation_N),
      m_availOrient(Orientation_N),
      m_isDefinedByShapes(false)
{
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node::~Node() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Node::adjustCurrOrient(unsigned newOri) {
  // Change the orientation of the cell, but leave the lower-left corner
  // alone.  This means changing the locations of pins and possibly
  // changing the edge types as well as the height and width.
  unsigned curOri = m_currentOrient;
  if (newOri == curOri) {
    return true;
  }

  if (curOri == Orientation_E || curOri == Orientation_FE || curOri == Orientation_FW || curOri == Orientation_W) {
    if (newOri == Orientation_N || curOri == Orientation_FN || curOri == Orientation_FS || curOri == Orientation_S) {
      // Rotate the cell counter-clockwise by 90 degrees.
      for (int pi = 0; pi < m_pins.size(); pi++) {
        Pin* pin = m_pins[pi];
        double dx = pin->getOffsetX();
        double dy = pin->getOffsetY();
        pin->setOffsetX(-dy);
        pin->setOffsetY(dx);
      }
      std::swap(m_h, m_w);
      if (curOri == Orientation_E) { curOri = Orientation_N; }
      else if (curOri == Orientation_FE) { curOri = Orientation_FS; }
      else if (curOri == Orientation_FW) { curOri = Orientation_FN; }
      else { curOri = Orientation_S; }
    }
  }
  else {
    if (newOri == Orientation_E || curOri == Orientation_FE || curOri == Orientation_FW || curOri == Orientation_W) {
      // Rotate the cell clockwise by 90 degrees.
      for (int pi = 0; pi < m_pins.size(); pi++) {
        Pin* pin = m_pins[pi];
        double dx = pin->getOffsetX();
        double dy = pin->getOffsetY();
        pin->setOffsetX(dy);
        pin->setOffsetY(-dx);
      }
      std::swap(m_h, m_w);
      if (curOri == Orientation_N) { curOri = Orientation_E; }
      else if (curOri == Orientation_FS) { curOri = Orientation_FE; }
      else if (curOri == Orientation_FN) { curOri = Orientation_FW; }
      else { curOri = Orientation_W; }
    }
  }
  // Both the current and new orientations should be {N, FN, FS, S} or {E, FE, FW, W}.
  int mX = 1;
  int mY = 1;
  bool changeEdgeTypes = false;
  if (curOri == Orientation_E || curOri == Orientation_FE || curOri == Orientation_FW || curOri == Orientation_W) {
    bool test1 = (curOri == Orientation_E || curOri == Orientation_FW);
    bool test2 = (newOri == Orientation_E || newOri == Orientation_FW);
    if (test1 != test2) {
      mX = -1;
    }
    bool test3 = (curOri == Orientation_E || curOri == Orientation_FE);
    bool test4 = (newOri == Orientation_E || newOri == Orientation_FE);
    if (test3 != test4) {
      changeEdgeTypes = true;
      mY = -1;
    }
  }
  else {
    bool test1 = (curOri == Orientation_N || curOri == Orientation_FS);
    bool test2 = (newOri == Orientation_N || newOri == Orientation_FS);
    if (test1 != test2) {
      changeEdgeTypes = true;
      mX = -1;
    }
    bool test3 = (curOri == Orientation_N || curOri == Orientation_FN);
    bool test4 = (newOri == Orientation_N || newOri == Orientation_FN);
    if (test3 != test4) {
      mY = -1;
    }
  }

  for (int pi = 0; pi < m_pins.size(); pi++) {
    Pin* pin = m_pins[pi];
    pin->setOffsetX(pin->getOffsetX()*mX);
    pin->setOffsetY(pin->getOffsetY()*mY);
  }
  if (changeEdgeTypes) {
    std::swap(m_etl, m_etr);
  }
  m_currentOrient = newOri;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Edge::Edge() : m_id(0), m_ndr(0) {}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Edge::~Edge() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin::Pin(void)
    : m_pinWidth(0),
      m_pinHeight(0),
      m_dir(Pin::Dir_INOUT),
      m_pinLayer(0),
      m_node(nullptr),
      m_edge(nullptr),
      m_offsetX(0.0),
      m_offsetY(0.0) {}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin::~Pin(void) {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Network::Network() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Network::~Network() {
  // Delete edges.
  for (int i = 0; i < m_edges.size(); i++) {
    delete m_edges[i];
  }
  m_edges.clear();
  m_edgeNames.clear();

  // Delete cells.
  for (int i = 0; i < m_nodes.size(); i++) {
    delete m_nodes[i];
  }
  m_nodes.clear();
  m_nodeNames.clear();

  // Delete pins.
  for (int i = 0; i < m_pins.size(); i++) {
    delete m_pins[i];
  }
  m_pins.clear();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Edge* Network::createAndAddEdge(void) {
  // Just allocate an edge, append it and give it the id
  // that corresponds to its index.
  int id = (int)m_edges.size();
  Edge* ptr = new Edge();
  ptr->setId(id);
  m_edges.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node* Network::createAndAddNode(void) {
  // Just allocate a node, append it and give it the id
  // that corresponds to its index.
  int id = (int)m_nodes.size();
  Node* ptr = new Node();
  ptr->setId(id);
  m_nodes.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node* Network::createAndAddFillerNode(int left, int bottom, int width,
                                      int height) {
  Node* ndi = new Node();
  int id = (int)m_nodes.size();
  ndi->setFixed(NodeFixed_FIXED_XY);
  ndi->setType(NodeType_FILLER);
  ndi->setId(id);
  ndi->setHeight(height);
  ndi->setWidth(width);
  ndi->setBottom(bottom);
  ndi->setLeft(left);
  m_nodes.push_back(ndi);
  return getNode(id);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node* Network::createAndAddShapeNode(int left, int bottom, int width, 
                                     int height) {
  // Add shape cells to list of network cells.  We have
  // the parent node from which we can derive a name.
  Node* ndi = new Node();
  int id = (int)m_nodes.size();
  ndi->setFixed(NodeFixed_FIXED_XY);
  ndi->setType(NodeType_SHAPE);
  ndi->setId(id);
  ndi->setHeight(height);
  ndi->setWidth(width);
  ndi->setBottom(bottom);
  ndi->setLeft(left);
  m_nodes.push_back(ndi);
  return getNode(id);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin* Network::createAndAddPin(void) {
  Pin* ptr = new Pin();
  m_pins.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin* Network::createAndAddPin(Node* nd, Edge* ed) {
  Pin* ptr = createAndAddPin();
  ptr->m_node = nd;
  ptr->m_edge = ed;
  ptr->m_node->m_pins.push_back(ptr);
  ptr->m_edge->m_pins.push_back(ptr);
  return ptr;
}

}  // namespace dpo
