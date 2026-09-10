// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include <cmath>
#include <cstdint>
#include <limits>

#include "dr/WatermarkCost.h"
#include "frBaseTypes.h"
#include "gtest/gtest.h"

namespace drt {
namespace {

TEST(WatermarkCost, RejectsUnrepresentableStrengths)
{
  EXPECT_FALSE(isValidWatermarkStrength(-1.0));
  EXPECT_FALSE(isValidWatermarkStrength(1e20));
  EXPECT_FALSE(
      isValidWatermarkStrength(std::numeric_limits<double>::infinity()));
  EXPECT_FALSE(
      isValidWatermarkStrength(std::numeric_limits<double>::quiet_NaN()));
  EXPECT_TRUE(isValidWatermarkStrength(0.0));
  EXPECT_TRUE(isValidWatermarkStrength(1.0));
  EXPECT_TRUE(isValidWatermarkStrength(100.0));
  EXPECT_TRUE(isValidWatermarkStrength(maxWatermarkStrength()));
  EXPECT_FALSE(isValidWatermarkStrength(std::nextafter(
      maxWatermarkStrength(), std::numeric_limits<float>::infinity())));
}

TEST(WatermarkCost, PreservesOrdinaryCosts)
{
  EXPECT_EQ(scaledWatermarkCost(2000, 0.0f), 0);
  EXPECT_EQ(scaledWatermarkCost(2000, 1.0f), 2000);
  EXPECT_EQ(scaledWatermarkCost(2000, 100.0f), 200000);
  EXPECT_EQ(scaledWatermarkCost(7, 0.5f), 3);
  EXPECT_EQ(addWatermarkCosts(200000, 1000), 201000);
}

TEST(WatermarkCost, LargeProductsSaturateBeforeIntegerConversion)
{
  const auto limit = std::numeric_limits<frCost>::max();
  EXPECT_EQ(scaledWatermarkCost(limit, 1.0f), limit);
  EXPECT_EQ(scaledWatermarkCost(limit, 100.0f), limit);
  EXPECT_EQ(scaledWatermarkCost(uint64_t{limit} * 2, 100.0f), limit);
  EXPECT_EQ(scaledWatermarkCost(2000, maxWatermarkStrength()), limit);
  EXPECT_EQ(scaledWatermarkCost(uint64_t{limit} * 2, 0.0f), 0);
}

TEST(WatermarkCost, SaturatedEdgesCannotWrapPathOrQueueCosts)
{
  const auto limit = std::numeric_limits<frCost>::max();
  const auto edge = scaledWatermarkCost(2000, maxWatermarkStrength());
  EXPECT_EQ(saturateWatermarkCost(uint64_t{edge} + 2000), limit);
  EXPECT_EQ(addWatermarkCosts(edge, 1), limit);
  EXPECT_EQ(addWatermarkCosts(edge, limit), limit);
  EXPECT_EQ(addWatermarkCosts(limit - 1, 1), limit);
}

TEST(WatermarkCost, IncreasingStrengthCannotDecreaseEdgeCost)
{
  frCost previous = 0;
  for (const float strength :
       {0.0f, 1.0f, 100.0f, 1e6f, maxWatermarkStrength()}) {
    const auto cost = scaledWatermarkCost(50000, strength);
    EXPECT_GE(cost, previous);
    previous = cost;
  }
}

}  // namespace
}  // namespace drt
