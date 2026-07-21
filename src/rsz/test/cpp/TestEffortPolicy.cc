// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// repair_timing -effort policy resolution: directives apply left to
// right, later directives override earlier ones (compiler driver -O
// semantics).

#include "gtest/gtest.h"
#include "rsz/EffortPolicy.hh"

namespace rsz {

TEST(EffortPolicy, DefaultIsTapeout)
{
  EffortPolicy policy;
  EXPECT_EQ(policy.plateau_start_iteration, 1000);
  EXPECT_FLOAT_EQ(policy.min_inc_fix_rate, 0.0001f);
  EXPECT_TRUE(policy.repair_hold);
}

TEST(EffortPolicy, TapeoutIsExplicitDefault)
{
  EffortPolicy policy;
  ASSERT_TRUE(policy.apply("tapeout"));
  EXPECT_EQ(policy, EffortPolicy());
}

TEST(EffortPolicy, ExploreBoundsSetupAndDisablesHold)
{
  EffortPolicy policy;
  ASSERT_TRUE(policy.apply("explore"));
  EXPECT_EQ(policy.plateau_start_iteration, 0);
  EXPECT_FLOAT_EQ(policy.min_inc_fix_rate, 0.05f);
  EXPECT_FALSE(policy.repair_hold);
}

TEST(EffortPolicy, LaterDirectiveWins)
{
  EffortPolicy policy;
  ASSERT_TRUE(policy.apply("explore"));
  ASSERT_TRUE(policy.apply("tapeout"));
  EXPECT_EQ(policy, EffortPolicy());

  ASSERT_TRUE(policy.apply("tapeout"));
  ASSERT_TRUE(policy.apply("explore"));
  EXPECT_FALSE(policy.repair_hold);
}

TEST(EffortPolicy, ExplicitHoldAfterExploreReenablesHoldRepair)
{
  EffortPolicy policy;
  ASSERT_TRUE(policy.apply("explore"));
  ASSERT_TRUE(policy.apply("-hold"));
  EXPECT_TRUE(policy.repair_hold);
  // The setup bound is unaffected.
  EXPECT_EQ(policy.plateau_start_iteration, 0);
  EXPECT_FLOAT_EQ(policy.min_inc_fix_rate, 0.05f);
}

TEST(EffortPolicy, ExploreAfterExplicitHoldWins)
{
  EffortPolicy policy;
  ASSERT_TRUE(policy.apply("-hold"));
  ASSERT_TRUE(policy.apply("explore"));
  EXPECT_FALSE(policy.repair_hold);
}

TEST(EffortPolicy, UnknownDirectiveIsRejectedAndIgnored)
{
  EffortPolicy policy;
  EXPECT_FALSE(policy.apply("medium"));
  EXPECT_EQ(policy, EffortPolicy());
}

}  // namespace rsz
