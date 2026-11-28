// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ret/Retimer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
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
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ret {

namespace {

// A wire or a guide on any net, or special wires on a signal or clock net: a
// power grid has them before routing.
bool isRouted(odb::dbBlock* block)
{
  for (odb::dbNet* net : block->getNets()) {
    if (net->getWire() != nullptr || !net->getGuides().empty()
        || (!net->getSigType().isSupply() && !net->getSWires().empty())) {
      return true;
    }
  }
  return false;
}

// A delay annotated on a timing arc (read_sdf, set_assigned_delay).  STA
// drops it with the arc, and an undone trial brings the arc back without it.
bool hasAnnotatedDelays(sta::dbSta* sta)
{
  sta::Graph* graph = sta->ensureGraph();
  const sta::DcalcAPIndex ap_count = sta->dcalcAnalysisPtCount();
  sta::VertexIterator vertex_iter(graph);
  while (vertex_iter.hasNext()) {
    sta::VertexOutEdgeIterator edge_iter(vertex_iter.next(), graph);
    while (edge_iter.hasNext()) {
      const sta::Edge* edge = edge_iter.next();
      for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
        for (sta::DcalcAPIndex ap = 0; ap < ap_count; ++ap) {
          if (graph->arcDelayAnnotated(edge, arc, ap)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

int unplacedCount(odb::dbBlock* block)
{
  return static_cast<int>(std::ranges::count_if(
      block->getInsts(), [](odb::dbInst* inst) { return !inst->isPlaced(); }));
}

// The gates with an edge whose register count can change, in index order:
// the slid gates and the gates that drive them.
std::vector<int> sourcesOfChangedEdges(const RetimingGraph& graph,
                                       const std::vector<int>& slid)
{
  std::vector<int> sources = slid;
  for (const int v : slid) {
    for (const auto& [u, k] : graph.fanin()[v]) {
      sources.push_back(u);
    }
  }
  utl::sort_and_unique(sources);
  return sources;
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

// A gate's inputs other than clock pins.  driver: the cell behind the last
// one that is driven; nullptr when none is.
struct GateInputs
{
  int count = 0;
  odb::dbInst* driver = nullptr;
};

GateInputs gateInputs(odb::dbInst* gate)
{
  GateInputs inputs;
  for (odb::dbITerm* pin : gate->getITerms()) {
    if (pin->getIoType() != odb::dbIoType::INPUT
        || pin->getSigType() == odb::dbSigType::CLOCK) {
      continue;
    }
    ++inputs.count;
    odb::dbNet* net = pin->getNet();
    if (net == nullptr) {
      continue;
    }
    odb::dbITerm* source = net->getFirstOutput();
    if (source != nullptr) {
      inputs.driver = source->getInst();
    }
  }
  return inputs;
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
    const GateInputs inputs = gateInputs(gate);
    if (inputs.count == 0) {
      return 0;  // a new tie cell
    }
    if (inputs.count != 1 || inputs.driver == nullptr) {
      return std::nullopt;
    }
    gate = inputs.driver;
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

// The j-th register upstream of the edge's sink came through its source:
// the edge held none and the source was crossed at least j times.
bool cameThroughSource(const RetimingGraph& graph,
                       const std::vector<int>& r,
                       const int u,
                       const int k,
                       const int j)
{
  return graph.adj()[u][k].weight == 0 && j <= -r[u];
}

// nullptr: the net has no module net in the module.
odb::dbModNet* modNetIn(odb::dbNet* net, odb::dbModule* module)
{
  odb::PtrSet<odb::dbModNet> related;
  net->findRelatedModNets(related);
  for (odb::dbModNet* mod_net : related) {
    if (mod_net->getParent() == module) {
      return mod_net;
    }
  }
  return nullptr;
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
  if (!canRetime(block)) {
    return;
  }
  if (!unmapper_->prepare()) {
    logger_->error(
        utl::RET,
        36,
        "Required library cells are missing; see the warnings above.");
  }
  warnNegedgeRegisters(block);

  discards_->clear();
  report_graph_ = true;
  graph_.reset();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  const int unplaced_before = unplacedCount(block);
  runRounds(block, guard);
  reportUnplaced(block, unplaced_before);
}

bool Retimer::canRetime(odb::dbBlock* block) const
{
  if (findClockNet(block) == nullptr) {
    logger_->info(
        utl::RET, 29, "No register with a clock pin; nothing to retime.");
    return false;
  }
  if (isRouted(block)) {
    logger_->warn(utl::RET, 44, "The design is routed; it is not retimed.");
    return false;
  }
  // The estimator cannot rebuild the wires of a net a trial changed.
  if (std::string_view(estimate_parasitics_->parasiticsSource()) == "spef") {
    logger_->warn(utl::RET,
                  48,
                  "The parasitics were read from SPEF; the design is not "
                  "retimed.");
    return false;
  }
  if (hasAnnotatedDelays(sta_)) {
    logger_->warn(
        utl::RET, 49, "The design has annotated delays; it is not retimed.");
    return false;
  }
  return true;
}

void Retimer::runRounds(odb::dbBlock* block,
                        est::IncrementalParasiticsGuard& guard)
{
  odb::dbDatabase::beginEco(block);
  try {
    StaCheck check(
        sta_,
        logger_,
        placer_.get(),
        block,
        &guard,
        discards_.get(),
        [&](const int max_moves) { return proposeSlides(block, max_moves); },
        [&](const std::vector<odb::dbInst*>& slides, const int max_moves) {
          return applySlides(slides, max_moves, guard);
        },
        [&] { trial_undone_ = true; });
    check.run(options_.max_moves);
    discards_->destroy(sta_);
  } catch (...) {
    odb::dbDatabase::undoEco(block);
    discards_->clear();
    throw;
  }
  odb::dbDatabase::commitEco(block);
  graph_.reset();
}

void Retimer::reportUnplaced(odb::dbBlock* block,
                             const int unplaced_before) const
{
  const int unplaced = unplacedCount(block);
  const int placed = static_cast<int>(block->getInsts().size()) - unplaced;
  if (placed > 0 && unplaced > unplaced_before) {
    logger_->info(utl::RET,
                  45,
                  "{} new cells are unplaced; run global_placement "
                  "-incremental.",
                  unplaced - unplaced_before);
  }
}

const RetimingGraph& Retimer::syncGraph()
{
  if (graph_ == nullptr) {
    graph_ = std::make_unique<RetimingGraph>(db_, sta_, resizer_, logger_);
    graph_->build(options_.move_sdc_registers, unmapper_->newCellMasters());
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
  const RetimingGraph& graph = syncGraph();
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
  const RetimingGraph& graph = syncGraph();
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
  Retime search(&graph,
                sta_,
                logger_,
                unmapper_->getBaseSequentialDMTerm(),
                [this](odb::dbMaster* master) {
                  return placer_->masterAndEquivalents(master);
                });
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
  const std::optional<std::vector<int>> extended = extendLag(graph, r);
  if (extended && applyOrMovers(graph, *extended).empty()) {
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

void Retimer::warnNegedgeRegisters(odb::dbBlock* block) const
{
  const int negedge = static_cast<int>(
      std::ranges::count_if(block->getInsts(), [this](odb::dbInst* inst) {
        return inst->getMaster()->isSequential()
               && isNegedgeRegister(sta_, inst);
      }));
  if (negedge > 0) {
    logger_->warn(
        utl::RET, 35, "{} negative-edge registers are left in place.", negedge);
  }
}

std::vector<odb::dbInst*> Retimer::originRegistersOnEdge(
    const RetimingGraph& graph,
    const std::vector<int>& r,
    const int u,
    const int k,
    const int j,
    Origins& known)
{
  if (cameThroughSource(graph, r, u, k, j)) {
    return originRegisters(graph, r, u, j, known);
  }
  const std::vector<odb::dbInst*>& flops = graph.phys()[u][k].flops;
  const int index = graph.adj()[u][k].weight - j;
  const int count = static_cast<int>(flops.size());
  return {index >= 0 && index < count ? flops[index] : nullptr};
}

const std::vector<odb::dbInst*>& Retimer::originRegisters(
    const RetimingGraph& graph,
    const std::vector<int>& r,
    const int v,
    const int j,
    Origins& known)
{
  // An entry stays where it is when others are added.
  const auto found = known.find({v, j});
  if (found != known.end()) {
    return found->second;
  }
  // On a list, not on the call stack: one register can cross thousands of
  // gates in a round.  A gate is traced once the gates its registers came
  // through are.
  std::vector<int> open = {v};
  while (!open.empty()) {
    const int gate = open.back();
    const std::vector<EdgeRef>& fanin = graph.fanin()[gate];
    if (known.try_emplace({gate, j}).second) {
      for (const auto& [u, k] : fanin) {
        if (cameThroughSource(graph, r, u, k, j) && !known.contains({u, j})) {
          open.push_back(u);
        }
      }
      if (open.back() != gate) {
        continue;
      }
    }
    known[{gate, j}] = faninOrigins(graph, r, gate, j, known);
    open.pop_back();
  }
  return known.at({v, j});
}

std::vector<odb::dbInst*> Retimer::faninOrigins(const RetimingGraph& graph,
                                                const std::vector<int>& r,
                                                const int gate,
                                                const int j,
                                                Origins& known)
{
  const std::vector<EdgeRef>& fanin = graph.fanin()[gate];
  std::vector<odb::dbInst*> registers;
  if (fanin.empty()) {
    registers.push_back(nullptr);
  }
  for (const auto& [u, k] : fanin) {
    for (odb::dbInst* reg : originRegistersOnEdge(graph, r, u, k, j, known)) {
      if (std::ranges::find(registers, reg) == registers.end()) {
        registers.push_back(reg);
      }
    }
  }
  return registers;
}

Retimer::Plan Retimer::planRebuild(const RetimingGraph& graph,
                                   const std::vector<int>& r) const
{
  const std::vector<int> slid = slidGates(r);
  Plan plan{.groups = changedGroups(graph, r, slid), .crossings = {}};
  Origins known;
  for (const int v : slid) {
    for (int j = 1; j <= -r[v]; ++j) {
      const std::vector<odb::dbInst*>& registers
          = originRegisters(graph, r, v, j, known);
      plan.crossings.push_back({
          .gate = v,
          .registers = registers,
          .filled = std::ranges::find(registers, nullptr) == registers.end(),
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
  // Ordered by net and class group: deterministic new-cell names.
  std::map<std::pair<uint32_t, int>, Group> by_net;
  std::unordered_map<odb::dbNet*, std::vector<odb::dbInst*>> heads;
  for (const int u : sourcesOfChangedEdges(graph, slid)) {
    const std::vector<Edge>& edges = graph.adj()[u];
    const std::vector<EdgePhys>& phys = graph.phys()[u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; ++k) {
      odb::dbNet* driver_net = chainNet(phys[k]);
      if (driver_net == nullptr) {
        continue;
      }
      const int branch = classBranch(graph, phys[k].flops, heads[driver_net]);
      Group& group = by_net[{driver_net->getId(), branch}];
      group.driver_net = driver_net;
      const Sink sink{
          .pin = phys[k].sink,
          .registers = registersOn(r, u, edges[k]),
          .source = u,
          .edge = k,
      };
      addSink(graph, sink, group);
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

void Retimer::addSink(const RetimingGraph& graph,
                      const Sink& sink,
                      Group& group)
{
  group.sinks.push_back(sink);
  if (sink.registers != graph.adj()[sink.source][sink.edge].weight) {
    group.changed = true;
  }
  const std::vector<odb::dbInst*>& flops
      = graph.phys()[sink.source][sink.edge].flops;
  if (!flops.empty()) {
    group.chains.push_back(&flops);
  }
  for (odb::dbInst* flop : flops) {
    if (group.flops.insert(flop).second) {
      group.dont_touch = group.dont_touch || graph.neverMoves(flop);
    }
  }
}

void Retimer::addGroupMovers(const RetimingGraph& graph,
                             const Group& group,
                             std::unordered_set<odb::dbInst*>& movers) const
{
  for (odb::dbInst* flop : group.flops) {
    const RegisterClass& cls = graph.regClass(flop);
    if (cls.kind == RegisterKind::kOther
        || (cls.kind == RegisterKind::kPlain
            && flop->getMaster() != unmapper_->getBaseSequentialMaster())) {
      movers.insert(flop);
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

Retimer::ModNets Retimer::groupModNets(const Group& group,
                                       const Chain& chain) const
{
  ModNets mod_nets;
  odb::dbITerm* source = group.driver_net->getFirstOutput();
  if (!sta_->getDbNetwork()->hasHierarchy() || source == nullptr) {
    return mod_nets;
  }
  odb::dbModule* module = source->getInst()->getModule();
  std::vector<odb::dbNet*> nets = {group.driver_net, chain.clock_net};
  for (const RegisterClass* cls : chain.classes) {
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
    odb::dbModNet* mod_net = modNetIn(net, module);
    if (mod_net != nullptr) {
      mod_nets[net] = mod_net;
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
    ff->getITerm(mterm)->connect(
        unmapper_->makeTieNet(parent, "ret_tie", "ret_net", high, ff));
  }
}

std::vector<odb::dbNet*> Retimer::buildChain(odb::dbNet* driver_net,
                                             const Chain& chain,
                                             const ModNets& mod_nets)
{
  odb::dbITerm* source = driver_net->getFirstOutput();
  sta::Instance* parent = source != nullptr
                              ? parentInstance(sta_, source->getInst())
                              : sta_->getDbNetwork()->topInstance();

  const int depth = static_cast<int>(chain.classes.size());
  std::vector<odb::dbNet*> nets(depth + 1);
  nets[0] = driver_net;
  for (int k = 1; k <= depth; ++k) {
    const RegisterClass* cls = chain.classes[k - 1];
    odb::dbMaster* master
        = cls != nullptr ? cls->master : unmapper_->getBaseSequentialMaster();
    odb::dbInst* ff = makeCell(sta_, parent, "ret_ff", master, chain.origin);
    odb::dbNet* out = makeNet(sta_, parent, "ret_net");
    connectRegister(ff, cls, nets[k - 1], out, chain.clock_net, mod_nets);
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
                                                const int p,
                                                Origins& known)
{
  const int taken_by_sink_gate = -r[graph.adj()[sink.source][sink.edge].target];
  return originRegistersOnEdge(graph,
                               r,
                               sink.source,
                               sink.edge,
                               taken_by_sink_gate + sink.registers - p + 1,
                               known);
}

Retimer::Chain Retimer::planChain(const RetimingGraph& graph,
                                  const std::vector<int>& r,
                                  const Group& group,
                                  Origins& known) const
{
  checkRebuildable(graph, group);
  const int max_depth
      = std::ranges::max(group.sinks, {}, &Sink::registers).registers;
  Chain chain{
      .classes = std::vector<const RegisterClass*>(max_depth, nullptr),
      .clock_net = nullptr,
      .origin = nullptr,
  };
  std::vector<bool> plain_at(max_depth, false);
  for (const Sink& sink : group.sinks) {
    for (int p = 1; p <= sink.registers; ++p) {
      for (odb::dbInst* reg : chainOrigins(graph, r, sink, p, known)) {
        if (reg == nullptr
            || !takeAtDepth(graph.regClass(reg), p - 1, chain, plain_at)) {
          logger_->error(utl::RET,
                         38,
                         "Registers on net {} cannot form one chain; "
                         "retiming not applied.",
                         group.driver_net->getName());
        }
        if (chain.clock_net == nullptr) {
          chain.clock_net = graph.regClass(reg).clock;
        }
        if (chain.origin == nullptr) {
          chain.origin = reg;
        }
      }
    }
  }
  if (chain.clock_net == nullptr) {
    chain.clock_net = clock_net_;
  }
  return chain;
}

void Retimer::checkRebuildable(const RetimingGraph& graph,
                               const Group& group) const
{
  for (odb::dbInst* flop : group.flops) {
    const RegisterClass& cls = graph.regClass(flop);
    if (cls.kind == RegisterKind::kOther || cls.data == nullptr) {
      logger_->error(utl::RET,
                     40,
                     "Cannot rebuild register {} on net {}; retiming not "
                     "applied.",
                     flop->getName(),
                     group.driver_net->getName());
    }
  }
}

bool Retimer::takeAtDepth(const RegisterClass& cls,
                          const int depth,
                          Chain& chain,
                          std::vector<bool>& plain_at)
{
  if (cls.kind == RegisterKind::kPlain) {
    plain_at[depth] = true;
    return chain.classes[depth] == nullptr;
  }
  if (!keepsClass(cls)) {
    return false;
  }
  const RegisterClass* taken = chain.classes[depth];
  chain.classes[depth] = &cls;
  return !plain_at[depth]
         && (taken == nullptr || sameRegisterClass(*taken, cls));
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

void Retimer::rebuildGroup(const Group& group, const Chain& chain)
{
  const ModNets mod_nets = groupModNets(group, chain);
  for (odb::dbInst* flop : group.flops) {
    removeRegister(flop);
  }
  if (chain.classes.empty()) {
    return;
  }
  const std::vector<odb::dbNet*> depth_net
      = buildChain(group.driver_net, chain, mod_nets);
  for (const Sink& sink : group.sinks) {
    if (sink.registers > 0) {
      movePin(sink.pin, depth_net[sink.registers]);
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
  Origins known;
  for (const Group& group : plan.groups) {
    if (group.dont_touch) {
      logger_->error(utl::RET,
                     27,
                     "Net {} has a don't-touch register; retiming not "
                     "applied.",
                     group.driver_net->getName());
    }
    chains.push_back(planChain(graph, r, group, known));
  }
  checkCrossings(graph, plan);

  for (size_t i = 0; i < plan.groups.size(); ++i) {
    // One gate drives the net of a group that changes: the registers on a
    // net with two drivers stay.
    trial_sources_.push_back(plan.groups[i].sinks.front().source);
    rebuildGroup(plan.groups[i], chains[i]);
  }
}

}  // namespace ret
