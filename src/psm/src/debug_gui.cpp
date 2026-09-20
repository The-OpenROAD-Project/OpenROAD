// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "debug_gui.h"

#include <algorithm>
#include <any>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "connection.h"
#include "ir_network.h"
#include "ir_solver.h"
#include "node.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "shape.h"
#include "sta/Scene.hh"
#include "web/core.h"

namespace psm {

SolverDescriptor::SolverDescriptor(
    const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers)
    : solvers_(solvers)
{
}

IRSolver* SolverDescriptor::getSolver(Node* node) const
{
  for (const auto& [net, solver] : solvers_) {
    if (solver->belongsTo(node)) {
      return solver.get();
    }
  }
  return nullptr;
}

IRSolver* SolverDescriptor::getSolver(Connection* connection) const
{
  for (const auto& [net, solver] : solvers_) {
    if (solver->belongsTo(connection)) {
      return solver.get();
    }
  }
  return nullptr;
}

/////////////////////////////////////

NodeDescriptor::NodeDescriptor(
    const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers)
    : SolverDescriptor(solvers)
{
}

std::string NodeDescriptor::getName(const std::any& object) const
{
  auto node = std::any_cast<Node*>(object);
  return node->getName();
}

bool NodeDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto node = std::any_cast<Node*>(object);
  bbox = odb::Rect(node->getPoint(), node->getPoint());
  return true;
}

web::Descriptor::Properties NodeDescriptor::getProperties(
    const std::any& object) const
{
  auto node = std::any_cast<Node*>(object);

  web::Descriptor::Properties props;

  auto gui = web::Gui::get();
  auto solver = getSolver(node);
  if (solver == nullptr) {
    return props;
  }

  std::vector<sta::Scene*> corners;
  if (solver != nullptr) {
    corners = solver->getCorners();
    props.push_back({"Net", gui->makeSelected(solver->getNet())});
  }

  props.push_back({"Layer", gui->makeSelected(node->getLayer())});
  props.push_back(
      {"X",
       web::Descriptor::Property::convert_dbu(node->getPoint().x(), true)});
  props.push_back(
      {"Y",
       web::Descriptor::Property::convert_dbu(node->getPoint().y(), true)});

  web::Descriptor::PropertyList net_voltages;
  for (auto* corner : corners) {
    net_voltages.emplace_back(corner->name(), solver->getNetVoltage(corner));
  }
  if (!net_voltages.empty()) {
    props.push_back({"Net voltage", net_voltages});
  }

  web::Descriptor::PropertyList voltages;
  for (auto* corner : corners) {
    if (!solver->hasSolution(corner)) {
      continue;
    }
    const auto volt = solver->getVoltage(corner, node);
    if (volt) {
      voltages.emplace_back(corner->name(), volt.value());
    } else {
      voltages.emplace_back(corner->name(), "N/A");
    }
  }
  if (!voltages.empty()) {
    props.push_back({"Voltage", voltages});
  }

  return props;
}

web::Selected NodeDescriptor::makeSelected(const std::any& object) const
{
  if (auto node = std::any_cast<Node*>(&object)) {
    return web::Selected(*node, this);
  }
  return web::Selected();
}

bool NodeDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_node = std::any_cast<Node*>(l);
  auto r_node = std::any_cast<Node*>(r);
  return l_node->compare(r_node) < 0;
}

void NodeDescriptor::highlight(const std::any& object,
                               web::Painter& painter) const
{
  auto node = std::any_cast<Node*>(object);
  auto& pt = node->getPoint();
  painter.drawCircle(pt.x(), pt.y(), 10);
}

/////////////////////////////////////

ITermNodeDescriptor::ITermNodeDescriptor(
    const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers)
    : NodeDescriptor(solvers)
{
}

std::string ITermNodeDescriptor::getName(const std::any& object) const
{
  auto node = std::any_cast<ITermNode*>(object);
  return node->getName();
}

bool ITermNodeDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto node = std::any_cast<ITermNode*>(object);
  bbox = node->getShape();
  return true;
}

web::Descriptor::Properties ITermNodeDescriptor::getProperties(
    const std::any& object) const
{
  auto node = std::any_cast<ITermNode*>(object);

  auto props = NodeDescriptor::getProperties(static_cast<Node*>(node));

  auto gui = web::Gui::get();
  props.push_back({"ITerm", gui->makeSelected(node->getITerm())});

  return props;
}

