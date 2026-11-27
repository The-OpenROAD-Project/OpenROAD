// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RetimingGraph.h"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"

namespace ret {

RetimingGraph::RetimingGraph(odb::dbDatabase* db,
                             sta::dbSta* sta,
                             rsz::Resizer* resizer,
                             utl::Logger* logger)
    : db_(db), sta_(sta), resizer_(resizer), logger_(logger)
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
    ++clocks;
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

void RetimingGraph::markBoundary(odb::dbInst* inst, const int id)
{
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (net == nullptr) {
      continue;
    }
    const odb::dbIoType io = pin->getIoType();
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if ((io == odb::dbIoType::INPUT || io == odb::dbIoType::OUTPUT)
          && bterm->getIoType() == io) {
        pinned_[id] = true;
      }
    }
  }
}

bool RetimingGraph::continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const
{
  const RegisterClass& cls = regClass(reg);
  if (cls.negedge || cls.scan) {
    return false;
  }
  if (cls.kind != RegisterKind::kOther) {
    return pin->getMTerm() == cls.data;
  }
  return isRegisterDataPin(sta_, pin);
}

bool RetimingGraph::readsSharedNet(odb::dbInst* reg) const
{
  return std::ranges::any_of(reg->getITerms(), [&](odb::dbITerm* pin) {
    return isSignalInput(pin) && shared_nets_.contains(pin->getNet());
  });
}

bool RetimingGraph::hasConstantData(odb::dbInst* reg) const
{
  odb::dbMTerm* data = regClass(reg).data;
  if (data == nullptr) {
    return false;
  }
  const sta::Pin* pin = sta_->getDbNetwork()->dbToSta(reg->getITerm(data));
  return std::ranges::any_of(sta_->modes(), [&](const sta::Mode* mode) {
    return sta_->isConstant(pin, mode);
  });
}

void RetimingGraph::findReached(odb::dbInst* source,
                                std::unordered_set<odb::dbInst*>& reached) const
{
  std::vector<odb::dbInst*> open = {source};
  while (!open.empty()) {
    odb::dbInst* current = open.back();
    open.pop_back();
    for (odb::dbITerm* out_pin : current->getITerms()) {
      odb::dbNet* net = out_pin->getNet();
      if (out_pin->getIoType() != odb::dbIoType::OUTPUT || net == nullptr) {
        continue;
      }
      for (odb::dbITerm* sink : net->getITerms()) {
        odb::dbInst* load = sink->getInst();
        if (sink->getIoType() == odb::dbIoType::INPUT
            && load->getMaster()->isSequential() && continuesThrough(load, sink)
            && reached.insert(load).second) {
          open.push_back(load);
        }
      }
    }
  }
}

bool RetimingGraph::reconverges(
    odb::dbInst* reg,
    const std::unordered_set<odb::dbInst*>& reached) const
{
  odb::dbNet* entry = nullptr;
  for (odb::dbITerm* pin : reg->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (pin->getIoType() != odb::dbIoType::INPUT || net == nullptr
        || !continuesThrough(reg, pin)) {
      continue;
    }
    odb::dbITerm* driver = net->getFirstOutput();
    if (driver == nullptr || !reached.contains(driver->getInst())) {
      continue;
    }
    if (entry != nullptr && net != entry) {
      return true;
    }
    entry = net;
  }
  return false;
}

