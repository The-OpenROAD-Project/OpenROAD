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

#include "ir_solver.h"

#include <Eigen/SparseLU>
#include <fstream>
#include <list>
#include <queue>

#include "connection.h"
#include "db_sta/dbNetwork.hh"
#include "ir_network.h"
#include "node.h"
#include "odb/dbShape.h"
#include "rsz/Resizer.hh"
#include "shape.h"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "utl/timer.h"

namespace psm {

struct ODBCompare
{
  bool operator()(odb::dbObject* lhs, odb::dbObject* rhs) const
  {
    return lhs->getId() < rhs->getId();
  }
};

IRSolver::IRSolver(
    odb::dbNet* net,
    bool floorplanning,
    sta::dbSta* sta,
    rsz::Resizer* resizer,
    utl::Logger* logger,
    const std::map<odb::dbNet*, std::map<sta::Corner*, Voltage>>& user_voltages,
    const PDNSim::GeneratedSourceSettings& generated_source_settings)
    : net_(net),
      logger_(logger),
      resizer_(resizer),
      sta_(sta),
      network_(new IRNetwork(net_, logger_, floorplanning)),
      gui_(nullptr),
      user_voltages_(user_voltages),
      generated_source_settings_(generated_source_settings)
{
}

void IRSolver::enableGui(bool enable)
{
  if (enable) {
    if (gui_ == nullptr) {
      gui_ = std::make_unique<DebugGui>(network_.get());
    }
    gui_->populate();
  } else {
    gui_ = nullptr;
  }
}

odb::dbBlock* IRSolver::getBlock() const
{
  return net_->getBlock();
}

odb::dbTech* IRSolver::getTech() const
{
  return getBlock()->getTech();
}

PDNSim::IRDropByPoint IRSolver::getIRDrop(odb::dbTechLayer* layer,
                                          sta::Corner* corner) const
{
  PDNSim::IRDropByPoint ir_drop;

  if (!hasSolution(corner)) {
    return ir_drop;
  }

  const auto& nodes = network_->getNodes();

  auto find_layer = nodes.find(layer);
  if (find_layer == nodes.end()) {
    return ir_drop;
  }

  const auto net_voltage = getNetVoltage(corner);
  const bool is_ground = net_voltage == 0.0;

  const auto& voltages = voltages_.at(corner);

  for (const auto& node : find_layer->second) {
    const auto node_voltage = voltages.at(node.get());
    if (is_ground) {
      ir_drop[node->getPoint()] = node_voltage;
    } else {
      ir_drop[node->getPoint()] = net_voltage - node_voltage;
    }
  }

  return ir_drop;
}

bool IRSolver::check()
{
  const utl::DebugScopedTimer timer(logger_, utl::PSM, "timer", 1, "Check: {}");
  if (connected_.has_value()) {
    return connected_.value();
  }

  // set to true and unset if it failed
  connected_ = true;
  if (!checkOpen()) {
    reportUnconnectedNodes();
    connected_ = false;
  }
  if (!checkShort()) {
    connected_ = false;
  }

  return connected_.value();
}

bool IRSolver::wasNodeVisited(const std::unique_ptr<ITermNode>& node) const
{
  return wasNodeVisited(node.get());
}

bool IRSolver::wasNodeVisited(const std::unique_ptr<Node>& node) const
{
  return wasNodeVisited(node.get());
}

bool IRSolver::wasNodeVisited(const Node* node) const
{
  return visited_.find(node) != visited_.end();
}

bool IRSolver::checkOpen()
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Check open: {}");

  visited_.clear();
  const std::size_t total_nodes = network_->getNodeCount(true);
  const auto connections_map = network_->getConnectionMap();

  std::queue<Node*> queue;

  // grab any node to start, choose one on the highest metal layer
  Node* start = network_->getTopLayerNodes().begin()->get();
  debugPrint(
      logger_, utl::PSM, "check", 1, "Starting at: {}", start->describe(""));
  queue.push(start);

  std::size_t visited = 0;
  const bool print_progress = logger_->debugCheck(utl::PSM, "check", 2);
  // walk nodes
  while (!queue.empty()) {
    Node* node = queue.front();
    queue.pop();
    if (wasNodeVisited(node)) {
      // already been here, so we can continue to next node
      continue;
    }

    visited++;

    if (print_progress && visited % 1000 == 0) {
      debugPrint(logger_,
                 utl::PSM,
                 "check",
                 2,
                 "Checked {} nodes of {}",
                 visited,
                 total_nodes);
    }

    visited_.insert(node);

    for (const auto* conn : connections_map.at(node)) {
      Node* next = conn->getOtherNode(node);
      if (wasNodeVisited(next)) {
        // already been here, so we do not need to add it to the queue
        continue;
      }
      queue.push(next);
    }
  }

  for (const auto& [layer, layer_nodes] : network_->getNodes()) {
    for (const auto& node : layer_nodes) {
      if (wasNodeVisited(node)) {
        continue;
      }

      return false;
    }
  }

  return true;
}

IRSolver::ConnectivityResults IRSolver::getConnectivityResults() const
{
  ConnectivityResults results;

  for (const auto& [layer, nodes] : network_->getNodes()) {
    for (const auto& node : nodes) {
      if (wasNodeVisited(node)) {
        continue;
      }

      results.unconnected_nodes_.insert(node.get());
    }
  }

  for (const auto& node : network_->getITermNodes()) {
    if (wasNodeVisited(node)) {
      continue;
    }

    results.unconnected_iterms_.emplace(node.get());
  }

  return results;
}

