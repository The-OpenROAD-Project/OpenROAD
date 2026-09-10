// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"

// ABC (pulled in transitively via //src/syn/src/ir → TritModel) ships its
// own main() that wins over gtest_main. Provide our own to force the gtest
// entry point.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

static std::string dumpToString(Graph* g)
{
  std::ostringstream os;
  g->dump(os);
  return os.str();
}

static std::string roundTrip(Graph* g)
{
  std::string dump1 = dumpToString(g);
  std::istringstream is(dump1);
  std::unique_ptr<Graph> g2 = Graph::parse(is);
  return dumpToString(g2.get());
}

TEST(ParseTest, RoundTrip)
{
  Graph g;

  // input / output
  Bundle a = g.add<Input>("a", 4);
  Bundle b = g.add<Input>("b", 4);
  Bundle clk = g.add<Input>("clk", 1);
  Bundle sel = g.add<Input>("sel", 1);

  // dangling (input without a name)
  Bundle dangling = g.add<Dangling>(3);

  // buf, not
  Bundle buf = g.add<Buffer>(a);
  Bundle na = g.add<Not>(b);

  // and, or, andnot, xor
  Bundle c = g.add<And>(a, b);
  Bundle d = g.add<Or>(a, b);
  Bundle e = g.add<Andnot>(a, b);
  Bundle f = g.add<Xor>(a, b);

  // mux
  Bundle m = g.add<Mux>(sel[0], a, b);

  // adc
  Bundle sum = g.add<Adc>(a, b, Net::zero());

  // comparisons
  Bundle eq = g.add<Eq>(a, b);
  Bundle ult = g.add<ULt>(a, b);
  Bundle slt = g.add<SLt>(a, b);

  // shifts
  Bundle sh = g.add<Shl>(a, b, 1);
  Bundle ushr = g.add<UShr>(a, b, 1);
  Bundle sshr = g.add<SShr>(a, b, 1);
  Bundle xshr = g.add<XShr>(a, b, 1);

  // arithmetic
  Bundle mul = g.add<Mul>(a, b);
  Bundle udiv = g.add<UDiv>(a, b);
  Bundle umod = g.add<UMod>(a, b);
  Bundle sdivt = g.add<SDivTrunc>(a, b);
  Bundle sdivf = g.add<SDivFloor>(a, b);
  Bundle smodt = g.add<SModTrunc>(a, b);
  Bundle smodf = g.add<SModFloor>(a, b);

  // dff with control signals
  Bundle dff = g.add<Dff>(a,
                          ControlNet::pos(clk[0]),
                          ControlNet::zero(),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(4),
                          Const::undef(4),
                          Const::undef(4));

  // name (non-tentative and tentative)
  g.add<Name>("wire_a", a);
  g.add<Name>("dbg_b", b, 0, 4, true, true);

  // other cell
  std::vector<Other::Port> ports;
  ports.push_back({"A", Other::Port::kInput, 4, Bundle(a)});
  ports.push_back({"Y", Other::Port::kOutput, 2, Bundle()});
  g.add<Other>("CUSTOM_CELL", std::move(ports));

  // output
  g.add<Output>("out", c);

  // Suppress unused warnings.
  (void) buf;
  (void) na;
  (void) d;
  (void) e;
  (void) f;
  (void) m;
  (void) sum;
  (void) eq;
  (void) ult;
  (void) slt;
  (void) sh;
  (void) ushr;
  (void) sshr;
  (void) xshr;
  (void) mul;
  (void) udiv;
  (void) umod;
  (void) sdivt;
  (void) sdivf;
  (void) smodt;
  (void) smodf;
  (void) dff;
  (void) dangling;

  std::string dump1 = dumpToString(&g);
  std::string dump2 = roundTrip(&g);
  EXPECT_EQ(dump1, dump2);
}

// Dumps from the slang frontend contain gaps in the net-ID space because
// Name / intermediate instances leave unused slots. The parser must honor
// the declared %N IDs so net references continue to resolve correctly.
TEST(ParseTest, NonConsecutiveIds)
{
  // %3..%5 intentionally unused; input "a" starts at %6 (occupies %6..%9),
  // input "b" at %10 (%10..%13), the and at %15 (%15..%18), output at %14.
  std::string src
      = "%6:4 = input \"a\"\n"
        "%10:4 = input \"b\"\n"
        "%14:0 = output \"y\" %15:4\n"
        "%15:4 = and %6:4 %10:4\n";
  std::istringstream is(src);
  std::unique_ptr<Graph> g = Graph::parse(is);

  const Input* in_a = nullptr;
  const Input* in_b = nullptr;
  const And* and_inst = nullptr;
  const Output* out = nullptr;
  g->forEachInstance([&](const Instance* inst) {
    if (auto* i = inst->try_as<Input>()) {
      if (i->name() == "a") {
        in_a = i;
      } else if (i->name() == "b") {
        in_b = i;
      }
    } else if (auto* a = inst->try_as<And>()) {
      and_inst = a;
    } else if (auto* o = inst->try_as<Output>()) {
      out = o;
    }
  });
  ASSERT_NE(in_a, nullptr);
  ASSERT_NE(in_b, nullptr);
  ASSERT_NE(and_inst, nullptr);
  ASSERT_NE(out, nullptr);

  // The and's inputs must be the two Input outputs, and the Output's value
  // bundle must be the and's outputs.
  BundleView a_out = g->output(in_a);
  BundleView b_out = g->output(in_b);
  for (uint32_t i = 0; i < 4; i++) {
    EXPECT_EQ(and_inst->a()[i], a_out[i])
        << "and.a[" << i << "] should reference input a";
    EXPECT_EQ(and_inst->b()[i], b_out[i])
        << "and.b[" << i << "] should reference input b";
  }
  BundleView and_out = g->output(and_inst);
  for (uint32_t i = 0; i < 4; i++) {
    EXPECT_EQ(out->value()[i], and_out[i])
        << "output.value[" << i << "] should reference and output";
  }
}

TEST(ParseTest, RejectsDeclaredWidthMismatch)
{
  std::istringstream is(
      "%3:8 = input \"a\"\n"
      "%11:8 = input \"b\"\n"
      "%19:16 = mul %3:8 %11:8\n");
  EXPECT_THROW({ (void) Graph::parse(is); }, std::runtime_error);
}

}  // namespace syn
