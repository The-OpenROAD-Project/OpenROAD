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
#include "network.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node::Node() = default;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Node::adjustCurrOrient(const dbOrientType& newOri)
{
  // Change the orientation of the cell, but leave the lower-left corner
  // alone.  This means changing the locations of pins and possibly
  // changing the edge types as well as the height and width.
  auto curOri = orient_;
  if (newOri == curOri) {
    return true;
  }

  if (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90
      || curOri == dbOrientType::R270 || curOri == dbOrientType::MYR90) {
    if (newOri == dbOrientType::R0 || newOri == dbOrientType::MY
        || newOri == dbOrientType::MX || newOri == dbOrientType::R180) {
      // Rotate the cell counter-clockwise by 90 degrees.
      for (Pin* pin : pins_) {
        const auto dx = pin->getOffsetX().v;
        const auto dy = pin->getOffsetY().v;
        pin->setOffsetX(DbuX{-dy});
        pin->setOffsetY(DbuY{dx});
      }
      {
        int tmp = width_.v;
        width_ = DbuX{height_.v};
        height_ = DbuY{tmp};
      }
      if (curOri == dbOrientType::R90) {
        curOri = dbOrientType::R0;
      } else if (curOri == dbOrientType::MXR90) {
        curOri = dbOrientType::MX;
      } else if (curOri == dbOrientType::MYR90) {
        curOri = dbOrientType::MY;
      } else {
        curOri = dbOrientType::R180;
      }
    }
  } else {
    if (newOri == dbOrientType::R90 || newOri == dbOrientType::MXR90
        || newOri == dbOrientType::MYR90 || newOri == dbOrientType::R270) {
      // Rotate the cell clockwise by 90 degrees.
      for (Pin* pin : pins_) {
        const auto dx = pin->getOffsetX().v;
        const auto dy = pin->getOffsetY().v;
        pin->setOffsetX(DbuX{dy});
        pin->setOffsetY(DbuY{-dx});
      }
      {
        int tmp = width_.v;
        width_ = DbuX{height_.v};
        height_ = DbuY{tmp};
      }
      if (curOri == dbOrientType::R0) {
        curOri = dbOrientType::R90;
      } else if (curOri == dbOrientType::MX) {
        curOri = dbOrientType::MXR90;
      } else if (curOri == dbOrientType::MY) {
        curOri = dbOrientType::MYR90;
      } else {
        curOri = dbOrientType::R270;
      }
    }
  }
  // Both the current and new orientations should be {N, FN, FS, S} or {E, FE,
  // FW, W}.
  int mX = 1;
  int mY = 1;
  if (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90
      || curOri == dbOrientType::MYR90 || curOri == dbOrientType::R270) {
    const bool test1
        = (curOri == dbOrientType::R90 || curOri == dbOrientType::MYR90);
    const bool test2
        = (newOri == dbOrientType::R90 || newOri == dbOrientType::MYR90);
    if (test1 != test2) {
      mX = -1;
    }
    const bool test3
        = (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90);
    const bool test4
        = (newOri == dbOrientType::R90 || newOri == dbOrientType::MXR90);
    if (test3 != test4) {
      mY = -1;
    }
  } else {
    const bool test1
        = (curOri == dbOrientType::R0 || curOri == dbOrientType::MX);
    const bool test2
        = (newOri == dbOrientType::R0 || newOri == dbOrientType::MX);
    if (test1 != test2) {
      mX = -1;
    }
    const bool test3
        = (curOri == dbOrientType::R0 || curOri == dbOrientType::MY);
    const bool test4
        = (newOri == dbOrientType::R0 || newOri == dbOrientType::MY);
    if (test3 != test4) {
      mY = -1;
    }
  }

  for (Pin* pin : pins_) {
    pin->setOffsetX(pin->getOffsetX() * DbuX{mX});
    pin->setOffsetY(pin->getOffsetY() * DbuY{mY});
  }
  orient_ = newOri;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin::Pin() = default;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Network::~Network()
{
  // Delete edges.
  for (auto edge : edges_) {
    delete edge;
  }
  edges_.clear();
  edgeNames_.clear();

  // Delete cells.
  for (auto node : nodes_) {
    delete node;
  }
  nodes_.clear();
  nodeNames_.clear();

  // Delete pins.
  for (auto pin : pins_) {
    delete pin;
  }
  pins_.clear();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Edge* Network::createAndAddEdge()
{
  // Just allocate an edge, append it and give it the id
  // that corresponds to its index.
  const int id = (int) edges_.size();
  Edge* ptr = new Edge();
  ptr->setId(id);
  edges_.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
dpl::Master* Network::createAndAddMaster()
{
  masters_.emplace_back(std::make_unique<dpl::Master>());
  return masters_.back().get();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node* Network::createAndAddNode()
{
  // Just allocate a node, append it and give it the id
  // that corresponds to its index.
  const int id = (int) nodes_.size();
  Node* ptr = new Node();
  ptr->setId(id);
  nodes_.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Network::createAndAddBlockage(const odb::Rect& bounds)
{
  blockages_.emplace_back(bounds);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node* Network::createAndAddFillerNode(const DbuX left,
                                      const DbuY bottom,
                                      const DbuX width,
                                      const DbuY height)
{
  Node* ndi = new Node();
  const int id = (int) nodes_.size();
  ndi->setFixed(true);
  ndi->setType(Node::FILLER);
  ndi->setId(id);
  ndi->setHeight(height);
  ndi->setWidth(width);
  ndi->setBottom(bottom);
  ndi->setLeft(left);
  nodes_.push_back(ndi);
  return getNode(id);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin* Network::createAndAddPin()
{
  Pin* ptr = new Pin();
  pins_.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin* Network::createAndAddPin(Node* nd, Edge* ed)
{
  Pin* ptr = createAndAddPin();
  ptr->node_ = nd;
  ptr->edge_ = ed;
  ptr->node_->pins_.push_back(ptr);
  ptr->edge_->pins_.push_back(ptr);
  return ptr;
}

}  // namespace dpo
