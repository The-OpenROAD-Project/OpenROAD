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

#include "ir_network.h"

#include <fstream>
#include <list>

#include "connection.h"
#include "node.h"
#include "odb/dbShape.h"
#include "odb/geom_boost.h"
#include "shape.h"
#include "utl/timer.h"

namespace psm {

IRNetwork::IRNetwork(odb::dbNet* net, utl::Logger* logger, bool floorplanning)
    : net_(net), logger_(logger), floorplanning_(floorplanning)
{
  if (!net_->getSigType().isSupply()) {
    logger_->error(utl::PSM, 87, "{} is not a supply net.", net_->getName());
  }
  initMinimumNodePitch();
  construct();
}

void IRNetwork::initMinimumNodePitch()
{
  min_node_pitch_.clear();

  const double dbus = getBlock()->getDbUnitsPerMicron();
  const int min_pitch = dbus * min_node_pitch_um_;

  for (auto* layer : getTech()->getLayers()) {
    if (layer->getRoutingLevel() != 0) {
      min_node_pitch_[layer] = std::min(
          min_pitch,
          min_node_pitch_multiplier_ * std::max(1, layer->getPitch()));
    }
  }

  for (const auto& [layer, pitch] : min_node_pitch_) {
    debugPrint(logger_,
               utl::PSM,
               "construct",
               2,
               "Node pitch on {} -> {:.4f} um",
               layer->getName(),
               pitch / dbus);
  }
}

odb::dbBlock* IRNetwork::getBlock() const
{
  return net_->getBlock();
}

odb::dbTech* IRNetwork::getTech() const
{
  return getBlock()->getTech();
}

void IRNetwork::reset()
{
  shapes_.clear();
  nodes_.clear();
  connections_.clear();

  iterm_nodes_.clear();
  bpin_nodes_.clear();

  recoverMemory();
}

void IRNetwork::construct()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Construct: {}");

  reset();

  generateRoutingLayerShapesAndNodes();
  generateCutLayerNodes();
  generateTopLayerFillerNodes();
  sortNodes();
  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    dumpNodes("intital_nodes");
  }

  cleanupNodes();
  cleanupConnections();
  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    dumpNodes("after_node_cleanup");
  }

  connectLayerNodes();
  sortConnections();
  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    dumpNodes("after_layer_connections");
  }
  cleanupConnections();
  sortConnections();
  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    dumpNodes("after_connection_cleanup");
  }

  if (logger_->debugCheck(utl::PSM, "construct", 1)) {
    reportStats();
  }
}

void IRNetwork::recoverMemory()
{
  for (auto& [layer, nodes] : nodes_) {
    nodes.shrink_to_fit();
    debugPrint(logger_,
               utl::PSM,
               "construct",
               2,
               "Layer node memory usage: {} / {}",
               nodes.size(),
               nodes.capacity());
  }

  connections_.shrink_to_fit();
  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Connections memory usage: {} / {}",
             connections_.size(),
             connections_.capacity());
}

IRNetwork::Polygon90 IRNetwork::rectToPolygon(const odb::Rect& rect) const
{
  using Pt = Polygon90::point_type;

  std::array<Pt, 4> pts = {Pt(rect.xMin(), rect.yMin()),
                           Pt(rect.xMax(), rect.yMin()),
                           Pt(rect.xMax(), rect.yMax()),
                           Pt(rect.xMin(), rect.yMax())};

  Polygon90 poly;
  poly.set(pts.begin(), pts.end());
  return poly;
}

