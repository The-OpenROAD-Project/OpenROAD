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
#include <iostream>
#include <iomanip>
#include <sstream>
#include "opendb/db.h"
#include "ir_solver.h"
#include <string>
#include <vector>
#include "gmat.h"
#include "node.h"
#include "utl/Logger.h"

namespace psm {

PDNSim::PDNSim()
    : _db(nullptr),
      _sta(nullptr),
      _vsrc_loc(""),
      _power_net(""),
      _out_file(""),
      _em_out_file(""),
      _enable_em(0),
      _spice_out_file(""),
      _bump_pitch_x(0),
      _bump_pitch_y(0)
      //_net_voltage_map(nullptr)
      {};

PDNSim::~PDNSim()
{
  _db             = nullptr;
  _sta            = nullptr;
  _vsrc_loc       = "";
  _power_net      = "";
  _out_file       = "";
  _em_out_file    = "";
  _enable_em      = 0;
  _spice_out_file = "";
  _bump_pitch_x   = 0;
  _bump_pitch_y   = 0;
  //_net_voltage_map = nullptr;
}

void PDNSim::init(utl::Logger* logger, odb::dbDatabase* db, sta::dbSta* sta)
{
  _db     = db;
  _sta    = sta;
  _logger = logger;
}

void PDNSim::set_power_net(std::string net)
{
  _power_net = net;
}

void PDNSim::set_bump_pitch_x(float bump_pitch)
{
  _bump_pitch_x = bump_pitch;
}

void PDNSim::set_bump_pitch_y(float bump_pitch)
{
  _bump_pitch_y = bump_pitch;
}

void PDNSim::set_pdnsim_net_voltage(std::string net, float voltage)
{
  _net_voltage_map.insert(std::pair<std::string, float>(net, voltage));
}

void PDNSim::import_vsrc_cfg(std::string vsrc)
{
  _vsrc_loc = vsrc;
  _logger->info(utl::PSM, 1, "Reading voltage source file: {}.", _vsrc_loc);
}

void PDNSim::import_out_file(std::string out_file)
{
  _out_file = out_file;
  _logger->info(
      utl::PSM, 2, "Output voltage file is specified as: {}.", _out_file);
}

void PDNSim::import_em_out_file(std::string em_out_file)
{
  _em_out_file = em_out_file;
  _logger->info(utl::PSM, 3, "Output current file specified {}.", _em_out_file);
}
void PDNSim::import_enable_em(int enable_em)
{
  _enable_em = enable_em;
  if (_enable_em == 1) {
    _logger->info(utl::PSM, 4, "EM calculation is enabled.");
  }
}

void PDNSim::import_spice_out_file(std::string out_file)
{
  _spice_out_file = out_file;
  _logger->info(
      utl::PSM, 5, "Output spice file is specified as: {}.", _spice_out_file);
}

void PDNSim::write_pg_spice()
{
  IRSolver* irsolve_h = new IRSolver(_db,
                                     _sta,
                                     _logger,
                                     _vsrc_loc,
                                     _power_net,
                                     _out_file,
                                     _em_out_file,
                                     _spice_out_file,
                                     _enable_em,
                                     _bump_pitch_x,
                                     _bump_pitch_y,
                                     _net_voltage_map);

  if (!irsolve_h->Build()) {
    delete irsolve_h;
  } else {
    int check_spice = irsolve_h->PrintSpice();
    if (check_spice) {
      _logger->info(
          utl::PSM, 6, "SPICE file is written at: {}.", _spice_out_file);
    } else {
      _logger->error(
          utl::PSM, 7, "Failed to write out spice file: {}.", _spice_out_file);
    }
  }
}

int PDNSim::analyze_power_grid()
{
  GMat*     gmat_obj;
  IRSolver* irsolve_h = new IRSolver(_db,
                                     _sta,
                                     _logger,
                                     _vsrc_loc,
                                     _power_net,
                                     _out_file,
                                     _em_out_file,
                                     _spice_out_file,
                                     _enable_em,
                                     _bump_pitch_x,
                                     _bump_pitch_y,
                                     _net_voltage_map);

  if (!irsolve_h->Build()) {
    delete irsolve_h;
    return 0;
  }
  gmat_obj = irsolve_h->GetGMat();
  irsolve_h->SolveIR();
  std::vector<Node*> nodes       = gmat_obj->GetAllNodes();
  int                unit_micron = (_db->getTech())->getDbUnitsPerMicron();
  int                vsize;
  vsize = nodes.size();
  for (int n = 0; n < vsize; n++) {
    Node* node = nodes[n];
    if (node->GetLayerNum() != 1)
      continue;
    NodeLoc loc = node->GetLoc();
  }
  _logger->report("########## IR report #################");
  _logger->report("Worstcase voltage: {:3.2e} V", irsolve_h->wc_voltage);
  _logger->report("Average IR drop  : {:3.2e} V",
                  abs(irsolve_h->supply_voltage_src - irsolve_h->avg_voltage));
  _logger->report("Worstcase IR drop: {:3.2e} V",
                  abs(irsolve_h->supply_voltage_src - irsolve_h->wc_voltage));
  _logger->report("######################################");
  if (_enable_em == 1) {
    _logger->report("########## EM analysis ###############");
    _logger->report("Maximum current: {:3.2e} A", irsolve_h->max_cur);
    _logger->report("Average current: {:3.2e} A", irsolve_h->avg_cur);
    _logger->report("Number of resistors: {}", irsolve_h->num_res);
    _logger->report("######################################");
  }

  delete irsolve_h;
  return 1;
}

int PDNSim::check_connectivity()
{
  IRSolver* irsolve_h = new IRSolver(_db,
                                     _sta,
                                     _logger,
                                     _vsrc_loc,
                                     _power_net,
                                     _out_file,
                                     _em_out_file,
                                     _spice_out_file,
                                     _enable_em,
                                     _bump_pitch_x,
                                     _bump_pitch_y,
                                     _net_voltage_map);
  if (!irsolve_h->BuildConnection()) {
    delete irsolve_h;
    return 0;
  }
  int val = irsolve_h->GetConnectionTest();
  delete irsolve_h;
  return val;
}

}  // namespace psm
