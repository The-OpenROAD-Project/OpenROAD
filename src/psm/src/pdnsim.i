/*
BSD 3-Clause License

Copyright (c) 2022, The Regents of the University of Minnesota

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

%include "../../Exception.i"
%{
#include "ord/OpenRoad.hh"
#include "psm/pdnsim.h"
#include "sta/Corner.hh"

namespace ord {
psm::PDNSim*
getPDNSim();
}

namespace odb {
class dbNet;
}

using ord::getPDNSim;
using psm::PDNSim;
using sta::Corner;

%}

%typemap(in) psm::GeneratedSourceType {
  int length;
  const char *arg = Tcl_GetStringFromObj($input, &length);

  if (strcmp(arg, "BUMPS") == 0) {
    $1 = psm::GeneratedSourceType::BUMPS;
  } else if (strcmp(arg, "FULL") == 0) {
    $1 = psm::GeneratedSourceType::FULL;
  } else if (strcmp(arg, "STRAPS") == 0) {
    $1 = psm::GeneratedSourceType::STRAPS;
  } else {
    $1 = psm::GeneratedSourceType::BUMPS;
  }
}

%inline %{


void 
set_net_voltage_cmd(odb::dbNet* net, Corner* corner, double voltage)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setNetVoltage(net, corner, voltage);
}

void 
analyze_power_grid_cmd(odb::dbNet* net, Corner* corner, psm::GeneratedSourceType type, const char* error_file, bool enable_em, const char* em_file, const char* voltage_file, const char* voltage_source_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->analyzePowerGrid(net, corner, type, voltage_file, enable_em, em_file, error_file, voltage_source_file);
}

void
add_decap_master(odb::dbMaster *master, float cap)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->addDecapMaster(master, cap);
}

void
insert_decap_cmd(const float target, const char* net_name)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->insertDecapCells(target, net_name);
}

bool
check_connectivity_cmd(odb::dbNet* net, bool floorplanning, const char* error_file)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->checkConnectivity(net, floorplanning, error_file);
}

void
write_spice_file_cmd(odb::dbNet* net, Corner* corner, psm::GeneratedSourceType type, const char* file, const char* voltage_source_file)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->writeSpiceNetwork(net, corner, type, file, voltage_source_file);
}

void set_debug_gui(bool enable)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setDebugGui(enable);
}

void clear_solvers()
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->clearSolvers();
}

void set_source_settings(int bump_dx, int bump_dy, int bump_size, int bump_interval, int track_pitch)
{
  PDNSim::GeneratedSourceSettings settings;
  settings.bump_dx = bump_dx;
  settings.bump_dy = bump_dy;
  settings.bump_size = bump_size;
  settings.bump_interval = bump_interval;
  settings.strap_track_pitch = track_pitch;

  PDNSim* pdnsim = getPDNSim();
  pdnsim->setGeneratedSourceSettings(settings);
}

void
set_inst_power(odb::dbInst* inst, Corner* corner, float power)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setInstPower(inst, corner, power);
}

%} // inline

