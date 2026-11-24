// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ret/Retimer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
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
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

bool oneClass(const RetimingGraph& graph, const std::vector<odb::dbInst*>& regs)
{
  for (size_t i = 1; i < regs.size(); ++i) {
    if (!sameRegisterClass(graph.regClass(regs[0]), graph.regClass(regs[i]))) {
      return false;
    }
  }
  return true;
}

bool moveKeepsClass(const RetimingGraph& graph,
                    const std::vector<odb::dbInst*>& regs,
                    int v)
{
  return oneClass(graph, regs)
         && (regs.empty()
             || graph.regClass(regs[0]).kind != RegisterKind::kInverting
             || graph.selfDual()[v]);
}

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
                      const std::vector<odb::dbInst*>& regs,
                      std::unordered_set<odb::dbInst*>& movers)
{
  for (odb::dbInst* reg : regs) {
    if (reg != nullptr && keepsClass(graph.regClass(reg))) {
      movers.insert(reg);
    }
  }
}

std::optional<int> lagOfNewGate(
    odb::dbInst* gate,
    const std::unordered_map<std::string, int>& lag)
{
  constexpr int kMaxDepth = 64;
  for (int depth = 0; depth <= kMaxDepth; ++depth) {
    auto it = lag.find(gate->getName());
    if (it != lag.end()) {
      return it->second;
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

// nullopt when a gate's lag cannot be traced through the unmapping.
std::optional<std::vector<int>> remapLag(
    const RetimingGraph& graph,
    const std::unordered_map<std::string, int>& lag)
{
  std::vector<int> r;
  for (odb::dbInst* gate : graph.nodes()) {
    const std::optional<int> gate_lag = lagOfNewGate(gate, lag);
    if (!gate_lag) {
      return std::nullopt;
    }
    r.push_back(*gate_lag);
  }
  return r;
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
      logger_(logger),
      unmapper_(std::make_unique<SequentialUnmapper>(db, sta, logger)),
      placer_(
          std::make_unique<RetimerPlacer>(sta, estimate_parasitics, resizer))
{
}

Retimer::~Retimer() = default;

odb::dbBlock* Retimer::getBlock()
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip != nullptr ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::RET, 11, "No design loaded.");
  }
  return block;
}

void Retimer::retime(int max_moves)
{
  odb::dbBlock* block = getBlock();
  if (isRouted(block)) {
    logger_->warn(utl::RET,
                  44,
                  "The design is routed; retime does not update the routing.");
  }
  if (!unmapper_->prepare()) {
    logger_->error(
        utl::RET,
        36,
        "The loaded libraries lack a usable base register, inverter, 2-input "
        "AND or 2-input OR cell (see the warnings above).");
  }
  checkRegisters(block);

  placer_->forgetCells();
  report_graph_ = true;

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  const int unplaced_before = countPlacement(block).unplaced;
  odb::dbDatabase::beginEco(block);
  try {
    StaCheck check(
        sta_, logger_, placer_.get(), block, &guard, [&](const int limit) {
          return searchAndApply(block, limit, guard);
        });
    check.run(max_moves);
  } catch (...) {
    odb::dbDatabase::undoEco(block);
    throw;
  }
  odb::dbDatabase::commitEco(block);

  const PlacementCount after = countPlacement(block);
  if (after.placed > 0 && after.unplaced > unplaced_before) {
    logger_->info(utl::RET,
                  45,
                  "New instances left unplaced: {}. Run global_placement "
                  "-incremental to place them.",
                  after.unplaced - unplaced_before);
  }
}

int Retimer::searchAndApply(odb::dbBlock* block,
                            int max_moves,
                            est::IncrementalParasiticsGuard& guard)
{
  RetimingGraph graph(db_, sta_, logger_);
  graph.build();
  if (std::exchange(report_graph_, false)) {
    graph.report();
  }
  clock_net_ = findClockNet(block);

  Retime search = newSearch(graph);
  search.run(max_moves);
  const std::vector<int>& r = search.getRetiming();

  const std::unordered_set<odb::dbInst*> movers = moversToUnmap(graph, r);
  if (movers.empty()) {
    applyRetiming(graph, r);
    return search.getMoves();
  }
  return applyAfterUnmapping(graph, search, movers, max_moves, guard);
}

Retime Retimer::newSearch(const RetimingGraph& graph)
{
  return Retime(
      &graph,
      sta_,
      logger_,
      unmapper_->getBaseSequentialDMTerm(),
      [this](odb::dbMaster* master) { return placer_->registerCells(master); });
}

int Retimer::applyAfterUnmapping(const RetimingGraph& graph,
                                 const Retime& search,
                                 const std::unordered_set<odb::dbInst*>& movers,
                                 int max_moves,
                                 est::IncrementalParasiticsGuard& guard)
{
  const std::vector<int>& r = search.getRetiming();
  // Keyed by name: unmapping deletes instances, and a new instance can
  // take the address of a deleted one.
  std::unordered_map<std::string, int> lag;
  const int n = static_cast<int>(graph.nodes().size());
  for (int i = 0; i < n; ++i) {
    lag[graph.nodes()[i]->getName()] = r[i];
  }
  unmapper_->run(&movers);
  guard.update();

  RetimingGraph graph2(db_, sta_, logger_);
  graph2.build();
  const std::optional<std::vector<int>> r2 = remapLag(graph2, lag);
  if (r2 && moversToUnmap(graph2, *r2).empty()) {
    applyRetiming(graph2, *r2);
    return search.getMoves();
  }

  constexpr int kMaxRounds = 20;
  for (int round = 0; round < kMaxRounds; ++round) {
    RetimingGraph g(db_, sta_, logger_);
    g.build();
    Retime s = newSearch(g);
    s.run(max_moves);
    const std::vector<int>& rr = s.getRetiming();
    const std::unordered_set<odb::dbInst*> more = moversToUnmap(g, rr);
    if (more.empty()) {
      applyRetiming(g, rr);
      return s.getMoves();
    }
    unmapper_->run(&more);
    guard.update();
  }
  logger_->error(utl::RET,
                 37,
                 "Retiming did not settle after {} rounds of unmapping "
                 "registers; the retiming is not applied.",
                 kMaxRounds);
  return 0;
}

odb::dbNet* Retimer::findClockNet(odb::dbBlock* block)
{
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential()) {
      odb::dbNet* clock = registerClockNet(inst);
      if (clock != nullptr) {
        return clock;
      }
    }
  }
  logger_->error(utl::RET, 29, "No register with a clock pin found.");
  return nullptr;
}

