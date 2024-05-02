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

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ir_network.h"
#include "odb/geom_boost.h"

namespace odb {
class dbTechLayer;
}

namespace psm {
class Node;
class Connection;

class Shape
{
 public:
  Shape(const odb::Rect& shape, odb::dbTechLayer* layer);

  std::vector<std::unique_ptr<Node>> createFillerNodes(
      int max_distance,
      const IRNetwork::NodeTree& layer_nodes);
  std::vector<std::unique_ptr<Connection>> connectNodes(
      const IRNetwork::NodeTree& layer_nodes);
  std::set<Node*> cleanupNodes(
      int min_distance,
      const IRNetwork::NodeTree& layer_nodes,
      const std::function<void(Node*, Node*)>& copy_func,
      const std::set<Node*>& shared_nodes);

  const odb::Rect& getShape() const { return shape_; }

  odb::dbTechLayer* getLayer() const;

  std::string describe(double dbus) const;

  void setID(std::size_t id) { id_ = id; }
  std::size_t getID() const { return id_; }

 private:
  struct NodeData
  {
    Node* node = nullptr;
    bool used = false;
    odb::Point getPoint() const { return node->getPoint(); }
  };
  using NodeDataTree
      = boost::geometry::index::rtree<NodeData*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<NodeData>>;

  Node::NodeSet getNodes(const IRNetwork::NodeTree& layer_nodes) const;
  IRNetwork::NodeTree getNodeTree(const Node::NodeSet& nodes) const;

  NodeDataTree createNodeDataValue(
      const Node::NodeSet& nodes,
      const std::set<Node*>& shared_nodes,
      std::vector<std::unique_ptr<NodeData>>& container,
      Node::NodeSet& shape_shared_nodes) const;
  std::map<Node*, std::set<Node*>> mergeNodes(
      const Node::NodeSet& nodes,
      int radius,
      const NodeDataTree& tree,
      std::set<Node*>& remove,
      const std::function<void(Node*, Node*)>& copy_func) const;

  odb::Rect shape_;
  odb::dbTechLayer* layer_;

  std::size_t id_ = 0;
};

}  // namespace psm
