// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "ret/Retimer.h"

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "Retime.h"
#include "RetimingGraph.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ret/SequentialUnmapper.h"
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

}  // namespace

Retimer::Retimer(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db),
      sta_(sta),
      logger_(logger),
      unmapper_(std::make_unique<SequentialUnmapper>(db, sta, logger))
{
}

Retimer::~Retimer() = default;

odb::dbBlock* Retimer::getBlock()
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::RET, 11, "No design loaded.");
  }
  return block;
}

// The net feeding a register's data input, from the class the graph cached.
static odb::dbNet* registerDataNet(const RetimingGraph& graph,
                                   sta::dbSta* sta,
                                   odb::dbInst* inst)
{
  const RegisterClass& cls = classOf(graph, inst);
  odb::dbITerm* d
      = cls.data ? inst->getITerm(cls.data) : registerDataPin(sta, inst);
  return d ? d->getNet() : nullptr;
}

void Retimer::retime(int max_moves)
{
  odb::dbBlock* block = getBlock();
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

  Retime search(&graph, sta_, logger_, dff_master_);
  search.run(max_moves);
  const std::vector<int> r = search.getRetiming();

  const std::unordered_set<odb::dbInst*> movers = moversToUnmap(graph, r);
  if (movers.empty()) {
    applyRetiming(graph, r);
    return;
  }
  std::map<uint, odb::dbInst*> ordered_movers;
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
  for (int i = 0; i < static_cast<int>(graph.nodes_.size()); i++) {
    lag[graph.nodes_[i]] = r[i];
  }
  unmapper_->run(&movers);

  RetimingGraph graph2(db_, sta_, logger_);
  graph2.build();
  bool ok = true;
  const std::vector<int> r2 = remapLag(graph2, lag, ok);
  if (ok && moversToUnmap(graph2, r2).empty()) {
    applyRetiming(graph2, r2);
    return;
  }

  logger_->info(utl::RET,
                28,
                "Lag remap is ambiguous after unmapping; re-solving on the new "
                "netlist.");
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
    if (!inst->getMaster()->isSequential()) {
      continue;
    }
    odb::dbNet* clock = registerClockNet(inst);
    if (clock != nullptr) {
      return clock;
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
    for (int k = 0; k < static_cast<int>(graph.adj_[u].size()); k++) {
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
    out.push_back(index >= 0 && index < static_cast<int>(flops.size())
                      ? flops[index]
                      : nullptr);
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

struct RegisterGroup
{
  std::unordered_set<odb::dbInst*> flops;
  std::vector<const std::vector<odb::dbInst*>*> chains;  // original edges
  bool weight_changed = false;
  bool has_dont_touch = false;
};

// Edges are grouped by the net driving their register chain (the first
// register's D net, or the driver pin's net when the edge has none); a group
// moves when any of its edges changed weight.
std::unordered_set<odb::dbInst*> Retimer::moversToUnmap(
    const RetimingGraph& graph,
    const std::vector<int>& r) const
{
  std::unordered_map<odb::dbNet*, RegisterGroup> groups;
  const int n = static_cast<int>(graph.nodes_.size());
  for (int u = 0; u < n; u++) {
    const std::vector<Edge>& edges = graph.adj_[u];
    const std::vector<EdgePhys>& handles = graph.phys_[u];
    for (int k = 0; k < static_cast<int>(edges.size()); k++) {
      const Edge& e = edges[k];
      const EdgePhys& ph = handles[k];
      odb::dbNet* group_net
          = ph.flops.empty() ? ph.driver->getNet()
                             : registerDataNet(graph, sta_, ph.flops.front());
      if (group_net == nullptr) {
        continue;
      }
      RegisterGroup& group = groups[group_net];
      if (e.weight + r[e.target] - r[u] != e.weight) {
        group.weight_changed = true;
      }
      if (!ph.flops.empty()) {
        group.chains.push_back(&ph.flops);
      }
      for (odb::dbInst* f : ph.flops) {
        group.flops.insert(f);
        if (isDontTouchRegister(sta_, f)) {
          group.has_dont_touch = true;
        }
      }
    }
  }

  std::unordered_set<odb::dbInst*> movers;
  // Registers that would otherwise keep their class become plain.
  auto unmap_classed = [&](const std::vector<odb::dbInst*>& regs) {
    for (odb::dbInst* reg : regs) {
      if (reg != nullptr && keepsClass(classOf(graph, reg))) {
        movers.insert(reg);
      }
    }
  };

  for (const auto& [group_net, group] : groups) {
    if (!group.weight_changed || group.has_dont_touch) {
      continue;
    }
    // A rebuilt chain is the base cell for a plain register and the original
    // cell for an enable or inverting register; anything else has to become
    // plain first.
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

  // A move across gate v takes one register from every fanin edge; those
  // registers merge into one and so must be of one class.  Inverting
  // registers can also only cross a self-dual gate.  Either conflict is
  // resolved by unmapping the classed registers involved, which makes them
  // plain.
  const FaninIndex fanin = faninIndex(graph);
  for (int v = 0; v < n; v++) {
    for (int j = 1; j <= -r[v]; j++) {
      std::vector<odb::dbInst*> regs;
      originRegisters(graph, fanin, r, v, j, regs);
      if (std::find(regs.begin(), regs.end(), nullptr) != regs.end()) {
        continue;  // applyRetiming reports the inconsistency
      }
      if (!moveKeepsClass(graph, regs, v)) {
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
      if (pin->getIoType() != odb::dbIoType::INPUT
          || pin->getSigType() == odb::dbSigType::CLOCK) {
        continue;
      }
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
  odb::dbITerm* d = cls.data ? inst->getITerm(cls.data) : nullptr;
  odb::dbNet* d_net = d ? d->getNet() : nullptr;
  if (cls.kind == RegisterKind::other || d_net == nullptr) {
    logger_->error(utl::RET,
                   39,
                   "Register {} cannot be bypassed; the retiming is only "
                   "partly applied.",
                   inst->getName());
  }
  odb::dbITerm* q = cls.output ? inst->getITerm(cls.output) : nullptr;
  odb::dbNet* q_net = q ? q->getNet() : nullptr;
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
// output pin feeding on.  New registers start at `location` (the spot of the
// registers they replace) so detailed placement can legalize them; detailed
// placement skips unplaced instances.
std::vector<odb::dbNet*> Retimer::buildChain(
    odb::dbBlock* block,
    odb::dbNet* driver_net,
    odb::dbNet* clock_net,
    const std::vector<const RegisterClass*>& classes,
    const odb::Point* location)
{
  const int depth = static_cast<int>(classes.size());
  std::vector<odb::dbNet*> nets(depth + 1);
  nets[0] = driver_net;
  odb::dbNet* cur = driver_net;
  for (int k = 1; k <= depth; k++) {
    const RegisterClass* cls = classes[k - 1];
    odb::dbMaster* master = cls ? cls->master : dff_master_;
    // The design may already hold names from an earlier retime run.
    std::string ff_name;
    std::string net_name;
    do {
      ff_name = "ret_ff_" + std::to_string(uniquify_);
      net_name = "ret_net_" + std::to_string(uniquify_);
      uniquify_++;
    } while (block->findInst(ff_name.c_str()) != nullptr
             || block->findNet(net_name.c_str()) != nullptr);
    odb::dbInst* ff = odb::dbInst::create(block, master, ff_name.c_str());
    odb::dbNet* out = odb::dbNet::create(block, net_name.c_str());
    if (location != nullptr) {
      ff->setLocation(location->x(), location->y());
      ff->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    for (odb::dbITerm* iterm : ff->getITerms()) {
      if (iterm->getSigType() == odb::dbSigType::CLOCK) {
        iterm->connect(clock_net);
      } else if (cls == nullptr) {
        if (iterm->getIoType() == odb::dbIoType::INPUT) {
          iterm->connect(cur);
        } else if (iterm->getMTerm() == dff_q_mterm_) {
          iterm->connect(out);
        }
      } else if (iterm->getMTerm() == cls->data) {
        iterm->connect(cur);
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
    cur = out;
    nets[k] = cur;
  }
  return nets;
}

// Minimal-perturbation apply: rebuild only the register groups whose edge
// weights changed.  Within a group the old registers are removed, one shared
// chain of the maximum new depth is built on the group's driver net, and each
// sink is reconnected at its own depth.
void Retimer::applyRetiming(RetimingGraph& graph, const std::vector<int>& r)
{
  odb::dbBlock* block = getBlock();

  struct SinkWr
  {
    odb::dbITerm* sink;
    int wr;
    int u;  // the edge (u, k) this sink ends
    int k;
  };
  // Keyed by net id so the rebuild order, and with it the names of the new
  // instances, is the same on every run.
  std::map<uint, odb::dbNet*> group_nets;
  std::unordered_map<odb::dbNet*, std::vector<SinkWr>> groups;
  std::unordered_map<odb::dbNet*, odb::dbITerm*> group_driver;
  // Clock net of the registers on each gate's fanin edges, for chains that
  // appear on a gate whose own registers all moved away.
  std::unordered_map<odb::dbInst*, odb::dbNet*> gate_clock;
  std::unordered_map<odb::dbNet*, std::unordered_set<odb::dbInst*>> group_flops;
  std::unordered_set<odb::dbNet*> changed;
  std::unordered_set<odb::dbNet*> dont_touch;

  const int n = static_cast<int>(graph.nodes_.size());
  for (int u = 0; u < n; u++) {
    const std::vector<Edge>& edges = graph.adj_[u];
    const std::vector<EdgePhys>& handles = graph.phys_[u];
    for (int k = 0; k < static_cast<int>(edges.size()); k++) {
      odb::dbNet* driver_net
          = handles[k].flops.empty()
                ? handles[k].driver->getNet()
                : registerDataNet(graph, sta_, handles[k].flops.front());
      if (driver_net == nullptr) {
        continue;
      }
      const int w = edges[k].weight;
      const int wr = w + r[edges[k].target] - r[u];
      group_nets[driver_net->getId()] = driver_net;
      groups[driver_net].push_back({handles[k].sink, wr, u, k});
      group_driver[driver_net] = handles[k].driver;
      if (!handles[k].flops.empty()) {
        gate_clock.emplace(graph.nodes_[edges[k].target],
                           registerClockNet(handles[k].flops.front()));
      }
      for (odb::dbInst* f : handles[k].flops) {
        group_flops[driver_net].insert(f);
        if (isDontTouchRegister(sta_, f)) {
          dont_touch.insert(driver_net);
        }
      }
      if (wr != w) {
        changed.insert(driver_net);
      }
    }
  }

  if (changed.empty()) {
    return;  // nothing moved; the search already said so
  }

  // Class of every chain position of every changed group, from the
  // registers each sink's edge stands for, before anything is modified.
  // Positions count from the driver; a sink at depth wr sees position wr as
  // its first register upstream.  A position must be all plain or all one
  // enable class, else the retiming cannot be rebuilt and nothing is applied.
  const FaninIndex fanin = faninIndex(graph);
  std::unordered_map<odb::dbNet*, std::vector<const RegisterClass*>>
      chain_classes;
  for (const auto& [net_id, driver_net] : group_nets) {
    if (changed.count(driver_net) == 0 || dont_touch.count(driver_net) != 0) {
      continue;
    }
    const std::vector<SinkWr>& sinks = groups[driver_net];
    // Every register of the group is bypassed, so each must be a kind whose
    // data and output pins are known.
    for (odb::dbInst* f : group_flops[driver_net]) {
      const RegisterClass& cls = classOf(graph, f);
      if (cls.kind == RegisterKind::other || cls.data == nullptr) {
        logger_->error(utl::RET,
                       40,
                       "Register {} on net {} cannot be rebuilt; the retiming "
                       "is not applied.",
                       f->getName(),
                       driver_net->getName());
      }
    }
    int max_depth = 0;
    for (const SinkWr& s : sinks) {
      max_depth = std::max(max_depth, s.wr);
    }
    std::vector<const RegisterClass*> classes(max_depth, nullptr);
    std::vector<bool> plain_at(max_depth, false);
    for (const SinkWr& s : sinks) {
      // The sink's gate took the first -r registers upstream of it across
      // itself; the wr registers left on the edge are the ones behind those.
      const int taken = -r[graph.adj_[s.u][s.k].target];
      for (int p = 1; p <= s.wr; p++) {
        std::vector<odb::dbInst*> regs;
        originRegistersOnEdge(
            graph, fanin, r, s.u, s.k, taken + s.wr - p + 1, regs);
        for (odb::dbInst* reg : regs) {
          const RegisterClass* cls = reg ? &classOf(graph, reg) : nullptr;
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
                           driver_net->getName());
          }
        }
      }
    }
    chain_classes[driver_net] = std::move(classes);
  }
  // Every move must merge registers of one class, across a gate that class
  // may cross; moversToUnmap leaves nothing else behind.
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

  int removed = 0;
  int inserted = 0;
  int inserted_enable = 0;
  int reregistered_ports = 0;
  int groups_changed = 0;
  int skipped_groups = 0;
  for (const auto& [net_id, driver_net] : group_nets) {
    const std::vector<SinkWr>& sinks = groups[driver_net];
    if (changed.count(driver_net) == 0) {
      continue;  // every sink unchanged: this group's registers stay put
    }
    if (dont_touch.count(driver_net) != 0) {
      skipped_groups++;  // the search never moves these; defensive
      continue;
    }
    groups_changed++;

    // The chain keeps the clock net of the registers it replaces; a chain on
    // a gate whose registers all moved on takes the clock of the registers
    // that fed that gate.
    odb::dbNet* chain_clock = nullptr;
    for (odb::dbInst* f : group_flops[driver_net]) {
      chain_clock = registerClockNet(f);
      if (chain_clock != nullptr) {
        break;
      }
    }
    if (chain_clock == nullptr) {
      auto it = gate_clock.find(group_driver[driver_net]->getInst());
      chain_clock = it != gate_clock.end() ? it->second : clock_net_;
    }

    // Start location for the rebuilt chain: centroid of the registers being
    // replaced, else the driving instance.
    odb::Point location;
    bool have_location = false;
    {
      long long sx = 0;
      long long sy = 0;
      int n_placed = 0;
      for (odb::dbInst* f : group_flops[driver_net]) {
        if (f->isPlaced()) {
          const odb::Point pt = f->getLocation();
          sx += pt.x();
          sy += pt.y();
          n_placed++;
        }
      }
      if (n_placed > 0) {
        location = odb::Point(static_cast<int>(sx / n_placed),
                              static_cast<int>(sy / n_placed));
        have_location = true;
      } else {
        odb::dbInst* driver = group_driver[driver_net]->getInst();
        if (driver->isPlaced()) {
          location = driver->getLocation();
          have_location = true;
        }
      }
    }

    const std::vector<const RegisterClass*>& classes
        = chain_classes[driver_net];
    const int max_depth = static_cast<int>(classes.size());

    for (odb::dbInst* f : group_flops[driver_net]) {
      removeRegister(f);
      removed++;
    }
    if (max_depth == 0) {
      continue;  // purely combinational now
    }
    const std::vector<odb::dbNet*> depth_net
        = buildChain(block,
                     driver_net,
                     chain_clock,
                     classes,
                     have_location ? &location : nullptr);
    inserted += max_depth;
    for (const RegisterClass* cls : classes) {
      if (cls != nullptr) {
        inserted_enable++;
      }
    }
    for (const SinkWr& s : sinks) {
      if (s.wr > 0) {
        s.sink->disconnect();
        s.sink->connect(depth_net[s.wr]);
      }
      // wr == 0: the sink already sits on driver_net after removeRegister
    }
    // A primary output that collapsed onto driver_net reads the deepest tap.
    const std::vector<odb::dbBTerm*> ports(driver_net->getBTerms().begin(),
                                           driver_net->getBTerms().end());
    for (odb::dbBTerm* bterm : ports) {
      bterm->disconnect();
      bterm->connect(depth_net[max_depth]);
      reregistered_ports++;
    }
  }

  logger_->info(utl::RET,
                24,
                "Removed {} registers ({} of {} register groups changed).",
                removed,
                groups_changed,
                groups.size());
  logger_->info(utl::RET,
                25,
                "Inserted {} registers ({} kept their register class).",
                inserted,
                inserted_enable);
  if (reregistered_ports > 0) {
    logger_->info(
        utl::RET, 26, "Re-registered {} output ports.", reregistered_ports);
  }
  if (skipped_groups > 0) {
    logger_->warn(utl::RET,
                  27,
                  "Left {} changed register groups untouched because they hold "
                  "a don't-touch register.",
                  skipped_groups);
  }
}

}  // namespace ret
