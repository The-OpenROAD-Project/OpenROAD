// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "psm/pdnsim.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

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
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

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
  heatmap_ = std::make_unique<IRDropDataSource>(this, sta, logger_);
  heatmap_->registerHeatMap();
}

PDNSim::~PDNSim() = default;

void PDNSim::setDebugGui(bool enable)
{
  debug_gui_enabled_ = enable;

  for (const auto& [net, solver] : solvers_) {
    solver->enableGui(debug_gui_enabled_);
  }

  gui::Gui::get()->registerDescriptor<Node*>(new NodeDescriptor(solvers_));
  gui::Gui::get()->registerDescriptor<ITermNode*>(
      new ITermNodeDescriptor(solvers_));
  gui::Gui::get()->registerDescriptor<BPinNode*>(
      new BPinNodeDescriptor(solvers_));
  gui::Gui::get()->registerDescriptor<Connection*>(
      new ConnectionDescriptor(solvers_));
}

void PDNSim::setNetVoltage(odb::dbNet* net, sta::Corner* corner, double voltage)
{
  auto& voltages = user_voltages_[net];
  voltages[corner] = voltage;
}

void PDNSim::setInstPower(odb::dbInst* inst, sta::Corner* corner, float power)
{
  auto& powers = user_powers_[inst];
  powers[corner] = power;
}

void PDNSim::analyzePowerGrid(odb::dbNet* net,
                              sta::Corner* corner,
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

  last_corner_ = corner;
  auto* solver = getIRSolver(net, false);
  if (!use_prev_solution || !solver->hasSolution(corner)) {
    solver->solve(corner, source_type, voltage_source_file);
  } else {
    logger_->info(utl::PSM, 11, "Reusing previous solution");
  }
  solver->report(corner);

  heatmap_->setNet(net);
  heatmap_->setCorner(corner);
  heatmap_->update();

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
  const bool check = solver->check(require_bterm);
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

void PDNSim::writeSpiceNetwork(odb::dbNet* net,
                               sta::Corner* corner,
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
                               sta::Corner* corner,
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
