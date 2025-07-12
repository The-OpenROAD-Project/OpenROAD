// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "cgt/ClockGating.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <unordered_set>

#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "base/main/abcapis.h"
#include "base/main/mainInt.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "lext/abc_library_factory.h"
#include "lext/logic_extractor.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "misc/vec/vecPtr.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/Bfs.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/VerilogReader.hh"
#include "utl/Logger.h"

using utl::CGT;
using utl::UniquePtrWithDeleter;

// Headers have duplicate declarations so we include
// a forward one to get at this function without angering
// gcc.
namespace abc {
extern Abc_Ntk_t* Abc_NtkMulti(Abc_Ntk_t* pNtk,
                               int nThresh,
                               int nFaninMax,
                               int fCnf,
                               int fMulti,
                               int fSimple,
                               int fFactor);
}  // namespace abc

namespace cgt {

ClockGating::ClockGating() = default;

ClockGating::~ClockGating() = default;

void ClockGating::init(utl::Logger* logger, sta::dbSta* open_sta)
{
  logger_ = logger;
  sta_ = open_sta;
  abc_factory_ = std::make_unique<lext::AbcLibraryFactory>(logger_);
}

// Dumps the given network as GraphViz.
static void dumpGraphviz(sta::dbNetwork* network,
                         sta::Instance* inst,
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
    out << "}\n{\nrank=max;\n";
    pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyOutput()) {
        out << '"' << network->name(pin)
            << "\"[shape=triangle, color=coral];\n";
      }
    }
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
  }
  auto pin_iter = network->pinIterator(inst);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    out << '"' << network->name(pin) << "\" [label=\"" << network->portName(pin)
        << "\"];\n";
  }
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
  auto child_iter = network->childIterator(inst);
  while (child_iter->hasNext()) {
    auto child = child_iter->next();
    dumpGraphviz(network, child, out);
  }
  out << "}\n";
}

// Times the given function `f` and logs the time taken. Aggregates the time in
// microseconds.
template <typename F>
static auto timed(Logger* logger, size_t& total_us, const char* label, F f)
{
  auto start = std::chrono::steady_clock::now();
  const auto end_timer = [&] {
    auto end = std::chrono::steady_clock::now() - start;
    auto time_us
        = std::chrono::duration_cast<std::chrono::microseconds>(end).count();
    debugPrint(
        logger, utl::CGT, "clock_gating", 2, "{} time: {} μs", label, time_us);
    total_us += time_us;
  };
  if constexpr (std::is_void_v<decltype(f())>) {
    f();
    end_timer();
  } else {
    auto result = f();
    end_timer();
    return std::move(result);
  }
}

// Returns the output pin of a register cell, if it exists.
static sta::Pin* getRegOutPin(sta::dbNetwork* network, sta::Instance* inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto seq : cell->sequentials()) {
    if (seq->isRegister() && seq->output()) {
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        auto port = port_iter.next();
        if (port->direction()->isAnyOutput()) {
          assert(port->function()->op() == sta::FuncExpr::op_port);
          assert(port->function()->port() == seq->output());
          return network->findPin(inst, port);
        }
      }
    }
  }
  return nullptr;
}

// Returns the function of a register cell, if it exists.
static sta::FuncExpr* getRegFunction(sta::dbNetwork* network,
                                     sta::Instance* inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto seq : cell->sequentials()) {
    if (seq->isRegister() && seq->data()) {
      return seq->data();
    }
  }
  return nullptr;
}

// Returns the clock pin of a register cell, if it exists.
static sta::Pin* getClockPin(sta::dbNetwork* network, sta::Instance* inst)
{
  auto cell = network->libertyCell(inst);
  if (!cell) {
    return nullptr;
  }
  for (auto seq : cell->sequentials()) {
    if (seq->isRegister() && seq->clock()) {
      auto clock_expr = seq->clock();
      assert(clock_expr->op() == sta::FuncExpr::op_port);
      return network->findPin(inst, clock_expr->port());
    }
  }
  return nullptr;
}

