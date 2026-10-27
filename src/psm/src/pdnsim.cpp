// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "psm/pdnsim.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "debug_gui.h"
#include "dpl/Opendp.h"
#include "heatMap.h"
#include "ir_network.h"
#include "ir_solver.h"
#include "node.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "shape.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "web/core.h"
#include "web/heatMap.h"

using odb::dbBlock;
using odb::dbSigType;

namespace psm {

PDNSim::PDNSim(utl::Logger* logger,
               odb::dbDatabase* db,
               sta::dbSta* sta,
               est::EstimateParasitics* estimate_parasitics,
               dpl::Opendp* opendp)
{
  db_ = db;
  sta_ = sta;
  estimate_parasitics_ = estimate_parasitics;
  opendp_ = opendp;
  logger_ = logger;
  heatmap_source_ = web::registerHeatMapSource(
      "IR Drop", "IRDrop", "IRDrop", [this, sta, logger]() {
        return std::make_shared<IRDropDataSource>(this, sta, logger);
      });
}

PDNSim::~PDNSim() = default;

void PDNSim::setInterchipResistance(sta::Scene* scene, double resistance)
{
  if (!std::isfinite(resistance) || resistance <= 0.0) {
    logger_->error(utl::PSM,
                   171,
                   "Inter-chip resistance must be positive and finite: {}.",
                   resistance);
  }
  interchip_resistances_[scene] = resistance;
  clearSolvers();
}

void PDNSim::setDebugGui(bool enable)
{
  debug_gui_enabled_ = enable;

  for (const auto& [net, solver] : solvers_) {
    solver->enableGui(debug_gui_enabled_);
  }

  web::Gui::get()->registerDescriptor<Node*>(new NodeDescriptor(solvers_));
  web::Gui::get()->registerDescriptor<ITermNode*>(
      new ITermNodeDescriptor(solvers_));
  web::Gui::get()->registerDescriptor<BPinNode*>(
      new BPinNodeDescriptor(solvers_));
  web::Gui::get()->registerDescriptor<Connection*>(
      new ConnectionDescriptor(solvers_));
}

void PDNSim::setNetVoltage(odb::dbNet* net, sta::Scene* corner, double voltage)
{
  auto& voltages = user_voltages_[net];
  voltages[corner] = voltage;
}

void PDNSim::setInstPower(odb::dbInst* inst, sta::Scene* corner, float power)
{
  auto& powers = user_powers_[inst];
  powers[corner] = power;
}

void PDNSim::analyzePowerGrid(odb::dbNet* net,
                              sta::Scene* corner,
                              GeneratedSourceType source_type,
                              const std::string& voltage_file,
                              bool use_prev_solution,
                              bool enable_em,
                              const std::string& em_file,
                              const std::string& error_file,
                              const std::string& voltage_source_file)
{
  if (!sta::dbNetwork::is3DicTopChip(db_->getChip())) {
    if (!checkConnectivity(net, false, error_file, false)) {
      return;
    }

    last_net_ = net;
    last_corner_ = corner;
    auto* solver = getIRSolver(net, false);
    if (!use_prev_solution || !solver->hasSolution(corner)) {
      solver->solve(corner, source_type, voltage_source_file);
    } else {
      logger_->info(utl::PSM, 11, "Reusing previous solution");
    }
    solver->report(corner);

    if (heatmap_source_) {
      heatmap_source_->invalidateInstances();
    }

    if (enable_em) {
      solver->reportEM(corner);
      solver->writeEMFile(em_file, corner);
    }

    solver->writeInstanceVoltageFile(voltage_file, corner);
    return;
  }

  if (!sta_->getDbNetwork()->has3DicChip()
      || sta_->getDbNetwork()->topChip() != db_->getChip()) {
    logger_->error(utl::PSM, 151, "The loaded assembly has no timing network.");
  }

  auto* chip_net = findChipNet(net);

  auto& solver = assembly_solvers_[chip_net][corner];
  if (solver == nullptr) {
    solver = makeAssemblySolver(chip_net, corner);
    for (auto* local_net : solver->getNetwork()->getNets()) {
      addOwner(local_net->getBlock());
    }
  }
  checkAssemblyConnectivity(*solver->getNetwork(), false, false, error_file);
  if (!use_prev_solution || !solver->hasSolution(corner)) {
    solver->solve(corner, source_type, voltage_source_file);
  } else {
    logger_->info(utl::PSM, 13, "Reusing previous solution");
  }
  solver->report(corner);
  if (enable_em) {
    solver->reportEM(corner);
    solver->writeEMFile(em_file, corner);
  }
  if (!voltage_file.empty()) {
    solver->writeInstanceVoltageFile(voltage_file, corner);
  }
}

bool PDNSim::checkConnectivity(odb::dbNet* net,
                               bool floorplanning,
                               const std::string& error_file,
                               bool require_bterm)
{
  if (sta::dbNetwork::is3DicTopChip(db_->getChip())) {
    auto* chip_net = findChipNet(net);
    IRNetwork network(chip_net, logger_, floorplanning, std::nullopt, true);
    checkAssemblyConnectivity(
        network, floorplanning, require_bterm, error_file);
    debugPrint(logger_,
               utl::PSM,
               "stats",
               1,
               "Built power grid {}: {} chiplet power grids, {} nodes, and {} "
               "connections ({} inter-chip bonds).",
               chip_net->getName(),
               network.getChipletNetworkCount(),
               network.getNodeCount(),
               network.getConnectionCount(),
               network.getInterDieConnectionCount());
    return true;
  }

  auto* solver = getIRSolver(net, floorplanning);
  const bool check = solver->check(require_bterm, !floorplanning);
  solver->writeErrorFile(error_file);

  if (debug_gui_enabled_) {
    solver->enableGui(true);
  }

  if (logger_->debugCheck(utl::PSM, "stats", 1)) {
    solver->getNetwork()->reportStats();
  }

  if (check) {
    logger_->info(
        utl::PSM, 40, "All shapes on net {} are connected.", net->getName());
  } else {
    logger_->error(
        utl::PSM, 69, "Check connectivity failed on {}.", net->getName());
  }
  return check;
}

odb::dbChipNet* PDNSim::findChipNet(const std::string& chip_net_name) const
{
  odb::dbChip* top_chip = db_->getChip();
  if (top_chip == nullptr) {
    logger_->error(utl::PSM, 101, "No chip is loaded.");
  }

  odb::dbChipNet* chip_net = nullptr;
  for (odb::dbChipNet* candidate : top_chip->getChipNets()) {
    if (candidate->getName() == chip_net_name) {
      chip_net = candidate;
      break;
    }
  }
  if (chip_net == nullptr) {
    logger_->error(utl::PSM,
                   102,
                   "The loaded assembly does not contain net {}.",
                   chip_net_name);
  }
  return chip_net;
}

odb::dbChipNet* PDNSim::findChipNet(odb::dbNet* local_net) const
{
  for (auto* candidate : db_->getChip()->getChipNets()) {
    for (uint32_t index = 0; index < candidate->getNumBumpInsts(); ++index) {
      std::vector<odb::dbChipInst*> path;
      auto* bump = candidate->getBumpInst(index, path);
      if (bump != nullptr && bump->getChipBump() != nullptr
          && bump->getChipBump()->getNet() == local_net) {
        return candidate;
      }
    }
  }
  logger_->error(utl::PSM,
                 105,
                 "Cannot find an assembly net containing local net {}.",
                 local_net->getName());
  return nullptr;
}

std::unique_ptr<IRSolver> PDNSim::makeAssemblySolver(odb::dbChipNet* net,
                                                     sta::Scene* scene)
{
  const auto resistance = interchip_resistances_.find(scene);
  auto network = std::make_unique<IRNetwork>(
      net,
      logger_,
      false,
      resistance == interchip_resistances_.end()
          ? std::nullopt
          : std::optional<double>(resistance->second));
  return std::make_unique<IRSolver>(std::move(network),
                                    sta_,
                                    estimate_parasitics_,
                                    logger_,
                                    user_voltages_,
                                    user_powers_,
                                    generated_source_settings_);
}

void PDNSim::checkAssemblyConnectivity(IRNetwork& network,
                                       bool floorplanning,
                                       bool require_bterm,
                                       const std::string& error_file)
{
  std::ofstream report;
  if (!error_file.empty()) {
    report.open(error_file);
    if (!report) {
      logger_->error(
          utl::PSM, 152, "Unable to open error file: {}", error_file);
    }
  }
  const auto name = network.getChipNet()->getName();
  if (network.getChipletNetworkCount() < 2
      || network.getInterDieConnectionCount() == 0) {
    if (report.is_open()) {
      report << "Net " << name << ": " << network.getChipletNetworkCount()
             << " chiplet power grids and "
             << network.getInterDieConnectionCount() << " inter-chip bonds.\n";
    }
    logger_->error(
        utl::PSM,
        103,
        "Power grid {} has {} chiplet power grids and {} inter-chip bonds.",
        name,
        network.getChipletNetworkCount(),
        network.getInterDieConnectionCount());
  }

  // Reuse the existing 2D connectivity checks inside each chiplet.
  bool local_ok = true;
  for (const auto& [chiplet, net] : network.getChipletNets()) {
    auto local_network
        = std::make_unique<IRNetwork>(net, logger_, floorplanning);
    IRSolver local(std::move(local_network),
                   sta_,
                   estimate_parasitics_,
                   logger_,
                   user_voltages_,
                   user_powers_,
                   generated_source_settings_);
    if (report.is_open()) {
      report << "Chiplet " << chiplet->getName() << ", net " << net->getName()
             << " (coordinates relative to the assembly origin in microns)\n";
    }
    try {
      if (!local.check(
              require_bterm, !floorplanning, chiplet->getTransform())) {
        local_ok = false;
      }
    } catch (const std::runtime_error& error) {
      local_ok = false;
      if (report.is_open()) {
        report << error.what() << '\n';
      }
    }
    if (report.is_open()) {
      local.writeErrorFile(report);
    }
  }
  const auto disconnected = network.getDisconnectedNodes();
  if (report.is_open() && !disconnected.empty()) {
    report << "The power grid for " << name << " is disconnected.\n";
  }
  if (!local_ok) {
    logger_->error(utl::PSM,
                   153,
                   "A chiplet power grid on {} failed the local checks.",
                   name);
  }
  if (!disconnected.empty() || network.getNodeCount() == 0) {
    logger_->error(
        utl::PSM, 140, "The power grid for {} is disconnected.", name);
  }
  logger_->info(utl::PSM, 154, "The power grid for {} is connected.", name);
}

void PDNSim::writeSpiceNetwork(odb::dbNet* net,
                               sta::Scene* corner,
                               GeneratedSourceType source_type,
                               const std::string& spice_file,
                               const std::string& voltage_source_file)
{
  auto* solver = getIRSolver(net, false);
  solver->writeSpiceFile(source_type, spice_file, corner, voltage_source_file);
}

psm::IRSolver* PDNSim::getIRSolver(odb::dbNet* net, bool floorplanning)
{
  auto& solver = solvers_[net];
  if (solver == nullptr) {
    solver = std::make_unique<IRSolver>(net,
                                        floorplanning,
                                        sta_,
                                        estimate_parasitics_,
                                        logger_,
                                        user_voltages_,
                                        user_powers_,
                                        generated_source_settings_);
    addOwner(net->getBlock());
  }

  return solver.get();
}

void PDNSim::getIRDropForLayer(odb::dbNet* net,
                               odb::dbTechLayer* layer,
                               IRDropByPoint& ir_drop) const
{
  auto find_solver = solvers_.find(net);
  if (last_corner_ == nullptr || find_solver == solvers_.end()) {
    return;
  }
  ir_drop = find_solver->second->getIRDrop(layer, last_corner_);
}

void PDNSim::getIRDropForLayer(odb::dbNet* net,
                               sta::Scene* corner,
                               odb::dbTechLayer* layer,
                               IRDropByPoint& ir_drop) const
{
  auto find_solver = solvers_.find(net);
  if (find_solver == solvers_.end()) {
    return;
  }
  ir_drop = find_solver->second->getIRDrop(layer, corner);
}

void PDNSim::setGeneratedSourceSettings(const GeneratedSourceSettings& settings)
{
  if (settings.bump_dx > 0) {
    generated_source_settings_.bump_dx = settings.bump_dx;
  }
  if (settings.bump_dy > 0) {
    generated_source_settings_.bump_dy = settings.bump_dy;
  }
  if (settings.bump_interval > 0) {
    generated_source_settings_.bump_interval = settings.bump_interval;
  }
  if (settings.bump_size > 0) {
    generated_source_settings_.bump_size = settings.bump_size;
  }
  if (settings.strap_track_pitch > 0) {
    generated_source_settings_.strap_track_pitch = settings.strap_track_pitch;
  }
  if (settings.resistance > 0) {
    generated_source_settings_.resistance = settings.resistance;
  }
}

void PDNSim::clearSolvers()
{
  solvers_.clear();
  assembly_solvers_.clear();
}

void PDNSim::inDbPostMoveInst(odb::dbInst*)
{
  clearSolvers();
}

void PDNSim::inDbNetDestroy(odb::dbNet*)
{
  clearSolvers();
}

void PDNSim::inDbBTermPostConnect(odb::dbBTerm*)
{
  clearSolvers();
}

void PDNSim::inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*)
{
  clearSolvers();
}