IRNetwork::LayerMap<IRNetwork::Polygon90Set> IRNetwork::generatePolygonsFromBox(
    odb::dbBox* box,
    const odb::dbTransform& transform) const
{
  using boost::polygon::operators::operator+=;

  LayerMap<Polygon90Set> shapes_by_layer;

  if (box->isVia()) {
    // handle as via
    std::vector<odb::dbShape> via_shapes;
    box->getViaBoxes(via_shapes);

    for (const auto& shape : via_shapes) {
      auto* layer = shape.getTechLayer();
      if (layer->getRoutingLevel() == 0) {
        // via box
      } else {
        // enclosure box
        Polygon90Set& shapes = shapes_by_layer[layer];

        odb::Rect rect = shape.getBox();
        transform.apply(rect);

        shapes += rectToPolygon(rect);
      }
    }
  } else {
    // handle as shape
    auto* layer = box->getTechLayer();

    if (layer->getRoutingLevel() != 0) {
      // Only collect shapes on routing layers
      odb::Rect rect = box->getBox();
      transform.apply(rect);
      shapes_by_layer[layer] += rectToPolygon(rect);
    }
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<IRNetwork::Polygon90Set>
IRNetwork::generatePolygonsFromSWire(odb::dbSWire* wire)
{
  using boost::polygon::operators::operator+=;

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from SWire: {}");

  LayerMap<Polygon90Set> shapes_by_layer;

  for (odb::dbSBox* box : wire->getWires()) {
    for (const auto& [layer, polygon] :
         generatePolygonsFromBox(box, odb::dbTransform())) {
      shapes_by_layer[layer] += polygon;
    }
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<IRNetwork::Polygon90Set>
IRNetwork::generatePolygonsFromITerms(std::vector<TerminalNode*>& terminals)
{
  using boost::polygon::operators::operator+=;

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from ITerms: {}");

  auto* base_layer = getTech()->findRoutingLayer(1);

  LayerMap<Polygon90Set> shapes_by_layer;
  bool floorplan_asseted_ = false;
  for (auto* iterm : net_->getITerms()) {
    auto* inst = iterm->getInst();
    if (!inst->isPlaced()) {
      continue;
    }
    if (floorplanning_ && !inst->getPlacementStatus().isFixed()) {
      floorplan_asseted_ = true;
      continue;
    }
    const auto transform = inst->getTransform();
    if (iterm->getBBox().isInverted()) {
      // iterm has no physical shape, so ignore.
      continue;
    }

    int x, y;
    iterm->getAvgXY(&x, &y);
    auto base_node
        = std::make_unique<ITermNode>(iterm, odb::Point(x, y), base_layer);

    bool has_routing_term = false;
    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        const auto pin_shapes = generatePolygonsFromBox(geom, transform);
        if (pin_shapes.empty()) {
          continue;
        }

        for (const auto& [layer, shapes] : pin_shapes) {
          shapes_by_layer[layer] += shapes;
        }

        if (geom->isVia()) {
          for (const auto& [layer, shapes] : pin_shapes) {
            std::vector<odb::Rect> via_rects;
            shapes.get_rectangles(via_rects);
            for (const auto& pin_shape : via_rects) {
              has_routing_term = true;

              // create iterm nodes
              auto center = std::make_unique<TerminalNode>(pin_shape, layer);
              terminals.push_back(center.get());

              connections_.push_back(std::make_unique<TermConnection>(
                  base_node.get(), center.get()));

              nodes_[layer].push_back(std::move(center));
            }
          }
        } else {
          auto* layer = geom->getTechLayer();

          has_routing_term = true;

          odb::Rect pin_shape = geom->getBox();
          transform.apply(pin_shape);

          // create iterm nodes
          auto center = std::make_unique<TerminalNode>(pin_shape, layer);
          terminals.push_back(center.get());

          connections_.push_back(
              std::make_unique<TermConnection>(base_node.get(), center.get()));

          nodes_[layer].push_back(std::move(center));
        }
      }
    }

    if (has_routing_term) {
      iterm_nodes_.push_back(std::move(base_node));
    }
  }

  if (floorplanning_ && !floorplan_asseted_) {
    // Since floorplanning did not impact solution, mark this okay for analysis
    floorplanning_ = false;
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<IRNetwork::Polygon90Set>
IRNetwork::generatePolygonsFromBTerms(std::vector<TerminalNode*>& terminals)
{
  using boost::polygon::operators::operator+=;

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from BTerms: {}");

  LayerMap<Polygon90Set> shapes_by_layer;

  for (auto* bterm : net_->getBTerms()) {
    for (auto* bpin : bterm->getBPins()) {
      for (auto* geom : bpin->getBoxes()) {
        for (const auto& [layer, shapes] :
             generatePolygonsFromBox(geom, odb::dbTransform())) {
          shapes_by_layer[layer] += shapes;
        }

        if (geom->isVia()) {
          continue;
        }

        auto* layer = geom->getTechLayer();
        const odb::Rect pin_shape = geom->getBox();

        // create bpin nodes
        auto term = std::make_unique<TerminalNode>(pin_shape, layer);
        auto pin_node = std::make_unique<BPinNode>(bpin, pin_shape, layer);

        connections_.push_back(
            std::make_unique<TermConnection>(term.get(), pin_node.get()));

        nodes_[layer].push_back(std::move(term));
        bpin_nodes_.push_back(std::move(pin_node));
      }
    }
  }

  return shapes_by_layer;
}

void IRNetwork::processPolygonToRectangles(
    odb::dbTechLayer* layer,
    const IRNetwork::Polygon90& polygon,
    const IRNetwork::TerminalTree& terminals,
    std::vector<std::unique_ptr<Shape>>& new_shapes,
    std::vector<std::unique_ptr<Node>>& new_nodes,
    std::map<Shape*, std::set<Node*>>& terminal_connections)
{
  using boost::polygon::operators::operator+=;

  auto get_layer_orientation
      = [](odb::dbTechLayer* layer) -> boost::polygon::orientation_2d_enum {
    switch (layer->getDirection().getValue()) {
      case odb::dbTechLayerDir::NONE:
      case odb::dbTechLayerDir::HORIZONTAL:
        return boost::polygon::orientation_2d_enum::VERTICAL;
        break;
      case odb::dbTechLayerDir::VERTICAL:
        return boost::polygon::orientation_2d_enum::HORIZONTAL;
    }
    return boost::polygon::orientation_2d_enum::VERTICAL;
  };

  Polygon90Set shape_poly_set;
  shape_poly_set += polygon;

  std::vector<odb::Rect> search_rect_shapes;
  shape_poly_set.get_rectangles(search_rect_shapes,
                                get_layer_orientation(layer));

  using EdgeTree
      = boost::geometry::index::rtree<Node*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<Node>>;

  EdgeTree poly_edge_nodes;
  auto search_start = search_rect_shapes.begin();
  for (const auto& rect : search_rect_shapes) {
    // remove current rect from search
    std::advance(search_start, 1);

    std::set<odb::Point> nodes;
    for (auto search = search_start; search != search_rect_shapes.end();
         search++) {
      if (search->intersects(rect)) {
        const odb::Rect intersect = search->intersect(rect);
        nodes.emplace(intersect.center());
      }
    }

    auto shape = std::make_unique<Shape>(rect, layer);

    // Create starter nodes
    nodes.emplace(rect.center());
    for (const auto& pt : nodes) {
      auto node = std::make_unique<Node>(pt, layer);

      // add edge to tree
      poly_edge_nodes.insert(node.get());

      new_nodes.push_back(std::move(node));
    }

    // check terminals
    auto& shape_terms = terminal_connections[shape.get()];
    for (auto itr = terminals.qbegin(
             boost::geometry::index::intersects(rect)
             && boost::geometry::index::satisfies([layer](const auto& other) {
                  return layer == other->getLayer();
                }));
         itr != terminals.qend();
         itr++) {
      auto* node = *itr;
      if (rect.overlaps(node->getPoint())) {
        shape_terms.insert(node);
      }
    }

    new_shapes.push_back(std::move(shape));
  }
}

IRNetwork::TerminalTree IRNetwork::getTerminalTree(
    const std::vector<TerminalNode*>& terminals) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Construct terminal node rtree: {}");

  return TerminalTree(terminals.begin(), terminals.end());
}

void IRNetwork::generateRoutingLayerShapesAndNodes()
{
  using boost::polygon::operators::operator+=;

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes: {}");

  LayerMap<Polygon90Set> shapes_by_layer;

  // Collect wires
  for (odb::dbSWire* wire : net_->getSWires()) {
    for (const auto& [layer, shapes] : generatePolygonsFromSWire(wire)) {
      shapes_by_layer[layer] += shapes;
    }
  }

  std::vector<TerminalNode*> terminals;
  // Collect ITerms
  for (const auto& [layer, shapes] : generatePolygonsFromITerms(terminals)) {
    shapes_by_layer[layer] += shapes;
  }

  // Collect BTerms
  for (const auto& [layer, shapes] : generatePolygonsFromBTerms(terminals)) {
    shapes_by_layer[layer] += shapes;
  }

  const TerminalTree terminal_nodes = getTerminalTree(terminals);

  // Simplify shapes
  std::vector<std::pair<odb::dbTechLayer*, Polygon90>> all_poly_shapes;
  for (auto& [layer, shapes] : shapes_by_layer) {
    const utl::DebugScopedTimer layer_timer(
        logger_,
        utl::PSM,
        "timer",
        1,
        fmt::format("Convert shapes to polygons shapes on {}: {{}}",
                    layer->getName()));

    std::vector<Polygon90> shape_polygons;
    shapes.get_polygons(shape_polygons);

    debugPrint(logger_,
               utl::PSM,
               "timer",
               1,
               "Shape reduction on {}: {}",
               layer->getName(),
               layer_timer);

    debugPrint(logger_,
               utl::PSM,
               "construct",
               1,
               "Shapes on {}: {} reduced to {}",
               layer->getName(),
               shapes.size(),
               shape_polygons.size());

    for (const auto& shape_poly : shape_polygons) {
      all_poly_shapes.emplace_back(layer, shape_poly);
    }
  }
  shapes_by_layer.clear();

  const utl::Timer generate_timer;
  std::vector<std::unique_ptr<Node>> poly_nodes;
  std::vector<std::unique_ptr<Shape>> poly_shapes;
  std::map<Shape*, std::set<Node*>> shape_term_nodes;
  for (const auto& [layer, shape_poly] : all_poly_shapes) {
    processPolygonToRectangles(layer,
                               shape_poly,
                               terminal_nodes,
                               poly_shapes,
                               poly_nodes,
                               shape_term_nodes);
  }

  debugPrint(
      logger_, utl::PSM, "timer", 1, "Shape generation: {}", generate_timer);

  for (auto& node : poly_nodes) {
    nodes_[node->getLayer()].push_back(std::move(node));
  }
  for (auto& shape : poly_shapes) {
    shapes_[shape->getLayer()].push_back(std::move(shape));
  }

  sortShapes();

  // Assign shape IDs
  std::size_t id = 0;
  for (const auto& [layer, shapes] : shapes_) {
    for (const auto& shape : shapes) {
      shape->setID(id++);
    }
  }
}

void IRNetwork::generateCutNodesForSBox(
    odb::dbSBox* box,
    bool single_via,
    std::vector<std::unique_ptr<Node>>& new_nodes,
    std::vector<std::unique_ptr<Connection>>& new_connections)
{
  // handle as via
  std::vector<odb::dbShape> via_shapes;
  box->getViaBoxes(via_shapes);
  via_shapes.erase(
      std::remove_if(via_shapes.begin(),
                     via_shapes.end(),
                     [](const auto& shape) {
                       return shape.getTechLayer()->getRoutingLevel() != 0;
                     }),
      via_shapes.end());

  odb::dbTechLayer* bottom = nullptr;
  odb::dbTechLayer* top = nullptr;

  odb::dbTechVia* tech_via = box->getTechVia();
  if (tech_via) {
    top = tech_via->getTopLayer();
    bottom = tech_via->getBottomLayer();
  } else {
    odb::dbVia* block_via = box->getBlockVia();
    top = block_via->getTopLayer();
    bottom = block_via->getBottomLayer();
  }

  const int min_pitch_
      = std::min(min_node_pitch_[bottom], min_node_pitch_[top]);
  const bool use_single_via = box->getBox().maxDXDY() < min_pitch_;

  if (single_via || use_single_via) {
    const odb::Point via_center = box->getViaXY();

    auto bottom_node = std::make_unique<Node>(via_center, bottom);
    auto top_node = std::make_unique<Node>(via_center, top);

    int cuts = 0;
    for (const auto& shape : via_shapes) {
      cuts += getEffectiveNumberOfCuts(shape);
    }
    new_connections.push_back(std::make_unique<ViaConnection>(
        bottom_node.get(), top_node.get(), cuts));

    new_nodes.push_back(std::move(top_node));
    new_nodes.push_back(std::move(bottom_node));
  } else {
    for (const auto& shape : via_shapes) {
      const odb::Rect via = shape.getBox();

      auto bottom_node = std::make_unique<Node>(via.center(), bottom);
      auto top_node = std::make_unique<Node>(via.center(), top);

      new_connections.push_back(std::make_unique<ViaConnection>(
          bottom_node.get(), top_node.get(), getEffectiveNumberOfCuts(shape)));

      new_nodes.push_back(std::move(top_node));
      new_nodes.push_back(std::move(bottom_node));
    }
  }
}

IRNetwork::ShapeTree IRNetwork::getShapeTree(odb::dbTechLayer* layer) const
{
  // generate Rtree of shapes to associate with vias
  std::vector<Shape*> tree_values;
  for (const auto& shape : shapes_.at(layer)) {
    tree_values.emplace_back(shape.get());
  }
  ShapeTree tree(tree_values.begin(), tree_values.end());
  return tree;
}

IRNetwork::NodeTree IRNetwork::getNodeTree(odb::dbTechLayer* layer) const
{
  // generate Rtree of shapes to associate with vias
  std::vector<Node*> tree_values;
  for (const auto& node : nodes_.at(layer)) {
    tree_values.emplace_back(node.get());
  }
  NodeTree tree(tree_values.begin(), tree_values.end());
  return tree;
}

void IRNetwork::generateCutLayerNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate cut nodes and connections: {}");

  const bool use_single_via = logger_->debugCheck(utl::PSM, "single_via", 1);

  // Collect vias boxes
  std::vector<odb::dbSBox*> boxes;
  for (odb::dbSWire* wire : net_->getSWires()) {
    for (odb::dbSBox* box : wire->getWires()) {
      if (box->isVia()) {
        boxes.push_back(box);
      }
    }
  }

  std::vector<std::unique_ptr<Node>> loop_via_nodes;
  std::vector<std::unique_ptr<Connection>> loop_via_connections;
  for (odb::dbSBox* box : boxes) {
    generateCutNodesForSBox(
        box, use_single_via, loop_via_nodes, loop_via_connections);
  }
  boxes.clear();

  LayerMap<std::vector<std::unique_ptr<Node>>> via_nodes;
  for (auto& node : loop_via_nodes) {
    via_nodes[node->getLayer()].push_back(std::move(node));
  }
  for (auto& connection : loop_via_connections) {
    connections_.push_back(std::move(connection));
  }
  loop_via_nodes.clear();
  loop_via_connections.clear();

  for (auto& [layer, nodes] : via_nodes) {
    // move vias to nodes_
    auto& global_nodes = nodes_[layer];
    for (auto& node : nodes) {
      global_nodes.push_back(std::move(node));
    }
  }
}

void IRNetwork::generateTopLayerFillerNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate top layer filler nodes: {}");
  // needed in case of vsrc
  odb::dbTechLayer* top = getTopLayer();
  const auto top_nodes = getNodeTree(top);

  const int max_distance = min_node_pitch_[top];

  for (const auto& shape : shapes_[top]) {
    for (auto& node : shape->createFillerNodes(max_distance, top_nodes)) {
      nodes_[node->getLayer()].push_back(std::move(node));
    }
  }
}

std::set<Node*> IRNetwork::getSharedShapeNodes() const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Build node -> shape count: {}");

  std::set<Node*> shared_nodes;
  for (const auto& [layer, nodes] : nodes_) {
    const auto layer_shapes = getShapeTree(layer);

    for (const auto& node : nodes) {
      const Point pt(node->getPoint().x(), node->getPoint().y());
      const auto shapes = std::distance(
          layer_shapes.qbegin(boost::geometry::index::intersects(pt)),
          layer_shapes.qend());
      if (shapes > 1) {
        shared_nodes.insert(node.get());
      }
    }
  }

  return shared_nodes;
}

