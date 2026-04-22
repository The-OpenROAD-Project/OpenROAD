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
           const int call_id)
    : const0_cell_(const0_cell),
      const0_cell_port_(const0_cell_port),
      const1_cell_(const1_cell),
      const1_cell_port_(const1_cell_port),
      call_id_(call_id)
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
  std::map<uint32_t, odb::dbInst*> inst_map;
  std::vector<std::string> subckts;
  std::set<std::string> inputs, outputs, const0, const1, clocks;

  subckts.resize(insts.size());
  int inst_index = 0;

  for (auto&& inst : insts) {
    inst_map.insert(std::pair<uint32_t, odb::dbInst*>(inst->getId(), inst));
  }

  for (auto&& inst : insts) {
    auto master = inst->getMaster();
    sta::LibertyCell* cell = open_sta_->getDbNetwork()->libertyCell(
        open_sta_->getDbNetwork()->dbToSta(master));
    auto master_name = master->getName();

    std::string current_gate
        = ((cell->hasSequentials()) ? ".mlatch " : ".gate ") + master_name;
    std::string current_connections, current_clock;
    std::set<std::string> current_clocks;

    auto iterms = inst->getITerms();

    for (auto&& iterm : iterms) {
      auto mterm = iterm->getMTerm();
      auto net = iterm->getNet();

      if (iterm->getSigType() == odb::dbSigType::POWER
          || iterm->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }

      sta::Vertex *vertex, *bidirect_drvr_vertex;
      auto pin = open_sta_->getDbNetwork()->dbToSta(iterm);
      open_sta_->getDbNetwork()->graph()->pinVertices(
          pin, vertex, bidirect_drvr_vertex);
      auto network = open_sta_->network();
      auto port = network->libertyPort(pin);
      if (port->isClock()) {
        if (net == nullptr) {
          continue;
        }
        clocks.insert(net->getName());
        current_clocks.insert(net->getName());
        current_clock = net->getName();
        continue;
      }

      const auto& mterm_name = mterm->getName();
      const auto& net_name = (net == nullptr)
                                 ? ("dummy_" + std::to_string(dummy_nets++))
                                 : net->getName();

      current_connections += fmt::format(" {}={}", mterm_name, net_name);

      if (net == nullptr) {
        continue;
      }
      // check whether connected net is input/output
      // If it's only connected to one Iterm OR
      // It's connected to another instance that's outside the bubble
      auto connected_iterms = net->getITerms();

      if (connected_iterms.size() == 1) {
        if (iterm->getIoType() == odb::dbIoType::INPUT) {
          inputs.insert(net_name);
          addArrival(pin, net_name);
        } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
          outputs.insert(net_name);
          addRequired(pin, net_name);
        }

      } else {
        bool add_as_input = false;
        for (auto&& connected_iterm : connected_iterms) {
          auto connected_inst_id = connected_iterm->getInst()->getId();

          if (inst_map.find(connected_inst_id) == inst_map.end()) {
            // Net is connected to an instance outside the cut out region
            // Check whether it's input or output
            if (iterm->getIoType() == odb::dbIoType::INPUT) {
              // Net is connected to a pin outside the bubble and should be
              // treated as an input If the driving pin is contant then we'll
              // add a constant gate in blif otherwise just add the net as input
              auto pin = open_sta_->getDbNetwork()->dbToSta(connected_iterm);
              auto network = open_sta_->network();
              auto port = network->libertyPort(pin);

              if (port) {
                auto expr = port->function();
                if (expr
                    // Tristate outputs do not force the output to be constant.
                    && port->tristateEnable() == nullptr
                    && (expr->op() == sta::FuncExpr::Op::zero
                        || expr->op() == sta::FuncExpr::Op::one)) {
                  if (expr->op() == sta::FuncExpr::Op::zero) {
                    if (const0.empty()) {
                      const0_cell_ = port->libertyCell()->name();
                      const0_cell_port_ = port->name();
                    }
                    const0.insert(net_name);
                  } else {
                    if (const1.empty()) {
                      const1_cell_ = port->libertyCell()->name();
                      const1_cell_port_ = port->name();
                    }
                    const1.insert(net_name);
                  }

                } else {
                  add_as_input = true;
                }
              } else {
                add_as_input = true;
              }

            } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
              outputs.insert(net_name);
              addRequired(pin, net_name);
            }
          }
        }
        if (add_as_input && const0.find(net_name) == const0.end()
            && const1.find(net_name) == const1.end()) {
          inputs.insert(net_name);
          addArrival(pin, net_name);
        }
      }

      // connect to original ports if not inferred already
      if (inputs.find(net_name) == inputs.end()
          && outputs.find(net_name) == outputs.end()
          && const0.find(net_name) == const0.end()
          && const1.find(net_name) == const1.end()) {
        auto connected_ports = net->getBTerms();
        for (auto connected_port : connected_ports) {
          if (connected_port->getIoType() == odb::dbIoType::INPUT) {
            auto pin = open_sta_->getDbNetwork()->dbToSta(connected_port);
            auto network = open_sta_->network();
            auto port = network->libertyPort(pin);

            if (port) {
              auto expr = port->function();
              if (expr
                  // Tristate outputs do not force the output to be constant.
                  && port->tristateEnable() == nullptr
                  && (expr->op() == sta::FuncExpr::Op::zero
                      || expr->op() == sta::FuncExpr::Op::one)) {
                if (expr->op() == sta::FuncExpr::Op::zero) {
                  const0.insert(net_name);
                } else {
                  const1.insert(net_name);
                }

              } else {
                inputs.insert(net_name);
                addArrival(pin, net_name);
              }
            } else {
              inputs.insert(net_name);
              addArrival(pin, net_name);
            }
          } else if (connected_port->getIoType() == odb::dbIoType::OUTPUT) {
            outputs.insert(net_name);
            addRequired(pin, net_name);
          }
        }
      }
    }

    current_gate += current_connections;

    if (cell->hasSequentials() && current_clocks.size() != 1) {
      continue;
    }
    if (cell->hasSequentials()) {
      current_gate += " " + current_clock;
    }

    subckts[inst_index++] = std::move(current_gate);
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
                inst_index);

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

