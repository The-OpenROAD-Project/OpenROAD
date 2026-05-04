// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "dispatch.hh"
#include "gtest/gtest.h"

namespace rsz {

TEST(ParsePhases, EmptyInputReturnsEmptyVector)
{
  EXPECT_TRUE(parsePhases("").empty());
  EXPECT_TRUE(parsePhases("   ").empty());
  EXPECT_TRUE(parsePhases("\t\n  ").empty());
}

TEST(ParsePhases, SingleTokenGetsFirstSpecialMarker)
{
  const std::vector<PhaseStep> steps = parsePhases("LEGACY");
  ASSERT_EQ(steps.size(), 1u);
  EXPECT_EQ(steps[0].name, "LEGACY");
  EXPECT_EQ(steps[0].marker, '*');
}

TEST(ParsePhases, MultipleTokensWhitespaceTokenized)
{
  const std::vector<PhaseStep> steps
      = parsePhases("LEGACY  LAST_GASP\tWNS_PATH");
  ASSERT_EQ(steps.size(), 3u);
  EXPECT_EQ(steps[0].name, "LEGACY");
  EXPECT_EQ(steps[1].name, "LAST_GASP");
  EXPECT_EQ(steps[2].name, "WNS_PATH");
}

TEST(ParsePhases, MarkerSequenceMatchesLegacyOrder)
{
  // Legacy lambda used 8 special chars then 'a'..'z' then 'A'..'Z'.  Same
  // input must produce identical markers for byte-equal regression.
  const std::vector<PhaseStep> steps
      = parsePhases("T0 T1 T2 T3 T4 T5 T6 T7 T8 T9");
  ASSERT_EQ(steps.size(), 10u);
  EXPECT_EQ(steps[0].marker, '*');
  EXPECT_EQ(steps[1].marker, '+');
  EXPECT_EQ(steps[2].marker, '^');
  EXPECT_EQ(steps[3].marker, '&');
  EXPECT_EQ(steps[4].marker, '@');
  EXPECT_EQ(steps[5].marker, '!');
  EXPECT_EQ(steps[6].marker, '-');
  EXPECT_EQ(steps[7].marker, '=');
  EXPECT_EQ(steps[8].marker, 'a');
  EXPECT_EQ(steps[9].marker, 'b');
}

TEST(ParsePhases, MarkerOverflowsToUppercaseThenQuestion)
{
  // 8 specials + 26 lowercase = 34; then 26 uppercase = 60; beyond → '?'.
  std::string many;
  for (int i = 0; i < 65; ++i) {
    if (!many.empty()) {
      many += ' ';
    }
    many += "T";
  }
  const std::vector<PhaseStep> steps = parsePhases(many);
  ASSERT_EQ(steps.size(), 65u);
  EXPECT_EQ(steps[33].marker, 'z');  // 8 + 26 - 1 = 33 → last lowercase
  EXPECT_EQ(steps[34].marker, 'A');
  EXPECT_EQ(steps[59].marker, 'Z');  // 8 + 26 + 26 - 1 = 59 → last upper
  EXPECT_EQ(steps[60].marker, '?');  // overflow
  EXPECT_EQ(steps[64].marker, '?');
}

TEST(ParsePhases, RepeatedSameTokenGetsDistinctMarkers)
{
  // Same name at two positions must get different markers (per-position).
  const std::vector<PhaseStep> steps = parsePhases("LEGACY LEGACY");
  ASSERT_EQ(steps.size(), 2u);
  EXPECT_EQ(steps[0].name, "LEGACY");
  EXPECT_EQ(steps[1].name, "LEGACY");
  EXPECT_NE(steps[0].marker, steps[1].marker);
  EXPECT_EQ(steps[0].marker, '*');
  EXPECT_EQ(steps[1].marker, '+');
}

}  // namespace rsz