void IRNetwork::mergeNodes(NodePtrMap<Connection>& connection_map)
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Merging nodes: {}");

  const auto shared_nodes = getSharedShapeNodes();

  const utl::Timer perform_timer;
  std::set<Node*> removes;
  for (auto& [layer, shapes] : shapes_) {
    debugPrint(logger_,
               utl::PSM,
               "timer",
               1,
               "Total shapes to check for merging on {}: {}",
               layer->getName(),
               shapes.size());

    const auto node_trees = getNodeTree(layer);
    for (const auto& shape : shapes) {
      const int min_distance = min_node_pitch_[shape->getLayer()];
      const auto shape_remove = shape->cleanupNodes(
          min_distance,
          node_trees,
          [&](Node* keep, Node* remove) { copy(keep, remove, connection_map); },
          shared_nodes);
      removes.insert(shape_remove.begin(), shape_remove.end());
    }
  }

  debugPrint(
      logger_, utl::PSM, "timer", 1, "Perform merges: {}", perform_timer);

  LayerMap<std::set<Node*>> remove_by_layer;
  for (auto* node : removes) {
    remove_by_layer[node->getLayer()].insert(node);
  }
  removes.clear();

  std::vector<std::pair<odb::dbTechLayer*, std::set<Node*>>> remove_sets;
  for (auto& [layer, layer_remove] : remove_by_layer) {
    remove_sets.emplace_back(layer, std::move(layer_remove));
  }
  remove_by_layer.clear();

  for (auto& [layer, layer_remove] : remove_sets) {
    debugPrint(logger_,
               utl::PSM,
               "construct",
               2,
               "Identified {} nodes to be merged on {}",
               layer_remove.size(),
               layer->getName());
    removeNodes(layer_remove, layer, nodes_[layer], connection_map);
  }

  recoverMemory();
}

