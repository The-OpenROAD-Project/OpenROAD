// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ir_solver_3d.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "Eigen/Core"
#include "connection.h"
#include "ir_network_3d.h"
#include "ir_solver.h"
#include "odb/db.h"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace psm {

IRSolver3D::IRSolver3D(odb::dbChipNet* chip_net,
                       utl::Logger* logger,
                       est::EstimateParasitics* estimate_parasitics,
                       sta::Scene* corner)
    : logger_(logger),
      estimate_parasitics_(estimate_parasitics),
      corner_(corner),
      network_(std::make_unique<IRNetwork3D>(chip_net, logger))
{
}

void IRSolver3D::addCurrentLoad(odb::dbChipInst* chip_inst,
                                const std::string& terminal,
                                Current current)
{
  if (!std::isfinite(current)) {
    logger_->error(utl::PSM, 144, "Current at {} must be finite.", terminal);
  }
  current_loads_.push_back({chip_inst, terminal, current});
  voltage_vector_.resize(0);
}

void IRSolver3D::addVoltageSource(odb::dbChipInst* chip_inst,
                                  const std::string& terminal,
                                  Voltage voltage)
{
  voltage_sources_.push_back({chip_inst, terminal, voltage});
  voltage_vector_.resize(0);
}

Connection::ResistanceMap IRSolver3D::getResistanceMap() const
{
  Connection::ResistanceMap resistance;
  for (odb::dbNet* net : network_->getNets()) {
    const auto local
        = getLayerResistanceMap(net, corner_, estimate_parasitics_, logger_);
    resistance.insert(local.begin(), local.end());
  }
  for (odb::dbTechLayer* layer : network_->getLayers()) {
    const auto value = resistance.at(layer);
    if ((layer->getType() == odb::dbTechLayerType::ROUTING
         || layer->getType() == odb::dbTechLayerType::CUT)
        && (!std::isfinite(value) || value <= 0.0)) {
      logger_->error(
          utl::PSM,
          107,
          "Layer {} has no positive finite resistance for 3D analysis.",
          layer->getName());
    }
  }
  return resistance;
}

void IRSolver3D::buildCondMatrixAndVoltages()
{
  voltage_vector_.resize(0);
  node_index_.clear();

  std::size_t index = 0;
  for (Node* node : network_->getNodes()) {
    node_index_[node] = index++;
  }

  g_matrix_.resize(node_index_.size(), node_index_.size());
  const Connection::ResistanceMap resistance_map = getResistanceMap();
  IRSolver::NodeConductances node_connections;

  for (Connection* connection : network_->getConnections()) {
    const Connection::Resistance resistance
        = connection->getResistance(resistance_map);
    if (!std::isfinite(resistance) || resistance <= 0.0) {
      logger_->error(utl::PSM,
                     108,
                     "A 3D power-grid connection has non-positive "
                     "resistance {}.",
                     resistance);
    }

    Node* node0 = connection->getNode0();
    Node* node1 = connection->getNode1();
    if (node0 == node1) {
      logger_->error(
          utl::PSM, 109, "A 3D power-grid connection forms a node loop.");
    }

    const Connection::Conductance conductance = 1.0 / resistance;
    node_connections[node0].emplace_back(connection, conductance);
    node_connections[node1].emplace_back(connection, conductance);
  }

  IRSolver::ValueNodeMap<Current> currents(sta_currents_.begin(),
                                           sta_currents_.end());
  for (const CurrentLoad& load : current_loads_) {
    Node* node = network_->findTerminalNode(load.chip_inst, load.terminal);
    if (node == nullptr) {
      logger_->error(
          utl::PSM,
          114,
          "Cannot map current load at port {} on chiplet {} into "
          "the combined 3D network.",
          load.terminal,
          load.chip_inst == nullptr ? "<null>" : load.chip_inst->getName());
    }
    currents[node] += load.current;
  }
  j_vector_ = CurrentVector::Zero(node_index_.size());
  IRSolver::buildCondMatrixAndVoltages(false,
                                       node_connections,
                                       currents,
                                       node_index_,
                                       g_matrix_,
                                       j_vector_,
                                       logger_);
}