web::Selected ITermNodeDescriptor::makeSelected(const std::any& object) const
{
  if (auto node = std::any_cast<ITermNode*>(&object)) {
    return web::Selected(*node, this);
  }
  return web::Selected();
}

bool ITermNodeDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_node = std::any_cast<ITermNode*>(l);
  auto r_node = std::any_cast<ITermNode*>(r);
  return l_node->compare(r_node) < 0;
}

void ITermNodeDescriptor::highlight(const std::any& object,
                                    web::Painter& painter) const
{
  auto node = std::any_cast<ITermNode*>(object);
  NodeDescriptor::highlight(static_cast<Node*>(node), painter);
}

/////////////////////////////////////

BPinNodeDescriptor::BPinNodeDescriptor(
    const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers)
    : NodeDescriptor(solvers)
{
}

std::string BPinNodeDescriptor::getName(const std::any& object) const
{
  auto node = std::any_cast<BPinNode*>(object);
  return node->getName();
}

bool BPinNodeDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto node = std::any_cast<BPinNode*>(object);
  bbox = node->getShape();
  return true;
}

web::Descriptor::Properties BPinNodeDescriptor::getProperties(
    const std::any& object) const
{
  auto node = std::any_cast<BPinNode*>(object);

  auto props = NodeDescriptor::getProperties(static_cast<Node*>(node));

  auto gui = web::Gui::get();
  props.push_back({"BTerm", gui->makeSelected(node->getBPin()->getBTerm())});

  return props;
}

web::Selected BPinNodeDescriptor::makeSelected(const std::any& object) const
{
  if (auto node = std::any_cast<BPinNode*>(&object)) {
    return web::Selected(*node, this);
  }
  return web::Selected();
}

bool BPinNodeDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_node = std::any_cast<BPinNode*>(l);
  auto r_node = std::any_cast<BPinNode*>(r);
  return l_node->compare(r_node) < 0;
}

void BPinNodeDescriptor::highlight(const std::any& object,
                                   web::Painter& painter) const
{
  auto node = std::any_cast<BPinNode*>(object);
  NodeDescriptor::highlight(static_cast<Node*>(node), painter);
}

/////////////////////////////////////

ConnectionDescriptor::ConnectionDescriptor(
    const odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>>& solvers)
    : SolverDescriptor(solvers)
{
}

std::string ConnectionDescriptor::getName(const std::any& object) const
{
  auto conn = std::any_cast<Connection*>(object);
  return conn->getNode0()->getName() + "->" + conn->getNode1()->getName();
}

bool ConnectionDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  auto conn = std::any_cast<Connection*>(object);
  bbox = odb::Rect(conn->getNode0()->getPoint(), conn->getNode1()->getPoint());
  return true;
}

web::Descriptor::Properties ConnectionDescriptor::getProperties(
    const std::any& object) const
{
  auto conn = std::any_cast<Connection*>(object);

  web::Descriptor::Properties props;

  auto gui = web::Gui::get();

  auto solver = getSolver(conn);
  if (solver == nullptr) {
    return props;
  }

  std::vector<sta::Scene*> corners;
  if (solver != nullptr) {
    corners = solver->getCorners();
    props.push_back({"Net", gui->makeSelected(solver->getNet())});
  }

  props.push_back({"Node 0", gui->makeSelected(conn->getNode0())});
  props.push_back({"Node 1", gui->makeSelected(conn->getNode1())});

  props.push_back({"Is via", conn->isVia()});

  web::Descriptor::PropertyList resistances;
  for (auto* corner : corners) {
    const auto res_map = solver->getResistanceMap(corner);
    resistances.emplace_back(corner->name(), conn->getResistance(res_map));
  }
  if (!resistances.empty()) {
    props.push_back({"Resistances", resistances});
  }

  web::Descriptor::PropertyList currents;
  for (auto* corner : corners) {
    const auto res_map = solver->getResistanceMap(corner);
    const auto cond = conn->getConductance(res_map);

    const auto volt0 = solver->getVoltage(corner, conn->getNode0());
    const auto volt1 = solver->getVoltage(corner, conn->getNode1());

    if (volt0 && volt1) {
      const auto delta_v = std::abs(volt0.value() - volt1.value());
      currents.emplace_back(corner->name(), delta_v * cond);
    }
  }
  if (!currents.empty()) {
    props.push_back({"Currents", currents});
  }

  return props;
}