void IRNetwork::sortShapes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Sorting shapes: {}");

  for (auto& [layer, shapes] : shapes_) {
    shapes.shrink_to_fit();

    std::stable_sort(
        shapes.begin(), shapes.end(), [](const auto& lhs, const auto& rhs) {
          return lhs->getShape() < rhs->getShape();
        });
  }
}

void IRNetwork::sortNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Sorting nodes: {}");

  for (auto& [layer, nodes] : nodes_) {
    std::stable_sort(
        nodes.begin(), nodes.end(), [](const auto& lhs, const auto& rhs) {
          return lhs->compare(rhs);
        });
  }
}

void IRNetwork::sortConnections()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Sorting connections: {}");
  std::stable_sort(
      connections_.begin(),
      connections_.end(),
      [](const auto& lhs, const auto& rhs) { return lhs->compare(rhs); });
}

int IRNetwork::getEffectiveNumberOfCuts(const odb::dbShape& shape) const
{
  auto* layer = shape.getTechLayer();

  const odb::Rect& via = shape.getBox();

  for (auto* cut_class : layer->getTechLayerCutClassRules()) {
    if (!cut_class->isCutsValid()) {
      continue;
    }
    const int width = cut_class->getWidth();
    const int length = cut_class->getLength();

    const bool valid_length = cut_class->isLengthValid();

    if (via.dx() == width
        && ((valid_length && via.dy() == length) || !valid_length)) {
      return cut_class->getNumCuts();
    }
    if (via.dy() == width
        && ((valid_length && via.dx() == length) || !valid_length)) {
      return cut_class->getNumCuts();
    }
  }

  return 1;
}

