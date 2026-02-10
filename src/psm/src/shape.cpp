// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "shape.h"

#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "connection.h"
#include "ir_network.h"
#include "node.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace psm {

Shape::Shape(const odb::Rect& shape, odb::dbTechLayer* layer)
    : shape_(shape), layer_(layer)
{
}

odb::dbTechLayer* Shape::getLayer() const
{
  return layer_;
}

Connections Shape::connectNodes(const IRNetwork::NodeTree& layer_nodes)
{
  Connections shape_connections;

  Node::NodeSet used;

  const auto sorted_nodes = getNodes(layer_nodes);
  const auto tree = getNodeTree(sorted_nodes);

  for (auto* node : sorted_nodes) {
    const auto& pt = node->getPoint();
    const IRNetwork::Point point(pt.x(), pt.y());

    std::vector<Node*> ordered_neighbors;

    used.insert(node);

    tree.query(boost::geometry::index::satisfies([&](const auto value) {
                 return used.find(value) == used.end();
               }) && boost::geometry::index::nearest(point, 1),
               std::back_inserter(ordered_neighbors));

    for (Node* other : ordered_neighbors) {
      const int len_x
          = std::abs(other->getPoint().getX() - node->getPoint().getX());
      const int len_y
          = std::abs(other->getPoint().getY() - node->getPoint().getY());

      int len;
      int width;
      if (len_x > len_y) {
        len = len_x;
        width = shape_.dy();
      } else {
        len = len_y;
        width = shape_.dx();
      }

      shape_connections.push_back(
          std::make_unique<LayerConnection>(node, other, len, width));
    }
  }

  return shape_connections;
}

std::string Shape::describe(double dbu) const
{
  return fmt::format("{}: ({:.4f}, {:.4f}) -- ({:.4f}, {:.4f})",
                     id_,
                     shape_.xMin() / dbu,
                     shape_.yMin() / dbu,
                     shape_.xMax() / dbu,
                     shape_.yMax() / dbu);
}

std::vector<std::unique_ptr<Node>> Shape::createFillerNodes(
    int max_distance,
    const IRNetwork::NodeTree& layer_nodes)
{
  std::vector<std::unique_ptr<Node>> new_nodes;

  int delta_x = 0;
  int delta_y = 0;

  const int radius = max_distance / 2;

  odb::Point start;
  if (shape_.dx() > shape_.dy()) {
    delta_x = max_distance;
    start = odb::Point(shape_.xMin() + radius, shape_.yCenter());
  } else {
    delta_y = max_distance;
    start = odb::Point(shape_.xCenter(), shape_.yMin() + radius);
  }

  const IRNetwork::NodeTree tree = getNodeTree(getNodes(layer_nodes));

  while (shape_.overlaps(start)) {
    new_nodes.push_back(std::make_unique<Node>(start, layer_));

    start.addX(delta_x);
    start.addY(delta_y);
  }

  return new_nodes;
}

Node::NodeSet Shape::getNodes(const IRNetwork::NodeTree& layer_nodes) const
{
  return Node::NodeSet(
      layer_nodes.qbegin(boost::geometry::index::intersects(shape_)),
      layer_nodes.qend());
}

IRNetwork::NodeTree Shape::getNodeTree(const Node::NodeSet& nodes) const
{
  return IRNetwork::NodeTree(nodes.begin(), nodes.end());
}

std::set<Node*> Shape::cleanupNodes(
    int min_distance,
    const IRNetwork::NodeTree& layer_nodes,
    const std::function<void(Node*, Node*)>& copy_func,
    const std::set<Node*>& shared_nodes)
{
  // Process and filter nodes
  const Node::NodeSet sorted_nodes = getNodes(layer_nodes);
  Node::NodeSet center_nodes;
  Node::NodeSet non_center_nodes;
  const odb::Point shape_center = shape_.center();
  for (auto* node : sorted_nodes) {
    const auto& pt = node->getPoint();
    if (pt.x() == shape_center.x() || pt.y() == shape_center.y()) {
      center_nodes.insert(node);
    } else {
      non_center_nodes.insert(node);
    }
  }

  // Build RTree of nodes for searching
  Node::NodeSet shape_shared_nodes;
  std::vector<std::unique_ptr<NodeData>> node_data;
  const auto tree = createNodeDataValue(
      sorted_nodes, shared_nodes, node_data, shape_shared_nodes);

  std::set<Node*> remove;
  const int radius = min_distance / 2;

  std::map<Node*, std::set<Node*>> node_cleanup;
  // start with shared nodes
  for (const auto& [node, merged_with] :
       mergeNodes(shape_shared_nodes, radius, tree, remove, copy_func)) {
    node_cleanup[node].insert(merged_with.begin(), merged_with.end());
  }

  // handle center line nodes
  for (const auto& [node, merged_with] :
       mergeNodes(center_nodes, radius, tree, remove, copy_func)) {
    node_cleanup[node].insert(merged_with.begin(), merged_with.end());
  }

  // handle remaining nodes
  for (const auto& [node, merged_with] :
       mergeNodes(non_center_nodes, radius, tree, remove, copy_func)) {
    node_cleanup[node].insert(merged_with.begin(), merged_with.end());
  }

  return remove;
}

Shape::NodeDataTree Shape::createNodeDataValue(
    const Node::NodeSet& nodes,
    const std::set<Node*>& shared_nodes,
    std::vector<std::unique_ptr<NodeData>>& container,
    Node::NodeSet& shape_shared_nodes) const
{
  // Build RTree of nodes for searching
  for (auto* node : nodes) {
    if (shared_nodes.find(node) != shared_nodes.end()) {
      // don't consider shared nodes
      shape_shared_nodes.insert(node);
      continue;
    }
    auto data = std::make_unique<NodeData>();
    data->node = node;
    container.push_back(std::move(data));
  }

  std::vector<NodeData*> node_values;
  node_values.reserve(container.size());
  for (const auto& node_data : container) {
    node_values.emplace_back(node_data.get());
  }

  return NodeDataTree(node_values.begin(), node_values.end());
}

std::map<Node*, std::set<Node*>> Shape::mergeNodes(
    const Node::NodeSet& nodes,
    int radius,
    const NodeDataTree& tree,
    std::set<Node*>& remove,
    const std::function<void(Node*, Node*)>& copy_func) const
{
  std::map<Node*, std::set<Node*>> node_cleanup;
  for (auto* node : nodes) {
    if (remove.find(node) != remove.end()) {
      continue;
    }
    const auto& pt = node->getPoint();
    const odb::Rect check_rect(pt.getX() - radius,
                               pt.getY() - radius,
                               pt.getX() + radius,
                               pt.getY() + radius);
    std::set<Node*> merge;
    for (auto itr
         = tree.qbegin(boost::geometry::index::intersects(check_rect)
                       && boost::geometry::index::satisfies(
                           [](const auto& val) { return !val->used; })
                       && boost::geometry::index::satisfies(
                           [&](const auto& val) { return val->node != node; }));
         itr != tree.qend();
         itr++) {
      auto* data = *itr;
      merge.insert(data->node);
      data->used = true;
    }

    for (auto* mnode : merge) {
      copy_func(node, mnode);
      remove.insert(mnode);
    }
  }
  return node_cleanup;
}

}  // namespace psm
