// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "kitty/dynamic_truth_table.hpp"
#include "mockturtle/algorithms/cleanup.hpp"
#include "mockturtle/io/genlib_reader.hpp"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/names_view.hpp"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace cut {

class LogicCut
{
 public:
  LogicCut(std::vector<sta::Net*>&& primary_inputs,
           std::vector<sta::Net*>&& primary_outputs,
           sta::InstanceSet&& cut_instances)
      : primary_inputs_(std::move(primary_inputs)),
        primary_outputs_(std::move(primary_outputs)),
        cut_instances_(std::move(cut_instances))
  {
  }
  ~LogicCut() = default;

  const std::vector<sta::Net*>& primary_inputs() const
  {
    return primary_inputs_;
  }
  const std::vector<sta::Net*>& primary_outputs() const
  {
    return primary_outputs_;
  }
  const sta::InstanceSet& cut_instances() const { return cut_instances_; }

  bool IsEmpty() const
  {
    return primary_inputs_.empty() && primary_outputs_.empty()
           && cut_instances_.empty();
  }

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> BuildMappedAbcNetwork(
      AbcLibrary& abc_library,
      sta::dbNetwork* network,
      utl::Logger* logger);

  template <unsigned MaxInputs>
  mockturtle::names_view<mockturtle::aig_network> BuildMappedMockturtleNetwork(
      mockturtle::tech_library<MaxInputs>& mockturtle_library,
      sta::dbNetwork* network,
      utl::Logger* logger);

  void InsertMappedAbcNetwork(abc::Abc_Ntk_t* abc_network,
                              AbcLibrary& abc_library,
                              sta::dbNetwork* network,
                              utl::UniqueName& unique_name,
                              utl::Logger* logger);

 private:
  std::vector<sta::Net*> primary_inputs_;
  std::vector<sta::Net*> primary_outputs_;
  sta::InstanceSet cut_instances_;
};

namespace {

mockturtle::signal<mockturtle::aig_network> tt_to_aig(
    mockturtle::names_view<mockturtle::aig_network>& ntk,
    const kitty::dynamic_truth_table& tt,
    const std::vector<mockturtle::signal<mockturtle::aig_network>>& inputs,
    utl::Logger* logger,
    unsigned var_index = 0)
{
  using aig_network = mockturtle::aig_network;
  using signal = mockturtle::signal<aig_network>;

  // Base cases: constant functions
  if (kitty::is_const0(tt)) {
    return ntk.get_constant(false);
  }
  if (kitty::is_const0(~tt)) {
    return ntk.get_constant(true);
  }

  const signal x = inputs[var_index];

  // Cofactors w.r.t. current variable index.
  const auto tt0 = kitty::cofactor0(tt, var_index);
  const auto tt1 = kitty::cofactor1(tt, var_index);

  // Recursively build sub-functions for cofactors.
  const signal f0 = tt_to_aig(ntk, tt0, inputs, logger, var_index + 1);
  const signal f1 = tt_to_aig(ntk, tt1, inputs, logger, var_index + 1);

  // Shannon decomposition:
  // f = (!x & f0) | (x & f1)
  const signal a0 = ntk.create_and(!x, f0);
  const signal a1 = ntk.create_and(x, f1);

  const signal tmp = ntk.create_and(!a0, !a1);

  return !tmp;
}

}  // anonymous namespace

