// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <iterator>
#include <set>
#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "syn/lir/Op.h"

namespace syn::test {
namespace {

using ::syn::lir::hasSignedFlag;
using ::syn::lir::isLoopPair;
using ::syn::lir::isReserved;
using ::syn::lir::isSignSensitive;
using ::syn::lir::isStateIn;
using ::syn::lir::isStateOut;
using ::syn::lir::isStorageOut;
using ::syn::lir::isTriState;
using ::syn::lir::kNumOps;
using ::syn::lir::Op;
using ::syn::lir::opName;

TEST(OpTest, EveryOpHasAUniqueName)
{
  std::set<std::string> names;
  for (int i = 0; i < kNumOps; ++i) {
    const std::string_view name = opName(static_cast<Op>(i));
    ASSERT_FALSE(name.empty()) << "op " << i;
    EXPECT_NE(name, "?") << "op " << i;
    names.insert(std::string(name));
  }
  EXPECT_EQ(std::ssize(names), kNumOps);
  EXPECT_EQ(opName(Op::kNumOps), "?");
  EXPECT_EQ(opName(Op::kAnd), "and");
  EXPECT_EQ(opName(Op::kRegOut), "reg_out");
  EXPECT_EQ(opName(Op::kCell), "cell");
  EXPECT_EQ(opName(Op::kTribuf), "tribuf");
  EXPECT_EQ(opName(Op::kTriJoin), "tri_join");
}

TEST(OpTest, StateClassification)
{
  EXPECT_TRUE(isStateOut(Op::kRegOut));
  EXPECT_TRUE(isStateOut(Op::kLoopOut));
  EXPECT_FALSE(isStateOut(Op::kRegIn));
  EXPECT_TRUE(isStateIn(Op::kRegIn));
  EXPECT_FALSE(isStateIn(Op::kAnd));
  // A loop pair cuts paths but stores nothing.
  EXPECT_TRUE(isStorageOut(Op::kRegOut));
  EXPECT_TRUE(isStorageOut(Op::kMemOut));
  EXPECT_FALSE(isStorageOut(Op::kLoopOut));
  EXPECT_FALSE(isStorageOut(Op::kRegIn));
  EXPECT_TRUE(isLoopPair(Op::kLoopOut));
  EXPECT_TRUE(isLoopPair(Op::kLoopIn));
  EXPECT_FALSE(isLoopPair(Op::kRegOut));
  EXPECT_TRUE(isReserved(Op::kMemRead));
  EXPECT_FALSE(isReserved(Op::kRegOut));
  EXPECT_TRUE(isTriState(Op::kTribuf));
  EXPECT_TRUE(isTriState(Op::kTriJoin));
  EXPECT_FALSE(isTriState(Op::kMux));
}

TEST(OpTest, SignedFlagClassification)
{
  int sensitive = 0;
  for (int i = 0; i < kNumOps; ++i) {
    const auto op = static_cast<Op>(i);
    sensitive += isSignSensitive(op) ? 1 : 0;
    // Every sign-sensitive kind may carry the flag.
    EXPECT_TRUE(!isSignSensitive(op) || hasSignedFlag(op)) << opName(op);
  }
  EXPECT_EQ(sensitive, 7);  // Cast, Lt, Div, Mod, Mul, Shr, Pow
  EXPECT_TRUE(isSignSensitive(Op::kShr));
  EXPECT_FALSE(isSignSensitive(Op::kShl));
  EXPECT_FALSE(isSignSensitive(Op::kPi));
  EXPECT_TRUE(hasSignedFlag(Op::kPi));
  EXPECT_TRUE(hasSignedFlag(Op::kRegOut));
  EXPECT_TRUE(hasSignedFlag(Op::kLoopIn));
  EXPECT_FALSE(hasSignedFlag(Op::kPo));
  EXPECT_FALSE(hasSignedFlag(Op::kAdd));
  EXPECT_FALSE(hasSignedFlag(Op::kAnd));
}

}  // namespace
}  // namespace syn::test