web::Selected ConnectionDescriptor::makeSelected(const std::any& object) const
{
  if (auto conn = std::any_cast<Connection*>(&object)) {
    return web::Selected(*conn, this);
  }
  return web::Selected();
}

bool ConnectionDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_conn = std::any_cast<Connection*>(l);
  auto r_conn = std::any_cast<Connection*>(r);
  return l_conn->compare(r_conn);
}

void ConnectionDescriptor::highlight(const std::any& object,
                                     web::Painter& painter) const
{
  auto conn = std::any_cast<Connection*>(object);
  painter.drawLine(conn->getNode0()->getPoint(), conn->getNode1()->getPoint());
}

/////////////////////////////////////

DebugGui::DebugGui(IRNetwork* network)
    : network_(network),
      control_group_("PSM: " + network_->getNet()->getName()),
      shape_color_(web::Painter::kWhite),
      node_color_(web::Painter::kCyan),
      src_node_color_(web::Painter::kMagenta),
      iterm_node_color_(web::Painter::kRed),
      bpin_node_color_(web::Painter::kBlue),
      connection_color_(web::Painter::kYellow),
      term_connection_color_(web::Painter::kRed),
      found_select_(false)
{
  addDisplayControl(kShapesText, true);
  addDisplayControl(kNodesText, true);
  addDisplayControl(kItermNodesText, true);
  addDisplayControl(kBpinNodesText, true);
  addDisplayControl(kConnectivityText, true);
  addDisplayControl(kSourceText, true);
  addDisplayControl(kSourceShapeText, true);

  web::Gui::get()->registerRenderer(this);
}

void DebugGui::reset()
{
  shapes_.clear();
  nodes_.clear();
  iterm_nodes_.clear();
  bpin_nodes_.clear();
  connections_.clear();
  sources_.clear();
  source_shapes_.clear();

  selected_shapes_.clear();
  selected_nodes_.clear();
  selected_connections_.clear();

  redraw();
}

void DebugGui::populate()
{
  reset();

  for (const auto& [layer, layer_shapes] : network_->getShapes()) {
    std::vector<Shape*> values;
    for (const auto& shape : layer_shapes) {
      values.emplace_back(shape.get());
    }

    shapes_[layer] = ShapeTree(values.begin(), values.end());
  }

  for (const auto& [layer, layer_nodes] : network_->getNodes()) {
    std::vector<Node*> values;
    for (const auto& node : layer_nodes) {
      values.emplace_back(node.get());
    }

    nodes_[layer] = NodeTree(values.begin(), values.end());
  }

  odb::PtrMap<odb::dbTechLayer, std::vector<ITermNode*>> iterms;
  for (const auto& node : network_->getITermNodes()) {
    iterms[node->getLayer()].emplace_back(node.get());
  }
  for (const auto& [layer, layer_nodes] : iterms) {
    iterm_nodes_[layer] = ITermNodeTree(layer_nodes.begin(), layer_nodes.end());
  }
  iterms.clear();

  odb::PtrMap<odb::dbTechLayer, std::vector<BPinNode*>> bpins;
  for (const auto& node : network_->getBPinNodes()) {
    bpins[node->getLayer()].emplace_back(node.get());
  }
  for (const auto& [layer, layer_nodes] : bpins) {
    bpin_nodes_[layer] = BPinNodeTree(layer_nodes.begin(), layer_nodes.end());
  }
  bpins.clear();

  odb::PtrMap<odb::dbTechLayer, std::vector<ConnectionValue>> conns;
  for (const auto& conn : network_->getConnections()) {
    const odb::Point& pt0 = conn->getNode0()->getPoint();
    const odb::Point& pt1 = conn->getNode1()->getPoint();
    const Line line(Point(pt0.x(), pt0.y()), Point(pt1.x(), pt1.y()));
    conns[conn->getNode0()->getLayer()].emplace_back(line, conn.get());

    if (conn->getNode0()->getLayer() != conn->getNode1()->getLayer()) {
      conns[conn->getNode1()->getLayer()].emplace_back(line, conn.get());
    }
  }

  for (const auto& [layer, layer_conns] : conns) {
    connections_[layer]
        = ConnectionTree(layer_conns.begin(), layer_conns.end());
  }

  redraw();
}

void DebugGui::drawShape(const Shape* shape, web::Painter& painter) const
{
  const bool bold = isSelected(shape);
  if (bold) {
    painter.saveState();
    painter.setPen(shape_color_, /* cosmetic */ true, kBoldMultiplier);
  }
  painter.drawRect(shape->getShape());
  if (bold) {
    painter.restoreState();
  }
}

