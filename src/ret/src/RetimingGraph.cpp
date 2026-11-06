// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RetimingGraph.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include "RegisterUtils.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
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

// The period of the single non-virtual clock; more than one is an error.
void RetimingGraph::findClockPeriod()
{
  clock_period_ = 0.0f;
  int clocks = 0;
  for (sta::Clock* clock : sta_->cmdMode()->sdc()->clocks()) {
    if (clock->isVirtual()) {
      continue;
    }
    clocks++;
    clock_period_ = static_cast<float>(clock->period());
  }
  if (clocks > 1) {
    logger_->error(
        utl::RET,
        33,
        "{} clocks defined; only single-clock designs can be retimed.",
        clocks);
  }
}

// A gate is pinned when an input pin is driven by a primary input or an
// output pin drives a primary output.
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

// True when a path entering register `reg` at `pin` continues through the
// register.  For a plain or enable register only the data pin does: an enable
// pin is a class attribute that travels with the register, not a data edge,
// so the enable logic never becomes combinational in the model.  Any other
// register continues through every pin in its next_state support, as it is
// unmapped to a plain register before it moves.
bool RetimingGraph::continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const
{
  auto it = reg_class_.find(reg);
  if (it != reg_class_.end() && it->second.kind != RegisterKind::other) {
    return pin->getMTerm() == it->second.data;
  }
  return isRegisterDataPin(sta_, pin);
}

// Walk forward from `current`, through register chains while counting the
// weight, and add an edge at every combinational sink reached.  The visited
// set guards register rings.
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
    for (odb::dbITerm* sink : net->getITerms()) {
      if (sink->getIoType() != odb::dbIoType::INPUT) {
        continue;
      }
      odb::dbInst* load = sink->getInst();
      if (load->getMaster()->isSequential()) {
        // Only a data pin continues the path; a control pin (enable, async
        // set/reset, scan, clock) is not a retimeable data edge.
        if (!continuesThrough(load, sink)) {
          continue;
        }
        if (visited.count(load)) {
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
        adj_[source].push_back({it->second, weight});
        phys_[source].push_back({out_pin, sink, flops});
      }
    }
  }
}

void RetimingGraph::build()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  findClockPeriod();

  nodes_.clear();
  index_.clear();
  reg_class_.clear();
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
    std::vector<odb::dbInst*> flops;
    std::unordered_set<odb::dbInst*> visited;
    walkFanout(i, nodes_[i], 0, flops, visited);
  }
  fenceDontTouch();
}

// Pin every gate that would have to move a don't-touch register, and every
// gate whose fanin registers sit on different clock nets (a gated and an
// ungated register cannot be merged into one), so the search never proposes
// those slides.
void RetimingGraph::fenceDontTouch()
{
  std::vector<std::unordered_set<odb::dbNet*>> fanin_clocks(nodes_.size());
  for (int u = 0; u < static_cast<int>(nodes_.size()); u++) {
    for (int k = 0; k < static_cast<int>(adj_[u].size()); k++) {
      const int target = adj_[u][k].target;
      if (pinned_[target]) {
        continue;
      }
      for (odb::dbInst* flop : phys_[u][k].flops) {
        fanin_clocks[target].insert(registerClockNet(flop));
        if (isDontTouchRegister(sta_, flop)) {
          pinned_[target] = true;
          break;
        }
      }
    }
  }
  // A register pushed across a gate lands on that gate's fanout edges, so a
  // gate also sees the clock nets of every gate that feeds it through an edge
  // without registers.
  bool grew = true;
  while (grew) {
    grew = false;
    for (int u = 0; u < static_cast<int>(nodes_.size()); u++) {
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
  for (int v = 0; v < static_cast<int>(nodes_.size()); v++) {
    if (fanin_clocks[v].size() > 1) {
      pinned_[v] = true;
    }
  }
}

void RetimingGraph::report() const
{
  int total_edges = 0;
  int registered_edges = 0;
  int max_weight = 0;
  for (const std::vector<Edge>& edges : adj_) {
    for (const Edge& e : edges) {
      total_edges++;
      if (e.weight >= 1) {
        registered_edges++;
        max_weight = std::max(max_weight, e.weight);
      }
    }
  }
  int flop_slots = 0;
  std::unordered_set<odb::dbInst*> distinct;
  for (const std::vector<EdgePhys>& handles : phys_) {
    for (const EdgePhys& p : handles) {
      flop_slots += static_cast<int>(p.flops.size());
      distinct.insert(p.flops.begin(), p.flops.end());
    }
  }
  int pinned = 0;
  for (bool b : pinned_) {
    if (b) {
      pinned++;
    }
  }
  logger_->info(utl::RET,
                14,
                "Retiming graph: {} gates, {} edges ({} registered, max weight "
                "{}), clock period {:.3g}",
                nodes_.size(),
                total_edges,
                registered_edges,
                max_weight,
                clock_period_);
  logger_->info(
      utl::RET,
      15,
      "{} register slots, {} distinct registers, {} pinned gates (I/O "
      "boundary or fed through a don't-touch register)",
      flop_slots,
      distinct.size(),
      pinned);
}

}  // namespace ret
