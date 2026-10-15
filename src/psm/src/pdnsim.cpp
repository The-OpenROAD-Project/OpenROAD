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
#include "rcx/interChipModel.h"
#include "shape.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "web/core.h"
#include "web/heatMap.h"

using odb::dbBlock;
using odb::dbSigType;

namespace psm {

// ODB's block observer can own only one block. Keep a separate observer for
// each chiplet so editing Chip A cannot leave a solution cached for Chip B.
class PDNSim::BlockObserver3D : public odb::dbBlockCallBackObj
{
 public:
  BlockObserver3D(PDNSim* owner, odb::dbBlock* block)
      : owner_(owner), block_(block)
  {
    addOwner(block);
  }

  bool observes(odb::dbBlock* block) const
  {
    return hasOwner() && block_ == block;
  }

  void inDbPostMoveInst(odb::dbInst*) override;
  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstDestroy(odb::dbInst*) override;
  void inDbInstPlacementStatusBefore(odb::dbInst*,
                                     const odb::dbPlacementStatus&) override;
  void inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*) override;
  void inDbITermPreDisconnect(odb::dbITerm*) override;
  void inDbITermPostConnect(odb::dbITerm*) override;
  void inDbNetDestroy(odb::dbNet*) override;
  void inDbBTermPostConnect(odb::dbBTerm*) override;
  void inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*) override;
  void inDbBPinCreate(odb::dbBPin*) override;
  void inDbBPinAddBox(odb::dbBox*) override;
  void inDbBPinRemoveBox(odb::dbBox*) override;
  void inDbBPinDestroy(odb::dbBPin*) override;
  void inDbSWireAddSBox(odb::dbSBox*) override;
  void inDbSWireRemoveSBox(odb::dbSBox*) override;
  void inDbSWirePostDestroySBoxes(odb::dbSWire*) override;
  void inDbFillCreate(odb::dbFill*) override;

 private:
  PDNSim* owner_;
  odb::dbBlock* block_;
};

void PDNSim::observe3DBlock(odb::dbBlock* block)
{
  std::erase_if(block_observers_3d_,
                [](const auto& observer) { return !observer->hasOwner(); });
  const auto found = std::ranges::find_if(
      block_observers_3d_,
      [block](const auto& observer) { return observer->observes(block); });
  if (found == block_observers_3d_.end()) {
    block_observers_3d_.push_back(
        std::make_unique<BlockObserver3D>(this, block));
  }
}

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

void PDNSim::setAssemblyRulesFile(const std::string& rules_file)
{
  assembly_bond_resistance_
      = rcx::parseInterChipRules(rules_file, logger_).resistance;
  solvers_3d_.clear();
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
}

bool PDNSim::checkConnectivity(odb::dbNet* net,
                               bool floorplanning,
                               const std::string& error_file,
                               bool require_bterm)
{
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
    logger_->error(utl::PSM, 101, "No top-level dbChip is loaded.");
  }

  odb::dbChipNet* chip_net = nullptr;
  for (odb::dbChipNet* candidate : top_chip->getChipNets()) {
    if (candidate->getName() == chip_net_name) {
      chip_net = candidate;
      break;
    }
  }
  if (chip_net == nullptr) {
    logger_->error(utl::PSM, 102, "Cannot find dbChipNet {}.", chip_net_name);
  }
  return chip_net;
}

odb::dbChipInst* PDNSim::find3DChip(odb::dbChipNet* chip_net,
                                    const std::string& chip_name,
                                    const std::string& terminal) const
{
  auto* chip = db_->getChip()->findChipInst(chip_name.c_str());
  if (chip == nullptr || chip->getMasterChip() == nullptr
      || chip->getMasterChip()->getBlock() == nullptr) {
    logger_->error(utl::PSM, 116, "Cannot find leaf chiplet {}.", chip_name);
  }
  auto* block = chip->getMasterChip()->getBlock();
  odb::dbNet* net = nullptr;
  if (auto* port = block->findBTerm(terminal.c_str())) {
    net = port->getNet();
  } else {
    const auto separator = terminal.rfind('/');
    if (separator != std::string::npos) {
      if (auto* inst = block->findInst(terminal.substr(0, separator).c_str())) {
        if (auto* pin
            = inst->findITerm(terminal.substr(separator + 1).c_str())) {
          net = pin->getNet();
        }
      }
    }
  }
  if (net == nullptr) {
    logger_->error(utl::PSM,
                   117,
                   "Cannot find connected terminal {} on chiplet {}.",
                   terminal,
                   chip_name);
  }
  for (uint32_t index = 0; index < chip_net->getNumBumpInsts(); ++index) {
    std::vector<odb::dbChipInst*> path;
    auto* bump = chip_net->getBumpInst(index, path);
    if (bump != nullptr && path.size() == 1 && path.front() == chip
        && bump->getChipBump()->getNet() == net) {
      return chip;
    }
  }
  logger_->error(utl::PSM,
                 118,
                 "Terminal {}/{} is not on assembly net {}.",
                 chip_name,
                 terminal,
                 chip_net->getName());
}

