// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The order the search lists its slides in, on paths and gains alone.

#include <numeric>
#include <vector>

#include "Retime.h"
#include "gtest/gtest.h"

namespace {

using Order = std::vector<int>;

TEST(RankPathsTest, HighestGainFirst)
{
  EXPECT_EQ(ret::rankPaths({{0}, {1}, {2}}, {1.0, 3.0, 2.0}), (Order{1, 2, 0}));
}

TEST(RankPathsTest, EarlierPathOnATie)
{
  EXPECT_EQ(ret::rankPaths({{0}, {1}, {2}}, {2.0, 2.0, 2.0}), (Order{0, 1, 2}));
}

// Slide 0 is on both paths: listed once, with the path that gains more.
TEST(RankPathsTest, SharedSlideIsListedOnce)
{
  EXPECT_EQ(ret::rankPaths({{0, 1}, {0, 2}}, {5.0, 1.0, 2.0}),
            (Order{0, 2, 1}));
}

// Slide 0 costs 1 and is on the first two paths.  Once the first lists it,
// the second gains 3, not 2, and passes the third.
TEST(RankPathsTest, GainOfWhatIsLeft)
{
  EXPECT_EQ(ret::rankPaths({{0, 1}, {0, 2}, {3}}, {-1.0, 5.0, 3.0, 2.5}),
            (Order{0, 1, 2, 3}));
}

// A ranking that reads every path for each one it lists does not finish
// here in the time of a test.
TEST(RankPathsTest, ManyIndependentPaths)
{
  const int count = 400000;
  std::vector<std::vector<int>> paths;
  std::vector<double> gains;
  for (int i = 0; i < count; ++i) {
    paths.push_back({i});
    gains.push_back(i);
  }
  Order expected(count);
  std::iota(expected.rbegin(), expected.rend(), 0);
  EXPECT_EQ(ret::rankPaths(paths, gains), expected);
}

}  // namespace
