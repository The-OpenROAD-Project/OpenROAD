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

#include <tcl.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "debug_gui.h"
#include "gmat.h"
#include "heatMap.h"
#include "ir_solver.h"
#include "node.h"
#include "odb/db.h"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace psm {

PDNSim::PDNSim() = default;

PDNSim::~PDNSim() = default;

void PDNSim::init(utl::Logger* logger,
                  odb::dbDatabase* db,
                  sta::dbSta* sta,
                  rsz::Resizer* resizer)
{
  db_ = db;
  sta_ = sta;
  resizer_ = resizer;
  logger_ = logger;
  heatmap_ = std::make_unique<IRDropDataSource>(this, logger_);
  heatmap_->registerHeatMap();
}

void PDNSim::setDebugGui()
{
  debug_gui_ = std::make_unique<DebugGui>(this);
}

void PDNSim::setNetVoltage(odb::dbNet* net, float voltage)
{
  net_voltage_map_[net] = voltage;
}

void PDNSim::setVsrcCfg(const std::string& vsrc)
{
  vsrc_loc_ = vsrc;
  logger_->info(utl::PSM, 1, "Reading voltage source file: {}.", vsrc_loc_);
}

std::unique_ptr<IRSolver> PDNSim::getIRSolver(bool require_voltage)
{
  if (corner_ == nullptr) {
    corner_ = sta_->cmdCorner();
  }
  if (corner_ == nullptr) {
    corner_ = sta_->corners()->findCorner(0);
  }
  if (corner_ == nullptr) {
    logger_->error(utl::PSM, 84, "Unable to proceed without a valid corner");
  }

  std::optional<float> voltage = getNetVoltage(net_, require_voltage);

  return std::make_unique<IRSolver>(db_,
                                    sta_,
                                    resizer_,
                                    logger_,
                                    net_,
                                    voltage,
                                    vsrc_loc_,
                                    require_voltage,
                                    bump_pitch_x_,
                                    bump_pitch_y_,
                                    node_density_,
                                    node_density_factor_,
                                    corner_);
}

void PDNSim::writeSpice(const std::string& file)
{
  auto irsolve_h = getIRSolver(false);

  if (irsolve_h->build()) {
    irsolve_h->writeSpiceFile(file);
  }
}

void PDNSim::saveIRDrop(psm::IRSolver* ir_solver)
{
  GMat* gmat_obj = ir_solver->getGMat();

  IRDropByLayer ir_drop;
  std::vector<Node*> nodes = gmat_obj->getAllNodes();
  int vsize;
  vsize = nodes.size();
  odb::dbTech* tech = db_->getTech();
  for (int n = 0; n < vsize; n++) {
    Node* node = nodes[n];
    int node_layer_num = node->getLayerNum();
    Point node_loc = node->getLoc();
    odb::Point point = odb::Point(node_loc.getX(), node_loc.getY());
    double voltage = node->getVoltage();
    odb::dbTechLayer* node_layer = tech->findRoutingLayer(node_layer_num);
    // Absolute is needed for GND nets. In case of GND net voltage is higher
    // than supply.
    ir_drop[node_layer][point]
        = std::abs(ir_solver->getSupplyVoltageSrc() - voltage);
  }
  ir_drop_ = ir_drop;

  if (debug_gui_) {
    debug_gui_->setSources(ir_solver->getSources());
  }
}