std::unique_ptr<IRSolver> PDNSim::make3DSolver(odb::dbChipNet* net,
                                               sta::Scene* corner,
                                               bool use_sta)
{
  auto network = std::make_unique<IRNetwork>(
      net, logger_, false, assembly_bond_resistance_);
  auto solver = std::make_unique<IRSolver>(std::move(network),
                                           sta_,
                                           estimate_parasitics_,
                                           logger_,
                                           user_voltages_,
                                           user_powers_,
                                           generated_source_settings_,
                                           corner);
  const auto loads = user_currents_3d_.find(net->getName());
  if (use_sta && loads != user_currents_3d_.end() && !loads->second.empty()) {
    logger_->error(utl::PSM,
                   150,
                   "Explicit test currents are set on {}. Clear the test setup "
                   "before analyzing with OpenSTA.",
                   net->getName());
  }
  if (use_sta
      && (!sta_->getDbNetwork()->has3DicChip()
          || sta_->getDbNetwork()->topChip() != db_->getChip())) {
    logger_->error(utl::PSM,
                   151,
                   "OpenSTA has no timing network for the loaded 3D assembly.");
  }
  if (loads != user_currents_3d_.end()) {
    for (const auto& load : loads->second) {
      auto* chip = find3DChip(net, load.chip_name, load.terminal);
      solver->addCurrentLoad(chip, load.terminal, load.current);
    }
  }
  const auto sources = user_voltage_sources_3d_.find(net->getName());
  if (sources != user_voltage_sources_3d_.end()) {
    for (const auto& source : sources->second) {
      if (source.corner != corner) {
        continue;
      }
      auto* chip = find3DChip(net, source.chip_name, source.terminal);
      solver->addVoltageSource(chip, source.terminal, source.voltage);
    }
  }
  if (use_sta) {
    auto powers = getInstancePower(sta_, corner, logger_);
    // Apply saved power only in the 3D path. Keep legacy 2D behavior unchanged.
    for (const auto& [inst, overrides] : user_powers_) {
      auto power = overrides.find(corner);
      if (power == overrides.end()) {
        power = overrides.find(nullptr);
      }
      if (power != overrides.end()) {
        powers[inst] = power->second;
      }
    }
    for (auto* local_net : solver->getNetwork()->getNets()) {
      // Read the shared voltage settings without building another network.
      const double voltage
          = getNominalVoltage(local_net, corner, sta_, user_voltages_, logger_);
      solver->addStaLoads(local_net, powers, voltage);
    }
  }
  solver->buildAssemblySystem();
  for (auto* local_net : solver->getNetwork()->getNets()) {
    observe3DBlock(local_net->getBlock());
  }
  return solver;
}