IRNetwork::NodePtrMap<Connection> IRNetwork::getConnectionMap() const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Build node -> connection mapping: {}");

  NodePtrMap<Connection> mapping;

  for (const auto& conn : connections_) {
    mapping[conn->getNode0()].push_back(conn.get());
    mapping[conn->getNode1()].push_back(conn.get());
  }

  for (auto& [node, conns] : mapping) {
    conns.shrink_to_fit();
  }

  return mapping;
}

void IRNetwork::cleanupOverlappingNodes(NodePtrMap<Connection>& connection_map)
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Cleanup overlapping nodes: {}");

  for (auto& [layer, nodes] : nodes_) {
    std::set<Node*> removes;

    // remove duplicate/overlapping nodes
    auto node = nodes.begin();
    Node* prev_node = node->get();
    for (std::advance(node, 1); node != nodes.end();) {
      const odb::Point& pt = (*node)->getPoint();
      if (pt == prev_node->getPoint()) {
        copy(prev_node, node->get(), connection_map);
        removes.insert(node->get());
      } else {
        prev_node = node->get();
      }
      node++;
    }

    debugPrint(logger_,
               utl::PSM,
               "construct",
               2,
               "Identified overlapping nodes on {}: {}",
               layer->getName(),
               removes.size());
    removeNodes(removes, layer, nodes, connection_map);
  }
}

