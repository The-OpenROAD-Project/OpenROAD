// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstddef>

#include "gtest/gtest.h"
#include "src/gpl/src/timingBase.h"

namespace gpl {
namespace {

constexpr float kTolerance = 1e-5f;

// Size of the worst-slack net list the resizer hands back for a design with
// net_total constrained nets, mirroring Resizer::resizeWorstSlackNets().
size_t worstSlackNetCount(const size_t net_total, const float nets_percentage)
{
  const auto nworst
      = static_cast<size_t>(std::ceil(net_total * nets_percentage / 100.0));
  return std::min(net_total, nworst);
}

// The reason the anchor is scaled by nets_percentage: widening the ranking
// must not also widen the group of nets pinned at the full weight.
TEST(SlackRampAnchorIndex, DoesNotMoveWithNetsPercentage)
{
  constexpr size_t kNetTotal = 150000;
  const size_t expected = kNetTotal * kSlackMinPercent / 100;
  for (const float pct : {10.0f, 20.0f, 25.0f, 50.0f, 70.0f, 100.0f}) {
    EXPECT_EQ(slackRampAnchorIndex(worstSlackNetCount(kNetTotal, pct), pct),
              expected)
        << "nets_percentage " << pct;
  }
}

// At 100% the list is the whole design, so the anchor is the plain percentage.
TEST(SlackRampAnchorIndex, AnchorsOnPercentageOfTheDesign)
{
  EXPECT_EQ(slackRampAnchorIndex(150000, 100.0f), 300u);
  EXPECT_EQ(slackRampAnchorIndex(50000, 50.0f), 200u);
}

// Below 100 / kSlackMinPercent constrained nets the percentage floors to zero,
// the pre-existing behavior of anchoring on the single worst net.
TEST(SlackRampAnchorIndex, SmallDesignsAnchorOnTheWorstNet)
{
  EXPECT_EQ(slackRampAnchorIndex(1, 100.0f), 0u);
  EXPECT_EQ(slackRampAnchorIndex(284, 100.0f), 0u);
  EXPECT_EQ(slackRampAnchorIndex(29, 10.0f), 0u);
}

// nets_percentage is user-supplied; it must not reach the division as zero.
TEST(SlackRampAnchorIndex, ZeroNetsPercentageIsSafe)
{
  EXPECT_EQ(slackRampAnchorIndex(1000, 0.0f), 0u);
}

TEST(SlackRampAnchorIndex, StaysInRange)
{
  for (size_t net_count = 1; net_count <= 1000; ++net_count) {
    for (const float pct : {0.1f, 1.0f, 10.0f, 100.0f}) {
      EXPECT_LT(slackRampAnchorIndex(net_count, pct), net_count)
          << "net_count " << net_count << " nets_percentage " << pct;
    }
  }
}

TEST(TimingNetWeight, RampEndpoints)
{
  EXPECT_NEAR(
      computeTimingNetWeight(-1.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
  EXPECT_NEAR(
      computeTimingNetWeight(1.0f, -1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

TEST(TimingNetWeight, RampIsLinearBetweenTheEndpoints)
{
  EXPECT_NEAR(
      computeTimingNetWeight(0.0f, -1.0f, 1.0f, 5.0f), 3.0f, kTolerance);
  EXPECT_NEAR(
      computeTimingNetWeight(0.5f, -1.0f, 1.0f, 5.0f), 2.0f, kTolerance);
}

// Without the clamp, a design with a long slack tail would weight its worst
// nets far above the configured max.
TEST(TimingNetWeight, SaturatesBelowSlackMin)
{
  EXPECT_NEAR(
      computeTimingNetWeight(-2.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
  EXPECT_NEAR(
      computeTimingNetWeight(-100.0f, -1.0f, 1.0f, 5.0f), 5.0f, kTolerance);
}

TEST(TimingNetWeight, SaturatesAboveSlackMax)
{
  EXPECT_NEAR(
      computeTimingNetWeight(2.0f, -1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

// A flat ramp carries no information to spread weights with.
TEST(TimingNetWeight, FlatRampLeavesWeightsAtOne)
{
  EXPECT_NEAR(computeTimingNetWeight(1.0f, 1.0f, 1.0f, 5.0f), 1.0f, kTolerance);
}

}  // namespace
}  // namespace gpl
