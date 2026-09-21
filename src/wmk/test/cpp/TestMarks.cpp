// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The keyed values of a mark are what embedding commits to and verification
// derives again.  These pin down the properties the scheme rests on: that
// they depend on the key, that they depend on nothing but the key and the
// names, and that naming a pair either way round changes nothing.

#include <array>
#include <cstdint>
#include <set>
#include <string>

#include "Marks.h"
#include "gtest/gtest.h"

namespace wmk {
namespace {

StageKey keyOf(std::uint8_t last)
{
  StageKey key{};
  key.back() = last;
  return key;
}

TEST(Marks, PairOrderIsLexicographicWhicheverWayItIsNamed)
{
  const OrderedPair expected{.first = "a", .second = "b"};
  EXPECT_EQ(orderPair("a", "b"), expected);
  EXPECT_EQ(orderPair("b", "a"), expected);
  EXPECT_EQ(orderPair("_10_", "_9_").first, "_10_");
}

TEST(Marks, PlacementBitDependsOnKeyAndNamesOnly)
{
  const OrderedPair pair = orderPair("cell_a", "cell_b");
  const int bit = placementTargetBit(keyOf(1), pair);
  EXPECT_TRUE(bit == 0 || bit == 1);
  EXPECT_EQ(placementTargetBit(keyOf(1), pair), bit);
  // Every key must be able to ask for either order, so across a handful of
  // keys both bits appear.
  std::set<int> bits;
  for (std::uint8_t k = 0; k < 16; ++k) {
    bits.insert(placementTargetBit(keyOf(k), pair));
  }
  EXPECT_EQ(bits.size(), 2);
  // Another pair under the same key is its own draw.
  std::set<int> other;
  for (int i = 0; i < 16; ++i) {
    other.insert(placementTargetBit(
        keyOf(1), orderPair("cell_" + std::to_string(i), "cell_z")));
  }
  EXPECT_EQ(other.size(), 2);
}

TEST(Marks, PlacementOrderIsKeyedPerTile)
{
  const OrderedPair pair = orderPair("cell_a", "cell_b");
  EXPECT_EQ(placementSortKey(keyOf(1), 0, 0, pair),
            placementSortKey(keyOf(1), 0, 0, pair));
  EXPECT_NE(placementSortKey(keyOf(1), 0, 0, pair),
            placementSortKey(keyOf(2), 0, 0, pair));
  EXPECT_NE(placementSortKey(keyOf(1), 0, 0, pair),
            placementSortKey(keyOf(1), 1, 0, pair));
  // The tile enters the order but not the bit, so a verifier that no longer
  // knows the tile can still derive the bit.
  EXPECT_EQ(placementTargetBit(keyOf(1), pair),
            placementTargetBit(keyOf(1), pair));
}

TEST(Marks, CtsTargetIsOneOfThePairAndDependsOnTheKey)
{
  const OrderedPair pair = orderPair("clkbuf_2", "clkbuf_1");
  EXPECT_EQ(ctsPairKey(pair), "clkbuf_1+clkbuf_2");
  std::set<std::string> targets;
  std::set<int> bits;
  for (std::uint8_t k = 0; k < 16; ++k) {
    const CtsTarget target = ctsTarget(keyOf(k), pair);
    EXPECT_TRUE(target.target == "clkbuf_1" || target.target == "clkbuf_2");
    EXPECT_NE(target.target, target.other);
    EXPECT_TRUE(target.other == "clkbuf_1" || target.other == "clkbuf_2");
    targets.insert(target.target);
    bits.insert(target.bit);
  }
  EXPECT_EQ(targets.size(), 2);
  EXPECT_EQ(bits.size(), 2);
  EXPECT_NE(ctsSortKey(keyOf(1), pair), ctsSortKey(keyOf(2), pair));
}

TEST(Marks, KeyedValuesAreStable)
{
  // Pinned so that a change to the derivation cannot slip by: it would
  // silently invalidate every claim file already written.
  const StageKey key = keyOf(4);
  EXPECT_EQ(placementTargetBit(key, orderPair("_101_", "_102_")), 1);
  EXPECT_EQ(placementTargetBit(key, orderPair("_101_", "_103_")), 0);
  const CtsTarget target = ctsTarget(key, orderPair("leaf_a", "leaf_b"));
  EXPECT_EQ(target.target, "leaf_b");
  EXPECT_EQ(target.bit, 1);
}

}  // namespace
}  // namespace wmk
