// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Tests for the bitblast pass: AIG rewriting rules (Brummayer-Biere)
// and structural hashing.

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/synthesis.h"

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

// Rule 11 (Substitution): a & ~(a & b) → a & ~b (= andnot)
TEST(BitblastTest, Rule11_Substitution)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle nab = g.add<Not>(ab);
  Bundle y = g.add<And>(a, nab);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Should simplify to andnot(a, b): one Andnot, no And, no Not.
  EXPECT_EQ(g.count<Andnot>(), 1);
  EXPECT_EQ(g.count<And>(), 0);
  EXPECT_EQ(g.count<Not>(), 0);
}

// Rule 9 (Idempotence): (a & b) & a → a & b
TEST(BitblastTest, Rule9_Idempotence)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle y = g.add<And>(ab, a);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Should simplify to a single And(a, b).
  EXPECT_EQ(g.count<And>(), 1);
}

// Rule 10 (Resolution): ~(a & b) & ~(a & ~b) → ~a
TEST(BitblastTest, Rule10_Resolution)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);

  Bundle ab = g.add<And>(a, b);
  Bundle nab = g.add<Not>(ab);
  Bundle anb = g.add<Andnot>(a, b);
  Bundle nanb = g.add<Not>(anb);
  Bundle y = g.add<And>(nab, nanb);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Should simplify to not(a): one Not, no And/Andnot/Or.
  EXPECT_EQ(g.count<Not>(), 1);
  EXPECT_EQ(g.count<And>(), 0);
  EXPECT_EQ(g.count<Andnot>(), 0);
  EXPECT_EQ(g.count<Or>(), 0);
}

// Rule 5 (Contradiction): (a & b) & ~a → 0
TEST(BitblastTest, Rule5_Contradiction)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle na = g.add<Not>(a);
  Bundle y = g.add<And>(ab, na);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Output should be constant zero — no gates at all.
  EXPECT_EQ(g.count<And>(), 0);
  EXPECT_EQ(g.count<Andnot>(), 0);
  EXPECT_EQ(g.count<Or>(), 0);
  EXPECT_EQ(g.count<Not>(), 0);
}

// Rule 7 (Subsumption): ~(a & b) & ~a → ~a
TEST(BitblastTest, Rule7_Subsumption)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  Bundle ab = g.add<And>(a, b);
  Bundle nab = g.add<Not>(ab);
  Bundle na = g.add<Not>(a);
  Bundle y = g.add<And>(nab, na);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Should simplify to not(a).
  EXPECT_EQ(g.count<Not>(), 1);
  EXPECT_EQ(g.count<And>(), 0);
}

// Structural hashing: two identical AND trees share the gate.
TEST(BitblastTest, StructuralHashing)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle b = g.add<Input>("b", 1);
  // Build (a & b) | (a & b) — should collapse to a & b.
  Bundle ab1 = g.add<And>(a, b);
  Bundle ab2 = g.add<And>(a, b);
  Bundle y = g.add<Or>(ab1, ab2);
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  // Should simplify to a single And — the Or is idempotent.
  EXPECT_EQ(g.count<And>(), 1);
  EXPECT_EQ(g.count<Or>(), 0);
}

TEST(BitblastTest, SignedMul7x9)
{
  Graph g;
  Bundle a = g.add<Input>("a", 7);
  Bundle b = g.add<Input>("b", 8);
  Bundle y = g.add<Mul>(a.signExtend(9), b.signExtend(9));
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  EXPECT_EQ(g.count<Mul>(), 0);
  int gates = g.count<And>() + g.count<Or>() + g.count<Andnot>();
  EXPECT_GT(gates, 100) << "Expected substantial AIG for 7x8 multiply, got "
                        << gates;
}

TEST(BitblastTest, SignedMul8x9)
{
  Graph g;
  Bundle a = g.add<Input>("a", 8);
  Bundle b = g.add<Input>("b", 9);
  Bundle y = g.add<Mul>(a.signExtend(10), b.signExtend(10));
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  EXPECT_EQ(g.count<Mul>(), 0);
  int gates = g.count<And>() + g.count<Or>() + g.count<Andnot>();
  EXPECT_GT(gates, 100) << "Expected substantial AIG for 7x8 multiply, got "
                        << gates;
}

TEST(BitblastTest, SignedMul9x9)
{
  Graph g;
  Bundle a = g.add<Input>("a", 9);
  Bundle b = g.add<Input>("b", 9);
  Bundle y = g.add<Mul>(a.signExtend(10), b.signExtend(10));
  g.add<Output>("y", y);

  bitblast(g);
  g.normalize();

  EXPECT_EQ(g.count<Mul>(), 0);
  int gates = g.count<And>() + g.count<Or>() + g.count<Andnot>();
  EXPECT_GT(gates, 100) << "Expected substantial AIG for 7x8 multiply, got "
                        << gates;
}

}  // namespace syn
