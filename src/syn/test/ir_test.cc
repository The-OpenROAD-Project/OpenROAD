// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
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

struct NetTestAccess
{
  static Net make(NetTableId id) { return Net(id); }
  static NetTableId id(Net n) { return n.id_; }
};

TEST(GraphTest, ConstantIndices)
{
  Graph g;
  // TieLow at index 0, TieHigh at index 1, TieX at index 2.
  EXPECT_TRUE(g.resolve(Net::zero()).first->is<TieLow>());
  EXPECT_TRUE(g.resolve(Net::one()).first->is<TieHigh>());
  EXPECT_TRUE(g.resolve(Net::undef()).first->is<TieX>());
}

TEST(ValueTest, Empty)
{
  Bundle v;
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.width(), 0u);
}

TEST(ValueTest, Single)
{
  Bundle v(NetTestAccess::make(5));
  EXPECT_EQ(v.width(), 1u);
  EXPECT_EQ(NetTestAccess::id(v[0]), 5u);
}

TEST(ValueTest, Consecutive)
{
  Bundle v(NetTestAccess::make(10), 4);
  EXPECT_EQ(v.width(), 4u);
  EXPECT_EQ(NetTestAccess::id(v[0]), 10u);
  EXPECT_EQ(NetTestAccess::id(v[1]), 11u);
  EXPECT_EQ(NetTestAccess::id(v[2]), 12u);
  EXPECT_EQ(NetTestAccess::id(v[3]), 13u);
}

TEST(ValueTest, Zero)
{
  Bundle v = Bundle::zero(3);
  EXPECT_EQ(v.width(), 3u);
  for (uint32_t i = 0; i < 3; ++i) {
    EXPECT_EQ(v[i], Net::zero());
  }
}

TEST(ValueTest, Ones)
{
  Bundle v = Bundle::ones(2);
  EXPECT_EQ(v.width(), 2u);
  for (uint32_t i = 0; i < 2; ++i) {
    EXPECT_EQ(v[i], Net::one());
  }
}

TEST(ValueTest, Undef)
{
  Bundle v = Bundle::undef(2);
  EXPECT_EQ(v.width(), 2u);
  for (uint32_t i = 0; i < 2; ++i) {
    EXPECT_EQ(v[i], Net::undef());
  }
}

TEST(ValueTest, Slice)
{
  Bundle v(NetTestAccess::make(10), 4);
  Bundle s = v.slice(1, 2);
  EXPECT_EQ(s.width(), 2u);
  EXPECT_EQ(NetTestAccess::id(s[0]), 11u);
  EXPECT_EQ(NetTestAccess::id(s[1]), 12u);
}

TEST(ValueTest, FromVecConsecutive)
{
  std::vector<Net> nets = {
      NetTestAccess::make(3), NetTestAccess::make(4), NetTestAccess::make(5)};
  Bundle v = Bundle::fromVec(std::move(nets));
  EXPECT_EQ(v.width(), 3u);
  EXPECT_EQ(NetTestAccess::id(v[0]), 3u);
  EXPECT_EQ(NetTestAccess::id(v[1]), 4u);
  EXPECT_EQ(NetTestAccess::id(v[2]), 5u);
}

TEST(GraphTest, AddInputSingleBit)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  EXPECT_EQ(a.width(), 1u);
  // Single-bit input is heap-allocated (sizeof(Input) > kSlotSize): the
  // slot holds a placeholder pointing to the off-table Input instance.
  auto [inst, offset] = g.resolve(a[0]);
  EXPECT_FALSE(inst->isInline());
  EXPECT_TRUE(inst->is<Input>());
  EXPECT_EQ(offset, 0u);
}

TEST(GraphTest, AddInputMultiBit)
{
  Graph g;
  Bundle a = g.add<Input>("a", 8);
  EXPECT_EQ(a.width(), 8u);
  // Multi-bit: every output bit resolves to the same heap-allocated Input.
  for (uint32_t i = 0; i < 8; ++i) {
    auto [inst, offset] = g.resolve(a[i]);
    EXPECT_FALSE(inst->isInline());
    EXPECT_TRUE(inst->is<Input>());
    EXPECT_EQ(offset, i);
  }
}

TEST(GraphTest, PlaceholderInstance)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  // The first output bit's placeholder should point back to the Input instance.
  auto [inst0, off0] = g.resolve(a[0]);
  EXPECT_TRUE(inst0->is<Input>());
  EXPECT_EQ(off0, 0u);
  // Second bit shares the same instance at offset 1.
  auto [inst1, off1] = g.resolve(a[1]);
  EXPECT_EQ(inst1, inst0);
  EXPECT_EQ(off1, 1u);
}

TEST(GraphTest, ConsecutiveOutputBits)
{
  Graph g;
  Bundle a = g.add<Input>("a", 4);
  // Output bits should be at consecutive indices.
  for (uint32_t i = 1; i < a.width(); ++i) {
    EXPECT_EQ(NetTestAccess::id(a[i]), NetTestAccess::id(a[0]) + i);
  }
}

TEST(ValueViewTest, FromNet)
{
  Net n = NetTestAccess::make(42);
  BundleView vv(n);
  EXPECT_EQ(vv.width(), 1u);
  EXPECT_FALSE(vv.empty());
  EXPECT_EQ(NetTestAccess::id(vv[0]), 42u);
}

TEST(ValueViewTest, FromValue)
{
  Bundle v(NetTestAccess::make(10), 4);
  BundleView vv(v);
  EXPECT_EQ(vv.width(), 4u);
  EXPECT_FALSE(vv.empty());
  for (uint32_t i = 0; i < 4; ++i) {
    EXPECT_EQ(NetTestAccess::id(vv[i]), 10u + i);
  }
}

TEST(ValueViewTest, FromEmptyValue)
{
  Bundle v;
  BundleView vv(v);
  EXPECT_EQ(vv.width(), 0u);
  EXPECT_TRUE(vv.empty());
}

TEST(ValueViewTest, Slice)
{
  Bundle v(NetTestAccess::make(10), 4);
  BundleView vv(v);
  BundleView s = vv.slice(1, 2);
  EXPECT_EQ(s.width(), 2u);
  EXPECT_EQ(NetTestAccess::id(s[0]), 11u);
  EXPECT_EQ(NetTestAccess::id(s[1]), 12u);
}

TEST(ValueViewTest, SliceOfSlice)
{
  Bundle v(NetTestAccess::make(10), 8);
  BundleView s1 = BundleView(v).slice(2, 4);
  BundleView s2 = s1.slice(1, 2);
  EXPECT_EQ(s2.width(), 2u);
  EXPECT_EQ(NetTestAccess::id(s2[0]), 13u);
  EXPECT_EQ(NetTestAccess::id(s2[1]), 14u);
}

}  // namespace syn
