// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <unistd.h>

#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "odb/db.h"

namespace ord {
class OpenRoad;
}  // namespace ord

namespace sta {
class dbSta;
class Pin;
}  // namespace sta

namespace cut {

class Blif
{
 public:
  Blif(utl::Logger* logger,
       sta::dbSta* sta,
       const std::string& const0_cell_,
       const std::string& const0_cell_port_,
       const std::string& const1_cell_,
       const std::string& const1_cell_port_,
       int call_id_);
  void setReplaceableInstances(std::set<odb::dbInst*>& insts);
  void addReplaceableInstance(odb::dbInst* inst);
  bool writeBlif(const char* file_name, bool write_arrival_requireds = false);
  bool readBlif(const char* file_name, odb::dbBlock* block);
  bool inspectBlif(const char* file_name, int& num_instances);
  float getRequiredTime(sta::Pin* term, bool is_rise);
  float getArrivalTime(sta::Pin* term, bool is_rise);
  void addArrival(sta::Pin* pin, const std::string& netName);
  void addRequired(sta::Pin* pin, const std::string& netName);

 private:
  std::set<odb::dbInst*> instances_to_optimize_;
  utl::Logger* logger_;
  sta::dbSta* open_sta_ = nullptr;
  std::string const0_cell_;
  std::string const0_cell_port_;
  std::string const1_cell_;
  std::string const1_cell_port_;
  std::map<std::string, std::pair<float, float>> requireds_;
  std::map<std::string, std::pair<float, float>> arrivals_;
  int call_id_;
};

}  // namespace cut
