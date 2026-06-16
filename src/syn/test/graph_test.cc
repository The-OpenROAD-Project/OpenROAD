// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/Graph.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTableEntry.h"

// ABC (pulled in transitively via //src/syn/src/ir → TritModel) ships its
// own main() that wins over gtest_main. Provide our own to force the gtest
// entry point.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

struct GraphTestAccess
{
  static size_t tableSize(const Graph& g) { return g.tableSize(); }
  static Net netFromId(NetTableId id) { return Net(id); }
};

TEST(GraphTest, AddNotFineDispatch)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle na = g.add<Not>(a);
  EXPECT_EQ(na.width(), 1u);
  EXPECT_EQ(g.findOne<NotFine>()->outputWidth(), 1u);
}

TEST(GraphTest, AddNotWideDispatch)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle na = g.add<Not>(a);
  EXPECT_EQ(na.width(), 4u);
  EXPECT_EQ(g.findOne<NotWide>()->outputWidth(), 4u);
}

TEST(GraphTest, AddAndFineDispatch)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<And>(a, b);
  EXPECT_EQ(c.width(), 1u);
  EXPECT_EQ(g.findOne<AndFine>()->outputWidth(), 1u);
}

TEST(GraphTest, AddAndWideDispatch)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle c = g.add<And>(a, b);
  EXPECT_EQ(c.width(), 4u);
  EXPECT_EQ(g.findOne<AndWide>()->outputWidth(), 4u);
}

TEST(GraphTest, AddAdcWide)
{
  Graph g;
  Bundle a = g.add<Input>("a", 8);
  Bundle b = g.add<Input>("b", 8);
  Bundle sum = g.add<Adc>(a, b, Net::zero());
  EXPECT_EQ(sum.width(), 9u);
  EXPECT_EQ(g.findOne<AdcWide>()->outputWidth(), 9u);
}

TEST(GraphTest, AddOutput)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle na = g.add<Not>(a);
  Bundle out = g.add<Output>("out", na);
  EXPECT_EQ(out.width(), 1u);
  EXPECT_EQ(g.findOne<Output>()->name(), "out");
}

TEST(GraphTest, EqSingleBitOutput)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle eq = g.add<Eq>(a, b);
  EXPECT_EQ(eq.width(), 1u);
  EXPECT_EQ(g.findOne<Eq>()->outputWidth(), 1u);
}

TEST(GraphTest, MuxFineDispatch)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle sel = g.add<Input>("sel", 1);
  Bundle m = g.add<Mux>(sel[0], a, b);
  EXPECT_EQ(m.width(), 1u);
  EXPECT_EQ(g.findOne<MuxFine>()->outputWidth(), 1u);
}

TEST(GraphTest, BufferFineAccessor)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle buf = g.add<Buffer>(a);
  auto [inst, offset] = g.resolve(buf[0]);
  EXPECT_EQ(offset, 0u);
  auto& buffer = *static_cast<const Buffer*>(inst);
  BundleView va = buffer.a();
  EXPECT_EQ(va.width(), 1u);
  EXPECT_EQ(va[0], a[0]);
}

TEST(GraphTest, BufferWideAccessor)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle buf = g.add<Buffer>(a);
  auto [inst, offset] = g.resolve(buf[0]);
  EXPECT_EQ(offset, 0u);
  auto& buffer = *static_cast<const Buffer*>(inst);
  BundleView va = buffer.a();
  EXPECT_EQ(va.width(), 4u);
  for (uint32_t i = 0; i < 4; ++i) {
    EXPECT_EQ(va[i], a[i]);
  }
}

TEST(GraphTest, AndBaseAccessors)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<And>(a, b);
  auto [inst, offset] = g.resolve(c[0]);
  auto& and_inst = *static_cast<const And*>(inst);
  EXPECT_EQ(and_inst.a().width(), 1u);
  EXPECT_EQ(and_inst.a()[0], a[0]);
  EXPECT_EQ(and_inst.b().width(), 1u);
  EXPECT_EQ(and_inst.b()[0], b[0]);
}

TEST(GraphTest, AndWideBaseAccessors)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle c = g.add<And>(a, b);
  auto [inst, offset] = g.resolve(c[0]);
  EXPECT_EQ(offset, 0u);
  auto& and_inst = *static_cast<const And*>(inst);
  EXPECT_EQ(and_inst.a().width(), 4u);
  EXPECT_EQ(and_inst.b().width(), 4u);
  for (uint32_t i = 0; i < 4; ++i) {
    EXPECT_EQ(and_inst.a()[i], a[i]);
    EXPECT_EQ(and_inst.b()[i], b[i]);
  }
}

