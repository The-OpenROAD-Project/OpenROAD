// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cut/blif.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cut/blifParser.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/PtrSetMap.h"
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

void Blif::setReplaceableInstances(odb::PtrSet<odb::dbInst>& insts)
{
  instances_to_optimize_ = insts;
}

void Blif::addReplaceableInstance(odb::dbInst* inst)
{
  instances_to_optimize_.insert(inst);
}

namespace {

// Returns the constant value port drives, or nullopt if port is not a tie
// cell output. Tristate outputs do not force the output to be constant.
std::optional<bool> constDriverValue(const sta::LibertyPort* port)
{
  if (port == nullptr || port->tristateEnable() != nullptr) {
    return std::nullopt;
  }
  const sta::FuncExpr* expr = port->function();
  if (expr == nullptr) {
    return std::nullopt;
  }
  if (expr->op() == sta::FuncExpr::Op::zero) {
    return false;
  }
  if (expr->op() == sta::FuncExpr::Op::one) {
    return true;
  }
  return std::nullopt;
}

// Removes comment lines.
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

}  // namespace

// Nets and gate lines collected while writing a blif.
struct BlifNetlist
{
  bool isClassified(const std::string& net_name) const
  {
    return inputs.contains(net_name) || outputs.contains(net_name)
           || const0.contains(net_name) || const1.contains(net_name);
  }

  std::set<std::string> inputs;
  std::set<std::string> outputs;
  std::set<std::string> const0;
  std::set<std::string> const1;
  std::set<std::string> clocks;
  std::vector<std::string> subckts;
};

bool Blif::writeBlif(const char* file_name, const bool write_arrival_requireds)
{
  std::ofstream f(file_name);

  // These always need to be done before writing blif
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();

  if (f.bad()) {
    logger_->error(CUT, 1, "Cannot open file {}.", file_name);
    return false;
  }

  BlifNetlist netlist;
  netlist.subckts.resize(instances_to_optimize_.size());
  int inst_index = 0;
  int dummy_nets = 0;

  for (odb::dbInst* inst : instances_to_optimize_) {
    std::optional<std::string> gate = gateLine(inst, netlist, dummy_nets);
    if (gate) {
      netlist.subckts[inst_index++] = std::move(*gate);
    }
  }

  // remove drivers from input list
  std::vector<std::string> common_ports;
  std::ranges::set_intersection(
      netlist.inputs, netlist.outputs, std::back_inserter(common_ports));

  for (const std::string& port : common_ports) {
    netlist.inputs.erase(port);
    arrivals_.erase(port);
  }

  writeNetlist(f, netlist, write_arrival_requireds);
  f.close();

  logger_->info(CUT,
                2,
                "Blif writer successfully dumped file with {} instances.",
                inst_index);

  return true;
}

std::optional<std::string> Blif::gateLine(odb::dbInst* inst,
                                          BlifNetlist& netlist,
                                          int& dummy_nets)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  odb::dbMaster* master = inst->getMaster();
  const sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));

  std::string gate
      = ((cell->isSequential()) ? ".mlatch " : ".gate ") + master->getName();
  std::string clock;
  std::set<std::string> clocks;

  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::POWER
        || iterm->getSigType() == odb::dbSigType::GROUND) {
      continue;
    }

    odb::dbNet* net = iterm->getNet();
    sta::Pin* pin = network->dbToSta(iterm);
    if (network->libertyPort(pin)->isClock()) {
      if (net == nullptr) {
        continue;
      }
      netlist.clocks.insert(net->getName());
      clocks.insert(net->getName());
      clock = net->getName();
      continue;
    }

    const std::string net_name = (net == nullptr)
                                     ? ("dummy_" + std::to_string(dummy_nets++))
                                     : net->getName();

    gate += fmt::format(" {}={}", iterm->getMTerm()->getName(), net_name);

    if (net != nullptr) {
      classifyNet(iterm, pin, net, net_name, netlist);
    }
  }

  if (cell->isSequential()) {
    if (clocks.size() != 1) {
      return std::nullopt;
    }
    gate += " " + clock;
  }

  return gate;
}