void RetimingGraph::walkFanout(const int source,
                               odb::dbInst* current,
                               const int weight,
                               EdgePhys& chain,
                               const std::unordered_set<odb::dbInst*>& reached)
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
      fenceChain(source, chain);
    }
    // The walk ends at a net with two drivers: what follows would be walked
    // from each of them, twice as often at every such net.
    if (shared_nets_.contains(net)) {
      continue;
    }
    // The registers entered from this net: one with two pins on it (data and
    // scan-in) is entered once.
    std::unordered_set<odb::dbInst*> entered;
    for (odb::dbITerm* sink : net->getITerms()) {
      if (sink->getIoType() != odb::dbIoType::INPUT) {
        continue;
      }
      odb::dbInst* load = sink->getInst();
      if (load->getMaster()->isSequential()) {
        if (!continuesThrough(load, sink) || !entered.insert(load).second) {
          continue;
        }
        // An edge holds one chain of registers: where two meet, or one
        // loops, each ends.  One edge per path would also double the edges
        // at every such stage.
        if (reconverges(load, reached) || chain.flops.size() >= kMaxChainLength
            || std::ranges::find(chain.flops, load) != chain.flops.end()) {
          fenceChain(source, chain);
          continue;
        }
        chain.flops.push_back(load);
        chain.entries.push_back(sink);
        walkFanout(source, load, weight + 1, chain, reached);
        chain.flops.pop_back();
        chain.entries.pop_back();
      } else {
        const auto it = index_.find(load);
        if (it == index_.end()) {
          continue;
        }
        adj_[source].push_back({.target = it->second, .weight = weight});
        phys_[source].push_back({
            .driver = out_pin,
            .sink = sink,
            .flops = chain.flops,
            .entries = chain.entries,
        });
      }
    }
  }
}

void RetimingGraph::fenceChain(const int source, const EdgePhys& chain)
{
  walk_fence_[source] = true;
  fenced_.insert(chain.flops.begin(), chain.flops.end());
  fenced_from_[source].insert(
      fenced_from_[source].end(), chain.flops.begin(), chain.flops.end());
}

bool RetimingGraph::fencesChain(odb::dbNet* net) const
{
  if (!net->getBTerms().empty() || shared_nets_.contains(net)) {
    return true;
  }
  for (odb::dbITerm* sink : net->getITerms()) {
    if (sink->getIoType() != odb::dbIoType::INPUT) {
      continue;
    }
    odb::dbInst* load = sink->getInst();
    if (load->isDoNotTouch()) {
      return true;
    }
    if (load->getMaster()->isSequential()) {
      if (!continuesThrough(load, sink)) {
        return true;
      }
    } else if (!index_.contains(load)) {
      return true;
    }
  }
  return false;
}

bool RetimingGraph::isDontTouch(odb::dbInst* reg) const
{
  return fenced_.contains(reg) || sdc_cells_.contains(reg) || isHeld(reg)
         || isDontTouchRegister(sta_, reg) || readsSharedNet(reg)
         || hasConstantData(reg);
}

const RegisterClass& RetimingGraph::regClass(odb::dbInst* reg) const
{
  auto it = reg_class_.find(reg);
  if (it == reg_class_.end()) {
    it = reg_class_.emplace(reg, registerClass(sta_, reg, resizer_)).first;
  }
  return it->second;
}

bool RetimingGraph::oneClass(const std::vector<odb::dbInst*>& registers) const
{
  for (size_t i = 1; i < registers.size(); ++i) {
    if (!sameRegisterClass(regClass(registers[0]), regClass(registers[i]))) {
      return false;
    }
  }
  return true;
}

bool RetimingGraph::moveKeepsClass(const std::vector<odb::dbInst*>& registers,
                                   const int v) const
{
  return oneClass(registers)
         && (registers.empty()
             || regClass(registers[0]).kind != RegisterKind::kInverting
             || self_dual_[v]);
}

odb::dbNet* RetimingGraph::chainNet(const EdgePhys& phys) const
{
  return phys.entries.empty() ? phys.driver->getNet()
                              : phys.entries.front()->getNet();
}

int RetimingGraph::indexOf(odb::dbInst* gate) const
{
  const auto it = index_.find(gate);
  return it != index_.end() ? it->second : -1;
}

bool RetimingGraph::isGate(odb::dbInst* inst) const
{
  if (inst->getMaster()->isSequential()) {
    return false;
  }
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (net == nullptr || pin->getSigType().isSupply()) {
      continue;
    }
    if (pin->getIoType() != odb::dbIoType::OUTPUT
        || !net->getBTerms().empty()) {
      return true;
    }
    for (odb::dbITerm* load : net->getITerms()) {
      odb::dbInst* reader = load->getInst();
      if (load != pin
          && (!reader->getMaster()->isSequential()
              || continuesThrough(reader, load))) {
        return true;
      }
    }
  }
  return false;
}

