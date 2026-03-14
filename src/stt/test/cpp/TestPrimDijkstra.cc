// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"
#include "stt/SteinerTreeBuilder.h"
#include "stt/pd.h"
#include "utl/Logger.h"

namespace pdr {
namespace {

int hpwl(const std::vector<int>& x, const std::vector<int>& y)
{
  const auto [xmin, xmax] = std::minmax_element(x.begin(), x.end());
  const auto [ymin, ymax] = std::minmax_element(y.begin(), y.end());
  return (*xmax - *xmin) + (*ymax - *ymin);
}

class PrimDijkstraTest : public ::testing::Test
{
 protected:
  utl::Logger logger_;
};

TEST_F(PrimDijkstraTest, TwoPin)
{
  std::vector<int> x = {0, 100};
  std::vector<int> y = {0, 200};

  stt::Tree tree = primDijkstra(x, y, 0, 0.3, &logger_);

  EXPECT_EQ(tree.deg, 2);
  EXPECT_EQ(tree.length, 300);
}

TEST_F(PrimDijkstraTest, ThreePin)
{
  std::vector<int> x = {0, 100, 50};
  std::vector<int> y = {0, 0, 100};

  stt::Tree tree = primDijkstra(x, y, 0, 0.3, &logger_);

  EXPECT_EQ(tree.deg, 3);
  EXPECT_GE(tree.length, hpwl(x, y));
}

TEST_F(PrimDijkstraTest, AlphaZeroPureSteiner)
{
  // Alpha=0: pure minimum Steiner tree (no driver-path bias)
  std::vector<int> x = {0, 100, 200, 100};
  std::vector<int> y = {0, 0, 0, 100};

  stt::Tree tree = primDijkstra(x, y, 0, 0.0, &logger_);

  EXPECT_EQ(tree.deg, 4);
  EXPECT_GE(tree.length, hpwl(x, y));
}

TEST_F(PrimDijkstraTest, AlphaOnePureShortestPath)
{
  // Alpha=1: pure shortest-path-to-driver
  std::vector<int> x = {0, 100, 200, 100};
  std::vector<int> y = {0, 0, 0, 100};

  stt::Tree tree = primDijkstra(x, y, 0, 1.0, &logger_);

  EXPECT_EQ(tree.deg, 4);
  EXPECT_GE(tree.length, hpwl(x, y));
}

TEST_F(PrimDijkstraTest, CollinearPoints)
{
  std::vector<int> x = {0, 100, 200, 300};
  std::vector<int> y = {0, 0, 0, 0};

  stt::Tree tree = primDijkstra(x, y, 0, 0.3, &logger_);

  EXPECT_EQ(tree.length, 300);
}

TEST_F(PrimDijkstraTest, Determinism)
{
  std::vector<int> x = {0, 50, 100, 200, 150};
  std::vector<int> y = {0, 100, 50, 200, 150};

  stt::Tree tree1 = primDijkstra(x, y, 0, 0.3, &logger_);
  stt::Tree tree2 = primDijkstra(x, y, 0, 0.3, &logger_);

  EXPECT_EQ(tree1.length, tree2.length);
  ASSERT_EQ(tree1.branch.size(), tree2.branch.size());
  for (size_t i = 0; i < tree1.branch.size(); ++i) {
    EXPECT_EQ(tree1.branch[i].x, tree2.branch[i].x);
    EXPECT_EQ(tree1.branch[i].y, tree2.branch[i].y);
    EXPECT_EQ(tree1.branch[i].n, tree2.branch[i].n);
  }
}

TEST_F(PrimDijkstraTest, ThreadLocalBufferNoLeaks)
{
  // Call multiple times to verify thread_local buffer reuse is correct
  std::vector<int> x1 = {0, 100, 200};
  std::vector<int> y1 = {0, 0, 0};
  stt::Tree tree1 = primDijkstra(x1, y1, 0, 0.3, &logger_);
  EXPECT_EQ(tree1.length, 200);

  std::vector<int> x2 = {0, 50};
  std::vector<int> y2 = {0, 50};
  stt::Tree tree2 = primDijkstra(x2, y2, 0, 0.3, &logger_);
  EXPECT_EQ(tree2.length, 100);

  // Repeat first call to verify no state leakage
  stt::Tree tree3 = primDijkstra(x1, y1, 0, 0.3, &logger_);
  EXPECT_EQ(tree3.length, tree1.length);
}

TEST_F(PrimDijkstraTest, DifferentDriverIndex)
{
  std::vector<int> x = {0, 100, 200};
  std::vector<int> y = {0, 0, 0};

  stt::Tree tree0 = primDijkstra(x, y, 0, 0.5, &logger_);
  stt::Tree tree2 = primDijkstra(x, y, 2, 0.5, &logger_);

  // Both should have the same total wirelength for collinear points
  EXPECT_EQ(tree0.length, 200);
  EXPECT_EQ(tree2.length, 200);
}

TEST_F(PrimDijkstraTest, BranchIndicesValid)
{
  std::vector<int> x = {0, 100, 200, 50, 150};
  std::vector<int> y = {0, 0, 0, 100, 100};

  stt::Tree tree = primDijkstra(x, y, 0, 0.3, &logger_);

  for (int i = 0; i < tree.branchCount(); ++i) {
    EXPECT_GE(tree.branch[i].n, 0);
    EXPECT_LT(tree.branch[i].n, tree.branchCount());
  }
}

}  // namespace
}  // namespace pdr
