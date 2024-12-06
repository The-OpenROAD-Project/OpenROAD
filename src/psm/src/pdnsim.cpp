/*
BSD 3-Clause License

Copyright (c) 2020, The Regents of the University of Minnesota

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "psm/pdnsim.h"

#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "heatMap.h"
#include "ir_network.h"
#include "ir_solver.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "shape.h"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

using odb::dbBlock;
using odb::dbSigType;

namespace psm {

PDNSim::PDNSim() = default;

PDNSim::~PDNSim() = default;

void PDNSim::init(utl::Logger* logger,
                  odb::dbDatabase* db,
                  sta::dbSta* sta,
                  rsz::Resizer* resizer,
                  dpl::Opendp* opendp)
{
  db_ = db;
  sta_ = sta;
  resizer_ = resizer;
  opendp_ = opendp;
  logger_ = logger;
  heatmap_ = std::make_unique<IRDropDataSource>(this, sta, logger_);
  heatmap_->registerHeatMap();
}

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
                              bool enable_em,
                              const std::string& em_file,
                              const std::string& error_file,
                              const std::string& voltage_source_file)
{
  if (!checkConnectivity(net, false, error_file)) {
    return;
  }

  last_corner_ = corner;
  auto* solver = getIRSolver(net, false);
  solver->solve(corner, source_type, voltage_source_file);
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
                               const std::string& error_file)
{
  auto* solver = getIRSolver(net, floorplanning);
  const bool check = solver->check();
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
                                        resizer_,
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
void PDNSim::addDecapMaster(dbMaster* decap_master, double decap_cap)
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
