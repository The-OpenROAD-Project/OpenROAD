// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <utility>
#include <vector>

#include "sta/Liberty.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn {

namespace {

// State assigned to each net by liveness analysis
//
// We perform 4-valued liveness propagation: starting from registers at U and
// primary inputs and blackboxes at A, propagate to fixed point through the
// netlist and replace registers (and optionally any combinational driver) whose
// abstract value is provably never A with a constant tie.
enum State
{
  U = 0,     // can be X
  Zero = 1,  // can be X or 0
  One = 2,   // can be X or 1
  A = 3      // can be anything
};

State NOT(State a)
{
  switch (a) {
    case U:
      return U;
    case Zero:
      return One;
    case One:
      return Zero;
    case A:
      return A;
  }
  std::abort();
}

State AND(State a, State b)
{
  bool can_be_1 = (a & One) && (b & One);
  bool can_be_0 = (a & Zero) || (b & Zero);
  return State((can_be_1 ? One : U) | (can_be_0 ? Zero : U));
}

State OR(State a, State b)
{
  return NOT(AND(NOT(a), NOT(b)));
}

State ANDNOT(State a, State b)
{
  return AND(a, NOT(b));
}

State XOR(State a, State b)
{
  bool can_be_00 = (a & Zero) && (b & Zero);
  bool can_be_01 = (a & Zero) && (b & One);
  bool can_be_10 = (a & One) && (b & Zero);
  bool can_be_11 = (a & One) && (b & One);

  bool can_be_0 = can_be_00 || can_be_11;
  bool can_be_1 = can_be_01 || can_be_10;

  return State((can_be_0 ? Zero : U) | (can_be_1 ? One : U));
}

// Mux convention: sel=1 selects a, sel=0 selects b
State MUX(State s, State a, State b)
{
  bool s0 = s & Zero;
  bool s1 = s & One;

  bool can_be_0
      = (s1 && (a & Zero)) || (s0 && (b & Zero)) || ((a & Zero) && (b & Zero));
  bool can_be_1
      = (s1 && (a & One)) || (s0 && (b & One)) || ((a & One) && (b & One));

  return State((can_be_0 ? Zero : U) | (can_be_1 ? One : U));
}

class LivenessAnalysis
{
 public:
  LivenessAnalysis(const Graph& g) : g_(g), values_(g.tableSize(), U)
  {
    assert(g.tableSize() >= 3);
    values_[0] = Zero;
    values_[1] = One;
    values_[2] = U;

    g.forEachInstance([&](const Instance* inst) {
      if (inst->is<Input>() || inst->is<Dangling>() || inst->is<Other>()
          || inst->is<Target>()) {
        BundleView out = g.output(inst);
        for (uint32_t i = 0; i < out.width(); i++) {
          values_[Graph::netId(out[i])] = A;
        }
      }
    });
  }

  State valueOf(Net n) const { return values_[Graph::netId(n)]; }

  State valueAt(BundleView bv, uint32_t i) const
  {
    if (i >= bv.width()) {
      return U;
    }
    return valueOf(bv[i]);
  }

  void run()
  {
    while (stepOnce()) {
    }
  }

  // Returns true if any value changed.
  bool stepOnce()
  {
    bool changed = false;
    g_.forEachInstance(
        [&](const Instance* inst) { computeOutputs(inst, changed); });
    return changed;
  }

  State operator[](Net n) const { return valueOf(n); }

