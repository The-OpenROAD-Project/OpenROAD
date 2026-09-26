// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <memory>
#include <random>
#include <stdexcept>

#include "FastRoute.h"
#include "Graph2D.h"
#include "Overflow.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/db_fixture.h"
#include "utl/Logger.h"
#include "utl/ServiceRegistry.h"

namespace grt {

class OverflowTestPeer
{
 public:
  static Graph2D& graph(FastRouteCore& router) { return router.graph2d_; }
  static bool cacheValid(const FastRouteCore& router)
  {
    return router.overflow_3d_valid_;
  }
  static void clearNetRoute(FastRouteCore& router) { router.clearNetRoute(0); }
  static int overflow3D(FastRouteCore& router)
  {
    return router.getOverflow3D();
  }
  static std::array<OverflowStatistics, 2> totals(FastRouteCore& router)
  {
    router.getOverflow3D();
    return {router.h_overflow_3d_.statistics(),
            router.v_overflow_3d_.statistics()};
  }
  static auto reference(const FastRouteCore& router)
  {
    return router.scanOverflow3D();
  }
  static void corrupt(FastRouteCore& router)
  {
    router.h_overflow_3d_.add(1, 0);
  }
  static void add3D(FastRouteCore& router,
                    int x,
                    int y,
                    int layer,
                    EdgeDirection direction,
                    int delta)
  {
    router.updateEdge3DUsage(x, y, layer, direction, delta);
  }
};

namespace {

class OverflowTest : public tst::DbFixture
{
 protected:
  static constexpr int kSize = 5;
  void SetUp() override
  {
    logger_.setDebugLevel(utl::GRT, "overflowcheck", 1);
    logger_.setDebugLevel(utl::GRT, "usedgridcheck", 1);
    auto* tech = odb::dbTech::create(db_.get(), "tech");
    auto* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    router_ = std::make_unique<FastRouteCore>(
        db_.get(), &logger_, &registry_, &stt_, nullptr);
    router_->setGridsAndLayers(kSize, kSize, 2);
    router_->initEdges();
    net_ = odb::dbNet::create(block_, "net");
    router_->addNet(net_, false, false, 0, 1, 0, 1, 0, nullptr);
    // Prime the cache before exercising mutations, as in subsequent ECO runs.
    EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  }

  void expectReference()
  {
    const auto actual = OverflowTestPeer::totals(*router_);
    EXPECT_EQ(actual, OverflowTestPeer::reference(*router_));
    EXPECT_EQ(router_->totalOverflow(),
              actual[0].overflow + actual[1].overflow);
  }

  odb::dbBlock* block_ = nullptr;
  odb::dbNet* net_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_{&logger_};
  std::unique_ptr<FastRouteCore> router_;
};

TEST_F(OverflowTest, TracksUsageCapacitiesAndDecreasingMaximum)
{
  router_->addTreeEdge(0, 1, 3, 1, 1, net_);
  router_->addTreeEdge(2, 0, 2, 3, 2, net_);
  router_->updateEdge2DAnd3DUsage(0, 1, 1, 1, 1, 8, net_);
  router_->updateEdge2DAnd3DUsage(1, 1, 2, 1, 1, 4, net_);
  auto totals = OverflowTestPeer::totals(*router_);
  EXPECT_EQ(totals[0].usage, 15);
  EXPECT_EQ(totals[0].max_overflow, 9);
  EXPECT_EQ(totals[1].usage, 3);
  router_->setEdgeCapacity(0, 1, 1, 1, 1, 7);
  totals = OverflowTestPeer::totals(*router_);
  EXPECT_EQ(totals[0].max_overflow, 5);
  router_->addAdjustment(1, 1, 2, 1, 1, 5, false);
  router_->addAdjustment(2, 0, 2, 1, 2, 3, false);
  totals = OverflowTestPeer::totals(*router_);
  EXPECT_EQ(totals[0].max_overflow, 2);
  router_->updateEdge2DAnd3DUsage(0, 1, 1, 1, 1, -8, net_);
  router_->updateEdge2DAnd3DUsage(1, 1, 2, 1, 1, -4, net_);
  expectReference();
  OverflowTestPeer::clearNetRoute(*router_);
  expectReference();
  OverflowTestPeer::graph(*router_).prepareForIncrementalRun();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  EXPECT_EQ(router_->totalOverflow(), 0);
}

TEST_F(OverflowTest, CountsOnlyUsedPlanarEdgesAcrossAllLayers)
{
  router_->incrementEdge3DUsage(1, 1, 2, 1, 1);
  router_->incrementEdge3DUsage(1, 1, 2, 1, 2);
  // 3D usage alone must not add this edge to the counted population.
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  auto& graph = OverflowTestPeer::graph(*router_);
  graph.addUsageH(1, 1, 1);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 2);
  graph.addUsageH(1, 1, -1);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 2);
  graph.prepareForIncrementalRun();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  graph.addUsageH(1, 1, 1);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 2);
  graph.clearUsed();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  graph.rebuildUsedGrids();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 2);
  expectReference();
}