void IRSolver::reportUnconnectedNodes() const
{
  // report unconnected nodes
  const double dbu = getBlock()->getDbUnitsPerMicron();
  const auto results = getConnectivityResults();

  for (auto* node : results.unconnected_nodes_) {
    logger_->warn(utl::PSM,
                  38,
                  "Unconnected node on net {} at location ({:4.3f}um, "
                  "{:4.3f}um), layer: {}.",
                  net_->getName(),
                  node->getPoint().getX() / dbu,
                  node->getPoint().getY() / dbu,
                  node->getLayer()->getName());
  }

  for (const auto& node : results.unconnected_iterms_) {
    logger_->warn(utl::PSM,
                  39,
                  "Unconnected instance {} at location ({:4.3f}um, "
                  "{:4.3f}um).",
                  node->getITerm()->getName(),
                  node->getPoint().getX() / dbu,
                  node->getPoint().getY() / dbu);
  }
}

bool IRSolver::checkShort() const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Check short: {}");

  // Dummy implementation of short
  return true;
}

Connection::ResistanceMap IRSolver::getResistanceMap(sta::Corner* corner) const
{
  Connection::ResistanceMap resistance;

  const double dbus = getBlock()->getDbUnitsPerMicron();
  odb::dbTech* tech = getBlock()->getTech();

  for (auto* layer : tech->getLayers()) {
    Connection::Resistance res = 0.0;

    switch (layer->getType()) {
      case odb::dbTechLayerType::ROUTING: {
        double r_per_meter, cap_per_meter;
        resizer_->layerRC(layer, corner, r_per_meter, cap_per_meter);
        const double width_meter
            = static_cast<double>(layer->getWidth()) / dbus * 1e-6;
        res = r_per_meter * width_meter;
        break;
      }
      case odb::dbTechLayerType::CUT: {
        double cap;
        resizer_->layerRC(layer, corner, res, cap);
        break;
      }
      default:
        break;
    }

    debugPrint(logger_,
               utl::PSM,
               "resistance",
               2,
               "Resizer resistance for {} = {}",
               layer->getName(),
               res);
    if (res == 0.0) {
      // Get database resistance
      res = layer->getResistance();
      debugPrint(logger_,
                 utl::PSM,
                 "resistance",
                 2,
                 "Database resistance for {} = {}",
                 layer->getName(),
                 res);
    }

    resistance[layer] = res;
  }

  if (logger_->debugCheck(utl::PSM, "resistance", 1)) {
    logger_->report("Layer resistance:");
    for (const auto& [layer, res] : resistance) {
      if (layer->getRoutingLevel() == 0) {
        logger_->report("  {}: {} Ohm per cut", layer->getName(), res);
      } else {
        logger_->report("  {}: {} Ohm per square", layer->getName(), res);
      }
    }
  }

  return resistance;
}

std::map<Connection*, Connection::Conductance> IRSolver::generateConductanceMap(
    sta::Corner* corner) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate conductance map: {}");

  const Connection::ResistanceMap resistance = getResistanceMap(corner);

  std::map<Connection*, Connection::Conductance> conductance;
  for (const auto& conn : network_->getConnections()) {
    const auto res = conn->getResistance(resistance);
    conductance[conn.get()] = 1.0 / res;
  }

  return conductance;
}

IRSolver::Voltage IRSolver::generateSourceNodes(
    GeneratedSourceType source_type,
    const std::string& source_file,
    sta::Corner* corner,
    std::vector<std::unique_ptr<SourceNode>>& sources) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate source nodes: {}");
  sources.clear();

  Voltage src_voltage = getNetVoltage(corner);
  if (!source_file.empty()) {
    src_voltage
        = generateSourceNodesFromSourceFile(source_file, corner, sources);
  } else {
    sources = generateSourceNodesFromBTerms();

    if (sources.empty()) {
      switch (source_type) {
        case GeneratedSourceType::FULL:
          sources = generateSourceNodesGenericFull();
          break;
        case GeneratedSourceType::STRAPS:
          sources = generateSourceNodesGenericStraps();
          break;
        case GeneratedSourceType::BUMPS:
          sources = generateSourceNodesGenericBumps();
          break;
      }
    }
  }
  if (gui_) {
    gui_->setSources(sources);
  }

  if (logger_->debugCheck(utl::PSM, "solve", 3)) {
    logger_->report("Source nodes: {}", sources.size());
  }

  if (sources.empty()) {
    logger_->error(utl::PSM, 70, "Unable to map source nodes into power grid");
  }

  return src_voltage;
}

std::vector<std::unique_ptr<SourceNode>>
IRSolver::generateSourceNodesFromBTerms() const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Generate source nodes from bterms: {}");

  std::vector<std::unique_ptr<SourceNode>> src_nodes;

  for (auto* root_node : network_->getBPinShapeNodes()) {
    src_nodes.push_back(std::make_unique<SourceNode>(root_node));
  }

  return src_nodes;
}

std::vector<std::unique_ptr<SourceNode>>
IRSolver::generateSourceNodesGenericFull() const
{
  std::vector<std::unique_ptr<SourceNode>> src_nodes;

  logger_->info(utl::PSM,
                71,
                "Using all nodes on {} as sources.",
                network_->getTopLayer()->getName());
  for (const auto& root_node : network_->getTopLayerNodes()) {
    src_nodes.push_back(std::make_unique<SourceNode>(root_node.get()));
  }

  return src_nodes;
}

