// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "tst/fixture.h"

// ABC ships its own main() that wins over gtest_main when linked in.
// Provide our own to force the gtest entry point.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

class AbcTest : public tst::Fixture
{
 protected:
  void roundtrip(Graph& g, const std::string& commands = "&st")
  {
    abcRoundtrip(g, commands, getLogger());
    g.normalize();
  }
};

// Single AND gate: a & b
TEST_F(AbcTest, SingleAnd)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<And>(a, b);
  g.add<Output>("out", c);
  roundtrip(g);

  // Should still have an and gate
  const And* and_ = g.findOne<And>();
  ASSERT_NE(and_, nullptr);
  ASSERT_EQ(and_->outputWidth(), 1);
  Instance* in1 = g.resolve(and_->a().asNet()).first;
  Instance* in2 = g.resolve(and_->b().asNet()).first;
  EXPECT_NE(in1, in2);
  EXPECT_TRUE(in1->is<Input>());
  EXPECT_TRUE(in2->is<Input>());
}

// Single OR gate: a | b
TEST_F(AbcTest, SingleOr)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<Or>(a, b);
  g.add<Output>("out", c);
  roundtrip(g);

  const Or* or_ = g.findOne<Or>();
  ASSERT_NE(or_, nullptr);
  ASSERT_EQ(or_->outputWidth(), 1);
  Instance* in1 = g.resolve(or_->a().asNet()).first;
  Instance* in2 = g.resolve(or_->b().asNet()).first;
  EXPECT_NE(in1, in2);
  EXPECT_TRUE(in1->is<Input>());
  EXPECT_TRUE(in2->is<Input>());
}

// Andnot: a & ~b
TEST_F(AbcTest, SingleAndnot)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<Andnot>(a, b);
  g.add<Output>("out", c);
  roundtrip(g);

  const Andnot* andnot_ = g.findOne<Andnot>();
  ASSERT_NE(andnot_, nullptr);
  ASSERT_EQ(andnot_->outputWidth(), 1);
  Instance* in1 = g.resolve(andnot_->a().asNet()).first;
  Instance* in2 = g.resolve(andnot_->b().asNet()).first;
  EXPECT_NE(in1, in2);
  EXPECT_TRUE(in1->is<Input>());
  EXPECT_TRUE(in2->is<Input>());
}

// Identity: a passes through an AND with constant 1
// ABC should optimize to a direct connection
TEST_F(AbcTest, AndWithOne)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle one = Bundle::ones(1);
  Bundle c = g.add<And>(a, one);
  g.add<Output>("out", c);
  roundtrip(g);

  const Output* output_ = g.findOne<Output>();
  ASSERT_EQ(output_->value().width(), 1);
  EXPECT_TRUE(g.resolve(output_->value().asNet()).first->is<Input>());
}

// Constant zero: a & 0 = 0
TEST_F(AbcTest, AndWithZero)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle z = Bundle::zero(1);
  Bundle c = g.add<And>(a, z);
  g.add<Output>("out", c);
  roundtrip(g);

  g.assertNone<And>();
  const Output* output_ = g.findOne<Output>();
  ASSERT_EQ(output_->value().width(), 1);
  EXPECT_EQ(output_->value().asNet(), Net::zero());
}

// Two-level: (a & b) | (c & d)
TEST_F(AbcTest, TwoLevel)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle c = g.add<Input>("c", 1);
  Bundle d = g.add<Input>("d", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle cd = g.add<And>(c, d);
  Bundle out = g.add<Or>(ab, cd);
  g.add<Output>("out", out);
  roundtrip(g);

  const Or* or_ = g.findOne<Or>();
  ASSERT_EQ(or_->outputWidth(), 1);
  const Output* output_ = g.findOne<Output>();
  EXPECT_EQ(output_->name(), "out");
  EXPECT_EQ(g.resolve(output_->value().asNet()).first, or_);
}

// Inverter chain: Not(a) through AIG
TEST_F(AbcTest, NotGate)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle na = g.add<Not>(a);
  g.add<Output>("out", na);
  roundtrip(g);

  const Not* not_ = g.findOne<Not>();
  ASSERT_EQ(not_->outputWidth(), 1);
  Instance* in = g.resolve(not_->a().asNet()).first;
  EXPECT_TRUE(in->is<Input>());
}

// Multi-output: two separate outputs from the same inputs
TEST_F(AbcTest, MultiOutput)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle aorb = g.add<Or>(a, b);
  g.add<Output>("out_and", ab);
  g.add<Output>("out_or", aorb);
  roundtrip(g);

  auto outputs = g.collectOutputs();
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(outputs.count("out_and"), 1);
  EXPECT_EQ(outputs.count("out_or"), 1);
  const And* and_ = g.findOne<And>();
  EXPECT_EQ(and_->outputWidth(), 1);
  const Or* or_ = g.findOne<Or>();
  EXPECT_EQ(or_->outputWidth(), 1);
}

// Redundant logic: a & a = a, ABC should simplify
TEST_F(AbcTest, RedundantAnd)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle c = g.add<And>(a, a);
  g.add<Output>("out", c);
  roundtrip(g);

  g.assertNone<And>();
  const Output* output_ = g.findOne<Output>();
  ASSERT_EQ(output_->value().width(), 1);
  EXPECT_TRUE(g.resolve(output_->value().asNet()).first->is<Input>());
}

// a & ~a = 0, ABC should optimize to constant
TEST_F(AbcTest, Contradiction)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle c = g.add<Andnot>(a, a);  // a & ~a = 0
  g.add<Output>("out", c);
  roundtrip(g);

  g.assertNone<Andnot>();
  const Output* output_ = g.findOne<Output>();
  ASSERT_EQ(output_->value().width(), 1);
  EXPECT_EQ(output_->value().asNet(), Net::zero());
}

}  // namespace syn
