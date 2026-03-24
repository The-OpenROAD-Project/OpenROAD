// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cut/blif.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cut/blifParser.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

using utl::CUT;

namespace cut {

Blif::Blif(utl::Logger* logger,
           sta::dbSta* sta,
           const std::string& const0_cell,
           const std::string& const0_cell_port,
           const std::string& const1_cell,
           const std::string& const1_cell_port,
           const int call_id_)
    : const0_cell_(const0_cell),
      const0_cell_port_(const0_cell_port),
      const1_cell_(const1_cell),
      const1_cell_port_(const1_cell_port),
      call_id_(call_id_)
{
  logger_ = logger;
  open_sta_ = sta;
}

void Blif::setReplaceableInstances(std::set<odb::dbInst*>& insts)
{
  instances_to_optimize_ = insts;
}

void Blif::addReplaceableInstance(odb::dbInst* inst)
{
  instances_to_optimize_.insert(inst);
}

bool Blif::writeBlif(const char* file_name, bool write_arrival_requireds)
{
  int dummy_nets = 0;

  std::ofstream f(file_name);

  // These always need to be done before writing blif
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();

  if (f.bad()) {
    logger_->error(CUT, 1, "Cannot open file {}.", file_name);
    return false;
  }

  std::set<odb::dbInst*>& insts = this->instances_to_optimize_;
  std::map<uint32_t, odb::dbInst*> instMap;
  std::vector<std::string> subckts;
  std::set<std::string> inputs, outputs, const0, const1, clocks;

  subckts.resize(insts.size());
  int instIndex = 0;

  for (auto&& inst : insts) {
    instMap.insert(std::pair<uint32_t, odb::dbInst*>(inst->getId(), inst));
  }

  for (auto&& inst : insts) {
    auto master = inst->getMaster();
    sta::LibertyCell* cell = open_sta_->getDbNetwork()->libertyCell(
        open_sta_->getDbNetwork()->dbToSta(master));
    auto masterName = master->getName();

    std::string currentGate
        = ((cell->hasSequentials()) ? ".mlatch " : ".gate ") + masterName;
    std::string currentConnections, currentClock;
    std::set<std::string> currentClocks;

    auto iterms = inst->getITerms();

    for (auto&& iterm : iterms) {
      auto mterm = iterm->getMTerm();
      auto net = iterm->getNet();

      if (iterm->getSigType() == odb::dbSigType::POWER
          || iterm->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }

      sta::Vertex *vertex, *bidirect_drvr_vertex;
      auto pin_ = open_sta_->getDbNetwork()->dbToSta(iterm);
      open_sta_->getDbNetwork()->graph()->pinVertices(
          pin_, vertex, bidirect_drvr_vertex);
      auto network_ = open_sta_->network();
      auto port_ = network_->libertyPort(pin_);
      if (port_->isClock()) {
        if (net == nullptr) {
          continue;
        }
        clocks.insert(net->getName());
        currentClocks.insert(net->getName());
        currentClock = net->getName();
        continue;
      }

      const auto& mtermName = mterm->getName();
      const auto& netName = (net == nullptr)
                                ? ("dummy_" + std::to_string(dummy_nets++))
                                : net->getName();

      currentConnections += fmt::format(" {}={}", mtermName, netName);

      if (net == nullptr) {
        continue;
      }
      // check whether connected net is input/output
      // If it's only connected to one Iterm OR
      // It's connected to another instance that's outside the bubble
      auto connectedIterms = net->getITerms();

      if (connectedIterms.size() == 1) {
        if (iterm->getIoType() == odb::dbIoType::INPUT) {
          inputs.insert(netName);
          addArrival(pin_, netName);
        } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
          outputs.insert(netName);
          addRequired(pin_, netName);
        }

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
                    && (expr->op() == sta::FuncExpr::Op::zero
                        || expr->op() == sta::FuncExpr::Op::one)) {
                  if (expr->op() == sta::FuncExpr::Op::zero) {
                    if (const0.empty()) {
                      const0_cell_ = port_->libertyCell()->name();
                      const0_cell_port_ = port_->name();
                    }
                    const0.insert(netName);
                  } else {
                    if (const1.empty()) {
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

            } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
              outputs.insert(netName);
              addRequired(pin_, netName);
            }
          }
        }
        if (addAsInput && const0.find(netName) == const0.end()
            && const1.find(netName) == const1.end()) {
          inputs.insert(netName);
          addArrival(pin_, netName);
        }
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
                  && (expr->op() == sta::FuncExpr::Op::zero
                      || expr->op() == sta::FuncExpr::Op::one)) {
                if (expr->op() == sta::FuncExpr::Op::zero) {
                  const0.insert(netName);
                } else {
                  const1.insert(netName);
                }

              } else {
                inputs.insert(netName);
                addArrival(pin_, netName);
              }
            } else {
              inputs.insert(netName);
              addArrival(pin_, netName);
            }
          } else if (connectedPort->getIoType() == odb::dbIoType::OUTPUT) {
            outputs.insert(netName);
            addRequired(pin_, netName);
          }
        }
      }
    }

    currentGate += currentConnections;

    if (cell->hasSequentials() && currentClocks.size() != 1) {
      continue;
    }
    if (cell->hasSequentials()) {
      currentGate += " " + currentClock;
    }

    subckts[instIndex++] = std::move(currentGate);
  }

  // remove drivers from input list
  std::vector<std::string> common_ports;
  std::ranges::set_intersection(
      inputs, outputs, std::back_inserter(common_ports));

  for (auto&& port : common_ports) {
    inputs.erase(port);
    arrivals_.erase(port);
  }

  f << ".model tmp_circuit\n";
  f << ".inputs";

  for (auto& input : inputs) {
    if (const0.find(input) != const0.end()
        || const1.find(input) != const1.end()) {
      continue;
    }

    f << " " << input;
  }
  f << "\n";

  f << ".outputs";

  for (auto& output : outputs) {
    f << " " << output;
  }
  f << "\n";

  if (!clocks.empty()) {
    f << ".clock";
    for (auto& clock : clocks) {
      f << " " << clock;
    }
  }

  if (write_arrival_requireds) {
    for (auto& arrival : arrivals_) {
      f << ".input_arrival " << arrival.first << " " << arrival.second.first
        << " " << arrival.second.second << '\n';
    }

    for (auto& required : requireds_) {
      f << ".output_required " << required.first << " " << required.second.first
        << " " << required.second.second << '\n';
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

  f << ".end\n";

  f.close();

  logger_->info(CUT,
                2,
                "Blif writer successfully dumped file with {} instances.",
                instIndex);

  return true;
}

void preprocessString(std::string& s)
{
  int ind, old_ind = -1;

  while ((ind = s.find('\n', old_ind + 1)) != std::string::npos) {
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
    logger_->error(CUT, 3, "Cannot open file {}.", file_name);
    return false;
  }

  std::string fileString((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(fileString);

  BlifParser blif;

  bool isValid = blif.parse(fileString);

  if (isValid) {
    numInstances = blif.getGates().size();
  }
  return isValid;
}

bool Blif::readBlif(const char* file_name, odb::dbBlock* block)
{
  std::ifstream f(file_name);
  if (f.bad()) {
    logger_->error(CUT, 4, "Cannot open file {}.", file_name);
    return false;
  }

  std::string fileString((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(fileString);

  BlifParser blif;

  bool isValid = blif.parse(fileString);
  if (!isValid) {
    logger_->error(CUT,
                   5,
                   "Blif parser failed. File doesn't follow blif spec.",
                   instances_to_optimize_.size());
    return false;
  }

  // Remove and disconnect old instances
  logger_->info(CUT,
                6,
                "Blif parsed successfully, will destroy {} existing instances.",
                instances_to_optimize_.size());
  logger_->info(CUT,
                7,
                "Found {} inputs, {} outputs, {} clocks, {} combinational "
                "gates, {} registers after parsing the blif file.",
                blif.getInputs().size(),
                blif.getOutputs().size(),
                blif.getClocks().size(),
                blif.getCombGateCount(),
                blif.getFlopCount());

  for (auto& inst : instances_to_optimize_) {
    std::set<odb::dbNet*> connectedNets;
    auto iterms = inst->getITerms();
    for (auto iterm : iterms) {
      auto net = iterm->getNet();
      iterm->disconnect();
      if (net && net->getITerms().empty() && net->getBTerms().empty()) {
        odb::dbNet::destroy(net);
      }
    }
    odb::dbInst::destroy(inst);
  }

  // Create and connect new instances
  auto gates = blif.getGates();
  logger_->info(CUT, 8, "Inserting {} new instances.", gates.size());
  std::map<std::string, int> instIds;

  for (auto&& gate : gates) {
    GateType masterType = gate.type;
    std::string masterName = gate.master;
    std::vector<std::string> connections = gate.connections;
    odb::dbMaster* master = nullptr;

    for (auto&& lib : block->getDb()->getLibs()) {
      master = lib->findMaster(masterName.c_str());
      if (master != nullptr) {
        break;
      }
    }

    if (master == nullptr
        && (masterName == "_const0_" || masterName == "_const1_")) {
      if (connections.empty()) {
        logger_->info(CUT,
                      9,
                      "Const driver {} doesn't have any connected nets.",
                      masterName.c_str());
        continue;
      }
      auto constNetName = connections[0].substr(connections[0].find('=') + 1);
      odb::dbNet* net = block->findNet(constNetName.c_str());
      if (net == nullptr) {
        std::string net_name_modified
            = std::string("or_") + std::to_string(call_id_) + constNetName;
        net = odb::dbNet::create(block, net_name_modified.c_str());
      }

      // Add tie cells
      std::string constMaster
          = (masterName == "_const0_") ? const0_cell_ : const1_cell_;
      std::string constPort
          = (masterName == "_const0_") ? const0_cell_port_ : const1_cell_port_;
      instIds[constMaster]
          = (instIds[constMaster]) ? instIds[constMaster] + 1 : 1;
      std::string instName = constMaster + "_" + std::to_string(call_id_)
                             + std::to_string(instIds[constMaster]);
      for (auto&& lib : block->getDb()->getLibs()) {
        master = lib->findMaster(constMaster.c_str());
        if (master != nullptr) {
          break;
        }
      }

      if (master != nullptr) {
        while (block->findInst(instName.c_str())) {
          instIds[constMaster]++;
          instName = constMaster + "_" + std::to_string(call_id_)
                     + std::to_string(instIds[constMaster]);
        }
        auto newInst = odb::dbInst::create(block, master, instName.c_str());
        newInst->findITerm(constPort.c_str())->connect(net);
      }

      continue;
    }

    if (master == nullptr) {
      logger_->info(CUT,
                    10,
                    "Master ({}) not found while stitching back instances.",
                    masterName.c_str());
      // return false;
      continue;
    }

    instIds[masterName] = (instIds[masterName]) ? instIds[masterName] + 1 : 1;
    std::string instName = masterName + "_" + std::to_string(call_id_) + "_"
                           + std::to_string(instIds[masterName]);
    while (block->findInst(instName.c_str())) {
      instIds[masterName]++;
      instName = masterName + "_" + std::to_string(call_id_) + "_"
                 + std::to_string(instIds[masterName]);
    }

    auto newInst = odb::dbInst::create(block, master, instName.c_str());

    if (newInst == nullptr) {
      logger_->error(CUT,
                     11,
                     "Could not create new instance of type {} with name {}.",
                     masterName,
                     instName);
      continue;
    }

    for (auto&& connection : connections) {
      auto equalSignPos = connection.find("=");

      std::string mtermName, netName;

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
            netName = std::move(connection);
            break;
          }
        }

      } else if (equalSignPos == std::string::npos) {
        continue;
      } else {
        if (equalSignPos == connection.length() - 1) {
          logger_->info(CUT,
                        12,
                        "Connection {} parsing failed for {} instance.",
                        connection,
                        masterName);
          continue;
        }
        mtermName = connection.substr(0, equalSignPos);
        netName = connection.substr(equalSignPos + 1);
      }

      odb::dbNet* net = block->findNet(netName.c_str());
      if (net == nullptr) {
        std::string net_name_modified
            = std::string("or_") + std::to_string(call_id_) + netName;
        net = block->findNet(net_name_modified.c_str());
        if (!net) {
          net = odb::dbNet::create(block, net_name_modified.c_str());
        }
      }

      if (mtermName.empty()) {
        logger_->info(CUT,
                      13,
                      "Could not connect instance of cell type {} to {} net "
                      "due to unknown mterm in blif.",
                      masterName,
                      netName);
        continue;
      }

      newInst->findITerm(mtermName.c_str())->connect(net);
    }
  }

  return true;
}

float Blif::getRequiredTime(sta::Pin* term, bool is_rise)
{
  auto vert = open_sta_->getDbNetwork()->graph()->pinLoadVertex(term);
  auto req = open_sta_->required(
      vert,
      is_rise ? sta::RiseFallBoth::rise() : sta::RiseFallBoth::fall(),
      open_sta_->scenes(),
      sta::MinMax::max());
  if (sta::delayInf(req)) {
    return 0;
  }
  return req;
}

float Blif::getArrivalTime(sta::Pin* term, bool is_rise)
{
  auto vert = open_sta_->getDbNetwork()->graph()->pinLoadVertex(term);
  auto path = open_sta_->vertexWorstArrivalPath(vert, sta::MinMax::max());
  if (path == nullptr) {
    return 0;
  }
  sta::SceneSeq scene1({path->scene(open_sta_)});
  auto arr = open_sta_->arrival(
      vert,
      is_rise ? sta::RiseFallBoth::rise() : sta::RiseFallBoth::fall(),
      scene1,
      path->minMax(open_sta_));
  if (sta::delayInf(arr)) {
    return 0;
  }
  return arr;
}

void Blif::addArrival(sta::Pin* pin, const std::string& netName)
{
  if (arrivals_.find(netName) == arrivals_.end()) {
    arrivals_[netName] = std::pair<float, float>(
        getArrivalTime(pin, true) * 1e12, getArrivalTime(pin, false) * 1e12);
  }
}

void Blif::addRequired(sta::Pin* pin, const std::string& netName)
{
  if (requireds_.find(netName) == requireds_.end()) {
    requireds_[netName] = std::pair<float, float>(
        getRequiredTime(pin, true) * 1e12, getRequiredTime(pin, false) * 1e12);
  }
}

}  // namespace cut