void ClockGating::setDumpDir(const char* dir)
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
// registers, and no more than max_count).
static std::vector<sta::Net*> downstreamNets(
    sta::dbSta* open_sta,
    const std::vector<sta::Instance*>& instances,
    size_t max_count)
{
  class SearchPred final : public sta::SearchPredNonReg2
  {
   public:
    SearchPred(sta::dbSta* open_sta) : SearchPredNonReg2(open_sta) {}
    bool searchFrom(const sta::Vertex* from_vertex) final { return true; }
    bool searchTo(const sta::Vertex* to_vertex) final
    {
      return visited_.find(to_vertex) == visited_.end();
    }

    std::unordered_set<const sta::Vertex*> visited_;
  };

  auto network = open_sta->getDbNetwork();
  auto graph = open_sta->graph();
  SearchPred pred(open_sta);
  std::vector<sta::Net*> nets;
  std::unordered_set<sta::Net*> visited_nets;
  {
    sta::BfsFwdIterator iter(sta::BfsIndex::other, &pred, open_sta);
    for (auto& inst : instances) {
      auto pin_iter = network->pinIterator(inst);
      while (pin_iter->hasNext()) {
        auto pin = pin_iter->next();
        if (network->direction(pin)->isAnyOutput()) {
          iter.enqueue(graph->pinLoadVertex(pin));
        }
      }
    }
    while (iter.hasNext()) {
      auto vertex = iter.next();
      auto net = network->net(vertex->pin());
      if (net && visited_nets.find(net) == visited_nets.end()) {
        visited_nets.insert(net);
        nets.push_back(net);
        if (nets.size() >= max_count) {
          return nets;
        }
      }
      pred.visited_.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return nets;
}

// Gathers nets that are upstream of the given nets.
static std::vector<sta::Net*> upstreamNets(sta::dbSta* open_sta,
                                           std::vector<sta::Net*>& nets)
{
  class SearchPred final : public sta::SearchPredNonReg2
  {
   public:
    SearchPred(sta::dbSta* open_sta) : SearchPredNonReg2(open_sta) {}
    bool searchFrom(const sta::Vertex* from_vertex) final
    {
      return visited_.find(from_vertex) == visited_.end();
    }
    bool searchTo(const sta::Vertex* to_vertex) final { return true; }

    std::unordered_set<const sta::Vertex*> visited_;
  };

  auto network = open_sta->getDbNetwork();
  auto graph = open_sta->graph();
  SearchPred pred(open_sta);
  std::unordered_set<sta::Net*> visited_nets;
  {
    sta::BfsBkwdIterator iter(sta::BfsIndex::other, &pred, open_sta);
    for (auto& inst : nets) {
      auto pin_iter = network->pinIterator(inst);
      while (pin_iter->hasNext()) {
        auto pin = pin_iter->next();
        if (network->direction(pin)->isAnyInput()) {
          iter.enqueue(graph->pinLoadVertex(pin));
        }
      }
    }
    while (iter.hasNext()) {
      auto vertex = iter.next();
      auto net = network->net(vertex->pin());
      if (net && visited_nets.find(net) == visited_nets.end()) {
        visited_nets.insert(net);
        nets.push_back(net);
      }
      pred.visited_.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return nets;
}

// Creates a string that combines names of the given gating condition.
static std::string combinedGatingCondNames(
    sta::dbNetwork* network,
    std::vector<sta::Net*>::const_iterator begin,
    std::vector<sta::Net*>::const_iterator end,
    bool clk_enable)
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
    sta::dbNetwork* network,
    std::vector<sta::Instance*>::const_iterator begin,
    std::vector<sta::Instance*>::const_iterator end)
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

void ClockGating::run()
{
  debugPrint(logger_,
             CGT,
             "clock_gating",
             1,
             "Starting clock gating with parameters:");
  debugPrint(logger_,
             CGT,
             "clock_gating",
             1,
             "Minimum instances per clock gate: {}",
             min_instances_);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             1,
             "Maximum gate condition cover size: {}",
             max_cover_);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             1,
             "Group registers: {}",
             group_instances_ == GroupInstances::No ? "No"
             : group_instances_ == GroupInstances::NetPrefix
                 ? "Net prefix"
                 : "Instance prefix");
  rand_bits_.seed(0);  // For reproducibility

  auto network = sta_->getDbNetwork();
  network_builder_.init(logger_, network);

  sta_->ensureGraph();
  sta_->ensureLevelized();
  sta_->searchPreamble();

  // Initialize ABC
  abc::Abc_Start();
  abc_factory_->AddDbSta(sta_);
  abc_library_ = std::make_unique<lext::AbcLibrary>(abc_factory_->Build());

  abc::Abc_SclInstallGenlib(abc_library_->abc_library(),
                            /*Slew=*/0,
                            /*Gain=*/0,
                            /*fUseAll=*/0,
                            /*nGatesMin=*/0);
  abc::Mio_LibraryTransferCellIds();

  dump("pre");

  using ClockNet = sta::Net;
  std::vector<std::pair<ClockNet*, sta::Instance*>> found_insts;
  found_insts.reserve(instances_.size());
  for (const auto& inst_name : instances_) {
    auto inst = network->findInstance(inst_name.c_str());
    if (inst) {
      auto clk_pin = getClockPin(network, inst);
      if (!clk_pin) {
        logger_->error(CGT,
                       1,
                       "Clock pin not found for instance '{}'",
                       network->name(inst));
        continue;
      }
      found_insts.emplace_back(network->net(clk_pin), inst);
    } else {
      logger_->warn(CGT, 2, "Instance '{}' not found in the design", inst_name);
    }
  }
  if (found_insts.empty()) {
    auto top = network->topInstance();
    auto inst_iter = network->childIterator(top);
    while (inst_iter->hasNext()) {
      auto inst = inst_iter->next();
      auto clk_pin = getClockPin(network, inst);
      if (clk_pin) {
        found_insts.emplace_back(network->net(clk_pin), inst);
      }
    }
  }

  std::unordered_map<std::string,
                     std::vector<std::pair<ClockNet*, sta::Instance*>>>
      inst_groups;
  for (auto [clk_net, inst] : found_insts) {
    std::string prefix = network->name(inst);
    if (group_instances_ == GroupInstances::NetPrefix) {
      auto out_pin = getRegOutPin(network, inst);
      prefix = network->name(network->net(out_pin));
    }
    if (group_instances_ != GroupInstances::No) {
      auto last_dot_pos = prefix.find_last_of('.');
      if (last_dot_pos == std::string::npos) {
        last_dot_pos = 0;
      }
      auto bracket_pos = prefix.find('[', last_dot_pos);
      if (bracket_pos != std::string::npos) {
        prefix = prefix.substr(0, bracket_pos);
      }
    }
    inst_groups[prefix].emplace_back(clk_net, inst);
  }
  found_insts.clear();

  std::vector<std::vector<sta::Instance*>> sorted_inst_groups;
  for (auto& [_, inst_group] : inst_groups) {
    // Group by clock net and sort by instance name
    std::sort(inst_group.begin(),
              inst_group.end(),
              [network](const auto& a, const auto& b) {
                int clk_net_cmp = std::strcmp(network->name(a.first),
                                              network->name(b.first));
                return clk_net_cmp < 0
                       || (clk_net_cmp == 0
                           && std::strcmp(network->name(a.second),
                                          network->name(b.second))
                                  < 0);
              });
    auto last_clk_net = inst_group.front().first;
    sorted_inst_groups.emplace_back();
    // Move to sorted_inst_groups, splitting on clock net groups
    for (auto [clk_net, inst] : inst_group) {
      if (last_clk_net == clk_net) {
        sorted_inst_groups.back().push_back(inst);
      } else {
        last_clk_net = clk_net;
        sorted_inst_groups.emplace_back(std::vector<sta::Instance*>{inst});
      }
    }
  }
  inst_groups.clear();
  // Sort by group's first instance name for stability between stdlib versions
  std::sort(sorted_inst_groups.begin(),
            sorted_inst_groups.end(),
            [network](const auto& insts_a, const auto& insts_b) {
              return std::strcmp(network->name(insts_a.front()),
                                 network->name(insts_b.front()))
                     < 0;
            });

  std::unordered_map<sta::Net*, std::vector<size_t>> net_to_gates;
  std::vector<
      std::tuple<std::vector<sta::Net*>, std::vector<sta::Instance*>, bool>>
      accepted_gates;

  for (size_t i = 0; i < sorted_inst_groups.size(); i++) {
    auto& inst_group = sorted_inst_groups[i];
    if (i % 100 == 0) {
      logger_->info(CGT,
                    3,
                    "Clock gating instance group {}/{}",
                    i,
                    sorted_inst_groups.size());
    }
    std::vector<sta::Net*> gate_cond_cover
        = downstreamNets(sta_, inst_group, max_cover_);
    debugPrint(
        logger_,
        CGT,
        "clock_gating",
        1,
        "Gate condition cover size: {} for instances: {}",
        gate_cond_cover.size(),
        combinedInstanceNames(network, inst_group.begin(), inst_group.end()));

    auto abc_network
        = timed(logger_, network_export_time_us_, "Network export ", [&] {
            return exportToAbc(inst_group, gate_cond_cover);
          });

    if (!gate_cond_nets_.empty()) {
      auto it = std::remove_if(
          gate_cond_cover.begin(), gate_cond_cover.end(), [&](sta::Net* net) {
            return gate_cond_nets_.find(network->name(net))
                   == gate_cond_nets_.end();
          });
      gate_cond_cover.erase(it, gate_cond_cover.end());
    }
    if (gate_cond_cover.empty()) {
      continue;
    }

    bool inserted = false;
    std::vector<size_t> idxs;
    timed(logger_,
          reuse_check_time_us_,
          "Existing gating conditions reuse check",
          [&] {
            auto related_nets = upstreamNets(sta_, gate_cond_cover);
            for (auto net : related_nets) {
              for (auto idx : net_to_gates[net]) {
                auto it = std::lower_bound(idxs.begin(), idxs.end(), idx);
                if (it == idxs.end() || *it != idx) {
                  idxs.insert(it, idx);
                }
              }
            }
            for (auto idx : idxs) {
              auto& [conds, insts, en] = accepted_gates[idx];
              if (isCorrectClockGate(inst_group, conds, *abc_network, en)) {
                debugPrint(
                    logger_,
                    CGT,
                    "clock_gating",
                    1,
                    "Found existing clock gate condition: {} for instances: {}",
                    combinedGatingCondNames(
                        network, conds.begin(), conds.end(), en),
                    combinedInstanceNames(
                        network, inst_group.begin(), inst_group.end()));
                insts.insert(insts.end(), inst_group.begin(), inst_group.end());
                inserted = true;
                break;
              }
            }
            debugPrint(
                logger_,
                CGT,
                "clock_gating",
                1,
                "Checked {} existing clock gate conditions for instances: {}",
                idxs.size(),
                combinedInstanceNames(
                    network, inst_group.begin(), inst_group.end()));
          });
    if (inserted) {
      continue;
    }

    bool clk_enable = false;

    std::vector<sta::Net*> correct_conds;
    timed(logger_, search_time_us_, "Clock gate condition search", [&] {
      if (isCorrectClockGate(inst_group, gate_cond_cover, *abc_network, true)) {
        clk_enable = true;
      } else if (isCorrectClockGate(
                     inst_group, gate_cond_cover, *abc_network, false)) {
        clk_enable = false;
      } else {
        debugPrint(logger_,
                   CGT,
                   "clock_gating",
                   1,
                   "Clock gating failed for instances: {}",
                   combinedInstanceNames(
                       network, inst_group.begin(), inst_group.end()));
        return;
      }

      correct_conds = gate_cond_cover;
      searchClockGates(inst_group,
                       correct_conds,
                       correct_conds.begin(),
                       correct_conds.end(),
                       *abc_network,
                       clk_enable);
    });
    if (correct_conds.empty()) {
      continue;
    }

    logger_->info(
        CGT,
        4,
        "Accepted clock {} '{}' for instances: {}",
        clk_enable ? "enable" : "disable",
        combinedGatingCondNames(
            network, correct_conds.begin(), correct_conds.end(), clk_enable),
        combinedInstanceNames(network, inst_group.begin(), inst_group.end()));

    timed(logger_,
          exist_check_time_us_,
          "Check if gate condition already exists",
          [&] {
            for (auto net : correct_conds) {
              for (auto idx : net_to_gates[net]) {
                auto& [conds, insts, en] = accepted_gates[idx];
                if (clk_enable == en
                    && std::includes(correct_conds.begin(),
                                     correct_conds.end(),
                                     conds.begin(),
                                     conds.end())) {
                  logger_->info(
                      CGT,
                      5,
                      "Extending previously accepted clock {} '{}' to '{}'",
                      clk_enable ? "enable" : "disable",
                      combinedGatingCondNames(
                          network, conds.begin(), conds.end(), clk_enable),
                      combinedGatingCondNames(network,
                                              correct_conds.begin(),
                                              correct_conds.end(),
                                              clk_enable));
                  conds = correct_conds;
                  for (auto net : correct_conds) {
                    auto& gates = net_to_gates[net];
                    auto it = std::lower_bound(gates.begin(), gates.end(), idx);
                    if (it == gates.end() || *it != idx) {
                      gates.insert(it, idx);
                    }
                  }
                  insts.insert(
                      insts.end(), inst_group.begin(), inst_group.end());
                  inserted = true;
                  break;
                }
              }
              if (inserted) {
                break;
              }
            }
          });
    if (!inserted) {
      accepted_gates.emplace_back(correct_conds, inst_group, clk_enable);
      for (auto net : correct_conds) {
        net_to_gates[net].push_back(accepted_gates.size() - 1);
      }
    }
  }

  for (auto& [correct_conds, instances, clk_enable] : accepted_gates) {
    if (instances.size() >= min_instances_) {
      insertClockGate(instances, correct_conds, clk_enable);
    } else {
      debugPrint(
          logger_,
          CGT,
          "clock_gating",
          1,
          "Not enough instances ({}) for clock gate: {}",
          instances.size(),
          combinedInstanceNames(network, instances.begin(), instances.end()));
    }
  }

  dump("post");
  abc::Abc_Stop();

  debugPrint(logger_, CGT, "clock_gating", 1, "Clock gating completed.");
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Rejected at simulation: {}",
             rejected_sim_count_);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Rejected at SAT: {}",
             rejected_sat_count_);
  debugPrint(logger_, CGT, "clock_gating", 1, "Accepted: {}", accepted_count_);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total gate condition reuse check time: {} s",
             reuse_check_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total gate condition search time: {} s",
             search_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total existing gate condition check time: {} s",
             exist_check_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total network export time: {} s",
             network_export_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total network build time: {} s",
             network_build_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total simulation time: {} s",
             sim_time_us_ / 1000 / 1000);
  debugPrint(logger_,
             CGT,
             "clock_gating",
             2,
             "Total SAT time: {} s",
             sat_time_us_ / 1000 / 1000);
}

