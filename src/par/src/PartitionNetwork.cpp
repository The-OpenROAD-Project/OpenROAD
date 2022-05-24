/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <map>
#include <set>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"
#include "sta/MakeConcreteNetwork.hh"
#include "sta/ParseBus.hh"
#include "sta/PortDirection.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

namespace par {

using utl::PAR;

using sta::Cell;
using sta::CellPortBitIterator;
using sta::ConcreteNetwork;
using sta::Instance;
using sta::InstancePinIterator;
using sta::Library;
using sta::Net;
using sta::NetPinIterator;
using sta::NetTermIterator;
using sta::Network;
using sta::NetworkReader;
using sta::Pin;
using sta::Port;
using sta::PortDirection;
using sta::Term;
using sta::writeVerilog;

using sta::isBusName;
using sta::parseBusName;

using sta::dbNetwork;
using sta::dbSta;

using odb::dbBlock;
using odb::dbInst;
using odb::dbIntProperty;

// determine the required direction of a port.
static PortDirection* determinePortDirection(const Net* net,
                                             const std::set<Instance*>* insts,
                                             const dbNetwork* db_network)
{
  bool local_only = true;
  bool locally_driven = false;
  bool externally_driven = false;

  NetTermIterator* term_iter = db_network->termIterator(net);
  while (term_iter->hasNext()) {
    Term* term = term_iter->next();
    PortDirection* dir = db_network->direction(db_network->pin(term));
    if (dir->isAnyInput()) {
      externally_driven = true;
    }
    local_only = false;
  }
  delete term_iter;

  if (insts != nullptr) {
    NetPinIterator* pin_iter = db_network->pinIterator(net);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      const PortDirection* dir = db_network->direction(pin);
      Instance* inst = db_network->instance(pin);

      if (insts->find(inst) == insts->end()) {
        local_only = false;
        if (dir->isAnyOutput()) {
          externally_driven = true;
        }
      } else {
        if (dir->isAnyOutput()) {
          locally_driven = true;
        }
      }
    }
    delete pin_iter;
  }

  // no port is needed
  if (local_only)
    return nullptr;

  if (locally_driven && externally_driven) {
    return PortDirection::bidirect();
  } else if (externally_driven) {
    return PortDirection::input();
  } else {
    return PortDirection::output();
  }
}

// find the correct brackets used in the liberty libraries.
static void determineLibraryBrackets(const dbNetwork* db_network,
                                     char* left,
                                     char* right)
{
  *left = '[';
  *right = ']';

  sta::LibertyLibraryIterator* lib_iter = db_network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    const sta::LibertyLibrary* lib = lib_iter->next();
    *left = lib->busBrktLeft();
    *right = lib->busBrktRight();
  }
  delete lib_iter;
}

