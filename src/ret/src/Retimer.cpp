// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "ret/Retimer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "Retime.h"
#include "RetimingGraph.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "ret/SequentialUnmapper.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

// The class recorded for a register when the graph was built.
const RegisterClass& classOf(const RetimingGraph& graph, odb::dbInst* reg)
{
  static const RegisterClass kOther;
  auto it = graph.reg_class_.find(reg);
  return it != graph.reg_class_.end() ? it->second : kOther;
}

// True for a register that is rebuilt as its own cell, not as the base cell.
bool keepsClass(const RegisterClass& cls)
{
  return cls.kind == RegisterKind::enable
         || cls.kind == RegisterKind::inverting;
}

// True when every register in `regs` may be rebuilt as one chain position:
// all plain, or all enable registers of one class.
bool oneClass(const RetimingGraph& graph, const std::vector<odb::dbInst*>& regs)
{
  for (size_t i = 1; i < regs.size(); i++) {
    if (!sameRegisterClass(classOf(graph, regs[0]), classOf(graph, regs[i]))) {
      return false;
    }
  }
  return true;
}

// True when the registers a move takes across gate v are one class and that
// class may cross the gate: inverting registers need a self-dual gate.
bool moveKeepsClass(const RetimingGraph& graph,
                    const std::vector<odb::dbInst*>& regs,
                    int v)
{
  if (!oneClass(graph, regs)) {
    return false;
  }
  return regs.empty() || classOf(graph, regs[0]).kind != RegisterKind::inverting
         || graph.self_dual_[v];
}

// The net feeding a register's data input, from the class the graph cached.
odb::dbNet* registerDataNet(const RetimingGraph& graph,
                            sta::dbSta* sta,
                            odb::dbInst* inst)
{
  const RegisterClass& cls = classOf(graph, inst);
  odb::dbITerm* d = cls.data != nullptr ? inst->getITerm(cls.data)
                                        : registerDataPin(sta, inst);
  return d != nullptr ? d->getNet() : nullptr;
}

// The net an edge's register chain starts on: the first register's D net,
// or the driver pin's net when the edge holds no register.
odb::dbNet* edgeDriverNet(const RetimingGraph& graph,
                          sta::dbSta* sta,
                          const EdgePhys& phys)
{
  if (phys.flops.empty()) {
    return phys.driver->getNet();
  }
  return registerDataNet(graph, sta, phys.flops.front());
}

// Centroid of the placed registers of a group, else the location of the
// group's driving instance; false when nothing is placed.
bool chainLocation(const std::unordered_set<odb::dbInst*>& flops,
                   odb::dbInst* driver,
                   odb::Point& location)
{
  int64_t sx = 0;
  int64_t sy = 0;
  int placed = 0;
  for (odb::dbInst* f : flops) {
    if (f->isPlaced()) {
      const odb::Point pt = f->getLocation();
      sx += pt.x();
      sy += pt.y();
      placed++;
    }
  }
  if (placed > 0) {
    location = odb::Point(static_cast<int>(sx / placed),
                          static_cast<int>(sy / placed));
    return true;
  }
  if (driver->isPlaced()) {
    location = driver->getLocation();
    return true;
  }
  return false;
}

}  // namespace

