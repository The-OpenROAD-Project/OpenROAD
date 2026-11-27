// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ret/Retimer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "Retime.h"
#include "RetimerPlacer.h"
#include "RetimingGraph.h"
#include "SequentialUnmapper.h"
#include "StaCheck.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

bool isRouted(odb::dbBlock* block)
{
  for (odb::dbNet* net : block->getNets()) {
    if (net->getWire() != nullptr || !net->getGuides().empty()) {
      return true;
    }
  }
  return false;
}

struct PlacementCount
{
  int placed = 0;
  int unplaced = 0;
};

PlacementCount countPlacement(odb::dbBlock* block)
{
  PlacementCount count;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
      ++count.placed;
    } else {
      ++count.unplaced;
    }
  }
  return count;
}

// Which of a net's class groups a chain belongs to: the one whose first
// register is the same class as this chain's.  A chain without a register
// goes with the first group.
int classBranch(const RetimingGraph& graph,
                const std::vector<odb::dbInst*>& flops,
                std::vector<odb::dbInst*>& heads)
{
  if (flops.empty()) {
    return 0;
  }
  odb::dbInst* first = flops.front();
  const int count = static_cast<int>(heads.size());
  for (int i = 0; i < count; ++i) {
    if (heads[i] == first
        || sameRegisterClass(graph.regClass(heads[i]), graph.regClass(first))) {
      return i;
    }
  }
  heads.push_back(first);
  return count;
}

void addClassedMovers(const RetimingGraph& graph,
                      const std::vector<odb::dbInst*>& registers,
                      std::unordered_set<odb::dbInst*>& movers)
{
  for (odb::dbInst* reg : registers) {
    if (reg != nullptr && keepsClass(graph.regClass(reg))) {
      movers.insert(reg);
    }
  }
}

std::vector<int> slidGates(const std::vector<int>& r)
{
  std::vector<int> slid;
  const int n = static_cast<int>(r.size());
  for (int v = 0; v < n; ++v) {
    if (r[v] != 0) {
      slid.push_back(v);
    }
  }
  return slid;
}

// The lag of a gate the unmapping added: that of the gate behind its only
// input.  r: the lags of the gates there were before.
std::optional<int> lagOfNewGate(const RetimingGraph& graph,
                                const std::vector<int>& r,
                                odb::dbInst* gate)
{
  // Unmapping makes no combinational cycle; the set only guards the walk.
  std::unordered_set<odb::dbInst*> seen;
  while (seen.insert(gate).second) {
    const int v = graph.indexOf(gate);
    if (v >= 0 && std::cmp_less(v, r.size())) {
      return r[v];
    }
    if (gate->getMaster()->isSequential()) {
      return std::nullopt;
    }
    odb::dbInst* driver = nullptr;
    int inputs = 0;
    for (odb::dbITerm* pin : gate->getITerms()) {
      if (pin->getIoType() == odb::dbIoType::INPUT
          && pin->getSigType() != odb::dbSigType::CLOCK) {
        ++inputs;
        odb::dbNet* net = pin->getNet();
        if (net != nullptr) {
          for (odb::dbITerm* t : net->getITerms()) {
            if (t->getIoType() == odb::dbIoType::OUTPUT) {
              driver = t->getInst();
            }
          }
        }
      }
    }
    if (inputs == 0) {
      return 0;  // a new tie cell
    }
    if (inputs != 1 || driver == nullptr) {
      return std::nullopt;
    }
    gate = driver;
  }
  return std::nullopt;
}

// The lags with one for each gate the unmapping added; nullopt when a new
// gate's lag cannot be traced.
std::optional<std::vector<int>> extendLag(const RetimingGraph& graph,
                                          const std::vector<int>& r)
{
  std::vector<int> extended = r;
  const size_t n = graph.nodes().size();
  for (size_t v = r.size(); v < n; ++v) {
    const std::optional<int> lag = lagOfNewGate(graph, r, graph.nodes()[v]);
    if (!lag) {
      return std::nullopt;
    }
    extended.push_back(*lag);
  }
  return extended;
}

}  // namespace

Retimer::Retimer(odb::dbDatabase* db,
                 sta::dbSta* sta,
                 est::EstimateParasitics* estimate_parasitics,
                 rsz::Resizer* resizer,
                 utl::Logger* logger)
    : db_(db),
      sta_(sta),
      estimate_parasitics_(estimate_parasitics),
      resizer_(resizer),
      logger_(logger),
      unmapper_(std::make_unique<SequentialUnmapper>(db, sta, logger, resizer)),
      placer_(
          std::make_unique<RetimerPlacer>(sta, estimate_parasitics, resizer)),
      discards_(std::make_unique<Discards>())
{
}

