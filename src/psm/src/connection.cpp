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

#include "connection.h"

#include "node.h"
#include "odb/db.h"

namespace psm {

Connection::Connection(Node* node0, Node* node1) : node0_(node0), node1_(node1)
{
  ensureNodeOrder();
}

void Connection::ensureNodeOrder()
{
  if (node0_ && !node0_->compare(node1_)) {
    std::swap(node0_, node1_);
  }
}

Node* Connection::getOtherNode(const Node* node) const
{
  if (node == node0_) {
    return node1_;
  }
  return node0_;
}

Connection::Conductance Connection::getConductance(
    const ResistanceMap& res_map) const
{
  const auto res = getResistance(res_map);
  if (res == 0) {
    return std::numeric_limits<Conductance>::infinity();
  }
  return 1.0 / res;
}

void Connection::changeNode(Node* orgnode, Node* newnode)
{
  if (node0_ == orgnode) {
    node0_ = newnode;
  }

  if (node1_ == orgnode) {
    node1_ = newnode;
  }
}

void Connection::print(utl::Logger* logger) const
{
  logger->report(describe());
}

int Connection::getDBUs() const
{
  const Node* node = node0_ ? node0_ : node1_;
  if (node) {
    return node->getLayer()->getTech()->getDbUnitsPerMicron();
  }
  return 1;
}

bool Connection::compare(const Connection* other) const
{
  return compareTuple() < other->compareTuple();
}

bool Connection::compare(const std::unique_ptr<Connection>& other) const
{
  return compare(other.get());
}

Connection::CompareInformation Connection::compareTuple() const
{
  Node::CompareInformation node0_info;
  if (node0_ != nullptr) {
    node0_info = node0_->compareTuple();
  } else {
    node0_info = Node::dummyCompareTuple();
  }

  Node::CompareInformation node1_info;
  if (node1_ != nullptr) {
    node1_info = node1_->compareTuple();
  } else {
    node1_info = Node::dummyCompareTuple();
  }

  return {node0_info, node1_info};
}

std::string Connection::describeWithNodes() const
{
  const std::string node0 = node0_ ? node0_->describe("") : "*no node0*";
  const std::string node1 = node1_ ? node1_->describe("") : "*no node1*";
  return fmt::format("{}: {} -> {}", describe(), node0, node1);
}

template <typename T>
bool Connection::hasNodeOfType() const
{
  return dynamic_cast<T*>(node0_) != nullptr
         || dynamic_cast<T*>(node1_) != nullptr;
}

bool Connection::hasITermNode() const
{
  return hasNodeOfType<ITermNode>();
}

bool Connection::hasBPinNode() const
{
  return hasNodeOfType<BPinNode>();
}

/////////////////////////////

LayerConnection::LayerConnection(Node* node0,
                                 Node* node1,
                                 int length,
                                 int width)
    : Connection(node0, node1), length_(length), width_(width)
{
}

Connection::Resistance LayerConnection::getResistance(
    const ResistanceMap& res_map) const
{
  const double squares = static_cast<double>(length_) / width_;
  return squares * res_map.at(node0_->getLayer());
}

std::string LayerConnection::describe() const
{
  const double dbus = getDBUs();
  return fmt::format("Layer Connection (w = {:.4f}, l = {:.4f})",
                     width_ / dbus,
                     length_ / dbus);
}

bool LayerConnection::isValid() const
{
  return width_ > 0.0 && length_ > 0.0;
}

void LayerConnection::mergeWith(const Connection* other)
{
  const LayerConnection* layer_other
      = dynamic_cast<const LayerConnection*>(other);
  if (layer_other == nullptr) {
    return;
  }

  // parallel merge so make width wider
  width_ += layer_other->width_;
}

/////////////////////////////

ViaConnection::ViaConnection(Node* node0, Node* node1, int cuts)
    : Connection(node0, node1), cuts_(cuts)
{
}

Connection::Resistance ViaConnection::getResistance(
    const ResistanceMap& res_map) const
{
  auto* via_layer = node0_->getLayer()->getUpperLayer();
  return res_map.at(via_layer) / cuts_;
}

std::string ViaConnection::describe() const
{
  return fmt::format("Via Connection (cuts = {})", cuts_);
}

bool ViaConnection::isValid() const
{
  return cuts_ > 0;
}

void ViaConnection::mergeWith(const Connection* other)
{
  const ViaConnection* via_other = dynamic_cast<const ViaConnection*>(other);
  if (via_other == nullptr) {
    return;
  }

  cuts_ += via_other->cuts_;
}

/////////////////////////////

TermConnection::TermConnection(Node* node0, Node* node1)
    : Connection(node0, node1)
{
}

Connection::Resistance TermConnection::getResistance(
    const ResistanceMap& res_map) const
{
  return resistance_;
}

std::string TermConnection::describe() const
{
  return "Terminal Connection";
}

}  // namespace psm
