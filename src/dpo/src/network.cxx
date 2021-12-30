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
// File: network.cxx
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <deque>
#include <string>

#include "network.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
Node::Node()
    : m_id(0),
      m_x(0.0),
      m_y(0.0),
      m_origX(0.0),
      m_origY(0.0),
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
      m_availOrient(Orientation_N) {
}
Node::~Node() {}

Edge::Edge() : m_id(0), m_ndr(0) {}

Edge::~Edge() {}

Pin::Pin()
    : m_pinW(0.0),
      m_pinH(0.0),
      m_dir(Pin::Dir_INOUT),
      m_pinLayer(0),  
      m_node(nullptr),
      m_edge(nullptr),
      m_offsetX(0.0),
      m_offsetY(0.0) {
}

Pin::~Pin() {}

Network::Network() {}

Network::~Network() {
  deleteFillerNodes();

  for (int i = 0; i < m_shapes.size(); i++) {
    for (int j = 0; j < m_shapes[i].size(); j++) {
      delete m_shapes[i][j];
    }
    m_shapes[i].clear();
  }
  m_shapes.clear();

  m_nodeNames.clear();
  m_edgeNames.clear();
  m_nodes.clear();
  m_edges.clear();

  for (int i = 0; i < m_pins.size(); i++) {
    delete m_pins[i];
  }
  m_pins.clear();
}

void Network::deleteFillerNodes() {
  for (int i = 0; i < m_filler.size(); i++) {
    delete m_filler[i];
  }
  m_filler.clear();
}

Node* Network::createAndAddFillerNode(double x,
    double y, double width, double height) {
  Node* ndi = new Node();
  ndi->setFixed(NodeFixed_FIXED_XY);
  ndi->setType(NodeType_FILLER);
  int id = m_nodes.size() + m_filler.size();
  ndi->setId(id);
  ndi->setHeight(height);
  ndi->setWidth(width);
  ndi->setY(y);
  ndi->setX(x);

  m_filler.push_back(ndi);
  return ndi;
}

Node* Network::createAndAddShapeNode(Node* ndi,
    double x, double y, double width, double height) {
  Node* shape = new Node();
  shape->setFixed(NodeFixed_FIXED_XY);
  shape->setType(NodeType_SHAPE);
  shape->setId(-1);
  shape->setHeight(height);
  shape->setWidth(width);
  shape->setY(y);
  shape->setX(x);

  m_shapes[ndi->getId()].push_back(shape);
  return shape;
}

Pin* Network::createAndAddPin() {
  Pin* ptr = new Pin();
  m_pins.push_back(ptr);
  return ptr;
}

Pin* Network::createAndAddPin(Node* nd, Edge* ed) {
  Pin* ptr = createAndAddPin();
  ptr->m_node = nd;
  ptr->m_edge = ed;
  ptr->m_node->m_pins.push_back(ptr);
  ptr->m_edge->m_pins.push_back(ptr);
  return ptr;
}

}  // namespace dpo
