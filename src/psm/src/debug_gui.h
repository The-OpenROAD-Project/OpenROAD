/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <boost/geometry.hpp>
#include <boost/polygon/polygon.hpp>
#include <map>
#include <memory>
#include <set>

#include "gui/gui.h"
#include "ir_network.h"
#include "odb/geom_boost.h"

namespace psm {
class Node;
class ITermNode;
class BPinNode;
class Shape;
class Connection;
class SourceNode;
class IRSolver;
class IRNetwork;

class SolverDescriptor : public gui::Descriptor
{
 public:
  SolverDescriptor(
      const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers);
  virtual ~SolverDescriptor() = default;

 protected:
  IRSolver* getSolver(Node* node) const;
  IRSolver* getSolver(Connection* connection) const;

 private:
  const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers_;
};

class NodeDescriptor : public SolverDescriptor
{
 public:
  NodeDescriptor(
      const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override { return "PSM Node"; }
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  bool getAllObjects(gui::SelectionSet& /* objects */) const override
  {
    return false;
  }
  gui::Descriptor::Properties getProperties(std::any object) const override;
  gui::Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  void highlight(std::any object, gui::Painter& painter) const override;
};

class ITermNodeDescriptor : public NodeDescriptor
{
 public:
  ITermNodeDescriptor(
      const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override { return "PSM ITerm Node"; }
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  gui::Descriptor::Properties getProperties(std::any object) const override;
  gui::Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  void highlight(std::any object, gui::Painter& painter) const override;
};

class BPinNodeDescriptor : public NodeDescriptor
{
 public:
  BPinNodeDescriptor(
      const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override { return "PSM BPin Node"; }
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  gui::Descriptor::Properties getProperties(std::any object) const override;
  gui::Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  void highlight(std::any object, gui::Painter& painter) const override;
};

class ConnectionDescriptor : public SolverDescriptor
{
 public:
  ConnectionDescriptor(
      const std::map<odb::dbNet*, std::unique_ptr<IRSolver>>& solvers);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override { return "PSM Connection"; }
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  bool getAllObjects(gui::SelectionSet& /* objects */) const override
  {
    return false;
  }
  gui::Descriptor::Properties getProperties(std::any object) const override;
  gui::Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  void highlight(std::any object, gui::Painter& painter) const override;
};

class DebugGui : public gui::Renderer
{
 public:
  DebugGui(IRNetwork* network);

  gui::SelectionSet select(odb::dbTechLayer* layer,
                           const odb::Rect& region) override;

  void populate();
  void reset();

  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  const char* getDisplayControlGroupName() override
  {
    return control_group_.c_str();
  }

  void setSources(const std::vector<std::unique_ptr<SourceNode>>& sources);
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

  void drawShape(const Shape* shape, gui::Painter& painter) const;
  void drawNode(const Node* node,
                gui::Painter& painter,
                const gui::Painter::Color& color) const;
  void drawSource(const Node* node, gui::Painter& painter) const;
  void drawSource(const odb::Rect& rect, gui::Painter& painter) const;
  void drawConnection(const Connection* connection,
                      gui::Painter& painter) const;

  IRNetwork* network_;

  std::string control_group_;

  const gui::Painter::Color shape_color_;
  const gui::Painter::Color node_color_;
  const gui::Painter::Color src_node_color_;
  const gui::Painter::Color iterm_node_color_;
  const gui::Painter::Color bpin_node_color_;
  const gui::Painter::Color connection_color_;
  const gui::Painter::Color term_connection_color_;

  bool found_select_;

  std::map<odb::dbTechLayer*, ShapeTree> shapes_;
  std::map<odb::dbTechLayer*, NodeTree> nodes_;
  std::map<odb::dbTechLayer*, ITermNodeTree> iterm_nodes_;
  std::map<odb::dbTechLayer*, BPinNodeTree> bpin_nodes_;
  std::map<odb::dbTechLayer*, ConnectionTree> connections_;

  std::map<odb::dbTechLayer*, NodeTree> sources_;
  std::map<odb::dbTechLayer*, RectTree> source_shapes_;

  std::set<const Shape*> selected_shapes_;
  std::set<const Node*> selected_nodes_;
  std::set<const Connection*> selected_connections_;

  static constexpr const char* shapes_text_ = "Shapes";
  static constexpr const char* nodes_text_ = "Nodes";
  static constexpr const char* iterm_nodes_text_ = "ITerm nodes";
  static constexpr const char* bpin_nodes_text_ = "BPin nodes";
  static constexpr const char* connectivity_text_ = "Node connectivity";
  static constexpr const char* source_text_ = "Source nodes";
  static constexpr const char* source_shape_text_ = "Source shapes";

  static constexpr int bold_multiplier_ = 2;
  static constexpr int node_pen_width_ = 2;
  static constexpr int node_size_ = 10;
  static constexpr int via_size_ = 10;
  static constexpr int src_node_max_size_ = 25;
};

}  // namespace psm
