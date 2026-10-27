// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ir_network.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "connection.h"
#include "node.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "shape.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace psm {

namespace {

// The middle of the edges two trapezoids share. They are slices of one
// polygon cut vertically, so they can only share a vertical edge.
std::vector<odb::Point> getSharedEdgeCenters(
    const std::vector<odb::Point>& points0,
    const std::vector<odb::Point>& points1)
{
  std::vector<odb::Point> centers;
  // the last point closes each polygon, so all but it start an edge
  for (std::size_t i = 0; i + 1 < points0.size(); i++) {
    const odb::Point& start0 = points0[i];
    const odb::Point& end0 = points0[i + 1];
    for (std::size_t j = 0; j + 1 < points1.size(); j++) {
      const odb::Point& start1 = points1[j];
      const odb::Point& end1 = points1[j + 1];
      if (start0.x() != end0.x() || start1.x() != end1.x()
          || start0.x() != start1.x()) {
        continue;
      }
      const int low = std::max(std::min(start0.y(), end0.y()),
                               std::min(start1.y(), end1.y()));
      const int high = std::min(std::max(start0.y(), end0.y()),
                                std::max(start1.y(), end1.y()));
      if (low < high) {
        centers.emplace_back(start0.x(), (low + high) / 2);
      }
    }
  }
  return centers;
}

}  // namespace

IRNetwork::IRNetwork(odb::dbNet* net, utl::Logger* logger, bool floorplanning)
    : net_(net), logger_(logger), floorplanning_(floorplanning)
{
  if (!net_->getSigType().isSupply()) {
    logger_->error(utl::PSM, 87, "{} is not a supply net.", net_->getName());
  }
  initMinimumNodePitch();
  construct();
}

IRNetwork::IRNetwork(odb::dbChipNet* chip_net,
                     utl::Logger* logger,
                     bool floorplanning,
                     std::optional<double> interchip_resistance,
                     bool connectivity_only)
    : net_(nullptr),
      chip_net_(chip_net),
      logger_(logger),
      floorplanning_(floorplanning),
      interchip_resistance_(interchip_resistance),
      connectivity_only_(connectivity_only)
{
  construct();
}

IRNetwork::~IRNetwork() = default;

