// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "network.h"

namespace dpo {

Node::Node() = default;

bool Node::adjustCurrOrient(const dbOrientType& newOri)
{
  // Change the orientation of the cell, but leave the lower-left corner
  // alone.  This means changing the locations of pins and possibly
  // changing the edge types as well as the height and width.
  auto curOri = currentOrient_;
  if (newOri == curOri) {
    return true;
  }

  if (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90
      || curOri == dbOrientType::R270 || curOri == dbOrientType::MYR90) {
    if (newOri == dbOrientType::R0 || newOri == dbOrientType::MY
        || newOri == dbOrientType::MX || newOri == dbOrientType::R180) {
      // Rotate the cell counter-clockwise by 90 degrees.
      for (Pin* pin : pins_) {
        const double dx = pin->getOffsetX();
        const double dy = pin->getOffsetY();
        pin->setOffsetX(-dy);
        pin->setOffsetY(dx);
      }
      {
        int tmp = w_.v;
        w_ = DbuX{h_.v};
        h_ = DbuY{tmp};
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
        const double dx = pin->getOffsetX();
        const double dy = pin->getOffsetY();
        pin->setOffsetX(dy);
        pin->setOffsetY(-dx);
      }
      {
        int tmp = w_.v;
        w_ = DbuX{h_.v};
        h_ = DbuY{tmp};
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
  bool changeEdgeTypes = false;
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
      changeEdgeTypes = true;
      mY = -1;
    }
  } else {
    const bool test1
        = (curOri == dbOrientType::R0 || curOri == dbOrientType::MX);
    const bool test2
        = (newOri == dbOrientType::R0 || newOri == dbOrientType::MX);
    if (test1 != test2) {
      changeEdgeTypes = true;
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
    pin->setOffsetX(pin->getOffsetX() * mX);
    pin->setOffsetY(pin->getOffsetY() * mY);
  }
  if (changeEdgeTypes) {
    std::swap(etls_, etrs_);
  }
  currentOrient_ = newOri;
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
Master* Network::createAndAddMaster()
{
  masters_.emplace_back(std::make_unique<Master>());
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
  ndi->setFixed(Node::FIXED_XY);
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
