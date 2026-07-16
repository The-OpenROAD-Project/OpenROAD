// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <set>
#include <span>
#include <unordered_set>
#include <vector>

#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"

namespace abc {
struct kissat;
}

namespace syn {

class Solver
{
 public:
  Solver();
  ~Solver();

  int newVar();
  void addClause(std::span<const int> lits);
  void addClause(std::initializer_list<int> lits)
  {
    addClause(std::span<const int>(lits.begin(), lits.size()));
  }

  // Tseitin helpers: return the variable of the encoded result.
  int encodeAnd(int a, int b);
  int encodeOr(int a, int b);
  int encodeXor(int a, int b);
  int encodeMux(int sel, int a, int b);

  // Solve and return result.
  enum Result
  {
    Sat,
    Unsat,
    Unknown
  };
  Result solve();
  bool value(int lit);

 private:
  abc::kissat* solver_;
  int next_var_ = 1;  // kissat variables are 1-indexed
};

class TritModel
{
 public:
  TritModel(Solver& solver, const Graph& graph);

  TritModel(const TritModel&) = delete;
  TritModel& operator=(const TritModel&) = delete;

  // Get the SAT variable pair for a net.
  int valVar(Net net) const;
  int defVar(Net net) const;

  // Encode the logic cone from roots backward to support boundary.
  // Support nets are left as free variables. Aborts if the cone
  // reaches an Input/Dff/Other that is not in support.
  void encodeCone(BundleView support, BundleView roots);

  // After solve() returns Sat, query the model.
  Trit value(Net net) const;
  Const value(BundleView bundle) const;

  // Dump the modeled cone in IR format with assigned values.
  void dumpCone(std::ostream& os) const;

 private:
  void encodeInstance(const Instance* inst);
  void encodeInstanceBit(const Instance* inst, uint32_t bit);

  struct VarPair
  {
    int val = 0;
    int def = 0;
  };
  std::vector<VarPair> net_vars_;
  // Set (not vector) because sliceable instances walk per-bit and would
  // otherwise get re-inserted once per encoded bit. Membership-only — the
  // stable iteration order for dumpCone comes from graph_.forEachInstance.
  std::unordered_set<const Instance*> encoded_instances_;
  // Nets for the bits we actually encoded. For sliceable instances only
  // the specific walked bits are in here; for non-sliceable instances
  // every output bit is. dumpCone uses this to skip unmodeled bits
  // (their SAT vars are free, so their "values" are meaningless).
  std::set<Net> encoded_nets_;

  Solver& s_;
  const Graph& graph_;
};

}  // namespace syn