void IRNetwork::cleanupNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Cleanup nodes: {}");

  auto node_connection_map = getConnectionMap();

  std::map<Node*, bool> marked_deleted;
  for (const auto& [layer, nodes] : nodes_) {
    for (const auto& node : nodes) {
      marked_deleted[node.get()] = false;
    }
  }

  cleanupOverlappingNodes(node_connection_map);

  mergeNodes(node_connection_map);

  recoverMemory();
}

void IRNetwork::removeNodes(std::set<Node*>& removes,
                            odb::dbTechLayer* layer,
                            std::vector<std::unique_ptr<Node>>& nodes,
                            const NodePtrMap<Connection>& connection_map)
{
  if (removes.empty()) {
    return;
  }

  const utl::DebugScopedTimer timer(
      logger_,
      utl::PSM,
      "timer",
      1,
      fmt::format("Remove nodes on {}: {{}}", layer->getName()));

  const std::size_t start_node_size = nodes.size();

  // remove connections
  for (auto* node : removes) {
    auto find_conn = connection_map.find(node);
    if (find_conn == connection_map.end()) {
      continue;
    }

    for (auto* conn : find_conn->second) {
      conn->changeNode(node, nullptr);
    }
  }

  // use list to speed up erase
  std::list<std::unique_ptr<Node>> cleanup;
  for (auto& node : nodes) {
    cleanup.push_back(std::move(node));
  }
  nodes.clear();

  cleanup.erase(std::remove_if(cleanup.begin(),
                               cleanup.end(),
                               [&](const auto& other) {
                                 return removes.find(other.get())
                                        != removes.end();
                               }),
                cleanup.end());

  for (auto& node : cleanup) {
    nodes.emplace_back(std::move(node));
  }

  const std::size_t final_node_size = nodes.size();

  removes.clear();

  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Cleanup of nodes on {}: {} -> {} ({} removed)",
             layer->getName(),
             start_node_size,
             final_node_size,
             start_node_size - final_node_size);
}