TEST(GraphTest, MuxBaseAccessors)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle sel = g.add<Input>("sel", 1);
  Bundle m = g.add<Mux>(sel[0], a, b);
  auto [inst, offset] = g.resolve(m[0]);
  auto& mux = *static_cast<const Mux*>(inst);
  EXPECT_EQ(mux.sel(), sel[0]);
  EXPECT_EQ(mux.a()[0], a[0]);
  EXPECT_EQ(mux.b()[0], b[0]);
}

TEST(GraphTest, AdcBaseAccessors)
{
  Graph g;
  Bundle a = g.add<Input>("a", 8);
  Bundle b = g.add<Input>("b", 8);
  Bundle sum = g.add<Adc>(a, b, Net::zero());
  auto [inst, offset] = g.resolve(sum[0]);
  EXPECT_EQ(offset, 0u);
  auto& adc = *static_cast<const Adc*>(inst);
  EXPECT_EQ(adc.a().width(), 8u);
  EXPECT_EQ(adc.b().width(), 8u);
  EXPECT_EQ(adc.cin(), Net::zero());
}

// ============================================================
// normalize() DCE tests
// ============================================================

TEST(GraphTest, NormalizeRemovesUnusedInstance)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle unused = g.add<And>(a, b);  // not connected to output
  Bundle na = g.add<Not>(a);
  g.add<Output>("out", na);
  (void) unused;

  size_t before = GraphTestAccess::tableSize(g);
  g.normalize();
  size_t after = GraphTestAccess::tableSize(g);

  // The And and input "b" are unreferenced → should shrink the table.
  EXPECT_LT(after, before);
}

TEST(GraphTest, NormalizePreservesOutput)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle na = g.add<Not>(a);
  g.add<Output>("out", na);

  size_t before = GraphTestAccess::tableSize(g);
  g.normalize();

  // Everything is reachable, so table size should not grow.
  EXPECT_LE(GraphTestAccess::tableSize(g), before);
}

TEST(GraphTest, NormalizePreservesDff)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle clk = g.add<Input>("clk", 1);
  g.add<Dff>(a,
             ControlNet::pos(clk[0]),
             ControlNet::zero(),
             ControlNet::zero(),
             ControlNet::zero(),
             Const::zero(4),
             Const::zero(4),
             Const::zero(4));
  // Dff has effects → kept even without Output.

  size_t before = GraphTestAccess::tableSize(g);
  g.normalize();
  // Nothing is unreachable, so the table shouldn't shrink much.
  EXPECT_LE(GraphTestAccess::tableSize(g), before);
}

TEST(GraphTest, NormalizeEmpty)
{
  Graph g;
  // No instances beyond constants.
  g.normalize();
  // Constants still there.
  EXPECT_GE(GraphTestAccess::tableSize(g), 3u);
}

TEST(GraphTest, NormalizePreservesConstants)
{
  Graph g;
  g.add<Output>("out", Bundle::zero(1));

  g.normalize();

  // Constants are always preserved at fixed indices.
  EXPECT_TRUE(g.resolve(Net::zero()).first->is<TieLow>());
  EXPECT_TRUE(g.resolve(Net::one()).first->is<TieHigh>());
  EXPECT_TRUE(g.resolve(Net::undef()).first->is<TieX>());
}

TEST(GraphTest, NormalizeTransitiveReachability)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle c = g.add<And>(a, b);
  Bundle d = g.add<Not>(c);
  g.add<Output>("out", d);
  // Unreferenced combinational gate — no path to an effects root, so
  // normalize should DCE it. (Inputs are preserved as port boundaries
  // even when unused, so an Input isn't a valid DCE witness.)
  Bundle unused = g.add<Or>(a, b);
  (void) unused;

  g.normalize();

  bool has_or = false;
  g.forEachInstance([&](const Instance* inst) {
    if (inst->is<Or>()) {
      has_or = true;
    }
  });
  EXPECT_FALSE(has_or);
}

// ============================================================
// normalize tests
// ============================================================