bool Blif::inspectBlif(const char* file_name, int& num_instances)
{
  std::ifstream f(file_name);
  if (f.bad()) {
    logger_->error(CUT, 3, "Cannot open file {}.", file_name);
    return false;
  }

  std::string file_string((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(file_string);

  BlifParser blif;

  bool is_valid = blif.parse(file_string);

  if (is_valid) {
    num_instances = blif.getGates().size();
  }
  return is_valid;
}

bool Blif::readBlif(const char* file_name, odb::dbBlock* block)
{
  std::ifstream f(file_name);
  if (f.bad()) {
    logger_->error(CUT, 4, "Cannot open file {}.", file_name);
    return false;
  }

  std::string file_string((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

  // Remove Comment Lines from Blif
  preprocessString(file_string);

  BlifParser blif;

  bool is_valid = blif.parse(file_string);
  if (!is_valid) {
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
    std::set<odb::dbNet*> connected_nets;
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
  std::map<std::string, int> inst_ids;

  for (auto&& gate : gates) {
    GateType master_type = gate.type;
    std::string master_name = gate.master;
    std::vector<std::string> connections = gate.connections;
    odb::dbMaster* master = nullptr;

    for (auto&& lib : block->getDb()->getLibs()) {
      master = lib->findMaster(master_name.c_str());
      if (master != nullptr) {
        break;
      }
    }

    if (master == nullptr
        && (master_name == "_const0_" || master_name == "_const1_")) {
      if (connections.empty()) {
        logger_->info(CUT,
                      9,
                      "Const driver {} doesn't have any connected nets.",
                      master_name.c_str());
        continue;
      }
      auto const_net_name = connections[0].substr(connections[0].find('=') + 1);
      odb::dbNet* net = block->findNet(const_net_name.c_str());
      if (net == nullptr) {
        std::string net_name_modified
            = std::string("or_") + std::to_string(call_id_) + const_net_name;
        net = odb::dbNet::create(block, net_name_modified.c_str());
      }

      // Add tie cells
      std::string const_master
          = (master_name == "_const0_") ? const0_cell_ : const1_cell_;
      std::string const_port
          = (master_name == "_const0_") ? const0_cell_port_ : const1_cell_port_;
      inst_ids[const_master]
          = (inst_ids[const_master]) ? inst_ids[const_master] + 1 : 1;
      std::string inst_name = const_master + "_" + std::to_string(call_id_)
                             + std::to_string(inst_ids[const_master]);
      for (auto&& lib : block->getDb()->getLibs()) {
        master = lib->findMaster(const_master.c_str());
        if (master != nullptr) {
          break;
        }
      }

      if (master != nullptr) {
        while (block->findInst(inst_name.c_str())) {
          inst_ids[const_master]++;
          inst_name = const_master + "_" + std::to_string(call_id_)
                     + std::to_string(inst_ids[const_master]);
        }
        auto new_inst = odb::dbInst::create(block, master, inst_name.c_str());
        new_inst->findITerm(const_port.c_str())->connect(net);
      }

      continue;
    }

    if (master == nullptr) {
      logger_->info(CUT,
                    10,
                    "Master ({}) not found while stitching back instances.",
                    master_name.c_str());
      // return false;
      continue;
    }

    inst_ids[master_name]
        = (inst_ids[master_name]) ? inst_ids[master_name] + 1 : 1;
    std::string inst_name = master_name + "_" + std::to_string(call_id_) + "_"
                           + std::to_string(inst_ids[master_name]);
    while (block->findInst(inst_name.c_str())) {
      inst_ids[master_name]++;
      inst_name = master_name + "_" + std::to_string(call_id_) + "_"
                 + std::to_string(inst_ids[master_name]);
    }

    auto new_inst = odb::dbInst::create(block, master, inst_name.c_str());

    if (new_inst == nullptr) {
      logger_->error(CUT,
                     11,
                     "Could not create new instance of type {} with name {}.",
                     master_name,
                     inst_name);
      continue;
    }

    for (auto&& connection : connections) {
      auto equal_sign_pos = connection.find("=");

      std::string mterm_name, net_name;

      if (equal_sign_pos == std::string::npos
          && master_type == GateType::kMlatch) {
        // Identified clock net!
        // Find clock pin
        auto master_terms = master->getMTerms();
        for (auto&& m_term : master_terms) {
          // Assuming that no more than 1 Pin can have clock type!
          auto pin = open_sta_->getDbNetwork()->dbToSta(m_term);
          auto network = open_sta_->network();
          auto port = network->libertyPort(pin);
          if (port->isClock()) {
            mterm_name = m_term->getName();
            net_name = std::move(connection);
            break;
          }
        }

      } else if (equal_sign_pos == std::string::npos) {
        continue;
      } else {
        if (equal_sign_pos == connection.length() - 1) {
          logger_->info(CUT,
                        12,
                        "Connection {} parsing failed for {} instance.",
                        connection,
                        master_name);
          continue;
        }
        mterm_name = connection.substr(0, equal_sign_pos);
        net_name = connection.substr(equal_sign_pos + 1);
      }

      odb::dbNet* net = block->findNet(net_name.c_str());
      if (net == nullptr) {
        std::string net_name_modified
            = std::string("or_") + std::to_string(call_id_) + net_name;
        net = block->findNet(net_name_modified.c_str());
        if (!net) {
          net = odb::dbNet::create(block, net_name_modified.c_str());
        }
      }

      if (mterm_name.empty()) {
        logger_->info(CUT,
                      13,
                      "Could not connect instance of cell type {} to {} net "
                      "due to unknown mterm in blif.",
                      master_name,
                      net_name);
        continue;
      }

      new_inst->findITerm(mterm_name.c_str())->connect(net);
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
  if (sta::delayInf(req, open_sta_)) {
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
  if (sta::delayInf(arr, open_sta_)) {
    return 0;
  }
  return arr;
}

void Blif::addArrival(sta::Pin* pin, const std::string& net_name)
{
  if (arrivals_.find(net_name) == arrivals_.end()) {
    arrivals_[net_name] = std::pair<float, float>(
        getArrivalTime(pin, true) * 1e12, getArrivalTime(pin, false) * 1e12);
  }
}

void Blif::addRequired(sta::Pin* pin, const std::string& net_name)
{
  if (requireds_.find(net_name) == requireds_.end()) {
    requireds_[net_name] = std::pair<float, float>(
        getRequiredTime(pin, true) * 1e12, getRequiredTime(pin, false) * 1e12);
  }
}

}  // namespace cut
