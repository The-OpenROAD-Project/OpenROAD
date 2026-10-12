// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%include "../../Exception.i"
%{
#include "db_sta/dbNetwork.hh"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include <vector>
#include "psm/pdnsim.h"
#include "sta/Scene.hh"

namespace ord {
psm::PDNSim*
getPDNSim();
}

namespace odb {
class dbNet;
}

using ord::getPDNSim;
using psm::PDNSim;
using sta::Scene;

#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
  typedef int Tcl_Size;
#endif

%}

// OpenSTA swig rules
%include "tcl/StaTclTypes.i"

%typemap(in) psm::GeneratedSourceType {
  Tcl_Size length;
  const char *arg = Tcl_GetStringFromObj($input, &length);

  if (strcmp(arg, "BUMPS") == 0) {
    $1 = psm::GeneratedSourceType::kBumps;
  } else if (strcmp(arg, "FULL") == 0) {
    $1 = psm::GeneratedSourceType::kFull;
  } else if (strcmp(arg, "STRAPS") == 0) {
    $1 = psm::GeneratedSourceType::kStraps;
  } else {
    $1 = psm::GeneratedSourceType::kBumps;
  }
}

%typemap(out) std::vector<odb::dbChipRSeg*> {
  Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
  for (auto* resistor : $1) {
    Tcl_ListObjAppendElement(interp, list,
        SWIG_NewPointerObj(resistor, $descriptor(odb::dbChipRSeg*), 0));
  }
  Tcl_SetObjResult(interp, list);
}

%inline %{

bool
is_3d_design(odb::dbChip* chip)
{
  return sta::dbNetwork::is3DicTopChip(chip);
}

// ODB's C++ collections are available even when its Tcl list wrappers are not.
std::vector<odb::dbChipRSeg*>
get_3d_chip_rsegs(odb::dbChipNet* net)
{
  std::vector<odb::dbChipRSeg*> resistors;
  if (net) {
    for (auto* resistor : net->getChipRSegs()) {
      resistors.push_back(resistor);
    }
  }
  return resistors;
}

// Internal regression helpers; these are not registered as user commands.
void
clear_3d_power_grid_cmd()
{
  getPDNSim()->clear3DPowerGrid();
}

bool
check_3d_power_grid_cmd(const char* chip_net_name,
                        bool floorplanning = false,
                        const char* error_file = "",
                        bool require_bterm = true)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->check3DPowerGrid(chip_net_name, floorplanning, error_file, require_bterm);
}

bool
check_3d_g_matrix_cmd(const char* chip_net_name)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->check3DGMatrix(chip_net_name);
}

void
add_3d_pdn_current_cmd(const char* chip_net_name,
                       const char* chip_name,
                       const char* port_name,
                       double current)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->add3DPDNCurrent(chip_net_name, chip_name, port_name, current);
}

bool
check_3d_j_vector_cmd(const char* chip_net_name, bool use_sta)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->check3DJVector(chip_net_name, use_sta);
}

void
set_pdnsim_chiplet_voltage_cmd(const char* chip_net_name,
                              const char* chip_name,
                              const char* port_name,
                              double voltage,
                              Scene* corner,
                              bool set_source = true,
                              bool set_nominal = false,
                              double nominal_voltage = 0.0)
{
  std::optional<double> source;
  std::optional<double> nominal;
  if (set_source) {
    source = voltage;
  }
  if (set_nominal) {
    nominal = nominal_voltage;
  }
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setChipletVoltage(
      chip_net_name, chip_name, port_name, source, corner, nominal);
}

bool
analyze_3d_power_grid_cmd(const char* chip_net_name,
                          Scene* corner,
                          bool use_sta = true,
                          const char* error_file = "",
                          const char* voltage_file = "")
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->analyze3DPowerGrid(chip_net_name, corner, use_sta,
                                  error_file, voltage_file);
}

double
get_3d_pdn_voltage_cmd(const char* chip_net_name,
                       const char* chip_name,
                       const char* port_name)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->get3DPDNVoltage(chip_net_name, chip_name, port_name);
}

void 
set_net_voltage_cmd(odb::dbNet* net, Scene* corner, double voltage)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setNetVoltage(net, corner, voltage);
}

void 
analyze_power_grid_cmd(odb::dbNet* net, Scene* corner, psm::GeneratedSourceType type, const char* error_file, bool reuse_solution, bool enable_em, const char* em_file, const char* voltage_file, const char* voltage_source_file)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->analyzePowerGrid(net, corner, type, voltage_file, reuse_solution, enable_em, em_file, error_file, voltage_source_file);
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
check_connectivity_cmd(odb::dbNet* net, bool floorplanning, const char* error_file, bool dont_require_bterm)
{
  PDNSim* pdnsim = getPDNSim();
  return pdnsim->checkConnectivity(net, floorplanning, error_file, !dont_require_bterm);
}

void
write_spice_file_cmd(odb::dbNet* net, Scene* corner, psm::GeneratedSourceType type, const char* file, const char* voltage_source_file)
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

void set_source_settings(int bump_dx, int bump_dy, int bump_size, int bump_interval, int track_pitch, float resistance)
{
  PDNSim::GeneratedSourceSettings settings;
  settings.bump_dx = bump_dx;
  settings.bump_dy = bump_dy;
  settings.bump_size = bump_size;
  settings.bump_interval = bump_interval;
  settings.strap_track_pitch = track_pitch;
  settings.resistance = resistance;

  PDNSim* pdnsim = getPDNSim();
  pdnsim->setGeneratedSourceSettings(settings);
}

void
set_inst_power(odb::dbInst* inst, Scene* corner, float power)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setInstPower(inst, corner, power);
}

%} // inline
