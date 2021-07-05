/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
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

#include "rmp/blif.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <streambuf>
#include <string>
#include <tuple>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/blifParser.h"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

using utl::RMP;

namespace rmp {

Blif::Blif(Logger* logger, sta::dbSta* sta,
           const std::string& const0_cell,
           const std::string& const0_cell_port,
           const std::string& const1_cell,
           const std::string& const1_cell_port)
    : const0_cell_(const0_cell),
      const0_cell_port_(const0_cell_port),
      const1_cell_(const1_cell),
      const1_cell_port_(const1_cell_port)
{
  logger_ = logger;
  open_sta_ = sta;
}

void Blif::setReplaceableInstances(std::set<odb::dbInst*>& insts)
{
  instances_to_optimize = insts;
}

void Blif::addReplaceableInstance(odb::dbInst* inst)
{
  instances_to_optimize.insert(inst);
}

bool Blif::writeBlif(const char* file_name)
{
  int dummy_nets = 0;

  std::ofstream f(file_name);

  if (f.bad()) {
    logger_->error(RMP, 1, "cannot open file {}", file_name);
    return false;
  }

  std::set<odb::dbInst*>& insts = this->instances_to_optimize;
  std::map<uint, odb::dbInst*> instMap;
  std::vector<std::string> subckts;
  std::set<std::string> inputs, outputs, const0, const1, clocks;

  subckts.resize(insts.size());
  int instIndex = 0;

  for (auto&& inst : insts) {
    instMap.insert(std::pair<uint, odb::dbInst*>(inst->getId(), inst));
  }

  for (auto&& inst : insts) {
    auto master = inst->getMaster();
    sta::LibertyCell* cell = open_sta_->getDbNetwork()->libertyCell(
        open_sta_->getDbNetwork()->dbToSta(master));
    auto masterName = master->getName();

    std::string currentGate
        = ((cell->hasSequentials()) ? ".mlatch " : ".gate ") + masterName;
    std::string currentConnections = "", currentClock = "";
    std::set<std::string> currentClocks;
    int totalOutPins = 0, totalOutConstPins = 0;

    auto iterms = inst->getITerms();

    for (auto&& iterm : iterms) {
      auto mterm = iterm->getMTerm();
      auto net = iterm->getNet();

      if (iterm->getSigType() == odb::dbSigType::POWER
          || iterm->getSigType() == odb::dbSigType::GROUND)
        continue;

      sta::Vertex *vertex, *bidirect_drvr_vertex;
      auto pin_ = open_sta_->getDbNetwork()->dbToSta(iterm);
      open_sta_->getDbNetwork()->graph()->pinVertices(
          pin_, vertex, bidirect_drvr_vertex);
      sta::LogicValue pinVal
          = ((vertex)
                 ? vertex->simValue()
                 : ((bidirect_drvr_vertex) ? bidirect_drvr_vertex->simValue()
                                           : sta::LogicValue::unknown));
      auto network_ = open_sta_->network();
      auto port_ = network_->libertyPort(pin_);
      if (port_->isClock()) {
        if (net == NULL)
          continue;
        clocks.insert(net->getName());
        currentClocks.insert(net->getName());
        currentClock = net->getName();
        continue;
      }

      auto mtermName = mterm->getName();
      auto netName = (net == NULL) ? ("dummy_" + std::to_string(dummy_nets++))
                                   : net->getName();

      currentConnections += " " + mtermName + "=" + netName;

      if (net == NULL)
        continue;
      // check whether connected net is input/output
      // If it's only connected to one Iterm OR
      // It's connected to another instance that's outside the bubble
      auto connectedIterms = net->getITerms();

      if (connectedIterms.size() == 1) {
        if (iterm->getIoType() == odb::dbIoType::INPUT)
          inputs.insert(netName);
        else if (iterm->getIoType() == odb::dbIoType::OUTPUT)
          outputs.insert(netName);

      } else {
        bool addAsInput = false;
        for (auto&& connectedIterm : connectedIterms) {
          auto connectedInstId = connectedIterm->getInst()->getId();

          if (instMap.find(connectedInstId) == instMap.end()) {
            // Net is connected to an instance outside the cut out region
            // Check whether it's input or output
            if (iterm->getIoType() == odb::dbIoType::INPUT) {
              // Net is connected to a pin outside the bubble and should be
              // treated as an input If the driving pin is contant then we'll
              // add a constant gate in blif otherwise just add the net as input
              auto pin_ = open_sta_->getDbNetwork()->dbToSta(connectedIterm);
              auto network_ = open_sta_->network();
              auto port_ = network_->libertyPort(pin_);

              if (port_) {
                auto expr = port_->function();
                if (expr
                    // Tristate outputs do not force the output to be constant.
                    && port_->tristateEnable() == nullptr
                    && (expr->op() == sta::FuncExpr::op_zero
                        || expr->op() == sta::FuncExpr::op_one)) {
                  if (expr->op() == sta::FuncExpr::op_zero) {
                    if (!const0.size()) {
                      const0_cell_ = port_->libertyCell()->name();
                      const0_cell_port_ = port_->name();
                    }
                    const0.insert(netName);
                  } else {
                    if (!const1.size()) {
                      const1_cell_ = port_->libertyCell()->name();
                      const1_cell_port_ = port_->name();
                    }
                    const1.insert(netName);
                  }

                } else {
                  addAsInput = true;
                }
              } else {
                addAsInput = true;
              }

            } else if (iterm->getIoType() == odb::dbIoType::OUTPUT)
              outputs.insert(netName);
          }
        }
        if (addAsInput && const0.find(netName) == const0.end()
            && const1.find(netName) == const1.end())
          inputs.insert(netName);
      }

      // connect to original ports if not inferred already
      if (inputs.find(netName) == inputs.end()
          && outputs.find(netName) == outputs.end()
          && const0.find(netName) == const0.end()
          && const1.find(netName) == const1.end()) {
        auto connectedPorts = net->getBTerms();
        for (auto connectedPort : connectedPorts) {
          if (connectedPort->getIoType() == odb::dbIoType::INPUT) {
            auto pin_ = open_sta_->getDbNetwork()->dbToSta(connectedPort);
            auto network_ = open_sta_->network();
            auto port_ = network_->libertyPort(pin_);

            if (port_) {
              auto expr = port_->function();
              if (expr
                  // Tristate outputs do not force the output to be constant.
                  && port_->tristateEnable() == nullptr
                  && (expr->op() == sta::FuncExpr::op_zero
                      || expr->op() == sta::FuncExpr::op_one)) {
                if (expr->op() == sta::FuncExpr::op_zero)
                  const0.insert(netName);
                else
                  const1.insert(netName);

              } else {
                inputs.insert(netName);
              }
            } else {
              inputs.insert(netName);
            }
          } else if (connectedPort->getIoType() == odb::dbIoType::OUTPUT) {
            outputs.insert(netName);
          }
        }
      }
    }

    currentGate += currentConnections;

    if (cell->hasSequentials() && currentClocks.size() != 1)
      continue;
    else if (cell->hasSequentials())
      currentGate += " " + currentClock;

    subckts[instIndex++] = currentGate;
  }

  // remove drivers from input list
  std::vector<std::string> common_ports;
  set_intersection(inputs.begin(),
                   inputs.end(),
                   outputs.begin(),
                   outputs.end(),
                   std::back_inserter(common_ports));

  for (auto&& port : common_ports) {
    inputs.erase(port);
  }

  f << ".model tmp_circuit\n";
  f << ".inputs";

  for (auto& input : inputs) {
    if (const0.find(input) != const0.end()
        || const1.find(input) != const1.end())
      continue;

    f << " " << input;
  }
  f << "\n";

  f << ".outputs";

  for (auto& output : outputs) {
    f << " " << output;
  }
  f << "\n";

  if (clocks.size() > 0) {
    f << ".clock";
    for (auto& clock : clocks) {
      f << " " << clock;
    }
  }

  f << "\n\n";

  for (auto& zero : const0) {
    std::string const_subctk = ".gate _const0_ z=" + zero;
    f << const_subctk << "\n";
  }

  for (auto& one : const1) {
    std::string const_subctk = ".gate _const1_ z=" + one;
    f << const_subctk << "\n";
  }

  for (auto& subckt : subckts) {
    f << subckt << "\n";
  }

  f << ".end";

  f.close();

  logger_->info(RMP,
                2,
                "Blif writer successfully dumped file with {} instances.",
                instIndex);

  return true;
}

void preprocessString(std::string& s)
{
  int ind, old_ind = -1;

  while ((ind = s.find("\n", old_ind + 1)) != std::string::npos) {
    if (s[old_ind + 1] == '#') {
      s.erase(old_ind + 1, ind - old_ind);
    }
    old_ind = ind;
  }
}

bool Blif::inspectBlif(const char* file_name, int& numInstances)
{
  std::ifstream f(file_name);
  if (f.bad()) {
    logger_->error(RMP, 3, "cannot open file {}", file_name);
    return false;
  }

  std::string fileString((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(fileString);

  BlifParser blif;

  bool isValid = blif.parse(fileString);

  if (isValid)
    numInstances = blif.getGates().size();
  return isValid;
}

bool Blif::readBlif(const char* file_name, odb::dbBlock* block)
{
  std::ifstream f(file_name);
  if (f.bad()) {
    logger_->error(RMP, 4, "cannot open file {}", file_name);
    return false;
  }

  std::string fileString((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(fileString);

  BlifParser blif;

  bool isValid = blif.parse(fileString);
  if (!isValid) {
    logger_->error(RMP,
                   34,
                   "Blif parser failed. File doesn't follow blif spec.",
                   instances_to_optimize.size());
    return false;
  }

  // Remove and disconnect old instances
  logger_->info(RMP,
                5,
                "blif parsed successfully, destroying {} existing instances...",
                instances_to_optimize.size());
  logger_->info(RMP,
                6,
                "Found {} Inputs, {} Outputs, {} Clocks, {} Combinational "
                "gates, {} Registers after parsing the blif file.",
                blif.getInputs().size(),
                blif.getOutputs().size(),
                blif.getClocks().size(),
                blif.getCombGateCount(),
                blif.getFlopCount());

  for (auto& inst : instances_to_optimize) {
    std::set<odb::dbNet*> connectedNets;
    auto iterms = inst->getITerms();
    for (auto iterm : iterms) {
      auto net = iterm->getNet();
      odb::dbITerm::disconnect(iterm);
      if (net && net->getITerms().size() == 0 && net->getBTerms().size() == 0) {
        odb::dbNet::destroy(net);
      }
    }
    odb::dbInst::destroy(inst);
  }

  // Create and connect new instances
  auto gates = blif.getGates();
  logger_->info(RMP, 7, "inserting {} new instances...", gates.size());
  std::map<std::string, int> instIds;

  for (auto&& gate : gates) {
    GateType masterType = gate.type_;
    std::string masterName = gate.master_;
    std::vector<std::string> connections = gate.connections_;
    odb::dbMaster* master;

    for (auto&& lib : block->getDb()->getLibs()) {
      master = lib->findMaster(masterName.c_str());
      if (master != NULL)
        break;
    }

    if (master == NULL
        && (masterName == "_const0_" || masterName == "_const1_")) {
      if (connections.size() < 1) {
        logger_->info(RMP,
                      8,
                      "Const driver {} doesn't have any connected nets\n",
                      masterName.c_str());
        continue;
      }
      auto constNetName = connections[0].substr(connections[0].find("=") + 1);
      odb::dbNet* net = block->findNet(constNetName.c_str());
      if (net == NULL)
        net = odb::dbNet::create(block, constNetName.c_str());

      // Add tie cells
      std::string constMaster
          = (masterName == "_const0_") ? const0_cell_ : const1_cell_;
      std::string constPort
          = (masterName == "_const0_") ? const0_cell_port_ : const1_cell_port_;
      instIds[constMaster]
          = (instIds[constMaster]) ? instIds[constMaster] + 1 : 1;
      std::string instName
          = constMaster + "_" + std::to_string(instIds[constMaster]);
      for (auto&& lib : block->getDb()->getLibs()) {
        master = lib->findMaster(constMaster.c_str());
        if (master != NULL)
          break;
      }

      if (master != NULL) {
        auto newInst = odb::dbInst::create(block, master, instName.c_str());
        odb::dbITerm::connect(newInst->findITerm(constPort.c_str()), net);
      }

      continue;
    }

    if (master == NULL) {
      logger_->info(RMP,
                    9,
                    "Master ({}) not found while stitching back instances\n",
                    masterName.c_str());
      // return false;
      continue;
    }

    instIds[masterName] = (instIds[masterName]) ? instIds[masterName] + 1 : 1;
    std::string instName
        = masterName + "_" + std::to_string(instIds[masterName]);
    auto newInst = odb::dbInst::create(block, master, instName.c_str());

    if (newInst == NULL) {
      logger_->error(RMP,
                     76,
                     "Could not create new instance of type {} with name {}",
                     masterName,
                     instName);
      continue;
    }

    for (auto&& connection : connections) {
      auto equalSignPos = connection.find("=");

      std::string mtermName = "", netName = "";

      if (equalSignPos == std::string::npos && masterType == GateType::Mlatch) {
        // Identified clock net!
        // Find clock pin
        auto masterTerms = master->getMTerms();
        for (auto&& mTerm : masterTerms) {
          // Assuming that no more than 1 Pin can have clock type!
          auto pin_ = open_sta_->getDbNetwork()->dbToSta(mTerm);
          auto network_ = open_sta_->network();
          auto port_ = network_->libertyPort(pin_);
          if (port_->isClock()) {
            mtermName = mTerm->getName();
            netName = connection;
            break;
          }
        }

      } else if (equalSignPos == std::string::npos) {
        continue;
      } else {
        if (equalSignPos == connection.length() - 1) {
          logger_->info(RMP,
                        10,
                        "{} connection parsing failed for {} instance",
                        connection,
                        masterName);
          continue;
        }
        mtermName = connection.substr(0, equalSignPos);
        netName = connection.substr(equalSignPos + 1);
      }

      odb::dbNet* net = block->findNet(netName.c_str());
      if (net == NULL)
        net = odb::dbNet::create(block, netName.c_str());

      if (mtermName == "") {
        logger_->info(RMP,
                      146,
                      "Could not connect instance of cell type {} to {} net "
                      "due to unknown mterm in blif.",
                      masterName,
                      netName);
        continue;
      }

      odb::dbITerm::connect(newInst->findITerm(mtermName.c_str()), net);
    }
  }

  return true;
}

}  // namespace rmp
