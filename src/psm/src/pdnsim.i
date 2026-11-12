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

namespace {

odb::dbNet* resolvePowerGridNet(const char* net_name)
{
  auto* openroad = ord::OpenRoad::openRoad();
  auto* chip = openroad->getDb()->getChip();
  if (!sta::dbNetwork::is3DicTopChip(chip)) {
    auto* net = chip->getBlock()->findNet(net_name);
    if (net != nullptr) {
      return net;
    }
  } else {
    for (auto* chip_net : chip->getChipNets()) {
      if (std::string(chip_net->getName()) != net_name) {
        continue;
      }
      for (uint32_t index = 0; index < chip_net->getNumBumpInsts(); ++index) {
        std::vector<odb::dbChipInst*> path;
        auto* bump = chip_net->getBumpInst(index, path);
        if (bump != nullptr && bump->getChipBump() != nullptr
            && bump->getChipBump()->getNet() != nullptr) {
          return bump->getChipBump()->getNet();
        }
      }
      break;
    }
  }
  openroad->getLogger()->error(
      utl::PSM, 30, "Cannot find net {} in the design.", net_name);
  return nullptr;
}

}  // namespace

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

%inline %{

bool
is_3d_design(odb::dbChip* chip)
{
  return sta::dbNetwork::is3DicTopChip(chip);
}

void
set_interchip_resistance_cmd(Scene* scene, float resistance)
{
  getPDNSim()->setInterchipResistance(scene, resistance);
}

void
set_net_voltage_cmd(odb::dbNet* net, Scene* corner, double voltage)
{
  PDNSim* pdnsim = getPDNSim();
  pdnsim->setNetVoltage(net, corner, voltage);
}

void
analyze_power_grid_cmd(const char* net_name,
                       Scene* corner,
                       psm::GeneratedSourceType type,
                       const char* error_file,
                       bool reuse_solution,
                       bool enable_em,
                       const char* em_file,
                       const char* voltage_file,
                       const char* voltage_source_file)
{
  auto* net = resolvePowerGridNet(net_name);

  getPDNSim()->analyzePowerGrid(net,
                                corner,
                                type,
                                voltage_file,
                                reuse_solution,
                                enable_em,
                                em_file,
                                error_file,
                                voltage_source_file);
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
check_connectivity_cmd(const char* net_name,
                       bool floorplanning,
                       const char* error_file,
                       bool dont_require_bterm)
{
  return getPDNSim()->checkConnectivity(resolvePowerGridNet(net_name),
                                        floorplanning,
                                        error_file,
                                        !dont_require_bterm);
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