void IRNetwork::initMinimumNodePitch()
{
  min_node_pitch_.clear();

  const double dbus = getBlock()->getDbUnitsPerMicron();
  const int min_pitch = dbus * kMinNodePitchInUm;

  for (auto* layer : getTech()->getLayers()) {
    if (layer->getRoutingLevel() != 0) {
      min_node_pitch_[layer] = std::min(
          min_pitch, kMinNodePitchMultiplier * std::max(1, layer->getPitch()));
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

bool IRNetwork::hasNodes() const
{
  if (isAssembly()) {
    return std::ranges::any_of(chiplet_networks_, [](const auto& chiplet) {
      return chiplet.network->hasNodes();
    });
  }

  for (const auto& [layer, nodes] : nodes_) {
    if (!nodes.empty()) {
      return true;
    }
  }
  return !iterm_nodes_.empty() || !bpin_nodes_.empty();
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
  if (isAssembly()) {
    constructAssembly();
    return;
  }

  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Construct: {}");

  reset();

  generateRoutingLayerShapesAndNodes();
  generateCutLayerNodes();

  if (!hasNodes()) {
    logger_->warn(utl::PSM, 86, "Net {} is empty.", net_->getName());
    return;
  }

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

IRNetwork::ChipletNetwork* IRNetwork::findChipletNetwork(
    odb::dbChipBumpInst* bump_inst)
{
  if (bump_inst == nullptr) {
    logger_->error(utl::PSM,
                   97,
                   "Inter-chip bond on net {} has no bump.",
                   chip_net_->getName());
  }

  odb::dbChipInst* chip_inst = bump_inst->getChipRegionInst()->getChipInst();
  odb::dbNet* net = bump_inst->getChipBump()->getNet();
  const auto network = std::ranges::find_if(
      chiplet_networks_, [chip_inst, net](const ChipletNetwork& candidate) {
        return candidate.chip_inst == chip_inst && candidate.net == net;
      });
  if (network == chiplet_networks_.end()) {
    logger_->error(utl::PSM,
                   98,
                   "Cannot find the power grid for net {} on chiplet {}.",
                   net->getName(),
                   chip_inst->getName());
  }
  return &*network;
}

Node* IRNetwork::findBondEndpoint(odb::dbChipCapNode* cap_node)
{
  if (cap_node == nullptr) {
    logger_->error(utl::PSM,
                   170,
                   "Inter-chip bond on net {} is missing one of its bumps.",
                   chip_net_->getName());
  }
  return findBondEndpoint(cap_node->getChipBumpInst());
}

Node* IRNetwork::findBondEndpoint(odb::dbChipBumpInst* bump_inst)
{
  ChipletNetwork* chiplet = findChipletNetwork(bump_inst);
  odb::dbInst* bump = bump_inst->getChipBump()->getInst();
  Node* node = nullptr;
  for (const auto& candidate : chiplet->network->getITermNodes()) {
    if (candidate->getITerm()->getInst() == bump) {
      if (node != nullptr) {
        logger_->error(utl::PSM,
                       141,
                       "Bump {} has more than one terminal on net {}.",
                       bump->getName(),
                       chiplet->net->getName());
      }
      node = candidate.get();
    }
  }
  if (node == nullptr) {
    logger_->error(utl::PSM,
                   100,
                   "Cannot map bump {} on chiplet {} into its power grid.",
                   bump->getName(),
                   chiplet->chip_inst->getName());
  }
  return node;
}

void IRNetwork::addChipletNetwork(odb::dbChipBumpInst* bump_inst,
                                  odb::dbChipInst* chip_inst)
{
  odb::dbNet* net = bump_inst->getChipBump()->getNet();
  if (net == nullptr) {
    logger_->error(utl::PSM,
                   95,
                   "A bump on chiplet {} is not connected to a net.",
                   chip_inst->getName());
  }

  const auto network = std::ranges::find_if(
      chiplet_networks_, [chip_inst, net](const ChipletNetwork& candidate) {
        return candidate.chip_inst == chip_inst && candidate.net == net;
      });
  if (network == chiplet_networks_.end()) {
    chiplet_networks_.push_back(
        {chip_inst,
         net,
         std::make_unique<IRNetwork>(net, logger_, floorplanning_)});
  }
}

void IRNetwork::addInterDieConnection(odb::dbChipRSeg* rseg)
{
  Node* source = findBondEndpoint(rseg->getSourceCapNode());
  Node* target = findBondEndpoint(rseg->getTargetCapNode());
  odb::dbChipBumpInst* source_bump
      = rseg->getSourceCapNode()->getChipBumpInst();
  odb::dbChipBumpInst* target_bump
      = rseg->getTargetCapNode()->getChipBumpInst();
  odb::dbChipInst* source_chip
      = source_bump->getChipRegionInst()->getChipInst();
  odb::dbChipInst* target_chip
      = target_bump->getChipRegionInst()->getChipInst();
  if (source_chip == target_chip) {
    odb::Point source_position
        = source_bump->getChipBump()->getInst()->getBBox()->getBox().center();
    odb::Point target_position
        = target_bump->getChipBump()->getInst()->getBBox()->getBox().center();
    source_chip->getTransform().apply(source_position);
    target_chip->getTransform().apply(target_position);
    const double dbus = chip_net_->getDb()->getDbuPerMicron();
    logger_->error(
        utl::PSM,
        106,
        "Bumps at ({:.4f}, {:.4f}) and ({:.4f}, {:.4f}) um on chiplet {} "
        "are bonded to each other on net {}. An inter-chip bond must connect "
        "different chiplets.",
        source_position.x() / dbus,
        source_position.y() / dbus,
        target_position.x() / dbus,
        target_position.y() / dbus,
        source_chip->getName(),
        chip_net_->getName());
  }

  odb::dbNet* source_net = source_bump->getChipBump()->getNet();
  odb::dbNet* target_net = target_bump->getChipBump()->getNet();
  if (source_net->getSigType() != target_net->getSigType()) {
    logger_->error(utl::PSM,
                   166,
                   "Inter-chip bond on {} shorts {}/{} ({}) to {}/{} ({}).",
                   chip_net_->getName(),
                   source_chip->getName(),
                   source_net->getName(),
                   source_net->getSigType().getString(),
                   target_chip->getName(),
                   target_net->getName(),
                   target_net->getSigType().getString());
  }
  if (connectivity_only_) {
    interdie_connections_.push_back(
        std::make_unique<TermConnection>(source, target));
  } else {
    const double resistance
        = interchip_resistance_.value_or(rseg->getResistance());
    if (!std::isfinite(resistance) || resistance <= 0.0) {
      logger_->error(utl::PSM,
                     96,
                     "Inter-chip bond on net {} must have positive and finite "
                     "resistance: {}.",
                     chip_net_->getName(),
                     resistance);
    }
    interdie_connections_.push_back(std::make_unique<FixedResistanceConnection>(
        source, target, resistance));
  }
}

// OpenRCX integration is deferred until its extractor supports multiple bump
// pairs on one assembly net. Until then, PSM creates one ODB resistor for each
// pair of aligned bumps using the custom resistance API with user-supplied
// resistance. Connectivity checks add only temporary graph edges.
void IRNetwork::createMatchedInterDieConnections(
    std::optional<double> resistance)
{
  std::unordered_set<odb::dbChipBumpInst*> net_bumps;
  for (uint32_t index = 0; index < chip_net_->getNumBumpInsts(); index++) {
    std::vector<odb::dbChipInst*> path;
    net_bumps.insert(chip_net_->getBumpInst(index, path));
  }

  for (odb::dbUnfoldedChipConn* connection :
       chip_net_->getDb()->getUnfoldedChipConns()) {
    odb::dbUnfoldedChipRegionInst* top = connection->getTopRegion();
    odb::dbUnfoldedChipRegionInst* bottom = connection->getBottomRegion();
    if (top == nullptr || bottom == nullptr) {
      continue;
    }

    std::map<odb::Point, odb::dbChipBumpInst*> bottom_bumps;
    for (odb::dbUnfoldedChipBumpInst* bump : bottom->getBumps()) {
      odb::dbChipBumpInst* bump_inst = bump->getChipBumpInst();
      if (net_bumps.contains(bump_inst)) {
        const odb::Point3D position = bump->getGlobalPosition();
        bottom_bumps[{position.x(), position.y()}] = bump_inst;
      }
    }

    for (odb::dbUnfoldedChipBumpInst* bump : top->getBumps()) {
      odb::dbChipBumpInst* bump_inst = bump->getChipBumpInst();
      if (!net_bumps.contains(bump_inst)) {
        continue;
      }
      const odb::Point3D position = bump->getGlobalPosition();
      const auto match = bottom_bumps.find({position.x(), position.y()});
      if (match == bottom_bumps.end()) {
        continue;
      }

      if (resistance) {
        odb::dbChipCapNode* source = odb::dbChipCapNode::create(chip_net_);
        source->setChipBumpInst(match->second);
        odb::dbChipCapNode* target = odb::dbChipCapNode::create(chip_net_);
        target->setChipBumpInst(bump_inst);
        odb::dbChipRSeg* resistor
            = odb::dbChipRSeg::create(chip_net_, source, target);
        resistor->setResistance(*resistance);
      } else {
        interdie_connections_.push_back(std::make_unique<TermConnection>(
            findBondEndpoint(match->second), findBondEndpoint(bump_inst)));
      }
    }
  }
}

void IRNetwork::constructAssembly()
{
  interdie_connections_.clear();
  chiplet_networks_.clear();

  for (uint32_t index = 0; index < chip_net_->getNumBumpInsts(); index++) {
    std::vector<odb::dbChipInst*> path;
    odb::dbChipBumpInst* bump_inst = chip_net_->getBumpInst(index, path);
    if (bump_inst == nullptr || path.size() != 1) {
      logger_->error(
          utl::PSM,
          94,
          "Net {} requires a flat assembly where each bump maps to one "
          "chiplet.",
          chip_net_->getName());
    }
    addChipletNetwork(bump_inst, path.back());
  }

  if (chip_net_->getChipRSegs().empty()
      && (interchip_resistance_ || connectivity_only_)) {
    createMatchedInterDieConnections(interchip_resistance_);
  }
  for (odb::dbChipRSeg* rseg : chip_net_->getChipRSegs()) {
    addInterDieConnection(rseg);
  }
  bond_state_ = getBondState();
}

std::vector<IRNetwork::BondState> IRNetwork::getBondState() const
{
  std::vector<BondState> state;
  for (odb::dbChipRSeg* bond : chip_net_->getChipRSegs()) {
    odb::dbChipCapNode* source = bond->getSourceCapNode();
    odb::dbChipCapNode* target = bond->getTargetCapNode();
    odb::dbChipBumpInst* source_bump = source->getChipBumpInst();
    odb::dbChipBumpInst* target_bump = target->getChipBumpInst();
    state.push_back({bond->getId(),
                     source->getId(),
                     target->getId(),
                     source_bump == nullptr ? 0 : source_bump->getId(),
                     target_bump == nullptr ? 0 : target_bump->getId(),
                     bond->getResistance()});
  }
  std::ranges::sort(state, {}, &BondState::id);
  return state;
}

bool IRNetwork::hasSameBonds() const
{
  return !isAssembly() || getBondState() == bond_state_;
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

IRNetwork::LayerMap<odb::geom::BoostPolygon90Set>
IRNetwork::generatePolygonsFromBox(odb::dbBox* box,
                                   const odb::dbTransform& transform) const
{
  LayerMap<odb::geom::BoostPolygon90Set> shapes_by_layer;

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
        odb::geom::BoostPolygon90Set& shapes = shapes_by_layer[layer];

        odb::Rect rect = shape.getBox();
        transform.apply(rect);

        shapes.insert(odb::geom::toPolygon90(rect));
      }
    }
  } else {
    // handle as shape
    auto* layer = box->getTechLayer();

    if (layer->getRoutingLevel() != 0) {
      // Only collect shapes on routing layers
      odb::Rect rect = box->getBox();
      transform.apply(rect);
      shapes_by_layer[layer].insert(odb::geom::toPolygon90(rect));
    }
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<odb::geom::BoostPolygon90Set>
IRNetwork::generatePolygonsFromSWire(odb::dbSWire* wire,
                                     LayerMap<Polygon45Set>& shapes_45)
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from SWire: {}");

  LayerMap<odb::geom::BoostPolygon90Set> shapes_by_layer;

  for (odb::dbSBox* box : wire->getWires()) {
    if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
      // a 45 degree wire, its box would be all of its bounding box
      odb::dbTechLayer* layer = box->getTechLayer();
      if (layer->getRoutingLevel() != 0) {
        const std::vector<odb::Point> points = box->getOct().getPoints();
        shapes_45[layer].insert(Polygon45(points.begin(), points.end()));
      }
      continue;
    }
    for (const auto& [layer, polygon] :
         generatePolygonsFromBox(box, odb::dbTransform())) {
      shapes_by_layer[layer].insert(polygon);
    }
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<odb::geom::BoostPolygon90Set>
IRNetwork::generatePolygonsFromITerms(std::vector<TerminalNode*>& terminals,
                                      LayerMap<Polygon45Set>& shapes_45)
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from ITerms: {}");

  odb::dbTechLayer* front_base = getTech()->firstFrontsideRoutingLayer();
  odb::dbTechLayer* back_base = getTech()->firstBacksideRoutingLayer();

  LayerMap<odb::geom::BoostPolygon90Set> shapes_by_layer;
  bool floorplan_asseted = false;
  for (auto* iterm : net_->getITerms()) {
    auto* inst = iterm->getInst();
    if (!inst->isPlaced()) {
      continue;
    }
    if (floorplanning_ && !inst->getPlacementStatus().isFixed()) {
      floorplan_asseted = true;
      continue;
    }
    const auto transform = inst->getTransform();
    if (iterm->getBBox().isInverted()) {
      // iterm has no physical shape, so ignore.
      continue;
    }

    // First pass: figure out which side(s) this iterm has pin geometry on
    // so we can pick the iterm base node's layer accordingly. Only the
    // layer matters here, so skip the polygon/transform work.
    bool has_front_pin = false;
    bool has_back_pin = false;
    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        odb::dbTechLayer* layer = geom->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        if (layer->isBackside()) {
          has_back_pin = true;
        } else {
          has_front_pin = true;
        }
      }
    }

    // Place the iterm base node on the cell's "primary" side. A
    // backside-only cell's base belongs on the backside grid so its
    // pin connects to real shapes; otherwise default to the front side.
    odb::dbTechLayer* base_layer = front_base;
    if (!has_front_pin && has_back_pin && back_base != nullptr) {
      base_layer = back_base;
    }
    const bool primary_is_back = base_layer == back_base;
    const bool is_bridge = inst->getMaster()->isBacksideBridge();

    int x, y;
    iterm->getAvgXY(&x, &y);
    auto base_node
        = std::make_unique<ITermNode>(iterm, odb::Point(x, y), base_layer);

    auto link_terminal = [&](Node* term, odb::dbTechLayer* term_layer) {
      const bool term_is_back = term_layer->isBackside();
      if (term_is_back == primary_is_back) {
        // Same-side terminal: ordinary virtual edge into the base node.
        connections_.push_back(
            std::make_unique<TermConnection>(base_node.get(), term));
      } else if (is_bridge) {
        // Bridge cell: explicit virtual edge across the front/back boundary.
        connections_.push_back(
            std::make_unique<BridgeConnection>(base_node.get(), term));
      }
      // Non-bridge cross-side terminals are intentionally left
      // unlinked from the iterm base node. They still join the
      // local layer graph through cleanupOverlappingNodes(); if
      // they end up unreachable, that surfaces as an open net,
      // which is the correct outcome for a malformed PG.
    };

    bool has_routing_term = false;
    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      // odb holds each pin polygon as boxes too. Those are exact for a
      // rectilinear polygon, but for one with 45 degree edges they reach
      // outside of it, so that polygon is added itself instead.
      odb::PtrSet<odb::dbBox> polygon_boxes;
      for (odb::dbPolygon* pin : mpin->getPolygonGeometry()) {
        odb::dbTechLayer* layer = pin->getTechLayer();
        odb::Polygon polygon = pin->getPolygon();
        std::vector<odb::Point> points = polygon.getPoints();
        const bool rectilinear = std::ranges::adjacent_find(
                                     points,
                                     [](const auto& a, const auto& b) {
                                       return a.x() != b.x() && a.y() != b.y();
                                     })
                                 == points.end();
        if (layer->getRoutingLevel() == 0 || rectilinear) {
          continue;
        }
        for (odb::dbBox* geom : pin->getGeometry()) {
          polygon_boxes.insert(geom);
        }

        transform.apply(polygon);
        points = polygon.getPoints();
        Polygon45Set pin_set;
        pin_set.insert(Polygon45(points.begin(), points.end()));
        shapes_45[layer].insert(pin_set);

        // create iterm nodes, one on each trapezoid of the polygon
        std::vector<Polygon45> pieces;
        pin_set.get_trapezoids(pieces);
        for (const Polygon45& piece : pieces) {
          has_routing_term = true;

          // a trapezoid of a vertical slicing holds the center of its box
          auto center = std::make_unique<TerminalNode>(
              odb::geom::getEnclosingRect(piece), layer);
          terminals.push_back(center.get());

          link_terminal(center.get(), layer);

          nodes_[layer].push_back(std::move(center));
        }
      }

      for (auto* geom : mpin->getGeometry()) {
        if (polygon_boxes.contains(geom)) {
          continue;
        }
        const auto pin_shapes = generatePolygonsFromBox(geom, transform);
        if (pin_shapes.empty()) {
          continue;
        }

        for (const auto& [layer, shapes] : pin_shapes) {
          shapes_by_layer[layer].insert(shapes);
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

              link_terminal(center.get(), layer);

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

          link_terminal(center.get(), layer);

          nodes_[layer].push_back(std::move(center));
        }
      }
    }

    if (has_routing_term) {
      iterm_nodes_.push_back(std::move(base_node));
    }
  }

  if (floorplanning_ && !floorplan_asseted) {
    // Since floorplanning did not impact solution, mark this okay for analysis
    floorplanning_ = false;
  }

  return shapes_by_layer;
}