std::vector<std::unique_ptr<SourceNode>>
IRSolver::generateSourceNodesGenericStraps() const
{
  odb::dbTechLayer* top_layer = network_->getTopLayer();
  odb::dbTechLayer* connect_layer
      = getTech()->findRoutingLayer(top_layer->getRoutingLevel() + 1);
  if (connect_layer == nullptr) {
    connect_layer = top_layer;
  }
  const int pitch = generated_source_settings_.strap_track_pitch
                    * connect_layer->getPitch();
  const int offset = pitch / 2;
  const int width = connect_layer->getWidth();

  const double dbus = getBlock()->getDbUnitsPerMicron();
  logger_->info(
      utl::PSM,
      72,
      "Using strap pattern on {} with pitch {:.4f}um and offset {:.4f}um.",
      connect_layer->getName(),
      pitch / dbus,
      offset / dbus);

  const odb::Rect core_area = getBlock()->getCoreArea();
  const odb::Rect die_area = getBlock()->getDieArea();
  std::set<odb::Rect> straps;
  if (top_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    for (int x_low = core_area.xMin() + offset; x_low < core_area.xMax();
         x_low += pitch) {
      straps.emplace(x_low, die_area.yMin(), x_low + width, die_area.yMax());
    }
  } else {
    for (int y_low = core_area.yMin() + offset; y_low < core_area.yMax();
         y_low += pitch) {
      straps.emplace(die_area.xMin(), y_low, die_area.xMax(), y_low + width);
    }
  }

  if (straps.empty()) {
    const odb::Point center_shift(core_area.xCenter() - width / 2,
                                  core_area.yCenter() - width / 2);
    if (top_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      straps.emplace(center_shift.x(),
                     die_area.yMin(),
                     center_shift.x() + width,
                     die_area.yMax());
    } else {
      straps.emplace(die_area.xMin(),
                     center_shift.y(),
                     die_area.xMax(),
                     center_shift.y() + width);
    }
  }

  return generateSourceNodesFromShapes(straps);
}

std::vector<std::unique_ptr<SourceNode>>
IRSolver::generateSourceNodesGenericBumps() const
{
  const double dbus = getBlock()->getDbUnitsPerMicron();

  const int dx = generated_source_settings_.bump_dx * dbus;
  const int dy = generated_source_settings_.bump_dy * dbus;
  const int size = generated_source_settings_.bump_size * dbus;

  logger_->info(utl::PSM,
                73,
                "Using bump pattern with x-pitch {:.4f}um, y-pitch {:.4f}um, "
                "and size {:.4f}um with an reduction factor of {}x.",
                dx / dbus,
                dy / dbus,
                size / dbus,
                generated_source_settings_.bump_interval);

  const odb::Rect die_area = getBlock()->getDieArea();
  std::set<odb::Rect> bumps;
  int row_count = 0;
  for (int x = die_area.xMin() + dx / 2; x < die_area.xMax(); x += dx) {
    const int col_offset = row_count % generated_source_settings_.bump_interval;
    int col_count = 0;
    for (int y = die_area.yMin() + dy / 2; y < die_area.yMax(); y += dy) {
      if ((col_offset + col_count) % generated_source_settings_.bump_interval
          == 0) {
        bumps.emplace(x - size / 2, y - size / 2, x + size / 2, y + size / 2);
      }
      col_count++;
    }
    row_count++;
  }

  if (bumps.empty()) {
    const odb::Point die_center(die_area.xCenter(), die_area.yCenter());
    bumps.emplace(die_center.x() - size / 2,
                  die_center.y() - size / 2,
                  die_center.x() + size / 2,
                  die_center.y() + size / 2);
  }

  return generateSourceNodesFromShapes(bumps);
}

std::vector<std::unique_ptr<SourceNode>>
IRSolver::generateSourceNodesFromShapes(const std::set<odb::Rect>& shapes) const
{
  std::set<odb::Rect> intersect_shapes;
  const auto& top_shapes = network_->getShapes().at(network_->getTopLayer());
  for (const auto& shape : shapes) {
    bool used = false;
    for (const auto& top_shape : top_shapes) {
      if (shape.intersects(top_shape->getShape())) {
        used = true;
        intersect_shapes.insert(shape.intersect(top_shape->getShape()));
      }
    }
    if (!used) {
      intersect_shapes.insert(shape);
    }
  }

  std::map<odb::Point, std::vector<std::unique_ptr<SourceNode>>> source_nodes;
  const double dbus = getBlock()->getDbUnitsPerMicron();

  const auto top_nodes = network_->getTopLayerNodeTree();
  for (const auto& shape : intersect_shapes) {
    debugPrint(
        logger_,
        utl::PSM,
        "solve",
        3,
        "Searching for source nodes in ({:.4f}, {:.4f}) - ({:.4f}, {:.4f})",
        shape.xMin() / dbus,
        shape.yMin() / dbus,
        shape.xMax() / dbus,
        shape.yMax() / dbus);
    bool found = false;
    for (auto itr = top_nodes.qbegin(boost::geometry::index::intersects(shape));
         itr != top_nodes.qend();
         itr++) {
      source_nodes[(*itr)->getPoint()].push_back(
          std::make_unique<SourceNode>(*itr));
      found = true;
    }

    if (!found) {
      // Since the shape didn't intersect anything, we need to pick the nearest
      // node
      const odb::Point pt(shape.xCenter(), shape.yCenter());
      std::vector<Node*> returned_nodes;
      top_nodes.query(boost::geometry::index::nearest(pt, 1),
                      std::back_inserter(returned_nodes));

      for (Node* node : returned_nodes) {
        source_nodes[node->getPoint()].push_back(
            std::make_unique<SourceNode>(node));
      }
    }
  }

  std::vector<std::unique_ptr<SourceNode>> src_nodes;
  src_nodes.reserve(source_nodes.size());
  for (auto& [pt, nodes] : source_nodes) {
    // ensure nodes are unique
    src_nodes.push_back(std::move(nodes[0]));
  }

  if (gui_) {
    gui_->setSourceShapes(network_->getTopLayer(), shapes);
  }

  return src_nodes;
}

