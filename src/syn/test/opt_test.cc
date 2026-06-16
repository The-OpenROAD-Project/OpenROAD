// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <sstream>

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

void constantFold(Graph& g);

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

}  // namespace syn
