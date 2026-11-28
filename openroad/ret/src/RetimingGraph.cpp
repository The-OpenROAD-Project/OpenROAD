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
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ret {

namespace {

// Whether the register and every cell that reads it are in the module, and no
// net it drives reaches a top-level port: its loads are rewired with it, also
// one that no edge of the graph ends on.
bool readInModule(odb::dbInst* flop, odb::dbModule* module)
{
  return flop->getModule() == module
         && std::ranges::all_of(flop->getITerms(), [module](odb::dbITerm* pin) {
              return !drivesNet(pin) || pin->getNet()->isInternalTo(module);
            });
}

// The domains (sameDomain: clock net and supply pins) of the registers that
// reach a gate through its inputs, counted up to the second.
struct ReachingDomains
{
  const RegisterClass* first = nullptr;
  int count = 0;

  // True when it grew.
  bool add(const RegisterClass* reg)
  {
    if (count > 1 || (count == 1 && sameDomain(*reg, *first))) {
      return false;
    }
    first = count == 0 ? reg : first;
    ++count;
    return true;
  }
  bool add(const ReachingDomains& other)
  {
    const bool grew = other.count > 0 && add(other.first);
    if (other.count > 1 && count < 2) {
      count = 2;
      return true;
    }
    return grew;
  }
};

// A gate passes its domains on over its edges without registers each time
// they grow, twice at most.
void passOnDomains(const std::vector<std::vector<Edge>>& adj,
                   std::vector<ReachingDomains>& domains)
{
  std::vector<int> open;
  const int n = static_cast<int>(domains.size());
  for (int u = 0; u < n; ++u) {
    if (domains[u].count > 0) {
      open.push_back(u);
    }
  }
  while (!open.empty()) {
    const int u = open.back();
    open.pop_back();
    for (const Edge& e : adj[u]) {
      if (e.weight == 0 && domains[e.target].add(domains[u])) {
        open.push_back(e.target);
      }
    }
  }
}

}  // namespace

odb::dbNet* chainNet(const EdgePhys& phys)
{
  return phys.entries.empty() ? phys.driver->getNet()
                              : phys.entries.front()->getNet();
}

RetimingGraph::RetimingGraph(odb::dbDatabase* db,
                             sta::dbSta* sta,
                             rsz::Resizer* resizer,
                             utl::Logger* logger)
    : db_(db), sta_(sta), resizer_(resizer), logger_(logger)
{
}

void RetimingGraph::build(const bool move_named_registers,
                          std::vector<odb::dbMaster*> new_cells)
{
  findClockPeriod();
  new_cells_ = std::move(new_cells);
  odb::dbBlock* block = db_->getChip()->getBlock();
  findSharedNets(block);
  findSdcCells(block, move_named_registers);
  addGates();
  const int n = size();
  for (int v = 0; v < n; ++v) {
    walkFrom(v);
  }
  pinGates();
  fence();
}

void RetimingGraph::findClockPeriod()
{
  int clocks = 0;
  for (sta::Clock* clock : sta_->cmdScene()->sdc()->clocks()) {
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

void RetimingGraph::findSharedNets(odb::dbBlock* block)
{
  for (odb::dbNet* net : block->getNets()) {
    if (hasSecondDriver(net)) {
      shared_nets_.insert(net);
    }
  }
}

void RetimingGraph::findSdcCells(odb::dbBlock* block,
                                 const bool move_named_registers)
{
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
      addExceptionCells(exception, move_named_registers);
    }
  }
}

void RetimingGraph::addExceptionCells(const sta::ExceptionPath* exception,
                                      const bool move_named_registers)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
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
    if (pin->isInputSignal(/*io=*/false)
        && (net == nullptr || net->getSigType().isSupply())) {
      return false;
    }
  }
  return true;
}

void RetimingGraph::walkFrom(const int source)
{
  clearEdges(source);
  EdgePhys chain{};
  std::unordered_set<odb::dbInst*> reached = {nodes_[source]};
  findReached(nodes_[source], reached);
  walkFanout(source, nodes_[source], chain, reached);
  linkEdges(source);
}

void RetimingGraph::clearEdges(const int source)
{
  for (odb::dbInst* flop : fenced_from_[source]) {
    fenced_.erase(flop);
  }
  unlinkEdges(source);
  adj_[source].clear();
  phys_[source].clear();
  fenced_from_[source].clear();
  walk_fence_[source] = false;
}

