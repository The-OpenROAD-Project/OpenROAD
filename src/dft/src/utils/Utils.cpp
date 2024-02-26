///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "Utils.hh"

#include <iostream>
#include <optional>
#include <sta/Sta.hh>

#include "db_sta/dbNetwork.hh"

//#define DEBUG_UTILS
namespace dft::utils {

namespace {

void PopulatePortNameToNet(
    odb::dbInst* instance,
    std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net)
{
  for (odb::dbITerm* iterm : instance->getITerms()) {
    port_name_to_net.emplace_back(iterm->getMTerm()->getName(),
                                  iterm->getNet());
  }
}

void PopulatePortNameToModNet(
    odb::dbInst* instance,
    std::vector<std::tuple<std::string, odb::dbModNet*>>& port_name_to_modnet)
{
  for (odb::dbITerm* iterm : instance->getITerms()) {
    port_name_to_modnet.emplace_back(iterm->getMTerm()->getName(),
                                     iterm->getModNet());
  }
}

void ConnectPinsToNets(
    odb::dbInst* instance,
    const std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  for (const auto& [port_name_old, net] : port_name_to_net) {
    if (net == nullptr) {
      continue;
    }
    std::string port_name_new = port_mapping.find(port_name_old)->second;
#ifdef DEBUG_UTILS
    printf("Setting up iterm %s with net %s\n",
           instance->findITerm(port_name_new.c_str())->getName().c_str(),
           net->getName().c_str());
#endif
    instance->findITerm(port_name_new.c_str())->connect(net);
  }
}

void ConnectPinsToModNets(
    odb::dbInst* instance,
    const std::vector<std::tuple<std::string, odb::dbModNet*>>&
        port_name_to_net,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  for (const auto& [port_name_old, modnet] : port_name_to_net) {
    if (modnet == nullptr) {
      continue;
    }
    std::string port_name_new = port_mapping.find(port_name_old)->second;
#ifdef DEBUG_UTILS
    printf("Setting up iterm %s with modnet %s\n",
           instance->findITerm(port_name_new.c_str())->getName().c_str(),
           modnet->getName());
#endif
    instance->findITerm(port_name_new.c_str())->connect(modnet);
  }
}
}  // namespace

bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance)
{
  odb::dbMaster* master = instance->getMaster();
  sta::Cell* master_cell = db_network->dbToSta(master);
  sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);
  return liberty_cell->hasSequentials();
}

odb::dbInst* ReplaceCell(
    sta::dbSta* sta,
    sta::dbNetwork* db_network,
    odb::dbBlock* top_block,
    odb::dbInst* old_instance,
    odb::dbMaster* new_master,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
#ifdef DEBUG_UTILS
  static int debug;
  debug++;
#endif
  std::vector<odb::dbITerm*> clock_pin = GetClockPin(old_instance);
  odb::dbModule* inst_module = old_instance->getModule();

  // Check have we got a clock pin accessible from the instane
  if (clock_pin.size() != 0) {
#ifdef DEBUG_UTILS
    printf("Hacking cell with clock %s with clock pin %p %s\n",
           old_instance->getName().c_str(),
           clock_pin.at(0),
           clock_pin.at(0)->getName().c_str());
    printf("Containing module %s\n", inst_module->getName());
#endif

    // Check is that dbiterm in the clock set for the sta.
#ifdef DEBUG_UTILS
    sta::Pin* clock_p = db_network->dbToSta(clock_pin.at(0));  // iterm -> pin
    printf("Clock pin %s\n", db_network->name(clock_p));
#endif
    const sta::ClockSet clock_set
        = sta->clocks(db_network->dbToSta(clock_pin.at(0)));
#ifdef DEBUG_UTILS
    if (clock_set.size() == 0)
      printf("Error cannot get clock pin\n");
    else
      printf("ok got clock pin\n");
#endif
  }

  std::vector<std::tuple<std::string, odb::dbNet*>> port_name_to_net;
  PopulatePortNameToNet(old_instance, port_name_to_net);

  std::vector<std::tuple<std::string, odb::dbModNet*>> port_name_to_modnet;
  PopulatePortNameToModNet(old_instance, port_name_to_modnet);

  odb::dbInst* new_instance
      = odb::dbInst::create(top_block,
                            new_master,
                            /*name=*/"tmp_scan_flop",
                            false,
                            inst_module /* put in module in hierarchy */
      );
  std::string old_cell_name = old_instance->getName();

  // Delete the old cell
  odb::dbInst::destroy(old_instance);

  // Connect the new cell to the old instance's nets
  ConnectPinsToNets(new_instance, port_name_to_net, port_mapping);
  ConnectPinsToModNets(new_instance, port_name_to_modnet, port_mapping);

  // Rename as the old cell
  new_instance->rename(old_cell_name.c_str());

#ifdef DEBUG_UTILS
  std::vector<odb::dbITerm*> clock_pin_new = GetClockPin(new_instance);
  if (clock_pin_new.size() != 0) {
    printf("Successfully hacked cell %s with clock\n",
           new_instance->getName().c_str());
    printf("Clock dbITerm %p %s\n",
           clock_pin_new.at(0),
           clock_pin_new.at(0)->getName().c_str());
  }
#endif
  return new_instance;
}

std::vector<odb::dbITerm*> GetClockPin(odb::dbInst* inst)
{
  std::vector<odb::dbITerm*> clocks;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      clocks.push_back(iterm);
    }
  }
  return clocks;
}

std::optional<sta::Clock*> GetClock(sta::dbSta* sta, odb::dbITerm* iterm)
{
#ifdef DEBUG_UTILS
  static int debug;
  debug++;
  printf("Getting clock for iterm %s %p\n", iterm->getName('/').c_str(), iterm);
#endif
  const sta::dbNetwork* db_network = sta->getDbNetwork();
  // but note that db network is wrong in the hierarchy.
  // we are somewhere deep in the hierarchy.
  const sta::ClockSet clock_set = sta->clocks(db_network->dbToSta(iterm));

  odb::dbInst* instance = (odb::dbInst*) (iterm->getInst());
  std::vector<odb::dbITerm*> clock_pin = GetClockPin(instance);
#ifdef DEBUG_UTILS
  if (clock_pin.size() != 0)
    printf("Success got clock %s for iterm (%p) from instance\n",
           clock_pin.at(0)->getName().c_str(),
           clock_pin.at(0));
  else
    printf("Failure no clock found\n");
#endif

  sta::ClockSet::ConstIterator iter(clock_set);
  if (!iter.container()->empty()) {
    // Returns the first clock for the given iterm, TODO can we have more than
    // one clock driver?
#ifdef DEBUG_UTILS
    printf("Success -- found clock\n");
#endif
    return *iter.container()->begin();
  } else {
#ifdef DEBUG_UTILS
    printf("D %d Failure, no clock for iterm %p in sta\n", debug, iterm);
#endif
    ;
  }
  return std::nullopt;
}

}  // namespace dft::utils