void IRSolver3D::addStaLoads(odb::dbNet* net,
                             const odb::PtrMap<odb::dbInst, float>& powers,
                             Voltage power_voltage)
{
  if (!std::isfinite(power_voltage) || power_voltage <= 0.0) {
    logger_->error(utl::PSM,
                   147,
                   "OpenSTA loads on {} require a positive supply voltage.",
                   net->getName());
  }
  const double sign = net->getSigType() == odb::dbSigType::GROUND ? 1.0 : -1.0;
  const auto inst_nodes = network_->getInstanceNodeMapping(net);
  IRSolver::ValueNodeMap<Current> currents;
  const auto instance_powers = IRSolver::buildNodeCurrentMap(
      inst_nodes, powers, power_voltage, currents);
  for (const auto& [inst, power] : instance_powers) {
    if (!std::isfinite(power) || power < 0.0) {
      logger_->error(utl::PSM,
                     148,
                     "OpenSTA reported invalid power for {}/{}.",
                     net->getBlock()->getName(),
                     inst->getName());
    }
    total_power_ += power;
    for (Node* node : inst_nodes.at(inst)) {
      sta_currents_[node] = sign * currents.at(node);
    }
  }
  nominal_voltages_[net] = power_voltage;
  if (instance_powers.empty()) {
    logger_->error(utl::PSM,
                   149,
                   "No cells with OpenSTA power models are connected to {}/{}.",
                   net->getBlock()->getName(),
                   net->getName());
  }
  voltage_vector_.resize(0);
}

bool IRSolver3D::checkCurrentVector() const
{
  constexpr double kTolerance = 1.0e-12;
  if (static_cast<std::size_t>(j_vector_.size()) != node_index_.size()) {
    return false;
  }

  CurrentVector expected = CurrentVector::Zero(node_index_.size());
  Current expected_total = 0.0;
  for (const auto& [node, current] : sta_currents_) {
    if (!std::isfinite(current)) {
      return false;
    }
    expected[node_index_.at(node)] += current;
    expected_total += current;
  }
  for (const CurrentLoad& load : current_loads_) {
    if (!std::isfinite(load.current)) {
      return false;
    }
    Node* node = network_->findTerminalNode(load.chip_inst, load.terminal);
    if (node == nullptr) {
      return false;
    }
    expected[node_index_.at(node)] += load.current;
    expected_total += load.current;
  }

  for (Eigen::Index index = 0; index < j_vector_.size(); index++) {
    if (!std::isfinite(j_vector_[index])
        || std::abs(j_vector_[index] - expected[index]) > kTolerance) {
      return false;
    }
  }
  return std::abs(getTotalCurrent() - expected_total) <= kTolerance;
}

bool IRSolver3D::buildSourceMap(std::map<std::size_t, Voltage>& sources) const
{
  sources.clear();
  for (const VoltageSource& source : voltage_sources_) {
    if (!std::isfinite(source.voltage)
        || source.voltage != voltage_sources_.front().voltage) {
      return false;
    }
    Node* node = network_->findTerminalNode(source.chip_inst, source.terminal);
    if (node == nullptr) {
      return false;
    }
    const std::size_t index = node_index_.at(node);
    sources.emplace(index, source.voltage);
  }
  return !sources.empty();
}

bool IRSolver3D::hasValidSources() const
{
  std::map<std::size_t, Voltage> sources;
  return buildSourceMap(sources);
}