// Returns a sequence of nets that excludes the range [begin, end) from the
// given sequence.
static std::vector<sta::Net*> except(std::vector<sta::Net*>& gate_cond_nets,
                                     std::vector<sta::Net*>::iterator begin,
                                     std::vector<sta::Net*>::iterator end)
{
  std::vector<sta::Net*> result;
  result.insert(result.end(), gate_cond_nets.begin(), begin);
  result.insert(result.end(), end, gate_cond_nets.end());
  return result;
}

void ClockGating::searchClockGates(const std::vector<sta::Instance*>& instances,
                                   std::vector<sta::Net*>& good_gate_conds,
                                   std::vector<sta::Net*>::iterator begin,
                                   std::vector<sta::Net*>::iterator end,
                                   abc::Abc_Ntk_t& abc_network,
                                   bool clk_enable)
{
  if (end - begin < 2) {
    return;
  }
  auto mid = begin + (end - begin) / 2;
  auto combined_candidate_names
      = combinedGatingCondNames(sta_->getDbNetwork(), mid, end, clk_enable);
  debugPrint(
      logger_,
      CGT,
      "clock_gating",
      3,
      "Checking if '{}' can be removed from clock {}: '{}' for instances: {}",
      combined_candidate_names,
      clk_enable ? "enable" : "disable",
      combined_candidate_names,
      combinedInstanceNames(
          sta_->getDbNetwork(), instances.begin(), instances.end()));
  if (isCorrectClockGate(instances,
                         except(good_gate_conds, mid, end),
                         abc_network,
                         clk_enable)) {
    debugPrint(logger_,
               CGT,
               "clock_gating",
               3,
               "Clock gating signals: '{}' can be dropped",
               combined_candidate_names);
    good_gate_conds.erase(mid, end);
  } else if (end - mid > 1) {
    debugPrint(logger_,
               CGT,
               "clock_gating",
               3,
               "Clock gating signals: {} cannot be dropped together",
               combined_candidate_names);
    searchClockGates(
        instances, good_gate_conds, mid, end, abc_network, clk_enable);
  } else {
    debugPrint(logger_,
               CGT,
               "clock_gating",
               3,
               "Clock gating signal: {} cannot be dropped",
               combined_candidate_names);
  }
  searchClockGates(
      instances, good_gate_conds, begin, mid, abc_network, clk_enable);
}