void PDNSim::check3DConnectivity(IRNetwork& network,
                                 bool floorplanning,
                                 bool require_bterm,
                                 const std::string& error_file)
{
  std::ofstream report;
  if (!error_file.empty()) {
    report.open(error_file);
    if (!report) {
      logger_->error(
          utl::PSM, 152, "Unable to open 3D error file: {}", error_file);
    }
  }
  const auto name = network.getChipNet()->getName();
  if (network.getChipletNetworkCount() < 2
      || network.getInterDieConnectionCount() == 0) {
    if (report.is_open()) {
      report << "Net " << name << ": " << network.getChipletNetworkCount()
             << " chiplet PDNs and " << network.getInterDieConnectionCount()
             << " inter-die connections.\n";
    }
    logger_->error(
        utl::PSM,
        103,
        "3D power grid {} has {} chiplet PDNs and {} inter-die connections.",
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
             << " (coordinates in local microns)\n";
    }
    try {
      if (!local.check(require_bterm, !floorplanning)) {
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
  if (report.is_open()) {
    const auto owners = network.getNodeChiplets();
    std::vector<std::string> descriptions;
    for (Node* node : disconnected) {
      descriptions.push_back(node->describe(
          std::string("Disconnected ") + owners.at(node)->getName() + "/"));
    }
    std::ranges::sort(descriptions);
    for (const auto& description : descriptions) {
      report << description << '\n';
    }
  }
  if (!local_ok) {
    logger_->error(utl::PSM,
                   153,
                   "A chiplet power grid on {} failed the local checks.",
                   name);
  }
  if (!disconnected.empty() || network.getNodeCount() == 0) {
    logger_->error(
        utl::PSM, 140, "The 3D power grid for {} is disconnected.", name);
  }
}

bool PDNSim::check3DPowerGrid(const std::string& chip_net_name,
                              bool floorplanning,
                              const std::string& error_file,
                              bool require_bterm)
{
  IRNetwork network(findChipNet(chip_net_name),
                    logger_,
                    floorplanning,
                    assembly_bond_resistance_);
  check3DConnectivity(network, floorplanning, require_bterm, error_file);
  logger_->info(utl::PSM,
                104,
                "Built 3D power grid {}: {} chiplet PDNs, {} nodes, and {} "
                "connections ({} inter-die).",
                chip_net_name,
                network.getChipletNetworkCount(),
                network.getNodeCount(),
                network.getConnectionCount(),
                network.getInterDieConnectionCount());
  return true;
}

bool PDNSim::check3DGMatrix(const std::string& chip_net_name)
{
  // Matrix-only checks do not require an OpenSTA power calculation.
  auto solver
      = make3DSolver(findChipNet(chip_net_name), sta_->cmdScene(), false);
  if (!solver->checkAssemblySystem()) {
    logger_->error(
        utl::PSM,
        110,
        "The combined 3D G matrix for {} is invalid or disconnected.",
        chip_net_name);
  }
  logger_->info(utl::PSM,
                111,
                "Built combined 3D G matrix {}: {} nodes, {} nonzero entries, "
                "and {} inter-die connections.",
                chip_net_name,
                solver->getNodeCount(),
                solver->getNonZeroCount(),
                solver->getNetwork()->getInterDieConnectionCount());
  return true;
}

void PDNSim::add3DPDNCurrent(const std::string& chip_net_name,
                             const std::string& chip_name,
                             const std::string& port_name,
                             double current)
{
  auto* net = findChipNet(chip_net_name);
  if (!std::isfinite(current)) {
    logger_->error(utl::PSM,
                   115,
                   "Current at {}/{} must be finite.",
                   chip_name,
                   port_name);
  }
  find3DChip(net, chip_name, port_name);
  user_currents_3d_[chip_net_name].push_back({chip_name, port_name, current});
  solvers_3d_.erase(net);
}

bool PDNSim::check3DJVector(const std::string& chip_net_name, bool use_sta)
{
  auto solver
      = make3DSolver(findChipNet(chip_net_name), sta_->cmdScene(), use_sta);
  if (!solver->checkAssemblySystem() || !solver->checkCurrentVector()) {
    logger_->error(utl::PSM,
                   119,
                   "The combined 3D G matrix or J vector for {} is invalid.",
                   chip_net_name);
  }
  logger_->info(
      utl::PSM,
      120,
      "Built combined 3D J vector {}: {} loads and {:.6g} A total current.",
      chip_net_name,
      solver->getCurrentLoadCount(),
      solver->getTotalCurrent());
  return true;
}

void PDNSim::setChipletVoltage(const std::string& chip_net_name,
                               const std::string& chip_name,
                               const std::string& port_name,
                               double voltage,
                               sta::Scene* corner)
{
  auto* net = findChipNet(chip_net_name);
  if (!std::isfinite(voltage)) {
    logger_->error(utl::PSM,
                   124,
                   "Voltage at {}/{} must be finite.",
                   chip_name,
                   port_name);
  }
  find3DChip(net, chip_name, port_name);

  auto& sources = user_voltage_sources_3d_[chip_net_name];
  const auto existing
      = std::ranges::find_if(sources, [&](const VoltageSource3D& source) {
          return source.chip_name == chip_name && source.terminal == port_name
                 && source.corner == corner;
        });
  if (existing == sources.end()) {
    sources.push_back({chip_name, port_name, voltage, corner});
  } else {
    existing->voltage = voltage;
  }
  solvers_3d_.erase(net);
}

bool PDNSim::analyze3DPowerGrid(const std::string& chip_net_name,
                                sta::Scene* corner,
                                bool use_sta,
                                const std::string& error_file,
                                const std::string& voltage_source_file,
                                const std::string& voltage_file,
                                bool enable_em)
{
  auto* net = findChipNet(chip_net_name);
  // A failed re-solve must not leave an earlier result available.
  solvers_3d_.erase(net);
  auto solver = make3DSolver(net, corner, use_sta);
  check3DConnectivity(*solver->getNetwork(), false, false, error_file);
  if (!voltage_source_file.empty()) {
    solver->addVoltageSourcesFromSourceFile(voltage_source_file);
  }
  if (solver->getVoltageSourceCount() == 0) {
    logger_->error(utl::PSM,
                   138,
                   "No voltage source is defined for 3D net {}.",
                   chip_net_name);
  }
  if (!solver->hasValidSources()) {
    logger_->error(
        utl::PSM,
        139,
        "The 3D grid for {} requires valid sources with identical voltages.",
        chip_net_name);
  }
  if (!solver->solveAssembly()) {
    logger_->error(utl::PSM,
                   128,
                   "Failed to solve the combined 3D power grid for {}.",
                   chip_net_name);
  }
  const auto& voltages = solver->getVoltageVector();
  logger_->info(
      utl::PSM,
      129,
      "Solved combined 3D power grid {}: {} nodes, {} current loads, {} "
      "fixed-voltage sources, and voltage range {:.6g} to {:.6g} V.",
      chip_net_name,
      solver->getNodeCount(),
      solver->getCurrentLoadCount(),
      solver->getVoltageSourceCount(),
      voltages.minCoeff(),
      voltages.maxCoeff());
  solver->report(corner);
  if (enable_em) {
    solver->reportEM(corner);
  }
  if (!voltage_file.empty()) {
    solver->writeInstanceVoltageFile(voltage_file, corner);
  }
  solvers_3d_[net] = {std::move(solver), user_powers_, user_voltages_};
  return true;
}

double PDNSim::get3DPDNVoltage(const std::string& chip_net_name,
                               const std::string& chip_name,
                               const std::string& port_name)
{
  auto* net = findChipNet(chip_net_name);
  const auto solver = solvers_3d_.find(net);
  if (solver == solvers_3d_.end()
      || !solver->second.solver->hasAssemblySolution()
      || solver->second.powers != user_powers_
      || solver->second.voltages != user_voltages_) {
    solvers_3d_.erase(net);
    logger_->error(utl::PSM,
                   130,
                   "No 3D voltage solution is available for {}.",
                   chip_net_name);
  }
  auto* chip = find3DChip(net, chip_name, port_name);
  const auto voltage = solver->second.solver->getVoltage(chip, port_name);
  if (!voltage.has_value()) {
    logger_->error(utl::PSM,
                   133,
                   "Cannot map terminal {}/{} into the solved 3D net {}.",
                   chip_name,
                   port_name,
                   chip_net_name);
  }
  return *voltage;
}

void PDNSim::clear3DPowerGrid()
{
  solvers_3d_.clear();
  user_currents_3d_.clear();
  user_voltage_sources_3d_.clear();
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
    auto network = std::make_unique<IRNetwork>(net, logger_, floorplanning);
    solver = std::make_unique<IRSolver>(std::move(network),
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
  solvers_3d_.clear();
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

void PDNSim::BlockObserver3D::inDbPostMoveInst(odb::dbInst*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbInstCreate(odb::dbInst*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbInstDestroy(odb::dbInst* inst)
{
  // Do not keep a power setting whose instance is about to be deleted.
  owner_->user_powers_.erase(inst);
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbInstSwapMasterBefore(odb::dbInst*,
                                                       odb::dbMaster*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbITermPreDisconnect(odb::dbITerm*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbITermPostConnect(odb::dbITerm*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbNetDestroy(odb::dbNet* net)
{
  owner_->user_voltages_.erase(net);
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBTermPostConnect(odb::dbBTerm*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBTermPostDisConnect(odb::dbBTerm*,
                                                      odb::dbNet*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBPinCreate(odb::dbBPin*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBPinAddBox(odb::dbBox*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBPinRemoveBox(odb::dbBox*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbBPinDestroy(odb::dbBPin*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbSWireAddSBox(odb::dbSBox*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbSWireRemoveSBox(odb::dbSBox*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbSWirePostDestroySBoxes(odb::dbSWire*)
{
  owner_->clearSolvers();
}

void PDNSim::BlockObserver3D::inDbFillCreate(odb::dbFill*)
{
  owner_->clearSolvers();
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
