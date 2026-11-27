// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "cgt/ClockGating.h"

// NOLINTNEXTLINE(modernize-deprecated-headers,misc-include-cleaner): strdup()
#include <string.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ClockGatingImpl.h"
#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "cut/abc_init.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "map/mio/mio.h"
#include "misc/util/abc_global.h"
#include "misc/vec/vecInt.h"
#include "misc/vec/vecPtr.h"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/ClkNetwork.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

using utl::CGT;
using utl::DebugScopedTimer;
using utl::UniquePtrWithDeleter;

// Headers have duplicate declarations so we include
// a forward one to get at this function without angering
// gcc.
namespace abc {
// NOLINTBEGIN(readability-identifier-naming)
extern Abc_Ntk_t* Abc_NtkMulti(Abc_Ntk_t* pNtk,
                               int nThresh,
                               int nFaninMax,
                               int fCnf,
                               int fMulti,
                               int fSimple,
                               int fFactor);
extern void Abc_FrameSetLibGen(void* pLib);
// NOLINTEND(readability-identifier-naming)
}  // namespace abc

namespace cgt {

ClockGating::ClockGating(utl::Logger* logger, sta::dbSta* open_sta)
    : impl_(std::make_unique<Impl>(logger, open_sta))
{
}

ClockGating::~ClockGating() = default;

void ClockGating::run(sta::Scene* scene)
{
  impl_->run(scene);
}

void ClockGating::setMinInstances(int min_instances)
{
  impl_->setMinInstances(min_instances);
}

void ClockGating::setMaxCover(int max_cover)
{
  impl_->setMaxCover(max_cover);
}

void ClockGating::setDumpDir(const char* dir)
{
}

//////////////////////////////////////////////////

ClockGating::Impl::Impl(utl::Logger* const logger, sta::dbSta* const sta)
    : logger_(logger),
      sta_(sta),
      abc_factory_(std::make_unique<cut::AbcLibraryFactory>(logger_))
{
}

ClockGating::Impl::~Impl() = default;

// Dumps the given network as GraphViz.
static void dumpGraphviz(sta::dbNetwork* const network,
                         sta::Instance* const inst,
                         std::ostream& out)
{
  if (inst == network->topInstance()) {
    out << "digraph " << network->name(inst) << " {\n";
    out << "{\nrank=min;\n";
    auto pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyInput()) {
        out << '"' << network->name(pin)
            << "\"[shape=invtriangle, color=coral];\n";
      }
    }
    delete pin_iter;
    out << "}\n{\nrank=max;\n";
    pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyOutput()) {
        out << '"' << network->name(pin)
            << "\"[shape=triangle, color=coral];\n";
      }
    }
    delete pin_iter;
    out << "}\n";
  } else if (auto cell = network->cell(inst)) {
    out << "subgraph \"" << network->name(inst) << "\" {\n";
    out << "cluster=true;\n";
    out << "label=\"" << network->name(cell) << '\n'
        << network->name(inst) << "\";\n";
    auto pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      out << '"' << network->name(pin) << "\";\n";
    }
    delete pin_iter;
  }
  auto pin_iter = network->pinIterator(inst);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    out << '"' << network->name(pin) << "\" [label=\"" << network->portName(pin)
        << "\"];\n";
  }
  delete pin_iter;
  auto net_iter = network->netIterator(inst);
  while (net_iter->hasNext()) {
    auto net = net_iter->next();
    out << '"' << network->name(net) << "\";\n";
    sta::NetPinIterator* pin_iter = network->pinIterator(net);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyInput()) {
        out << '"' << network->name(net) << "\" -> \"" << network->name(pin)
            << "\";\n";
      }
      if (network->direction(pin)->isAnyOutput()) {
        out << '"' << network->name(pin) << "\" -> \"" << network->name(net)
            << "\";\n";
      }
    }
  }
  delete net_iter;
  auto child_iter = network->childIterator(inst);
  while (child_iter->hasNext()) {
    auto child = child_iter->next();
    dumpGraphviz(network, child, out);
  }
  delete child_iter;
  out << "}\n";
}

// Returns the output pin of a register cell, if it exists.
static sta::Pin* getRegOutPin(sta::dbNetwork* const network,
                              sta::Instance* const inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto& seq : cell->sequentials()) {
    if (seq.isRegister() && seq.output()) {
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        auto port = port_iter.next();
        if (port->direction()->isAnyOutput()) {
          assert(port->function()->op() == sta::FuncExpr::Op::port);
          assert(port->function()->port() == seq.output());
          return network->findPin(inst, port);
        }
      }
    }
  }
  return nullptr;
}

// Returns the data function of a register cell, if it exists.
static sta::FuncExpr* getRegDataFunction(sta::dbNetwork* const network,
                                         sta::Instance* const inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto& seq : cell->sequentials()) {
    if (seq.isRegister() && seq.data()) {
      return seq.data();
    }
  }
  return nullptr;
}

// Returns the clock pin of a register cell, if it exists.
static sta::Pin* getClockPin(sta::dbNetwork* const network,
                             sta::Instance* const inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto& seq : cell->sequentials()) {
    if (seq.isRegister() && seq.clock()) {
      auto clock_expr = seq.clock();
      assert(clock_expr->op() == sta::FuncExpr::Op::port);
      return network->findPin(inst, clock_expr->port());
    }
  }
  return nullptr;
}

void ClockGating::Impl::setDumpDir(const char* const dir)
{
  if (*dir == '\0') {
    return;
  }
  dump_dir_ = std::filesystem::current_path() / dir;
  if (std::filesystem::exists(*dump_dir_)) {
    std::filesystem::remove_all(*dump_dir_);
  }
  std::filesystem::create_directory(*dump_dir_);
}