// A modified version of Abc_NtkDup that only works on netlists and copies all
// object names.
static abc::Abc_Ntk_t* Abc_NtkDupNetlist(abc::Abc_Ntk_t* network)
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
    new_network->pExcare = Abc_NtkDup((abc::Abc_Ntk_t*) network->pExcare);
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
static utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk)
{
  return utl::UniquePtrWithDeleter<abc::Abc_Ntk_t>(ntk, &abc::Abc_NtkDelete);
}

// Translates the function of a given instance into an ABC network,
static abc::Abc_Obj_t* regFunctionToAbc(sta::dbNetwork* network,
                                        abc::Abc_Ntk_t* abc_network,
                                        abc::Vec_Ptr_t* fanins,
                                        sta::Instance* inst)
{
  std::unordered_map<sta::LibertyPort*, std::pair<abc::Abc_Obj_t*, const char*>>
      port_to_obj;
  std::unordered_map<sta::LibertyPort*, abc::Abc_Obj_t*> output_to_obj;
  auto pin_iter = network->pinIterator(inst);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    auto net = network->net(pin);
    auto port = network->libertyPort(pin);
    if (port->isClock()) {
      continue;
    }
    auto& [obj, net_name] = port_to_obj[port];
    net_name = network->name(pin);
    obj = abc::Abc_NtkFindNet(abc_network,
                              const_cast<char*>(network->name(net)));
  }
  std::vector<sta::FuncExpr*> traversal_stack = {getRegFunction(network, inst)};
  std::vector<sta::FuncExpr*> expr_stack;
  while (!traversal_stack.empty()) {
    auto expr = traversal_stack.back();
    expr_stack.push_back(expr);
    traversal_stack.pop_back();
    switch (expr->op()) {
      case sta::FuncExpr::op_port:
      case sta::FuncExpr::op_zero:
      case sta::FuncExpr::op_one:
        break;
      case sta::FuncExpr::op_and:
      case sta::FuncExpr::op_or:
      case sta::FuncExpr::op_xor:
        traversal_stack.push_back(expr->right());
      case sta::FuncExpr::op_not:
        traversal_stack.push_back(expr->left());
        break;
    }
  }
  std::vector<abc::Abc_Obj_t*> obj_stack;
  while (!expr_stack.empty()) {
    auto expr = expr_stack.back();
    expr_stack.pop_back();
    switch (expr->op()) {
      case sta::FuncExpr::op_port: {
        auto port = expr->port();
        assert(port->direction()->isAnyInput());
        auto& [obj, net_name] = port_to_obj[port];
        if (!obj) {
          abc::Abc_Obj_t* pi = abc::Abc_NtkCreatePi(abc_network);
          obj = abc::Abc_NtkCreateNet(abc_network);
          abc::Abc_ObjAssignName(obj, const_cast<char*>(net_name), nullptr);
          abc::Abc_ObjAddFanin(obj, pi);
        }
        obj_stack.emplace_back(obj);
      } break;
      case sta::FuncExpr::op_zero: {
        abc::Abc_Obj_t* zero_node = abc::Abc_NtkCreateNodeConst0(abc_network);
        abc::Abc_Obj_t* zero_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(zero_net, zero_node);
        obj_stack.emplace_back(zero_net);
      }
      case sta::FuncExpr::op_one: {
        abc::Abc_Obj_t* one_node = abc::Abc_NtkCreateNodeConst1(abc_network);
        abc::Abc_Obj_t* one_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(one_net, one_node);
        obj_stack.emplace_back(one_net);
      } break;
      case sta::FuncExpr::op_not: {
        auto obj = obj_stack.back();
        obj_stack.pop_back();
        abc::Abc_Obj_t* inv_node = abc::Abc_NtkCreateNodeInv(abc_network, obj);
        abc::Abc_Obj_t* inv_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(inv_net, inv_node);
        obj_stack.emplace_back(inv_net);
      } break;
      case sta::FuncExpr::op_and: {
        abc::Vec_PtrClear(fanins);
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Abc_Obj_t* and_node
            = abc::Abc_NtkCreateNodeAnd(abc_network, fanins);
        abc::Abc_Obj_t* and_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(and_net, and_node);
        obj_stack.emplace_back(and_net);
      } break;
      case sta::FuncExpr::op_or: {
        abc::Vec_PtrClear(fanins);
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Abc_Obj_t* or_node = abc::Abc_NtkCreateNodeOr(abc_network, fanins);
        abc::Abc_Obj_t* or_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(or_net, or_node);
        obj_stack.emplace_back(or_net);
      } break;
      case sta::FuncExpr::op_xor: {
        abc::Vec_PtrClear(fanins);
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Vec_PtrPush(fanins, obj_stack.back());
        obj_stack.pop_back();
        abc::Abc_Obj_t* xor_node
            = abc::Abc_NtkCreateNodeExor(abc_network, fanins);
        abc::Abc_Obj_t* xor_net = abc::Abc_NtkCreateNet(abc_network);
        abc::Abc_ObjAddFanin(xor_net, xor_node);
        obj_stack.emplace_back(xor_net);
      } break;
    }
  }
  assert(obj_stack.size() == 1);
  return obj_stack.back();
}

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> ClockGating::makeTestNetwork(
    const std::vector<sta::Instance*>& instances,
    const std::vector<sta::Net*>& gate_cond_nets,
    abc::Abc_Ntk_t& abc_network_ref,
    bool clk_enable)
{
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

  abc::Vec_Ptr_t* xor_states = abc::Vec_PtrAlloc(1);
  abc::Vec_Ptr_t* fanins = abc::Vec_PtrAlloc(2);
  auto network = sta_->getDbNetwork();
  for (auto inst : instances) {
    auto prev_state
        = regFunctionToAbc(network, abc_network.get(), fanins, inst);
    abc::Vec_PtrClear(fanins);
    abc::Vec_PtrPush(fanins, prev_state);
    auto out_pin = getRegOutPin(network, inst);
    assert(out_pin);
    auto out_pin_name = network->name(out_pin);
    auto out_net_name = network->name(network->net(out_pin));
    for (const auto& output_name : {out_pin_name, out_net_name}) {
      abc::Abc_Obj_t* curr_state = abc::Abc_NtkFindNet(
          abc_network.get(), const_cast<char*>(output_name));
      if (!curr_state) {
        abc::Abc_Obj_t* pi = abc::Abc_NtkCreatePi(abc_network.get());
        abc::Abc_Obj_t* net = abc::Abc_NtkCreateNet(abc_network.get());
        abc::Abc_ObjAssignName(net, const_cast<char*>(output_name), nullptr);
        abc::Abc_ObjAddFanin(net, pi);
        curr_state = net;
      } else {
        assert(abc::Abc_ObjIsNet(curr_state));
      }
      abc::Vec_PtrPush(fanins, curr_state);
      abc::Abc_Obj_t* xor_state
          = abc::Abc_NtkCreateNodeExor(abc_network.get(), fanins);
      abc::Vec_PtrPop(fanins);
      abc::Abc_Obj_t* net_xor_state = abc::Abc_NtkCreateNet(abc_network.get());
      abc::Abc_ObjAddFanin(net_xor_state, xor_state);
      abc::Vec_PtrPush(xor_states, net_xor_state);
    }
  }
  abc::Vec_PtrFree(fanins);

  std::unordered_set<std::string> gate_cond_names;
  for (auto gate_cond_net : gate_cond_nets) {
    gate_cond_names.insert(network->name(gate_cond_net));
  }

  {
    abc::Abc_Obj_t* obj;
    int idx;
    abc::Vec_Ptr_t* gate_cond_nets = abc::Vec_PtrAlloc(1);
    Abc_NtkForEachNet(abc_network.get(), obj, idx)
    {
      if (gate_cond_names.find(abc::Abc_ObjName(obj))
          != gate_cond_names.end()) {
        abc::Vec_PtrPush(gate_cond_nets, obj);
        debugPrint(logger_,
                   CGT,
                   "clock_gating",
                   4,
                   "Found gate net: {}",
                   abc::Abc_ObjName(obj));
      }
    }

    assert(abc::Vec_PtrSize(xor_states) > 0);
    auto xor_state_net
        = static_cast<abc::Abc_Obj_t*>(abc::Vec_PtrEntry(xor_states, 0));
    if (abc::Vec_PtrSize(xor_states) > 1) {
      auto xors_or = abc::Abc_NtkCreateNodeOr(abc_network.get(), xor_states);
      abc::Abc_ObjAssignName(xors_or, const_cast<char*>("or_xors"), nullptr);
      xor_state_net = abc::Abc_NtkCreateNet(abc_network.get());
      abc::Abc_ObjAssignName(
          xor_state_net, const_cast<char*>("or_xors"), nullptr);
      abc::Abc_ObjAddFanin(xor_state_net, xors_or);
    }
    abc::Vec_PtrFree(xor_states);

    if (abc::Vec_PtrSize(gate_cond_nets) == 0) {
      debugPrint(logger_, CGT, "clock_gating", 4, "No gate condition found");
      return {};
    }
    auto gate_cond_net
        = static_cast<abc::Abc_Obj_t*>(abc::Vec_PtrEntry(gate_cond_nets, 0));
    if (abc::Vec_PtrSize(gate_cond_nets) > 1) {
      auto gate_reduce
          = clk_enable
                ? abc::Abc_NtkCreateNodeOr(abc_network.get(), gate_cond_nets)
                : abc::Abc_NtkCreateNodeAnd(abc_network.get(), gate_cond_nets);
      abc::Abc_ObjAssignName(
          gate_reduce, const_cast<char*>("gate_cond"), nullptr);
      gate_cond_net = abc::Abc_NtkCreateNet(abc_network.get());
      abc::Abc_ObjAssignName(
          gate_cond_net, const_cast<char*>("gate_cond"), nullptr);
      abc::Abc_ObjAddFanin(gate_cond_net, gate_reduce);
    }

    if (clk_enable) {
      auto not_enable
          = abc::Abc_NtkCreateNodeInv(abc_network.get(), gate_cond_net);
      abc::Abc_Obj_t* not_enable_net = abc::Abc_NtkCreateNet(abc_network.get());
      abc::Abc_ObjAssignName(
          not_enable_net, const_cast<char*>("not_gate_cond"), nullptr);
      abc::Abc_ObjAddFanin(not_enable_net, not_enable);
      gate_cond_net = not_enable_net;
    }

    abc::Vec_Ptr_t* and_fanins = abc::Vec_PtrAlloc(2);
    abc::Vec_PtrPush(and_fanins, xor_state_net);
    abc::Vec_PtrPush(and_fanins, gate_cond_net);
    abc::Abc_Obj_t* and_gate
        = abc::Abc_NtkCreateNodeAnd(abc_network.get(), and_fanins);
    abc::Abc_Obj_t* and_net = abc::Abc_NtkCreateNet(abc_network.get());
    abc::Abc_ObjAssignName(and_net, const_cast<char*>("and"), nullptr);
    abc::Abc_ObjAddFanin(and_net, and_gate);

    abc::Abc_Obj_t* result = abc::Abc_NtkCreatePo(abc_network.get());
    abc::Abc_ObjAssignName(result, (char*) "result", nullptr);
    abc::Abc_ObjAddFanin(result, and_net);
    abc::Vec_PtrFree(and_fanins);

    // For error reporting
    std::string combined_gate_name;
    Vec_PtrForEachEntry(abc::Abc_Obj_t*, gate_cond_nets, gate_cond_net, idx)
    {
      if (idx == 0) {
        combined_gate_name = abc::Abc_ObjName(gate_cond_net);
      } else {
        combined_gate_name += (clk_enable ? " | " : " & ")
                              + std::string(abc::Abc_ObjName(gate_cond_net));
      }
    }
    abc::Vec_PtrFree(gate_cond_nets);

    abc_network = WrapUnique(abc::Abc_NtkToLogic(abc_network.get()));

    return WrapUnique(abc::Abc_NtkStrash(abc_network.get(),
                                         /*fAllNodes=*/false,
                                         /*fCleanup=*/false,
                                         /*fRecord=*/false));
  }
}

