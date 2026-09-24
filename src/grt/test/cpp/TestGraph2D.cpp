// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <random>
#include <stdexcept>

#include "Graph2D.h"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace grt {

class Graph2DTestPeer
{
 public:
  static size_t pendingCount(const Graph2D& graph)
  {
    return graph.h_dirty_used_grids_.size() + graph.v_dirty_used_grids_.size();
  }

  static void forgetHorizontalMembership(Graph2D& graph, int x, int y)
  {
    graph.h_used_ggrid_.erase({x, y});
  }
};

namespace {

// Synthetic grids only: compare incremental maintenance with the original
// clear-and-rebuild operation, without loading a technology or running a flow.
class Graph2DTest : public testing::Test
{
 protected:
  static constexpr int kXGrid = 6;
  static constexpr int kYGrid = 5;

  void SetUp() override
  {
    logger_.setDebugLevel(utl::GRT, "usedgridcheck", 1);
    graph_.init(kXGrid, kYGrid, 2, &logger_);
    graph_.initCap3D();
    graph_.InitLastUsage(1);
    net_.setEdgeCost(1);
  }

  void expectReferenceMatch()
  {
    Graph2D reference;
    reference.copyRoutingStateFrom(graph_, false);
    reference.clearUsed();
    reference.rebuildUsedGrids();
    graph_.prepareForIncrementalRun();
    EXPECT_EQ(graph_.getUsedGridsH(), reference.getUsedGridsH());
    EXPECT_EQ(graph_.getUsedGridsV(), reference.getUsedGridsV());
    EXPECT_TRUE(graph_.usedGridsMatchUsage());
    EXPECT_EQ(Graph2DTestPeer::pendingCount(graph_), 0u);
  }