bool IRSolver3D::solve()
{
  voltage_vector_.resize(0);
  if (!check() || !checkCurrentVector()) {
    return false;
  }

  std::map<std::size_t, Voltage> sources;
  if (!buildSourceMap(sources)) {
    return false;
  }

  const std::size_t node_count = node_index_.size();
  const std::size_t matrix_size = node_count + sources.size();
  ConductanceMatrix matrix = g_matrix_;
  matrix.conservativeResize(matrix_size, matrix_size);
  CurrentVector currents = CurrentVector::Zero(matrix_size);
  currents.head(node_count) = j_vector_;

  std::vector<IRSolver::MatrixSource> matrix_sources;
  for (const auto& [node_index, voltage] : sources) {
    matrix_sources.push_back(
        {node_count + matrix_sources.size(), node_index, voltage});
  }
  IRSolver::addSourcesToMatrixAndVoltages(
      matrix_sources, matrix, currents, logger_);
  voltage_vector_ = IRSolver::solve(matrix, currents, logger_, node_index_)
                        .head(node_count);
  if (!checkSolution()) {
    voltage_vector_.resize(0);
    return false;
  }
  // Report the prescribed source voltages exactly after validating the solve.
  for (const auto& [node_index, voltage] : sources) {
    voltage_vector_[node_index] = voltage;
  }
  return true;
}

bool IRSolver3D::checkSolution() const
{
  constexpr double kTolerance = 1.0e-8;
  if (!hasSolution()) {
    return false;
  }

  std::map<std::size_t, Voltage> sources;
  if (!buildSourceMap(sources)) {
    return false;
  }
  for (Eigen::Index index = 0; index < voltage_vector_.size(); index++) {
    if (!std::isfinite(voltage_vector_[index])) {
      return false;
    }
  }
  for (const auto& [source_index, source_voltage] : sources) {
    if (std::abs(voltage_vector_[source_index] - source_voltage) > kTolerance) {
      return false;
    }
  }

  const CurrentVector calculated_current = g_matrix_ * voltage_vector_;
  for (Eigen::Index index = 0; index < calculated_current.size(); index++) {
    if (sources.contains(index)) {
      continue;
    }
    const double scale = std::max(
        {1.0, std::abs(calculated_current[index]), std::abs(j_vector_[index])});
    if (std::abs(calculated_current[index] - j_vector_[index])
        > kTolerance * scale) {
      return false;
    }
  }
  return true;
}

std::optional<IRSolver3D::Voltage> IRSolver3D::getVoltage(
    odb::dbChipInst* chip_inst,
    const std::string& terminal) const
{
  if (!hasSolution()) {
    return std::nullopt;
  }
  Node* node = network_->findTerminalNode(chip_inst, terminal);
  if (node == nullptr) {
    return std::nullopt;
  }
  const auto index = node_index_.find(node);
  if (index == node_index_.end()) {
    return std::nullopt;
  }
  return voltage_vector_[index->second];
}

bool IRSolver3D::check() const
{
  constexpr double kTolerance = 1.0e-9;
  if (static_cast<std::size_t>(g_matrix_.rows()) != node_index_.size()
      || static_cast<std::size_t>(g_matrix_.cols()) != node_index_.size()) {
    return false;
  }

  std::vector<double> row_sums(g_matrix_.rows(), 0.0);
  for (Eigen::Index column = 0; column < g_matrix_.outerSize(); column++) {
    for (ConductanceMatrix::InnerIterator value(g_matrix_, column); value;
         ++value) {
      if (!std::isfinite(value.value())) {
        return false;
      }
      if (std::abs(value.value() - g_matrix_.coeff(value.col(), value.row()))
          > kTolerance * std::max(1.0, std::abs(value.value()))) {
        return false;
      }
      row_sums[value.row()] += value.value();
    }
  }

  for (std::size_t row = 0; row < row_sums.size(); row++) {
    const double diagonal = std::abs(g_matrix_.coeff(row, row));
    if (diagonal == 0.0
        || std::abs(row_sums[row]) > kTolerance * std::max(1.0, diagonal)) {
      return false;
    }
  }

  std::map<std::pair<std::size_t, std::size_t>, Connection::Conductance>
      interdie_conductance;
  const Connection::ResistanceMap empty_resistance_map;
  for (const auto& connection : network_->getInterDieConnections()) {
    std::size_t node0 = node_index_.at(connection->getNode0());
    std::size_t node1 = node_index_.at(connection->getNode1());
    if (node1 < node0) {
      std::swap(node0, node1);
    }
    const Connection::Resistance resistance
        = connection->getResistance(empty_resistance_map);
    interdie_conductance[{node0, node1}] += 1.0 / resistance;
  }

  for (const auto& [nodes, conductance] : interdie_conductance) {
    const double matrix_value = g_matrix_.coeff(nodes.first, nodes.second);
    if (std::abs(matrix_value + conductance)
        > kTolerance * std::max(1.0, conductance)) {
      return false;
    }
  }

  return !node_index_.empty() && !interdie_conductance.empty()
         && network_->isConnected();
}