// Gathers networks downstream of the given instances (until it encounters
// registers).
static std::vector<sta::Net*> downstreamNets(sta::dbSta* const sta,
                                             sta::Instance* const instance)
{
  class SearchPred final : public sta::SearchPred1
  {
   public:
    explicit SearchPred(sta::dbSta* const sta) : sta::SearchPred1(sta) {}
    bool searchFrom(const sta::Vertex* const from_vertex,
                    const sta::Mode* mode) const final
    {
      return true;
    }
    bool searchTo(const sta::Vertex* const to_vertex,
                  const sta::Mode* mode) const final
    {
      return !visited.contains(to_vertex);
    }
    bool searchThru(sta::Edge* edge, const sta::Mode* mode) const override
    {
      const sta::TimingRole* role = edge->role();
      return sta::SearchPred1::searchThru(edge, mode)
             && role->genericRole() != sta::TimingRole::regClkToQ()
             && role->genericRole() != sta::TimingRole::latchDtoQ();
    }

    std::unordered_set<const sta::Vertex*> visited;
  };

  auto network = sta->getDbNetwork();
  auto graph = sta->graph();
  SearchPred pred(sta);
  std::vector<sta::Net*> nets;
  std::unordered_set<sta::Net*> visited_nets;
  {
    sta::BfsFwdIterator iter(sta::BfsIndex::other, &pred, sta);
    auto pin_iter = network->pinIterator(instance);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyOutput()) {
        iter.enqueue(graph->pinLoadVertex(pin));
      }
    }
    delete pin_iter;
    while (iter.hasNext()) {
      auto vertex = iter.next();
      auto net = network->net(vertex->pin());
      if (net && visited_nets.insert(net).second) {
        nets.push_back(net);
      }
      pred.visited.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return nets;
}