void PDNSim::inDbBPinCreate(odb::dbBPin*)
{
  clearSolvers();
}

void PDNSim::inDbBPinAddBox(odb::dbBox*)
{
  clearSolvers();
}

void PDNSim::inDbBPinRemoveBox(odb::dbBox*)
{
  clearSolvers();
}

void PDNSim::inDbBPinDestroy(odb::dbBPin*)
{
  clearSolvers();
}

void PDNSim::inDbSWireAddSBox(odb::dbSBox*)
{
  clearSolvers();
}

void PDNSim::inDbSWireRemoveSBox(odb::dbSBox*)
{
  clearSolvers();
}

void PDNSim::inDbSWirePostDestroySBoxes(odb::dbSWire*)
{
  clearSolvers();
}

void PDNSim::inDbFillCreate(odb::dbFill*)
{
  clearSolvers();
}

// Functions of decap cells
void PDNSim::addDecapMaster(odb::dbMaster* decap_master, double decap_cap)
{
  opendp_->addDecapMaster(decap_master, decap_cap);
}

// Return the lowest layer of db_net route
odb::dbTechLayer* PDNSim::getLowestLayer(odb::dbNet* db_net)
{
  int min_layer_level = std::numeric_limits<int>::max();
  std::vector<odb::dbShape> via_boxes;
  for (odb::dbSWire* swire : db_net->getSWires()) {
    for (odb::dbSBox* s : swire->getWires()) {
      if (!s->isVia()) {
        odb::dbTechLayer* tech_layer = s->getTechLayer();
        min_layer_level
            = std::min(min_layer_level, tech_layer->getRoutingLevel());
      }
    }
  }
  return db_->getTech()->findRoutingLayer(min_layer_level);
}