// Builds an instance/cell of a partition
Instance* PartitionMgr::buildPartitionedInstance(
    const char* name,
    const char* port_prefix,
    sta::Library* library,
    sta::NetworkReader* network,
    sta::Instance* parent,
    const std::set<Instance*>* insts,
    std::map<Net*, Port*>* port_map)
{
  // build cell
  Cell* cell = network->makeCell(library, name, false, nullptr);

  // add global ports
  InstancePinIterator* pin_iter
      = db_network_->pinIterator(db_network_->topInstance());
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();

    bool add_port = false;
    Net* net = db_network_->net(db_network_->term(pin));
    if (net != nullptr) {
      NetPinIterator* net_pin_iter = db_network_->pinIterator(net);
      while (net_pin_iter->hasNext()) {
        // check if port is connected to instance in this partition
        if (insts->find(db_network_->instance(net_pin_iter->next()))
            != insts->end()) {
          add_port = true;
          break;
        }
      }
      delete net_pin_iter;
    }

    if (add_port) {
      const char* portname = db_network_->name(pin);

      Port* port = network->makePort(cell, portname);
      // copy exactly the parent port direction
      network->setDirection(port, db_network_->direction(pin));
      PortDirection* sub_module_dir
          = determinePortDirection(net, insts, db_network_);
      if (sub_module_dir != nullptr)
        network->setDirection(port, sub_module_dir);

      port_map->insert({net, port});
    }
  }
  delete pin_iter;

  // make internal ports for partitions and if port is not needed.
  std::set<Net*> local_nets;
  for (Instance* inst : *insts) {
    InstancePinIterator* pin_iter = db_network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      Net* net = db_network_->net(pin_iter->next());
      if (net != nullptr &&                          // connected
          port_map->find(net) == port_map->end() &&  // port not present
          local_nets.find(net) == local_nets.end()) {
        // check if connected to anything in a different partition
        NetPinIterator* net_pin_iter = db_network_->pinIterator(net);
        while (net_pin_iter->hasNext()) {
          Net* net = db_network_->net(net_pin_iter->next());
          PortDirection* port_dir
              = determinePortDirection(net, insts, db_network_);
          if (port_dir == nullptr) {
            local_nets.insert(net);
            continue;
          }
          std::string port_name = port_prefix;
          port_name += db_network_->name(net);

          Port* port = network->makePort(cell, port_name.c_str());
          network->setDirection(port, port_dir);

          port_map->insert({net, port});
          break;
        }
        delete net_pin_iter;
      }
    }
    delete pin_iter;
  }

  // loop over buses and to ensure all bit ports are created, only needed for
  // partitioned modules
  char path_escape = db_network_->pathEscape();
  char left_bracket;
  char right_bracket;
  determineLibraryBrackets(db_network_, &left_bracket, &right_bracket);
  std::map<std::string, std::vector<Port*>> port_buses;
  for (auto& [net, port] : *port_map) {
    std::string portname = network->name(port);

    // check if bus and get name
    if (isBusName(portname.c_str(), left_bracket, right_bracket, path_escape)) {
      char* bus_name;
      int idx;
      parseBusName(portname.c_str(),
                   left_bracket,
                   right_bracket,
                   path_escape,
                   bus_name,
                   idx);
      portname = bus_name;
      delete[] bus_name;

      port_buses[portname].push_back(port);
    }
  }
  for (auto& [bus, ports] : port_buses) {
    std::set<int> port_idx;
    std::set<PortDirection*> port_dirs;
    for (Port* port : ports) {
      char* bus_name;
      int idx;
      parseBusName(network->name(port),
                   left_bracket,
                   right_bracket,
                   path_escape,
                   bus_name,
                   idx);
      delete[] bus_name;

      port_idx.insert(idx);

      port_dirs.insert(network->direction(port));
    }

    // determine real direction of port
    PortDirection* overall_direction = nullptr;
    if (port_dirs.size() == 1)  // only one direction is used.
      overall_direction = *port_dirs.begin();
    else
      overall_direction = PortDirection::bidirect();

    // set port direction to match
    for (Port* port : ports)
      network->setDirection(port, overall_direction);

    // fill in missing ports in bus
    const auto [min_idx, max_idx]
        = std::minmax_element(port_idx.begin(), port_idx.end());
    for (int idx = *min_idx; idx <= *max_idx; idx++) {
      if (port_idx.find(idx) == port_idx.end()) {
        // build missing port
        std::string portname = bus;
        portname += left_bracket + std::to_string(idx) + right_bracket;
        Port* port = network->makePort(cell, portname.c_str());
        network->setDirection(port, overall_direction);
      }
    }
  }

  network->groupBusPorts(cell, [](const char*) { return true; });

  // build instance
  std::string instname = name;
  instname += "_inst";
  Instance* inst = network->makeInstance(cell, instname.c_str(), parent);

  // create nets for ports in cell
  for (auto& [db_net, port] : *port_map) {
    Net* net = network->makeNet(network->name(port), inst);
    Pin* pin = network->makePin(inst, port, nullptr);
    network->makeTerm(pin, net);
  }

  // create and connect instances
  for (Instance* instance : *insts) {
    Instance* leaf_inst = network->makeInstance(
        db_network_->cell(instance), db_network_->name(instance), inst);

    InstancePinIterator* pin_iter = db_network_->pinIterator(instance);
    while (pin_iter->hasNext()) {
      Pin* pin = pin_iter->next();
      Net* net = db_network_->net(pin);
      if (net != nullptr) {  // connected
        Port* port = db_network_->port(pin);

        // check if connected to a port
        auto port_find = port_map->find(net);
        if (port_find != port_map->end()) {
          Net* new_net
              = network->findNet(inst, network->name(port_find->second));
          network->connect(leaf_inst, port, new_net);
        } else {
          Net* new_net = network->findNet(inst, db_network_->name(net));
          if (new_net == nullptr)
            new_net = network->makeNet(db_network_->name(net), inst);
          network->connect(leaf_inst, port, new_net);
        }
      }
    }
    delete pin_iter;
  }

  return inst;
}