Retimer::Retimer(odb::dbDatabase* db,
                 sta::dbSta* sta,
                 dpl::Opendp* opendp,
                 est::EstimateParasitics* estimate_parasitics,
                 utl::Logger* logger)
    : db_(db),
      sta_(sta),
      opendp_(opendp),
      estimate_parasitics_(estimate_parasitics),
      logger_(logger),
      unmapper_(std::make_unique<SequentialUnmapper>(db, sta, logger))
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
  if (sta_->getDbNetwork()->hasHierarchy()) {
    logger_->error(utl::RET,
                   42,
                   "Hierarchical netlists are not supported yet; link the "
                   "design without -hier.");
  }
  if (!unmapper_->prepare()) {
    logger_->error(
        utl::RET,
        36,
        "The loaded libraries lack a usable base register, inverter, 2-input "
        "AND or 2-input OR cell (see the warnings above).");
  }
  dff_master_ = unmapper_->getBaseSequentialMaster();
  dff_q_mterm_ = unmapper_->getBaseSequentialQMTerm();
  checkRegisters(block);

  RetimingGraph graph(db_, sta_, logger_);
  graph.build();
  graph.report();
  if (graph.clock_period_ <= 0.0f) {
    logger_->error(
        utl::RET, 30, "No clock defined; retiming needs a clock period.");
  }
  clock_net_ = findClockNet(block);

  // Every netlist edit below marks the nets it touches, and their estimated
  // parasitics are refreshed before the next search and when the guard
  // goes out of scope.
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);

  Retime search(&graph, sta_, logger_, dff_master_);
  search.run(max_moves);
  const std::vector<int> r = search.getRetiming();

  const std::unordered_set<odb::dbInst*> movers = moversToUnmap(graph, r);
  if (movers.empty()) {
    applyRetiming(graph, r);
  } else {
    applyAfterUnmapping(graph, r, movers, max_moves, guard);
  }
}

// Unmap the registers that cannot move as they are, carry the lags over to
// the new netlist and apply; when the carry-over is ambiguous, solve again
// on the new netlist, unmapping more registers if that solution needs it.
void Retimer::applyAfterUnmapping(
    const RetimingGraph& graph,
    const std::vector<int>& r,
    const std::unordered_set<odb::dbInst*>& movers,
    int max_moves,
    est::IncrementalParasiticsGuard& guard)
{
  std::map<unsigned, odb::dbInst*> ordered_movers;
  for (odb::dbInst* mover : movers) {
    ordered_movers[mover->getId()] = mover;
  }
  for (const auto& [id, mover] : ordered_movers) {
    debugPrint(logger_,
               utl::RET,
               "unmap",
               1,
               "{} ({}) moves and cannot keep its class; unmapping it first",
               mover->getName(),
               mover->getMaster()->getName());
  }

  // Normalize only the registers that must change, then carry the lags over.
  std::unordered_map<odb::dbInst*, int> lag;
  const int n = static_cast<int>(graph.nodes_.size());
  for (int i = 0; i < n; i++) {
    lag[graph.nodes_[i]] = r[i];
  }
  unmapper_->run(&movers);
  guard.update();

  RetimingGraph graph2(db_, sta_, logger_);
  graph2.build();
  bool ok = true;
  const std::vector<int> r2 = remapLag(graph2, lag, ok);
  if (ok && moversToUnmap(graph2, r2).empty()) {
    applyRetiming(graph2, r2);
    return;
  }

  debugPrint(logger_,
             utl::RET,
             "unmap",
             1,
             "lag remap is ambiguous after unmapping; re-solving on the new "
             "netlist");
  constexpr int kMaxRounds = 20;
  for (int round = 0; round < kMaxRounds; round++) {
    RetimingGraph g(db_, sta_, logger_);
    g.build();
    Retime s(&g, sta_, logger_, dff_master_);
    s.run(max_moves);
    const std::vector<int>& rr = s.getRetiming();
    const std::unordered_set<odb::dbInst*> more = moversToUnmap(g, rr);
    if (more.empty()) {
      applyRetiming(g, rr);
      return;
    }
    unmapper_->run(&more);
    guard.update();
  }
  logger_->error(utl::RET,
                 37,
                 "Retiming did not settle after {} rounds of unmapping "
                 "registers; the design is left with the registers unmapped so "
                 "far and no register moved.",
                 kMaxRounds);
}

// Clock net of the first register, the fallback for a rebuilt chain whose
// group has no register to inherit a clock net from.
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

// Register kinds the retimer does not handle yet are an error, not a silent
// no-op.
void Retimer::checkRegisters(odb::dbBlock* block)
{
  int negedge = 0;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential() && isNegedgeRegister(sta_, inst)) {
      negedge++;
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
  const int n = static_cast<int>(graph.nodes_.size());
  FaninIndex fanin(n);
  for (int u = 0; u < n; u++) {
    const int edge_count = static_cast<int>(graph.adj_[u].size());
    for (int k = 0; k < edge_count; k++) {
      fanin[graph.adj_[u][k].target].emplace_back(u, k);
    }
  }
  return fanin;
}

