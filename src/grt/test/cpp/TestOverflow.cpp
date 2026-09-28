// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cmath>
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
  struct Result2D
  {
    int overflow;
    int maximum;
    int usage;
    int threshold;
    bool operator==(const Result2D&) const = default;
  };
  static void setUsageLimits(FastRouteCore& router, int capacity)
  {
    router.h_capacity_ = capacity;
    router.v_capacity_ = capacity;
  }
  static Result2D query2D(FastRouteCore& router, bool estimated, bool reference)
  {
    int maximum;
    int usage = 0;
    int overflow;
    if (estimated) {
      overflow = reference ? router.scanOverflow2D(&maximum)
                           : router.getOverflow2D(&maximum);
    } else {
      overflow = reference ? router.scanOverflow2Dmaze(&maximum, &usage)
                           : router.getOverflow2Dmaze(&maximum, &usage);
    }
    return {overflow, maximum, usage, router.ahth_};
  }
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
    fr_net_ = router_->addNet(net_, false, false, 0, 1, 0, 1, 0, nullptr);
    OverflowTestPeer::setUsageLimits(*router_, 1000);
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
  FrNet* fr_net_ = nullptr;
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

TEST_F(OverflowTest, TwoDimensionalCountersMatchLegacyScans)
{
  auto& graph = OverflowTestPeer::graph(*router_);
  std::mt19937 random(2345);
  for (int i = 0; i < 1000; i++) {
    const int x = random() % (kSize - 1);
    const int y = random() % (kSize - 1);
    graph.addUsageH(x, y, 2);
    graph.addUsageV(x, y, 3);
    graph.updateEstUsageH(x, y, fr_net_, 0.5);
    graph.updateEstUsageV(x, y, fr_net_, 1.5);
    graph.addCapH(x, y, 1);
    graph.addCapV(x, y, 1);
    for (bool estimated : {false, true}) {
      EXPECT_EQ(OverflowTestPeer::query2D(*router_, estimated, false),
                OverflowTestPeer::query2D(*router_, estimated, true));
    }
    if (i % 3 == 0) {
      graph.addEstUsageToUsage();
      graph.InitEstUsage();
    }
    if (i % 5 == 0) {
      graph.addUsageH(x, y, -graph.getUsageH(x, y));
      graph.addUsageV(x, y, -graph.getUsageV(x, y));
      graph.prepareForIncrementalRun();
    }
    for (bool estimated : {false, true}) {
      EXPECT_EQ(OverflowTestPeer::query2D(*router_, estimated, false),
                OverflowTestPeer::query2D(*router_, estimated, true));
    }
  }
}

TEST_F(OverflowTest, EstimatedUsageTruncationAndNegativeFallback)
{
  auto& graph = OverflowTestPeer::graph(*router_);
  graph.updateEstUsageH(0, 0, fr_net_, 0.5);
  graph.updateEstUsageH(1, 0, fr_net_, 0.5);
  EXPECT_EQ(graph.overflowStatistics(true)[0].usage, 0);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false).overflow, 0);
  graph.InitEstUsage();
  graph.updateEstUsageH(0, 0, fr_net_, 800001.5);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false).threshold, 30);
  graph.updateEstUsageV(0, 0, fr_net_, 0.5);
  graph.updateEstUsageV(0, 0, fr_net_, -1);
  ASSERT_TRUE(graph.needsEstimatedUsageScan());
  // 800001 + (-0.5) truncates to 800000 in the original ordered scan.
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false).threshold, 20);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false),
            OverflowTestPeer::query2D(*router_, true, true));
  graph.InitEstUsage();
  ASSERT_FALSE(graph.needsEstimatedUsageScan());
  graph.updateEstUsageH(0, 0, fr_net_, -0.5);
  graph.updateEstUsageV(0, 0, fr_net_, 800001.5);
  // Reversing the signs changes the ordered result: trunc(0 - 0.5) is zero.
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false).threshold, 30);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false),
            OverflowTestPeer::query2D(*router_, true, true));
}

TEST_F(OverflowTest, PreservesUsageLimitErrorsWithoutDebugScans)
{
  logger_.setDebugLevel(utl::GRT, "overflowcheck", 0);
  auto& graph = OverflowTestPeer::graph(*router_);
  OverflowTestPeer::setUsageLimits(*router_, 0);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, false, false).overflow, 0);
  graph.addUsageH(1, 1, 1);
  try {
    OverflowTestPeer::query2D(*router_, false, false);
    FAIL() << "Expected the original horizontal usage error";
  } catch (const std::runtime_error& error) {
    EXPECT_STREQ(error.what(), "GRT-0228");
  }
  graph.addUsageH(1, 1, -1);
  graph.addUsageV(1, 1, 1);
  try {
    OverflowTestPeer::query2D(*router_, true, false);
    FAIL() << "Expected the original vertical usage error";
  } catch (const std::runtime_error& error) {
    EXPECT_STREQ(error.what(), "GRT-0229");
  }
  graph.addUsageV(1, 1, -1);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, false, false).usage, 0);
}

TEST_F(OverflowTest, PreservesRoundingOfPositiveNonHalfEstimates)
{
  auto& graph = OverflowTestPeer::graph(*router_);
  graph.updateEstUsageH(0, 0, fr_net_, 800000);
  graph.updateEstUsageH(1, 0, fr_net_, std::nextafter(1.0, 0.0));
  ASSERT_TRUE(graph.needsEstimatedUsageScan());
  // The legacy double addition rounds up to 800001 before conversion to int.
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false).threshold, 30);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false),
            OverflowTestPeer::query2D(*router_, true, true));
}

TEST_F(OverflowTest, AccountsForNDRPenaltiesAndCancellation)
{
  auto& graph = OverflowTestPeer::graph(*router_);
  router_->initEdgesCapacityPerLayer();
  fr_net_->setEdgeCost(2);
  graph.overflowStatistics(true);
  graph.updateEstUsageH(0, 0, fr_net_, 0.5);
  graph.updateEstUsageH(0, 0, fr_net_, 0.5);
  EXPECT_EQ(graph.overflowStatistics(true)[0].usage, 200);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, true, false),
            OverflowTestPeer::query2D(*router_, true, true));
  graph.updateEstUsageH(0, 0, fr_net_, -0.5);
  graph.InitEstUsage();
  graph.updateUsageH(0, 0, fr_net_, 1);
  EXPECT_EQ(graph.overflowStatistics(false)[0].usage, 200);
  graph.updateUsageH(0, 0, fr_net_, -1);
  EXPECT_EQ(OverflowTestPeer::query2D(*router_, false, false).usage, 0);
  graph.prepareForIncrementalRun();
  EXPECT_EQ(graph.overflowStatistics(false)[0].capacity, 0);
}

}  // namespace
}  // namespace grt