IRNetwork::LayerMap<odb::geom::BoostPolygon90Set>
IRNetwork::generatePolygonsFromBTerms(std::vector<TerminalNode*>& terminals)
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes from BTerms: {}");

  LayerMap<odb::geom::BoostPolygon90Set> shapes_by_layer;

  for (auto* bterm : net_->getBTerms()) {
    for (auto* bpin : bterm->getBPins()) {
      for (odb::dbBox* geom : bpin->getBoxes()) {
        for (const auto& [layer, shapes] :
             generatePolygonsFromBox(geom, odb::dbTransform())) {
          shapes_by_layer[layer].insert(shapes);
        }

        if (geom->isVia()) {
          continue;
        }

        auto* layer = geom->getTechLayer();
        const odb::Rect pin_shape = geom->getBox();

        // create bpin nodes
        auto term = std::make_unique<TerminalNode>(pin_shape, layer);
        auto pin_node = std::make_unique<BPinNode>(bpin, geom, layer);

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
    const odb::geom::BoostPolygon90WithHoles& polygon,
    const IRNetwork::TerminalTree& terminals,
    std::vector<std::unique_ptr<Shape>>& new_shapes,
    std::vector<std::unique_ptr<Node>>& new_nodes,
    std::map<Shape*, std::set<Node*>>& terminal_connections)
{
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

  odb::geom::BoostPolygon90Set shape_poly_set;
  shape_poly_set.insert(polygon);

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

    auto shape = std::make_unique<RectShape>(rect, layer);

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

void IRNetwork::processPolygonToTrapezoids(
    odb::dbTechLayer* layer,
    const Polygon45WithHoles& polygon,
    const IRNetwork::TerminalTree& terminals,
    std::vector<std::unique_ptr<Shape>>& new_shapes,
    std::vector<std::unique_ptr<Node>>& new_nodes,
    std::map<Shape*, std::set<Node*>>& terminal_connections)
{
  Polygon45Set shape_poly_set;
  shape_poly_set.insert(polygon);

  std::vector<Polygon45> trapezoids;
  shape_poly_set.get_trapezoids(trapezoids);

  // a piece that is a rectangle is held as a RectShape, only the others
  // need the polygon
  std::vector<std::unique_ptr<Shape>> shapes;
  std::vector<std::vector<odb::Point>> shape_points;
  for (const Polygon45& trapezoid : trapezoids) {
    std::vector<odb::Point> points;
    for (const auto& pt : trapezoid) {
      points.emplace_back(pt.x(), pt.y());
    }
    if (!points.empty() && points.front() != points.back()) {
      points.push_back(points.front());
    }
    const odb::Polygon piece(points);
    if (piece.isRect()) {
      shapes.push_back(
          std::make_unique<RectShape>(piece.getEnclosingRect(), layer));
    } else {
      shapes.push_back(std::make_unique<PolygonShape>(piece, layer));
    }
    shape_points.push_back(piece.getPoints());
  }

  for (std::size_t i = 0; i < shapes.size(); i++) {
    auto& shape = shapes[i];

    // Create starter nodes, at the center of the piece and of each edge it
    // shares with a later piece
    std::set<odb::Point> nodes;
    nodes.emplace(shape->getShape().center());
    for (std::size_t j = i + 1; j < shapes.size(); j++) {
      if (!shape->getShape().intersects(shapes[j]->getShape())) {
        continue;
      }
      for (const odb::Point& pt :
           getSharedEdgeCenters(shape_points[i], shape_points[j])) {
        nodes.emplace(pt);
      }
    }
    for (const auto& pt : nodes) {
      new_nodes.push_back(std::make_unique<Node>(pt, layer));
    }

    // check terminals
    auto& shape_terms = terminal_connections[shape.get()];
    for (auto itr = terminals.qbegin(
             boost::geometry::index::intersects(shape->getShape())
             && boost::geometry::index::satisfies([layer](const auto& other) {
                  return layer == other->getLayer();
                }));
         itr != terminals.qend();
         itr++) {
      auto* node = *itr;
      if (shape->contains(node->getPoint())) {
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
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate shapes: {}");

  LayerMap<odb::geom::BoostPolygon90Set> shapes_by_layer;
  LayerMap<Polygon45Set> shapes_45_by_layer;

  // Collect wires
  for (odb::dbSWire* wire : net_->getSWires()) {
    for (const auto& [layer, shapes] :
         generatePolygonsFromSWire(wire, shapes_45_by_layer)) {
      shapes_by_layer[layer].insert(shapes);
    }
  }

  std::vector<TerminalNode*> terminals;
  // Collect ITerms
  for (const auto& [layer, shapes] :
       generatePolygonsFromITerms(terminals, shapes_45_by_layer)) {
    shapes_by_layer[layer].insert(shapes);
  }

  // Collect BTerms
  for (const auto& [layer, shapes] : generatePolygonsFromBTerms(terminals)) {
    shapes_by_layer[layer].insert(shapes);
  }

  const TerminalTree terminal_nodes = getTerminalTree(terminals);

  // A layer with shapes with 45 degree edges is merged with them and sliced
  // into trapezoids rather than rectangles
  std::vector<std::pair<odb::dbTechLayer*, Polygon45WithHoles>>
      all_poly_45_shapes;
  for (auto& [layer, shapes_45] : shapes_45_by_layer) {
    const auto find_layer = shapes_by_layer.find(layer);
    if (find_layer != shapes_by_layer.end()) {
      shapes_45.insert(find_layer->second);
      shapes_by_layer.erase(find_layer);
    }

    std::vector<Polygon45WithHoles> shape_polygons;
    shapes_45.get_polygons_with_holes(shape_polygons);
    for (const Polygon45WithHoles& shape_poly : shape_polygons) {
      all_poly_45_shapes.emplace_back(layer, shape_poly);
    }
  }
  shapes_45_by_layer.clear();

  // Simplify shapes
  std::vector<std::pair<odb::dbTechLayer*, odb::geom::BoostPolygon90WithHoles>>
      all_poly_shapes;
  for (auto& [layer, shapes] : shapes_by_layer) {
    const utl::DebugScopedTimer layer_timer(
        logger_,
        utl::PSM,
        "timer",
        1,
        fmt::format("Convert shapes to polygons shapes on {}: {{}}",
                    layer->getName()));

    std::vector<odb::geom::BoostPolygon90WithHoles> shape_polygons;
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
  for (const auto& [layer, shape_poly] : all_poly_45_shapes) {
    processPolygonToTrapezoids(layer,
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
    Connections& new_connections)
{
  // handle as via
  std::vector<odb::dbShape> via_shapes;
  box->getViaBoxes(via_shapes);
  std::erase_if(via_shapes, [](const auto& shape) {
    return shape.getTechLayer()->getRoutingLevel() != 0;
  });

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

  const int min_pitch = std::min(min_node_pitch_[bottom], min_node_pitch_[top]);
  const bool use_single_via
      = floorplanning_ || box->getBox().maxDXDY() < min_pitch;

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
  const auto find_layer = shapes_.find(layer);
  if (find_layer == shapes_.end()) {
    return ShapeTree();
  }

  std::vector<Shape*> tree_values;
  for (const auto& shape : find_layer->second) {
    tree_values.emplace_back(shape.get());
  }
  ShapeTree tree(tree_values.begin(), tree_values.end());
  return tree;
}

IRNetwork::NodeTree IRNetwork::getNodeTree(odb::dbTechLayer* layer) const
{
  // generate Rtree of shapes to associate with vias
  const auto find_layer = nodes_.find(layer);
  if (find_layer == nodes_.end()) {
    return NodeTree();
  }

  std::vector<Node*> tree_values;
  for (const auto& node : find_layer->second) {
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
  Connections loop_via_connections;
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
  if (floorplanning_) {
    // these are only needed if running for IR drop.
    return;
  }

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
      // the box of a shape with 45 degree edges can hold a node that is
      // only in the shape next to it
      const odb::Point& pt = node->getPoint();
      const auto shapes = std::count_if(
          layer_shapes.qbegin(boost::geometry::index::intersects(pt)),
          layer_shapes.qend(),
          [&pt](const Shape* shape) { return shape->contains(pt); });
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
      const int min_distance = floorplanning_
                                   ? (2 * shape->getShape().maxDXDY())
                                   : min_node_pitch_[shape->getLayer()];
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

    std::ranges::stable_sort(shapes, [](const auto& lhs, const auto& rhs) {
      return lhs->getShape() < rhs->getShape();
    });
  }
}

void IRNetwork::sortNodes()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Sorting nodes: {}");

  for (auto& [layer, nodes] : nodes_) {
    std::ranges::stable_sort(nodes, [](const auto& lhs, const auto& rhs) {
      return lhs->compare(rhs) < 0;
    });
  }
}

void IRNetwork::sortConnections()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Sorting connections: {}");
  std::ranges::stable_sort(connections_, [](const auto& lhs, const auto& rhs) {
    return lhs->compare(rhs);
  });
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

  const auto add_connection = [&mapping](Connection* connection) {
    mapping[connection->getNode0()].push_back(connection);
    mapping[connection->getNode1()].push_back(connection);
  };
  if (isAssembly()) {
    for (Connection* connection : getAllConnections()) {
      add_connection(connection);
    }
  } else {
    for (const auto& connection : connections_) {
      add_connection(connection.get());
    }
  }

  for (auto& [node, conns] : mapping) {
    conns.shrink_to_fit();
  }

  return mapping;
}

std::size_t IRNetwork::getConnectionCount() const
{
  if (!isAssembly()) {
    return connections_.size();
  }
  std::size_t count = interdie_connections_.size();
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    count += chiplet.network->getConnectionCount();
  }
  return count;
}

std::vector<Connection*> IRNetwork::getAllConnections() const
{
  std::vector<Connection*> connections;
  connections.reserve(getConnectionCount());
  if (isAssembly()) {
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      const auto local_connections = chiplet.network->getAllConnections();
      connections.insert(connections.end(),
                         local_connections.begin(),
                         local_connections.end());
    }
    for (const auto& connection : interdie_connections_) {
      connections.push_back(connection.get());
    }
    return connections;
  }

  for (const auto& connection : connections_) {
    connections.push_back(connection.get());
  }
  return connections;
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

  std::erase_if(cleanup, [&](const auto& other) {
    return removes.find(other.get()) != removes.end();
  });

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

  std::erase_if(cleanup, [&](const auto& other) {
    return removes.find(other.get()) != removes.end();
  });

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
  odb::dbTechLayer* bstop = nodes_.begin()->first;
  if (bstop->isBackside()) {
    return bstop;
  }
  return nodes_.rbegin()->first;
}

odb::PtrSet<odb::dbTechLayer> IRNetwork::getLayers() const
{
  odb::PtrSet<odb::dbTechLayer> layers;
  if (isAssembly()) {
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      const auto local_layers = chiplet.network->getLayers();
      layers.insert(local_layers.begin(), local_layers.end());
    }
    return layers;
  }

  for (const auto& [layer, nodes] : nodes_) {
    layers.insert(layer);
  }
  for (const auto& conn : connections_) {
    if (conn->isVia()) {
      layers.insert(conn->getNode0()->getLayer()->getUpperLayer());
    }
  }
  return layers;
}

const std::vector<std::unique_ptr<Node>>& IRNetwork::getTopLayerNodes() const
{
  return nodes_.at(getTopLayer());
}

IRNetwork::NodeTree IRNetwork::getTopLayerNodeTree() const
{
  return getNodeTree(getTopLayer());
}

odb::PtrMap<odb::dbInst, Node::NodeSet> IRNetwork::getInstanceNodeMapping()
    const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate instance node map: {}");

  odb::PtrMap<odb::dbInst, Node::NodeSet> inst_nodes;
  for (const auto& node : iterm_nodes_) {
    odb::dbITerm* iterm = node->getITerm();
    odb::dbInst* inst = iterm->getInst();
    inst_nodes[inst].insert(node.get());
  }

  return inst_nodes;
}

odb::PtrMap<odb::dbInst, Node::NodeSet> IRNetwork::getInstanceNodeMapping(
    odb::dbNet* net) const
{
  if (!isAssembly()) {
    return net == net_ ? getInstanceNodeMapping()
                       : odb::PtrMap<odb::dbInst, Node::NodeSet>{};
  }
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    if (chiplet.net == net) {
      return chiplet.network->getInstanceNodeMapping();
    }
  }
  return {};
}

std::vector<odb::dbNet*> IRNetwork::getNets() const
{
  if (!isAssembly()) {
    return {net_};
  }
  std::vector<odb::dbNet*> nets;
  nets.reserve(chiplet_networks_.size());
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    nets.push_back(chiplet.net);
  }
  return nets;
}

std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>>
IRNetwork::getChipletNets() const
{
  std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>> result;
  result.reserve(chiplet_networks_.size());
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    result.emplace_back(chiplet.chip_inst, chiplet.net);
  }
  return result;
}