void RetimingGraph::addGates()
{
  for (odb::dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    if (!index_.contains(inst) && isGate(inst)) {
      index_[inst] = size();
      nodes_.push_back(inst);
      adj_.emplace_back();
      phys_.emplace_back();
      fanin_.emplace_back();
      walk_fence_.push_back(false);
      fenced_from_.emplace_back();
      self_dual_.push_back(selfDual(inst));
    }
  }
}

void RetimingGraph::build(const bool move_named_registers)
{
  findClockPeriod();

  nodes_.clear();
  index_.clear();
  adj_.clear();
  phys_.clear();
  fanin_.clear();
  walk_fence_.clear();
  fenced_from_.clear();
  self_dual_.clear();
  self_dual_master_.clear();
  reg_class_.clear();
  fenced_.clear();
  odb::dbBlock* block = db_->getChip()->getBlock();
  shared_nets_.clear();
  for (odb::dbNet* net : block->getNets()) {
    if (hasSecondDriver(net)) {
      shared_nets_.insert(net);
    }
  }
  findSdcCells(block, move_named_registers);
  addGates();
  const int n = size();
  for (int i = 0; i < n; ++i) {
    walkFrom(i);
  }
  pinGates();
  fence();
}

void RetimingGraph::walkFrom(const int source)
{
  for (odb::dbInst* flop : fenced_from_[source]) {
    fenced_.erase(flop);
  }
  unlinkEdges(source);
  adj_[source].clear();
  phys_[source].clear();
  fenced_from_[source].clear();
  walk_fence_[source] = false;
  EdgePhys chain{};
  std::unordered_set<odb::dbInst*> reached = {nodes_[source]};
  findReached(nodes_[source], reached);
  walkFanout(source, nodes_[source], 0, chain, reached);
  linkEdges(source);
}

void RetimingGraph::unlinkEdges(const int source)
{
  for (const Edge& e : adj_[source]) {
    // A gate an undone unmapping added is gone already.
    if (std::cmp_less(e.target, fanin_.size())) {
      std::erase_if(fanin_[e.target],
                    [source](const EdgeRef& in) { return in.gate == source; });
    }
  }
}

void RetimingGraph::linkEdges(const int source)
{
  const int edge_count = static_cast<int>(adj_[source].size());
  for (int k = 0; k < edge_count; ++k) {
    std::vector<EdgeRef>& in = fanin_[adj_[source][k].target];
    const EdgeRef edge{.gate = source, .index = k};
    in.insert(std::ranges::lower_bound(in, edge), edge);
  }
}

void RetimingGraph::pinGates()
{
  const int n = size();
  pinned_.assign(n, false);
  for (int i = 0; i < n; ++i) {
    // Only ever set: an earlier gate may have pinned this one already.
    if (sdc_cells_.contains(nodes_[i]) || isGrouped(nodes_[i])
        || !isCombinationalGate(sta_, nodes_[i])) {
      pinned_[i] = true;
    }
    markBoundary(nodes_[i], i);
    markDontTouch(nodes_[i], i);
  }
  pinUnmodeledInputs();
  fixed_pinned_ = pinned_;
}

void RetimingGraph::fence()
{
  pinned_ = fixed_pinned_;
  const int n = size();
  for (int i = 0; i < n; ++i) {
    if (walk_fence_[i]) {
      pinned_[i] = true;
    }
  }
  fenceDontTouch();
  fenceModules();
}

int RetimingGraph::sourceOfNet(odb::dbNet* net) const
{
  std::unordered_set<odb::dbNet*> seen;
  while (net != nullptr && seen.insert(net).second) {
    odb::dbITerm* driver = net->getFirstOutput();
    if (driver == nullptr) {
      return -1;
    }
    odb::dbInst* inst = driver->getInst();
    if (!inst->getMaster()->isSequential()) {
      return indexOf(inst);
    }
    odb::dbITerm* data = registerDataPin(sta_, inst);
    net = data != nullptr ? data->getNet() : nullptr;
  }
  return -1;
}