// Along edge (u, k): an edge that held registers in the original netlist
// keeps them in driver-to-sink order and only loses them from the sink end
// (its source gate never moves, see Retime::forwardMovable), so the j-th
// register upstream of the sink is the j-th from the end of that chain.  An
// edge that held none carries registers pushed through u, and the j-th of
// those is what the j-th register upstream of u stood for.
void Retimer::originRegistersOnEdge(const RetimingGraph& graph,
                                    const FaninIndex& fanin,
                                    const std::vector<int>& r,
                                    int u,
                                    int k,
                                    int j,
                                    std::vector<odb::dbInst*>& out)
{
  const int w = graph.adj_[u][k].weight;
  if (w > 0) {
    const std::vector<odb::dbInst*>& flops = graph.phys_[u][k].flops;
    const int index = w - j;
    const int count = static_cast<int>(flops.size());
    out.push_back(index >= 0 && index < count ? flops[index] : nullptr);
    return;
  }
  if (j > -r[u]) {
    out.push_back(nullptr);  // u did not push that many registers
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
  // Keyed by net id so the order, and with it the names of the new
  // instances, is the same on every run.
  std::map<unsigned, Group> by_net;
  const int n = static_cast<int>(graph.nodes_.size());
  for (int u = 0; u < n; u++) {
    const std::vector<Edge>& edges = graph.adj_[u];
    const std::vector<EdgePhys>& handles = graph.phys_[u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; k++) {
      odb::dbNet* driver_net = edgeDriverNet(graph, sta_, handles[k]);
      if (driver_net != nullptr) {
        const int w = edges[k].weight;
        const int wr = w + r[edges[k].target] - r[u];
        Group& group = by_net[driver_net->getId()];
        group.driver_net = driver_net;
        group.driver = handles[k].driver;
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

std::unordered_set<odb::dbInst*> Retimer::moversToUnmap(
    const RetimingGraph& graph,
    const std::vector<int>& r) const
{
  std::unordered_set<odb::dbInst*> movers;
  // Registers that would otherwise keep their class become plain.
  auto unmap_classed = [&](const std::vector<odb::dbInst*>& regs) {
    for (odb::dbInst* reg : regs) {
      if (reg != nullptr && keepsClass(classOf(graph, reg))) {
        movers.insert(reg);
      }
    }
  };

  for (const Group& group : collectGroups(graph, r)) {
    if (group.changed && !group.dont_touch) {
      // A rebuilt chain is the base cell for a plain register and the
      // original cell for an enable or inverting register; anything else
      // has to become plain first.
      for (odb::dbInst* f : group.flops) {
        const RegisterClass& cls = classOf(graph, f);
        if (cls.kind == RegisterKind::other
            || (cls.kind == RegisterKind::plain
                && f->getMaster() != dff_master_)) {
          movers.insert(f);
        }
      }
      // Registers that share a position in a group's chains are rebuilt as
      // one register, so they must be of one class.
      size_t longest = 0;
      for (const std::vector<odb::dbInst*>* chain : group.chains) {
        longest = std::max(longest, chain->size());
      }
      for (size_t i = 0; i < longest; i++) {
        std::vector<odb::dbInst*> at_position;
        for (const std::vector<odb::dbInst*>* chain : group.chains) {
          if (i < chain->size()) {
            at_position.push_back((*chain)[i]);
          }
        }
        if (!oneClass(graph, at_position)) {
          unmap_classed(at_position);
        }
      }
    }
  }

  // A move across gate v takes one register from every fanin edge; those
  // registers merge into one and so must be of one class.  Inverting
  // registers can also only cross a self-dual gate.  Either conflict is
  // resolved by unmapping the classed registers involved, which makes them
  // plain.
  const FaninIndex fanin = faninIndex(graph);
  const int n = static_cast<int>(graph.nodes_.size());
  for (int v = 0; v < n; v++) {
    for (int j = 1; j <= -r[v]; j++) {
      std::vector<odb::dbInst*> regs;
      originRegisters(graph, fanin, r, v, j, regs);
      // A position the retiming never filled is reported by applyRetiming.
      if (std::find(regs.begin(), regs.end(), nullptr) == regs.end()
          && !moveKeepsClass(graph, regs, v)) {
        unmap_classed(regs);
      }
    }
  }
  return movers;
}

// A gate the unmapper created sits in a single-input chain in front of an
// original gate (inverter) or is a mux/AND/OR gate with several inputs.  The
// lag of an original gate is known; a new single-input gate takes the lag of
// what it drives through; anything else is ambiguous.
int Retimer::lagOfNewGate(odb::dbInst* gate,
                          const std::unordered_map<odb::dbInst*, int>& lag,
                          bool& ok) const
{
  constexpr int kMaxDepth = 64;
  for (int depth = 0; depth <= kMaxDepth; depth++) {
    auto it = lag.find(gate);
    if (it != lag.end()) {
      return it->second;
    }
    if (gate->getMaster()->isSequential()) {
      ok = false;
      return 0;
    }
    odb::dbInst* driver = nullptr;
    int inputs = 0;
    for (odb::dbITerm* pin : gate->getITerms()) {
      if (pin->getIoType() == odb::dbIoType::INPUT
          && pin->getSigType() != odb::dbSigType::CLOCK) {
        inputs++;
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
    if (inputs != 1 || driver == nullptr) {
      ok = false;
      return 0;
    }
    gate = driver;
  }
  ok = false;
  return 0;
}

std::vector<int> Retimer::remapLag(
    const RetimingGraph& graph,
    const std::unordered_map<odb::dbInst*, int>& lag,
    bool& ok) const
{
  ok = true;
  const int n = static_cast<int>(graph.nodes_.size());
  std::vector<int> r(n, 0);
  for (int i = 0; i < n && ok; i++) {
    r[i] = lagOfNewGate(graph.nodes_[i], lag, ok);
  }
  return r;
}

// Bypass and delete one register: the loads of the output pin it drives move
// onto its D net.  The class names that output pin (a register cell can have
// a second, unconnected one), so the bypass never guesses it.
void Retimer::removeRegister(odb::dbInst* inst)
{
  const RegisterClass cls = registerClass(sta_, inst);
  odb::dbITerm* d = cls.data != nullptr ? inst->getITerm(cls.data) : nullptr;
  odb::dbNet* d_net = d != nullptr ? d->getNet() : nullptr;
  if (cls.kind == RegisterKind::other || d_net == nullptr) {
    logger_->error(utl::RET,
                   39,
                   "Register {} cannot be bypassed; the retiming is only "
                   "partly applied.",
                   inst->getName());
  }
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
    const std::vector<odb::dbBTerm*> ports(q_net->getBTerms().begin(),
                                           q_net->getBTerms().end());
    for (odb::dbITerm* load : loads) {
      load->disconnect();
      load->connect(d_net);
    }
    for (odb::dbBTerm* bterm : ports) {
      bterm->disconnect();
      bterm->connect(d_net);
    }
  }
  odb::dbInst::destroy(inst);
  if (q_net != nullptr) {
    odb::dbNet::destroy(q_net);
  }
}

// A chain of registers fed by driver_net; nets[k] is the net after the k-th
// register.  Position k is the base cell, or the cell of classes[k-1] with
// its data pin on the chain, its enable pins on the class nets and its
// output pin feeding on.  New cells are created next to the gate driving
// driver_net (its parent instance, the top level in a flat netlist) with
// names the block makes unique, and start at `location` (the spot of the
// registers they replace) so detailed placement can legalize them; detailed
// placement skips unplaced instances.  With routed parasitics the cells are
// legalized here, as the resizer does.
std::vector<odb::dbNet*> Retimer::buildChain(
    odb::dbNet* driver_net,
    odb::dbNet* clock_net,
    const std::vector<const RegisterClass*>& classes,
    const odb::Point* location)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbITerm* source = driver_net->getFirstOutput();
  sta::Instance* parent
      = source != nullptr
            ? network->getOwningInstanceParent(network->dbToSta(source))
            : network->topInstance();
  odb::dbModInst* parent_mod_inst = network->getModInst(parent);
  const est::ParasiticsSrc parasitics_src
      = estimate_parasitics_->getParasiticsSrc();
  const bool legalize
      = parasitics_src == est::ParasiticsSrc::kGlobalRouting
        || parasitics_src == est::ParasiticsSrc::kDetailedRouting;

  const int depth = static_cast<int>(classes.size());
  std::vector<odb::dbNet*> nets(depth + 1);
  nets[0] = driver_net;
  for (int k = 1; k <= depth; k++) {
    const RegisterClass* cls = classes[k - 1];
    odb::dbMaster* master = cls != nullptr ? cls->master : dff_master_;
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
    if (location != nullptr) {
      ff->setLocation(location->x(), location->y());
      ff->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      if (legalize) {
        opendp_->legalCellPos(ff);
      }
    }
    for (odb::dbITerm* iterm : ff->getITerms()) {
      if (iterm->getSigType() == odb::dbSigType::CLOCK) {
        iterm->connect(clock_net);
      } else if (cls == nullptr) {
        if (iterm->getIoType() == odb::dbIoType::INPUT) {
          iterm->connect(nets[k - 1]);
        } else if (iterm->getMTerm() == dff_q_mterm_) {
          iterm->connect(out);
        }
      } else if (iterm->getMTerm() == cls->data) {
        iterm->connect(nets[k - 1]);
      } else if (iterm->getMTerm() == cls->output) {
        iterm->connect(out);
      } else {
        for (const auto& [mterm, net] : cls->controls) {
          if (iterm->getMTerm() == mterm) {
            iterm->connect(net);
          }
        }
      }
    }
    nets[k] = out;
  }
  return nets;
}

// Positions count from the driver; a sink at depth wr sees position wr as
// its first register upstream.  A position must be all plain or all one
// enable class, else the retiming cannot be rebuilt and nothing is applied.
std::vector<const RegisterClass*> Retimer::chainClasses(
    const RetimingGraph& graph,
    const FaninIndex& fanin,
    const std::vector<int>& r,
    const Group& group) const
{
  // Every register of the group is bypassed, so each must be a kind whose
  // data and output pins are known.
  for (odb::dbInst* f : group.flops) {
    const RegisterClass& cls = classOf(graph, f);
    if (cls.kind == RegisterKind::other || cls.data == nullptr) {
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
    // The sink's gate took the first -r registers upstream of it across
    // itself; the wr registers left on the edge are the ones behind those.
    const int taken = -r[graph.adj_[s.u][s.k].target];
    for (int p = 1; p <= s.wr; p++) {
      std::vector<odb::dbInst*> regs;
      originRegistersOnEdge(
          graph, fanin, r, s.u, s.k, taken + s.wr - p + 1, regs);
      for (odb::dbInst* reg : regs) {
        const RegisterClass* cls
            = reg != nullptr ? &classOf(graph, reg) : nullptr;
        bool consistent = cls != nullptr;
        if (consistent && cls->kind == RegisterKind::plain) {
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

// Every move must merge registers of one class, across a gate that class
// may cross; moversToUnmap leaves nothing else behind.
void Retimer::checkMoves(const RetimingGraph& graph,
                         const FaninIndex& fanin,
                         const std::vector<int>& r) const
{
  const int n = static_cast<int>(graph.nodes_.size());
  for (int v = 0; v < n; v++) {
    for (int j = 1; j <= -r[v]; j++) {
      std::vector<odb::dbInst*> regs;
      originRegisters(graph, fanin, r, v, j, regs);
      if (std::find(regs.begin(), regs.end(), nullptr) != regs.end()
          || !moveKeepsClass(graph, regs, v)) {
        logger_->error(utl::RET,
                       41,
                       "The registers moved across {} are not one class that "
                       "can cross it; the retiming is not applied.",
                       graph.nodes_[v]->getName());
      }
    }
  }
}

// Remove the group's registers, build the shared chain on its driver net
// and reconnect every sink at its own depth.
void Retimer::rebuildGroup(const Group& group,
                           const std::vector<const RegisterClass*>& classes,
                           odb::dbNet* clock_net,
                           ApplyStats& stats)
{
  odb::Point location;
  const bool have_location
      = chainLocation(group.flops, group.driver->getInst(), location);

  for (odb::dbInst* f : group.flops) {
    removeRegister(f);
    stats.removed++;
  }
  const int max_depth = static_cast<int>(classes.size());
  if (max_depth == 0) {
    return;  // purely combinational now
  }
  const std::vector<odb::dbNet*> depth_net
      = buildChain(group.driver_net,
                   clock_net,
                   classes,
                   have_location ? &location : nullptr);
  stats.inserted += max_depth;
  for (const RegisterClass* cls : classes) {
    if (cls != nullptr) {
      stats.inserted_classed++;
    }
  }
  for (const Sink& s : group.sinks) {
    // wr == 0: the sink already sits on the driver net after removeRegister
    if (s.wr > 0) {
      s.pin->disconnect();
      s.pin->connect(depth_net[s.wr]);
    }
  }
}

// Minimal-perturbation apply: rebuild only the register groups whose edge
// weights changed.  Within a group the old registers are removed, one shared
// chain of the maximum new depth is built on the group's driver net, and each
// sink is reconnected at its own depth.  Every check runs before the first
// edit, so a retiming that cannot be rebuilt leaves the netlist untouched.
void Retimer::applyRetiming(const RetimingGraph& graph,
                            const std::vector<int>& r)
{
  const std::vector<Group> groups = collectGroups(graph, r);
  const bool any_changed
      = std::ranges::any_of(groups, [](const Group& g) { return g.changed; });
  if (!any_changed) {
    return;  // nothing moved; the search already said so
  }

  const FaninIndex fanin = faninIndex(graph);
  std::vector<std::vector<const RegisterClass*>> classes(groups.size());
  for (size_t i = 0; i < groups.size(); i++) {
    if (groups[i].changed && !groups[i].dont_touch) {
      classes[i] = chainClasses(graph, fanin, r, groups[i]);
    }
  }
  checkMoves(graph, fanin, r);

  // Clock net of the registers on each gate's fanin edges, for a chain that
  // appears on a gate whose own registers all moved away.
  std::unordered_map<odb::dbInst*, odb::dbNet*> gate_clock;
  for (const Group& group : groups) {
    for (const Sink& s : group.sinks) {
      const std::vector<odb::dbInst*>& flops = graph.phys_[s.u][s.k].flops;
      if (!flops.empty()) {
        gate_clock.emplace(graph.nodes_[graph.adj_[s.u][s.k].target],
                           registerClockNet(flops.front()));
      }
    }
  }

  ApplyStats stats;
  for (size_t i = 0; i < groups.size(); i++) {
    const Group& group = groups[i];
    if (group.changed && group.dont_touch) {
      stats.groups_skipped++;  // the search never moves these; defensive
    } else if (group.changed) {
      stats.groups_changed++;
      // The chain keeps the clock net of the registers it replaces; a chain
      // on a gate whose registers all moved on takes the clock of the
      // registers that fed that gate.
      odb::dbNet* clock_net = nullptr;
      for (odb::dbInst* f : group.flops) {
        if (clock_net == nullptr) {
          clock_net = registerClockNet(f);
        }
      }
      if (clock_net == nullptr) {
        auto it = gate_clock.find(group.driver->getInst());
        clock_net = it != gate_clock.end() ? it->second : clock_net_;
      }
      rebuildGroup(group, classes[i], clock_net, stats);
    }
  }

  logger_->info(utl::RET,
                24,
                "Rebuilt {} of {} register groups: {} registers removed, {} "
                "inserted, {} of them keeping their class.",
                stats.groups_changed,
                groups.size(),
                stats.removed,
                stats.inserted,
                stats.inserted_classed);
  if (stats.groups_skipped > 0) {
    logger_->warn(utl::RET,
                  27,
                  "Left {} changed register groups untouched because they hold "
                  "a don't-touch register.",
                  stats.groups_skipped);
  }
}

}  // namespace ret
