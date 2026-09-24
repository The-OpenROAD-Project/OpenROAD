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
  static void corruptStatistics(Graph2D& graph)
  {
    graph.h_overflow_.usage.add(1, 0);
  }

  static size_t estimateCount(const Graph2D& graph)
  {
    return graph.h_dirty_est_edges_.size() + graph.v_dirty_est_edges_.size();
  }

  static size_t historyCount(const Graph2D& graph)
  {
    return graph.h_dirty_history_edges_.size()
           + graph.v_dirty_history_edges_.size();
  }

  // Original full-grid operations, independent of the sparse lists.
  static void resetEstimates(Graph2D& graph)
  {
    for (auto* edges : {&graph.h_edges_, &graph.v_edges_}) {
      for (size_t i = 0; i < edges->num_elements(); ++i) {
        edges->data()[i].est_usage = 0;
      }
    }
    graph.invalidateOverflow2D();
  }

  static void resetHistory(Graph2D& graph, int up_type)
  {
    for (auto* edges : {&graph.h_edges_, &graph.v_edges_}) {
      for (size_t i = 0; i < edges->num_elements(); ++i) {
        auto& edge = edges->data()[i];
        edge.last_usage = 0;
        if (up_type == 1) {
          edge.congCNT = 0;
        } else if (up_type == 2) {
          edge.last_usage = edge.last_usage * 0.2;
        }
      }
    }
  }

  static void convertEstimates(Graph2D& graph)
  {
    for (auto direction :
         {EdgeDirection::Horizontal, EdgeDirection::Vertical}) {
      auto& edges = direction == EdgeDirection::Horizontal ? graph.h_edges_
                                                           : graph.v_edges_;
      const int nx = edges.shape()[0];
      const int ny = edges.shape()[1];
      for (int x = 0; x < nx; ++x) {
        for (int y = 0; y < ny; ++y) {
          if (edges[x][y].est_usage != 0) {
            graph.mutateEdge(x, y, direction, [](Edge& edge) {
              edge.usage += edge.est_usage;
            });
            graph.markUsedGridDirty(x, y, direction);
          }
        }
      }
    }
  }

  static void expectSameState(const Graph2D& graph, const Graph2D& reference)
  {
    const auto compare = [](const auto& edges, const auto& ref) {
      ASSERT_EQ(edges.num_elements(), ref.num_elements());
      for (size_t i = 0; i < edges.num_elements(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(edges.data()[i].usage, ref.data()[i].usage);
        EXPECT_EQ(edges.data()[i].est_usage, ref.data()[i].est_usage);
        EXPECT_EQ(edges.data()[i].last_usage, ref.data()[i].last_usage);
        EXPECT_EQ(edges.data()[i].congCNT, ref.data()[i].congCNT);
      }
    };
    compare(graph.h_edges_, reference.h_edges_);
    compare(graph.v_edges_, reference.v_edges_);
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
    logger_.setDebugLevel(utl::GRT, "overflowcheck", 1);
    graph_.init(kXGrid, kYGrid, 2, &logger_);
    graph_.initCap3D();
    graph_.InitLastUsage(1);
    net_.setEdgeCost(1);
    graph_.overflowStatistics(false);
    graph_.overflowStatistics(true);
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
    graph_.overflowStatistics(false);
    graph_.overflowStatistics(true);
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
    copied.overflowStatistics(false);
    copied.overflowStatistics(true);
    EXPECT_TRUE(copied.getUsedGridsH().empty());
    EXPECT_EQ(copied.getUsedGridsV().size(), 1u);

    // Restore into an already allocated graph with its own pending changes.
    copied.addUsageH(4, 4, 1);
    copied.copyRoutingStateFrom(graph_, include_ndr);
    copied.prepareForIncrementalRun();
    EXPECT_TRUE(copied.usedGridsMatchUsage());
    copied.overflowStatistics(false);
    copied.overflowStatistics(true);
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

TEST_F(Graph2DTest, PreservesUnsignedUsageAndCapacityConversions)
{
  graph_.addUsageH(1, 1, 65535);
  graph_.addCapH(1, 1, 65535);
  EXPECT_EQ(graph_.maxUsage(EdgeDirection::Horizontal), 65535);
  graph_.addUsageH(1, 1, 1);
  graph_.addCapH(1, 1, 1);
  EXPECT_EQ(graph_.getUsageH(1, 1), 0);
  EXPECT_EQ(graph_.getCapH(1, 1), 0);
  EXPECT_EQ(graph_.maxUsage(EdgeDirection::Horizontal), 0);
  expectReferenceMatch();
}

TEST_F(Graph2DTest, DebugCheckDetectsDrifting2DCountersInRelease)
{
  Graph2DTestPeer::corruptStatistics(graph_);
  EXPECT_THROW(graph_.overflowStatistics(false), std::runtime_error);
}

TEST_F(Graph2DTest, ConvertsAndResetsEstimatesOutsideUsedGrids)
{
  graph_.addUsageH(1, 1, 5);
  graph_.addUsageV(1, 1, 5);
  graph_.clearUsed();
  // Negative updates do not insert membership, but must still be converted.
  graph_.updateEstUsageH(1, 1, &net_, -0.5);
  graph_.updateEstUsageV(1, 1, &net_, -1.5);
  graph_.updateEstUsageH(2, 2, &net_, 0.5);
  graph_.updateEstUsageH(2, 2, &net_, -0.5);
  EXPECT_EQ(Graph2DTestPeer::estimateCount(graph_), 3u);
  graph_.clearUsed();
  graph_.overflowStatistics(true);
  graph_.addEstUsageToUsage();
  EXPECT_EQ(graph_.getUsageH(1, 1), 4);
  EXPECT_EQ(graph_.getUsageV(1, 1), 3);
  EXPECT_EQ(graph_.getUsageH(2, 2), 0);
  // Conversion leaves estimates intact until the explicit reset.
  graph_.addEstUsageToUsage();
  EXPECT_EQ(graph_.getUsageH(1, 1), 3);
  EXPECT_EQ(graph_.getUsageV(1, 1), 1);
  graph_.InitEstUsage();
  graph_.InitEstUsage();
  EXPECT_EQ(Graph2DTestPeer::estimateCount(graph_), 0u);
  EXPECT_EQ(graph_.getEstUsageH(1, 1), 0);
  EXPECT_EQ(graph_.getEstUsageV(1, 1), 0);
  graph_.addEstUsageToUsage();
  EXPECT_EQ(graph_.getUsageH(1, 1), 3);
  expectReferenceMatch();
}

TEST_F(Graph2DTest, ResetsHistoryAfterMembershipRemovalAndStressAccumulation)
{
  graph_.addCapH(1, 1, 1);
  graph_.addCapV(1, 1, 1);
  graph_.addUsageH(1, 1, 4);
  graph_.addUsageV(1, 1, 4);
  int max_adj = 0;
  graph_.updateCongestionHistory(1, 20, false, max_adj);
  graph_.updateCongestionHistory(1, 20, false, max_adj);
  graph_.clearUsed();
  Graph2D reference;
  reference.copyRoutingStateFrom(graph_, false);

  for (int up_type : {2, 3, 1, 1}) {
    graph_.InitLastUsage(up_type);
    Graph2DTestPeer::resetHistory(reference, up_type);
    Graph2DTestPeer::expectSameState(graph_, reference);
    // str_accu visits edges outside the used sets and reuses retained congCNT.
    graph_.str_accu(0);
    reference.str_accu(0);
    Graph2DTestPeer::expectSameState(graph_, reference);
    EXPECT_EQ(Graph2DTestPeer::historyCount(graph_), up_type == 1 ? 0u : 2u);
  }
}

TEST_F(Graph2DTest, CopiesEstimateAndHistoryResetState)
{
  graph_.updateEstUsageH(1, 1, &net_, 2.5);
  graph_.updateEstUsageV(2, 2, &net_, 1.5);
  graph_.addUsageH(1, 1, 3);
  graph_.addUsageV(2, 2, 4);
  int max_adj = 0;
  graph_.updateCongestionHistory(1, 20, false, max_adj);
  graph_.InitLastUsage(2);
  graph_.clearUsed();

  for (bool include_ndr : {false, true}) {
    Graph2D copied;
    for (int restore = 0; restore < 2; ++restore) {
      copied.copyRoutingStateFrom(graph_, include_ndr);
      Graph2D reference;
      reference.copyRoutingStateFrom(graph_, include_ndr);
      copied.addEstUsageToUsage();
      Graph2DTestPeer::convertEstimates(reference);
      copied.InitEstUsage();
      Graph2DTestPeer::resetEstimates(reference);
      copied.InitLastUsage(1);
      Graph2DTestPeer::resetHistory(reference, 1);
      copied.str_accu(0);
      reference.str_accu(0);
      Graph2DTestPeer::expectSameState(copied, reference);
      EXPECT_EQ(Graph2DTestPeer::estimateCount(copied), 0u);
      EXPECT_EQ(Graph2DTestPeer::historyCount(copied), 0u);
      copied.prepareForIncrementalRun();
      copied.overflowStatistics(false);
      copied.overflowStatistics(true);
      // The second restore replaces existing pending state and cached totals.
      copied.updateEstUsageH(4, 4, &net_, 1);
      copied.updateCongestionHistory(1, 20, false, max_adj);
    }
  }
}

TEST_F(Graph2DTest, InitAndClearDiscardPendingResets)
{
  for (bool clear : {false, true}) {
    graph_.updateEstUsageH(4, 4, &net_, 1);
    graph_.addUsageV(5, 3, 2);
    int max_adj = 0;
    graph_.updateCongestionHistory(1, 20, false, max_adj);
    if (clear) {
      graph_.clear();
      graph_.InitEstUsage();
      graph_.InitLastUsage(1);
    }
    graph_.init(2, 2, 1, &logger_);
    graph_.InitEstUsage();
    graph_.InitLastUsage(1);
    EXPECT_EQ(Graph2DTestPeer::estimateCount(graph_), 0u);
    EXPECT_EQ(Graph2DTestPeer::historyCount(graph_), 0u);
    graph_.init(kXGrid, kYGrid, 2, &logger_);
    graph_.initCap3D();
  }
}

TEST_F(Graph2DTest, SparseResetsMatchFullGridAcrossRepeatedRuns)
{
  Graph2D reference;
  reference.copyRoutingStateFrom(graph_, false);
  std::mt19937 random(4567);
  for (int run = 0; run < 200; ++run) {
    SCOPED_TRACE(run);
    for (int update = 0; update < 20; ++update) {
      const int x = random() % (kXGrid - 1);
      const int y = random() % (kYGrid - 1);
      const double estimate = (random() % 5) * 0.5;
      for (auto* graph : {&graph_, &reference}) {
        graph->updateEstUsageH(x, y, &net_, estimate);
        graph->updateEstUsageV(x, y, &net_, estimate);
        graph->addUsageH(x, y, 1);
        graph->addUsageV(x, y, 1);
      }
    }
    if (run % 3 == 0) {
      graph_.clearUsed();
      reference.clearUsed();
    }
    graph_.addEstUsageToUsage();
    Graph2DTestPeer::convertEstimates(reference);
    graph_.InitEstUsage();
    Graph2DTestPeer::resetEstimates(reference);
    const int up_type = run % 3 + 1;
    graph_.InitLastUsage(up_type);
    Graph2DTestPeer::resetHistory(reference, up_type);
    int actual_adj = 0;
    int reference_adj = 0;
    graph_.updateCongestionHistory(up_type, 20, run % 2, actual_adj);
    reference.updateCongestionHistory(up_type, 20, run % 2, reference_adj);
    EXPECT_EQ(actual_adj, reference_adj);
    graph_.str_accu(12);
    reference.str_accu(12);
    Graph2DTestPeer::expectSameState(graph_, reference);
    graph_.prepareForIncrementalRun();
    reference.prepareForIncrementalRun();
    EXPECT_EQ(graph_.overflowStatistics(false),
              reference.overflowStatistics(false));
    EXPECT_EQ(graph_.overflowStatistics(true),
              reference.overflowStatistics(true));
  }
}

TEST_F(Graph2DTest, ResetListsRemainBoundedAcrossManyRuns)
{
  for (int run = 0; run < 2000; ++run) {
    graph_.addUsageH(1, 1, 2);
    graph_.addUsageV(2, 2, 2);
    for (int update = 0; update < 10; ++update) {
      graph_.updateEstUsageH(1, 1, &net_, 0.5);
      graph_.updateEstUsageH(1, 1, &net_, -0.5);
      graph_.updateEstUsageV(2, 2, &net_, 0.5);
      graph_.updateEstUsageV(2, 2, &net_, -0.5);
      int max_adj = 0;
      graph_.updateCongestionHistory(1, 20, false, max_adj);
    }
    ASSERT_EQ(Graph2DTestPeer::estimateCount(graph_), 2u);
    ASSERT_EQ(Graph2DTestPeer::historyCount(graph_), 2u);
    graph_.addUsageH(1, 1, -2);
    graph_.addUsageV(2, 2, -2);
    graph_.prepareForIncrementalRun();
    graph_.InitEstUsage();
    graph_.InitLastUsage(1);
    ASSERT_EQ(Graph2DTestPeer::estimateCount(graph_), 0u);
    ASSERT_EQ(Graph2DTestPeer::historyCount(graph_), 0u);
    ASSERT_EQ(graph_.getLastUsageH(1, 1), 0);
    ASSERT_EQ(graph_.getLastUsageV(2, 2), 0);
  }
}

}  // namespace
}  // namespace grt