bool ClockGating::simulationTest(abc::Abc_Ntk_t* abc_network,
                                 const std::string& combined_gate_name)
{
  abc::Abc_Obj_t* obj;
  int idx;
  std::vector<int> sim_input;
  for (int i = 0; i < 10; i++) {
    Abc_NtkForEachPi(abc_network, obj, idx)
    {
      sim_input.resize(idx + 1);
      sim_input[idx] = rand_bits_.get();
      debugPrint(logger_,
                 CGT,
                 "clock_gating",
                 5,
                 "Input: {} == {}",
                 abc::Abc_ObjName(obj),
                 sim_input[idx]);
    }

    UniquePtrWithDeleter<int> sim_output(
        abc::Abc_NtkVerifySimulatePattern(abc_network, sim_input.data()),
        &free);
    if (*sim_output) {
      debugPrint(logger_,
                 CGT,
                 "clock_gating",
                 4,
                 "Clock gate signal '{}' is not valid (simulation test)",
                 combined_gate_name);
      rejected_sim_count_++;
      return false;
    }
  }
  return true;
}

bool ClockGating::satTest(abc::Abc_Ntk_t* abc_network,
                          const std::string& combined_gate_name)
{
  if (!abc::Abc_NtkMiterSat(abc_network, 0, 0, 0, nullptr, nullptr)) {
    // 0 is SAT
    debugPrint(logger_,
               CGT,
               "clock_gating",
               4,
               "Clock gate signal '{}' is not valid (simulation test)",
               combined_gate_name);
    abc::Abc_Obj_t* obj;
    int idx;
    Abc_NtkForEachPi(abc_network, obj, idx)
    {
      debugPrint(logger_,
                 CGT,
                 "clock_gating",
                 5,
                 "SAT value: {} == {}",
                 abc::Abc_ObjName(obj),
                 abc_network->pModel[idx]);
    }
    rejected_sat_count_++;
    return false;
  }
  return true;
}