IRSolver::Voltage IRSolver::generateSourceNodesFromSourceFile(
    const std::string& source_file,
    sta::Corner* corner,
    std::vector<std::unique_ptr<SourceNode>>& sources) const
{
  const utl::DebugScopedTimer timer(
      logger_,
      utl::PSM,
      "timer",
      1,
      "Generate source nodes from voltage source file: {}");

  sources.clear();

  struct Record
  {
    int x = 0.0;
    int y = 0.0;
    int size = 0.0;
    Voltage voltage = 0.0;
  };

  std::ifstream vsrc(source_file);
  if (!vsrc) {
    logger_->error(utl::PSM, 89, "Unable to open {}.", source_file);
  }

  logger_->info(
      utl::PSM, 15, "Reading location of sources from: {}.", source_file);

  std::vector<Record> source_info;

  const int dbus = getBlock()->getDbUnitsPerMicron();
  std::string line;
  // Iterate through each line and split the content using delimiter
  while (std::getline(vsrc, line)) {
    std::stringstream line_stream(line);
    std::string value;

    double x = 0.0;
    double y = 0.0;
    double size = 0.0;
    Voltage voltage = 0.0;

    for (int i = 0; i < 4; ++i) {
      if (line_stream.eof()) {
        logger_->error(
            utl::PSM, 75, "Expected four values on line: \"{}\"", line);
      }
      std::getline(line_stream, value, ',');
      if (i == 0) {
        x = std::stod(value);
      } else if (i == 1) {
        y = std::stod(value);
      } else if (i == 2) {
        size = std::stod(value);
      } else {
        voltage = std::stof(value);
      }
    }

    source_info.emplace_back(Record{static_cast<int>(dbus * x),
                                    static_cast<int>(dbus * y),
                                    static_cast<int>(dbus * size),
                                    voltage});
  }

  const Voltage src_voltage = source_info.begin()->voltage;
  for (const auto& info : source_info) {
    if (src_voltage != info.voltage) {
      logger_->error(utl::PSM, 80, "Source voltage cannot be different");
    }
  }

  std::set<odb::Rect> source_shapes;

  for (const auto& info : source_info) {
    const odb::Rect check_area(info.x - info.size / 2,
                               info.y - info.size / 2,
                               info.x + info.size / 2,
                               info.y + info.size / 2);
    source_shapes.insert(check_area);
  }

  sources = generateSourceNodesFromShapes(source_shapes);

  return src_voltage;
}

void IRSolver::buildNodeCurrentMap(sta::Corner* corner,
                                   ValueNodeMap<Current>& currents) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Build node/current map: {}");
  // Build power map
  const auto inst_nodes = network_->getInstanceNodeMapping();
  const Voltage power_voltage = getPowerNetVoltage(corner);
  if (power_voltage == 0) {
    logger_->error(utl::PSM, 74, "Unable to determine voltage for power nets.");
  }
  for (const auto& [inst, power] : getInstancePower(corner)) {
    const Current current = power / power_voltage;
    auto find_inst = inst_nodes.find(inst);
    if (find_inst == inst_nodes.end()) {
      continue;
    }
    const auto& nodes = find_inst->second;
    for (auto* node : nodes) {
      currents[node] += current / nodes.size();
    }
  }
}

std::map<Node*, Connection::ConnectionSet> IRSolver::getNodeConnectionMap(
    const std::map<psm::Connection*, Connection::Conductance>& conductance)
    const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Build node/connection mapping: {}");

  std::map<Node*, Connection::ConnectionSet> node_connections;
  for (const auto& [connection, cond] : conductance) {
    Node* node0 = connection->getNode0();
    Node* node1 = connection->getNode1();

    node_connections[node0].insert(connection);
    node_connections[node1].insert(connection);
  }

  return node_connections;
}

std::map<Node*, std::size_t> IRSolver::assignNodeIDs(const Node::NodeSet& nodes,
                                                     std::size_t start) const
{
  std::size_t idx = start;
  std::map<Node*, std::size_t> node_index;
  for (auto* node : nodes) {
    node_index[node] = idx++;
  }
  return node_index;
}

std::map<Node*, std::size_t> IRSolver::assignNodeIDs(
    const std::vector<std::unique_ptr<SourceNode>>& nodes,
    std::size_t start) const
{
  Node::NodeSet node_set;
  for (const auto& node : nodes) {
    node_set.insert(node.get());
  }
  return assignNodeIDs(node_set, start);
}

void IRSolver::buildCondMatrixAndVoltages(
    bool is_ground,
    const std::map<Node*, Connection::ConnectionSet>& node_connections,
    const ValueNodeMap<Current>& currents,
    const std::map<psm::Connection*, Connection::Conductance>& conductance,
    const std::map<Node*, std::size_t>& node_index,
    Eigen::SparseMatrix<Connection::Conductance>& G,
    Eigen::VectorXd& J) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Build G and J: {}");

  const bool print_progress = logger_->debugCheck(utl::PSM, "progress", 1);
  std::size_t count = 0;
  std::vector<Eigen::Triplet<Connection::Conductance>> cond_values;
  for (const auto& [node, connections] : node_connections) {
    const std::size_t node_idx = node_index.at(node);

    auto find_node = currents.find(node);
    if (find_node == currents.end()) {
      J[node_idx] = 0;
    } else {
      J[node_idx] = find_node->second;
    }

    Connection::Conductance node_cond = 0.0;
    for (auto* conn : connections) {
      Node* other = conn->getOtherNode(node);
      const std::size_t other_idx = node_index.at(other);

      const Connection::Conductance cond = conductance.at(conn);
      node_cond += cond;

      cond_values.emplace_back(node_idx, other_idx, -cond);
    }
    cond_values.emplace_back(node_idx, node_idx, node_cond);
    if (print_progress && count % 1000 == 0) {
      logger_->report(
          "Processed nodes: {} of {}", count, node_connections.size());
    }
    count++;
  }
  G.setFromTriplets(cond_values.begin(), cond_values.end());
  if (!is_ground) {
    for (auto& j : J) {
      j = -j;
    }
  }
  cond_values.clear();
}

void IRSolver::addSourcesToMatrixAndVoltages(
    Voltage src_voltage,
    const std::vector<std::unique_ptr<psm::SourceNode>>& sources,
    const std::map<Node*, std::size_t>& node_index,
    Eigen::SparseMatrix<Connection::Conductance>& G,
    Eigen::VectorXd& J) const
{
  // Attach sources as current sources through a 1 ohm resistor
  constexpr Connection::Resistance src_res = 1.0;
  const Connection::Conductance src_cond = 1.0 / src_res;

  for (const auto& src_node : sources) {
    const std::size_t idx = node_index.at(src_node.get());

    J[idx] = src_voltage / src_res;

    Node* real_node = src_node->getSource();

    const std::size_t real_node_idx = node_index.at(real_node);

    debugPrint(logger_,
               utl::PSM,
               "solve",
               2,
               "Adding source node {} to {}",
               idx,
               real_node_idx);

    G.insert(idx, real_node_idx) = src_cond;
    G.insert(real_node_idx, idx) = src_cond;
  }
}