void IRSolver3D::report() const
{
  IRSolver::Results results;
  results.net_voltage = voltage_sources_.front().voltage;
  results.total_power = total_power_;
  results.worst_voltage = results.net_voltage;
  const bool is_ground
      = network_->getNets().front()->getSigType() == odb::dbSigType::GROUND;

  odb::PtrMap<odb::dbChipInst, Voltage> reference_voltages;
  for (const auto& [chiplet, net] : network_->getChipletNets()) {
    const auto nominal = nominal_voltages_.find(net);
    reference_voltages[chiplet]
        = is_ground && nominal != nominal_voltages_.end() ? nominal->second
                                                          : results.net_voltage;
  }

  double total_voltage = 0.0;
  std::size_t count = 0;
  // Match 2D statistics: use routing nodes, excluding terminal attachment
  // nodes.
  for (const auto& [node, chiplet] : network_->getNodeChiplets()) {
    if (dynamic_cast<TerminalNode*>(node) != nullptr) {
      continue;
    }
    const Voltage voltage = voltage_vector_[node_index_.at(node)];
    total_voltage += voltage;
    ++count;
    results.worst_voltage = is_ground
                                ? std::max(results.worst_voltage, voltage)
                                : std::min(results.worst_voltage, voltage);
    const Voltage drop = is_ground ? voltage - results.net_voltage
                                   : results.net_voltage - voltage;
    const Voltage reference = reference_voltages.at(chiplet);
    if (reference > 0.0) {
      results.max_percent = std::max(
          results.max_percent, static_cast<float>(100.0 * drop / reference));
    } else {
      results.max_percent = std::numeric_limits<float>::quiet_NaN();
    }
  }
  results.avg_voltage = total_voltage / count;
  results.worst_ir_drop = is_ground
                              ? results.worst_voltage - results.net_voltage
                              : results.net_voltage - results.worst_voltage;
  results.avg_ir_drop = is_ground ? results.avg_voltage - results.net_voltage
                                  : results.net_voltage - results.avg_voltage;
  IRSolver::report(
      results, network_->getChipNet()->getName(), corner_, logger_);
}

void IRSolver3D::reportEM() const
{
  IRSolver::EMResults results;
  const auto resistance_map = getResistanceMap();
  Current total_current = 0.0;
  for (Connection* connection : network_->getConnections()) {
    // Exclude local terminal attachments as in 2D, but retain the bond
    // resistors.
    if ((connection->hasITermNode() || connection->hasBPinNode())
        && dynamic_cast<FixedResistanceConnection*>(connection) == nullptr) {
      continue;
    }
    const Voltage voltage0
        = voltage_vector_[node_index_.at(connection->getNode0())];
    const Voltage voltage1
        = voltage_vector_[node_index_.at(connection->getNode1())];
    const Current current = std::abs(voltage0 - voltage1)
                            / connection->getResistance(resistance_map);
    results.max_current = std::max(results.max_current, current);
    total_current += current;
    ++results.resistors;
  }
  if (results.resistors > 0) {
    results.avg_current = total_current / results.resistors;
  }
  IRSolver::reportEM(
      results, network_->getChipNet()->getName(), corner_, logger_);
}

void IRSolver3D::writeInstanceVoltageFile(const std::string& voltage_file) const
{
  std::ofstream report(voltage_file);
  if (!report) {
    logger_->error(
        utl::PSM, 155, "Unable to open 3D voltage file: {}", voltage_file);
  }
  writeVoltageHeader(report);
  for (const auto& [chiplet, node] : network_->getITermNodes()) {
    writeVoltageRow(report,
                    node,
                    voltage_vector_[node_index_.at(node)],
                    std::string(chiplet->getName()) + "/");
  }
}

}  // namespace psm