odb::dbNet* PDNSim::findPowerNet(const char* net_name)
{
  dbBlock* block = db_->getChip()->getBlock();
  odb::dbNet* power_net = nullptr;
  // If net name is defined by user
  if (!std::string(net_name).empty()) {
    power_net = block->findNet(net_name);
    if (power_net == nullptr) {
      logger_->error(
          utl::PSM, 48, "Cannot find net {} in the design.", net_name);
    }
    // Check if net is supply
    if (!power_net->getSigType().isSupply()) {
      logger_->error(
          utl::PSM, 47, "{} is not a supply net.", power_net->getName());
    }
    return power_net;
  }
  // Otherwise find power net
  for (auto db_net : block->getNets()) {
    if (db_net->getSigType().isSupply()
        && db_net->getSigType() == dbSigType::POWER) {
      power_net = db_net;
      break;
    }
  }
  return power_net;
}

void PDNSim::insertDecapCells(double target, const char* net_name)
{
  // Get db_net
  odb::dbNet* db_net = findPowerNet(net_name);

  // Get lowest layer
  odb::dbTechLayer* tech_layer = getLowestLayer(db_net);

  IRDropByPoint ir_drops;
  getIRDropForLayer(db_net, tech_layer, ir_drops);

  if (ir_drops.empty()) {
    logger_->error(utl::PSM,
                   93,
                   "No IR drop data found. Run analyse_power_grid for net {} "
                   "before inserting decap cells.",
                   db_net->getName());
  }

  // call DPL to insert decap cells
  opendp_->insertDecapCells(target, ir_drops);
}

}  // namespace psm