  utl::Logger logger_;
  Graph2D graph_;
  FrNet net_{};
};

TEST_F(Graph2DTest, ReconcilesAddUpdateAndIntervalMutations)
{
  graph_.addUsageH({0, 4}, 2, 2);
  graph_.addUsageV(3, {0, 4}, 2);
  graph_.updateUsageH({1, 3}, 2, &net_, -2);
  graph_.updateUsageV(3, {1, 3}, &net_, -2);
  graph_.updateUsageH(4, 4, &net_, 1);
  graph_.updateUsageV(5, 3, &net_, 1);
  graph_.addUsageH(4, 4, -1);
  graph_.addUsageV(5, 3, -1);

  // Zero-usage entries remain visible until the next incremental run.
  EXPECT_EQ(graph_.getUsedGridsH().size(), 5u);
  EXPECT_EQ(graph_.getUsedGridsV().size(), 5u);
  expectReferenceMatch();
  EXPECT_EQ(graph_.getUsedGridsH().size(), 2u);
  EXPECT_EQ(graph_.getUsedGridsV().size(), 2u);

  // Changes between runs, including a remove-and-restore on the same edge.
  graph_.addUsageH(0, 2, -2);
  graph_.addUsageH(0, 2, 2);
  graph_.addUsageV(3, 0, -2);
  expectReferenceMatch();
  expectReferenceMatch();
}

TEST_F(Graph2DTest, PreservesWithinRunCongestionHistory)
{
  graph_.addCapH(1, 2, 1);
  graph_.addUsageH(1, 2, 3);
  int max_adj = 0;
  graph_.updateCongestionHistory(1, 20, false, max_adj);
  EXPECT_EQ(graph_.getLastUsageH(1, 2), 2);

  graph_.addUsageH(1, 2, -3);
  EXPECT_EQ(graph_.getUsedGridsH().count({1, 2}), 1u);
  graph_.updateCongestionHistory(3, 20, false, max_adj);
  EXPECT_EQ(graph_.getLastUsageH(1, 2), 1);

  expectReferenceMatch();
  EXPECT_TRUE(graph_.getUsedGridsH().empty());
  graph_.updateCongestionHistory(3, 20, false, max_adj);
  EXPECT_EQ(graph_.getLastUsageH(1, 2), 1);
}

TEST_F(Graph2DTest, ReconcilesCanceledAndConvertedEstimates)
{
  graph_.updateEstUsageH({0, 3}, 1, &net_, 0.5);
  graph_.updateEstUsageV(2, {0, 3}, &net_, 0.5);
  graph_.updateEstUsageH(1, 1, &net_, -0.5);
  graph_.updateEstUsageV(2, 1, &net_, -0.5);
  graph_.updateEstUsageH(2, 1, &net_, 0.5);
  graph_.updateEstUsageV(2, 2, &net_, 0.5);
  graph_.addEstUsageToUsage();
  graph_.InitEstUsage();

  // Fractional conversion truncates as before; only one edge per direction
  // acquires committed usage. Neither fractional nor canceled entries linger.
  EXPECT_EQ(graph_.getUsageH(0, 1), 0);
  EXPECT_EQ(graph_.getUsageH(2, 1), 1);
  EXPECT_EQ(graph_.getUsageV(2, 0), 0);
  EXPECT_EQ(graph_.getUsageV(2, 2), 1);
  expectReferenceMatch();
  EXPECT_EQ(graph_.getUsedGridsH().size(), 1u);
  EXPECT_EQ(graph_.getUsedGridsV().size(), 1u);
}

TEST_F(Graph2DTest, ConversionTracksChangesAfterAnEarlierReconciliation)
{
  graph_.updateEstUsageH(0, 0, &net_, 1);
  graph_.updateEstUsageV(0, 0, &net_, 1);
  expectReferenceMatch();
  EXPECT_TRUE(graph_.getUsedGridsH().empty());
  EXPECT_TRUE(graph_.getUsedGridsV().empty());

  graph_.addEstUsageToUsage();
  graph_.InitEstUsage();
  expectReferenceMatch();
  EXPECT_EQ(graph_.getUsedGridsH().size(), 1u);
  EXPECT_EQ(graph_.getUsedGridsV().size(), 1u);
}

TEST_F(Graph2DTest, ClearUsedCanRecoverUnchangedCommittedEdges)
{
  graph_.addUsageH(1, 1, 1);
  graph_.addUsageV(1, 1, 1);
  expectReferenceMatch();
  graph_.clearUsed();
  graph_.clearUsed();
  EXPECT_TRUE(graph_.getUsedGridsH().empty());
  EXPECT_TRUE(graph_.getUsedGridsV().empty());
  expectReferenceMatch();
  EXPECT_EQ(graph_.getUsedGridsH().size(), 1u);
  EXPECT_EQ(graph_.getUsedGridsV().size(), 1u);
}

TEST_F(Graph2DTest, CopiesPendingStateAndDeduplicationFlags)
{
  for (bool include_ndr : {false, true}) {
    graph_.addUsageH(2, 1, 1);
    graph_.addUsageH(2, 1, -1);
    graph_.updateEstUsageV(1, 2, &net_, 1);

    Graph2D copied;
    copied.copyRoutingStateFrom(graph_, include_ndr);
    copied.addUsageV(1, 2, 1);
    EXPECT_EQ(Graph2DTestPeer::pendingCount(copied), 2u);
    copied.prepareForIncrementalRun();
    EXPECT_TRUE(copied.usedGridsMatchUsage());
    EXPECT_TRUE(copied.getUsedGridsH().empty());
    EXPECT_EQ(copied.getUsedGridsV().size(), 1u);

    // Restore into an already allocated graph with its own pending changes.
    copied.addUsageH(4, 4, 1);
    copied.copyRoutingStateFrom(graph_, include_ndr);
    copied.prepareForIncrementalRun();
    EXPECT_TRUE(copied.usedGridsMatchUsage());
    EXPECT_TRUE(copied.getUsedGridsH().empty());
    EXPECT_TRUE(copied.getUsedGridsV().empty());
    expectReferenceMatch();
    graph_.InitEstUsage();
  }
}

TEST_F(Graph2DTest, InitAndClearDiscardOldCoordinates)
{
  graph_.addUsageH(4, 4, 1);
  graph_.addUsageV(5, 3, 1);
  graph_.init(2, 2, 1, &logger_);
  EXPECT_EQ(Graph2DTestPeer::pendingCount(graph_), 0u);
  graph_.prepareForIncrementalRun();
  EXPECT_TRUE(graph_.usedGridsMatchUsage());

  graph_.addUsageH(0, 0, 1);
  graph_.clear();
  EXPECT_FALSE(graph_.hasEdges());
  EXPECT_TRUE(graph_.getUsedGridsH().empty());
  EXPECT_EQ(Graph2DTestPeer::pendingCount(graph_), 0u);
  graph_.init(kXGrid, kYGrid, 2, &logger_);
  graph_.prepareForIncrementalRun();
  EXPECT_TRUE(graph_.usedGridsMatchUsage());
}

TEST_F(Graph2DTest, DeduplicatesAndReusesPendingListsAcrossManyRuns)
{
  for (int run = 0; run < 2000; run++) {
    for (int update = 0; update < 10; update++) {
      graph_.addUsageH(1, 2, 1);
      graph_.addUsageH(1, 2, -1);
      graph_.updateEstUsageV(2, 1, &net_, 0.5);
      graph_.updateEstUsageV(2, 1, &net_, -0.5);
    }
    ASSERT_EQ(Graph2DTestPeer::pendingCount(graph_), 2u);
    graph_.prepareForIncrementalRun();
    ASSERT_EQ(Graph2DTestPeer::pendingCount(graph_), 0u);
    ASSERT_TRUE(graph_.getUsedGridsH().empty());
    ASSERT_TRUE(graph_.getUsedGridsV().empty());
  }
}

TEST_F(Graph2DTest, RandomMutationsMatchLegacyRebuild)
{
  std::mt19937 random(1234);
  for (int run = 0; run < 200; run++) {
    for (int update = 0; update < 50; update++) {
      const int x = random() % (kXGrid - 1);
      const int y = random() % (kYGrid - 1);
      const bool horizontal = random() % 2;
      const int usage
          = horizontal ? graph_.getUsageH(x, y) : graph_.getUsageV(x, y);
      const int delta = usage > 0 && random() % 2 ? -1 : 1;
      if (horizontal) {
        graph_.updateUsageH(x, y, &net_, delta);
        graph_.updateEstUsageH(x, y, &net_, 0.5);
      } else {
        graph_.updateUsageV(x, y, &net_, delta);
        graph_.updateEstUsageV(x, y, &net_, 0.5);
      }
    }
    graph_.addEstUsageToUsage();
    graph_.InitEstUsage();
    if (run % 7 == 0) {
      graph_.clearUsed();
    }
    expectReferenceMatch();
  }
}

TEST_F(Graph2DTest, DebugReferenceCheckDetectsMissingMembershipInRelease)
{
  graph_.addUsageH(1, 2, 1);
  graph_.prepareForIncrementalRun();
  Graph2DTestPeer::forgetHorizontalMembership(graph_, 1, 2);
  EXPECT_FALSE(graph_.usedGridsMatchUsage());
  EXPECT_THROW(graph_.prepareForIncrementalRun(), std::runtime_error);
}

}  // namespace
}  // namespace grt
