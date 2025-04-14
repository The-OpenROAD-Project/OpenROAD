// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>

namespace odb {
class dbDatabase;
class dbTech;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

extern "C" {
struct Tcl_Interp;
}

namespace ord {

class OpenRoad;

class Tech
{
 public:
  // interp is only passed by standalone OR as it gets its
  // interpreter from Tcl_Main.
  Tech(Tcl_Interp* interp = nullptr,
       const char* log_filename = nullptr,
       const char* metrics_filename = nullptr);
  ~Tech();

  void readLef(const std::string& file_name);
  void readLiberty(const std::string& file_name);
  odb::dbDatabase* getDB();
  odb::dbTech* getTech();
  sta::dbSta* getSta();

  float nominalProcess();
  float nominalVoltage();
  float nominalTemperature();

  float timeScale();
  float resistanceScale();
  float capacitanceScale();
  float voltageScale();
  float currentScale();
  float powerScale();
  float distanceScale();

 private:
  OpenRoad* app_;

  friend class Design;
};

}  // namespace ord