// Builds an instance/cell instantiating the partitions
Instance* PartitionMgr::buildPartitionedTopInstance(const char* name,
                                                    sta::Library* library,
                                                    sta::NetworkReader* network)
{
  // build cell
  Cell* cell = network->makeCell(library, name, false, nullptr);

  // add global ports
  InstancePinIterator* pin_iter
      = db_network_->pinIterator(db_network_->topInstance());
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();

    const char* portname = db_network_->name(pin);
    Port* port = network->makePort(cell, portname);
    network->setDirection(port, db_network_->direction(pin));
  }
  delete pin_iter;

  network->groupBusPorts(cell, [](const char*) { return true; });

  // build instance
  std::string instname = name;
  instname += "_inst";
  Instance* inst = network->makeInstance(cell, instname.c_str(), nullptr);

  CellPortBitIterator* port_iter = network->portBitIterator(cell);
  while (port_iter->hasNext()) {
    Port* port = port_iter->next();
    const char* port_name = network->name(port);
    Net* net = network->makeNet(port_name, inst);
    Pin* pin = network->makePin(inst, port, nullptr);
    network->makeTerm(pin, net);
  }
  delete port_iter;

  return inst;
}

void PartitionMgr::writePartitionVerilog(const char* path,
                                         const char* port_prefix,
                                         const char* module_suffix)
{
  dbBlock* block = getDbBlock();
  if (block == nullptr)
    return;

  logger_->report("Writing partition to verilog.");
  // get top module name
  const std::string top_name = db_network_->name(db_network_->topInstance());

  // build partition instance map
  std::map<long, std::set<Instance*>> instance_map;
  for (dbInst* inst : block->getInsts()) {
    dbIntProperty* prop_id = dbIntProperty::find(inst, "partition_id");
    if (!prop_id) {
      logger_->warn(PAR,
                    15,
                    "Property 'partition_id' not found for inst {}.",
                    inst->getName());
    } else {
      const long partition = prop_id->getValue();
      instance_map[partition].insert(db_network_->dbToSta(inst));
    }
  }

  // create new network and library
  NetworkReader* network = sta::makeConcreteNetwork();
  Library* library = network->makeLibrary("Partitions", nullptr);

  // new top module
  Instance* top_inst
      = buildPartitionedTopInstance(top_name.c_str(), library, network);

  // build submodule partitions
  std::map<long, Instance*> sta_instance_map;
  std::map<long, std::map<Net*, Port*>> sta_port_map;
  for (auto& [partition, instances] : instance_map) {
    const std::string cell_name
        = top_name + module_suffix + std::to_string(partition);
    sta_instance_map[partition]
        = buildPartitionedInstance(cell_name.c_str(),
                                   port_prefix,
                                   library,
                                   network,
                                   top_inst,
                                   &instances,
                                   &sta_port_map[partition]);
  }

  // connect submodule partitions in new top module
  for (auto& [partition, instance] : sta_instance_map) {
    for (auto& [portnet, port] : sta_port_map[partition]) {
      const char* net_name = network->name(port);

      Net* net = network->findNet(top_inst, net_name);
      if (net == nullptr)
        net = network->makeNet(net_name, top_inst);

      network->connect(instance, port, net);
    }
  }

  reinterpret_cast<ConcreteNetwork*>(network)->setTopInstance(top_inst);

  writeVerilog(path, true, false, {}, network);

  delete network;
}

}  // namespace par
