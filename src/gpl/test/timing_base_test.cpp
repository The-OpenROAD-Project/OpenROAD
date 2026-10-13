// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>

#include "gtest/gtest.h"
#include "src/gpl/src/timingBase.h"

namespace gpl {
namespace {

constexpr float kTolerance = 1e-5f;

TEST(SlackRampAnchorIndex, AnchorsOnPercentileOfTheList)
{
  EXPECT_EQ(slackRampAnchorIndex(1000), 1000 * kSlackMinPercentile / 100);
  EXPECT_EQ(slackRampAnchorIndex(150), 150 * kSlackMinPercentile / 100);
}

// Below 100 / kSlackMinPercentile nets the percentile floors to zero, the
// pre-existing behavior of anchoring on the single worst net.
TEST(SlackRampAnchorIndex, ShortListsAnchorOnTheWorstNet)
{
  EXPECT_EQ(slackRampAnchorIndex(1), 0u);
  EXPECT_EQ(slackRampAnchorIndex(100 / kSlackMinPercentile - 1), 0u);
}

// Must stay a valid subscript even if kSlackMinPercentile is raised to 100.
TEST(SlackRampAnchorIndex, StaysInRange)
{
  for (size_t net_count = 1; net_count <= 1000; ++net_count) {
    EXPECT_LT(slackRampAnchorIndex(net_count), net_count);
  }
}

TEST(TimingNetWeight, RampEndpoints)
{
  EXPECT_NEAR(timingNetWeight(-1.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
  EXPECT_NEAR(timingNetWeight(1.0f, -1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

TEST(TimingNetWeight, RampIsLinearBetweenTheEndpoints)
{
  EXPECT_NEAR(timingNetWeight(0.0f, -1.0f, 1.0f, 5.0f), 3.0f, kTolerance);
  EXPECT_NEAR(timingNetWeight(0.5f, -1.0f, 1.0f, 5.0f), 2.0f, kTolerance);
}

// Without the clamp, a design with a long slack tail would weight its worst
// nets far above the configured max.
TEST(TimingNetWeight, SaturatesBelowSlackMin)
{
  EXPECT_NEAR(timingNetWeight(-2.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
  EXPECT_NEAR(timingNetWeight(-100.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
}

TEST(TimingNetWeight, SaturatesAboveSlackMax)
{
  EXPECT_NEAR(timingNetWeight(2.0f, -1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

// A flat ramp carries no information to spread weights with.
TEST(TimingNetWeight, FlatRampLeavesWeightsAtOne)
{
  EXPECT_NEAR(timingNetWeight(1.0f, 1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

}  // namespace
}  // namespace gpl