bool ClockGating::isCorrectClockGate(
    const std::vector<sta::Instance*>& instances,
    const std::vector<sta::Net*>& gate_cond_nets,
    abc::Abc_Ntk_t& abc_network_ref,
    bool clk_enable)
{
  auto network = sta_->getDbNetwork();
  debugPrint(
      logger_,
      CGT,
      "clock_gating",
      4,
      "Checking if clock {} '{}' is good for instances: {}",
      clk_enable ? "enable" : "disable",
      combinedGatingCondNames(
          network, gate_cond_nets.begin(), gate_cond_nets.end(), clk_enable),
      combinedInstanceNames(network, instances.begin(), instances.end()));

  auto abc_network = timed(
      logger_, network_build_time_us_, "Network build for sim/SAT", [&] {
        return makeTestNetwork(
            instances, gate_cond_nets, abc_network_ref, clk_enable);
      });
  if (!abc_network) {
    return false;
  }

  dumpAbc("simsat", abc_network.get());

  if (!timed(logger_, sim_time_us_, "Simulation", [&] {
        // Simulate with random stimuli
        return simulationTest(abc_network.get(), "");
      })) {
    return false;
  }

  if (!timed(logger_, sat_time_us_, "SAT", [&] {
        // SAT prove correctness of selected clock gate
        return satTest(abc_network.get(), "");
      })) {
    return false;
  }

  dumpAbc("accepted", abc_network.get());
  accepted_count_++;
  return true;
}

