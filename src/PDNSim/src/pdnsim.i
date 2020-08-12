%module pdnsim

%{
#include "openroad/OpenRoad.hh"
#include "pdnsim/pdnsim.h"

namespace ord {
pdnsim::PDNSim*
getPDNSim();
}

using ord::getPDNSim;
using pdnsim::PDNSim;
%}

%inline %{

void 
pdnsim_import_vsrc_cfg_cmd(const char* vsrc)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_vsrc_cfg(vsrc);
}

void 
pdnsim_set_power_net_cmd(const char* net)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->set_power_net(net);
}



void 
pdnsim_import_out_file_cmd(const char* out_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_out_file(out_file);
}

void 
pdnsim_import_spice_out_file_cmd(const char* out_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->import_spice_out_file(out_file);
}

void 
pdnsim_analyze_power_grid_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->analyze_power_grid();
}

int
pdnsim_check_connectivity_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->check_connectivity();
}

void
pdnsim_write_pg_spice_cmd()
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->write_pg_spice();
}


%} // inline