TEST_F(OverflowTest, ClearUsedDefersRebuildThroughMutationsAndReconciliation)
{
  router_->addTreeEdge(0, 1, 1, 1, 1, net_);
  router_->addTreeEdge(2, 0, 2, 1, 2, net_);
  auto& graph = OverflowTestPeer::graph(*router_);
  // Drain pending changes so recovery depends on clearUsed marking old entries.
  graph.prepareForIncrementalRun();
  expectReference();
  ASSERT_TRUE(OverflowTestPeer::cacheValid(*router_));

  graph.clearUsed();
  EXPECT_FALSE(OverflowTestPeer::cacheValid(*router_));
  graph.clearUsed();

  // Change both directions and layers before any query rebuilds the cache.
  OverflowTestPeer::add3D(*router_, 0, 1, 0, EdgeDirection::Horizontal, 4);
  OverflowTestPeer::add3D(*router_, 0, 1, 1, EdgeDirection::Horizontal, 3);
  OverflowTestPeer::add3D(*router_, 2, 0, 1, EdgeDirection::Vertical, 2);
  router_->setEdgeCapacity(0, 1, 1, 1, 1, 2);
  router_->setEdgeCapacity(2, 0, 2, 1, 2, 1);
  // New membership must also be included when the cache is rebuilt.
  OverflowTestPeer::add3D(*router_, 3, 3, 0, EdgeDirection::Horizontal, 4);
  graph.addUsageH(3, 3, 1);
  graph.prepareForIncrementalRun();
  EXPECT_TRUE(graph.usedGridsMatchUsage());
  EXPECT_FALSE(OverflowTestPeer::cacheValid(*router_));

  const auto totals = OverflowTestPeer::totals(*router_);
  EXPECT_TRUE(OverflowTestPeer::cacheValid(*router_));
  EXPECT_EQ(totals[0].usage, 12);
  EXPECT_EQ(totals[0].capacity, 2);
  EXPECT_EQ(totals[0].overflow, 10);
  EXPECT_EQ(totals[0].max_overflow, 4);
  EXPECT_EQ(totals[1].usage, 3);
  EXPECT_EQ(totals[1].capacity, 1);
  EXPECT_EQ(totals[1].overflow, 2);
  expectReference();
}

TEST_F(OverflowTest, ResetAndCopyInvalidateTheOwnersCache)
{
  router_->addTreeEdge(0, 1, 2, 1, 1, net_);
  auto& graph = OverflowTestPeer::graph(*router_);
  graph.initCap3D();
  Graph2D empty;
  empty.init(kSize, kSize, 2, &logger_);
  empty.initCap3D();
  graph.copyRoutingStateFrom(empty, false);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  // The destination keeps its callback, rather than copying the source's.
  graph.addUsageH(0, 1, 1);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 1);
  router_->init3DEdges();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  router_->incrementEdge3DUsage(0, 1, 1, 1, 1);
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 1);
  router_->initEdges();
  EXPECT_EQ(OverflowTestPeer::overflow3D(*router_), 0);
  expectReference();
}

TEST_F(OverflowTest, RepeatedMutationsMatchIndependentScan)
{
  std::mt19937 random(9876);
  auto& graph = OverflowTestPeer::graph(*router_);
  for (int i = 0; i < 2000; i++) {
    const int x = random() % (kSize - 1);
    const int y = random() % (kSize - 1);
    const int layer = random() % 2;
    const bool horizontal = random() % 2;
    const auto direction
        = horizontal ? EdgeDirection::Horizontal : EdgeDirection::Vertical;
    const auto& edges = horizontal ? router_->getHorizontalEdges3D()
                                   : router_->getVerticalEdges3D();
    const int delta = edges[layer][y][x].usage > 0 && random() % 2 ? -1 : 1;
    OverflowTestPeer::add3D(*router_, x, y, layer, direction, delta);
    if (horizontal) {
      graph.addUsageH(x, y, 1);
    } else {
      graph.addUsageV(x, y, 1);
    }
    router_->setEdgeCapacity(
        x, y, x + horizontal, y + !horizontal, layer + 1, random() % 5);
    if (i % 29 == 0) {
      graph.clearUsed();
    }
    if (i % 7 == 0) {
      graph.prepareForIncrementalRun();
    }
    expectReference();
  }
}

TEST_F(OverflowTest, DebugCheckDetectsCounterDriftInRelease)
{
  OverflowTestPeer::corrupt(*router_);
  EXPECT_THROW(OverflowTestPeer::overflow3D(*router_), std::runtime_error);
}

}  // namespace
}  // namespace grt