 private:
  // Update outputs of `inst` based on the current values_ snapshot.
  void computeOutputs(const Instance* inst, bool& changed)
  {
    // Skip instances with no output bits, instances whose outputs are
    // boundary-pinned (Input, Dangling), and the constant tie-offs.
    if (inst->is<Input>() || inst->is<Dangling>() || inst->is<Other>()
        || inst->is<Target>() || inst->is<TieLow>() || inst->is<TieHigh>()
        || inst->is<TieX>()) {
      return;
    }

    BundleView out = g_.output(inst);
    if (out.width() == 0) {
      return;
    }

    auto setBit = [&](uint32_t i, State v) {
      uint32_t id = Graph::netId(out[i]);
      if (values_[id] != v) {
        values_[id] = v;
        changed = true;
      }
    };

    if (auto* op = inst->try_as<Buffer>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, valueOf(op->a()[i]));
      }
      return;
    }
    if (auto* op = inst->try_as<Not>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, NOT(valueOf(op->a()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<And>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, AND(valueOf(op->a()[i]), valueOf(op->b()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<Or>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, OR(valueOf(op->a()[i]), valueOf(op->b()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<Andnot>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, ANDNOT(valueOf(op->a()[i]), valueOf(op->b()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<Xor>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, XOR(valueOf(op->a()[i]), valueOf(op->b()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<Mux>()) {
      State s = valueOf(op->sel());
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, MUX(s, valueOf(op->a()[i]), valueOf(op->b()[i])));
      }
      return;
    }
    if (auto* op = inst->try_as<LoopBreaker>()) {
      for (uint32_t i = 0; i < out.width(); i++) {
        setBit(i, valueOf(op->a()[i]));
      }
      return;
    }
    if (inst->is<UShr>() || inst->is<XShr>() || inst->is<SShr>()) {
      computeShr(inst, out, setBit);
      return;
    }
    if (auto* dff = inst->try_as<Dff>()) {
      computeDff(dff, out, setBit);
      return;
    }

    // Over-approximated: Shl, Adc, Mul, UDiv, UMod, SDiv*, SMod*, Eq,
    // ULt, SLt
    State merged = inputJoin(inst);
    State output_val = (merged == U) ? U : A;
    for (uint32_t i = 0; i < out.width(); i++) {
      setBit(i, output_val);
    }
  }

  State inputJoin(const Instance* inst) const
  {
    State r = U;
    inst->visit([&](Net n) { r = State(r | valueOf(n)); });
    return r;
  }

  template <typename SetFn>
  void computeShr(const Instance* inst, BundleView out, SetFn&& setBit)
  {
    auto [a_bv, b_bv, stride]
        = [&]() -> std::tuple<BundleView, BundleView, uint32_t> {
      if (auto* s = inst->try_as<UShr>()) {
        return {s->a(), s->b(), s->stride()};
      }
      if (auto* s = inst->try_as<XShr>()) {
        return {s->a(), s->b(), s->stride()};
      }
      const auto* s = inst->as<SShr>();
      return {s->a(), s->b(), s->stride()};
    }();
    uint32_t n = out.width();
    uint32_t shift_bits = b_bv.width();

    State fill;
    if (inst->is<UShr>()) {
      fill = Zero;
    } else if (inst->is<XShr>()) {
      fill = U;
    } else {
      // SShr: replicate the sign bit.
      fill = (a_bv.width() > 0) ? valueOf(a_bv[a_bv.width() - 1]) : U;
    }

    std::vector<State> cur(n);
    for (uint32_t i = 0; i < n; i++) {
      cur[i] = (i < a_bv.width()) ? valueOf(a_bv[i]) : fill;
    }

    for (uint32_t k = 0; k < shift_bits; k++) {
      State s = valueOf(b_bv[k]);
      uint32_t amount = stride * (1u << k);
      std::vector<State> next(n);
      for (uint32_t i = 0; i < n; i++) {
        State shifted = ((uint64_t) i + amount < n) ? cur[i + amount] : fill;
        next[i] = MUX(s, shifted, cur[i]);
      }
      cur = std::move(next);
    }

    for (uint32_t i = 0; i < n; i++) {
      setBit(i, cur[i]);
    }
  }

  template <typename SetFn>
  void computeDff(const Dff* dff, BundleView out, SetFn&& setBit)
  {
    bool reset_can_fire = !dff->reset().isAlways(false);
    bool clear_can_fire = !dff->clear().isAlways(false);

    for (uint32_t i = 0; i < out.width(); i++) {
      State v = valueOf(out[i]);  // monotonic safety net (old Q)
      v = State(v | valueAt(dff->data(), i));
      v = State(v | valueOf(Net(dff->initValue()[i])));
      if (reset_can_fire) {
        v = State(v | valueOf(Net(dff->resetValue()[i])));
      }
      if (clear_can_fire) {
        v = State(v | valueOf(Net(dff->clearValue()[i])));
      }
      setBit(i, v);
    }
  }

  const Graph& g_;
  std::vector<State> values_;
};

Net netForState(State v)
{
  switch (v) {
    case U:
      return Net::undef();
    case Zero:
      return Net::zero();
    case One:
      return Net::one();
    default:
      return Net::sentinel();
  }
}

}  // namespace

void livenessOpt(Graph& g, utl::Logger* logger, bool replace_combinational)
{
  g.normalize();

  LivenessAnalysis analysis(g);
  analysis.run();

  struct Replacement
  {
    Net target;
    Net replacement;
  };
  std::vector<Replacement> reg_repls;
  std::vector<Replacement> comb_repls;

  g.forEachInstance([&](const Instance* inst) {
    if (inst->is<Input>() || inst->is<Dangling>() || inst->is<Output>()
        || inst->is<Name>() || inst->is<TieLow>() || inst->is<TieHigh>()
        || inst->is<TieX>() || inst->is<Other>()) {
      return;
    }

    bool is_register = inst->is<Dff>();
    if (auto* t = inst->try_as<Target>()) {
      if (t->cell() && t->cell()->hasSequentials()) {
        is_register = true;
      }
    }
    if (!is_register && !replace_combinational) {
      return;
    }

    BundleView out = g.output(inst);
    for (uint32_t i = 0; i < out.width(); i++) {
      Net n = out[i];
      State v = analysis.valueOf(n);
      if (v == A) {
        continue;
      }
      Net rep = netForState(v);
      if (rep.isSentinel() || n == rep) {
        continue;
      }
      Replacement r{.target = n, .replacement = rep};
      if (is_register) {
        reg_repls.push_back(r);
      } else {
        comb_repls.push_back(r);
      }
    }
  });

  for (const auto& r : reg_repls) {
    g.forceReplace(BundleView(r.target), BundleView(r.replacement));
  }
  for (const auto& r : comb_repls) {
    g.forceReplace(BundleView(r.target), BundleView(r.replacement));
  }

  if (!reg_repls.empty() || !comb_repls.empty()) {
    g.normalize();
  }

  logger->info(utl::SYN,
               56,
               "liveness: replaced {} register bits, {} combinational bits",
               reg_repls.size(),
               comb_repls.size());
}

}  // namespace syn