void ClockGating::insertClockGate(const std::vector<sta::Instance*>& instances,
                                  const std::vector<sta::Net*>& gate_cond_nets,
                                  bool clk_enable)
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
  for (size_t i = 1; i < gate_cond_nets.size(); i++) {
    auto inst_name = getUniqueName("clk_gate");
    auto inst_builder = clk_enable ? network_builder_.makeOr(inst_name)
                                   : network_builder_.makeAnd(inst_name);
    gate_cond_net = inst_builder.connectInput<0>(gate_cond_net)
                        .connectInput<1>(gate_cond_nets[i])
                        .makeOutputNet(getUniqueName("clk_gate_cond_net"));
  }

  if (!clk_enable) {
    gate_cond_net = network_builder_.makeNot(getUniqueName("clk_gate_cond_not"))
                        .connectInput<0>(gate_cond_net)
                        .makeOutputNet(getUniqueName("gate_cond_net"));
  }

  sta::Net* clk_net = nullptr;
  for (const auto inst : instances) {
    auto clk_pin = getClockPin(network, inst);
    assert(clk_pin);
    if (!clk_net) {
      clk_net = network->net(clk_pin);
    }
    assert(clk_net == network->net(clk_pin));
  }

  auto gated_clk
      = network_builder_.makeClockGate(getUniqueName("clk_gate"))
            .connectClock(clk_net)
            .connectInput<0>(gate_cond_net)
            .makeOutputNet(getUniqueName("gated_", network->name(clk_net)));

  dump("gate");

  for (const auto inst : instances) {
    auto clk_pin = getClockPin(network, inst);
    if (!clk_pin) {
      continue;
    }

    debugPrint(logger_,
               CGT,
               "clock_gating",
               3,
               "Disconnecting {} from {}",
               network->name(clk_pin),
               network->name(clk_net));
    network->disconnectPin(clk_pin);

    debugPrint(logger_,
               CGT,
               "clock_gating",
               3,
               "Connecting {} to {}",
               network->name(clk_pin),
               network->name(gated_clk));
    network->connectPin(clk_pin, gated_clk);
  }
}