IRNetwork* IRNetwork::getChipletNetwork(odb::dbChipInst* chip_inst) const
{
  const auto chiplet = std::ranges::find_if(
      chiplet_networks_, [chip_inst](const ChipletNetwork& candidate) {
        return candidate.chip_inst == chip_inst;
      });
  return chiplet == chiplet_networks_.end() ? nullptr : chiplet->network.get();
}

std::vector<std::pair<odb::dbChipInst*, ITermNode*>>
IRNetwork::getChipletITermNodes() const
{
  std::vector<std::pair<odb::dbChipInst*, ITermNode*>> result;
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    for (const auto& node : chiplet.network->getITermNodes()) {
      result.emplace_back(chiplet.chip_inst, node.get());
    }
  }
  return result;
}

std::map<Node*, odb::dbChipInst*> IRNetwork::getNodeChiplets() const
{
  std::map<Node*, odb::dbChipInst*> result;
  for (const ChipletNetwork& chiplet : chiplet_networks_) {
    for (Node* node : chiplet.network->getAllNodes()) {
      result[node] = chiplet.chip_inst;
    }
  }
  return result;
}

void IRNetwork::reportStats() const
{
  if (isAssembly()) {
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      logger_->report("Chiplet {}, net {}:",
                      chiplet.chip_inst->getName(),
                      chiplet.net->getName());
      chiplet.network->reportStats();
    }
    logger_->report("Inter-chip bonds: {}", interdie_connections_.size());
    return;
  }

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
  if (isAssembly()) {
    return std::ranges::any_of(chiplet_networks_, [node](const auto& chiplet) {
      return chiplet.network->belongsTo(node);
    });
  }

  for (const auto& [layer, nodes] : nodes_) {
    if (std::ranges::find_if(
            nodes, [node](const auto& other) { return other.get() == node; })
        != nodes.end()) {
      return true;
    }
  }

  if (std::ranges::find_if(
          iterm_nodes_,
          [node](const auto& other) { return other.get() == node; })
      != iterm_nodes_.end()) {
    return true;
  }

  if (std::ranges::find_if(
          bpin_nodes_,
          [node](const auto& other) { return other.get() == node; })
      != bpin_nodes_.end()) {
    return true;
  }

  return false;
}

