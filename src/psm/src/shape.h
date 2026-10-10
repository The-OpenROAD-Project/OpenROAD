// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
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
  virtual ~Shape() = default;

  std::vector<std::unique_ptr<Node>> createFillerNodes(
      int max_distance,
      const IRNetwork::NodeTree& layer_nodes);
  Connections connectNodes(const IRNetwork::NodeTree& layer_nodes);
  std::set<Node*> cleanupNodes(
      int min_distance,
      const IRNetwork::NodeTree& layer_nodes,
      const std::function<void(Node*, Node*)>& copy_func,
      const std::set<Node*>& shared_nodes);

  // The bounding box of the shape, which is the shape itself for a
  // RectShape
  const odb::Rect& getShape() const { return shape_; }
  // The shape of a PolygonShape, nullptr for a RectShape
  virtual const odb::Polygon* getPolygon() const = 0;
  // True when the point is in the shape or on its boundary
  virtual bool contains(const odb::Point& pt) const = 0;

  odb::dbTechLayer* getLayer() const;

  std::string describe(double dbus) const;

  void setID(std::size_t id) { id_ = id; }
  std::size_t getID() const { return id_; }

 protected:
  Shape(const odb::Rect& shape, odb::dbTechLayer* layer);

  struct ConnectionSize
  {
    int length;
    int width;
  };
  // The length and width of the metal connecting two nodes in the shape
  virtual ConnectionSize getConnectionSize(const odb::Point& pt0,
                                           const odb::Point& pt1) const
      = 0;
  // Where the filler node for pt goes, pt steps along the middle of the
  // bounding box in x when along_x and in y otherwise. Empty when the shape
  // holds no metal there.
  virtual std::optional<odb::Point> getFillerPoint(const odb::Point& pt,
                                                   bool along_x) const
      = 0;

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

class RectShape : public Shape
{
 public:
  RectShape(const odb::Rect& shape, odb::dbTechLayer* layer);

  const odb::Polygon* getPolygon() const override { return nullptr; }
  bool contains(const odb::Point& pt) const override;

 protected:
  ConnectionSize getConnectionSize(const odb::Point& pt0,
                                   const odb::Point& pt1) const override;
  std::optional<odb::Point> getFillerPoint(const odb::Point& pt,
                                           bool along_x) const override;
};

// A shape with 45 degree edges: a trapezoid with vertical parallel sides,
// as the network slices its polygons into, so it holds the center of its
// bounding box.
class PolygonShape : public Shape
{
 public:
  PolygonShape(const odb::Polygon& shape, odb::dbTechLayer* layer);

  const odb::Polygon* getPolygon() const override { return &polygon_; }
  bool contains(const odb::Point& pt) const override;

 protected:
  // Measured along the line between the points and across the shape square
  // to it, so a 45 degree wire has its own length and width rather than
  // those of its bounding box
  ConnectionSize getConnectionSize(const odb::Point& pt0,
                                   const odb::Point& pt1) const override;
  // On the middle of the shape rather than of its bounding box, which can
  // lie outside of the shape
  std::optional<odb::Point> getFillerPoint(const odb::Point& pt,
                                           bool along_x) const override;

 private:
  odb::Polygon polygon_;
};

}  // namespace psm