Retimer::~Retimer() = default;

odb::dbBlock* Retimer::getBlock() const
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip != nullptr ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::RET, 11, "No design loaded.");
  }
  return block;
}

void Retimer::retime(const RetimeOptions& options)
{
  options_ = options;
  odb::dbBlock* block = getBlock();
  if (findClockNet(block) == nullptr) {
    logger_->info(
        utl::RET, 29, "No register with a clock pin; nothing to retime.");
    return;
  }
  if (isRouted(block)) {
    logger_->warn(utl::RET, 44, "The design is routed; it is not retimed.");
    return;
  }
  if (!unmapper_->prepare()) {
    logger_->error(
        utl::RET,
        36,
        "Required library cells are missing; see the warnings above.");
  }
  checkRegisters(block);

  placer_->forgetCells();
  discards_->clear();
  report_graph_ = true;
  graph_.reset();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  const int unplaced_before = countPlacement(block).unplaced;
  odb::dbDatabase::beginEco(block);
  try {
    StaCheck check(
        sta_,
        logger_,
        placer_.get(),
        block,
        &guard,
        discards_.get(),
        [&](const int limit) { return proposeSlides(block, limit); },
        [&](const std::vector<odb::dbInst*>& slides, const int limit) {
          return applySlides(slides, limit, guard);
        },
        [&] { trial_undone_ = true; });
    check.run(options_);
    discards_->destroy(sta_);
  } catch (...) {
    odb::dbDatabase::undoEco(block);
    discards_->clear();
    throw;
  }
  odb::dbDatabase::commitEco(block);
  graph_.reset();

  const PlacementCount after = countPlacement(block);
  if (after.placed > 0 && after.unplaced > unplaced_before) {
    logger_->info(utl::RET,
                  45,
                  "{} new cells are unplaced; run global_placement "
                  "-incremental.",
                  after.unplaced - unplaced_before);
  }
}

const RetimingGraph& Retimer::graph()
{
  if (graph_ == nullptr) {
    graph_ = std::make_unique<RetimingGraph>(db_, sta_, resizer_, logger_);
    graph_->build(options_.move_named_registers);
  } else {
    const bool gates_dropped
        = trial_undone_
          && std::cmp_greater(graph_->nodes().size(), gates_before_);
    if (gates_dropped) {
      graph_->truncate(gates_before_);
      std::erase_if(trial_sources_,
                    [&](const int u) { return u >= gates_before_; });
    }
    if (gates_dropped || !trial_sources_.empty()) {
      graph_->update(trial_sources_, gates_dropped);
    }
  }
  trial_undone_ = false;
  trial_sources_.clear();
  return *graph_;
}

bool Retimer::unmap(const std::unordered_set<odb::dbInst*>& registers,
                    est::IncrementalParasiticsGuard& guard)
{
  const std::vector<int> sources = graph_->sourcesOf(registers);
  if (unmapper_->run(&registers, discards_.get()) == 0) {
    return false;
  }
  guard.update();
  const std::vector<int> read = graph_->update(sources, /*gates_changed=*/true);
  trial_sources_.insert(trial_sources_.end(), read.begin(), read.end());
  return true;
}

std::vector<odb::dbInst*> Retimer::proposeSlides(odb::dbBlock* block,
                                                 const int max_moves)
{
  const RetimingGraph& graph = this->graph();
  if (report_graph_) {
    report_graph_ = false;
    graph.report();
  }
  clock_net_ = findClockNet(block);

  const Retime search = runSearch(graph, max_moves);
  std::vector<odb::dbInst*> slides;
  for (const int v : search.getSlides()) {
    slides.push_back(graph.nodes()[v]);
  }
  return slides;
}

int Retimer::applySlides(const std::vector<odb::dbInst*>& slides,
                         const int max_moves,
                         est::IncrementalParasiticsGuard& guard)
{
  const RetimingGraph& graph = this->graph();
  gates_before_ = graph.size();
  std::vector<int> r(gates_before_, 0);
  for (odb::dbInst* gate : slides) {
    --r[graph.indexOf(gate)];
  }
  const int count = static_cast<int>(slides.size());

  const std::unordered_set<odb::dbInst*> movers = applyOrMovers(graph, r);
  if (movers.empty()) {
    return count;
  }
  return applyAfterUnmapping(r, count, movers, max_moves, guard);
}