void DebugGui::drawNode(const Node* node,
                        web::Painter& painter,
                        const web::Painter::Color& color) const
{
  const bool bold = isSelected(node);
  if (bold) {
    painter.saveState();
    painter.setPen(color, /* cosmetic */ true, kNodePenWidth * kBoldMultiplier);
  }
  const odb::Point& pt = node->getPoint();
  painter.drawCircle(pt.getX(), pt.getY(), kNodeSize);
  if (bold) {
    painter.restoreState();
  }
}

void DebugGui::drawConnection(const Connection* connection,
                              web::Painter& painter) const
{
  const bool is_terminal_connection
      = dynamic_cast<const TermConnection*>(connection) != nullptr;
  web::Painter::Color color
      = is_terminal_connection ? term_connection_color_ : connection_color_;
  const bool bold = isSelected(connection);
  if (bold) {
    painter.setPen(color, /* cosmetic */ true, kBoldMultiplier);
  } else {
    painter.setPen(color, /* cosmetic */ true);
  }
  Node* node = connection->getNode0();
  const odb::Point& pt = node->getPoint();

  if (connection->isVia()) {
    // draw an X to indicate cross layer connection
    painter.drawX(pt.getX(), pt.getY(), kViaSize);
  } else {
    const Node* other = connection->getOtherNode(node);
    painter.drawLine(pt, other->getPoint());
  }
}

void DebugGui::drawSource(const Node* node, web::Painter& painter) const
{
  const int src_node_size
      = std::max(kSrcNodeMaxSize,
                 static_cast<int>(static_cast<double>(kSrcNodeMaxSize)
                                  / painter.getPixelsPerDBU()));

  const odb::Point& pt = node->getPoint();
  painter.drawCircle(pt.getX(), pt.getY(), src_node_size);
}

void DebugGui::drawSource(const odb::Rect& rect, web::Painter& painter) const
{
  painter.drawRect(rect);
}

void DebugGui::drawLayer(odb::dbTechLayer* layer, web::Painter& painter)
{
  const odb::Rect& rect = painter.getBounds();

  if (checkDisplayControl(kShapesText)) {
    painter.setPen(shape_color_, /* cosmetic */ true);

    for (auto shape_itr
         = shapes_[layer].qbegin(boost::geometry::index::intersects(rect));
         shape_itr != shapes_[layer].qend();
         shape_itr++) {
      drawShape(*shape_itr, painter);
    }
  }

  if (checkDisplayControl(kNodesText)) {
    painter.setPen(node_color_, /* cosmetic */ true, kNodePenWidth);

    for (auto node_itr
         = nodes_[layer].qbegin(boost::geometry::index::intersects(rect));
         node_itr != nodes_[layer].qend();
         node_itr++) {
      drawNode(*node_itr, painter, node_color_);
    }
  }

  if (checkDisplayControl(kItermNodesText)) {
    painter.setPen(iterm_node_color_, /* cosmetic */ true, kNodePenWidth);

    for (auto node_itr
         = iterm_nodes_[layer].qbegin(boost::geometry::index::intersects(rect));
         node_itr != iterm_nodes_[layer].qend();
         node_itr++) {
      drawNode(*node_itr, painter, iterm_node_color_);
    }
  }

  if (checkDisplayControl(kBpinNodesText)) {
    painter.setPen(bpin_node_color_, /* cosmetic */ true, kNodePenWidth);

    for (auto node_itr
         = bpin_nodes_[layer].qbegin(boost::geometry::index::intersects(rect));
         node_itr != bpin_nodes_[layer].qend();
         node_itr++) {
      drawNode(*node_itr, painter, bpin_node_color_);
    }
  }

  if (checkDisplayControl(kConnectivityText)) {
    for (auto conn_itr
         = connections_[layer].qbegin(boost::geometry::index::intersects(rect));
         conn_itr != connections_[layer].qend();
         conn_itr++) {
      drawConnection(conn_itr->second, painter);
    }
  }

  if (checkDisplayControl(kSourceShapeText)) {
    web::Painter::Color color = src_node_color_;
    color.a = 100;
    painter.setPen(color, /* cosmetic */ true, 1);

    for (auto node_itr = source_shapes_[layer].qbegin(
             boost::geometry::index::intersects(rect));
         node_itr != source_shapes_[layer].qend();
         node_itr++) {
      drawSource(*node_itr, painter);
    }
  }

  if (checkDisplayControl(kSourceText)) {
    painter.setPen(src_node_color_, /* cosmetic */ true, kNodePenWidth);

    for (auto node_itr
         = sources_[layer].qbegin(boost::geometry::index::intersects(rect));
         node_itr != sources_[layer].qend();
         node_itr++) {
      drawSource(*node_itr, painter);
    }
  }

  web::DiscreteLegend legend;
  legend.addLegendKey(shape_color_, "Shape");
  legend.addLegendKey(node_color_, "Node");
  legend.addLegendKey(iterm_node_color_, "ITerm Node");
  legend.addLegendKey(bpin_node_color_, "BPin Node");
  legend.addLegendKey(connection_color_, "Connection");
  legend.addLegendKey(term_connection_color_, "Terminal Connection");
  legend.addLegendKey(src_node_color_, "Source Node/Shape");
  legend.draw(painter);
}

