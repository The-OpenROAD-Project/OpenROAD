// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "network.h"

#include <memory>

#include "dpl/Objects.h"
namespace dpo {

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
  ptr->setNode(nd);
  ptr->setEdge(ed);
  ptr->getNode()->addPin(ptr);
  ptr->getEdge()->addPin(ptr);
  return ptr;
}

}  // namespace dpo