Retime Retimer::runSearch(const RetimingGraph& graph, const int max_moves) const
{
  Retime search(
      &graph,
      sta_,
      logger_,
      unmapper_->getBaseSequentialDMTerm(),
      [this](odb::dbMaster* master) { return placer_->registerCells(master); });
  search.run(max_moves, options_.margin);
  return search;
}

int Retimer::applyAfterUnmapping(const std::vector<int>& r,
                                 const int slides,
                                 const std::unordered_set<odb::dbInst*>& movers,
                                 const int max_moves,
                                 est::IncrementalParasiticsGuard& guard)
{
  // A register the unmapper skips would be proposed again every round:
  // StaCheck undoes the trial and tries shorter ones.
  if (!unmap(movers, guard)) {
    return -slides;
  }
  const RetimingGraph& graph = *graph_;
  const std::optional<std::vector<int>> r2 = extendLag(graph, r);
  if (r2 && applyOrMovers(graph, *r2).empty()) {
    return slides;
  }

  // Ends: each pass rebuilds at least one register as the base flip-flop,
  // which never needs unmapping again, or returns.
  while (true) {
    const Retime search = runSearch(graph, max_moves);
    const int moves = search.getMoves();
    const std::unordered_set<odb::dbInst*> more
        = applyOrMovers(graph, search.getRetiming());
    if (more.empty()) {
      return moves;
    }
    if (!unmap(more, guard)) {
      return -moves;
    }
  }
}

odb::dbNet* Retimer::findClockNet(odb::dbBlock* block) const
{
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential()) {
      odb::dbNet* clock = registerClockNet(sta_, inst);
      if (clock != nullptr) {
        return clock;
      }
    }
  }
  return nullptr;
}

void Retimer::checkRegisters(odb::dbBlock* block) const
{
  int negedge = 0;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential() && isNegedgeRegister(sta_, inst)) {
      ++negedge;
    }
  }
  if (negedge > 0) {
    logger_->warn(
        utl::RET, 35, "{} negative-edge registers are left in place.", negedge);
  }
}

void Retimer::Origins::add(odb::dbInst* reg)
{
  if (seen.insert(reg).second) {
    registers.push_back(reg);
  }
}

void Retimer::originRegistersOnEdge(const RetimingGraph& graph,
                                    const std::vector<int>& r,
                                    const int u,
                                    const int k,
                                    const int j,
                                    Origins& out)
{
  const int w = graph.adj()[u][k].weight;
  if (w > 0) {
    const std::vector<odb::dbInst*>& flops = graph.phys()[u][k].flops;
    const int index = w - j;
    const int count = static_cast<int>(flops.size());
    out.add(index >= 0 && index < count ? flops[index] : nullptr);
    return;
  }
  if (j > -r[u]) {
    out.add(nullptr);
    return;
  }
  originRegisters(graph, r, u, j, out);
}

void Retimer::originRegisters(const RetimingGraph& graph,
                              const std::vector<int>& r,
                              const int v,
                              const int j,
                              Origins& out)
{
  if (!out.visited.insert(v).second) {
    return;
  }
  if (graph.fanin()[v].empty()) {
    out.add(nullptr);
    return;
  }
  for (const auto& [u, k] : graph.fanin()[v]) {
    originRegistersOnEdge(graph, r, u, k, j, out);
  }
}

Retimer::Plan Retimer::planRebuild(const RetimingGraph& graph,
                                   const std::vector<int>& r) const
{
  const std::vector<int> slid = slidGates(r);
  Plan plan{.groups = changedGroups(graph, r, slid), .crossings = {}};
  for (const int v : slid) {
    for (int j = 1; j <= -r[v]; ++j) {
      Origins origins;
      originRegisters(graph, r, v, j, origins);
      plan.crossings.push_back({
          .gate = v,
          .registers = origins.registers,
          .filled = !origins.seen.contains(nullptr),
      });
    }
  }
  return plan;
}