web::SelectionSet DebugGui::select(odb::dbTechLayer* layer,
                                   const odb::Rect& region)
{
  if (layer == nullptr) {
    if (!found_select_) {
      selected_shapes_.clear();
      selected_nodes_.clear();
      selected_connections_.clear();
    }
    found_select_ = false;
  } else {
    web::SelectionSet selection;

    if (checkDisplayControl(kShapesText)) {
      for (auto shape_itr
           = shapes_[layer].qbegin(boost::geometry::index::intersects(region));
           shape_itr != shapes_[layer].qend();
           shape_itr++) {
        selected_shapes_.insert(*shape_itr);
      }
    }
    if (checkDisplayControl(kNodesText)) {
      for (auto node_itr
           = nodes_[layer].qbegin(boost::geometry::index::intersects(region));
           node_itr != nodes_[layer].qend();
           node_itr++) {
        selected_nodes_.insert(*node_itr);
        selection.insert(web::Gui::get()->makeSelected(*node_itr));
      }
    }
    if (checkDisplayControl(kItermNodesText)) {
      for (auto node_itr = iterm_nodes_[layer].qbegin(
               boost::geometry::index::intersects(region));
           node_itr != iterm_nodes_[layer].qend();
           node_itr++) {
        selected_nodes_.insert(*node_itr);
        selection.insert(web::Gui::get()->makeSelected(*node_itr));
      }
    }
    if (checkDisplayControl(kBpinNodesText)) {
      for (auto node_itr = bpin_nodes_[layer].qbegin(
               boost::geometry::index::intersects(region));
           node_itr != bpin_nodes_[layer].qend();
           node_itr++) {
        selected_nodes_.insert(*node_itr);
        selection.insert(web::Gui::get()->makeSelected(*node_itr));
      }
    }
    if (checkDisplayControl(kConnectivityText)) {
      for (auto conn_itr = connections_[layer].qbegin(
               boost::geometry::index::intersects(region));
           conn_itr != connections_[layer].qend();
           conn_itr++) {
        selected_connections_.insert(conn_itr->second);
        selection.insert(web::Gui::get()->makeSelected(conn_itr->second));
      }
    }

    if (!selection.empty()) {
      found_select_ = true;
    }

    return selection;
  }

  return {};
}

bool DebugGui::isSelected(const Node* node) const
{
  return selected_nodes_.find(node) != selected_nodes_.end();
}

bool DebugGui::isSelected(const Shape* shape) const
{
  return selected_shapes_.find(shape) != selected_shapes_.end();
}

bool DebugGui::isSelected(const Connection* connection) const
{
  return selected_connections_.find(connection) != selected_connections_.end();
}

void DebugGui::setSources(const SourceNodes& sources)
{
  sources_.clear();

  odb::PtrMap<odb::dbTechLayer, std::vector<Node*>> srcs;
  for (const auto& node : sources) {
    srcs[node->getLayer()].emplace_back(node->getSource());
  }

  for (const auto& [layer, values] : srcs) {
    sources_[layer] = NodeTree(values.begin(), values.end());
  }

  redraw();
}

void DebugGui::setSourceShapes(odb::dbTechLayer* layer,
                               const std::set<odb::Rect>& shapes)
{
  source_shapes_.clear();

  source_shapes_[layer] = RectTree(shapes.begin(), shapes.end());

  redraw();
}

}  // namespace psm