template <unsigned MaxInputs>
mockturtle::names_view<mockturtle::aig_network>
LogicCut::BuildMappedMockturtleNetwork(
    mockturtle::tech_library<MaxInputs>& mockturtle_library,
    sta::dbNetwork* network,
    utl::Logger* logger)
{
  using aig_ntk = mockturtle::names_view<mockturtle::aig_network>;
  using signal = mockturtle::signal<aig_ntk>;
  using gate = mockturtle::gate;

  aig_ntk ntk;

  // Assert that constant-0 node exists.
  ntk.get_constant(false);

  // Build a map: cell name -> mockturtle gate*
  std::unordered_map<std::string, const gate*> gate_by_name;

  const auto gates = mockturtle_library.get_gates();

  for (const mockturtle::gate& g : gates) {
    gate_by_name.emplace(std::format("{}_{}", g.name, g.output_name), &g);
  }

  // Build a set of CUT instances for quick membership checks
  std::unordered_set<const sta::Instance*> cut_instances_set;
  for (const sta::Instance* inst : cut_instances_) {
    cut_instances_set.insert(inst);
  }

  // Net -> AIG signal memoization
  std::unordered_map<const sta::Net*, signal> net_to_signal;

  // Create PIs for primary_inputs_ (CUT boundary inputs)
  std::vector<sta::Net*> sorted_inputs(primary_inputs_.begin(),
                                       primary_inputs_.end());
  std::sort(sorted_inputs.begin(),
            sorted_inputs.end(),
            [network](sta::Net* a, sta::Net* b) {
              return network->id(a) < network->id(b);
            });

  for (sta::Net* net : sorted_inputs) {
    const signal pi = ntk.create_pi(network->name(net));
    net_to_signal[net] = pi;
  }

  // Recursive builder: given a STA net, return the corresponding AIG signal
  std::function<signal(sta::Net*, int)> build_net_signal
      = [&](sta::Net* net, int depth) -> signal {
    // Memoized?
    if (auto it = net_to_signal.find(net); it != net_to_signal.end()) {
      return it->second;
    }

    sta::PinSet* drivers = network->drivers(net);
    // Net driven from outside the cut handled by logic_extractor
    assert(drivers && !drivers->empty());

    if (drivers->size() != 1) {
      logger->error(utl::CUT,
                    51,
                    "Net {} has {} drivers; CUT expects exactly one.",
                    network->name(net),
                    drivers->size());
    }

    const sta::Pin* driver_pin = *drivers->begin();
    sta::Instance* driver_inst = network->instance(driver_pin);
    sta::LibertyCell* driver_cell = network->libertyCell(driver_inst);

    // Driver should not be sequential or outside the cut (handled by logic
    // extractor)
    assert(driver_cell && !driver_cell->hasSequentials()
           && cut_instances_set.contains(driver_inst));

    const std::string cell_name = driver_cell->name();
    const std::string pin_name = network->portName(driver_pin);

    auto gate_it = gate_by_name.find(std::format("{}_{}", cell_name, pin_name));
    if (gate_it == gate_by_name.end()) {
      logger->error(utl::CUT,
                    52,
                    "Cell {} not found in mockturtle technology library.",
                    cell_name);
    }

    const gate* g = gate_it->second;

    // Build fanin signals in the same order as GENLIB pins
    std::vector<signal> fanins;
    fanins.reserve(g->pins.size());

    for (const auto& p : g->pins) {
      sta::Pin* sta_pin = network->findPin(driver_inst, p.name.c_str());
      if (sta_pin == nullptr) {
        logger->error(utl::CUT,
                      53,
                      "Cannot find pin {} on instance {}.",
                      p.name,
                      network->name(driver_inst));
      }

      sta::Net* fanin_net = network->net(sta_pin);
      fanins.push_back(build_net_signal(fanin_net, depth + 1));
    }

    // Realize gate function as AIG fragment.
    const signal out_sig = tt_to_aig(ntk, g->function, fanins, logger);
    if (!ntk.has_name(out_sig)) {
      ntk.set_name(out_sig, network->name(net));
    }

    net_to_signal[net] = out_sig;
    return out_sig;
  };

  // Hook up primary outputs
  std::vector<sta::Net*> sorted_outputs(primary_outputs_.begin(),
                                        primary_outputs_.end());
  std::sort(sorted_outputs.begin(),
            sorted_outputs.end(),
            [network](sta::Net* a, sta::Net* b) {
              return network->id(a) < network->id(b);
            });

  for (sta::Net* net : sorted_outputs) {
    const signal s = build_net_signal(net, 0);
    ntk.create_po(s, network->name(net));
  }

  auto cleaned = mockturtle::cleanup_dangling(ntk, true, false);

  return cleaned;
}

}  // namespace cut