void RetimingGraph::findReached(odb::dbInst* source,
                                std::unordered_set<odb::dbInst*>& reached) const
{
  std::vector<odb::dbInst*> open = {source};
  while (!open.empty()) {
    odb::dbInst* current = open.back();
    open.pop_back();
    for (odb::dbITerm* out_pin : current->getITerms()) {
      if (!drivesNet(out_pin)) {
        continue;
      }
      for (odb::dbITerm* sink : out_pin->getNet()->getITerms()) {
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

void RetimingGraph::walkFanout(const int source,
                               odb::dbInst* current,
                               EdgePhys& chain,
                               const std::unordered_set<odb::dbInst*>& reached)
{
  for (odb::dbITerm* out_pin : current->getITerms()) {
    if (drivesNet(out_pin)) {
      walkNet(source, out_pin, chain, reached);
    }
  }
}

void RetimingGraph::walkNet(const int source,
                            odb::dbITerm* driver,
                            EdgePhys& chain,
                            const std::unordered_set<odb::dbInst*>& reached)
{
  odb::dbNet* net = driver->getNet();
  if (isFencingNet(net)) {
    fenceChain(source, chain);
  }
  // The walk ends at a net with two drivers: what follows would be walked
  // from each of them, twice as often at every such net.
  if (shared_nets_.contains(net)) {
    return;
  }
  // The registers entered from this net: one with two pins on it (data and
  // scan-in) is entered once.
  std::unordered_set<odb::dbInst*> entered;
  for (odb::dbITerm* sink : net->getITerms()) {
    if (sink->getIoType() != odb::dbIoType::INPUT) {
      continue;
    }
    odb::dbInst* load = sink->getInst();
    if (!load->getMaster()->isSequential()) {
      addEdge(source, driver, sink, chain);
    } else if (continuesThrough(load, sink) && entered.insert(load).second) {
      walkThrough(source, sink, chain, reached);
    }
  }
}

void RetimingGraph::walkThrough(const int source,
                                odb::dbITerm* entry,
                                EdgePhys& chain,
                                const std::unordered_set<odb::dbInst*>& reached)
{
  odb::dbInst* reg = entry->getInst();
  // An edge holds one chain of registers: where two meet, or one loops, each
  // ends.  One edge per path would also double the edges at every such stage.
  if (reconverges(reg, reached) || chain.flops.size() >= kMaxChainLength
      || std::ranges::find(chain.flops, reg) != chain.flops.end()) {
    fenceChain(source, chain);
    return;
  }
  chain.flops.push_back(reg);
  chain.entries.push_back(entry);
  walkFanout(source, reg, chain, reached);
  chain.flops.pop_back();
  chain.entries.pop_back();
}

void RetimingGraph::addEdge(const int source,
                            odb::dbITerm* driver,
                            odb::dbITerm* sink,
                            const EdgePhys& chain)
{
  const int target = indexOf(sink->getInst());
  if (target < 0) {
    return;
  }
  const int weight = static_cast<int>(chain.flops.size());
  adj_[source].push_back({.target = target, .weight = weight});
  phys_[source].push_back({
      .driver = driver,
      .sink = sink,
      .flops = chain.flops,
      .entries = chain.entries,
  });
}

void RetimingGraph::fenceChain(const int source, const EdgePhys& chain)
{
  walk_fence_[source] = true;
  fenced_.insert(chain.flops.begin(), chain.flops.end());
  fenced_from_[source].insert(
      fenced_from_[source].end(), chain.flops.begin(), chain.flops.end());
}

bool RetimingGraph::isFencingNet(odb::dbNet* net) const
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
    odb::dbITerm* driver = drivers_.driverOf(net);
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
  for (int v = 0; v < n; ++v) {
    // Only ever set: an earlier gate may have pinned this one already.
    if (sdc_cells_.contains(nodes_[v]) || isGrouped(nodes_[v])
        || !isCombinationalGate(sta_, nodes_[v])) {
      pinned_[v] = true;
    }
    pinAtPorts(v);
    pinDontTouch(v);
  }
  pinUnmodeledInputs();
  fixed_pinned_ = pinned_;
}

void RetimingGraph::pinAtPorts(const int v)
{
  for (odb::dbITerm* pin : nodes_[v]->getITerms()) {
    odb::dbNet* net = pin->getNet();
    const odb::dbIoType io = pin->getIoType();
    if (net == nullptr
        || (io != odb::dbIoType::INPUT && io != odb::dbIoType::OUTPUT)) {
      continue;
    }
    for (odb::dbBTerm* port : net->getBTerms()) {
      if (port->getIoType() == io) {
        pinned_[v] = true;
      }
    }
  }
}

void RetimingGraph::pinDontTouch(const int v)
{
  odb::dbInst* gate = nodes_[v];
  if (!gate->isDoNotTouch()
      && std::ranges::none_of(gate->getITerms(), drivesKeptNet)) {
    return;
  }
  pinned_[v] = true;
  for (odb::dbITerm* pin : gate->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (pin->getIoType() != odb::dbIoType::INPUT || net == nullptr) {
      continue;
    }
    for (odb::dbITerm* driver : net->getITerms()) {
      const int u = indexOf(driver->getInst());
      if (driver->getIoType() == odb::dbIoType::OUTPUT && u >= 0) {
        pinned_[u] = true;
      }
    }
  }
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

void RetimingGraph::fence()
{
  pinned_ = fixed_pinned_;
  const int n = size();
  for (int v = 0; v < n; ++v) {
    if (walk_fence_[v]) {
      pinned_[v] = true;
    }
  }
  fenceDontTouch();
  fenceModules();
}

void RetimingGraph::fenceDontTouch()
{
  const std::unordered_set<odb::dbNet*> fixed_nets = fixedChainNets();
  const int n = size();
  std::vector<ReachingDomains> fanin_domains(nodes_.size());
  for (int u = 0; u < n; ++u) {
    const int edge_count = static_cast<int>(adj_[u].size());
    for (int k = 0; k < edge_count; ++k) {
      const int target = adj_[u][k].target;
      if (pinned_[target]) {
        continue;
      }
      const EdgePhys& phys = phys_[u][k];
      for (odb::dbInst* flop : phys.flops) {
        fanin_domains[target].add(&regClass(flop));
      }
      if (!phys.flops.empty() && fixed_nets.contains(chainNet(phys))) {
        pinned_[target] = true;
      }
    }
  }
  passOnDomains(adj_, fanin_domains);
  for (int v = 0; v < n; ++v) {
    if (fanin_domains[v].count > 1) {
      pinned_[v] = true;
    }
  }
}

std::unordered_set<odb::dbNet*> RetimingGraph::fixedChainNets() const
{
  std::unordered_set<odb::dbNet*> fixed_nets;
  std::unordered_map<odb::dbNet*, const RegisterClass*> net_domain;
  for (const std::vector<EdgePhys>& edges : phys_) {
    for (const EdgePhys& phys : edges) {
      odb::dbNet* net = chainNet(phys);
      for (odb::dbInst* flop : phys.flops) {
        const RegisterClass* reg = &regClass(flop);
        if (neverMoves(flop)
            || !sameDomain(*net_domain.emplace(net, reg).first->second, *reg)) {
          fixed_nets.insert(net);
        }
      }
    }
  }
  return fixed_nets;
}

void RetimingGraph::fenceModules()
{
  if (!sta_->getDbNetwork()->hasHierarchy()) {
    return;
  }
  const std::unordered_set<odb::dbNet*> crossing_nets = moduleCrossingNets();
  const int n = size();
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

std::unordered_set<odb::dbNet*> RetimingGraph::moduleCrossingNets() const
{
  std::unordered_set<odb::dbNet*> crossing_nets;
  const int n = size();
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
  return crossing_nets;
}

std::vector<int> RetimingGraph::update(std::vector<int> sources,
                                       const bool gates_changed)
{
  // A new register or net can take the address of a deleted one.
  reg_class_.clear();
  drivers_.clear();
  if (gates_changed) {
    addNewGates(sources);
  }
  utl::sort_and_unique(sources);
  for (const int u : sources) {
    walkFrom(u);
  }
  if (gates_changed) {
    pinGates();
  }
  fence();
  return sources;
}

void RetimingGraph::addNewGates(std::vector<int>& sources)
{
  const int old_size = size();
  addGates();
  const int n = size();
  for (int v = old_size; v < n; ++v) {
    sources.push_back(v);
    for (odb::dbITerm* pin : nodes_[v]->getITerms()) {
      if (pin->getIoType() != odb::dbIoType::INPUT) {
        continue;
      }
      const int source = sourceOfNet(pin->getNet());
      if (source >= 0) {
        sources.push_back(source);
      }
    }
  }
}

int RetimingGraph::sourceOfNet(odb::dbNet* net) const
{
  std::unordered_set<odb::dbNet*> seen;
  while (net != nullptr && seen.insert(net).second) {
    odb::dbITerm* driver = drivers_.driverOf(net);
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

void RetimingGraph::truncate(const int gates)
{
  const int n = size();
  for (int v = gates; v < n; ++v) {
    clearEdges(v);
    index_.erase(nodes_[v]);
  }
  nodes_.resize(gates);
  adj_.resize(gates);
  phys_.resize(gates);
  fanin_.resize(gates);
  walk_fence_.resize(gates);
  fenced_from_.resize(gates);
  self_dual_.resize(gates);
}

int RetimingGraph::indexOf(odb::dbInst* gate) const
{
  const auto it = index_.find(gate);
  return it != index_.end() ? it->second : -1;
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

bool RetimingGraph::neverMoves(odb::dbInst* reg) const
{
  return fenced_.contains(reg) || sdc_cells_.contains(reg) || isHeld(reg)
         || isDontTouchRegister(sta_, reg) || readsSharedNet(reg)
         || hasConstantData(reg) || !suppliesFit(reg);
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

bool RetimingGraph::suppliesFit(odb::dbInst* reg) const
{
  odb::dbMaster* master = reg->getMaster();
  auto it = has_new_supply_pins_.find(master);
  if (it == has_new_supply_pins_.end()) {
    const bool has_all
        = std::ranges::all_of(new_cells_, [master](odb::dbMaster* cell) {
            return hasSupplyPinsOf(master, cell);
          });
    it = has_new_supply_pins_.emplace(master, has_all).first;
  }
  return it->second || hasOneSupplyNetPerKind(reg);
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