void Retimer::checkRegisters(odb::dbBlock* block)
{
  int negedge = 0;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential() && isNegedgeRegister(sta_, inst)) {
      ++negedge;
    }
  }
  if (negedge > 0) {
    logger_->error(utl::RET,
                   35,
                   "{} negative-edge registers found; only positive-edge "
                   "registers can be retimed.",
                   negedge);
  }
}

Retimer::FaninIndex Retimer::faninIndex(const RetimingGraph& graph)
{
  const int n = static_cast<int>(graph.nodes().size());
  FaninIndex fanin(n);
  for (int u = 0; u < n; ++u) {
    const int edge_count = static_cast<int>(graph.adj()[u].size());
    for (int k = 0; k < edge_count; ++k) {
      fanin[graph.adj()[u][k].target].emplace_back(u, k);
    }
  }
  return fanin;
}

void Retimer::originRegistersOnEdge(const RetimingGraph& graph,
                                    const FaninIndex& fanin,
                                    const std::vector<int>& r,
                                    int u,
                                    int k,
                                    int j,
                                    std::vector<odb::dbInst*>& out)
{
  const int w = graph.adj()[u][k].weight;
  if (w > 0) {
    const std::vector<odb::dbInst*>& flops = graph.phys()[u][k].flops;
    const int index = w - j;
    const int count = static_cast<int>(flops.size());
    out.push_back(index >= 0 && index < count ? flops[index] : nullptr);
    return;
  }
  if (j > -r[u]) {
    out.push_back(nullptr);
    return;
  }
  originRegisters(graph, fanin, r, u, j, out);
}

