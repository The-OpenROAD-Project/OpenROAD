// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "ir_network.h"
#include "node.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "web/core.h"

namespace psm {
class Node;
class ITermNode;
class BPinNode;
class Shape;
class Connection;
class SourceNode;
class IRSolver;
class IRNetwork;

class SolverDescriptor : public web::Descriptor
{
 public:
  SolverDescriptor(
      const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers);

 protected:
  IRSolver* getSolver(Node* node) const;
  IRSolver* getSolver(Connection* connection) const;

 private:
  const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers_;
};

class NodeDescriptor : public SolverDescriptor
{
 public:
  NodeDescriptor(
      const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override { return "PSM Node"; }
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void visitAllObjects(
      const std::function<void(const web::Selected&)>&) const override
  {
  }
  web::Descriptor::Properties getProperties(
      const std::any& object) const override;
  web::Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void highlight(const std::any& object, web::Painter& painter) const override;
};

class ITermNodeDescriptor : public NodeDescriptor
{
 public:
  ITermNodeDescriptor(
      const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override { return "PSM ITerm Node"; }
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  web::Descriptor::Properties getProperties(
      const std::any& object) const override;
  web::Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void highlight(const std::any& object, web::Painter& painter) const override;
};

class BPinNodeDescriptor : public NodeDescriptor
{
 public:
  BPinNodeDescriptor(
      const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override { return "PSM BPin Node"; }
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  web::Descriptor::Properties getProperties(
      const std::any& object) const override;
  web::Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void highlight(const std::any& object, web::Painter& painter) const override;
};

class ConnectionDescriptor : public SolverDescriptor
{
 public:
  ConnectionDescriptor(
      const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override { return "PSM Connection"; }
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void visitAllObjects(
      const std::function<void(const web::Selected&)>&) const override
  {
  }
  web::Descriptor::Properties getProperties(
      const std::any& object) const override;
  web::Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void highlight(const std::any& object, web::Painter& painter) const override;
};

class DebugGui : public web::Renderer
{
 public:
  DebugGui(IRNetwork* network);

  web::SelectionSet select(odb::dbTechLayer* layer,
                           const odb::Rect& region) override;

  void populate();
  void reset();

  void drawLayer(odb::dbTechLayer* layer, web::Painter& painter) override;

  const char* getDisplayControlGroupName() override
  {
    return control_group_.c_str();
  }

  void setSources(const SourceNodes& sources);
  void setSourceShapes(odb::dbTechLayer* layer,
                       const std::set<odb::Rect>& shapes);

 private:
  using Point
      = boost::geometry::model::d2::point_xy<int,
                                             boost::geometry::cs::cartesian>;
  using Line = boost::geometry::model::segment<Point>;
  using ConnectionValue = std::pair<Line, Connection*>;
  using ShapeTree
      = boost::geometry::index::rtree<Shape*,
                                      boost::geometry::index::quadratic<16>,
                                      RectIndexableGetter<Shape>>;
  using ConnectionTree
      = boost::geometry::index::rtree<ConnectionValue,
                                      boost::geometry::index::quadratic<16>>;
  using NodeTree
      = boost::geometry::index::rtree<Node*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<Node>>;
  using ITermNodeTree
      = boost::geometry::index::rtree<ITermNode*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<ITermNode>>;
  using BPinNodeTree
      = boost::geometry::index::rtree<BPinNode*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<BPinNode>>;
  using RectTree
      = boost::geometry::index::rtree<odb::Rect,
                                      boost::geometry::index::quadratic<16>>;

  bool isSelected(const Node* node) const;
  bool isSelected(const Shape* shape) const;
  bool isSelected(const Connection* connection) const;

  void drawShape(const Shape* shape, web::Painter& painter) const;
  void drawNode(const Node* node,
                web::Painter& painter,
                const web::Painter::Color& color) const;
  void drawSource(const Node* node, web::Painter& painter) const;
  void drawSource(const odb::Rect& rect, web::Painter& painter) const;
  void drawConnection(const Connection* connection,
                      web::Painter& painter) const;

  IRNetwork* network_;

  std::string control_group_;

  const web::Painter::Color shape_color_;
  const web::Painter::Color node_color_;
  const web::Painter::Color src_node_color_;
  const web::Painter::Color iterm_node_color_;
  const web::Painter::Color bpin_node_color_;
  const web::Painter::Color connection_color_;
  const web::Painter::Color term_connection_color_;

  bool found_select_;

  odb::PtrMap<odb::dbTechLayer, ShapeTree> shapes_;
  odb::PtrMap<odb::dbTechLayer, NodeTree> nodes_;
  odb::PtrMap<odb::dbTechLayer, ITermNodeTree> iterm_nodes_;
  odb::PtrMap<odb::dbTechLayer, BPinNodeTree> bpin_nodes_;
  odb::PtrMap<odb::dbTechLayer, ConnectionTree> connections_;

  odb::PtrMap<odb::dbTechLayer, NodeTree> sources_;
  odb::PtrMap<odb::dbTechLayer, RectTree> source_shapes_;

  std::set<const Shape*> selected_shapes_;
  std::set<const Node*> selected_nodes_;
  std::set<const Connection*> selected_connections_;

  static constexpr const char* kShapesText = "Shapes";
  static constexpr const char* kNodesText = "Nodes";
  static constexpr const char* kItermNodesText = "ITerm nodes";
  static constexpr const char* kBpinNodesText = "BPin nodes";
  static constexpr const char* kConnectivityText = "Node connectivity";
  static constexpr const char* kSourceText = "Source nodes";
  static constexpr const char* kSourceShapeText = "Source shapes";

  static constexpr int kBoldMultiplier = 2;
  static constexpr int kNodePenWidth = 2;
  static constexpr int kNodeSize = 10;
  static constexpr int kViaSize = 10;
  static constexpr int kSrcNodeMaxSize = 25;
};

}  // namespace psm