std::vector<Retimer::Group> Retimer::changedGroups(
    const RetimingGraph& graph,
    const std::vector<int>& r,
    const std::vector<int>& slid) const
{
  std::vector<int> sources = slid;
  for (const int v : slid) {
    for (const auto& [u, k] : graph.fanin()[v]) {
      sources.push_back(u);
    }
  }
  sortUnique(sources);

  // Ordered by net and class group: deterministic new-cell names.
  std::map<std::pair<uint32_t, int>, Group> by_net;
  std::unordered_map<odb::dbNet*, std::vector<odb::dbInst*>> heads;
  for (const int u : sources) {
    const std::vector<Edge>& edges = graph.adj()[u];
    const std::vector<EdgePhys>& handles = graph.phys()[u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; ++k) {
      odb::dbNet* driver_net = graph.chainNet(handles[k]);
      if (driver_net != nullptr) {
        const int w = edges[k].weight;
        const int wr = w + r[edges[k].target] - r[u];
        const int branch
            = classBranch(graph, handles[k].flops, heads[driver_net]);
        Group& group = by_net[{driver_net->getId(), branch}];
        group.driver_net = driver_net;
        group.sinks.push_back(
            {.pin = handles[k].sink, .registers = wr, .source = u, .edge = k});
        group.changed = group.changed || wr != w;
        if (!handles[k].flops.empty()) {
          group.chains.push_back(&handles[k].flops);
        }
        for (odb::dbInst* f : handles[k].flops) {
          group.flops.insert(f);
          group.dont_touch = group.dont_touch || graph.isDontTouch(f);
        }
      }
    }
  }
  std::vector<Group> groups;
  for (auto& [id, group] : by_net) {
    if (group.changed) {
      groups.push_back(std::move(group));
    }
  }
  return groups;
}

void Retimer::addGroupMovers(const RetimingGraph& graph,
                             const Group& group,
                             std::unordered_set<odb::dbInst*>& movers) const
{
  for (odb::dbInst* f : group.flops) {
    const RegisterClass& cls = graph.regClass(f);
    if (cls.kind == RegisterKind::kOther
        || (cls.kind == RegisterKind::kPlain
            && f->getMaster() != unmapper_->getBaseSequentialMaster())) {
      movers.insert(f);
    }
  }
  size_t longest = 0;
  for (const std::vector<odb::dbInst*>* chain : group.chains) {
    longest = std::max(longest, chain->size());
  }
  for (size_t i = 0; i < longest; ++i) {
    std::vector<odb::dbInst*> at_position;
    for (const std::vector<odb::dbInst*>* chain : group.chains) {
      if (i < chain->size()) {
        at_position.push_back((*chain)[i]);
      }
    }
    if (!graph.oneClass(at_position)) {
      addClassedMovers(graph, at_position, movers);
    }
  }
}

std::unordered_set<odb::dbInst*> Retimer::moversToUnmap(
    const RetimingGraph& graph,
    const Plan& plan) const
{
  std::unordered_set<odb::dbInst*> movers;
  for (const Group& group : plan.groups) {
    if (!group.dont_touch) {
      addGroupMovers(graph, group, movers);
    }
  }
  for (const Crossing& crossing : plan.crossings) {
    if (crossing.filled
        && !graph.moveKeepsClass(crossing.registers, crossing.gate)) {
      addClassedMovers(graph, crossing.registers, movers);
    }
  }
  return movers;
}

std::unordered_set<odb::dbInst*> Retimer::applyOrMovers(
    const RetimingGraph& graph,
    const std::vector<int>& r)
{
  const Plan plan = planRebuild(graph, r);
  std::unordered_set<odb::dbInst*> movers = moversToUnmap(graph, plan);
  if (movers.empty()) {
    applyRetiming(graph, r, plan);
  }
  return movers;
}

void Retimer::removeRegister(odb::dbInst* inst)
{
  const RegisterClass cls = registerClass(sta_, inst);
  odb::dbITerm* d = cls.data != nullptr ? inst->getITerm(cls.data) : nullptr;
  odb::dbNet* d_net = d != nullptr ? d->getNet() : nullptr;
  if (cls.kind == RegisterKind::kOther || d_net == nullptr) {
    logger_->error(utl::RET,
                   39,
                   "Cannot bypass register {}; retiming not applied.",
                   inst->getName());
  }
  // In a hierarchical netlist the loads are in the register's module, so
  // they take its D pin's module net along with the flat net.
  odb::dbModNet* d_mod_net = d->getModNet();
  odb::dbITerm* q
      = cls.output != nullptr ? inst->getITerm(cls.output) : nullptr;
  odb::dbNet* q_net = q != nullptr ? q->getNet() : nullptr;
  if (q_net != nullptr) {
    std::vector<odb::dbITerm*> loads;
    for (odb::dbITerm* iterm : q_net->getITerms()) {
      if (iterm != q) {
        loads.push_back(iterm);
      }
    }
    for (odb::dbITerm* load : loads) {
      movePin(load, d_net, d_mod_net);
    }
  }
  discardRegister(sta_, inst, *discards_);
}

