// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdlib>
#include <vector>

#include "gtest/gtest.h"
#include "stt/SteinerTreeBuilder.h"
#include "stt/pd.h"
#include "utl/Logger.h"

namespace stt {
namespace {

using std::vector;

struct Net
{
  vector<int> x;
  vector<int> y;
};

// Eleven pins, driver 0.  Prim-Dijkstra at alpha 0.3 returns a tree that
// checkTree rejects.
Net rejectedByCheckTree()
{
  return {{0, 9, 1, 12, 14, 15, 2, 4, 19, 0, 15},
          {10, 2, 17, 14, 6, 12, 14, 14, 3, 9, 0}};
}

// Prim-Dijkstra at alpha 0.3 returns a valid tree that is longer than the
// Steiner tree for these pins.
Net primDijkstraLongerThanFlute()
{
  return {{10, 50, 10, 90, 40}, {30, 10, 10, 50, 90}};
}

int treeLength(const Tree& tree)
{
  int length = 0;
  for (const Branch& branch : tree.branch) {
    const Branch& neighbor = tree.branch[branch.n];
    length += std::abs(branch.x - neighbor.x) + std::abs(branch.y - neighbor.y);
  }
  return length;
}

void expectSameTree(const Tree& actual, const Tree& expected)
{
  ASSERT_EQ(actual.deg, expected.deg);
  ASSERT_EQ(actual.length, expected.length);
  ASSERT_EQ(actual.branchCount(), expected.branchCount());
  for (int i = 0; i < actual.branchCount(); ++i) {
    EXPECT_EQ(actual.branch[i].x, expected.branch[i].x);
    EXPECT_EQ(actual.branch[i].y, expected.branch[i].y);
    EXPECT_EQ(actual.branch[i].n, expected.branch[i].n);
  }
}

class SteinerTreeBuilderTest : public ::testing::Test
{
 protected:
  utl::Logger logger_;
  SteinerTreeBuilder builder_{&logger_};
};

TEST_F(SteinerTreeBuilderTest, ZeroAlphaIsFlute)
{
  const Net net = primDijkstraLongerThanFlute();
  const Tree tree = builder_.makeSteinerTree(net.x, net.y, 0, 0.0f);
  expectSameTree(tree, builder_.flute(net.x, net.y, 3));
}

TEST_F(SteinerTreeBuilderTest, PositiveAlphaIsPrimDijkstra)
{
  const Net net = primDijkstraLongerThanFlute();
  const Tree pd = pdr::primDijkstra(net.x, net.y, 0, 0.3f, &logger_);
  ASSERT_TRUE(builder_.checkTree(pd));

  const Tree tree = builder_.makeSteinerTree(net.x, net.y, 0, 0.3f);
  expectSameTree(tree, pd);
  EXPECT_GT(tree.length, builder_.flute(net.x, net.y, 3).length);
}

TEST_F(SteinerTreeBuilderTest, DefaultAlphaSelectsBuilder)
{
  const Net net = primDijkstraLongerThanFlute();
  builder_.setAlpha(0.0f);
  expectSameTree(builder_.makeSteinerTree(net.x, net.y, 0),
                 builder_.flute(net.x, net.y, 3));

  builder_.setAlpha(0.3f);
  expectSameTree(builder_.makeSteinerTree(net.x, net.y, 0),
                 pdr::primDijkstra(net.x, net.y, 0, 0.3f, &logger_));
}

TEST_F(SteinerTreeBuilderTest, RejectedPrimDijkstraFallsBackToFlute)
{
  const Net net = rejectedByCheckTree();
  const Tree pd = pdr::primDijkstra(net.x, net.y, 0, 0.3f, &logger_);
  ASSERT_FALSE(builder_.checkTree(pd));

  const Tree tree = builder_.makeSteinerTree(net.x, net.y, 0, 0.3f);
  expectSameTree(tree, builder_.flute(net.x, net.y, 3));
  EXPECT_TRUE(builder_.checkTree(tree));
}

TEST_F(SteinerTreeBuilderTest, TwoPinNet)
{
  const vector<int> x = {10, 40};
  const vector<int> y = {20, 60};
  for (const float alpha : {0.0f, 0.3f}) {
    const Tree tree = builder_.makeSteinerTree(x, y, 0, alpha);
    EXPECT_EQ(tree.deg, 2);
    EXPECT_EQ(tree.length, 70);
    EXPECT_EQ(treeLength(tree), 70);
  }
}

// Degree 9 is the largest handled by the lookup table.
TEST_F(SteinerTreeBuilderTest, NinePinGrid)
{
  vector<int> x;
  vector<int> y;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      x.push_back(i * 10);
      y.push_back(j * 10);
    }
  }
  for (const float alpha : {0.0f, 0.3f}) {
    const Tree tree = builder_.makeSteinerTree(x, y, 4, alpha);
    EXPECT_EQ(tree.deg, 9);
    EXPECT_EQ(tree.length, 80);
    EXPECT_EQ(treeLength(tree), 80);
    EXPECT_TRUE(builder_.checkTree(tree));
  }
}

TEST_F(SteinerTreeBuilderTest, CheckTreeRejectsCrossingBranches)
{
  // (0,5)-(10,5) crosses (5,0)-(5,10) and they share no corner.
  Tree tree;
  tree.deg = 4;
  tree.length = 20;
  tree.branch = {{0, 5, 1}, {10, 5, 1}, {5, 0, 3}, {5, 10, 3}};
  EXPECT_FALSE(builder_.checkTree(tree));
}

TEST_F(SteinerTreeBuilderTest, CheckTreeAcceptsBranchesSharingCorner)
{
  Tree tree;
  tree.deg = 3;
  tree.length = 20;
  tree.branch = {{0, 0, 1}, {10, 0, 1}, {10, 10, 1}};
  EXPECT_TRUE(builder_.checkTree(tree));
}

TEST_F(SteinerTreeBuilderTest, CheckTreeSkipsHighFanout)
{
  Tree tree;
  tree.deg = 101;
  tree.length = 20;
  tree.branch = {{0, 5, 1}, {10, 5, 1}, {5, 0, 3}, {5, 10, 3}};
  EXPECT_TRUE(builder_.checkTree(tree));
}

}  // namespace
}  // namespace stt