void IRSolver::solve(sta::Corner* corner,
                     GeneratedSourceType source_type,
                     const std::string& source_file)
{
  const utl::DebugScopedTimer timer(logger_, utl::PSM, "timer", 1, "Solve: {}");

  if (network_->isFloorplanningOnly()) {
    network_->setFloorplanning(false);
    network_->construct();
  }

  // Reset
  auto& voltages = voltages_[corner];
  auto& currents = currents_[corner];

  voltages.clear();
  currents.clear();

  const auto conductance = generateConductanceMap(corner);
  debugPrint(logger_,
             utl::PSM,
             "stats",
             1,
             "Connections in conductance map: {}",
             conductance.size());

  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    dumpConductance(conductance, "cond");
  }

  const auto node_connections = getNodeConnectionMap(conductance);
  Node::NodeSet all_nodes;
  for (const auto& [node, conns] : node_connections) {
    all_nodes.insert(node);
  }

  buildNodeCurrentMap(corner, currents);

  // Build source map
  std::vector<std::unique_ptr<SourceNode>> src_nodes;
  Voltage src_voltage
      = generateSourceNodes(source_type, source_file, corner, src_nodes);

  // Solve
  // create vector of nodes
  std::map<Node*, std::size_t> node_index = assignNodeIDs(all_nodes);
  const std::map<Node*, std::size_t> real_node_index = node_index;
  for (const auto& [node, id] : assignNodeIDs(src_nodes, node_index.size())) {
    node_index[node] = id;
  }

  const std::size_t num_nodes = node_index.size();

  debugPrint(logger_,
             utl::PSM,
             "stats",
             1,
             "Nodes in all nodes: {}",
             all_nodes.size());
  debugPrint(logger_, utl::PSM, "stats", 1, "Nodes in matrix: {}", num_nodes);

  // create sparse matrix and vector
  Eigen::SparseMatrix<Connection::Conductance> G(num_nodes, num_nodes);
  Eigen::VectorXd J(num_nodes);

  // Build G and J
  buildCondMatrixAndVoltages(src_voltage == 0.0,
                             node_connections,
                             currents,
                             conductance,
                             node_index,
                             G,
                             J);
  addSourcesToMatrixAndVoltages(src_voltage, src_nodes, node_index, G, J);

  Eigen::SparseLU<Eigen::SparseMatrix<Connection::Conductance>> eigen_solver;

  debugPrint(logger_, utl::PSM, "solve", 1, "Factorizing the G matrix");
  eigen_solver.compute(G);
  if (eigen_solver.info() != Eigen::ComputationInfo::Success) {
    // decomposition failed
    if (logger_->debugCheck(utl::PSM, "dump", 1)) {
      network_->dumpNodes(node_index);
      dumpMatrix(G, "G");
    }
    logger_->error(
        utl::PSM,
        10,
        "LU factorization of the G Matrix failed. SparseLU solver message: {}.",
        eigen_solver.lastErrorMessage());
  }

  debugPrint(logger_, utl::PSM, "solve", 1, "Solving system of equations GV=J");
  const Eigen::VectorXd V = eigen_solver.solve(J);
  if (eigen_solver.info() != Eigen::ComputationInfo::Success) {
    // solving failed
    if (logger_->debugCheck(utl::PSM, "dump", 1)) {
      network_->dumpNodes(node_index);
      dumpMatrix(G, "G");
      dumpVector(J, "J");
    }
    logger_->error(utl::PSM, 12, "Solving V = inv(G)*J failed.");
  }
  debugPrint(logger_,
             utl::PSM,
             "solve",
             1,
             "Solving system of equations GV=J complete");

  if (logger_->debugCheck(utl::PSM, "dump", 2)) {
    network_->dumpNodes(node_index);
    dumpMatrix(G, "G");
    dumpVector(J, "J");
    dumpVector(V, "V");
  }
  for (const auto& [node, node_idx] : real_node_index) {
    voltages[node] = V[node_idx];
  }
  solution_voltages_[corner] = src_voltage;
}

std::map<odb::dbInst*, IRSolver::Power> IRSolver::getInstancePower(
    sta::Corner* corner) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "Power calculation: {}");

  std::map<odb::dbInst*, IRSolver::Power> inst_power;

  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    odb::dbInst* db_inst = network->staToDb(inst);

    sta::LibertyCell* cell = network->libertyCell(inst);
    if (cell != nullptr) {
      const sta::PowerResult power = sta_->power(inst, corner);

      inst_power[db_inst] = power.total();
      debugPrint(logger_,
                 utl::PSM,
                 "power",
                 1,
                 "Power of instance {} is {}",
                 db_inst->getName(),
                 power.total());
    }
  }

  return inst_power;
}

std::optional<IRSolver::Voltage> IRSolver::getSDCVoltage(sta::Corner* corner,
                                                         odb::dbNet* net) const
{
  const auto max = sta::MinMax::max();
  const sta::dbNetwork* network = sta_->getDbNetwork();

  sta::Sdc* sdc = sta_->sdc();
  bool exists;
  float sdc_voltage;
  sdc->voltage(network->dbToSta(net), max, sdc_voltage, exists);
  if (exists) {
    return sdc_voltage;
  }

  sdc->voltage(max, sdc_voltage, exists);
  if (exists) {
    return sdc_voltage;
  }

  return {};
}

std::optional<IRSolver::Voltage> IRSolver::getPVTVoltage(
    sta::Corner* corner) const
{
  const auto max = sta::MinMax::max();
  const sta::dbNetwork* network = sta_->getDbNetwork();

  const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max);
  const sta::Pvt* pvt = dcalc_ap->operatingConditions();
  if (pvt == nullptr) {
    const sta::LibertyLibrary* default_library
        = network->defaultLibertyLibrary();

    if (default_library != nullptr) {
      pvt = default_library->defaultOperatingConditions();
    }
  }
  if (pvt) {
    return pvt->voltage();
  }

  return {};
}