// Appends the nets upstream of the given nets to them, and returns the result.
static std::vector<sta::Net*> upstreamNets(sta::dbSta* const sta,
                                           std::vector<sta::Net*>& nets)
{
  class SearchPred final : public sta::SearchPred1
  {
   public:
    explicit SearchPred(sta::dbSta* const sta) : sta::SearchPred1(sta) {}
    bool searchFrom(const sta::Vertex* const from_vertex,
                    const sta::Mode* mode) const final
    {
      return !visited.contains(from_vertex);
    }
    bool searchTo(const sta::Vertex* const to_vertex,
                  const sta::Mode* mode) const final
    {
      return true;
    }
    bool searchThru(sta::Edge* edge, const sta::Mode* mode) const override
    {
      const sta::TimingRole* role = edge->role();
      return sta::SearchPred1::searchThru(edge, mode)
             && role->genericRole() != sta::TimingRole::regClkToQ()
             && role->genericRole() != sta::TimingRole::latchDtoQ();
    }

    std::unordered_set<const sta::Vertex*> visited;
  };

  auto network = sta->getDbNetwork();
  auto graph = sta->graph();
  SearchPred pred(sta);
  std::unordered_set<sta::Net*> visited_nets;
  {
    sta::BfsBkwdIterator iter(sta::BfsIndex::other, &pred, sta);
    for (auto& inst : nets) {
      auto pin_iter = network->pinIterator(inst);
      while (pin_iter->hasNext()) {
        auto pin = pin_iter->next();
        if (network->direction(pin)->isAnyInput()) {
          iter.enqueue(graph->pinLoadVertex(pin));
        }
      }
      delete pin_iter;
    }
    while (iter.hasNext()) {
      auto vertex = iter.next();
      auto net = network->net(vertex->pin());
      if (net && visited_nets.insert(net).second) {
        nets.push_back(net);
      }
      pred.visited.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return nets;
}

// Creates a string that combines names of the given gating condition.
static std::string combinedGatingCondNames(
    sta::dbNetwork* const network,
    const std::vector<sta::Net*>::const_iterator begin,
    const std::vector<sta::Net*>::const_iterator end,
    const bool clk_enable)
{
  std::string result;
  for (auto it = begin; it != end; ++it) {
    if (it != begin) {
      result += clk_enable ? " | " : " & ";
    }
    result += network->name(*it);
  }
  return result;
}

// Creates a string that combines names of the given instances.
static std::string combinedInstanceNames(
    sta::dbNetwork* const network,
    const std::vector<sta::Instance*>::const_iterator begin,
    const std::vector<sta::Instance*>::const_iterator end)
{
  std::string result;
  for (auto it = begin; it != end; ++it) {
    if (it != begin) {
      result += ", ";
    }
    result += network->name(*it);
  }
  return result;
}

std::vector<int> AcceptedGates::gatesUsing(
    const std::vector<sta::Net*>& nets) const
{
  std::vector<int> result;
  for (sta::Net* net : nets) {
    auto it = net_to_gates_.find(net);
    if (it != net_to_gates_.end()) {
      result.insert(result.end(), it->second.begin(), it->second.end());
    }
  }
  std::ranges::sort(result);
  const auto [first, last] = std::ranges::unique(result);
  result.erase(first, last);
  return result;
}

std::optional<int> AcceptedGates::findSubset(
    const std::vector<sta::Net*>& conds,
    const bool clk_enable) const
{
  for (sta::Net* net : conds) {
    auto it = net_to_gates_.find(net);
    if (it == net_to_gates_.end()) {
      continue;
    }
    for (const int idx : it->second) {
      const Gate& gate = gates_[idx];
      if (gate.clk_enable == clk_enable
          && std::ranges::includes(conds, gate.conds)) {
        return idx;
      }
    }
  }
  return std::nullopt;
}

void AcceptedGates::extend(const int idx,
                           const std::vector<sta::Net*>& conds,
                           sta::Instance* const instance)
{
  Gate& gate = gates_[idx];
  gate.conds = conds;
  for (sta::Net* net : conds) {
    index(net, idx);
  }
  gate.insts.push_back(instance);
}

void AcceptedGates::add(const std::vector<sta::Net*>& conds,
                        sta::Instance* const instance,
                        const bool clk_enable)
{
  const int idx = gates_.size();
  gates_.push_back(
      {.conds = conds, .insts = {instance}, .clk_enable = clk_enable});
  for (sta::Net* net : conds) {
    index(net, idx);
  }
}

void AcceptedGates::index(sta::Net* const net, const int idx)
{
  std::vector<int>& idxs = net_to_gates_[net];
  auto it = std::ranges::lower_bound(idxs, idx);
  if (it == idxs.end() || *it != idx) {
    idxs.insert(it, idx);
  }
}

void ClockGating::Impl::run(sta::Scene* const scene)
{
  debug(1, "Starting clock gating with parameters:");
  debug(1, "Minimum instances per clock gate: {}", min_instances_);
  debug(1, "Maximum gate condition cover size: {}", max_cover_);

  auto network = sta_->getDbNetwork();
  network_builder_.init(logger_, network);

  sta_->ensureGraph();
  sta_->ensureLevelized();
  sta_->searchPreamble();

  // Initialize ABC
  cut::abcInit();
  abc_factory_->AddDbSta(sta_);
  abc_factory_->SetCorner(scene);
  abc_library_ = std::make_unique<cut::AbcLibrary>(abc_factory_->Build());

  dump("pre");

  const std::vector<sta::Instance*> instances = collectRegisters(network);
  AcceptedGates accepted;
  for (int i = 0; i < instances.size(); i++) {
    auto instance = instances[i];
    if (i % 100 == 0) {
      logger_->info(CGT, 3, "Clock gating instance {}/{}", i, instances.size());
    }

    auto downstream = downstreamNets(sta_, instance);
    auto gate_cond_cover = downstream;
    if (max_cover_ < gate_cond_cover.size()) {
      gate_cond_cover.resize(max_cover_);
    }

    if (gate_cond_cover.empty()) {
      logger_->warn(CGT,
                    2,
                    "No gating condition candidates found for instance {}",
                    network->name(instance));
      continue;
    }

    debug(1,
          "Gate condition cover size: {} for instance {}",
          gate_cond_cover.size(),
          network->name(instance));

    auto abc_network = exportToAbc(instance, downstream);
    if (reuseAcceptedGate(instance, gate_cond_cover, *abc_network, accepted)) {
      continue;
    }

    bool clk_enable = false;
    const std::vector<sta::Net*> gate_cond = findGateCondition(
        instance, std::move(gate_cond_cover), *abc_network, clk_enable);
    if (gate_cond.empty()) {
      continue;
    }

    logger_->info(CGT,
                  4,
                  "Accepted clock {} '{}' for instance {}",
                  clk_enable ? "enable" : "disable",
                  combinedGatingCondNames(
                      network, gate_cond.begin(), gate_cond.end(), clk_enable),
                  network->name(instance));

    if (!extendAcceptedGate(instance, gate_cond, clk_enable, accepted)) {
      accepted.add(gate_cond, instance, clk_enable);
    }
  }

  for (const auto& [gate_cond, gated, clk_enable] : accepted.gates()) {
    if (gated.size() >= min_instances_) {
      insertClockGate(gated, gate_cond, clk_enable);
    } else {
      debugPrint(logger_,
                 CGT,
                 "clock_gating",
                 1,
                 "Not enough instances ({}) for clock gate: {}",
                 gated.size(),
                 combinedInstanceNames(network, gated.begin(), gated.end()));
    }
  }

  dump("post");
  reportStats();
}

std::vector<sta::Instance*> ClockGating::Impl::collectRegisters(
    sta::dbNetwork* const network)
{
  std::vector<sta::Instance*> instances;
  auto inst_iter = network->childIterator(network->topInstance());
  while (inst_iter->hasNext()) {
    auto instance = inst_iter->next();
    auto cell = network->libertyCell(instance);
    if (!cell) {
      continue;
    }
    const int num_regs = std::ranges::count_if(
        cell->sequentials(), [](const auto& seq) { return seq.isRegister(); });
    if (num_regs == 1) {
      instances.emplace_back(instance);
    } else if (num_regs > 1) {
      logger_->warn(
          CGT, 10, "Skipping multi-bit instance {}.", network->name(instance));
    }
  }
  delete inst_iter;
  return instances;
}

bool ClockGating::Impl::reuseAcceptedGate(
    sta::Instance* const instance,
    std::vector<sta::Net*>& gate_cond_cover,
    abc::Abc_Ntk_t& abc_network,
    AcceptedGates& accepted)
{
  DebugScopedTimer timer(reuse_check_time_,
                         logger_,
                         CGT,
                         "clock_gating",
                         3,
                         "Existing gating conditions reuse check time: {}");
  auto network = sta_->getDbNetwork();
  const std::vector<int> candidates
      = accepted.gatesUsing(upstreamNets(sta_, gate_cond_cover));
  bool reused = false;
  for (const int idx : candidates) {
    AcceptedGates::Gate& gate = accepted.gate(idx);
    if (isCorrectClockGate(
            instance, gate.conds, abc_network, gate.clk_enable)) {
      debugPrint(
          logger_,
          CGT,
          "clock_gating",
          1,
          "Found existing clock gate condition: {} for instance {}",
          combinedGatingCondNames(
              network, gate.conds.begin(), gate.conds.end(), gate.clk_enable),
          network->name(instance));
      gate.insts.push_back(instance);
      reused = true;
      break;
    }
  }
  debug(1,
        "Checked {} existing clock gate conditions for instance {}",
        candidates.size(),
        network->name(instance));
  return reused;
}

std::vector<sta::Net*> ClockGating::Impl::findGateCondition(
    sta::Instance* const instance,
    std::vector<sta::Net*> gate_cond_cover,
    abc::Abc_Ntk_t& abc_network,
    bool& clk_enable)
{
  DebugScopedTimer timer(search_time_,
                         logger_,
                         CGT,
                         "clock_gating",
                         3,
                         "Clock gate condition search time: {}");
  if (isCorrectClockGate(instance, gate_cond_cover, abc_network, true)) {
    clk_enable = true;
  } else if (isCorrectClockGate(
                 instance, gate_cond_cover, abc_network, false)) {
    clk_enable = false;
  } else {
    debug(1,
          "Clock gating failed for instance {}",
          sta_->getDbNetwork()->name(instance));
    return {};
  }
  searchClockGates(instance,
                   gate_cond_cover,
                   gate_cond_cover.begin(),
                   gate_cond_cover.end(),
                   abc_network,
                   clk_enable);
  return gate_cond_cover;
}

bool ClockGating::Impl::extendAcceptedGate(
    sta::Instance* const instance,
    const std::vector<sta::Net*>& gate_cond,
    const bool clk_enable,
    AcceptedGates& accepted)
{
  DebugScopedTimer timer(exist_check_time_,
                         logger_,
                         CGT,
                         "clock_gating",
                         3,
                         "Check if gate condition already exists time: {}");
  const std::optional<int> idx = accepted.findSubset(gate_cond, clk_enable);
  if (!idx) {
    return false;
  }
  auto network = sta_->getDbNetwork();
  const std::vector<sta::Net*>& old_cond = accepted.gate(*idx).conds;
  logger_->info(CGT,
                5,
                "Extending previously accepted clock {} '{}' to '{}'",
                clk_enable ? "enable" : "disable",
                combinedGatingCondNames(
                    network, old_cond.begin(), old_cond.end(), clk_enable),
                combinedGatingCondNames(
                    network, gate_cond.begin(), gate_cond.end(), clk_enable));
  accepted.extend(*idx, gate_cond, instance);
  return true;
}

void ClockGating::Impl::reportStats() const
{
  debug(1, "Clock gating completed.");
  debug(1, "Accepted: {}", accepted_count_);
  debug(2, "Rejected at simulation: {}", rejected_sim_count_);
  debug(2, "Rejected at SAT: {}", rejected_sat_count_);
  const std::pair<const char*, double> times[] = {
      {"gate condition reuse check", reuse_check_time_},
      {"gate condition search", search_time_},
      {"existing gate condition check", exist_check_time_},
      {"network export", network_export_time_},
      {"network build", network_build_time_},
      {"simulation", sim_time_},
      {"SAT", sat_time_},
  };
  for (const auto& [what, seconds] : times) {
    debug(2, "Total {} time: {} s", what, seconds);
  }
}

// Returns a sequence of nets that excludes the range [begin, end) from the
// given sequence.
static std::vector<sta::Net*> except(
    std::vector<sta::Net*>& gate_cond_nets,
    const std::vector<sta::Net*>::iterator begin,
    const std::vector<sta::Net*>::iterator end)
{
  std::vector<sta::Net*> result;
  result.insert(result.end(), gate_cond_nets.begin(), begin);
  result.insert(result.end(), end, gate_cond_nets.end());
  return result;
}

void ClockGating::Impl::searchClockGates(
    sta::Instance* const instance,
    std::vector<sta::Net*>& good_gate_conds,
    const std::vector<sta::Net*>::iterator begin,
    const std::vector<sta::Net*>::iterator end,
    abc::Abc_Ntk_t& abc_network,
    const bool clk_enable)
{
  int half_len = (end - begin) / 2;
  if (half_len == 0) {
    return;
  }
  auto mid = begin + half_len;
  auto combined_candidate_names
      = combinedGatingCondNames(sta_->getDbNetwork(), mid, end, clk_enable);
  debug(3,
        "Checking if '{}' can be removed from clock {}: '{}' for instance {}",
        combined_candidate_names,
        clk_enable ? "enable" : "disable",
        combined_candidate_names,
        sta_->getDbNetwork()->name(instance));
  if (isCorrectClockGate(instance,
                         except(good_gate_conds, mid, end),
                         abc_network,
                         clk_enable)) {
    debug(3,
          "Clock gating signals: '{}' can be dropped",
          combined_candidate_names);
    good_gate_conds.erase(mid, end);
    mid = begin + half_len;
  } else if (end - mid > 1) {
    debug(3,
          "Clock gating signals: {} cannot be dropped together",
          combined_candidate_names);
    searchClockGates(
        instance, good_gate_conds, mid, end, abc_network, clk_enable);
  } else {
    debug(3,
          "Clock gating signal: {} cannot be dropped",
          combined_candidate_names);
  }
  searchClockGates(
      instance, good_gate_conds, begin, mid, abc_network, clk_enable);
}

// A modified version of Abc_NtkDup that only works on netlists and copies all
// object names.
static abc::Abc_Ntk_t* Abc_NtkDupNetlist(abc::Abc_Ntk_t* const network)
{
  assert(network);
  // start the network
  auto new_network
      = Abc_NtkStartFrom(network, network->ntkType, network->ntkFunc);
  // copy the internal nodes
  {
    abc::Abc_Obj_t* obj;
    int i;
    // duplicate the nets and nodes (CIs/COs/latches already dupped)
    Abc_NtkForEachObj(network, obj, i)
    {
      if (!obj->pCopy) {
        Abc_NtkDupObj(new_network, obj, true);
      }
    }
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj(network, obj, i)
    {
      if (!Abc_ObjIsBox(obj) && !Abc_ObjIsBo(obj)) {
        abc::Abc_Obj_t* fanin;
        int k;
        Abc_ObjForEachFanin(obj, fanin, k)
        {
          Abc_ObjAddFanin(obj->pCopy, fanin->pCopy);
        }
      }
    }
    // move object IDs
    if (network->vOrigNodeIds) {
      new_network->vOrigNodeIds
          = abc::Vec_IntStartFull(Abc_NtkObjNumMax(new_network));
      Abc_NtkForEachObj(network, obj, i)
      {
        if (obj->pCopy && Vec_IntEntry(network->vOrigNodeIds, obj->Id) > 0) {
          Vec_IntWriteEntry(new_network->vOrigNodeIds,
                            obj->pCopy->Id,
                            Vec_IntEntry(network->vOrigNodeIds, obj->Id));
        }
      }
    }
  }
  // duplicate the EXDC Ntk
  if (network->pExdc) {
    new_network->pExdc = Abc_NtkDup(network->pExdc);
  }
  if (network->pExcare) {
    new_network->pExcare
        = Abc_NtkDup(static_cast<abc::Abc_Ntk_t*>(network->pExcare));
  }
  // duplicate timing manager
  if (network->pManTime) {
    Abc_NtkTimeInitialize(new_network, network);
  }
  if (network->vPhases) {
    Abc_NtkTransferPhases(new_network, network);
  }
  if (network->pWLoadUsed) {
    new_network->pWLoadUsed = abc::Abc_UtilStrsav(network->pWLoadUsed);
  }
  // check correctness
  if (!Abc_NtkCheck(new_network)) {
    fprintf(stdout, "Abc_NtkDupNetlist(): Network check has failed.\n");
  }
  network->pCopy = new_network;
  return new_network;
}

// Constructs a unique pointer to an ABC network with a custom deleter.
static utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(
    abc::Abc_Ntk_t* const ntk)
{
  return utl::UniquePtrWithDeleter<abc::Abc_Ntk_t>(ntk, &abc::Abc_NtkDelete);
}

static utl::UniquePtrWithDeleter<abc::Vec_Ptr_t> WrapUnique(
    abc::Vec_Ptr_t* const vec)
{
  return utl::UniquePtrWithDeleter<abc::Vec_Ptr_t>(vec, &abc::Vec_PtrFree);
}

// Adds a net driven by node to abc_network, named name if one is given.
static abc::Abc_Obj_t* addNet(abc::Abc_Ntk_t* const abc_network,
                              abc::Abc_Obj_t* const node,
                              const char* const name = nullptr)
{
  abc::Abc_Obj_t* net = abc::Abc_NtkCreateNet(abc_network);
  if (name) {
    abc::Abc_ObjAssignName(net, const_cast<char*>(name), nullptr);
  }
  abc::Abc_ObjAddFanin(net, node);
  return net;
}

// Returns the only net in nets, or else a new net named name that ORs
// (use_or) or ANDs them.
static abc::Abc_Obj_t* reduceNets(abc::Abc_Ntk_t* const abc_network,
                                  abc::Vec_Ptr_t* const nets,
                                  const bool use_or,
                                  const char* const name)
{
  auto net = static_cast<abc::Abc_Obj_t*>(abc::Vec_PtrEntry(nets, 0));
  if (abc::Vec_PtrSize(nets) > 1) {
    abc::Abc_Obj_t* node = use_or
                               ? abc::Abc_NtkCreateNodeOr(abc_network, nets)
                               : abc::Abc_NtkCreateNodeAnd(abc_network, nets);
    abc::Abc_ObjAssignName(node, const_cast<char*>(name), nullptr);
    net = addNet(abc_network, node, name);
  }
  return net;
}

// Translates the data function of a given instance into an ABC network,
static abc::Abc_Obj_t* regDataFunctionToAbc(sta::dbNetwork* const network,
                                            abc::Abc_Ntk_t* const abc_network,
                                            abc::Vec_Ptr_t* const fanins,
                                            sta::Instance* const inst)
{
  std::unordered_map<sta::LibertyPort*, abc::Abc_Obj_t*> port_to_obj;
  auto pin_iter = network->pinIterator(inst);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    auto net = network->net(pin);
    auto port = network->libertyPort(pin);
    if (port->isClock()) {
      continue;
    }
    auto& obj = port_to_obj[port];
    std::string net_name = network->name(net);
    obj = abc::Abc_NtkFindNet(abc_network, net_name.data());
  }
  delete pin_iter;
  std::vector<sta::FuncExpr*> expr_stack = {getRegDataFunction(network, inst)};
  assert(expr_stack[0]);
  std::vector<sta::FuncExpr*> eval_stack;
  while (!expr_stack.empty()) {
    auto expr = expr_stack.back();
    eval_stack.push_back(expr);
    expr_stack.pop_back();
    switch (expr->op()) {
      case sta::FuncExpr::Op::port:
      case sta::FuncExpr::Op::zero:
      case sta::FuncExpr::Op::one:
        break;
      case sta::FuncExpr::Op::and_:
      case sta::FuncExpr::Op::or_:
      case sta::FuncExpr::Op::xor_:
        expr_stack.push_back(expr->right());
        expr_stack.push_back(expr->left());
        break;
      case sta::FuncExpr::Op::not_:
        expr_stack.push_back(expr->left());
        break;
    }
  }
  std::vector<abc::Abc_Obj_t*> obj_stack;
  while (!eval_stack.empty()) {
    auto expr = eval_stack.back();
    eval_stack.pop_back();
    switch (expr->op()) {
      case sta::FuncExpr::Op::port: {
        auto port = expr->port();
        assert(port->direction()->isAnyInput());
        auto& obj = port_to_obj[port];
        if (!obj) {
          std::string name = network->name(inst);
          name += '/';
          name += port->name();
          obj = addNet(
              abc_network, abc::Abc_NtkCreatePi(abc_network), name.c_str());
        }
        obj_stack.emplace_back(obj);
        break;
      }
      case sta::FuncExpr::Op::zero:
        obj_stack.emplace_back(
            addNet(abc_network, abc::Abc_NtkCreateNodeConst0(abc_network)));
        break;
      case sta::FuncExpr::Op::one:
        obj_stack.emplace_back(
            addNet(abc_network, abc::Abc_NtkCreateNodeConst1(abc_network)));
        break;
      case sta::FuncExpr::Op::not_: {
        auto obj = obj_stack.back();
        obj_stack.pop_back();
        obj_stack.emplace_back(
            addNet(abc_network, abc::Abc_NtkCreateNodeInv(abc_network, obj)));
        break;
      }
      case sta::FuncExpr::Op::and_:
      case sta::FuncExpr::Op::or_:
      case sta::FuncExpr::Op::xor_: {
        abc::Vec_PtrClear(fanins);
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Abc_Obj_t* node;
        if (expr->op() == sta::FuncExpr::Op::and_) {
          node = abc::Abc_NtkCreateNodeAnd(abc_network, fanins);
        } else if (expr->op() == sta::FuncExpr::Op::or_) {
          node = abc::Abc_NtkCreateNodeOr(abc_network, fanins);
        } else {
          node = abc::Abc_NtkCreateNodeExor(abc_network, fanins);
        }
        obj_stack.emplace_back(addNet(abc_network, node));
        break;
      }
    }
  }
  assert(obj_stack.size() == 1);
  return obj_stack.back();
}

// Adds nets to abc_network that are true when instance's next state (its
// data function) differs from its current state, one for the output pin and
// one for the output net, and returns them.
static utl::UniquePtrWithDeleter<abc::Vec_Ptr_t> addStateChangeNets(
    sta::dbNetwork* const network,
    abc::Abc_Ntk_t* const abc_network,
    sta::Instance* const instance)
{
  auto xor_states = WrapUnique(abc::Vec_PtrAlloc(1));
  auto fanins = WrapUnique(abc::Vec_PtrAlloc(2));

  auto prev_state
      = regDataFunctionToAbc(network, abc_network, fanins.get(), instance);
  abc::Vec_PtrClear(fanins.get());
  abc::Vec_PtrPush(fanins.get(), prev_state);
  auto out_pin = getRegOutPin(network, instance);
  assert(out_pin);
  auto out_pin_name = network->name(out_pin);
  std::string out_net_name = network->name(network->net(out_pin));
  for (const auto& output_name : {out_pin_name, out_net_name}) {
    abc::Abc_Obj_t* curr_state = abc::Abc_NtkFindNet(
        abc_network, const_cast<char*>(output_name.c_str()));
    if (!curr_state) {
      curr_state = addNet(
          abc_network, abc::Abc_NtkCreatePi(abc_network), output_name.c_str());
    } else {
      assert(abc::Abc_ObjIsNet(curr_state));
    }
    abc::Vec_PtrPush(fanins.get(), curr_state);
    abc::Abc_Obj_t* xor_state
        = abc::Abc_NtkCreateNodeExor(abc_network, fanins.get());
    abc::Vec_PtrPop(fanins.get());
    abc::Vec_PtrPush(xor_states.get(), addNet(abc_network, xor_state));
  }

  return xor_states;
}

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> ClockGating::Impl::makeTestNetwork(
    sta::Instance* const instance,
    const std::vector<sta::Net*>& gate_cond_nets,
    abc::Abc_Ntk_t& abc_network_ref,
    const bool clk_enable)
{
  DebugScopedTimer timer(network_export_time_,
                         logger_,
                         CGT,
                         "clock_gating",
                         3,
                         "Network build for sim/SAT time: {}");
  auto abc_network = WrapUnique(Abc_NtkDupNetlist(&abc_network_ref));
  abc::Abc_NtkMapToSop(abc_network.get());
  {
    abc::Abc_Obj_t* obj;
    int idx;
    // Reverse, as it's much faster due to how NtkDeleteObj works.
    Abc_NtkForEachObjReverse(abc_network.get(), obj, idx)
    {
      if (Abc_ObjIsCo(obj)) {
        abc::Abc_NtkDeleteObj(obj);
      }
    }
  }

  auto network = sta_->getDbNetwork();
  auto xor_states = addStateChangeNets(network, abc_network.get(), instance);

  std::unordered_set<std::string> gate_cond_names;
  for (auto gate_cond_net : gate_cond_nets) {
    gate_cond_names.insert(network->name(gate_cond_net));
  }

  auto gate_cond_objs = WrapUnique(abc::Vec_PtrAlloc(1));
  abc::Abc_Obj_t* obj;
  int idx;
  Abc_NtkForEachNet(abc_network.get(), obj, idx)
  {
    if (gate_cond_names.contains(abc::Abc_ObjName(obj))) {
      abc::Vec_PtrPush(gate_cond_objs.get(), obj);
      debug(4, "Found gate net: {}", abc::Abc_ObjName(obj));
    }
  }

  assert(abc::Vec_PtrSize(xor_states.get()) > 0);
  abc::Abc_Obj_t* xor_state_net = reduceNets(
      abc_network.get(), xor_states.get(), /*use_or=*/true, "or_xors");

  if (abc::Vec_PtrSize(gate_cond_objs.get()) == 0) {
    debug(4, "No gate condition found");
    return {};
  }
  abc::Abc_Obj_t* gate_cond_net = reduceNets(
      abc_network.get(), gate_cond_objs.get(), clk_enable, "gate_cond");

  if (clk_enable) {
    gate_cond_net
        = addNet(abc_network.get(),
                 abc::Abc_NtkCreateNodeInv(abc_network.get(), gate_cond_net),
                 "not_gate_cond");
  }

  auto and_fanins = WrapUnique(abc::Vec_PtrAlloc(2));
  abc::Vec_PtrPush(and_fanins.get(), xor_state_net);
  abc::Vec_PtrPush(and_fanins.get(), gate_cond_net);
  abc::Abc_Obj_t* and_net
      = addNet(abc_network.get(),
               abc::Abc_NtkCreateNodeAnd(abc_network.get(), and_fanins.get()),
               "and");

  abc::Abc_Obj_t* result = abc::Abc_NtkCreatePo(abc_network.get());
  abc::Abc_ObjAssignName(result, const_cast<char*>("result"), nullptr);
  abc::Abc_ObjAddFanin(result, and_net);

  abc_network = WrapUnique(abc::Abc_NtkToLogic(abc_network.get()));

  return WrapUnique(abc::Abc_NtkStrash(abc_network.get(),
                                       /*fAllNodes=*/false,
                                       /*fCleanup=*/false,
                                       /*fRecord=*/false));
}

bool ClockGating::Impl::simulationTest(abc::Abc_Ntk_t* const abc_network,
                                       const std::string& combined_gate_name)
{
  DebugScopedTimer timer(
      sim_time_, logger_, CGT, "clock_gating", 3, "Simulation time: {}");
  abc::Abc_Obj_t* obj;
  int idx;
  std::vector<int> sim_input;
  for (int i = 0; i < 10; i++) {
    Abc_NtkForEachPi(abc_network, obj, idx)
    {
      sim_input.resize(idx + 1);
      sim_input[idx] = rand_bits_.get();
      debug(5, "Input: {} == {}", abc::Abc_ObjName(obj), sim_input[idx]);
    }

    UniquePtrWithDeleter<int> sim_output(
        abc::Abc_NtkVerifySimulatePattern(abc_network, sim_input.data()),
        &free);
    if (*sim_output) {
      debug(4,
            "Clock gate signal '{}' is not valid (simulation test)",
            combined_gate_name);
      rejected_sim_count_++;
      return false;
    }
  }
  return true;
}

bool ClockGating::Impl::satTest(abc::Abc_Ntk_t* const abc_network,
                                const std::string& combined_gate_name)
{
  DebugScopedTimer timer(
      sat_time_, logger_, CGT, "clock_gating", 3, "SAT time: {}");
  if (!abc::Abc_NtkMiterSat(abc_network, 0, 0, 0, nullptr, nullptr)) {
    // 0 is SAT
    debug(4,
          "Clock gate signal '{}' is not valid (SAT test)",
          combined_gate_name);
    abc::Abc_Obj_t* obj;
    int idx;
    Abc_NtkForEachPi(abc_network, obj, idx)
    {
      debug(5,
            "SAT value: {} == {}",
            abc::Abc_ObjName(obj),
            abc_network->pModel[idx]);
    }
    rejected_sat_count_++;
    return false;
  }
  return true;
}

bool ClockGating::Impl::isCorrectClockGate(
    sta::Instance* const instance,
    const std::vector<sta::Net*>& gate_cond_nets,
    abc::Abc_Ntk_t& abc_network_ref,
    const bool clk_enable)
{
  auto network = sta_->getDbNetwork();
  // Only the level 4 messages use the names, so skip building them otherwise.
  std::string gate_cond_names;
  if (logger_->debugCheck(CGT, "clock_gating", 4)) {
    gate_cond_names = combinedGatingCondNames(
        network, gate_cond_nets.begin(), gate_cond_nets.end(), clk_enable);
  }
  debug(4,
        "Checking if clock {} '{}' is good for instance {}",
        clk_enable ? "enable" : "disable",
        gate_cond_names,
        network->name(instance));

  auto abc_network
      = makeTestNetwork(instance, gate_cond_nets, abc_network_ref, clk_enable);
  if (!abc_network) {
    return false;
  }

  dumpAbc("simsat", abc_network.get());

  // Simulate with random stimuli
  if (!simulationTest(abc_network.get(), gate_cond_names)) {
    return false;
  }

  // SAT prove correctness of selected clock gate
  if (!satTest(abc_network.get(), gate_cond_names)) {
    return false;
  }

  dumpAbc("accepted", abc_network.get());
  accepted_count_++;
  return true;
}

void ClockGating::Impl::insertClockGate(
    const std::vector<sta::Instance*>& instances,
    const std::vector<sta::Net*>& gate_cond_nets,
    const bool clk_enable)
{
  auto network = sta_->getDbNetwork();

  debugPrint(
      logger_,
      CGT,
      "clock_gating",
      1,
      "Inserting clock {} '{}' for instances: {}",
      clk_enable ? "enable" : "disable",
      combinedGatingCondNames(
          network, gate_cond_nets.begin(), gate_cond_nets.end(), clk_enable),
      combinedInstanceNames(network, instances.begin(), instances.end()));

  std::vector<sta::Instance*> new_instances;

  auto gate_cond_net = gate_cond_nets[0];
  for (int i = 1; i < gate_cond_nets.size(); i++) {
    auto inst_name = unique_name_cond_.GetUniqueName("clk_gate_cond_");
    auto inst_builder = clk_enable ? network_builder_.makeOr(inst_name)
                                   : network_builder_.makeAnd(inst_name);
    gate_cond_net = inst_builder.connectInput<0>(gate_cond_net)
                        .connectInput<1>(gate_cond_nets[i])
                        .makeOutputNet("clk_gate_cond_net_");
  }

  if (!clk_enable) {
    gate_cond_net = network_builder_
                        .makeNot(unique_name_cond_not_.GetUniqueName(
                            "clk_gate_cond_not_"))
                        .connectInput<0>(gate_cond_net)
                        .makeOutputNet("gate_cond_net_");
  }

  auto clk_pin = getClockPin(network, instances[0]);
  assert(clk_pin);
  auto clk_net = network->net(clk_pin);
  auto clk_network = sta_->cmdMode()->clkNetwork();
  auto clocks = clk_network->clocks(clk_pin);
  for (int i = 1; i < instances.size(); i++) {
    auto clk_pin = getClockPin(network, instances[i]);
    assert(clk_pin);
    if (*clk_network->clocks(clk_pin) != *clocks) {
      logger_->error(
          utl::CGT,
          12,
          "Cannot insert a clock gate for multiple distinct clocks.");
    }
  }

  auto gated_clk
      = network_builder_
            .makeClockGate(unique_name_gate_.GetUniqueName("clk_gate_"))
            .connectClock(clk_net)
            .connectInput<0>(gate_cond_net)
            .makeOutputNet(std::string("gated_") + network->name(clk_net)
                           + '_');

  dump("gate");

  for (const auto inst : instances) {
    auto clk_pin = getClockPin(network, inst);
    if (!clk_pin) {
      continue;
    }

    debug(3,
          "Disconnecting {} from {}",
          network->name(clk_pin),
          network->name(clk_net));
    network->disconnectPin(clk_pin);

    debug(3,
          "Connecting {} to {}",
          network->name(clk_pin),
          network->name(gated_clk));
    network->connectPin(clk_pin, gated_clk);
  }
}

UniquePtrWithDeleter<abc::Abc_Ntk_t> ClockGating::Impl::exportToAbc(
    sta::Instance* const instance,
    const std::vector<sta::Net*>& nets)
{
  utl::DebugScopedTimer timer(network_export_time_,
                              logger_,
                              CGT,
                              "clock_gating",
                              3,
                              "Network export time: {}");
  auto network = sta_->getDbNetwork();
  auto graph = sta_->graph();
  // Endpoints in discovery order; the set only removes duplicates.
  std::vector<sta::Vertex*> endpoints;
  std::unordered_set<sta::Vertex*> seen_endpoints;
  for (auto gate_cond_net : nets) {
    auto pin_iter = network->pinIterator(gate_cond_net);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      auto vertex = graph->pinLoadVertex(pin);
      if (sta_->search()->isEndpoint(vertex)
          && seen_endpoints.insert(vertex).second) {
        endpoints.push_back(vertex);
      }
    }
    delete pin_iter;
  }

  auto pin_iter = network->pinIterator(instance);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    if (network->direction(pin)->isAnyInput()) {
      auto vertex = graph->pinLoadVertex(pin);
      if (sta_->search()->isEndpoint(vertex)
          && seen_endpoints.insert(vertex).second) {
        endpoints.push_back(vertex);
      }
    }
  }
  delete pin_iter;

  cut::LogicExtractorFactory logic_extractor(sta_, logger_);
  for (auto endpoint : endpoints) {
    logic_extractor.AppendEndpoint(endpoint);
  }
  cut::LogicCut cut = logic_extractor.BuildLogicCut(*abc_library_);

  UniquePtrWithDeleter<abc::Abc_Ntk_t> abc_network
      = cut.BuildMappedAbcNetwork(*abc_library_, network, logger_);

  auto top = network->topInstance();
  abc::Abc_NtkSetName(abc_network.get(), strdup(network->name(top).c_str()));

  auto library = static_cast<abc::Mio_Library_t*>(abc_network->pManFunc);
  // Install library for NtkMap
  abc::Abc_FrameSetLibGen(library);

  if (logger_->debugCheck(CGT, "clock_gating", 5)) {
    abc::Abc_NtkPrintStats(abc_network.get(),
                           /*fFactored=*/false,
                           /*fSaveBest=*/false,
                           /*fDumpResult=*/false,
                           /*fUseLutLib=*/false,
                           /*fPrintMuxes=*/false,
                           /*fPower=*/false,
                           /*fGlitch=*/false,
                           /*fSkipBuf=*/false,
                           /*fSkipSmall=*/false,
                           /*fPrintMem=*/false);
  }

  dumpAbc("export", abc_network.get());

  return abc_network;
}

