// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "ord/Tech.h"

#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Scene.hh"
#include "sta/Units.hh"
#include "tcl.h"

namespace ord {

Tech::Tech(Tcl_Interp* interp,
           const char* log_filename,
           const char* metrics_filename)
    : app_(new OpenRoad())
{
  if (!interp) {
    interp = Tcl_CreateInterp();
    Tcl_Init(interp);
    app_->init(interp, log_filename, metrics_filename, false);
  }
}

Tech::~Tech()
{
  delete app_;
}

odb::dbDatabase* Tech::getDB()
{
  return app_->getDb();
}

odb::dbTech* Tech::getTech()
{
  return getDB()->getTech();
}

sta::dbSta* Tech::getSta()
{
  return app_->getSta();
}

void Tech::readLef(const std::string& file_name)
{
  const bool make_tech = getDB()->getTech() == nullptr;
  const bool make_library = true;
  std::string lib_name = file_name;

  // Hacky but easier than dealing with stdc++fs linking
  auto slash_pos = lib_name.find_last_of('/');
  if (slash_pos != std::string::npos) {
    lib_name.erase(0, slash_pos + 1);
  }
  auto dot_pos = lib_name.find_last_of('.');
  if (dot_pos != std::string::npos) {
    lib_name.erase(lib_name.begin() + dot_pos, lib_name.end());
  }

  app_->readLef(
      file_name.c_str(), lib_name.c_str(), "", make_tech, make_library);
}

void Tech::readLiberty(const std::string& file_name)
{
  // TODO: take corner & min/max args
  getSta()->readLiberty(file_name.c_str(),
                        getSta()->cmdScene(),
                        sta::MinMaxAll::all(),
                        true /* infer_latches */);
}

float Tech::nominalProcess()
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  return lib->nominalProcess();
}

float Tech::nominalVoltage()
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  return lib->nominalVoltage();
}

float Tech::nominalTemperature()
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  return lib->nominalTemperature();
}

float Tech::timeScale()
{
  return getSta()->units()->timeUnit()->scale();
}

float Tech::resistanceScale()
{
  return getSta()->units()->resistanceUnit()->scale();
}

float Tech::capacitanceScale()
{
  return getSta()->units()->capacitanceUnit()->scale();
}

float Tech::voltageScale()
{
  return getSta()->units()->voltageUnit()->scale();
}

float Tech::currentScale()
{
  return getSta()->units()->currentUnit()->scale();
}

float Tech::powerScale()
{
  return getSta()->units()->powerUnit()->scale();
}

float Tech::distanceScale()
{
  return getSta()->units()->distanceUnit()->scale();
}

}  // namespace ord
