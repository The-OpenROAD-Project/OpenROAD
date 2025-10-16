// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace upf {

class UPFWriter
{
 public:
  using ArgList = std::vector<std::string>;

  UPFWriter(odb::dbBlock* block, utl::Logger* logger);

  void write(const std::string& file);

 private:
  void writeHeader();
  void writeTitle(const std::string& title);
  void writeDomain(odb::dbPowerDomain* domain);
  void writePort(odb::dbLogicPort* port);
  void writePowerSwitch(odb::dbPowerSwitch* ps);
  void writePowerSwitchPortMap(odb::dbPowerSwitch* ps);
  void writeIsolation(odb::dbIsolation* isolation);
  void writeLevelShifter(odb::dbLevelShifter* ls);

  void lineContinue();

  void writeList(const ArgList& list);

  odb::dbBlock* block_;
  utl::Logger* logger_;
  std::ofstream stream_;
};
}  // namespace upf
