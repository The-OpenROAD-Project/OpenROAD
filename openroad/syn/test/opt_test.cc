// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

#include "equivalence_check.h"
#include "flow/constant_fold.h"
#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/TritModel.h"

// ABC (pulled in transitively via //src/syn/src/ir → TritModel) ships its
// own main() that wins over gtest_main. Provide our own to force the gtest
// entry point.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

TEST(OptTest, AndZero)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<And>(a, z);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<And>();
}

TEST(OptTest, AndOne)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle one = Bundle::ones(1);
  Bundle c = g.add<And>(a, one);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<And>();
}

TEST(OptTest, OrOne)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle one = Bundle::ones(1);
  Bundle c = g.add<Or>(a, one);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<Or>();
}

TEST(OptTest, OrZero)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<Or>(a, z);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<Or>();
}

TEST(OptTest, MuxConstSel)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle m = g.add<Mux>(Net::one(), a, b);
  g.add<Output>("out", m);
  constantFold(g);
  g.normalize();
  g.assertNone<Mux>();
}

TEST(OptTest, MuxSameArms)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle sel = g.add<Input>("sel", 1);
  Bundle m = g.add<Mux>(sel[0], a, a);
  g.add<Output>("out", m);
  constantFold(g);
  g.normalize();
  g.assertNone<Mux>();
}

TEST(OptTest, NotConst)
{
  Graph g;
  Bundle z = Bundle::zero(1);
  Bundle n = g.add<Not>(z);
  g.add<Output>("out", n);
  constantFold(g);
  g.normalize();
  g.assertNone<Not>();
}

TEST(OptTest, XorZero)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<Xor>(a, z);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<Xor>();
}

TEST(OptTest, XorSelf)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle c = g.add<Xor>(a, a);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<Xor>();
}

TEST(OptTest, Chained)
{
  // and(1, or(0, input)) should fold to just input.
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle one = Bundle::ones(1);
  Bundle o = g.add<Or>(z, a);
  Bundle c = g.add<And>(one, o);
  g.add<Output>("out", c);
  constantFold(g);
  g.normalize();
  g.assertNone<And>();
  g.assertNone<Or>();
}

TEST(OptTest, EqConst)
{
  // eq(0101, 0101) = 1
  std::istringstream is(
      "%3:4 = input \"a\"\n"
      "%7:1 = eq 0101 0101\n"
      "%8:0 = output \"out\" %7\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  constantFold(*g);
  g->normalize();
  g->assertNone<Eq>();
}

TEST(OptTest, PreservesDff)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle clk = g.add<Input>("clk", 1);
  Bundle output = g.add<Dff>(a,
                             ControlNet::pos(clk[0]),
                             ControlNet::zero(),
                             ControlNet::zero(),
                             ControlNet::zero(),
                             Const::zero(4),
                             Const::zero(4),
                             Const::zero(4));
  g.add<Output>("q", output);
  constantFold(g);
  g.findOne<Dff>();
}

TEST(OptTest, AdcEqualSummandsBreaksCarryChain)
{
  // A carry chain with equal summands at a middle stage can be cut there:
  // the sum bit at that stage is just the carry in and the carry out is
  // the shared summand. The single full-width adder should become two
  // shorter adders, one below and one above the cut.
  const uint32_t width = 8;
  const uint32_t stage = 4;

  auto build = [&]() {
    auto g = std::make_unique<Graph>();
    Bundle a = g->add<Input>("a", width);
    Bundle b = g->add<Input>("b", width);
    Bundle cin = g->add<Input>("cin", 1);
    b.mutableNet(stage) = a[stage];
    Bundle sum = g->add<Adc>(a, b, cin[0]);
    g->add<Output>("sum", sum);
    return g;
  };

  std::unique_ptr<Graph> gold = build();
  std::unique_ptr<Graph> gate = build();

  constantFold(*gate);
  gate->normalize();

  EXPECT_EQ(gate->count<Adc>(), 2);
  std::vector<uint32_t> widths;
  gate->forEachInstance([&](const Instance* inst) {
    if (auto* adc = inst->try_as<Adc>()) {
      widths.push_back(adc->a().width());
    }
  });
  std::sort(widths.begin(), widths.end());
  EXPECT_EQ(widths, (std::vector<uint32_t>{width - stage - 1, stage}));

  EXPECT_TRUE(equivalenceCheck(
      *gold, *gate, /*inputsDefined=*/true, /*allowRefinement=*/false));
}

TEST(OptTest, AdcCarryInEqualsSummand)
{
  // When the carry in to a stage equals one of that stage's summands, the
  // sum bit is the other summand and the carry passes through unchanged.
  // Here a[0] is also the primary carry in, so the first stage disappears
  // and the remaining adder shrinks by one bit.
  const uint32_t width = 8;

  auto build = [&]() {
    auto g = std::make_unique<Graph>();
    Bundle a = g->add<Input>("a", width);
    Bundle b = g->add<Input>("b", width);
    Bundle sum = g->add<Adc>(a, b, a[0]);
    g->add<Output>("sum", sum);
    return g;
  };

  std::unique_ptr<Graph> gold = build();
  std::unique_ptr<Graph> gate = build();

  constantFold(*gate);
  gate->normalize();

  EXPECT_EQ(gate->count<Adc>(), 1);
  EXPECT_EQ(gate->findOne<Adc>()->a().width(), width - 1);

  EXPECT_TRUE(equivalenceCheck(
      *gold, *gate, /*inputsDefined=*/true, /*allowRefinement=*/false));
}

}  // namespace syn
