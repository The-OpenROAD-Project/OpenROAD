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

#include "debug_gui.h"
#include "gmat.h"
#include "heatMap.h"
#include "ir_solver.h"
#include "node.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace psm {

PDNSim::PDNSim()
    : db_(nullptr),
      sta_(nullptr),
      logger_(nullptr),
      vsrc_loc_(""),
      out_file_(""),
      em_out_file_(""),
      enable_em_(false),
      bump_pitch_x_(0),
      bump_pitch_y_(0),
      spice_out_file_(""),
      power_net_(""),
      node_density_(-1),
      node_density_factor_(0),
      min_resolution_(-1)
{
}

PDNSim::~PDNSim()
{
  db_ = nullptr;
  sta_ = nullptr;
  vsrc_loc_ = "";
  power_net_ = "";
  out_file_ = "";
  em_out_file_ = "";
  enable_em_ = false;
  spice_out_file_ = "";
  bump_pitch_x_ = 0;
  bump_pitch_y_ = 0;
  node_density_ = -1.0;
  node_density_factor_ = 0;
  min_resolution_ = -1.0;
}

void PDNSim::init(utl::Logger* logger, odb::dbDatabase* db, sta::dbSta* sta)
{
  db_ = db;
  sta_ = sta;
  logger_ = logger;
  heatmap_ = std::make_unique<IRDropDataSource>(this, logger_);
  heatmap_->registerHeatMap();
}

void PDNSim::setDebugGui()
{
  debug_gui_ = std::make_unique<DebugGui>(this);
}

void PDNSim::set_power_net(std::string net)
{
  power_net_ = net;
}

void PDNSim::set_bump_pitch_x(float bump_pitch)
{
  bump_pitch_x_ = bump_pitch;
}

void PDNSim::set_bump_pitch_y(float bump_pitch)
{
  bump_pitch_y_ = bump_pitch;
}

void PDNSim::set_node_density(float node_density)
{
  node_density_ = node_density;
}

void PDNSim::set_node_density_factor(int node_density_factor)
{
  node_density_factor_ = node_density_factor;
}

void PDNSim::set_pdnsim_net_voltage(std::string net, float voltage)
{
  net_voltage_map_.insert(std::pair<std::string, float>(net, voltage));
}

void PDNSim::import_vsrc_cfg(std::string vsrc)
{
  vsrc_loc_ = vsrc;
  logger_->info(utl::PSM, 1, "Reading voltage source file: {}.", vsrc_loc_);
}

void PDNSim::import_out_file(std::string out_file)
{
  out_file_ = out_file;
  logger_->info(
      utl::PSM, 2, "Output voltage file is specified as: {}.", out_file_);
}

void PDNSim::import_em_out_file(std::string em_out_file)
{
  em_out_file_ = em_out_file;
  logger_->info(utl::PSM, 3, "Output current file specified {}.", em_out_file_);
}
void PDNSim::import_enable_em(bool enable_em)
{
  enable_em_ = enable_em;
  if (enable_em_) {
    logger_->info(utl::PSM, 4, "EM calculation is enabled.");
  }
}

void PDNSim::import_spice_out_file(std::string out_file)
{
  spice_out_file_ = out_file;
  logger_->info(
      utl::PSM, 5, "Output spice file is specified as: {}.", spice_out_file_);
}

void PDNSim::write_pg_spice()
{
  auto irsolve_h = std::make_unique<IRSolver>(db_,
                                              sta_,
                                              logger_,
                                              vsrc_loc_,
                                              power_net_,
                                              out_file_,
                                              em_out_file_,
                                              spice_out_file_,
                                              enable_em_,
                                              bump_pitch_x_,
                                              bump_pitch_y_,
                                              node_density_,
                                              node_density_factor_,
                                              net_voltage_map_);

  if (irsolve_h->build()) {
    int check_spice = irsolve_h->printSpice();
    if (check_spice) {
      logger_->info(
          utl::PSM, 6, "SPICE file is written at: {}.", spice_out_file_);
    } else {
      logger_->error(
          utl::PSM, 7, "Failed to write out spice file: {}.", spice_out_file_);
    }
  }
}

void PDNSim::analyze_power_grid()
{
  GMat* gmat_obj;
  auto irsolve_h = std::make_unique<IRSolver>(db_,
                                              sta_,
                                              logger_,
                                              vsrc_loc_,
                                              power_net_,
                                              out_file_,
                                              em_out_file_,
                                              spice_out_file_,
                                              enable_em_,
                                              bump_pitch_x_,
                                              bump_pitch_y_,
                                              node_density_,
                                              node_density_factor_,
                                              net_voltage_map_);

  if (!irsolve_h->build()) {
    logger_->error(
        utl::PSM, 78, "IR drop setup failed.  Analysis can't proceed.");
  }
  gmat_obj = irsolve_h->getGMat();
  irsolve_h->solveIR();
  logger_->report("########## IR report #################");
  logger_->report("Worstcase voltage: {:3.2e} V",
                  irsolve_h->getWorstCaseVoltage());
  logger_->report(
      "Average IR drop  : {:3.2e} V",
      abs(irsolve_h->getSupplyVoltageSrc() - irsolve_h->getAvgVoltage()));
  logger_->report(
      "Worstcase IR drop: {:3.2e} V",
      abs(irsolve_h->getSupplyVoltageSrc() - irsolve_h->getWorstCaseVoltage()));
  logger_->report("######################################");
  if (enable_em_) {
    logger_->report("########## EM analysis ###############");
    logger_->report("Maximum current: {:3.2e} A", irsolve_h->getMaxCurrent());
    logger_->report("Average current: {:3.2e} A", irsolve_h->getAvgCurrent());
    logger_->report("Number of resistors: {}", irsolve_h->getNumResistors());
    logger_->report("######################################");
  }

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
        = abs(irsolve_h->getSupplyVoltageSrc() - voltage);
  }
  ir_drop_ = ir_drop;
  min_resolution_ = irsolve_h->getMinimumResolution();

  heatmap_->update();
  if (debug_gui_) {
    debug_gui_->setBumps(irsolve_h->getBumps(), irsolve_h->getTopLayer());
  }
}

bool PDNSim::check_connectivity()
{
  auto irsolve_h = std::make_unique<IRSolver>(db_,
                                              sta_,
                                              logger_,
                                              vsrc_loc_,
                                              power_net_,
                                              out_file_,
                                              em_out_file_,
                                              spice_out_file_,
                                              enable_em_,
                                              bump_pitch_x_,
                                              bump_pitch_y_,
                                              node_density_,
                                              node_density_factor_,
                                              net_voltage_map_);
  if (!irsolve_h->buildConnection()) {
    return false;
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

}  // namespace psm
