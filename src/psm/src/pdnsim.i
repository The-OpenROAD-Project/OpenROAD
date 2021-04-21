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


%} // inline

