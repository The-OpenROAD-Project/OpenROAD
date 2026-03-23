// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "connection.h"
#include "ir_network.h"
#include "odb/geom.h"
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
  Connections connectNodes(const IRNetwork::NodeTree& layer_nodes);
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
