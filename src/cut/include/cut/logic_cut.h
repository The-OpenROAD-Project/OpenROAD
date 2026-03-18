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
#include "mockturtle/algorithms/decomposition.hpp"
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
    gate_by_name.emplace(fmt::format("{}_{}", g.name, g.output_name), &g);
  }

  // Build a set of CUT instances for quick membership checks
  std::unordered_set<const sta::Instance*> cut_instances_set;
  for (const sta::Instance* inst : cut_instances_) {
    cut_instances_set.insert(inst);
  }

  // Net -> AIG signal memoization
  std::unordered_map<const sta::Net*, signal> net_to_signal;

  // Create PIs for primary_inputs_ (CUT boundary inputs)
  for (sta::Net* net : primary_inputs_) {
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
    if (!drivers || drivers->empty()) {
      logger->error(utl::CUT, 55, "Net driven from outside the cut {}", network->name(net));
    }

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
    if (!driver_cell) {
      logger->error(utl::CUT, 54, "Driver pin not found: {}", network->name(driver_inst));
    }
    if (driver_cell->hasSequentials() || !cut_instances_set.contains(driver_inst)) {
      logger->error(utl::CUT, 55, "Invalid driver for: {}", network->name(driver_inst));
    }

    const std::string cell_name = driver_cell->name();
    const std::string pin_name = network->portName(driver_pin);

    auto gate_it = gate_by_name.find(fmt::format("{}_{}", cell_name, pin_name));
    if (gate_it == gate_by_name.end()) {
      logger->error(utl::CUT,
                    52,
                    "Cell {} not found in mockturtle technology library.",
                    cell_name);
    }

    const gate* g = gate_it->second;

    if (g->pins.size() != g->function.num_vars()) {
      logger->error(utl::CUT, 52, "Invalid pin count");
    }

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
    std::vector<uint32_t> vars(g->function.num_vars());
    std::iota(vars.begin(), vars.end(), 0u);

    const signal out_sig = mockturtle::shannon_decomposition(ntk, g->function, vars, fanins);
    if (!ntk.has_name(out_sig)) {
      ntk.set_name(out_sig, network->name(net));
    }

    net_to_signal[net] = out_sig;
    return out_sig;
  };

  // Hook up primary outputs
  for (sta::Net* net : primary_outputs_) {
    const signal s = build_net_signal(net, 0);
    ntk.create_po(s, network->name(net));
  }

  return ntk;
}

}  // namespace cut