void ClockGating::Impl::dump(const char* const name)
{
  if (!dump_dir_) {
    return;
  }
  auto network = sta_->getDbNetwork();
  auto top = network->topInstance();
  std::ofstream graphviz_dump(getNetworkGraphvizDumpPath(name));
  dumpGraphviz(network, top, graphviz_dump);
  dump_counter_++;
}

void ClockGating::Impl::dumpAbc(const char* const name,
                                abc::Abc_Ntk_t* const network)
{
  if (!dump_dir_) {
    return;
  }
  auto nodes = WrapUnique(Abc_NtkCollectObjects(network));
  Io_WriteDotNtk(network,
                 nodes.get(),
                 nullptr,
                 getAbcGraphvizDumpPath(name).generic_string().data(),
                 true,
                 false,
                 false);
  auto netlist = network;
  if (!abc::Abc_NtkIsNetlist(network)) {
    netlist = abc::Abc_NtkToNetlist(network);
  }
  if (!abc::Abc_NtkHasMapping(netlist) && !abc::Abc_NtkHasAig(netlist)) {
    abc::Abc_NtkToAig(netlist);
  }
  abc::Io_WriteVerilog(
      netlist, getAbcVerilogDumpPath(name).generic_string().data(), 0, 0);
  if (!abc::Abc_NtkIsNetlist(network)) {
    abc::Abc_NtkDelete(netlist);
  }
  dump_counter_++;
}

std::filesystem::path ClockGating::Impl::getNetworkGraphvizDumpPath(
    const char* const name)
{
  return getDumpDir()
         / (std::to_string(dump_counter_) + "_openroad_" + name + ".dot");
}

std::filesystem::path ClockGating::Impl::getAbcGraphvizDumpPath(
    const char* const name)
{
  return getDumpDir()
         / (std::to_string(dump_counter_) + "_abc_" + name + ".dot");
}

std::filesystem::path ClockGating::Impl::getAbcVerilogDumpPath(
    const char* const name)
{
  return getDumpDir() / (std::to_string(dump_counter_) + "_abc_" + name + ".v");
}

std::filesystem::path ClockGating::Impl::getDumpDir()
{
  if (dump_dir_) {
    return *dump_dir_;
  }
  logger_->error(
      utl::CGT, 1, "Dump directory is not set. Cannot create dump path.");
}

}  // namespace cgt