std::optional<IRSolver::Voltage> IRSolver::getUserVoltage(sta::Corner* corner,
                                                          odb::dbNet* net) const
{
  auto find_net = user_voltages_.find(net);
  if (find_net == user_voltages_.end()) {
    return {};
  }

  const auto& voltages = find_net->second;

  auto find_corner = voltages.find(corner);
  if (find_corner != voltages.end()) {
    return find_corner->second;
  }

  auto find_default = voltages.find(nullptr);
  if (find_default != voltages.end()) {
    return find_default->second;
  }

  return {};
}

std::optional<IRSolver::Voltage> IRSolver::getSolutionVoltage(
    sta::Corner* corner) const
{
  auto find_corner = solution_voltages_.find(corner);
  if (find_corner != solution_voltages_.end()) {
    return find_corner->second;
  }

  return {};
}

odb::dbNet* IRSolver::getPowerNet() const
{
  if (net_->getSigType() == odb::dbSigType::POWER) {
    return net_;
  }

  for (const auto& [net, voltages] : user_voltages_) {
    if (net->getSigType() == odb::dbSigType::POWER) {
      return net;
    }
  }

  return nullptr;
}

IRSolver::Voltage IRSolver::getPowerNetVoltage(sta::Corner* corner) const
{
  odb::dbNet* net = getPowerNet();

  if (net == net_) {
    const auto solution_voltage = getSolutionVoltage(corner);
    if (solution_voltage.has_value()) {
      return solution_voltage.value();
    }
  }

  if (net != nullptr) {
    const auto user_voltage = getUserVoltage(corner, net);
    if (user_voltage.has_value()) {
      return user_voltage.value();
    }

    const auto sdc_voltage = getSDCVoltage(corner, net);
    if (sdc_voltage.has_value()) {
      return sdc_voltage.value();
    }
  }

  const auto pvt_voltage = getPVTVoltage(corner);
  if (pvt_voltage.has_value()) {
    return pvt_voltage.value();
  }

  logger_->error(utl::PSM,
                 79,
                 "Cannot determine the supply voltage for {}.",
                 net_->getName());

  return 0.0;
}

IRSolver::Voltage IRSolver::getNetVoltage(sta::Corner* corner) const
{
  if (net_->getSigType() == odb::dbSigType::GROUND) {
    return 0.0;
  }

  return getPowerNetVoltage(corner);
}

bool IRSolver::hasSolution(sta::Corner* corner) const
{
  const bool has_voltages = voltages_.find(corner) != voltages_.end();
  const bool has_currents = currents_.find(corner) != currents_.end();

  if (has_voltages && has_currents) {
    return !voltages_.at(corner).empty() && !currents_.at(corner).empty();
  }
  return false;
}

IRSolver::Results IRSolver::getSolution(sta::Corner* corner) const
{
  Results results;
  if (!hasSolution(corner)) {
    return results;
  }

  results.net_voltage = solution_voltages_.at(corner);

  const bool is_ground = results.net_voltage == 0.0;
  auto worst_calc = [is_ground](Voltage& worst, Voltage check) {
    if (is_ground) {
      worst = std::max(worst, check);
    } else {
      worst = std::min(worst, check);
    }
  };

  double all_voltage = 0.0;
  int node_count = 0;
  Voltage worst_voltage = results.net_voltage;
  const auto& corner_voltages = voltages_.at(corner);

  for (const auto& [layer, nodes] : network_->getNodes()) {
    for (const auto& node : nodes) {
      const Voltage node_voltage = corner_voltages.at(node.get());
      all_voltage += node_voltage;
      node_count++;
      worst_calc(worst_voltage, node_voltage);
    }
  }

  if (node_count == 0) {
    return results;
  }

  results.worst_voltage = worst_voltage;
  results.avg_voltage = all_voltage / node_count;

  if (results.net_voltage == 0.0) {
    results.worst_ir_drop = results.worst_voltage;
    results.avg_ir_drop = results.avg_voltage;
  } else {
    results.worst_ir_drop = results.net_voltage - results.worst_voltage;
    results.avg_ir_drop = results.net_voltage - results.avg_voltage;
  }

  const auto power_voltage = getPowerNetVoltage(corner);
  results.max_percent = 100 * results.worst_ir_drop / power_voltage;

  return results;
}

IRSolver::EMResults IRSolver::getEMSolution(sta::Corner* corner) const
{
  const utl::DebugScopedTimer timer(
      logger_, utl::PSM, "timer", 1, "EM solution: {}");

  EMResults results;
  if (!hasSolution(corner)) {
    return results;
  }

  Current total_current = 0.0;
  for (const auto& [connection, current] : generateCurrentMap(corner)) {
    results.resistors++;
    total_current += current;
    results.max_current = std::max(results.max_current, current);
  }

  if (results.resistors > 0) {
    results.avg_current = total_current / results.resistors;
  }

  return results;
}

std::string IRSolver::getMetricKey(const std::string& key,
                                   sta::Corner* corner) const
{
  const std::string corner_name
      = corner != nullptr ? corner->name() : "default";
  const std::string metric_suffix
      = fmt::format("__net:{}__corner:{}", net_->getName(), corner_name);

  return key + metric_suffix;
}

