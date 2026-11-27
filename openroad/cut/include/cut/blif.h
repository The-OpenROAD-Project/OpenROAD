// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <unistd.h>

#include <cstdio>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>

#include "odb/PtrSetMap.h"
#include "odb/db.h"

namespace ord {
class OpenRoad;
}  // namespace ord

namespace sta {
class dbSta;
class Pin;
}  // namespace sta

namespace cut {

struct BlifNetlist;
struct Gate;

class Blif
{
 public:
  Blif(utl::Logger* logger,
       sta::dbSta* sta,
       const std::string& const0_cell,
       const std::string& const0_cell_port,
       const std::string& const1_cell,
       const std::string& const1_cell_port,
       int call_id);
  void setReplaceableInstances(odb::PtrSet<odb::dbInst>& insts);
  void addReplaceableInstance(odb::dbInst* inst);
  bool writeBlif(const char* file_name, bool write_arrival_requireds = false);
  bool readBlif(const char* file_name, odb::dbBlock* block);
  bool inspectBlif(const char* file_name, int& num_instances);
  float getRequiredTime(sta::Pin* term, bool is_rise);
  float getArrivalTime(sta::Pin* term, bool is_rise);
  void addArrival(sta::Pin* pin, const std::string& net_name);
  void addRequired(sta::Pin* pin, const std::string& net_name);

 private:
  // Returns inst's .gate/.mlatch line, or nullopt for a sequential cell
  // without exactly one clock. Records inst's nets in netlist.
  std::optional<std::string> gateLine(odb::dbInst* inst,
                                      BlifNetlist& netlist,
                                      int& dummy_nets);
  // Records net (connected to iterm/pin) as a primary input, output, or
  // constant of the cut, if it is one.
  void classifyNet(odb::dbITerm* iterm,
                   sta::Pin* pin,
                   odb::dbNet* net,
                   const std::string& net_name,
                   BlifNetlist& netlist);
  void writeNetlist(std::ostream& f,
                    const BlifNetlist& netlist,
                    bool write_arrival_requireds) const;
  // Creates the instance for one parsed gate and connects it.
  void insertGate(const Gate& gate,
                  odb::dbBlock* block,
                  std::map<std::string, int>& inst_ids);
  // Inserts the tie cell for a parsed _const0_/_const1_ gate.
  void insertTieCell(const Gate& gate,
                     odb::dbBlock* block,
                     std::map<std::string, int>& inst_ids);
  std::string uniqueInstName(const std::string& master_name,
                             odb::dbBlock* block,
                             std::map<std::string, int>& inst_ids) const;
  odb::dbNet* findOrCreateNet(const std::string& net_name,
                              odb::dbBlock* block) const;

  odb::PtrSet<odb::dbInst> instances_to_optimize_;
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
