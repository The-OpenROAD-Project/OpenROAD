// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "shape.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
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

namespace {

// The range of t over which pt + t * dir lies in the convex polygon, empty
// (first above second) when the line misses it. odb polygons are clockwise
// and close on their first point, so the inside of an edge is to its right.
std::pair<double, double> clipLine(const std::vector<odb::Point>& points,
                                   double pt_x,
                                   double pt_y,
                                   double dir_x,
                                   double dir_y)
{
  double t_min = -std::numeric_limits<double>::infinity();
  double t_max = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i + 1 < points.size(); i++) {
    const double edge_x = points[i + 1].x() - points[i].x();
    const double edge_y = points[i + 1].y() - points[i].y();
    // the line is inside of the edge where offset + t * rate >= 0
    const double offset
        = edge_y * (pt_x - points[i].x()) - edge_x * (pt_y - points[i].y());
    const double rate = edge_y * dir_x - edge_x * dir_y;
    if (rate == 0) {
      if (offset < 0) {
        // runs alongside the edge, on the outside of it
        return {1, 0};
      }
      continue;
    }
    const double t = -offset / rate;
    if (rate > 0) {
      t_min = std::max(t_min, t);
    } else {
      t_max = std::min(t_max, t);
    }
  }
  return {t_min, t_max};
}

}  // namespace

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

    std::vector<Node*> ordered_neighbors;

    used.insert(node);

    tree.query(boost::geometry::index::satisfies([&](const auto value) {
                 return used.find(value) == used.end();
               }) && boost::geometry::index::nearest(pt, 1),
               std::back_inserter(ordered_neighbors));

    for (Node* other : ordered_neighbors) {
      const ConnectionSize size
          = getConnectionSize(node->getPoint(), other->getPoint());

      shape_connections.push_back(std::make_unique<LayerConnection>(
          node, other, size.length, size.width));
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
    if (const auto pt = getFillerPoint(start, delta_x != 0)) {
      new_nodes.push_back(std::make_unique<Node>(*pt, layer_));
    }

    start.addX(delta_x);
    start.addY(delta_y);
  }

  return new_nodes;
}

Node::NodeSet Shape::getNodes(const IRNetwork::NodeTree& layer_nodes) const
{
  // the bounding box of a PolygonShape holds nodes of the shapes next to it
  // too
  Node::NodeSet nodes;
  for (auto itr
       = layer_nodes.qbegin(boost::geometry::index::intersects(shape_));
       itr != layer_nodes.qend();
       itr++) {
    if (contains((*itr)->getPoint())) {
      nodes.insert(*itr);
    }
  }
  return nodes;
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

///////////////////

RectShape::RectShape(const odb::Rect& shape, odb::dbTechLayer* layer)
    : Shape(shape, layer)
{
}

bool RectShape::contains(const odb::Point& pt) const
{
  return getShape().intersects(pt);
}

Shape::ConnectionSize RectShape::getConnectionSize(const odb::Point& pt0,
                                                   const odb::Point& pt1) const
{
  const int len_x = std::abs(pt1.getX() - pt0.getX());
  const int len_y = std::abs(pt1.getY() - pt0.getY());

  if (len_x > len_y) {
    return {.length = len_x, .width = getShape().dy()};
  }
  return {.length = len_y, .width = getShape().dx()};
}

std::optional<odb::Point> RectShape::getFillerPoint(const odb::Point& pt,
                                                    bool /* along_x */) const
{
  return pt;
}

///////////////////

PolygonShape::PolygonShape(const odb::Polygon& shape, odb::dbTechLayer* layer)
    : Shape(shape.getEnclosingRect(), layer), polygon_(shape)
{
}

bool PolygonShape::contains(const odb::Point& pt) const
{
  return boost::geometry::covered_by(pt, polygon_.getPoints());
}

Shape::ConnectionSize PolygonShape::getConnectionSize(
    const odb::Point& pt0,
    const odb::Point& pt1) const
{
  const double dx = pt1.x() - pt0.x();
  const double dy = pt1.y() - pt0.y();
  const double length = std::hypot(dx, dy);
  if (length == 0) {
    // no direction to measure along or across
    return {.length = 0, .width = getShape().dx()};
  }

  // the width is across the shape, square to the line between the points
  // and through the middle of them
  const auto [t_min, t_max] = clipLine(polygon_.getPoints(),
                                       (pt0.x() + pt1.x()) / 2.0,
                                       (pt0.y() + pt1.y()) / 2.0,
                                       -dy / length,
                                       dx / length);
  return {.length = static_cast<int>(std::lround(length)),
          .width = std::max(1, static_cast<int>(std::lround(t_max - t_min)))};
}

std::optional<odb::Point> PolygonShape::getFillerPoint(const odb::Point& pt,
                                                       bool along_x) const
{
  // the middle of the line across the shape through pt
  const auto [t_min, t_max] = clipLine(
      polygon_.getPoints(), pt.x(), pt.y(), along_x ? 0 : 1, along_x ? 1 : 0);
  if (t_min > t_max) {
    return std::nullopt;
  }

  const int middle = static_cast<int>(std::lround((t_min + t_max) / 2));
  odb::Point filler = pt;
  if (along_x) {
    filler.addY(middle);
  } else {
    filler.addX(middle);
  }
  return filler;
}

}  // namespace psm
