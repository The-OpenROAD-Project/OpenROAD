/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "PartitionMgr.h"

#include "opendb/db.h"
#include "utl/Logger.h"
#include "ord/OpenRoad.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"

#include "sta/NetworkClass.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/ConcreteLibrary.hh"
#include "sta/VerilogWriter.hh"
#include "sta/VerilogWriter.hh"
#include "sta/PortDirection.hh"
#include "sta/ParseBus.hh"

#include <map>
#include <set>
#include <algorithm>

namespace par {

using utl::PAR;

using sta::Network;
using sta::ConcreteNetwork;
using sta::ConcreteLibrary;
using sta::writeVerilog;
using sta::ConcreteCell;
using sta::ConcretePort;
using sta::ConcreteInstance;
using sta::Cell;
using sta::Instance;
using sta::Net;
using sta::Port;
using sta::Pin;
using sta::PortDirection;
using sta::ConcretePin;
using sta::ConcreteNet;

using sta::isBusName;
using sta::parseBusName;

using sta::dbNetwork;

using odb::dbInst;
using odb::dbIntProperty;
using odb::dbBlock;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbNet;
using odb::dbIoType;

PortDirection*
determinePortDirection(dbNet* net, std::set<dbInst*>* insts) {
  bool localOnly = true;
  bool locallyDriven = false;
  bool externallyDriven = false;

  for (dbBTerm* bterm : net->getBTerms()) {
    switch (bterm->getIoType()) {
    case dbIoType::INPUT:
      externallyDriven = true;
      localOnly = false;
      break;
    case dbIoType::INOUT:
      externallyDriven = true;
      localOnly = false;
      break;
    }
  }

  if (insts != nullptr) {
     for (dbITerm* iterm : net->getITerms()) {
      dbInst* inst = iterm->getInst();

      // check if instance is not present in the current partition, port will be needed
      if (insts->find(inst) == insts->end()) {
        localOnly = false;
        // instance on other partition, so iterm has opposite direction
        switch (iterm->getIoType()) {
        case dbIoType::OUTPUT:
          externallyDriven = true;
          break;
        case dbIoType::INOUT:
          externallyDriven = true;
          break;
        }
      }
      else {
        // instance in partition, so iterm has correct port direction
        switch (iterm->getIoType()) {
        case dbIoType::OUTPUT:
          locallyDriven = true;
          break;
        case dbIoType::INOUT:
          locallyDriven = true;
          break;
        }
      }
    }
  }

  // no port is needed
  if (localOnly)
    return nullptr;

  if (locallyDriven && externallyDriven) {
    return PortDirection::bidirect();
  } else if (externallyDriven) {
    return PortDirection::input();
  } else { // internally driven
    return PortDirection::output();
  }
}

Instance*
PartitionMgr::buildPartitionedInstance(
    const char* name,
    const char* portPrefix,
    sta::ConcreteLibrary* lib,
    sta::ConcreteNetwork* network,
    sta::Instance* parent,
    std::set<dbInst*>* insts,
    std::map<dbNet*, ConcretePort*>* portMap) {

  // setup access
  dbNetwork* db_network = ord::OpenRoad::openRoad()->getSta()->getDbNetwork();
  dbBlock* block = getDbBlock();

  // make cell and instance
  ConcreteCell* cell = lib->makeCell(name, false, "");
  std::string instname = name;
  instname += "_inst";
  Instance* inst = network->makeInstance(reinterpret_cast<Cell*>(cell), instname.c_str(), parent);

  Instance* topCellInst = db_network->dbToSta(block->getParentInst());

  std::map<dbNet*, Net*> netMap;
  // add global ports
  for (dbBTerm* bterm : block->getBTerms()) {
    bool addPort = parent == nullptr; // add port if parent
    dbNet* net = bterm->getNet();
    if (net != nullptr && insts != nullptr) {
      for (dbITerm* iterm : bterm->getNet()->getITerms()) {
        if (addPort)
          break;

        // check if port is connected to instance in this partition
        if (insts->find(iterm->getInst()) != insts->end()) {
          addPort = true;
          break;
        }
      }
    }

    if (addPort) {
      std::string portname = bterm->getName();

      ConcretePort* port = cell->makePort(portname.c_str());
      // copy exactly the parent port direction
      port->setDirection(db_network->dbToSta(bterm->getSigType(), bterm->getIoType()));
      if (parent != nullptr) {
        PortDirection* submoddir = determinePortDirection(bterm->getNet(), insts);
        if (submoddir != nullptr)
          port->setDirection(submoddir);
      }

      if (portMap != nullptr)
        portMap->insert({net, port});
      netMap[net] = network->makeNet(portname.c_str(), inst);
    }
  }

  // make internal ports for partitions and if port is not needed, make net instead.
  if (insts != nullptr) {
    for (dbInst* dbinst : *insts) {
      for (dbITerm* term : dbinst->getITerms()) {
        dbNet* dbnet = term->getNet();
        if (dbnet == nullptr) // not connected
          continue;

        // port already present
        if (portMap->find(dbnet) != portMap->end())
          continue;

        // check if connected to anything in a different partition
        bool addedInternalPort = false;
        for (dbITerm* iterms : dbnet->getITerms()) {
          if (addedInternalPort)
            break;

          PortDirection* portdir = determinePortDirection(dbnet, insts);
          if (portdir != nullptr) {
            std::string portName = portPrefix;
            portName += dbnet->getName();

            ConcretePort* port = cell->makePort(portName.c_str());
            port->setDirection(portdir);

            portMap->insert({dbnet, port});
            netMap[dbnet] = network->makeNet(portName.c_str(), inst);

            addedInternalPort = true;
            break;
          }
        }
        if (addedInternalPort) // added port, no need for net
          continue;

        // add net
        if (netMap.find(dbnet) != netMap.end()) // check if net is there.
          continue;

        netMap[dbnet] = network->makeNet(dbnet->getName().c_str(), inst);
      }
    }

    // create and connect instances
    for (dbInst* dbinst : *insts) {
      Instance* leafInst = network->makeInstance(db_network->dbToSta(dbinst->getMaster()),
                                                 dbinst->getName().c_str(),
                                                 inst);
      for (dbITerm* term : dbinst->getITerms()) {
        dbNet* dbnet = term->getNet();
        if (dbnet == nullptr) // not connected
          continue;

        auto netFind = netMap.find(dbnet);
        if (netFind != netMap.end())
          network->connect(leafInst, db_network->dbToSta(term->getMTerm()), netFind->second);
      }
    }
  }

  if (parent != nullptr) {
    // loop over buses and to ensure all bit ports are created, only needed for partitioned modules
    char pathEscape = network->pathEscape();
    char leftBracket = lib->busBrktLeft();
    char rightBracket = lib->busBrktRight();
    std::map<std::string, std::vector<ConcretePort*>> portBuses;
    for (auto& [net, port] : *portMap) {
      std::string portname = reinterpret_cast<ConcretePort*>(port)->name();

      // check if bus and get name
      if (isBusName(portname.c_str(), leftBracket, rightBracket, pathEscape)) {
        char* bus_name;
        int idx;
        parseBusName(portname.c_str(), leftBracket, rightBracket, pathEscape, bus_name, idx);
        portname = bus_name;
        delete bus_name;

        if (portBuses.find(portname) == portBuses.end()) {
          portBuses[portname] = std::vector<ConcretePort*>();
        }
        portBuses[portname].push_back(port);
      }
    }
    for (auto& [bus, ports] : portBuses) {
      std::set<int> portIdx;
      std::set<PortDirection*> portDirs;
      for (ConcretePort* port : ports) {
        char* bus_name;
        int idx;
        parseBusName(port->name(), leftBracket, rightBracket, pathEscape, bus_name, idx);
        delete bus_name;

        portIdx.insert(idx);

        portDirs.insert(port->direction());
      }

      // determine real direction of port
      PortDirection* overallDirection = nullptr;
      if (portDirs.size() == 1) // only one direction is used.
        overallDirection = *portDirs.begin();
      else
        overallDirection = PortDirection::bidirect();

      // set port direction to match
      for (ConcretePort* port : ports)
        port->setDirection(overallDirection);

      // fill in missing ports in bus
      const auto [minIdx, maxIdx] = std::minmax_element(portIdx.begin(), portIdx.end());
      for (int idx = *minIdx; idx <= *maxIdx; idx++) {
        if (portIdx.find(idx) == portIdx.end()) {
          // build missing port
          std::string portname = bus;
          portname += leftBracket + std::to_string(idx) + rightBracket;
          ConcretePort* port = cell->makePort(portname.c_str());
          port->setDirection(overallDirection);
        }
      }
    }
  }

  network->groupBusPorts(reinterpret_cast<Cell*>(cell));
  reinterpret_cast<ConcreteInstance*>(inst)->initPins();
  network->makePins(inst);

  return inst;
}

void PartitionMgr::writePartitioningToVerilog(const char* path,
                                              const char* portPrefix,
                                              const char* moduleSuffix) {
  dbBlock* block = getDbBlock();
  if (block == nullptr)
    return;

  _logger->report("Writing partition to verilog.");

  // build partition instance map
  std::map<long, std::set<dbInst*>> instanceMap;
  for (dbInst* inst : block->getInsts()) {
    dbIntProperty* propId = dbIntProperty::find(inst, "partition_id");
    if (!propId) {
      _logger->warn(PAR, 15, "Property not found for inst {}", inst->getName());
      continue;
    }

    long partition = propId->getValue();
    if (instanceMap.find(partition) == instanceMap.end())
      instanceMap.emplace(partition, std::set<dbInst*>());

    instanceMap[partition].insert(inst);
  }

  dbNetwork* db_network = ord::OpenRoad::openRoad()->getSta()->getDbNetwork();
  std::string topName = db_network->name(db_network->topInstance());

  // create new network and library
  ConcreteNetwork* network = new ConcreteNetwork();
  ConcreteLibrary* partLib = reinterpret_cast<ConcreteLibrary*>(network->makeLibrary("Partitions", ""));

  // new top module
  Instance* topInst = buildPartitionedInstance(topName.c_str(),
                                               "", // no changes to port
                                               partLib,
                                               network,
                                               nullptr, // no parent
                                               nullptr,
                                               nullptr);
  ConcreteInstance* ctopInst = reinterpret_cast<ConcreteInstance*>(topInst);
  network->setTopInstance(topInst);

  // build submodule partitions
  std::map<long, Instance*> staInstanceMap;
  std::map<long, std::map<dbNet*, ConcretePort*>> staPortMap;
  for (auto& [partition, instances] : instanceMap) {
    std::string cellName = topName + moduleSuffix + std::to_string(partition);
    staPortMap[partition] = std::map<dbNet*, ConcretePort*>();
    staInstanceMap[partition] = buildPartitionedInstance(cellName.c_str(),
                                                         portPrefix,
                                                         partLib,
                                                         network,
                                                         topInst,
                                                         &instances,
                                                         &staPortMap[partition]);
  }

  // connect submodule partitions in new top module
  for (auto& [partition, instance] : staInstanceMap) {
    ConcreteInstance* cinst = reinterpret_cast<ConcreteInstance*>(instance);
    ConcreteCell* ccell = reinterpret_cast<ConcreteCell*>(cinst->cell());

    for (auto& [portnet, cport] : staPortMap[partition]) {
      Port* port = reinterpret_cast<Port*>(cport);

      dbBTerm* bterm = block->findBTerm(portnet->getName().c_str());
      if (bterm != nullptr) { // global connection
        Net* net = reinterpret_cast<Net*>(ctopInst->findNet(portnet->getName().c_str()));
        network->connect(instance, port, net);
      }
      else { // partition connections
        Net* net = reinterpret_cast<Net*>(ctopInst->findNet(cport->name()));
        if (net == nullptr)
          net = network->makeNet(cport->name(), topInst);

        network->connect(instance, port, net);
      }
    }
  }

  writeVerilog(path, false, false, {}, reinterpret_cast<Network*>(network));

  delete network;
}

}  // namespace par
