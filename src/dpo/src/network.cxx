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
      m_firstPin(0),
      m_lastPin(0),
      m_type(0),
      m_fixed(NodeFixed_NOT_FIXED),
      m_attributes(NodeAttributes_EMPTY),
      m_currentOrient(Orientation_N),
      m_availOrient(Orientation_N),
      m_x(0.0),
      m_y(0.0),
      m_w(0.0),
      m_h(0.0),
      m_etl(EDGETYPE_DEFAULT),
      m_etr(EDGETYPE_DEFAULT),
      m_regionId(0) {
  m_powerTop = dpo::RowPower_UNK;
  m_powerBot = dpo::RowPower_UNK;
}
Node::Node(const Node& other)
    : m_id(other.m_id),
      m_firstPin(other.m_firstPin),
      m_lastPin(other.m_lastPin),
      m_type(other.m_type),
      m_fixed(other.m_fixed),
      m_attributes(other.m_attributes),
      m_currentOrient(other.m_currentOrient),
      m_availOrient(other.m_availOrient),
      m_x(other.m_x),
      m_y(other.m_y),
      m_w(other.m_w),
      m_h(other.m_h),
      m_etl(other.m_etl),
      m_etr(other.m_etr),
      m_regionId(other.m_regionId) {
  m_powerTop = dpo::RowPower_UNK;
  m_powerBot = dpo::RowPower_UNK;
}
Node& Node::operator=(const Node& other) {
  if (this != &other) {
    m_id = other.m_id;
    m_firstPin = other.m_firstPin;
    m_lastPin = other.m_lastPin;
    m_type = other.m_type;
    m_fixed = other.m_fixed;
    m_attributes = other.m_attributes;
    m_currentOrient = other.m_currentOrient;
    m_availOrient = other.m_availOrient;
    m_x = other.m_x;
    m_y = other.m_y;
    m_w = other.m_w;
    m_h = other.m_h;
    m_etl = other.m_etl;
    m_etr = other.m_etr;
    m_regionId = other.m_regionId;
  }
  return *this;
}
Node::~Node() {}

bool Node::isFlop() const {
  if ((m_attributes & NodeAttributes_IS_FLOP) != 0) return true;
  return false;
}

Edge::Edge() : m_id(0), m_firstPin(0), m_lastPin(0), m_ndr(0) {}

Edge::~Edge() {}

Pin::Pin()
    : m_id(-1),
      m_dir(Pin::Dir_INOUT),
      m_nodeId(0),
      m_edgeId(0),
      m_offsetX(0.0),
      m_offsetY(0.0),
      m_pinLayer(
          0),  // Assume layer 0 which is understood to correspond to metal1.
      m_pinW(0.0),
      m_pinH(0.0) {
#ifdef USE_ICCAD14
  m_portName = 0;
  m_cap = 0.0;
  m_delay = 0.0;
  m_rTran = 0.0;
  m_fTran = 0.0;
  m_driverType = 0;
  m_earlySlack = 0;
  m_lateSlack = 0;
  m_crit = 1.0;
#endif
}

Pin::Pin(const Pin& other)
    : m_id(other.m_id),
      m_dir(other.m_dir),
      m_nodeId(other.m_nodeId),
      m_edgeId(other.m_edgeId),
      m_offsetX(other.m_offsetX),
      m_offsetY(other.m_offsetY),
      m_pinLayer(other.m_pinLayer),
      m_pinW(other.m_pinW),
      m_pinH(other.m_pinH) {
#ifdef USE_ICCAD14
  if (other.m_portName == 0) {
    m_portName = 0;
  } else {
    m_portName = new char[strlen(other.m_portName) + 1];
    strcpy(&m_portName[0], &other.m_portName[0]);
  }
  m_cap = 0.0;
  m_delay = 0.0;
  m_rTran = 0.0;
  m_fTran = 0.0;
  m_driverType = 0;
  m_earlySlack = 0;
  m_lateSlack = 0;
  m_crit = 1.0;
#endif
}

Pin& Pin::operator=(const Pin& other) {
  if (this != &other) {
    m_id = other.m_id;
    m_dir = other.m_dir;
    m_nodeId = other.m_nodeId;
    m_edgeId = other.m_edgeId;
    m_offsetX = other.m_offsetX;
    m_offsetY = other.m_offsetY;
    m_pinLayer = other.m_pinLayer;
    m_pinW = other.m_pinW;
    m_pinH = other.m_pinH;
#ifdef USE_ICCAD14
    if (m_portName != 0) delete[] m_portName;
    if (other.m_portName == 0) {
      m_portName = 0;
    } else {
      m_portName = new char[strlen(other.m_portName) + 1];
      strcpy(&m_portName[0], &other.m_portName[0]);
    }
    m_cap = 0.0;
    m_delay = 0.0;
    m_rTran = 0.0;
    m_fTran = 0.0;
    m_driverType = 0;
    m_earlySlack = 0;
    m_lateSlack = 0;
    m_crit = 1.0;
#endif
  }
  return *this;
}

Pin::~Pin() {
#ifdef USE_ICCAD14
  if (m_portName != 0) delete[] m_portName;
#endif
}

#ifdef USE_ICCAD14
Node* Pin::getOwner(Network* network) const {
  return &(network->m_nodes[this->m_nodeId]);
}
bool Pin::getName(Network* network, std::string& name) const {
  std::string& nodeName = network->m_nodeNames[this->m_nodeId];
  if (strncasecmp(nodeName.c_str(), "FakeInstForExtPin",
                  strlen("FakeInstForExtPin")) == 0) {
    name = this->m_portName;
  } else {
    name = std::string(nodeName.c_str()) + "/" + std::string(this->m_portName);
  }
  return true;
}
bool Pin::isFlopInput(Network* network) const {
  Node* owner = &(network->m_nodes[this->m_nodeId]);
  if (owner->isFlop() && this->m_dir == Pin::Dir_IN) return true;
  return false;
}
bool Pin::isPi(Network* network) const {
  // A pin is a PI pin if it is on a fake instance and it is a source (note that
  // a PI is a source as far as the circuit is concerned!).
  std::string& nodeName = network->m_nodeNames[this->m_nodeId];
  if (strncasecmp(nodeName.c_str(), "FakeInstForExtPin",
                  strlen("FakeInstForExtPin")) == 0) {
    return isSource();
  }
  return false;
}
bool Pin::isPo(Network* network) const {
  // A pin is a PO pin if it is on a fake instance and it is a sink (note that a
  // PO is a sink as far as the circuit is concerned!).
  std::string& nodeName = network->m_nodeNames[this->m_nodeId];
  if (strncasecmp(nodeName.c_str(), "FakeInstForExtPin",
                  strlen("FakeInstForExtPin")) == 0) {
    return isSink();
  }
  return false;
}
#endif

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
  m_pins.clear();
}

void Network::deleteFillerNodes(void) {
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

}  // namespace dpo
