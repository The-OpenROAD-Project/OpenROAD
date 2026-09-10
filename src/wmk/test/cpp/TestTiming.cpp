// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include <limits>

#include "Timing.h"
#include "gtest/gtest.h"
#include "sta/MinMax.hh"

namespace wmk {
namespace {

TEST(WatermarkTiming, UnconstrainedSlackIsNotATimingMeasurement)
{
  EXPECT_FALSE(isConstrainedSlack(sta::INF));
  EXPECT_FALSE(isConstrainedSlack(-sta::INF));
  EXPECT_FALSE(isConstrainedSlack(std::numeric_limits<float>::max()));
  EXPECT_FALSE(isConstrainedSlack(std::numeric_limits<float>::infinity()));
  EXPECT_FALSE(isConstrainedSlack(std::numeric_limits<float>::quiet_NaN()));
  EXPECT_TRUE(isConstrainedSlack(0.0f));
  EXPECT_TRUE(isConstrainedSlack(-1e-9f));
}

TEST(WatermarkTiming, UnrelatedClockCannotHideSkewDegradation)
{
  const ClockSkews before{{{1, 0, 0}, 100e-12f}, {{2, 0, 0}, 10e-12f}};
  const ClockSkews after{{{1, 0, 0}, 100e-12f}, {{2, 0, 0}, 60e-12f}};
  EXPECT_FALSE(clockSkewsWithin(before, after, 20e-12f));
  EXPECT_TRUE(clockSkewsWithin(before, before, 0.0f));
}

TEST(WatermarkTiming, EverySceneAndSourceEdgeHasItsOwnBudget)
{
  const ClockSkews before{
      {{1, 0, 0}, 100e-12f}, {{1, 1, 0}, 10e-12f}, {{1, 1, 1}, 5e-12f}};
  auto after = before;
  after[{1, 1, 0}] = 60e-12f;
  EXPECT_FALSE(clockSkewsWithin(before, after, 20e-12f));
  after = before;
  after[{1, 1, 1}] = 40e-12f;
  EXPECT_FALSE(clockSkewsWithin(before, after, 20e-12f));
}

TEST(WatermarkTiming, MissingClockTimingCannotPass)
{
  const ClockSkews before{{{1, 0, 0}, 10e-12f}};
  EXPECT_FALSE(haveClockSkews({}, before));
  EXPECT_FALSE(haveClockSkews({1, 2}, before));
  EXPECT_TRUE(haveClockSkews({1}, before));
  EXPECT_FALSE(clockSkewsWithin(before, {}, 20e-12f));
  EXPECT_FALSE(clockSkewsWithin({}, {}, 20e-12f));
}

TEST(WatermarkTiming, CumulativeSkewIsComparedWithTheOriginalDesign)
{
  const ClockSkews before{{{1, 0, 0}, 10e-12f}};
  const ClockSkews first{{{1, 0, 0}, 25e-12f}};
  const ClockSkews second{{{1, 0, 0}, 40e-12f}};
  EXPECT_TRUE(clockSkewsWithin(before, first, 20e-12f));
  EXPECT_FALSE(clockSkewsWithin(before, second, 20e-12f));
}

}  // namespace
}  // namespace wmk
