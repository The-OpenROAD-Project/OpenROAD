///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

#include "node.h"

#include "odb/db.h"
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
  return {-1, 0, 0, NodeType::Unknown, -1};
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
  return layer_->getTech()->getLefUnits();
}

void Node::print(utl::Logger* logger, const std::string& prefix) const
{
  logger->report(describe(prefix));
}

std::string Node::getName() const
{
  std::string type = getTypeName();

  type.erase(remove(type.begin(), type.end(), ' '), type.end());

  return fmt::format(
      "{}_{}_{}_{}", type, layer_->getName(), pt_.getX(), pt_.getY());
}

std::string Node::getTypeName() const
{
  switch (getType()) {
    case NodeType::Node:
      return "Node";
    case NodeType::Source:
      return "Source Node";
    case NodeType::ITerm:
      return "ITerm Node";
    case NodeType::BPin:
      return "BPin Node";
    case NodeType::Unknown:
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

BPinNode::BPinNode(odb::dbBPin* pin,
                   const odb::Rect& shape,
                   odb::dbTechLayer* layer)
    : TerminalNode(shape, layer), pin_(pin)
{
}

int BPinNode::getTypeCompareInfo() const
{
  return pin_->getId();
}

}  // namespace psm