TEST(GraphTest, NormalizePreserveWide)
{
  // Create a combinational loop via forward reference.
  // Use wide instances (8-bit) with only partial bits used in the output
  // to exercise per-bit liveness and slicing.
  std::istringstream is(
      "%3:4 = input \"a\"\n"
      "%7:4 = not %3:4\n"
      "%11:0 = output \"out\" %7:4\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->dump(std::cout);
  g->normalize();

  std::ostringstream os;
  g->dump(os);
  EXPECT_EQ(os.str(),
            R"(%3:4 = input "a"
%7:0 = output "out" %8:4
%8:4 = not %3:4
)");
}

TEST(GraphTest, NormalizeWithCycle)
{
  // Create a combinational loop via forward reference.
  // Use wide instances (8-bit) with only partial bits used in the output
  // to exercise per-bit liveness and slicing.
  std::istringstream is(
      "%3:8 = input \"a\"\n"
      "%11:8 = and %3:8 %19:8\n"
      "%19:8 = or %11:8 %3:8\n"
      "%27:0 = output \"out\" %19+0:4\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->normalize();
  std::ostringstream os;
  g->dump(os);
  EXPECT_EQ(os.str(),
            R"(%3:8 = input "a"
%11:0 = output "out" [ %23 %20 %17 %14 ]
%12:1 = loop_breaker %14
%13:1 = and %3 %12
%14:1 = or %13 %3
%15:1 = loop_breaker %17
%16:1 = and %3+1:1 %15
%17:1 = or %16 %3+1:1
%18:1 = loop_breaker %20
%19:1 = and %3+2:1 %18
%20:1 = or %19 %3+2:1
%21:1 = loop_breaker %23
%22:1 = and %3+3:1 %21
%23:1 = or %22 %3+3:1
)");
}

TEST(GraphTest, NormalizeRemovesBuffers)
{
  // buf → not chain: normalize should eliminate the buffer.
  std::istringstream is(
      "%3:4 = input \"a\"\n"
      "%7:4 = buf %3:4\n"
      "%11:4 = not %7:4\n"
      "%15:0 = output \"out\" %11:4\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->normalize();
  g->assertNone<Buffer>();
}

TEST(GraphTest, NormalizeRemovesLoopBreakers)
{
  // A loop_breaker with no actual cycle should be eliminated.
  std::istringstream is(
      "%3:1 = input \"a\"\n"
      "%4:1 = loop_breaker %3\n"
      "%5:1 = not %4\n"
      "%6:0 = output \"out\" %5\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->normalize();
  g->assertNone<LoopBreaker>();
}

TEST(GraphTest, NormalizeRemovesUnusedSlices)
{
  // A loop_breaker with no actual cycle should be eliminated.
  std::istringstream is(
      R"(%3:8 = input "a"
      %11:0 = output "out" [ %16+0:2 ]
      %12:4 = xor %3:4 %3+4:4
      %16:2 = not %12:2
  )");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->normalize();
  EXPECT_EQ(g->findOne<Xor>()->outputWidth(), 2);
}

// A mapped flop (Other/Target with state) in a feedback loop
// should NOT produce a loop_breaker — the flop breaks the loop.
TEST(GraphTest, NormalizeNoLoopBreakerForMappedFlop)
{
  // Build: input "d" → Other("DFF") → q → Not → d_next → Other.D
  // The Other cell has an output Q and an input D forming a feedback
  // loop through the Not gate.  Since Other hasState(), normalize
  // should treat it like a Dff and not insert a loop_breaker.
  std::istringstream is(R"(
%3:1 = input "clk"
%4:1 = other "DFF" {
  input "D" = %5
  input "CLK" = %3
  %4:1 = output "Q"
}
%5:1 = not %4
%6:0 = output "out" %4
)");
  std::unique_ptr<Graph> g = Graph::parse(is);
  g->normalize();
  g->assertNone<LoopBreaker>();
}

// ============================================================
// replace() tests
// ============================================================
TEST(GraphTest, ReplaceSpanningMultipleInstances)
{
  // Replace a range that spans bits from two different heap instances.
  std::istringstream is(
      "%3:8 = input \"a\"\n"
      "%11:4 = not %3:4\n"
      "%15:4 = not %3+4:4\n"
      "%19:4 = not %11:4\n"
      "%23:4 = not %15:4\n"
      "%27:0 = output \"out\" [ %23:4 %19:4 ]\n");
  std::unique_ptr<Graph> g = Graph::parse(is);
  // Replace nets 21-24 (bits 2-3 of %19:4, bits 0-1 of %23:4)
  // with nets 5-8 (bits 2-5 of input "a").
  g->forceReplace(BundleView(GraphTestAccess::netFromId(21), 4),
                  BundleView(GraphTestAccess::netFromId(5), 4));
  g->checkConsistency();
}

}  // namespace syn
