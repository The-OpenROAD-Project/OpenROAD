// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "flow/combinational_mapper_npn.h"
#include "flow/target_index.h"
#include "sta/Delay.hh"

namespace sta {
class dbNetwork;
class Net;
class Instance;
class LibertyCell;
class Pin;
class RiseFall;
class Scene;
class Sta;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace odb {
class dbNet;
class dbInst;
}  // namespace odb

namespace syn::acd {

// A possibly-multi-output truth table over an arbitrary set of integer
// variable IDs.  `values` and `dontcares` are laid out so that the index for
// output `o` at minterm `m` is `o * (1 << vars.size()) + m`.
struct TruthTable
{
  std::vector<int> vars;
  int noutputs = 1;
  std::vector<bool> values;
  std::vector<bool> dontcares;

  int nminterms() const { return 1 << (int) vars.size(); }
  int totalSize() const { return noutputs * nminterms(); }

  // Swap variable positions `a` and `b` in the truth table representation,
  // applying the same permutation to every output.
  void swapVars(int a, int b);

  // Permute the variable list to `new_vars` (a permutation of the current
  // `vars`).  Rewrites `values` and `dontcares` accordingly.
  void changeVars(const std::vector<int>& new_vars);

  std::optional<int> findUnsupportedVar() const;
  std::optional<int> outputPassthrough(int o) const;

  // Drop unsupported variables.  Reduces |vars| and contracts the value
  // tables.  Variable IDs of the surviving vars are preserved.
  void shrinkToSupport();

  bool operator==(const TruthTable&) const = default;
  bool hasDontcares() const;
};

// The timing objective is given to resynthesis as a collection of arcs
// going from cut entry point to cut exit point with each arc being assigned
// a propagation delay budget.
//
// When processing a candidate network in a forward pass, the remaining
// delay budget from the current node to one of the cut exit points can,
// up to a sign, be viewed as an arrival time. The below `ArrivalSet` and
// `NodeArrivals` classes provide such 'arrival time' view over the budgets.
class ArrivalSet
{
 public:
  float& atExit(int out, const sta::RiseFall* out_rf);
  float atExit(int out, const sta::RiseFall* out_rf) const;
  void mergeMax(const ArrivalSet& other);
  ArrivalSet plus(float delay) const;
  float maxEntry() const;

  static constexpr float kNegInf = -std::numeric_limits<float>::infinity();

 private:
  // Minus budget remaining on arcs going to first input rise, first input fall,
  // second output rise, second output fall
  std::array<float, 4> v = {kNegInf, kNegInf, kNegInf, kNegInf};
};

class NodeArrivals
{
 public:
  ArrivalSet& atTransition(const sta::RiseFall* rf);
  const ArrivalSet& atTransition(const sta::RiseFall* rf) const;

  // Assuming the current node is the exit point `idx`, what is the slack?
  float exitSlack(int idx) const;
  float maxEntry() const;

 private:
  // Each node has a rise and fall arrival set
  ArrivalSet rise, fall;
};

struct SynthesisProblem
{
  TruthTable function;

  struct PrimaryInput
  {
    NodeArrivals arrivals;
  };
  struct PrimaryOutput
  {
    float external_load = 0;
    bool critical = false;
    double criticality = 0.0;
  };

  std::vector<PrimaryInput> inputs;
  std::vector<PrimaryOutput> outputs;

  SynthesisProblem() = default;
  SynthesisProblem(TruthTable f)
      : function(std::move(f)),
        inputs(function.vars.size()),
        outputs((size_t) function.noutputs)
  {
  }
};

using Cost = double;
constexpr Cost kInfCost = std::numeric_limits<Cost>::max() / 200.0;

constexpr int kMaxBoundVars = 6;

// Describes a cell matching a given truth table up to a permutation
// of its inputs
struct CellMatch
{
  sta::LibertyPort* driver = nullptr;
  std::array<int, 6> perm = {-1, -1, -1, -1, -1, -1};
  int arity = 0;
  Cost area = 0;
};

struct DelayEstimationParameters
{
  float nand_delay;
  float fixed_slews[2];
  const sta::Scene* corner;
};

// Cell matches keyed by truth table.  Holding on to one across `synthesize`
// calls is what saves redoing the NPN work for functions already seen.
class MatchCache
{
 public:
  MatchCache(utl::Logger* logger, const cm::TargetIndex& index, int max_arity);

  // Finds cell matching `tt` up to a permutation of inputs
  std::optional<CellMatch> match(Truth6 tt, int arity);

  // As `match`, but free to pick any fill of the don't-care positions
  std::optional<CellMatch> matchDC(Truth6 care_tt, Truth6 care_mask, int arity);

  Cost minAreaForWidth(int w) const
  {
    if (w < 0 || w >= (int) min_area_for_width_.size()) {
      return kInfCost;
    }
    return min_area_for_width_[w];
  }

  sta::LibertyCell* inverter() const { return index_.inverter; }
  sta::LibertyCell* nand2() const { return nand2_; }

 private:
  // Max don't-care bits the DC-aware cell matcher will enumerate fills for
  // (2^n).
  static const int kMaxDcFill = 8;

  sta::LibertyCell* nand2_;
  const cm::TargetIndex& index_;
  std::map<std::pair<Truth6, int>, std::optional<CellMatch>> cache_;

  // Area lower bound among cells of given arity
  std::vector<Cost> min_area_for_width_;
};

struct GateNode
{
  sta::LibertyPort* driver_port = nullptr;
  // The pin this gate was read off of, when the network describes existing
  // logic.  Null for networks `synthesize` produces: those gates have no
  // counterpart in the netlist (yet).
  const sta::Pin* driver_pin = nullptr;
  std::vector<std::pair<bool, int>> fanins;
};

struct GateNetwork
{
  int ninputs = 0;
  std::vector<GateNode> nodes;  // topologically ordered
  // For each output: (is_primary_input, index).  is_primary_input=true means
  // the output is directly the primary input at `index`; false means it is
  // the node at `index` in `nodes`.
  std::vector<std::pair<bool, int>> outs;
};

// Synthesize `problem.function` into a gate network
bool synthesize(const SynthesisProblem& problem,
                MatchCache& mc,
                const DelayEstimationParameters& dparams,
                utl::Logger* logger,
                GateNetwork& out,
                double budget,
                bool allow_lateral,
                long long* explore_calls,
                float effort);

// The (possibly multi-output) truth table `net` realizes, read back off each
// node's Liberty function.  Used to check a network against the function it was
// meant to compute.
TruthTable evaluateFunction(const GateNetwork& net);

NodeArrivals outputArrival(const sta::LibertyPort* out_port,
                           const std::vector<NodeArrivals>& input_pin_arrivals,
                           float load,
                           const sta::Scene* corner,
                           const float fixed_slew[2]);

float avgInputCap(const sta::LibertyCell* cell);
float findDelayLowerBound(const sta::Scene* corner,
                          const sta::LibertyCell* cell,
                          const float fixed_slew[2]);

float networkSlack(utl::Logger* logger,
                   const acd::SynthesisProblem& problem,
                   const acd::GateNetwork& net,
                   const sta::Scene* corner,
                   const float fixed_slew[2]);

void populateCutTiming(sta::dbNetwork* network,
                       sta::Sta* sta,
                       acd::SynthesisProblem& problem,
                       const std::span<sta::Net*> roots,
                       const std::span<sta::Net*> leaves,
                       const std::unordered_set<sta::Instance*>& cone);

}  // namespace syn::acd