Retimer::ModNets Retimer::groupModNets(
    const Group& group,
    const std::vector<const RegisterClass*>& classes,
    odb::dbNet* clock_net) const
{
  ModNets mod_nets;
  odb::dbITerm* source = group.driver_net->getFirstOutput();
  if (!sta_->getDbNetwork()->hasHierarchy() || source == nullptr) {
    return mod_nets;
  }
  odb::dbModule* module = source->getInst()->getModule();
  std::vector<odb::dbNet*> nets = {group.driver_net, clock_net};
  for (const RegisterClass* cls : classes) {
    if (cls != nullptr) {
      for (const auto& [mterm, net] : cls->enables) {
        nets.push_back(net);
      }
    }
  }
  for (odb::dbNet* net : nets) {
    if (net == nullptr || mod_nets.contains(net)) {
      continue;
    }
    odb::PtrSet<odb::dbModNet> related;
    net->findRelatedModNets(related);
    for (odb::dbModNet* mod_net : related) {
      if (mod_net->getParent() == module) {
        mod_nets[net] = mod_net;
        break;
      }
    }
  }
  return mod_nets;
}

void Retimer::connectRegister(odb::dbInst* ff,
                              const RegisterClass* cls,
                              odb::dbNet* in,
                              odb::dbNet* out,
                              odb::dbNet* clock_net,
                              const ModNets& mod_nets)
{
  odb::dbMTerm* data
      = cls != nullptr ? cls->data : unmapper_->getBaseSequentialDMTerm();
  odb::dbMTerm* output
      = cls != nullptr ? cls->output : unmapper_->getBaseSequentialQMTerm();
  odb::dbITerm* clock = registerClockPin(sta_, ff);
  for (odb::dbITerm* iterm : ff->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm == clock) {
      connectPin(iterm, clock_net, mod_nets);
    } else if (iterm->getMTerm() == data) {
      connectPin(iterm, in, mod_nets);
    } else if (iterm->getMTerm() == output) {
      iterm->connect(out);
    } else if (cls != nullptr) {
      for (const auto& [mterm, net] : cls->enables) {
        if (iterm->getMTerm() == mterm) {
          connectPin(iterm, net, mod_nets);
        }
      }
    }
  }
}

void Retimer::tieBaseControls(odb::dbInst* ff, sta::Instance* parent)
{
  for (const auto& [mterm, high] : unmapper_->getBaseSequentialTies()) {
    odb::dbInst* tie
        = makeCell(sta_, parent, "ret_tie", unmapper_->getTieMaster(high));
    odb::dbNet* net = makeNet(sta_, parent, "ret_net");
    tie->getITerm(unmapper_->getTieOutputMTerm(high))->connect(net);
    ff->getITerm(mterm)->connect(net);
  }
}

std::vector<odb::dbNet*> Retimer::buildChain(
    odb::dbNet* driver_net,
    odb::dbNet* clock_net,
    const std::vector<const RegisterClass*>& classes,
    const ModNets& mod_nets)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbITerm* source = driver_net->getFirstOutput();
  sta::Instance* parent
      = source != nullptr
            ? network->getOwningInstanceParent(network->dbToSta(source))
            : network->topInstance();

  const int depth = static_cast<int>(classes.size());
  std::vector<odb::dbNet*> nets(depth + 1);
  nets[0] = driver_net;
  for (int k = 1; k <= depth; ++k) {
    const RegisterClass* cls = classes[k - 1];
    odb::dbMaster* master
        = cls != nullptr ? cls->master : unmapper_->getBaseSequentialMaster();
    odb::dbInst* ff = makeCell(sta_, parent, "ret_ff", master);
    odb::dbNet* out = makeNet(sta_, parent, "ret_net");
    connectRegister(ff, cls, nets[k - 1], out, clock_net, mod_nets);
    if (cls == nullptr) {
      tieBaseControls(ff, parent);
    }
    nets[k] = out;
  }
  return nets;
}

std::vector<odb::dbInst*> Retimer::chainOrigins(const RetimingGraph& graph,
                                                const std::vector<int>& r,
                                                const Sink& sink,
                                                const int p)
{
  const int taken_by_sink_gate = -r[graph.adj()[sink.source][sink.edge].target];
  Origins origins;
  originRegistersOnEdge(graph,
                        r,
                        sink.source,
                        sink.edge,
                        taken_by_sink_gate + sink.registers - p + 1,
                        origins);
  return origins.registers;
}

