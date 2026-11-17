// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RetimingGraph.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "RegisterUtils.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/Mode.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"

namespace ret {

RetimingGraph::RetimingGraph(odb::dbDatabase* db,
                             sta::dbSta* sta,
                             utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
}

void RetimingGraph::findClockPeriod()
{
  clock_period_ = 0.0f;
  int clocks = 0;
  for (sta::Clock* clock : sta_->cmdMode()->sdc()->clocks()) {
    if (clock->isVirtual()) {
      continue;
    }
    clocks++;
    clock_period_ = clock->period();
  }
  if (clocks == 0) {
    logger_->error(utl::RET, 30, "No clock defined.");
  }
  if (clocks > 1) {
    logger_->error(
        utl::RET,
        33,
        "{} clocks defined; only single-clock designs can be retimed.",
        clocks);
  }
}

void RetimingGraph::markBoundary(odb::dbInst* inst, int id)
{
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (net == nullptr) {
      continue;
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (pin->getIoType() == odb::dbIoType::INPUT
          && bterm->getIoType() == odb::dbIoType::INPUT) {
        pinned_[id] = true;
      }
      if (pin->getIoType() == odb::dbIoType::OUTPUT
          && bterm->getIoType() == odb::dbIoType::OUTPUT) {
        pinned_[id] = true;
      }
    }
  }
}

// An enable pin travels with its register's class; it is not a data edge.
bool RetimingGraph::continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const
{
  auto it = reg_class_.find(reg);
  if (it != reg_class_.end() && it->second.kind != RegisterKind::other) {
    return pin->getMTerm() == it->second.data;
  }
  return isRegisterDataPin(sta_, pin);
}

void RetimingGraph::walkFanout(int source,
                               odb::dbInst* current,
                               int weight,
                               std::vector<odb::dbInst*>& flops,
                               std::unordered_set<odb::dbInst*>& visited)
{
  for (odb::dbITerm* out_pin : current->getITerms()) {
    if (out_pin->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    odb::dbNet* net = out_pin->getNet();
    if (net == nullptr) {
      continue;
    }
    if (fencesChain(net)) {
      if (flops.empty()) {
        pinned_[source] = true;
      } else {
        fenced_.insert(flops.begin(), flops.end());
      }
    }
    for (odb::dbITerm* sink : net->getITerms()) {
      if (sink->getIoType() != odb::dbIoType::INPUT) {
        continue;
      }
      odb::dbInst* load = sink->getInst();
      if (load->getMaster()->isSequential()) {
        if (!continuesThrough(load, sink)) {
          continue;
        }
        if (visited.contains(load)) {
          continue;
        }
        visited.insert(load);
        flops.push_back(load);
        walkFanout(source, load, weight + 1, flops, visited);
        flops.pop_back();
        visited.erase(load);
      } else {
        auto it = index_.find(load);
        if (it == index_.end()) {
          continue;
        }
        adj_[source].push_back({.target = it->second, .weight = weight});
        phys_[source].push_back(
            {.driver = out_pin, .sink = sink, .flops = flops});
      }
    }
  }
}

bool RetimingGraph::fencesChain(odb::dbNet* net) const
{
  if (!net->getBTerms().empty()) {
    return true;
  }
  for (odb::dbITerm* sink : net->getITerms()) {
    if (sink->getIoType() == odb::dbIoType::INPUT) {
      odb::dbInst* load = sink->getInst();
      if (load->getMaster()->isSequential()) {
        if (!continuesThrough(load, sink)) {
          return true;
        }
      } else if (!index_.contains(load)) {
        return true;
      }
    }
  }
  return false;
}

void RetimingGraph::hold(const std::unordered_set<odb::dbInst*>& registers)
{
  held_ = registers;
}

bool RetimingGraph::isDontTouch(odb::dbInst* reg) const
{
  if (fenced_.contains(reg) || held_.contains(reg) || reg->isDoNotTouch()
      || isDontTouchRegister(sta_, reg)) {
    return true;
  }
  for (odb::dbITerm* pin : reg->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (net != nullptr && net->isDoNotTouch()) {
      return true;
    }
  }
  return false;
}

odb::dbITerm* RetimingGraph::dataPin(odb::dbInst* reg) const
{
  auto it = reg_class_.find(reg);
  return it != reg_class_.end() && it->second.data != nullptr
             ? reg->getITerm(it->second.data)
             : registerDataPin(sta_, reg);
}

odb::dbNet* RetimingGraph::chainNet(const EdgePhys& phys) const
{
  if (phys.flops.empty()) {
    return phys.driver->getNet();
  }
  odb::dbITerm* d = dataPin(phys.flops.front());
  return d != nullptr ? d->getNet() : nullptr;
}

void RetimingGraph::build()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  findClockPeriod();

  nodes_.clear();
  index_.clear();
  reg_class_.clear();
  fenced_.clear();
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential()) {
      reg_class_[inst] = registerClass(sta_, inst);
      continue;
    }
    index_[inst] = static_cast<int>(nodes_.size());
    nodes_.push_back(inst);
  }
  const int n = static_cast<int>(nodes_.size());
  adj_.assign(n, {});
  phys_.assign(n, {});
  pinned_.assign(n, false);
  self_dual_.assign(n, false);
  std::unordered_map<odb::dbMaster*, bool> self_dual_master;
  for (int i = 0; i < n; i++) {
    odb::dbMaster* master = nodes_[i]->getMaster();
    auto it = self_dual_master.find(master);
    if (it == self_dual_master.end()) {
      it = self_dual_master.emplace(master, isSelfDualGate(sta_, nodes_[i]))
               .first;
    }
    self_dual_[i] = it->second;
  }

  for (int i = 0; i < n; i++) {
    markBoundary(nodes_[i], i);
    markDontTouch(nodes_[i], i);
    std::vector<odb::dbInst*> flops;
    std::unordered_set<odb::dbInst*> visited;
    walkFanout(i, nodes_[i], 0, flops, visited);
  }
  fenceDontTouch();
  fenceModules();
}