UniquePtrWithDeleter<abc::Abc_Ntk_t> ClockGating::exportToAbc(
    const std::vector<sta::Instance*>& instances,
    const std::vector<sta::Net*>& gate_cond_nets)
{
  auto network = sta_->getDbNetwork();
  auto graph = sta_->graph();
  std::unordered_set<sta::Vertex*> endpoints;
  for (auto gate_cond_net : gate_cond_nets) {
    auto pin_iter = network->pinIterator(gate_cond_net);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyInput()) {
        auto instance = network->instance(pin);
        auto pin_iter = network->pinIterator(instance);
        while (pin_iter->hasNext()) {
          auto pin = pin_iter->next();
          if (network->direction(pin)->isAnyOutput()) {
            endpoints.insert(graph->pinLoadVertex(pin));
          }
        }
      } else if (network->direction(pin)->isAnyOutput()) {
        endpoints.insert(graph->pinLoadVertex(pin));
      }
    }
  }

  for (const auto& inst : instances) {
    auto pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      auto pin = pin_iter->next();
      if (network->direction(pin)->isAnyInput()) {
        auto vertex = graph->pinLoadVertex(pin);
        endpoints.insert(vertex);
      }
    }
  }

  lext::LogicExtractorFactory logic_extractor(sta_, logger_);
  for (auto endpoint : endpoints) {
    logic_extractor.AppendEndpoint(endpoint);
  }
  lext::LogicCut cut = logic_extractor.BuildLogicCut(*abc_library_);

  UniquePtrWithDeleter<abc::Abc_Ntk_t> abc_network
      = cut.BuildMappedAbcNetwork(*abc_library_, network, logger_);

  auto top = network->topInstance();
  abc::Abc_NtkSetName(abc_network.get(), strdup(network->name(top)));

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

void ClockGating::dump(const char* name)
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

void ClockGating::dumpAbc(const char* name, abc::Abc_Ntk_t* network)
{
  if (!dump_dir_) {
    return;
  }
  abc::Vec_Ptr_t* nodes = Abc_NtkCollectObjects(network);
  Io_WriteDotNtk(network,
                 nodes,
                 nullptr,
                 getAbcGraphvizDumpPath(name).generic_string().data(),
                 true,
                 false,
                 false);
  auto netlist = network;
  if (!abc::Abc_NtkIsNetlist(network)) {
    netlist = abc::Abc_NtkToNetlist(network);
  }
  Vec_PtrFree(nodes);
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

}  // namespace cgt