void PDNSim::analyzePowerGrid(const std::string& voltage_file,
                              bool enable_em,
                              const std::string& em_file,
                              const std::string& error_file)
{
  auto irsolve_h = getIRSolver(enable_em);

  if (!irsolve_h->build(error_file)) {
    saveIRDrop(irsolve_h.get());
    logger_->error(
        utl::PSM, 78, "IR drop setup failed.  Analysis can't proceed.");
  }
  const std::string corner_name
      = corner_ != nullptr ? corner_->name() : "default";
  const std::string metric_suffix
      = fmt::format("__net:{}__corner:{}", net_->getName(), corner_name);
  irsolve_h->solveIR();
  logger_->report("########## IR report #################");
  logger_->report("Corner: {}", corner_name);
  logger_->report("Worstcase voltage: {:3.2e} V",
                  irsolve_h->getWorstCaseVoltage());
  const double avg_drop
      = std::abs(irsolve_h->getSupplyVoltageSrc() - irsolve_h->getAvgVoltage());
  const double worst_drop = std::abs(irsolve_h->getSupplyVoltageSrc()
                                     - irsolve_h->getWorstCaseVoltage());
  logger_->report("Average IR drop  : {:3.2e} V", avg_drop);
  logger_->report("Worstcase IR drop: {:3.2e} V", worst_drop);
  logger_->report("######################################");

  logger_->metric(
      fmt::format("design_powergrid__voltage__worst{}", metric_suffix),
      irsolve_h->getWorstCaseVoltage());
  logger_->metric(
      fmt::format("design_powergrid__drop__average{}", metric_suffix),
      avg_drop);
  logger_->metric(fmt::format("design_powergrid__drop__worst{}", metric_suffix),
                  worst_drop);

  if (!voltage_file.empty()) {
    irsolve_h->writeVoltageFile(voltage_file);
  }

  if (enable_em) {
    logger_->report("########## EM analysis ###############");
    logger_->report("Maximum current: {:3.2e} A", irsolve_h->getMaxCurrent());
    logger_->report("Average current: {:3.2e} A", irsolve_h->getAvgCurrent());
    logger_->report("Number of resistors: {}", irsolve_h->getNumResistors());
    logger_->report("######################################");

    logger_->metric(
        fmt::format("design_powergrid__current__average{}", metric_suffix),
        irsolve_h->getAvgCurrent());
    logger_->metric(
        fmt::format("design_powergrid__current__max{}", metric_suffix),
        irsolve_h->getMaxCurrent());

    if (!em_file.empty()) {
      irsolve_h->writeEMFile(em_file);
    }
  }

  saveIRDrop(irsolve_h.get());
  min_resolution_ = irsolve_h->getMinimumResolution();
  heatmap_->update();
}

bool PDNSim::checkConnectivity(const std::string& error_file)
{
  auto irsolve_h = getIRSolver(false);
  if (!irsolve_h->build(error_file, true)) {
    return false;
  }
  if (debug_gui_) {
    debug_gui_->setSources(irsolve_h->getSources());
  }
  return irsolve_h->getConnectionTest();
}

void PDNSim::getIRDropMap(IRDropByLayer& ir_drop)
{
  ir_drop = ir_drop_;
}

void PDNSim::getIRDropForLayer(odb::dbTechLayer* layer, IRDropByPoint& ir_drop)
{
  auto it = ir_drop_.find(layer);
  if (it == ir_drop_.end()) {
    ir_drop.clear();
    return;
  }

  ir_drop = it->second;
}

int PDNSim::getMinimumResolution()
{
  if (min_resolution_ <= 0) {
    logger_->error(
        utl::PSM,
        68,
        "Minimum resolution not set. Please run analyze_power_grid first.");
  }
  return min_resolution_;
}

std::optional<float> PDNSim::getNetVoltage(odb::dbNet* net,
                                           bool require_voltage) const
{
  if (net == nullptr) {
    return {};
  }

  auto find_net = net_voltage_map_.find(net);
  if (find_net != net_voltage_map_.end()) {
    return find_net->second;
  }

  if (net->getSigType() == odb::dbSigType::GROUND) {
    return 0.0;
  }

  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::LibertyLibrary* default_library = network->defaultLibertyLibrary();
  if (default_library == nullptr) {
    if (!require_voltage) {
      return {};
    }

    logger_->error(
        utl::PSM,
        79,
        "Can't determine the supply voltage as no Liberty is loaded.");
  }

  const sta::DcalcAnalysisPt* dcalc_ap
      = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  const sta::Pvt* pvt = dcalc_ap->operatingConditions();
  if (pvt == nullptr) {
    pvt = default_library->defaultOperatingConditions();
  }
  if (pvt) {
    return pvt->voltage();
  }

  return {};
}

}  // namespace psm
