// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/TritModel.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <span>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Const.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "utl/Logger.h"

namespace abc {
extern kissat* kissat_init();
extern void kissat_release(kissat* solver);
extern void kissat_add(kissat* solver, int lit);
extern int kissat_solve(kissat* solver);
extern int kissat_value(kissat* solver, int lit);
extern void kissat_set_terminate(kissat* solver,
                                 void* state,
                                 int (*terminate)(void* state));
}  // namespace abc

namespace syn {

Solver::Solver()
{
  solver_ = abc::kissat_init();
}

Solver::~Solver()
{
  abc::kissat_release(solver_);
}

int Solver::newVar()
{
  return next_var_++;
}

void Solver::addClause(std::span<const int> lits)
{
  for (int lit : lits) {
    abc::kissat_add(solver_, lit);
  }
  abc::kissat_add(solver_, 0);
}

int Solver::encodeAnd(int a, int b)
{
  int out = newVar();
  addClause({-a, -b, out});
  addClause({a, -out});
  addClause({b, -out});
  return out;
}

int Solver::encodeOr(int a, int b)
{
  int out = newVar();
  addClause({a, b, -out});
  addClause({-a, out});
  addClause({-b, out});
  return out;
}

int Solver::encodeXor(int a, int b)
{
  int out = newVar();
  addClause({-a, -b, -out});
  addClause({a, b, -out});
  addClause({a, -b, out});
  addClause({-a, b, out});
  return out;
}

int Solver::encodeMux(int sel, int a, int b)
{
  int out = newVar();
  // If sel=1, out must equal a:
  addClause({-sel, -a, out});
  addClause({-sel, a, -out});
  // If sel=0, out must equal b:
  addClause({sel, -b, out});
  addClause({sel, b, -out});
  // If a=b, out must equal them regardless of sel:
  addClause({-a, -b, out});
  addClause({a, b, -out});
  return out;
}

struct SolveDeadline
{
  std::chrono::steady_clock::time_point deadline;
};

static int checkDeadline(void* state)
{
  auto* d = static_cast<SolveDeadline*>(state);
  return std::chrono::steady_clock::now() >= d->deadline ? 1 : 0;
}

Solver::Result Solver::solve()
{
  SolveDeadline deadline{std::chrono::steady_clock::now()
                         + std::chrono::seconds(5)};
  abc::kissat_set_terminate(solver_, &deadline, checkDeadline);
  int r = abc::kissat_solve(solver_);
  abc::kissat_set_terminate(solver_, nullptr, nullptr);
  if (r == 10) {
    return Sat;
  }
  if (r == 20) {
    return Unsat;
  }
  return Unknown;
}

bool Solver::value(int lit)
{
  return abc::kissat_value(solver_, lit) > 0;
}

TritModel::TritModel(Solver& solver, const Graph& graph)
    : s_(solver), graph_(graph)
{
  // Allocate val/def variable pairs for all nets.
  size_t sz = graph_.tableSize();
  net_vars_.resize(sz);
  for (size_t i = 0; i < sz; i++) {
    net_vars_[i].val = s_.newVar();
    net_vars_[i].def = s_.newVar();
  }

  // Constrain constants.
  s_.addClause({-net_vars_[0].val});  // Net::zero(): val=0, def=1
  s_.addClause({net_vars_[0].def});
  s_.addClause({net_vars_[1].val});  // Net::one(): val=1, def=1
  s_.addClause({net_vars_[1].def});
  s_.addClause({-net_vars_[2].def});  // Net::undef(): def=0
}

int TritModel::valVar(Net net) const
{
  return net_vars_[Graph::netId(net)].val;
}

int TritModel::defVar(Net net) const
{
  return net_vars_[Graph::netId(net)].def;
}

Trit TritModel::value(Net net) const
{
  uint32_t id = Graph::netId(net);
  if (!s_.value(net_vars_[id].def)) {
    return Trit::Undef;
  }
  bool val = s_.value(net_vars_[id].val);
  return val ? Trit::One : Trit::Zero;
}

Const TritModel::value(BundleView bundle) const
{
  std::vector<Trit> ret;
  ret.reserve(bundle.width());
  for (auto bit : bundle) {
    ret.push_back(value(bit));
  }
  return Const::from(ret);
}

void TritModel::encodeCone(BundleView support, BundleView roots)
{
  // Build support set for O(1) lookup, and mark support bits as modeled
  // so dumpCone prints their (free-var) assignments and includes the
  // instances that own them.
  std::unordered_set<uint32_t> support_set;
  for (uint32_t i = 0; i < support.width(); i++) {
    Net bit = support[i];
    support_set.insert(Graph::netId(bit));
    if (!bit.isConst()) {
      encoded_nets_.insert(bit);
      encoded_instances_.insert(graph_.resolve(bit).first);
    }
  }

  std::unordered_set<uint32_t> visited;
  std::function<void(Net)> walk = [&](Net net) {
    if (net.isConst()) {
      return;
    }
    uint32_t id = Graph::netId(net);
    if (visited.contains(id)) {
      return;
    }
    if (support_set.contains(id)) {
      visited.insert(id);
      return;
    }

    auto [inst, offset] = graph_.resolve(net);

    if (inst->is<Input>() || inst->is<Dff>() || inst->is<Other>()
        || inst->is<Dangling>()) {
      std::stringstream ss;
      ss << "encodeCone: net ";
      writeNet(ss, graph_, net);
      ss << ", which is a combinational root, reached without reaching "
            "verification support ";
      ss << "(" << support_set.size() << " nets):";

      for (uint32_t s : support_set) {
        ss << " ";
        writeNet(ss, graph_, Graph::netFromId(s));
      }
      ss << "\nroots: ";
      writeValue(ss, graph_, roots);
      ss << "\n";

      // Add the instance to encoded_instances_ to enhance the dump
      encoded_instances_.insert(inst);
      dumpCone(ss);
      auto logger = graph_.logger();
      logger->reportLiteral(ss.str());
      logger->error(
          utl::SYN, 112, "TritModel cone encoding failed, see details above");
    }

    if (inst->isSliceable()) {
      // Only mark, encode, and walk the specific bit we need.
      visited.insert(id);
      encodeInstanceBit(inst, offset);
      encoded_instances_.insert(inst);
      encoded_nets_.insert(net);
      inst->visitSlice(offset, [&](Net fanin) { walk(fanin); });
    } else {
      // Mark all output bits of this instance as visited.
      BundleView out = graph_.output(inst);
      for (uint32_t i = 0; i < out.width(); i++) {
        visited.insert(Graph::netId(out[i]));
        encoded_nets_.insert(out[i]);
      }
      encodeInstance(inst);
      encoded_instances_.insert(inst);
      inst->visit([&](Net fanin) { walk(fanin); });
    }
  };

  for (uint32_t i = 0; i < roots.width(); i++) {
    walk(roots[i]);
  }
}

void TritModel::dumpCone(std::ostream& os) const
{
  for (const Instance* inst : encoded_instances_) {
    graph_.dumpInstance(os, inst);
    BundleView out = graph_.output(inst);
    os << "; cex ";
    // MSB-first, matching writeValue's constant formatting. '-' for
    // bits not in encoded_nets_ (unmodeled slots).
    for (int i = (int) out.width() - 1; i >= 0; i--) {
      if (!encoded_nets_.contains(out[i])) {
        os << '-';
        continue;
      }
      os << value(out[i]);
    }
    os << "\n";
  }
}

void TritModel::encodeInstanceBit(const Instance* inst, uint32_t bit)
{
  BundleView out = graph_.output(inst);
  int o_v = valVar(out[bit]), o_d = defVar(out[bit]);

  if (inst->is<Buffer>()) {
    auto* op = inst->as<Buffer>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    s_.addClause({-o_v, a_v});
    s_.addClause({o_v, -a_v});
    s_.addClause({-o_d, a_d});
    s_.addClause({o_d, -a_d});

  } else if (inst->is<Not>()) {
    auto* op = inst->as<Not>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    s_.addClause({-o_v, -a_v});
    s_.addClause({o_v, a_v});
    s_.addClause({-o_d, a_d});
    s_.addClause({o_d, -a_d});

  } else if (inst->is<And>()) {
    auto* op = inst->as<And>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    int b_v = valVar(op->b()[bit]), b_d = defVar(op->b()[bit]);
    s_.addClause({-a_v, -b_v, o_v});
    s_.addClause({a_v, -o_v});
    s_.addClause({b_v, -o_v});
    int both_def = s_.encodeAnd(a_d, b_d);
    int a_zero = s_.encodeAnd(a_d, -a_v);
    int b_zero = s_.encodeAnd(b_d, -b_v);
    int def_result = s_.encodeOr(both_def, s_.encodeOr(a_zero, b_zero));
    s_.addClause({-o_d, def_result});
    s_.addClause({o_d, -def_result});

  } else if (inst->is<Or>()) {
    auto* op = inst->as<Or>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    int b_v = valVar(op->b()[bit]), b_d = defVar(op->b()[bit]);
    s_.addClause({a_v, b_v, -o_v});
    s_.addClause({-a_v, o_v});
    s_.addClause({-b_v, o_v});
    int both_def = s_.encodeAnd(a_d, b_d);
    int a_one = s_.encodeAnd(a_d, a_v);
    int b_one = s_.encodeAnd(b_d, b_v);
    int def_result = s_.encodeOr(both_def, s_.encodeOr(a_one, b_one));
    s_.addClause({-o_d, def_result});
    s_.addClause({o_d, -def_result});

  } else if (inst->is<Andnot>()) {
    auto* op = inst->as<Andnot>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    int b_v = valVar(op->b()[bit]), b_d = defVar(op->b()[bit]);
    s_.addClause({-a_v, b_v, o_v});
    s_.addClause({a_v, -o_v});
    s_.addClause({-b_v, -o_v});
    int both_def = s_.encodeAnd(a_d, b_d);
    int a_zero = s_.encodeAnd(a_d, -a_v);
    int b_one = s_.encodeAnd(b_d, b_v);
    int def_result = s_.encodeOr(both_def, s_.encodeOr(a_zero, b_one));
    s_.addClause({-o_d, def_result});
    s_.addClause({o_d, -def_result});

  } else if (inst->is<Xor>()) {
    auto* op = inst->as<Xor>();
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    int b_v = valVar(op->b()[bit]), b_d = defVar(op->b()[bit]);
    s_.addClause({-a_v, -b_v, -o_v});
    s_.addClause({a_v, b_v, -o_v});
    s_.addClause({a_v, -b_v, o_v});
    s_.addClause({-a_v, b_v, o_v});
    s_.addClause({-o_d, a_d});
    s_.addClause({-o_d, b_d});
    s_.addClause({o_d, -a_d, -b_d});

  } else if (inst->is<Mux>()) {
    auto* op = inst->as<Mux>();
    int s_v = valVar(op->sel()), s_d = defVar(op->sel());
    int a_v = valVar(op->a()[bit]), a_d = defVar(op->a()[bit]);
    int b_v = valVar(op->b()[bit]), b_d = defVar(op->b()[bit]);
    int mux_val = s_.encodeMux(s_v, a_v, b_v);
    s_.addClause({-o_v, mux_val});
    s_.addClause({o_v, -mux_val});
    int sel_a = s_.encodeAnd(s_d, s_.encodeAnd(s_v, a_d));
    int sel_b = s_.encodeAnd(s_d, s_.encodeAnd(-s_v, b_d));
    int branches_eq
        = s_.encodeAnd(a_d, s_.encodeAnd(b_d, -s_.encodeXor(a_v, b_v)));
    int def_result = s_.encodeOr(sel_a, s_.encodeOr(sel_b, branches_eq));
    s_.addClause({-o_d, def_result});
    s_.addClause({o_d, -def_result});

  } else {
    // Not a sliceable type — shouldn't be called.
    std::abort();
  }
}

void TritModel::encodeInstance(const Instance* inst)
{
  if (inst->isSliceable()) {
    for (uint32_t i = 0; i < inst->outputWidth(); i++) {
      encodeInstanceBit(inst, i);
    }
  } else if (inst->is<Eq>()) {
    auto* op = inst->as<Eq>();
    BundleView out = graph_.output(inst);
    int o_v = valVar(out[0]), o_d = defVar(out[0]);

    // eq is 1 iff all bit pairs match, 0 if any definite mismatch.
    // Build: any_diff = OR of per-bit XORs (on values), considering def.
    // If both defined and values differ → definite mismatch → output = 0.
    // If all defined and all equal → output = 1.
    // Otherwise → output = X.
    uint32_t n = op->a().width();

    // per-bit: bit_diff[i] = (a_def & b_def & (a_val ^ b_val))
    //          bit_undef[i] = ~a_def | ~b_def
    int any_diff = 0;   // OR of definite mismatches
    int any_undef = 0;  // OR of undefined comparisons
    for (uint32_t i = 0; i < n; i++) {
      int a_v = valVar(op->a()[i]), a_d = defVar(op->a()[i]);
      int b_v = valVar(op->b()[i]), b_d = defVar(op->b()[i]);
      int both_def = s_.encodeAnd(a_d, b_d);
      int val_diff = s_.encodeXor(a_v, b_v);
      int bit_diff = s_.encodeAnd(both_def, val_diff);
      int bit_undef = s_.encodeOr(-a_d, -b_d);
      any_diff = (i == 0) ? bit_diff : s_.encodeOr(any_diff, bit_diff);
      any_undef = (i == 0) ? bit_undef : s_.encodeOr(any_undef, bit_undef);
    }
    // out_val: 0 if any_diff, 1 if ~any_diff & ~any_undef, X otherwise
    // Simplify: out_val = ~any_diff & ~any_undef
    // (When any_diff=1: val=0. When any_diff=0 & any_undef=0: val=1.
    //  When any_diff=0 & any_undef=1: val is free, but def=0.)
    int result_val = s_.encodeAnd(-any_diff, -any_undef);
    s_.addClause({-o_v, result_val});
    s_.addClause({o_v, -result_val});
    // out_def: defined when there's a definite mismatch (result=0) or
    // all pairs are defined (result=1). Undefined only when no mismatch
    // but some pair is undefined.
    // out_def = any_diff | ~any_undef
    int result_def = s_.encodeOr(any_diff, -any_undef);
    s_.addClause({-o_d, result_def});
    s_.addClause({o_d, -result_def});

  } else if (inst->is<Adc>()) {
    auto* op = inst->as<Adc>();
    BundleView out = graph_.output(inst);
    uint32_t n = op->a().width();
    // Ripple-carry adder: carry[0] = cin, sum[i] = a[i]^b[i]^carry[i],
    // carry[i+1] = (a[i]&b[i]) | (a[i]&carry[i]) | (b[i]&carry[i])
    int carry_v = valVar(op->cin()), carry_d = defVar(op->cin());
    for (uint32_t i = 0; i < n; i++) {
      int a_v = valVar(op->a()[i]), a_d = defVar(op->a()[i]);
      int b_v = valVar(op->b()[i]), b_d = defVar(op->b()[i]);
      int o_v = valVar(out[i]), o_d = defVar(out[i]);
      // sum = a ^ b ^ carry
      int ab_xor = s_.encodeXor(a_v, b_v);
      int sum_v = s_.encodeXor(ab_xor, carry_v);
      s_.addClause({-o_v, sum_v});
      s_.addClause({o_v, -sum_v});
      // sum_def = a_def & b_def & carry_def
      int sum_d = s_.encodeAnd(a_d, s_.encodeAnd(b_d, carry_d));
      s_.addClause({-o_d, sum_d});
      s_.addClause({o_d, -sum_d});
      // next carry = (a & b) | (a & carry) | (b & carry) = majority
      int ab = s_.encodeAnd(a_v, b_v);
      int ac = s_.encodeAnd(a_v, carry_v);
      int bc = s_.encodeAnd(b_v, carry_v);
      int next_carry_v = s_.encodeOr(ab, s_.encodeOr(ac, bc));
      int next_carry_d = s_.encodeAnd(a_d, s_.encodeAnd(b_d, carry_d));
      carry_v = next_carry_v;
      carry_d = next_carry_d;
    }
    // carry out
    if (out.width() > n) {
      int o_v = valVar(out[n]), o_d = defVar(out[n]);
      s_.addClause({-o_v, carry_v});
      s_.addClause({o_v, -carry_v});
      s_.addClause({-o_d, carry_d});
      s_.addClause({o_d, -carry_d});
    }

  } else if (inst->is<ULt>()) {
    auto* op = inst->as<ULt>();
    BundleView out = graph_.output(inst);
    int o_v = valVar(out[0]), o_d = defVar(out[0]);
    uint32_t n = op->a().width();
    // Unsigned less-than: scan LSB to MSB so that higher bits dominate.
    // running_lt = bit_lt | (bit_eq & prev_lt)
    int running_lt_v = s_.newVar();
    s_.addClause({-running_lt_v});  // start: not less than
    int all_def = s_.newVar();
    s_.addClause({all_def});  // start: all defined
    for (uint32_t i = 0; i < n; i++) {
      int a_v = valVar(op->a()[i]), a_d = defVar(op->a()[i]);
      int b_v = valVar(op->b()[i]), b_d = defVar(op->b()[i]);
      int bit_lt = s_.encodeAnd(-a_v, b_v);
      int bit_eq = -s_.encodeXor(a_v, b_v);
      int eq_and_lt = s_.encodeAnd(bit_eq, running_lt_v);
      running_lt_v = s_.encodeOr(bit_lt, eq_and_lt);
      all_def = s_.encodeAnd(all_def, s_.encodeAnd(a_d, b_d));
    }
    s_.addClause({-o_v, running_lt_v});
    s_.addClause({o_v, -running_lt_v});
    s_.addClause({-o_d, all_def});
    s_.addClause({o_d, -all_def});

  } else if (inst->is<SLt>()) {
    auto* op = inst->as<SLt>();
    BundleView out = graph_.output(inst);
    int o_v = valVar(out[0]), o_d = defVar(out[0]);
    uint32_t n = op->a().width();
    // Signed less-than: scan LSB to MSB with sign bit flipped.
    int running_lt_v = s_.newVar();
    s_.addClause({-running_lt_v});
    int all_def = s_.newVar();
    s_.addClause({all_def});
    for (uint32_t i = 0; i < n; i++) {
      int a_v = valVar(op->a()[i]), a_d = defVar(op->a()[i]);
      int b_v = valVar(op->b()[i]), b_d = defVar(op->b()[i]);
      // For MSB (sign bit): flip sense — negative (sign=1) is less than
      // positive
      int eff_a = (i == n - 1) ? -a_v : a_v;
      int eff_b = (i == n - 1) ? -b_v : b_v;
      int bit_lt = s_.encodeAnd(-eff_a, eff_b);
      int bit_eq = -s_.encodeXor(eff_a, eff_b);
      int eq_and_lt = s_.encodeAnd(bit_eq, running_lt_v);
      running_lt_v = s_.encodeOr(bit_lt, eq_and_lt);
      all_def = s_.encodeAnd(all_def, s_.encodeAnd(a_d, b_d));
    }
    s_.addClause({-o_v, running_lt_v});
    s_.addClause({o_v, -running_lt_v});
    s_.addClause({-o_d, all_def});
    s_.addClause({o_d, -all_def});

  } else if (inst->is<Shl>()) {
    auto* op = inst->as<Shl>();
    BundleView out = graph_.output(inst);
    uint32_t n = out.width();
    uint32_t shift_bits = op->b().width();
    uint32_t stride = op->stride();
    // Barrel shifter: for each shift amount bit, conditionally shift.
    // Start with input, then for each bit k of shift amount,
    // if shift[k]=1, shift left by stride * 2^k positions.
    // Use mux per stage.
    std::vector<int> cur_v(n), cur_d(n);
    for (uint32_t i = 0; i < n; i++) {
      if (i < op->a().width()) {
        cur_v[i] = valVar(op->a()[i]);
        cur_d[i] = defVar(op->a()[i]);
      } else {
        cur_v[i] = s_.newVar();
        s_.addClause({-cur_v[i]});  // zero-extend
        cur_d[i] = s_.newVar();
        s_.addClause({cur_d[i]});
      }
    }
    for (uint32_t k = 0; k < shift_bits; k++) {
      int s_v = valVar(op->b()[k]), s_d = defVar(op->b()[k]);
      uint32_t amount = stride * (1u << k);
      std::vector<int> next_v(n), next_d(n);
      for (uint32_t i = 0; i < n; i++) {
        int shifted_v, shifted_d;
        if (i >= amount) {
          shifted_v = cur_v[i - amount];
          shifted_d = cur_d[i - amount];
        } else {
          shifted_v = s_.newVar();
          s_.addClause({-shifted_v});  // zero
          shifted_d = s_.newVar();
          s_.addClause({shifted_d});  // defined
        }
        // mux: if s=1 use shifted, else use current
        next_v[i] = s_.encodeMux(s_v, shifted_v, cur_v[i]);
        // def: s must be defined for output to be defined
        int mux_d = s_.encodeMux(s_v, shifted_d, cur_d[i]);
        next_d[i] = s_.encodeAnd(s_d, mux_d);
      }
      cur_v = std::move(next_v);
      cur_d = std::move(next_d);
    }
    for (uint32_t i = 0; i < n; i++) {
      int o_v = valVar(out[i]), o_d = defVar(out[i]);
      s_.addClause({-o_v, cur_v[i]});
      s_.addClause({o_v, -cur_v[i]});
      s_.addClause({-o_d, cur_d[i]});
      s_.addClause({o_d, -cur_d[i]});
    }

  } else if (inst->is<UShr>() || inst->is<SShr>() || inst->is<XShr>()) {
    // Right shifts: similar barrel shifter but shifting right.
    // UShr fills with 0, SShr fills with sign bit, XShr fills with X.
    BundleView out = graph_.output(inst);
    auto extract = [](auto* op) {
      return std::tuple<BundleView, BundleView, uint32_t>{
          op->a(), op->b(), op->stride()};
    };
    auto [a_bv, b_bv, stride] = [&] {
      if (inst->is<UShr>()) {
        return extract(inst->as<UShr>());
      }
      if (inst->is<SShr>()) {
        return extract(inst->as<SShr>());
      }
      return extract(inst->as<XShr>());
    }();
    uint32_t n = out.width();
    uint32_t shift_bits = b_bv.width();

    // Fill value for vacated positions
    int fill_v, fill_d;
    if (inst->is<SShr>()) {
      fill_v = valVar(a_bv[a_bv.width() - 1]);  // sign bit
      fill_d = defVar(a_bv[a_bv.width() - 1]);
    } else if (inst->is<XShr>()) {
      fill_v = s_.newVar();  // free
      fill_d = s_.newVar();
      s_.addClause({-fill_d});  // undefined
    } else {
      fill_v = s_.newVar();
      s_.addClause({-fill_v});  // zero
      fill_d = s_.newVar();
      s_.addClause({fill_d});  // defined
    }

    std::vector<int> cur_v(n), cur_d(n);
    for (uint32_t i = 0; i < n; i++) {
      if (i < a_bv.width()) {
        cur_v[i] = valVar(a_bv[i]);
        cur_d[i] = defVar(a_bv[i]);
      } else {
        cur_v[i] = fill_v;
        cur_d[i] = fill_d;
      }
    }
    for (uint32_t k = 0; k < shift_bits; k++) {
      int s_v = valVar(b_bv[k]), s_d = defVar(b_bv[k]);
      uint32_t amount = stride * (1u << k);
      std::vector<int> next_v(n), next_d(n);
      for (uint32_t i = 0; i < n; i++) {
        int shifted_v, shifted_d;
        if (i + amount < n) {
          shifted_v = cur_v[i + amount];
          shifted_d = cur_d[i + amount];
        } else {
          shifted_v = fill_v;
          shifted_d = fill_d;
        }
        next_v[i] = s_.encodeMux(s_v, shifted_v, cur_v[i]);
        int mux_d = s_.encodeMux(s_v, shifted_d, cur_d[i]);
        next_d[i] = s_.encodeAnd(s_d, mux_d);
      }
      cur_v = std::move(next_v);
      cur_d = std::move(next_d);
    }
    for (uint32_t i = 0; i < n; i++) {
      int o_v = valVar(out[i]), o_d = defVar(out[i]);
      s_.addClause({-o_v, cur_v[i]});
      s_.addClause({o_v, -cur_v[i]});
      s_.addClause({-o_d, cur_d[i]});
      s_.addClause({o_d, -cur_d[i]});
    }

  } else if (inst->is<Mul>()) {
    // Multiplication: schoolbook shift-and-add.
    auto* op = inst->as<Mul>();
    BundleView out = graph_.output(inst);
    uint32_t n = out.width();
    uint32_t a_len = op->a().width();
    // Initialize accumulator to zero.
    std::vector<int> acc_v(n);
    for (uint32_t i = 0; i < n; i++) {
      acc_v[i] = s_.newVar();
      s_.addClause({-acc_v[i]});
    }

    int def = s_.newVar();
    s_.addClause({def});
    inst->visit([&](Net fanin) { def = s_.encodeAnd(def, defVar(fanin)); });
    for (uint32_t i = 0; i < n; i++) {
      s_.addClause({-def, defVar(out[i])});
      s_.addClause({def, -defVar(out[i])});
    }

    // For each bit of b, if b[k]=1, add a<<k to accumulator.
    for (uint32_t k = 0; k < op->b().width() && k < n; k++) {
      int b_v = valVar(op->b()[k]);
      int carry_v = s_.newVar();
      s_.addClause({-carry_v});
      for (uint32_t i = k; i < n; i++) {
        int pp_v;
        if (i - k < a_len) {
          pp_v = s_.encodeAnd(valVar(op->a()[i - k]), b_v);
        } else {
          pp_v = s_.newVar();
          s_.addClause({-pp_v});
        }
        int sum_xor = s_.encodeXor(acc_v[i], pp_v);
        int new_sum = s_.encodeXor(sum_xor, carry_v);
        int new_carry = s_.encodeOr(s_.encodeAnd(acc_v[i], pp_v),
                                    s_.encodeOr(s_.encodeAnd(acc_v[i], carry_v),
                                                s_.encodeAnd(pp_v, carry_v)));
        acc_v[i] = new_sum;
        carry_v = new_carry;
      }
    }
    for (uint32_t i = 0; i < n; i++) {
      int o_v = valVar(out[i]);
      s_.addClause({-o_v, acc_v[i]});
      s_.addClause({o_v, -acc_v[i]});
    }
  } else if (inst->is<Target>()) {
    auto* op = inst->as<Target>();
    sta::LibertyCell* cell = op->cell();
    if (!cell->hasSequentials()) {
      // Build port→index map for inputs, collect output ports.
      std::vector<sta::LibertyPort*> in_ports;
      std::vector<sta::LibertyPort*> out_ports;
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->isPwrGnd()) {
          continue;
        }
        if (port->direction()->isInput()) {
          in_ports.push_back(port);
        } else if (port->direction()->isOutput()) {
          out_ports.push_back(port);
        }
      }

      // Check all outputs have functions defined.
      bool all_have_func = true;
      for (auto* p : out_ports) {
        if (!p->function()) {
          all_have_func = false;
          break;
        }
      }

      if (all_have_func) {
        BundleView out = graph_.output(inst);
        const Bundle& inputs = op->inputs();

        // all_inputs_def = AND of all input def vars.
        // If any input is X, all outputs are X.
        int all_def;
        if (inputs.width() == 0) {
          all_def = s_.newVar();
          s_.addClause({all_def});
        } else {
          all_def = defVar(inputs[0]);
          for (uint32_t i = 1; i < inputs.width(); i++) {
            all_def = s_.encodeAnd(all_def, defVar(inputs[i]));
          }
        }

        // Map LibertyPort* → input index for FuncExpr evaluation.
        std::unordered_map<sta::LibertyPort*, int> port_to_idx;
        for (int i = 0; i < in_ports.size(); i++) {
          port_to_idx[in_ports[i]] = i;
        }

        // Recursive FuncExpr → SAT literal encoder.
        std::function<int(sta::FuncExpr*)> encodeFExpr
            = [&](sta::FuncExpr* e) -> int {
          using Op = sta::FuncExpr::Op;
          switch (e->op()) {
            case Op::port:
              return valVar(inputs[port_to_idx.at(e->port())]);
            case Op::not_:
              return -encodeFExpr(e->left());
            case Op::and_:
              return s_.encodeAnd(encodeFExpr(e->left()),
                                  encodeFExpr(e->right()));
            case Op::or_:
              return s_.encodeOr(encodeFExpr(e->left()),
                                 encodeFExpr(e->right()));
            case Op::xor_:
              return s_.encodeXor(encodeFExpr(e->left()),
                                  encodeFExpr(e->right()));
            case Op::one: {
              int v = s_.newVar();
              s_.addClause({v});
              return v;
            }
            case Op::zero: {
              int v = s_.newVar();
              s_.addClause({-v});
              return v;
            }
          }
          std::abort();
        };

        // Encode each output pin.
        int out_idx = 0;
        for (auto* port : out_ports) {
          int func_val = encodeFExpr(port->function());
          int o_v = valVar(out[out_idx]), o_d = defVar(out[out_idx]);
          s_.addClause({-o_v, func_val});
          s_.addClause({o_v, -func_val});
          s_.addClause({-o_d, all_def});
          s_.addClause({o_d, -all_def});
          out_idx++;
        }
      }
    }
    // Cells with sequentials or missing functions: left unconstrained.

  } else if (inst->is<UDiv>() || inst->is<UMod>()) {
    // Unsigned division/modulo: restoring division algorithm.
    // q * b + r = a, with 0 <= r < b (when b != 0).
    bool is_div = inst->is<UDiv>();
    BundleView a_bv = is_div ? inst->as<UDiv>()->a() : inst->as<UMod>()->a();
    BundleView b_bv = is_div ? inst->as<UDiv>()->b() : inst->as<UMod>()->b();
    BundleView out = graph_.output(inst);
    uint32_t n = a_bv.width();

    // Helper: zero constant variable.
    auto makeZero = [&]() {
      int v = s_.newVar();
      s_.addClause({-v});
      return v;
    };
    auto makeOne = [&]() {
      int v = s_.newVar();
      s_.addClause({v});
      return v;
    };

    // b as SAT variables (zero-extended to n bits).
    std::vector<int> b_v(n), b_d(n);
    for (uint32_t i = 0; i < n; i++) {
      b_v[i] = (i < b_bv.width()) ? valVar(b_bv[i]) : makeZero();
      b_d[i] = (i < b_bv.width()) ? defVar(b_bv[i]) : makeOne();
    }

    // Remainder R, initialized to 0.
    std::vector<int> r_v(n), r_d(n);
    for (uint32_t i = 0; i < n; i++) {
      r_v[i] = makeZero();
      r_d[i] = makeOne();
    }

    std::vector<int> q_v(n), q_d(n);

    // Restoring division: for each bit from MSB to LSB.
    for (int k = (int) n - 1; k >= 0; k--) {
      // Shift R left by 1, insert a[k] at bit 0.
      for (int i = (int) n - 1; i > 0; i--) {
        r_v[i] = r_v[i - 1];
        r_d[i] = r_d[i - 1];
      }
      r_v[0] = valVar(a_bv[k]);
      r_d[0] = defVar(a_bv[k]);

      // Subtractor: trial = R - b, borrow = (R < b).
      // Negated literals (-x) represent NOT(x) in Tseitin encoding.
      std::vector<int> trial_v(n), trial_d(n);
      int borrow_v = makeZero();
      int borrow_d = makeOne();
      for (uint32_t i = 0; i < n; i++) {
        trial_v[i] = s_.encodeXor(s_.encodeXor(r_v[i], b_v[i]), borrow_v);
        trial_d[i] = s_.encodeAnd(r_d[i], s_.encodeAnd(b_d[i], borrow_d));
        // borrow_out = (~R[i] & b[i]) | (~R[i] & borrow) | (b[i] & borrow)
        borrow_v = s_.encodeOr(s_.encodeAnd(-r_v[i], b_v[i]),
                               s_.encodeOr(s_.encodeAnd(-r_v[i], borrow_v),
                                           s_.encodeAnd(b_v[i], borrow_v)));
        borrow_d = s_.encodeAnd(r_d[i], s_.encodeAnd(b_d[i], borrow_d));
      }

      // q[k] = !borrow (R >= b), R = borrow ? R : trial.
      q_v[k] = -borrow_v;
      q_d[k] = borrow_d;
      for (uint32_t i = 0; i < n; i++) {
        r_v[i] = s_.encodeMux(borrow_v, r_v[i], trial_v[i]);
        r_d[i] = s_.encodeAnd(r_d[i], trial_d[i]);
      }
    }

    // Assign outputs.
    std::vector<int>& res_v = is_div ? q_v : r_v;
    std::vector<int>& res_d = is_div ? q_d : r_d;
    for (uint32_t i = 0; i < out.width(); i++) {
      int o_v = valVar(out[i]), o_d = defVar(out[i]);
      s_.addClause({-o_v, res_v[i]});
      s_.addClause({o_v, -res_v[i]});
      s_.addClause({-o_d, res_d[i]});
      s_.addClause({o_d, -res_d[i]});
    }
  }
  // SDivTrunc, SDivFloor, SModTrunc, SModFloor are not modeled —
  // their outputs are left unconstrained.
}

}  // namespace syn
