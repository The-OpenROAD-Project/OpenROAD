// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Tests for the liveness pass: 4-valued forward propagation with
// register/combinational replacement.

#include <cstdint>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "utl/Logger.h"

// ABC ships its own main() that wins over gtest_main; provide our own.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

void livenessOpt(Graph& g, utl::Logger* logger, bool replace_combinational);

// A Dff with init/reset/D all reaching only constant 0 fixpoints to 0
// and gets replaced with Net::zero(); the dependent Dff downstream
// likewise.
TEST(LivenessTest, DeadResetChain)
{
  Graph g;
  Bundle clk = g.add<Input>("clk", 1);
  // Dff1: D = self (held), init = 0, reset = clk, reset_value = 0.
  // (Reset can fire but its value is also 0, so Q stays at 0.)
  Bundle q1_placeholder = Bundle::undef(1);
  Bundle dff1 = g.add<Dff>(q1_placeholder,
                           ControlNet::pos(clk[0]),
                           ControlNet::zero(),
                           ControlNet::pos(clk[0]),
                           ControlNet::one(),
                           Const::zero(1),
                           Const::zero(1),
                           Const::undef(1));
  // Tie D back to Q (self-feedback) — represents "hold value".
  // We won't actually wire it; it's enough that data == undef which
  // joins with init=0 → 0.
  // Dff2 driven by Dff1; init = undef.
  Bundle dff2 = g.add<Dff>(dff1,
                           ControlNet::pos(clk[0]),
                           ControlNet::zero(),
                           ControlNet::zero(),
                           ControlNet::one(),
                           Const::undef(1),
                           Const::undef(1),
                           Const::undef(1));
  g.add<Output>("out", dff2);

  utl::Logger logger;
  livenessOpt(g, &logger, false);

  // Both registers should be DCE'd (their outputs forced to 0 → became
  // unused) and the output should drive a constant 0.
  g.assertNone<Dff>();
  EXPECT_EQ(g.findOne<Output>()->value().asNet(), Net::zero());
}

// A held register with all-undef init/reset/D fixpoints at U → replaced
// with Net::undef().
TEST(LivenessTest, UndefHeldRegister)
{
  Graph g;
  Bundle clk = g.add<Input>("clk", 1);
  Bundle dff = g.add<Dff>(Bundle::undef(1),
                          ControlNet::pos(clk[0]),
                          ControlNet::zero(),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(1),
                          Const::undef(1),
                          Const::undef(1));
  g.add<Output>("out", dff);

  utl::Logger logger;
  livenessOpt(g, &logger, false);

  g.assertNone<Dff>();
  EXPECT_EQ(g.findOne<Output>()->value().asNet(), Net::undef());
}

// A Dff fed by a primary input must remain — its Q reaches A.
TEST(LivenessTest, LiveRegister)
{
  Graph g;
  Bundle clk = g.add<Input>("clk", 1);
  Bundle d = g.add<Input>("d", 1);
  Bundle dff = g.add<Dff>(d,
                          ControlNet::pos(clk[0]),
                          ControlNet::zero(),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(1),
                          Const::undef(1),
                          Const::undef(1));
  g.add<Output>("out", dff);

  utl::Logger logger;
  livenessOpt(g, &logger, false);

  EXPECT_GT(g.count<Dff>(), 0);
}

// And of an input with constant 0 has output 0. Without -replace_comb
// the And stays. With -replace_comb, the And's bit is replaced with
// Net::zero() and the And gets DCE'd.
TEST(LivenessTest, AndWithZero_NoCombFlag)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<And>(a, z);
  g.add<Output>("out", c);

  utl::Logger logger;
  livenessOpt(g, &logger, /*replace_combinational=*/false);

  // The And instance must still be present without the flag (this pass
  // by default only replaces register-driven nets).
  EXPECT_GT(g.count<And>(), 0);
}

TEST(LivenessTest, AndWithZero_CombFlag)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<And>(a, z);
  g.add<Output>("out", c);

  utl::Logger logger;
  livenessOpt(g, &logger, /*replace_combinational=*/true);

  g.assertNone<And>();
  EXPECT_EQ(g.findOne<Output>()->value().asNet(), Net::zero());
}

// Mux with undefined selector and different arms evaluates to U
TEST(LivenessTest, MuxUndefSelector_CombFlag)
{
  Graph g;
  Bundle m = g.add<Mux>(Net::undef(), Net::one(), Net::zero());
  g.add<Output>("out", m);

  utl::Logger logger;
  livenessOpt(g, &logger, /*replace_combinational=*/true);
  g.normalize();
  g.assertNone<Mux>();
}

// UShr where every input bit is 0/U should yield all-zero output bits
// (faithful per-bit barrel shift).
TEST(LivenessTest, UShrFaithfulZero)
{
  Graph g;
  Bundle data = Bundle::zero(4);
  Bundle amt = g.add<Input>("amt", 2);
  Bundle r = g.add<UShr>(data, amt, /*stride=*/1);
  g.add<Output>("out", r);

  utl::Logger logger;
  livenessOpt(g, &logger, /*replace_combinational=*/true);

  // After replacement, the UShr should have been DCE'd and the output
  // should drive an all-zero bundle.
  g.assertNone<UShr>();
  const Bundle& v = g.findOne<Output>()->value();
  EXPECT_EQ(v.width(), 4u);
  for (uint32_t i = 0; i < v.width(); i++) {
    EXPECT_EQ(v[i], Net::zero());
  }
}

// Shl with all-zero data is over-approximated to A — the pass must not
// claim it can be replaced.
TEST(LivenessTest, ShlOverapproxKeepsCells)
{
  Graph g;
  Bundle data = Bundle::zero(4);
  Bundle amt = g.add<Input>("amt", 2);
  Bundle r = g.add<Shl>(data, amt, /*stride=*/1);
  g.add<Output>("out", r);

  utl::Logger logger;
  livenessOpt(g, &logger, /*replace_combinational=*/true);

  // Shl operands include the primary input `amt` (A), so output is A
  // and Shl stays. (Any input bit that is non-U gives output A under
  // the documented over-approximation.)
  EXPECT_GT(g.count<Shl>(), 0);
}

}  // namespace syn