void IRSolver::report(sta::Corner* corner) const
{
  const auto results = getSolution(corner);

  logger_->report("########## IR report #################");
  logger_->report("Net              : {}", net_->getName());
  logger_->report("Corner           : {}", corner->name());
  logger_->report("Supply voltage   : {:3.2e} V", results.net_voltage);
  logger_->report("Worstcase voltage: {:3.2e} V", results.worst_voltage);
  logger_->report("Average voltage  : {:3.2e} V", results.avg_voltage);
  logger_->report("Average IR drop  : {:3.2e} V", results.avg_ir_drop);
  logger_->report("Worstcase IR drop: {:3.2e} V", results.worst_ir_drop);
  logger_->report("Percentage drop  : {:3.2f} %", results.max_percent);
  logger_->report("######################################");

  logger_->metric(getMetricKey("design_powergrid__voltage__worst", corner),
                  results.worst_voltage);
  logger_->metric(getMetricKey("design_powergrid__drop__average", corner),
                  results.avg_voltage);
  logger_->metric(getMetricKey("design_powergrid__drop__worst", corner),
                  results.worst_ir_drop);
}

void IRSolver::reportEM(sta::Corner* corner) const
{
  const auto results = getEMSolution(corner);

  logger_->report("########## EM analysis ###############");
  logger_->report("Net                : {}", net_->getName());
  logger_->report("Corner             : {}", corner->name());
  logger_->report("Maximum current    : {:3.2e} A", results.max_current);
  logger_->report("Average current    : {:3.2e} A", results.avg_current);
  logger_->report("Number of resistors: {}", results.resistors);
  logger_->report("######################################");

  logger_->metric(getMetricKey("design_powergrid__current__max", corner),
                  results.max_current);
  logger_->metric(getMetricKey("design_powergrid__current__average", corner),
                  results.avg_current);
}

void IRSolver::writeErrorFile(const std::string& error_file) const
{
  if (error_file.empty()) {
    return;
  }

  std::ofstream report(error_file);
  if (!report) {
    logger_->error(
        utl::PSM, 92, "Unable to open {} to write error file", error_file);
  }

  const auto results = getConnectivityResults();

  const double bbox_size = 0.05;
  const double dbus = getBlock()->getDbUnitsPerMicron();
  for (auto* node : results.unconnected_nodes_) {
    const odb::Point& pt = node->getPoint();
    const double pt_x = pt.getX() / dbus;
    const double pt_y = pt.getY() / dbus;

    report << "violation type: Unconnected node\n";
    report << "  srcs: net:" << net_->getName() << '\n';
    report << fmt::format(
        "    bbox = ({:.4f}, {:.4f}) - ({:.4f}, {:.4f}) on Layer {}",
        pt_x - bbox_size,
        pt_y - bbox_size,
        pt_x + bbox_size,
        pt_y + bbox_size,
        node->getLayer()->getName())
           << '\n';
  }

  std::set<odb::dbInst*, ODBCompare> insts;
  for (const auto& node : results.unconnected_iterms_) {
    insts.insert(node->getITerm()->getInst());
  }
  for (auto* inst : insts) {
    const odb::Rect inst_rect = inst->getBBox()->getBox();
    report << "violation type: Unconnected instance\n";
    report << "  srcs: inst:" << inst->getName() << '\n';
    report << fmt::format(
        "    bbox = ({:.4f}, {:.4f}) - ({:.4f}, {:.4f}) on Layer -",
        inst_rect.xMin() / dbus,
        inst_rect.yMin() / dbus,
        inst_rect.xMax() / dbus,
        inst_rect.yMax() / dbus)
           << '\n';
  }
}

void IRSolver::writeInstanceVoltageFile(const std::string& voltage_file,
                                        sta::Corner* corner) const
{
  if (voltage_file.empty()) {
    return;
  }

  std::ofstream report(voltage_file);
  if (!report) {
    logger_->error(utl::PSM,
                   90,
                   "Unable to open {} to write instance voltage file",
                   voltage_file);
  }

  report << "Instance,Terminal,Layer,X location,Y location,Voltage\n";

  const auto& voltages = voltages_.at(corner);

  const double dbus = getBlock()->getDbUnitsPerMicron();
  for (const auto& node : network_->getITermNodes()) {
    const auto& pt = node->getPoint();
    odb::dbTechLayer* layer = node->getLayer();

    const std::string x_loc = fmt::format("{:.4f}", pt.getX() / dbus);
    const std::string y_loc = fmt::format("{:.4f}", pt.getY() / dbus);
    const std::string voltage = fmt::format("{:.6f}", voltages.at(node.get()));

    odb::dbITerm* iterm = node->getITerm();
    odb::dbInst* inst = iterm->getInst();
    odb::dbMTerm* term = iterm->getMTerm();

    report << inst->getName() << ",";
    report << term->getName() << ",";
    report << layer->getName() << ",";
    report << x_loc << ",";
    report << y_loc << ",";
    report << voltage << '\n';
  }
}

void IRSolver::writeEMFile(const std::string& em_file,
                           sta::Corner* corner) const
{
  if (em_file.empty()) {
    return;
  }

  std::ofstream report(em_file);
  if (!report) {
    logger_->error(utl::PSM, 91, "Unable to open {} to write EM file", em_file);
  }

  report << "Node0 Layer,Node0 X location,Node0 Y location,Node1 Layer,Node1 X "
            "location,Node1 Y location,Current\n";

  const auto current_map = generateCurrentMap(corner);
  const std::map<Connection*, Current, Connection::Compare> sorted_current_map(
      current_map.begin(), current_map.end());

  const double dbus = getBlock()->getDbUnitsPerMicron();
  for (const auto& [connection, current] : sorted_current_map) {
    const Node* node0 = connection->getNode0();
    const Node* node1 = connection->getNode1();

    const odb::Point& node0_pt = node0->getPoint();
    const odb::Point& node1_pt = node1->getPoint();

    report << node0->getLayer()->getName() << ",";
    report << fmt::format("{:.4f}", node0_pt.getX() / dbus) << ",";
    report << fmt::format("{:.4f}", node0_pt.getY() / dbus) << ",";
    report << node1->getLayer()->getName() << ",";
    report << fmt::format("{:.4f}", node1_pt.getX() / dbus) << ",";
    report << fmt::format("{:.4f}", node1_pt.getY() / dbus) << ",";
    report << fmt::format("{:.3e}", current) << '\n';
  }
}