std::vector<int> RetimingGraph::sourcesOf(
    const std::unordered_set<odb::dbInst*>& registers) const
{
  std::vector<int> sources;
  const int n = size();
  for (int u = 0; u < n; ++u) {
    const bool holds = std::ranges::any_of(phys_[u], [&](const EdgePhys& phys) {
      return std::ranges::any_of(phys.flops, [&](odb::dbInst* flop) {
        return registers.contains(flop);
      });
    });
    if (holds) {
      sources.push_back(u);
    }
  }
  return sources;
}

std::vector<int> RetimingGraph::update(std::vector<int> sources,
                                       const bool gates_changed)
{
  // A new register can take the address of a deleted one.
  reg_class_.clear();
  if (gates_changed) {
    const int old_size = size();
    addGates();
    const int n = size();
    for (int v = old_size; v < n; ++v) {
      sources.push_back(v);
      for (odb::dbITerm* pin : nodes_[v]->getITerms()) {
        if (pin->getIoType() == odb::dbIoType::INPUT) {
          const int source = sourceOfNet(pin->getNet());
          if (source >= 0) {
            sources.push_back(source);
          }
        }
      }
    }
  }
  sortUnique(sources);
  for (const int u : sources) {
    walkFrom(u);
  }
  if (gates_changed) {
    pinGates();
  }
  fence();
  return sources;
}

void RetimingGraph::truncate(const int gates)
{
  for (int v = gates; v < size(); ++v) {
    unlinkEdges(v);
    index_.erase(nodes_[v]);
    for (odb::dbInst* flop : fenced_from_[v]) {
      fenced_.erase(flop);
    }
  }
  nodes_.resize(gates);
  adj_.resize(gates);
  phys_.resize(gates);
  fanin_.resize(gates);
  walk_fence_.resize(gates);
  fenced_from_.resize(gates);
  self_dual_.resize(gates);
}

bool RetimingGraph::selfDual(odb::dbInst* gate)
{
  odb::dbMaster* master = gate->getMaster();
  auto it = self_dual_master_.find(master);
  if (it == self_dual_master_.end()) {
    it = self_dual_master_.emplace(master, isSelfDualGate(sta_, gate)).first;
  }
  if (!it->second) {
    return false;
  }
  for (odb::dbITerm* pin : gate->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (pin->getIoType() == odb::dbIoType::INPUT
        && !pin->getSigType().isSupply()
        && (net == nullptr || net->getSigType().isSupply())) {
      return false;
    }
  }
  return true;
}

void RetimingGraph::findSdcCells(odb::dbBlock* block,
                                 const bool move_named_registers)
{
  sdc_cells_.clear();
  sta::dbNetwork* network = sta_->getDbNetwork();
  for (odb::dbInst* inst : block->getInsts()) {
    // A gate keeps what names it as a cell; a register would lose it, which
    // the caller can allow.
    const bool named_register = inst->getMaster()->isSequential()
                                && !move_named_registers
                                && hasCellConstraint(sta_, inst);
    if (named_register || hasPinConstraint(sta_, inst)
        || hasDisabledEdge(sta_, inst)) {
      sdc_cells_.insert(inst);
    }
  }
  for (odb::dbInst* inst : portDelayReferenceCells(sta_)) {
    sdc_cells_.insert(inst);
  }
  for (const sta::Mode* mode : sta_->modes()) {
    for (const sta::ExceptionPath* exception : mode->sdc()->exceptions()) {
      // A move across a cell that an exception names would change which
      // paths it covers.  The caller can release the registers that a path
      // delay or a path group lists as cells in its -from or -to.
      const bool releases
          = move_named_registers
            && (exception->isPathDelay() || exception->isGroupPath());
      sta::ExceptionPtIterator pt_iter(exception);
      while (pt_iter.hasNext()) {
        sta::ExceptionPt* point = pt_iter.next();
        const sta::PinSet* pins = point->pins();
        for (const sta::Pin* pin : point->allPins(network)) {
          odb::dbInst* inst = network->staToDb(network->instance(pin));
          if (inst == nullptr) {
            continue;
          }
          // Listed as a cell: a -from or -to that does not name this pin.
          const bool listed
              = !point->isThru() && (pins == nullptr || !pins->contains(pin));
          if (!(releases && listed && inst->getMaster()->isSequential())) {
            sdc_cells_.insert(inst);
          }
        }
      }
    }
  }
}