void Retimer::originRegisters(const RetimingGraph& graph,
                              const FaninIndex& fanin,
                              const std::vector<int>& r,
                              int v,
                              int j,
                              std::vector<odb::dbInst*>& out)
{
  if (fanin[v].empty()) {
    out.push_back(nullptr);
    return;
  }
  for (const auto& [u, k] : fanin[v]) {
    originRegistersOnEdge(graph, fanin, r, u, k, j, out);
  }
}

std::vector<Retimer::Group> Retimer::collectGroups(
    const RetimingGraph& graph,
    const std::vector<int>& r) const
{
  // Ordered by net and class group: deterministic new-cell names.
  std::map<std::pair<uint32_t, int>, Group> by_net;
  std::unordered_map<odb::dbNet*, std::vector<odb::dbInst*>> heads;
  const int n = static_cast<int>(graph.nodes().size());
  for (int u = 0; u < n; ++u) {
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
            {.pin = handles[k].sink, .wr = wr, .u = u, .k = k});
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
  groups.reserve(by_net.size());
  for (auto& [id, group] : by_net) {
    groups.push_back(std::move(group));
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
    if (!oneClass(graph, at_position)) {
      addClassedMovers(graph, at_position, movers);
    }
  }
}

std::unordered_set<odb::dbInst*> Retimer::moversToUnmap(
    const RetimingGraph& graph,
    const std::vector<int>& r) const
{
  std::unordered_set<odb::dbInst*> movers;
  for (const Group& group : collectGroups(graph, r)) {
    if (group.changed && !group.dont_touch) {
      addGroupMovers(graph, group, movers);
    }
  }
  const FaninIndex fanin = faninIndex(graph);
  const int n = static_cast<int>(graph.nodes().size());
  for (int v = 0; v < n; ++v) {
    for (int j = 1; j <= -r[v]; ++j) {
      std::vector<odb::dbInst*> regs;
      originRegisters(graph, fanin, r, v, j, regs);
      if (std::find(regs.begin(), regs.end(), nullptr) == regs.end()
          && !moveKeepsClass(graph, regs, v)) {
        addClassedMovers(graph, regs, movers);
      }
    }
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
                   "Register {} cannot be bypassed; the retiming is not "
                   "applied.",
                   inst->getName());
  }
  // In a hierarchical netlist the loads are in the register's module, so
  // they take its D pin's module net along with the flat net.
  odb::dbModNet* d_mod_net = d->getModNet();
  odb::dbITerm* q
      = cls.output != nullptr ? inst->getITerm(cls.output) : nullptr;
  odb::dbNet* q_net = q != nullptr ? q->getNet() : nullptr;
  odb::dbModNet* q_mod_net = q != nullptr ? q->getModNet() : nullptr;
  if (q_net != nullptr) {
    std::vector<odb::dbITerm*> loads;
    for (odb::dbITerm* iterm : q_net->getITerms()) {
      if (iterm != q) {
        loads.push_back(iterm);
      }
    }
    const std::vector<odb::dbBTerm*> ports(q_net->getBTerms().begin(),
                                           q_net->getBTerms().end());
    for (odb::dbITerm* load : loads) {
      load->disconnect();
      load->connect(d_net);
      if (d_mod_net != nullptr) {
        load->connect(d_mod_net);
      }
    }
    for (odb::dbBTerm* bterm : ports) {
      bterm->disconnect();
      bterm->connect(d_net);
    }
  }
  deleteRegister(sta_, inst);
  if (q_net != nullptr) {
    sta::dbNetwork* network = sta_->getDbNetwork();
    network->deleteNet(network->dbToSta(q_net));
  }
  if (q_mod_net != nullptr && q_mod_net->connectionCount() == 0) {
    odb::dbModNet::destroy(q_mod_net);
  }
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
  for (odb::dbITerm* iterm : ff->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      connectPin(iterm, clock_net, mod_nets);
    } else if (cls == nullptr) {
      if (iterm->getMTerm() == unmapper_->getBaseSequentialDMTerm()) {
        connectPin(iterm, in, mod_nets);
      } else if (iterm->getMTerm() == unmapper_->getBaseSequentialQMTerm()) {
        iterm->connect(out);
      }
    } else if (iterm->getMTerm() == cls->data) {
      connectPin(iterm, in, mod_nets);
    } else if (iterm->getMTerm() == cls->output) {
      iterm->connect(out);
    } else {
      for (const auto& [mterm, net] : cls->enables) {
        if (iterm->getMTerm() == mterm) {
          connectPin(iterm, net, mod_nets);
        }
      }
    }
  }
}

