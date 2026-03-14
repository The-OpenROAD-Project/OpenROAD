// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "gtest/gtest.h"
#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"

namespace stt::flt {
namespace {

// Minimum Steiner tree wirelength for a set of points equals the
// half-perimeter wirelength (HPWL) of the bounding box when the
// points are rectilinear.  For d <= 3 the optimal RSMT == HPWL.
int hpwl(const std::vector<int>& x, const std::vector<int>& y)
{
  const auto [xmin, xmax] = std::minmax_element(x.begin(), x.end());
  const auto [ymin, ymax] = std::minmax_element(y.begin(), y.end());
  return (*xmax - *xmin) + (*ymax - *ymin);
}

class FluteTest : public ::testing::Test
{
 protected:
  Flute flute_;
};

TEST_F(FluteTest, TwoPin)
{
  std::vector<int> x = {0, 100};
  std::vector<int> y = {0, 200};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 2);
  EXPECT_EQ(tree.length, 300);
  EXPECT_EQ(flute_.wirelength(tree), 300);
}

TEST_F(FluteTest, ThreePin)
{
  std::vector<int> x = {0, 100, 50};
  std::vector<int> y = {0, 0, 100};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 3);
  // Optimal RSMT for 3 points is HPWL
  EXPECT_EQ(tree.length, hpwl(x, y));
}

TEST_F(FluteTest, FourPin)
{
  std::vector<int> x = {0, 100, 0, 100};
  std::vector<int> y = {0, 0, 100, 100};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 4);
  // Square: optimal RSMT = 300 (perimeter - one side)
  EXPECT_EQ(tree.length, 300);
}

TEST_F(FluteTest, NinePinMaxLUT)
{
  // 9-pin net exercises the maximum LUT degree
  std::vector<int> x = {0, 100, 200, 0, 100, 200, 0, 100, 200};
  std::vector<int> y = {0, 0, 0, 100, 100, 100, 200, 200, 200};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 9);
  // Wirelength must be at least HPWL
  EXPECT_GE(tree.length, hpwl(x, y));
  // For a 3x3 grid the optimal RSMT wirelength is 800
  EXPECT_LE(tree.length, 800);
}

TEST_F(FluteTest, TenPinAllDegree)
{
  // 10-pin net exceeds LUT degree, exercises the decomposition path
  std::vector<int> x = {0, 100, 200, 300, 400, 0, 100, 200, 300, 400};
  std::vector<int> y = {0, 0, 0, 0, 0, 100, 100, 100, 100, 100};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 10);
  EXPECT_GE(tree.length, hpwl(x, y));
}

TEST_F(FluteTest, CollinearPoints)
{
  std::vector<int> x = {0, 100, 200, 300};
  std::vector<int> y = {0, 0, 0, 0};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.length, 300);
}

TEST_F(FluteTest, DuplicatePoints)
{
  std::vector<int> x = {0, 0, 100, 100};
  std::vector<int> y = {0, 0, 100, 100};

  Tree tree = flute_.flute(x, y, 3);

  // Two unique points, wirelength should be Manhattan distance
  EXPECT_EQ(tree.length, 200);
}

TEST_F(FluteTest, SinglePoint)
{
  std::vector<int> x = {42};
  std::vector<int> y = {99};

  Tree tree = flute_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 1);
  EXPECT_EQ(tree.length, 0);
}

TEST_F(FluteTest, Determinism)
{
  std::vector<int> x = {0, 50, 100, 200, 150};
  std::vector<int> y = {0, 100, 50, 200, 150};

  Tree tree1 = flute_.flute(x, y, 3);
  Tree tree2 = flute_.flute(x, y, 3);

  EXPECT_EQ(tree1.length, tree2.length);
  ASSERT_EQ(tree1.branch.size(), tree2.branch.size());
  for (size_t i = 0; i < tree1.branch.size(); ++i) {
    EXPECT_EQ(tree1.branch[i].x, tree2.branch[i].x);
    EXPECT_EQ(tree1.branch[i].y, tree2.branch[i].y);
    EXPECT_EQ(tree1.branch[i].n, tree2.branch[i].n);
  }
}

TEST_F(FluteTest, WirelengthOnly)
{
  std::vector<int> x = {0, 100, 50};
  std::vector<int> y = {0, 0, 100};

  int wl = flute_.flute_wl(3, x, y, 3);

  EXPECT_EQ(wl, hpwl(x, y));
}

TEST_F(FluteTest, BranchCountConsistency)
{
  std::vector<int> x = {0, 100, 200, 50, 150};
  std::vector<int> y = {0, 0, 0, 100, 100};

  Tree tree = flute_.flute(x, y, 3);

  // For d pins, branch count should be 2*d - 2 (d pins + d-2 Steiner points)
  EXPECT_EQ(tree.branchCount(), 2 * tree.deg - 2);

  // Every branch index must be valid
  for (int i = 0; i < tree.branchCount(); ++i) {
    EXPECT_GE(tree.branch[i].n, 0);
    EXPECT_LT(tree.branch[i].n, tree.branchCount());
  }
}

}  // namespace
}  // namespace stt::flt