void Blif::classifyNet(odb::dbITerm* iterm,
                       sta::Pin* pin,
                       odb::dbNet* net,
                       const std::string& net_name,
                       BlifNetlist& netlist)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  const odb::dbIoType io_type = iterm->getIoType();

  // The net is a primary input/output if it only connects to this iterm or
  // if it connects to an instance outside the cut.
  odb::dbSet<odb::dbITerm> connected_iterms = net->getITerms();
  if (connected_iterms.size() == 1) {
    if (io_type == odb::dbIoType::INPUT) {
      netlist.inputs.insert(net_name);
      addArrival(pin, net_name);
    } else if (io_type == odb::dbIoType::OUTPUT) {
      netlist.outputs.insert(net_name);
      addRequired(pin, net_name);
    }
  } else {
    bool add_as_input = false;
    for (odb::dbITerm* connected_iterm : connected_iterms) {
      if (instances_to_optimize_.contains(connected_iterm->getInst())) {
        continue;
      }
      if (io_type == odb::dbIoType::OUTPUT) {
        netlist.outputs.insert(net_name);
        addRequired(pin, net_name);
        continue;
      }
      if (io_type != odb::dbIoType::INPUT) {
        continue;
      }
      // A constant driver outside the cut becomes a constant gate in the
      // blif; any other driver makes the net an input.
      const sta::LibertyPort* port
          = network->libertyPort(network->dbToSta(connected_iterm));
      const std::optional<bool> const_value = constDriverValue(port);
      if (!const_value) {
        add_as_input = true;
      } else if (*const_value) {
        if (netlist.const1.empty()) {
          const1_cell_ = port->libertyCell()->name();
          const1_cell_port_ = port->name();
        }
        netlist.const1.insert(net_name);
      } else {
        if (netlist.const0.empty()) {
          const0_cell_ = port->libertyCell()->name();
          const0_cell_port_ = port->name();
        }
        netlist.const0.insert(net_name);
      }
    }
    if (add_as_input && !netlist.const0.contains(net_name)
        && !netlist.const1.contains(net_name)) {
      netlist.inputs.insert(net_name);
      addArrival(pin, net_name);
    }
  }

  // connect to original ports if not inferred already
  if (netlist.isClassified(net_name)) {
    return;
  }
  for (odb::dbBTerm* connected_port : net->getBTerms()) {
    if (connected_port->getIoType() == odb::dbIoType::INPUT) {
      sta::Pin* port_pin = network->dbToSta(connected_port);
      const std::optional<bool> const_value
          = constDriverValue(network->libertyPort(port_pin));
      if (!const_value) {
        netlist.inputs.insert(net_name);
        addArrival(port_pin, net_name);
      } else if (*const_value) {
        netlist.const1.insert(net_name);
      } else {
        netlist.const0.insert(net_name);
      }
    } else if (connected_port->getIoType() == odb::dbIoType::OUTPUT) {
      netlist.outputs.insert(net_name);
      addRequired(pin, net_name);
    }
  }
}

void Blif::writeNetlist(std::ostream& f,
                        const BlifNetlist& netlist,
                        const bool write_arrival_requireds) const
{
  f << ".model tmp_circuit\n";
  f << ".inputs";

  for (const std::string& input : netlist.inputs) {
    if (netlist.const0.contains(input) || netlist.const1.contains(input)) {
      continue;
    }

    f << " " << input;
  }
  f << "\n";

  f << ".outputs";

  for (const std::string& output : netlist.outputs) {
    f << " " << output;
  }
  f << "\n";

  if (!netlist.clocks.empty()) {
    f << ".clock";
    for (const std::string& clock : netlist.clocks) {
      f << " " << clock;
    }
  }

  if (write_arrival_requireds) {
    for (const auto& [net_name, arrival] : arrivals_) {
      f << ".input_arrival " << net_name << " " << arrival.first << " "
        << arrival.second << '\n';
    }

    for (const auto& [net_name, required] : requireds_) {
      f << ".output_required " << net_name << " " << required.first << " "
        << required.second << '\n';
    }
  }

  f << "\n\n";

  for (const std::string& zero : netlist.const0) {
    f << ".gate _const0_ z=" << zero << "\n";
  }

  for (const std::string& one : netlist.const1) {
    f << ".gate _const1_ z=" << one << "\n";
  }

  for (const std::string& subckt : netlist.subckts) {
    f << subckt << "\n";
  }

  f << ".end\n";
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
    logger_->error(
        CUT, 5, "Blif parser failed. File doesn't follow blif spec.");
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
    odb::PtrSet<odb::dbNet> connected_nets;
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
  const std::vector<Gate>& gates = blif.getGates();
  logger_->info(CUT, 8, "Inserting {} new instances.", gates.size());
  std::map<std::string, int> inst_ids;

  for (const Gate& gate : gates) {
    insertGate(gate, block, inst_ids);
  }

  return true;
}