Retimer::Chain Retimer::planChain(const RetimingGraph& graph,
                                  const std::vector<int>& r,
                                  const Group& group) const
{
  for (odb::dbInst* f : group.flops) {
    const RegisterClass& cls = graph.regClass(f);
    if (cls.kind == RegisterKind::kOther || cls.data == nullptr) {
      logger_->error(utl::RET,
                     40,
                     "Cannot rebuild register {} on net {}; retiming not "
                     "applied.",
                     f->getName(),
                     group.driver_net->getName());
    }
  }
  int max_depth = 0;
  for (const Sink& s : group.sinks) {
    max_depth = std::max(max_depth, s.registers);
  }
  Chain chain{
      .classes = std::vector<const RegisterClass*>(max_depth, nullptr),
      .clock_net = nullptr,
  };
  std::vector<bool> plain_at(max_depth, false);
  for (const Sink& s : group.sinks) {
    for (int p = 1; p <= s.registers; ++p) {
      for (odb::dbInst* reg : chainOrigins(graph, r, s, p)) {
        const RegisterClass* cls
            = reg != nullptr ? &graph.regClass(reg) : nullptr;
        bool consistent = cls != nullptr;
        if (consistent && cls->kind == RegisterKind::kPlain) {
          consistent = chain.classes[p - 1] == nullptr;
          plain_at[p - 1] = true;
        } else if (consistent && keepsClass(*cls)) {
          consistent = !plain_at[p - 1]
                       && (chain.classes[p - 1] == nullptr
                           || sameRegisterClass(*chain.classes[p - 1], *cls));
          chain.classes[p - 1] = cls;
        } else {
          consistent = false;
        }
        if (!consistent) {
          logger_->error(utl::RET,
                         38,
                         "Registers on net {} cannot form one chain; "
                         "retiming not applied.",
                         group.driver_net->getName());
        }
        if (chain.clock_net == nullptr) {
          chain.clock_net = cls->clock;
        }
      }
    }
  }
  if (chain.clock_net == nullptr) {
    chain.clock_net = clock_net_;
  }
  return chain;
}

void Retimer::checkCrossings(const RetimingGraph& graph, const Plan& plan) const
{
  for (const Crossing& crossing : plan.crossings) {
    if (!crossing.filled
        || !graph.moveKeepsClass(crossing.registers, crossing.gate)) {
      logger_->error(utl::RET,
                     41,
                     "Registers of different classes cannot cross {}; "
                     "retiming not applied.",
                     graph.nodes()[crossing.gate]->getName());
    }
  }
}

void Retimer::rebuildGroup(const Group& group,
                           const std::vector<const RegisterClass*>& classes,
                           odb::dbNet* clock_net)
{
  const ModNets mod_nets = groupModNets(group, classes, clock_net);
  // In id order, not in the order of a set of pointers: the removal order
  // decides how the nets list the rewired pins.
  for (odb::dbInst* f : inIdOrder(group.flops)) {
    removeRegister(f);
  }
  if (classes.empty()) {
    return;
  }
  const std::vector<odb::dbNet*> depth_net
      = buildChain(group.driver_net, clock_net, classes, mod_nets);
  for (const Sink& s : group.sinks) {
    if (s.registers > 0) {
      movePin(s.pin, depth_net[s.registers]);
    }
  }
}

void Retimer::applyRetiming(const RetimingGraph& graph,
                            const std::vector<int>& r,
                            const Plan& plan)
{
  if (plan.groups.empty()) {
    return;
  }
  std::vector<Chain> chains;
  for (const Group& group : plan.groups) {
    if (group.dont_touch) {
      logger_->error(utl::RET,
                     27,
                     "Net {} has a don't-touch register; retiming not "
                     "applied.",
                     group.driver_net->getName());
    }
    chains.push_back(planChain(graph, r, group));
  }
  checkCrossings(graph, plan);

  for (size_t i = 0; i < plan.groups.size(); ++i) {
    // One gate drives the net of a group that changes: the registers on a
    // net with two drivers stay.
    trial_sources_.push_back(plan.groups[i].sinks.front().source);
    rebuildGroup(plan.groups[i], chains[i].classes, chains[i].clock_net);
  }
}

}  // namespace ret