bool IRNetwork::belongsTo(Connection* connection) const
{
  if (isAssembly()) {
    if (std::ranges::any_of(chiplet_networks_,
                            [connection](const auto& chiplet) {
                              return chiplet.network->belongsTo(connection);
                            })) {
      return true;
    }
    return std::ranges::any_of(interdie_connections_,
                               [connection](const auto& candidate) {
                                 return candidate.get() == connection;
                               });
  }

  return std::ranges::find_if(connections_,
                              [connection](const auto& other) {
                                return other.get() == connection;
                              })
         != connections_.end();
}

IRNetwork::Nodes IRNetwork::getAllNodes() const
{
  Nodes nodes;
  if (isAssembly()) {
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      const Nodes local_nodes = chiplet.network->getAllNodes();
      nodes.insert(nodes.end(), local_nodes.begin(), local_nodes.end());
    }
    return nodes;
  }

  for (const auto& [layer, layer_nodes] : nodes_) {
    for (const auto& node : layer_nodes) {
      nodes.push_back(node.get());
    }
  }
  for (const auto& node : iterm_nodes_) {
    nodes.push_back(node.get());
  }
  for (const auto& node : bpin_nodes_) {
    nodes.push_back(node.get());
  }
  return nodes;
}

std::size_t IRNetwork::getNodeCount(bool include_iterms) const
{
  if (isAssembly()) {
    std::size_t count = 0;
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      count += chiplet.network->getNodeCount(true);
      count += chiplet.network->getBPinNodes().size();
    }
    return count;
  }

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

  odb::PtrMap<odb::dbTechLayer, std::set<odb::Rect>> nodes;
  for (const auto& bpin : bpin_nodes_) {
    if (bpin->shouldConnect()) {
      nodes[bpin->getLayer()].insert(bpin->getBBox());
    }
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

void IRNetwork::clearVisitedNodes()
{
  if (isAssembly()) {
    for (const ChipletNetwork& chiplet : chiplet_networks_) {
      chiplet.network->clearVisitedNodes();
    }
    return;
  }

  for (const auto& [layer, nodes] : nodes_) {
    for (const auto& node : nodes) {
      node->setVisited(false);
    }
  }
  for (const auto& node : iterm_nodes_) {
    node->setVisited(false);
  }
  for (const auto& node : bpin_nodes_) {
    node->setVisited(false);
  }
}

bool IRNetwork::isConnected() const
{
  return !getAllNodes().empty() && getDisconnectedNodes().empty();
}

IRNetwork::Nodes IRNetwork::getDisconnectedNodes() const
{
  const Nodes nodes = getAllNodes();
  if (nodes.empty()) {
    return {};
  }

  const NodePtrMap<Connection> connection_map = getConnectionMap();
  std::unordered_set<Node*> visited;
  std::queue<Node*> pending;
  Node* start = nodes.front();
  visited.insert(start);
  pending.push(start);
  while (!pending.empty()) {
    Node* node = pending.front();
    pending.pop();
    const auto connections = connection_map.find(node);
    if (connections == connection_map.end()) {
      continue;
    }
    for (Connection* connection : connections->second) {
      Node* neighbor = connection->getOtherNode(node);
      if (visited.insert(neighbor).second) {
        pending.push(neighbor);
      }
    }
  }

  Nodes disconnected;
  for (Node* node : nodes) {
    if (!visited.contains(node)) {
      disconnected.push_back(node);
    }
  }
  return disconnected;
}

}  // namespace psm