std::vector<odb::dbITerm*> RetimingGraph::unmodeledInputs(const int v) const
{
  std::vector<odb::dbITerm*> pins;
  for (odb::dbITerm* pin : nodes_[v]->getITerms()) {
    if (isSignalInput(pin)
        && std::ranges::none_of(fanin_[v], [&](const EdgeRef& in) {
             return phys_[in.gate][in.index].sink == pin;
           })) {
      pins.push_back(pin);
    }
  }
  return pins;
}

void RetimingGraph::pinUnmodeledInputs()
{
  const int n = size();
  for (int v = 0; v < n; ++v) {
    if (!unmodeledInputs(v).empty()) {
      pinned_[v] = true;
    }
  }
}

namespace {

// Whether the register and every cell that reads it are in the module: its
// loads are rewired with it, also one that no edge of the graph ends on.
bool readInModule(odb::dbInst* flop, odb::dbModule* module)
{
  for (odb::dbITerm* pin : flop->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (pin->getIoType() != odb::dbIoType::OUTPUT || net == nullptr) {
      continue;
    }
    for (odb::dbITerm* load : net->getITerms()) {
      if (load->getInst()->getModule() != module) {
        return false;
      }
    }
  }
  return flop->getModule() == module;
}

}  // namespace

void RetimingGraph::fenceModules()
{
  if (!sta_->getDbNetwork()->hasHierarchy()) {
    return;
  }
  const int n = size();
  std::unordered_set<odb::dbNet*> crossing_nets;
  for (int u = 0; u < n; ++u) {
    odb::dbModule* module = nodes_[u]->getModule();
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; ++k) {
      const EdgePhys& phys = phys_[u][k];
      const bool one_module
          = nodes_[adj_[u][k].target]->getModule() == module
            && std::ranges::all_of(phys.flops, [module](odb::dbInst* flop) {
                 return readInModule(flop, module);
               });
      odb::dbNet* net = chainNet(phys);
      if (!one_module && net != nullptr) {
        crossing_nets.insert(net);
      }
    }
  }
  for (int u = 0; u < n; ++u) {
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; ++k) {
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

void RetimingGraph::markDontTouch(odb::dbInst* inst, const int id)
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
          const auto it = index_.find(driver->getInst());
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
  const int n = size();
  // A don't-touch register or a second clock net keeps every register that
  // starts on the same net in place.
  std::unordered_set<odb::dbNet*> fixed_nets;
  std::unordered_map<odb::dbNet*, odb::dbNet*> net_clock;
  for (int u = 0; u < n; ++u) {
    for (const EdgePhys& phys : phys_[u]) {
      odb::dbNet* net = chainNet(phys);
      for (odb::dbInst* flop : phys.flops) {
        odb::dbNet* clock = regClass(flop).clock;
        if (isDontTouch(flop)
            || net_clock.emplace(net, clock).first->second != clock) {
          fixed_nets.insert(net);
        }
      }
    }
  }

  std::vector<std::unordered_set<odb::dbNet*>> fanin_clocks(nodes_.size());
  for (int u = 0; u < n; ++u) {
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; ++k) {
      const int target = adj_[u][k].target;
      if (pinned_[target]) {
        continue;
      }
      const EdgePhys& phys = phys_[u][k];
      for (odb::dbInst* flop : phys.flops) {
        fanin_clocks[target].insert(regClass(flop).clock);
      }
      if (!phys.flops.empty() && fixed_nets.contains(chainNet(phys))) {
        pinned_[target] = true;
      }
    }
  }
  bool grew = true;
  while (grew) {
    grew = false;
    for (int u = 0; u < n; ++u) {
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
  for (int v = 0; v < n; ++v) {
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
  for (const std::vector<EdgePhys>& edges : phys_) {
    for (const EdgePhys& phys : edges) {
      distinct.insert(phys.flops.begin(), phys.flops.end());
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