void IRNetwork::removeConnections(std::set<Connection*>& removes)
{
  if (removes.empty()) {
    return;
  }

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Remove connections: {}");

  const std::size_t start_connection_size = connections_.size();

  // use list to speed up erase
  std::list<std::unique_ptr<Connection>> cleanup;
  for (auto& conn : connections_) {
    cleanup.push_back(std::move(conn));
  }
  connections_.clear();

  cleanup.erase(std::remove_if(cleanup.begin(),
                               cleanup.end(),
                               [&](const auto& other) {
                                 return removes.find(other.get())
                                        != removes.end();
                               }),
                cleanup.end());

  for (auto& conn : cleanup) {
    conn->ensureNodeOrder();
    connections_.emplace_back(std::move(conn));
  }

  removes.clear();

  const std::size_t final_connection_size = connections_.size();

  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Cleanup of connections: {} -> {} ({} removed)",
             start_connection_size,
             final_connection_size,
             start_connection_size - final_connection_size);
}

void IRNetwork::cleanupInvalidConnections()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Cleanup invalid connections: {}");

  std::set<Connection*> removes;
  for (const auto& conn : connections_) {
    if (conn->isStub() || conn->isLoop() || !conn->isValid()) {
      removes.insert(conn.get());
    }
  }
  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Identified invalid connections: {}",
             removes.size());
  removeConnections(removes);
}

void IRNetwork::cleanupDuplicateConnections()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Cleanup duplicate connections: {}");

  using ConnectionSet = std::set<Connection*>;
  ConnectionSet removes;

  // remove duplicates
  std::map<std::pair<Node*, Node*>, ConnectionSet> check_duplicates;
  for (const auto& conn : connections_) {
    check_duplicates[{conn->getNode0(), conn->getNode1()}].insert(conn.get());
  }

  for (const auto& [nodes, connections] : check_duplicates) {
    if (connections.size() > 1) {
      Connection* keep = *connections.begin();

      auto remove_conn = connections.begin();
      std::advance(remove_conn, 1);
      for (; remove_conn != connections.end(); remove_conn++) {
        keep->mergeWith(*remove_conn);
        removes.insert(*remove_conn);
      }
    }
  }

  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Identified duplicate connections: {}",
             removes.size());
  removeConnections(removes);
}

void IRNetwork::cleanupConnections()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Cleanup connections: {}");

  cleanupInvalidConnections();
  cleanupDuplicateConnections();

  recoverMemory();
}

void IRNetwork::connectLayerNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Connect nodes: {}");

  const std::size_t start_connections = connections_.size();

  for (const auto& [layer, layer_shapes] : shapes_) {
    const auto layer_nodes = getNodeTree(layer);
    for (const auto& shape : layer_shapes) {
      for (auto& conn : shape->connectNodes(layer_nodes)) {
        connections_.push_back(std::move(conn));
      }
    }
  }

  const std::size_t end_connections = connections_.size();

  debugPrint(logger_,
             utl::PSM,
             "construct",
             2,
             "Created {} connections",
             end_connections - start_connections);
}

odb::dbTechLayer* IRNetwork::getTopLayer() const
{
  return nodes_.rbegin()->first;
}

const std::vector<std::unique_ptr<Node>>& IRNetwork::getTopLayerNodes() const
{
  return nodes_.rbegin()->second;
}

IRNetwork::NodeTree IRNetwork::getTopLayerNodeTree() const
{
  return getNodeTree(getTopLayer());
}

