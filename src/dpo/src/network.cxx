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

#include "architecture.h"
#include "orientation.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Node::Node()
    : id_(0),
      left_(0),
      bottom_(0),
      origLeft_(0.0),
      origBottom_(0.0),
      w_(0.0),
      h_(0.0),
      type_(UNKNOWN),
      fixed_(Node::NOT_FIXED),
      etl_(EDGETYPE_DEFAULT),
      etr_(EDGETYPE_DEFAULT),
      powerTop_(Architecture::Row::Power_UNK),
      powerBot_(Architecture::Row::Power_UNK),
      regionId_(0),
      currentOrient_(Orientation_N),
      availOrient_(Orientation_N),
      isDefinedByShapes_(false)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Node::adjustCurrOrient(unsigned newOri)
{
  // Change the orientation of the cell, but leave the lower-left corner
  // alone.  This means changing the locations of pins and possibly
  // changing the edge types as well as the height and width.
  unsigned curOri = currentOrient_;
  if (newOri == curOri) {
    return true;
  }

  if (curOri == Orientation_E || curOri == Orientation_FE
      || curOri == Orientation_FW || curOri == Orientation_W) {
    if (newOri == Orientation_N || curOri == Orientation_FN
        || curOri == Orientation_FS || curOri == Orientation_S) {
      // Rotate the cell counter-clockwise by 90 degrees.
      for (int pi = 0; pi < pins_.size(); pi++) {
        Pin* pin = pins_[pi];
        double dx = pin->getOffsetX();
        double dy = pin->getOffsetY();
        pin->setOffsetX(-dy);
        pin->setOffsetY(dx);
      }
      std::swap(h_, w_);
      if (curOri == Orientation_E) {
        curOri = Orientation_N;
      } else if (curOri == Orientation_FE) {
        curOri = Orientation_FS;
      } else if (curOri == Orientation_FW) {
        curOri = Orientation_FN;
      } else {
        curOri = Orientation_S;
      }
    }
  } else {
    if (newOri == Orientation_E || curOri == Orientation_FE
        || curOri == Orientation_FW || curOri == Orientation_W) {
      // Rotate the cell clockwise by 90 degrees.
      for (int pi = 0; pi < pins_.size(); pi++) {
        Pin* pin = pins_[pi];
        double dx = pin->getOffsetX();
        double dy = pin->getOffsetY();
        pin->setOffsetX(dy);
        pin->setOffsetY(-dx);
      }
      std::swap(h_, w_);
      if (curOri == Orientation_N) {
        curOri = Orientation_E;
      } else if (curOri == Orientation_FS) {
        curOri = Orientation_FE;
      } else if (curOri == Orientation_FN) {
        curOri = Orientation_FW;
      } else {
        curOri = Orientation_W;
      }
    }
  }
  // Both the current and new orientations should be {N, FN, FS, S} or {E, FE,
  // FW, W}.
  int mX = 1;
  int mY = 1;
  bool changeEdgeTypes = false;
  if (curOri == Orientation_E || curOri == Orientation_FE
      || curOri == Orientation_FW || curOri == Orientation_W) {
    const bool test1 = (curOri == Orientation_E || curOri == Orientation_FW);
    const bool test2 = (newOri == Orientation_E || newOri == Orientation_FW);
    if (test1 != test2) {
      mX = -1;
    }
    const bool test3 = (curOri == Orientation_E || curOri == Orientation_FE);
    const bool test4 = (newOri == Orientation_E || newOri == Orientation_FE);
    if (test3 != test4) {
      changeEdgeTypes = true;
      mY = -1;
    }
  } else {
    const bool test1 = (curOri == Orientation_N || curOri == Orientation_FS);
    const bool test2 = (newOri == Orientation_N || newOri == Orientation_FS);
    if (test1 != test2) {
      changeEdgeTypes = true;
      mX = -1;
    }
    const bool test3 = (curOri == Orientation_N || curOri == Orientation_FN);
    const bool test4 = (newOri == Orientation_N || newOri == Orientation_FN);
    if (test3 != test4) {
      mY = -1;
    }
  }

  for (int pi = 0; pi < pins_.size(); pi++) {
    Pin* pin = pins_[pi];
    pin->setOffsetX(pin->getOffsetX() * mX);
    pin->setOffsetY(pin->getOffsetY() * mY);
  }
  if (changeEdgeTypes) {
    std::swap(etl_, etr_);
  }
  currentOrient_ = newOri;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Pin::Pin()
    : pinWidth_(0),
      pinHeight_(0),
      dir_(Pin::Dir_INOUT),
      pinLayer_(0),
      node_(nullptr),
      edge_(nullptr),
      offsetX_(0.0),
      offsetY_(0.0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Network::~Network()
{
  // Delete edges.
  for (int i = 0; i < edges_.size(); i++) {
    delete edges_[i];
  }
  edges_.clear();
  edgeNames_.clear();

  // Delete cells.
  for (int i = 0; i < nodes_.size(); i++) {
    delete nodes_[i];
  }
  nodes_.clear();
  nodeNames_.clear();

  // Delete pins.
  for (int i = 0; i < pins_.size(); i++) {
    delete pins_[i];
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
Node* Network::createAndAddFillerNode(int left,
                                      int bottom,
                                      int width,
                                      int height)
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
Node* Network::createAndAddShapeNode(int left,
                                     int bottom,
                                     int width,
                                     int height)
{
  // Add shape cells to list of network cells.  We have
  // the parent node from which we can derive a name.
  Node* ndi = new Node();
  const int id = (int) nodes_.size();
  ndi->setFixed(Node::FIXED_XY);
  ndi->setType(Node::SHAPE);
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