void Retimer::tieBaseControls(odb::dbInst* ff,
                              sta::Instance* parent,
                              odb::dbModInst* parent_mod_inst)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  for (const auto& [mterm, high] : unmapper_->getBaseSequentialTies()) {
    odb::dbMaster* master = unmapper_->getTieMaster(high);
    sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
    const std::string name = getBlock()->makeNewInstName(
        parent_mod_inst,
        "ret_tie",
        odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
    odb::dbInst* tie
        = network->staToDb(network->makeInstance(cell, name, parent));
    tie->setSourceType(odb::dbSourceType::TIMING);
    odb::dbNet* net = network->staToDb(network->makeNet(
        "ret_net", parent, odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE));
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
  odb::dbModInst* parent_mod_inst = network->getModInst(parent);

  const int depth = static_cast<int>(classes.size());
  std::vector<odb::dbNet*> nets(depth + 1);
  nets[0] = driver_net;
  for (int k = 1; k <= depth; ++k) {
    const RegisterClass* cls = classes[k - 1];
    odb::dbMaster* master
        = cls != nullptr ? cls->master : unmapper_->getBaseSequentialMaster();
    sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
    const std::string name = getBlock()->makeNewInstName(
        parent_mod_inst,
        "ret_ff",
        odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
    odb::dbInst* ff
        = network->staToDb(network->makeInstance(cell, name, parent));
    ff->setSourceType(odb::dbSourceType::TIMING);
    odb::dbNet* out = network->staToDb(network->makeNet(
        "ret_net", parent, odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE));
    connectRegister(ff, cls, nets[k - 1], out, clock_net, mod_nets);
    if (cls == nullptr) {
      tieBaseControls(ff, parent, parent_mod_inst);
    }
    nets[k] = out;
  }
  return nets;
}

std::vector<odb::dbInst*> Retimer::chainOrigins(const RetimingGraph& graph,
                                                const FaninIndex& fanin,
                                                const std::vector<int>& r,
                                                const Sink& sink,
                                                int p)
{
  const int taken_by_sink_gate = -r[graph.adj()[sink.u][sink.k].target];
  std::vector<odb::dbInst*> regs;
  originRegistersOnEdge(graph,
                        fanin,
                        r,
                        sink.u,
                        sink.k,
                        taken_by_sink_gate + sink.wr - p + 1,
                        regs);
  return regs;
}

std::vector<const RegisterClass*> Retimer::chainClasses(
    const RetimingGraph& graph,
    const FaninIndex& fanin,
    const std::vector<int>& r,
    const Group& group) const
{
  for (odb::dbInst* f : group.flops) {
    const RegisterClass& cls = graph.regClass(f);
    if (cls.kind == RegisterKind::kOther || cls.data == nullptr) {
      logger_->error(utl::RET,
                     40,
                     "Register {} on net {} cannot be rebuilt; the retiming "
                     "is not applied.",
                     f->getName(),
                     group.driver_net->getName());
    }
  }
  int max_depth = 0;
  for (const Sink& s : group.sinks) {
    max_depth = std::max(max_depth, s.wr);
  }
  std::vector<const RegisterClass*> classes(max_depth, nullptr);
  std::vector<bool> plain_at(max_depth, false);
  for (const Sink& s : group.sinks) {
    for (int p = 1; p <= s.wr; ++p) {
      for (odb::dbInst* reg : chainOrigins(graph, fanin, r, s, p)) {
        const RegisterClass* cls
            = reg != nullptr ? &graph.regClass(reg) : nullptr;
        bool consistent = cls != nullptr;
        if (consistent && cls->kind == RegisterKind::kPlain) {
          consistent = classes[p - 1] == nullptr;
          plain_at[p - 1] = true;
        } else if (consistent && keepsClass(*cls)) {
          consistent = !plain_at[p - 1]
                       && (classes[p - 1] == nullptr
                           || sameRegisterClass(*classes[p - 1], *cls));
          classes[p - 1] = cls;
        } else {
          consistent = false;
        }
        if (!consistent) {
          logger_->error(utl::RET,
                         38,
                         "Register classes on net {} cannot be rebuilt as "
                         "one chain; the retiming is not applied.",
                         group.driver_net->getName());
        }
      }
    }
  }
  return classes;
}

void Retimer::checkMoves(const RetimingGraph& graph,
                         const FaninIndex& fanin,
                         const std::vector<int>& r) const
{
  const int n = static_cast<int>(graph.nodes().size());
  for (int v = 0; v < n; ++v) {
    for (int j = 1; j <= -r[v]; ++j) {
      std::vector<odb::dbInst*> regs;
      originRegisters(graph, fanin, r, v, j, regs);
      if (std::find(regs.begin(), regs.end(), nullptr) != regs.end()
          || !moveKeepsClass(graph, regs, v)) {
        logger_->error(utl::RET,
                       41,
                       "The registers moved across {} are not one class that "
                       "can cross it; the retiming is not applied.",
                       graph.nodes()[v]->getName());
      }
    }
  }
}

void Retimer::rebuildGroup(const Group& group,
                           const std::vector<const RegisterClass*>& classes,
                           odb::dbNet* clock_net)
{
  const ModNets mod_nets = groupModNets(group, classes, clock_net);
  // In id order: odb gives a new instance the slot of a deleted one, so the
  // order of removal decides the ids of the new registers.
  std::vector<odb::dbInst*> flops(group.flops.begin(), group.flops.end());
  std::ranges::sort(flops, [](odb::dbInst* a, odb::dbInst* b) {
    return a->getId() < b->getId();
  });
  for (odb::dbInst* f : flops) {
    removeRegister(f);
  }
  if (classes.empty()) {
    return;
  }
  const std::vector<odb::dbNet*> depth_net
      = buildChain(group.driver_net, clock_net, classes, mod_nets);
  for (const Sink& s : group.sinks) {
    if (s.wr > 0) {
      s.pin->disconnect();
      s.pin->connect(depth_net[s.wr]);
    }
  }
}

odb::dbNet* Retimer::groupClockNet(const RetimingGraph& graph,
                                   const FaninIndex& fanin,
                                   const std::vector<int>& r,
                                   const Group& group) const
{
  for (const Sink& s : group.sinks) {
    for (int p = 1; p <= s.wr; ++p) {
      for (odb::dbInst* reg : chainOrigins(graph, fanin, r, s, p)) {
        odb::dbNet* clock = reg != nullptr ? registerClockNet(reg) : nullptr;
        if (clock != nullptr) {
          return clock;
        }
      }
    }
  }
  return clock_net_;
}

void Retimer::applyRetiming(const RetimingGraph& graph,
                            const std::vector<int>& r)
{
  const std::vector<Group> groups = collectGroups(graph, r);
  if (std::ranges::none_of(groups, [](const Group& g) { return g.changed; })) {
    return;
  }

  const FaninIndex fanin = faninIndex(graph);
  std::vector<std::vector<const RegisterClass*>> classes(groups.size());
  std::vector<odb::dbNet*> clock_nets(groups.size(), nullptr);
  for (size_t i = 0; i < groups.size(); ++i) {
    if (!groups[i].changed) {
      continue;
    }
    if (groups[i].dont_touch) {
      logger_->error(utl::RET,
                     27,
                     "The registers on net {} include a don't-touch register "
                     "and cannot be rebuilt; the retiming is not applied.",
                     groups[i].driver_net->getName());
    }
    classes[i] = chainClasses(graph, fanin, r, groups[i]);
    clock_nets[i] = groupClockNet(graph, fanin, r, groups[i]);
  }
  checkMoves(graph, fanin, r);

  for (size_t i = 0; i < groups.size(); ++i) {
    if (groups[i].changed) {
      rebuildGroup(groups[i], classes[i], clock_nets[i]);
    }
  }
}

}  // namespace ret