std::map<odb::dbInst*, Node::NodeSet> IRNetwork::getInstanceNodeMapping() const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate instance node map: {}");

  std::map<odb::dbInst*, Node::NodeSet> inst_nodes;
  for (const auto& node : iterm_nodes_) {
    odb::dbITerm* iterm = node->getITerm();
    odb::dbInst* inst = iterm->getInst();
    inst_nodes[inst].insert(node.get());
  }

  return inst_nodes;
}

void IRNetwork::reportStats() const
{
  std::size_t count = 0;
  for (const auto& [layer, shape] : shapes_) {
    count += shape.size();
    logger_->report("Shapes on {}: {}", layer->getName(), shape.size());
  }
  logger_->report("Total shapes: {}", count);
  count = 0;
  for (const auto& [layer, nodes] : nodes_) {
    count += nodes.size();
    logger_->report("Nodes on {}: {}", layer->getName(), nodes.size());
  }
  logger_->report("Total nodes: {}", count);
  logger_->report("Total connections: {}", connections_.size());
}

void IRNetwork::copy(Node* keep,
                     Node* remove,
                     NodePtrMap<Connection>& connection_map)
{
  // change connections
  auto& remove_connections = connection_map[remove];
  for (auto* conn : remove_connections) {
    conn->changeNode(remove, keep);
  }
  // update connection map
  auto& keep_connections = connection_map[keep];
  keep_connections.insert(keep_connections.end(),
                          remove_connections.begin(),
                          remove_connections.end());
  remove_connections.clear();
  remove_connections.shrink_to_fit();
}

void IRNetwork::dumpNodes(const std::map<Node*, std::size_t>& node_map,
                          const std::string& name) const
{
  const std::string report_file = fmt::format("psm_{}.txt", name);
  std::ofstream report(report_file);
  if (!report) {
    logger_->report("Failed to open {} for nodes", report_file);
    return;
  }

  std::map<std::size_t, Node*> nodeidx_map;
  for (const auto& [node, idx] : node_map) {
    nodeidx_map[idx] = node;
  }

  for (const auto& [idx, node] : nodeidx_map) {
    report << std::to_string(idx) << ": " << node->describe("") << '\n';
  }
}

void IRNetwork::dumpNodes(const std::string& name) const
{
  std::map<Node*, std::size_t> node_idx;
  std::size_t idx = 0;
  for (const auto& [layer, layer_nodes] : nodes_) {
    for (const auto& node : layer_nodes) {
      node_idx[node.get()] = idx++;
    }
  }

  dumpNodes(node_idx, name);
}

bool IRNetwork::belongsTo(Node* node) const
{
  for (const auto& [layer, nodes] : nodes_) {
    if (std::find_if(nodes.begin(),
                     nodes.end(),
                     [node](const auto& other) { return other.get() == node; })
        != nodes.end()) {
      return true;
    }
  }

  if (std::find_if(iterm_nodes_.begin(),
                   iterm_nodes_.end(),
                   [node](const auto& other) { return other.get() == node; })
      != iterm_nodes_.end()) {
    return true;
  }

  if (std::find_if(bpin_nodes_.begin(),
                   bpin_nodes_.end(),
                   [node](const auto& other) { return other.get() == node; })
      != bpin_nodes_.end()) {
    return true;
  }

  return false;
}

bool IRNetwork::belongsTo(Connection* connection) const
{
  return std::find_if(connections_.begin(),
                      connections_.end(),
                      [connection](const auto& other) {
                        return other.get() == connection;
                      })
         != connections_.end();
}

std::size_t IRNetwork::getNodeCount(bool include_iterms) const
{
  std::size_t count = 0;
  for (const auto& [layer, nodes] : nodes_) {
    count += nodes.size();
  }

  if (include_iterms) {
    count += iterm_nodes_.size();
  }

  return count;
}

Node::NodeSet IRNetwork::getBPinShapeNodes() const
{
  if (bpin_nodes_.empty()) {
    return {};
  }

  std::map<odb::dbTechLayer*, std::set<odb::Rect>> nodes;
  for (const auto& bpin : bpin_nodes_) {
    nodes[bpin->getLayer()].insert(bpin->getShape());
  }

  Node::NodeSet pin_nodes;
  for (const auto& [layer, shapes] : nodes) {
    const auto node_tree = getNodeTree(layer);

    for (const auto& shape : shapes) {
      for (auto itr
           = node_tree.qbegin(boost::geometry::index::intersects(shape));
           itr != node_tree.qend();
           itr++) {
        pin_nodes.insert(*itr);
      }
    }
  }

  return pin_nodes;
}

}  // namespace psm
