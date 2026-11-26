// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The rule that keeps or undoes a trial, on slack values alone.

#include <algorithm>

#include "RetimerPlacer.h"
#include "StaCheck.h"
#include "gtest/gtest.h"

namespace {

constexpr float kPs = 1e-12f;

ret::StaSlack scene(const float setup, const float hold)
{
  ret::StaSlack slack;
  slack.worst = setup;
  slack.total = setup;
  slack.hold_worst = hold;
  slack.hold_total = hold;
  return slack;
}

// The worst of the scenes on top, as staSlack() returns it.
ret::StaSlack design(const ret::StaSlack& a, const ret::StaSlack& b)
{
  ret::StaSlack all
      = scene(std::min(a.worst, b.worst), std::min(a.hold_worst, b.hold_worst));
  all.scenes = {a, b};
  return all;
}

TEST(StaCheckTest, KeepsAGainAndNothingElse)
{
  const ret::StaSlack start
      = design(scene(-100 * kPs, 0.0f), scene(-50 * kPs, 0.0f));
  const ret::StaSlack gain
      = design(scene(-90 * kPs, 0.0f), scene(-50 * kPs, 0.0f));
  EXPECT_TRUE(ret::keepsTrial(gain, start, start));
  EXPECT_FALSE(ret::keepsTrial(start, start, start));
}

// The worst scene gains, the other one falls below its start.
TEST(StaCheckTest, RefusesASceneThatGetsWorse)
{
  const ret::StaSlack start
      = design(scene(-100 * kPs, 0.0f), scene(-50 * kPs, 0.0f));
  const ret::StaSlack trade
      = design(scene(-90 * kPs, 0.0f), scene(-60 * kPs, 0.0f));
  EXPECT_FALSE(ret::keepsTrial(trade, start, start));
}

// Each round gains setup slack and loses 0.06 ps of hold slack, less than
// the tolerance of 0.1 ps.  The second round is within it of the first, but
// not of the start of the command.
TEST(StaCheckTest, ToleranceDoesNotAddUpOverRounds)
{
  const ret::StaSlack other = scene(-50 * kPs, 0.0f);
  const ret::StaSlack start = design(scene(-100 * kPs, 0.0f), other);
  const ret::StaSlack round1 = design(scene(-90 * kPs, -0.06f * kPs), other);
  const ret::StaSlack round2 = design(scene(-80 * kPs, -0.12f * kPs), other);
  EXPECT_TRUE(ret::keepsTrial(round1, start, start));
  EXPECT_TRUE(ret::keepsTrial(round2, round1, round1));
  EXPECT_FALSE(ret::keepsTrial(round2, round1, start));
}

}  // namespace
