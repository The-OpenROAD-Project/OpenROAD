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

namespace ord {
psm::PDNSim*
getPDNSim();
}

using ord::getPDNSim;
using psm::PDNSim;
%}

%inline %{

void 
import_vsrc_cfg_cmd(const char* vsrc)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_vsrc_cfg(vsrc);
}

void 
set_power_net_cmd(const char* net)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_power_net(net);
}

void 
set_bump_pitch_x_cmd(float bump_pitch)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_bump_pitch_x(bump_pitch);
}

void 
set_bump_pitch_y_cmd(float bump_pitch)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_bump_pitch_y(bump_pitch);
}

void
set_node_density(float node_density)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_node_density(node_density);
}

void
set_node_density_factor(int node_density_factor)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_node_density_factor(node_density_factor);
}



void 
set_net_voltage_cmd(const char* net_name, float voltage)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_pdnsim_net_voltage(net_name, voltage);
}



void 
import_em_enable(int enable_em)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_enable_em(enable_em);
}


void 
import_out_file_cmd(const char* out_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_out_file(out_file);
}

void 
import_em_out_file_cmd(const char* out_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_em_out_file(out_file);
}


void 
import_spice_out_file_cmd(const char* out_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_spice_out_file(out_file);
}

void 
analyze_power_grid_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->analyze_power_grid();
}

int
check_connectivity_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->check_connectivity();
}

void
write_pg_spice_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->write_pg_spice();
}

void set_debug_gui_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setDebugGui();
}

%} // inline

