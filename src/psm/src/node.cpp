// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "node.h"

#include <algorithm>
#include <memory>
#include <string>

#include "odb/db.h"
#include "odb/geom.h"
#include "shape.h"

namespace psm {

Node::Node(const odb::Point& pt, odb::dbTechLayer* layer)
    : pt_(pt), layer_(layer)
{
}

Node::CompareInformation Node::compareTuple() const
{
  const int x = pt_.getX();
  const int y = pt_.getY();
  const int layer = layer_->getNumber();

  return {layer, x, y, getType(), getTypeCompareInfo()};
}

Node::CompareInformation Node::dummyCompareTuple()
{
  return {-1, 0, 0, NodeType::kUnknown, -1};
}

bool Node::compare(const Node* other) const
{
  if (other == nullptr) {
    return true;
  }

  return compareTuple() < other->compareTuple();
}

bool Node::compare(const std::unique_ptr<Node>& other) const
{
  return compare(other.get());
}

std::string Node::describe(const std::string& prefix) const
{
  const double dbus = getDBUs();

  return fmt::format("{}{}: ({:.4f}, {:.4f}) on {}",
                     prefix,
                     getTypeName(),
                     pt_.getX() / dbus,
                     pt_.getY() / dbus,
                     layer_->getName());
}

double Node::getDBUs() const
{
  return layer_->getTech()->getDbUnitsPerMicron();
}

void Node::print(utl::Logger* logger, const std::string& prefix) const
{
  logger->report(describe(prefix));
}

std::string Node::getName() const
{
  std::string type = getTypeName();

  std::erase(type, ' ');

  return fmt::format(
      "{}_{}_{}_{}", type, layer_->getName(), pt_.getX(), pt_.getY());
}

std::string Node::getTypeName() const
{
  switch (getType()) {
    case NodeType::kNode:
      return "Node";
    case NodeType::kSource:
      return "Source Node";
    case NodeType::kITerm:
      return "ITerm Node";
    case NodeType::kBPin:
      return "BPin Node";
    case NodeType::kUnknown:
      return "Unknown";
  }

  return "unknown";
}

///////////////////

TerminalNode::TerminalNode(const odb::Rect& shape, odb::dbTechLayer* layer)
    : Node(odb::Point(shape.center()), layer), shape_(shape)
{
}

///////////////////

SourceNode::SourceNode(Node* node)
    : Node(node->getPoint(), node->getLayer()), source_(node)
{
}

////////////////////

ITermNode::ITermNode(odb::dbITerm* iterm,
                     const odb::Point& pt,
                     odb::dbTechLayer* layer)
    : TerminalNode({pt, pt}, layer), iterm_(iterm)
{
}

int ITermNode::getTypeCompareInfo() const
{
  return iterm_->getId();
}

std::string ITermNode::describe(const std::string& prefix) const
{
  return TerminalNode::describe(prefix) + ": " + iterm_->getName();
}

////////////////////

BPinNode::BPinNode(odb::dbBPin* pin, odb::dbBox* box, odb::dbTechLayer* layer)
    : TerminalNode(box->getBox(), layer), pin_(pin), box_(box)
{
}

int BPinNode::getTypeCompareInfo() const
{
  return pin_->getId();
}

bool BPinNode::shouldConnect() const
{
  if (auto prop = odb::dbBoolProperty::find(pin_, kDisconnectProperty)) {
    if (prop != nullptr && prop->getValue()) {
      return false;
    }
  }
  if (auto prop = odb::dbBoolProperty::find(box_, kDisconnectProperty)) {
    if (prop != nullptr && prop->getValue()) {
      return false;
    }
  }
  return true;
}

}  // namespace psm