void IRSolver::writeSpiceFile(GeneratedSourceType source_type,
                              const std::string& spice_file,
                              sta::Corner* corner,
                              const std::string& voltage_source_file) const
{
  std::ofstream spice(spice_file);
  if (!spice.is_open()) {
    logger_->error(
        utl::PSM, 41, "Could not open {} to write spice file", spice_file);
  }

  const auto res_map = getResistanceMap(corner);

  spice << "* Netlist for " << net_->getName() << " on " << corner->name()
        << "\n\n";

  // Add resistive network
  spice << "* Resistive network\n";
  std::size_t res_num = 0;
  for (const auto& conn : network_->getConnections()) {
    const std::string res_name = fmt::format("R{}", res_num++);

    const std::string resistance
        = fmt::format("{:.6e}", conn->getResistance(res_map));

    spice << res_name << " " << conn->getNode0()->getName() << " "
          << conn->getNode1()->getName() << " R=" << resistance << '\n';
  }

  // Add current sinks
  spice << '\n';
  spice << "* Sinks" << '\n';
  const auto& currents = currents_.at(corner);
  std::size_t current_number = 0;
  for (const auto& node : network_->getITermNodes()) {
    const auto current = currents.at(node.get());
    if (std::abs(current) < spice_file_min_current_) {
      continue;
    }

    spice << "* Sink for " << node->getITerm()->getName() << '\n';

    const std::string current_name = fmt::format("I{}", current_number++);
    const std::string node_current = fmt::format("{:.6e}", current);

    spice << current_name << " " << node->getName() << " 0 DC " << node_current
          << '\n';
  }

  // Add sources
  spice << '\n';
  spice << "* Sources\n";
  std::vector<std::unique_ptr<SourceNode>> src_nodes;
  const Voltage src_voltage = generateSourceNodes(
      source_type, voltage_source_file, corner, src_nodes);

  std::size_t volt_number = 0;
  for (const auto& src_node : src_nodes) {
    auto* node = src_node->getSource();
    const std::string volt_name = fmt::format("V{}", volt_number++);
    const std::string node_voltage = fmt::format("{:.6f}", src_voltage);

    spice << volt_name << " " << node->getName() << " 0 DC " << node_voltage
          << '\n';
  }

  spice << '\n';
  spice << "* Footer\n";
  spice << ".OPTION NUMDGT=6\n";
  spice << ".OP\n";
  spice << ".SAVE TYPE=IC FILE=compare.ic\n";
  spice << ".END\n\n";
}

std::map<Connection*, IRSolver::Current> IRSolver::generateCurrentMap(
    sta::Corner* corner) const
{
  const auto& voltages = voltages_.at(corner);
  std::map<Connection*, IRSolver::Current> currents;
  for (const auto& [connection, cond] : generateConductanceMap(corner)) {
    if (connection->hasITermNode() || connection->hasBPinNode()) {
      continue;
    }

    const Voltage node0_v = voltages.at(connection->getNode0());
    const Voltage node1_v = voltages.at(connection->getNode1());
    const Voltage voltage_drop = std::abs(node0_v - node1_v);
    currents[connection] = voltage_drop * cond;
  }
  return currents;
}

void IRSolver::dumpVector(const Eigen::VectorXd& vector,
                          const std::string& name) const
{
  const std::string report_file = fmt::format("psm_{}.txt", name);
  std::ofstream report(report_file);
  if (!report) {
    logger_->report("Failed to open {} for {}", report_file, name);
    return;
  }
  for (std::size_t i = 0; i < vector.size(); i++) {
    report << fmt::format("{}[{}] = {:.15e}", name, i, vector[i]) << '\n';
  }
}

void IRSolver::dumpMatrix(
    const Eigen::SparseMatrix<Connection::Conductance>& matrix,
    const std::string& name) const
{
  const std::string report_file = fmt::format("psm_{}.txt", name);
  std::ofstream report(report_file);
  if (!report) {
    logger_->report("Failed to open {} for {}", report_file, name);
    return;
  }
  for (int k = 0; k < matrix.outerSize(); k++) {
    for (Eigen::SparseMatrix<Connection::Conductance>::InnerIterator it(matrix,
                                                                        k);
         it;
         ++it) {
      report << fmt::format(
          "{}[{}, {}] = {:.15e}", name, it.col(), it.row(), it.value())
             << '\n';
    }
  }
}

void IRSolver::dumpConductance(
    const std::map<Connection*, Connection::Conductance>& cond,
    const std::string& name) const
{
  const std::string report_file = fmt::format("psm_{}.txt", name);
  std::ofstream report(report_file);
  if (!report) {
    logger_->report("Failed to open {} for {}", report_file, name);
    return;
  }
  const std::map<Connection*, Connection::Conductance, Connection::Compare>
      sorted_cond(cond.begin(), cond.end());

  for (const auto& [connection, cond] : sorted_cond) {
    const Node* node0 = connection->getNode0();
    const Node* node1 = connection->getNode1();
    report << fmt::format(
        "{} -> {}: {:.15e}", node0->describe(""), node1->describe(""), cond)
           << '\n';
  }
}

bool IRSolver::belongsTo(Node* node) const
{
  return network_->belongsTo(node);
}

bool IRSolver::belongsTo(Connection* connection) const
{
  return network_->belongsTo(connection);
}

std::vector<sta::Corner*> IRSolver::getCorners() const
{
  std::vector<sta::Corner*> corners;
  corners.reserve(voltages_.size());

  for (const auto& [corner, nodes] : voltages_) {
    corners.push_back(corner);
  }

  return corners;
}

std::optional<IRSolver::Voltage> IRSolver::getVoltage(sta::Corner* corner,
                                                      Node* node) const
{
  auto find_c = voltages_.find(corner);

  if (find_c == voltages_.end()) {
    return {};
  }

  auto& nodes = find_c->second;
  auto find_n = nodes.find(node);

  if (find_n == nodes.end()) {
    return {};
  }

  return find_n->second;
}

}  // namespace psm