void Blif::insertGate(const Gate& gate,
                      odb::dbBlock* block,
                      std::map<std::string, int>& inst_ids)
{
  const std::string& master_name = gate.master;
  odb::dbDatabase* db = block->getDb();
  odb::dbMaster* master = db->findMaster(master_name.c_str());

  if (master == nullptr
      && (master_name == "_const0_" || master_name == "_const1_")) {
    insertTieCell(gate, block, inst_ids);
    return;
  }

  if (master == nullptr) {
    logger_->info(CUT,
                  10,
                  "Master ({}) not found while stitching back instances.",
                  master_name);
    return;
  }

  const std::string inst_name = uniqueInstName(master_name, block, inst_ids);
  odb::dbInst* new_inst = odb::dbInst::create(block, master, inst_name.c_str());

  if (new_inst == nullptr) {
    logger_->error(CUT,
                   11,
                   "Could not create new instance of type {} with name {}.",
                   master_name,
                   inst_name);
    return;
  }

  sta::dbNetwork* network = open_sta_->getDbNetwork();
  for (const std::string& connection : gate.connections) {
    const size_t equal_sign_pos = connection.find('=');

    std::string mterm_name, net_name;

    if (equal_sign_pos == std::string::npos && gate.type == GateType::kMlatch) {
      // Identified clock net; find the clock pin, assuming that no more
      // than one pin can have clock type.
      for (odb::dbMTerm* m_term : master->getMTerms()) {
        if (network->libertyPort(network->dbToSta(m_term))->isClock()) {
          mterm_name = m_term->getName();
          net_name = connection;
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

    odb::dbNet* net = findOrCreateNet(net_name, block);

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

void Blif::insertTieCell(const Gate& gate,
                         odb::dbBlock* block,
                         std::map<std::string, int>& inst_ids)
{
  const std::string& master_name = gate.master;
  if (gate.connections.empty()) {
    logger_->info(CUT,
                  9,
                  "Const driver {} doesn't have any connected nets.",
                  master_name);
    return;
  }
  const std::string& connection = gate.connections[0];
  odb::dbNet* net
      = findOrCreateNet(connection.substr(connection.find('=') + 1), block);

  // Add tie cells
  const bool is_const0 = master_name == "_const0_";
  const std::string& const_master = is_const0 ? const0_cell_ : const1_cell_;
  const std::string& const_port
      = is_const0 ? const0_cell_port_ : const1_cell_port_;
  odb::dbMaster* master = block->getDb()->findMaster(const_master.c_str());
  if (master != nullptr) {
    const std::string inst_name = uniqueInstName(const_master, block, inst_ids);
    odb::dbInst* new_inst
        = odb::dbInst::create(block, master, inst_name.c_str());
    new_inst->findITerm(const_port.c_str())->connect(net);
  }
}

std::string Blif::uniqueInstName(const std::string& master_name,
                                 odb::dbBlock* block,
                                 std::map<std::string, int>& inst_ids) const
{
  int& id = inst_ids[master_name];
  std::string inst_name;
  do {
    ++id;
    inst_name = fmt::format("{}_{}_{}", master_name, call_id_, id);
  } while (block->findInst(inst_name.c_str()));
  return inst_name;
}

odb::dbNet* Blif::findOrCreateNet(const std::string& net_name,
                                  odb::dbBlock* block) const
{
  odb::dbNet* net = block->findNet(net_name.c_str());
  if (net != nullptr) {
    return net;
  }
  const std::string prefixed_name = fmt::format("or_{}{}", call_id_, net_name);
  net = block->findNet(prefixed_name.c_str());
  if (net == nullptr) {
    net = odb::dbNet::create(block, prefixed_name.c_str());
  }
  return net;
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
