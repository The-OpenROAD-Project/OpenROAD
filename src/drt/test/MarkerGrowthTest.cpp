// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "dr/MarkerGrowth.h"
#include "gtest/gtest.h"

namespace drt {
namespace {

TEST(MarkerGrowth, LimitIsTenTimesTheInitialMarkersPlusSlack)
{
  EXPECT_EQ(markerGrowthLimit(0), 20);
  EXPECT_EQ(markerGrowthLimit(1), 30);
  EXPECT_EQ(markerGrowthLimit(5), 70);
}

TEST(MarkerGrowth, ChecksEveryPeriodOfRoutedNets)
{
  EXPECT_FALSE(isMarkerGrowthCheck(0));
  EXPECT_FALSE(isMarkerGrowthCheck(1));
  EXPECT_FALSE(isMarkerGrowthCheck(kMarkerGrowthCheckPeriod - 1));
  EXPECT_TRUE(isMarkerGrowthCheck(kMarkerGrowthCheckPeriod));
  EXPECT_FALSE(isMarkerGrowthCheck(kMarkerGrowthCheckPeriod + 1));
  EXPECT_TRUE(isMarkerGrowthCheck(2 * kMarkerGrowthCheckPeriod));
}

}  // namespace
}  // namespace drt