// In a hierarchical netlist a register never crosses a module boundary.
// The registers that start on one net are rebuilt together with the gate
// that drives them and the gates that read them; when those are not all in
// one module, the driving gate and the gates behind the registers are never
// crossed.  A flat netlist has no boundaries to keep.
void RetimingGraph::fenceModules()
{
  if (!sta_->getDbNetwork()->hasHierarchy()) {
    return;
  }
  const int n = static_cast<int>(nodes_.size());
  std::unordered_set<odb::dbNet*> crossing_nets;
  for (int u = 0; u < n; u++) {
    odb::dbModule* module = nodes_[u]->getModule();
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; k++) {
      const EdgePhys& phys = phys_[u][k];
      bool one_module = nodes_[adj_[u][k].target]->getModule() == module;
      for (odb::dbInst* flop : phys.flops) {
        one_module = one_module && flop->getModule() == module;
      }
      odb::dbNet* net = chainNet(phys);
      if (!one_module && net != nullptr) {
        crossing_nets.insert(net);
      }
    }
  }
  for (int u = 0; u < n; u++) {
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; k++) {
      const EdgePhys& phys = phys_[u][k];
      if (crossing_nets.contains(chainNet(phys))) {
        pinned_[u] = true;
        if (!phys.flops.empty()) {
          pinned_[adj_[u][k].target] = true;
        }
      }
    }
  }
}

void RetimingGraph::markDontTouch(odb::dbInst* inst, int id)
{
  bool dont_touch = inst->isDoNotTouch();
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (pin->getIoType() == odb::dbIoType::OUTPUT && net != nullptr
        && net->isDoNotTouch()) {
      dont_touch = true;
    }
  }
  if (dont_touch) {
    pinned_[id] = true;
    for (odb::dbITerm* pin : inst->getITerms()) {
      odb::dbNet* net = pin->getNet();
      if (pin->getIoType() == odb::dbIoType::INPUT && net != nullptr) {
        for (odb::dbITerm* driver : net->getITerms()) {
          auto it = index_.find(driver->getInst());
          if (driver->getIoType() == odb::dbIoType::OUTPUT
              && it != index_.end()) {
            pinned_[it->second] = true;
          }
        }
      }
    }
  }
}

void RetimingGraph::fenceDontTouch()
{
  const int n = static_cast<int>(nodes_.size());
  // Registers that start on one net are rebuilt together, as one chain on
  // one clock net, so a don't-touch register or a second clock net keeps
  // every register on that net in place.
  std::unordered_set<odb::dbNet*> fixed_nets;
  std::unordered_map<odb::dbNet*, odb::dbNet*> net_clock;
  for (int u = 0; u < n; u++) {
    for (const EdgePhys& phys : phys_[u]) {
      for (odb::dbInst* flop : phys.flops) {
        odb::dbNet* net = chainNet(phys);
        odb::dbNet* clock = registerClockNet(flop);
        if (isDontTouch(flop)
            || net_clock.emplace(net, clock).first->second != clock) {
          fixed_nets.insert(net);
        }
      }
    }
  }

  std::vector<std::unordered_set<odb::dbNet*>> fanin_clocks(nodes_.size());
  for (int u = 0; u < n; u++) {
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; k++) {
      const int target = adj_[u][k].target;
      if (pinned_[target]) {
        continue;
      }
      const EdgePhys& phys = phys_[u][k];
      for (odb::dbInst* flop : phys.flops) {
        fanin_clocks[target].insert(registerClockNet(flop));
      }
      if (!phys.flops.empty() && fixed_nets.contains(chainNet(phys))) {
        pinned_[target] = true;
      }
    }
  }
  bool grew = true;
  while (grew) {
    grew = false;
    for (int u = 0; u < n; u++) {
      if (fanin_clocks[u].empty()) {
        continue;
      }
      for (const Edge& e : adj_[u]) {
        if (e.weight > 0) {
          continue;
        }
        for (odb::dbNet* clock : fanin_clocks[u]) {
          if (fanin_clocks[e.target].insert(clock).second) {
            grew = true;
          }
        }
      }
    }
  }
  for (int v = 0; v < n; v++) {
    if (fanin_clocks[v].size() > 1) {
      pinned_[v] = true;
    }
  }
}

void RetimingGraph::report() const
{
  int total_edges = 0;
  for (const std::vector<Edge>& edges : adj_) {
    total_edges += static_cast<int>(edges.size());
  }
  std::unordered_set<odb::dbInst*> distinct;
  for (const std::vector<EdgePhys>& handles : phys_) {
    for (const EdgePhys& p : handles) {
      distinct.insert(p.flops.begin(), p.flops.end());
    }
  }
  const int pinned = static_cast<int>(std::ranges::count(pinned_, true));
  logger_->info(utl::RET,
                14,
                "Retiming graph: {} gates, {} edges, {} registers, {} pinned "
                "gates, {} fenced registers.",
                nodes_.size(),
                total_edges,
                distinct.size(),
                pinned,
                fenced_.size());
}

}  // namespace ret
