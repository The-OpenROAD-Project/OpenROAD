// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#include <memory>
#include <utility>
#include <vector>

#include "FastRoute.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/db_fixture.h"
#include "utl/ServiceRegistry.h"

namespace grt {
namespace {

// Exercises FastRouteCore::getCongestionNets on a small synthetic grid.
//
// The state getCongestionNets reads (routed tree edges and 2D edge usage)
// is built entirely through the public API: addNet() registers a net and
// its (empty) Steiner tree, addTreeEdge() appends a routed edge and bumps
// graph2d_ usage, and updateEdge2DAnd3DUsage() adds raw usage to create
// overflow at chosen cells without giving any scannable net a route there.
class CongestionNetsTest : public tst::DbFixture
{
 protected:
  static constexpr int kXGrid = 10;
  static constexpr int kYGrid = 10;
  static constexpr int kNumLayers = 2;
  // Per-layer edge capacity; the 2D capacity of an edge is the sum over
  // layers.  Routes added in tests use 1 unit per edge, so they never
  // overflow on their own.
  static constexpr int kEdgeCap = 10;
  static constexpr int kOverflowUsage = kEdgeCap * kNumLayers + 5;

  void SetUp() override
  {
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "L1", odb::dbTechLayerType::MASTERSLICE);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");

    fastroute_ = std::make_unique<FastRouteCore>(
        db_.get(), &logger_, &registry_, &stt_builder_, /*sta=*/nullptr);
    fastroute_->setLowerLeft(0, 0);
    fastroute_->setTileSize(100);
    fastroute_->setGridsAndLayers(kXGrid, kYGrid, kNumLayers);
    fastroute_->initEdges();
    for (int layer = 1; layer <= kNumLayers; layer++) {
      for (int y = 0; y < kYGrid; y++) {
        for (int x = 0; x < kXGrid - 1; x++) {
          fastroute_->setEdgeCapacity(x, y, x + 1, y, layer, kEdgeCap);
        }
      }
      for (int y = 0; y < kYGrid - 1; y++) {
        for (int x = 0; x < kXGrid; x++) {
          fastroute_->setEdgeCapacity(x, y, x, y + 1, layer, kEdgeCap);
        }
      }
    }

    // Carrier net for injecting overflow usage.  It is a clock net with no
    // tree edges, so getCongestionNets never selects it.
    overflow_net_ = makeFrNet("__overflow_carrier", /*is_clock=*/true);
  }

  odb::dbNet* makeFrNet(const char* name, const bool is_clock = false)
  {
    odb::dbNet* db_net = odb::dbNet::create(block_, name);
    fastroute_->addNet(db_net,
                       is_clock,
                       /*is_local=*/false,
                       /*driver_idx=*/0,
                       /*cost=*/1,
                       /*min_layer=*/0,
                       /*max_layer=*/kNumLayers - 1,
                       /*slack=*/0,
                       /*edge_cost_per_layer=*/nullptr);
    return db_net;
  }

  // Routes on layer 1; coordinates must be ascending.
  void addRouteH(odb::dbNet* net, const int x1, const int x2, const int y)
  {
    fastroute_->addTreeEdge(x1, y, x2, y, /*layer=*/1, net);
  }

  void addRouteV(odb::dbNet* net, const int x, const int y1, const int y2)
  {
    fastroute_->addTreeEdge(x, y1, x, y2, /*layer=*/1, net);
  }

  void overflowH(const int x, const int y)
  {
    fastroute_->updateEdge2DAnd3DUsage(
        x, y, x + 1, y, /*layer=*/1, kOverflowUsage, overflow_net_);
  }

  void overflowV(const int x, const int y)
  {
    fastroute_->updateEdge2DAnd3DUsage(
        x, y, x, y + 1, /*layer=*/1, kOverflowUsage, overflow_net_);
  }

  odb::PtrSet<odb::dbNet> getCongestionNets()
  {
    odb::PtrSet<odb::dbNet> nets;
    fastroute_->getCongestionNets(nets);
    return nets;
  }

  size_t overflowPositionCount()
  {
    std::vector<std::pair<odb::Point, bool>> positions;
    fastroute_->getOverflowPositions(positions);
    return positions.size();
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_builder_{&logger_};
  std::unique_ptr<FastRouteCore> fastroute_;
  odb::dbNet* overflow_net_ = nullptr;
};

TEST_F(CongestionNetsTest, FindsNetsCrossingOverflowAtRadiusZero)
{
  odb::dbNet* h_net = makeFrNet("h_net");
  addRouteH(h_net, 2, 5, 3);
  odb::dbNet* v_net = makeFrNet("v_net");
  addRouteV(v_net, 7, 2, 5);

  overflowH(3, 3);
  overflowV(7, 3);
  EXPECT_EQ(overflowPositionCount(), 2u);

  const auto nets = getCongestionNets();
  EXPECT_EQ(nets.size(), 2u);
  EXPECT_EQ(nets.count(h_net), 1u);
  EXPECT_EQ(nets.count(v_net), 1u);
}

TEST_F(CongestionNetsTest, HorizontalOverflowIgnoresVerticalOnlyNet)
{
  odb::dbNet* v_net = makeFrNet("v_net");
  addRouteV(v_net, 5, 3, 7);

  // Horizontal overflow on a cell the vertical route passes through.
  overflowH(5, 5);

  EXPECT_TRUE(getCongestionNets().empty());
}

TEST_F(CongestionNetsTest, VerticalOverflowIgnoresHorizontalOnlyNet)
{
  odb::dbNet* h_net = makeFrNet("h_net");
  addRouteH(h_net, 3, 7, 5);

  overflowV(5, 5);

  EXPECT_TRUE(getCongestionNets().empty());
}

TEST_F(CongestionNetsTest, ClockNetsExcluded)
{
  odb::dbNet* clk_net = makeFrNet("clk", /*is_clock=*/true);
  addRouteH(clk_net, 2, 6, 4);

  overflowH(3, 4);

  EXPECT_TRUE(getCongestionNets().empty());
}

TEST_F(CongestionNetsTest, NoOverflowFindsNothing)
{
  odb::dbNet* h_net = makeFrNet("h_net");
  addRouteH(h_net, 2, 6, 4);

  EXPECT_EQ(overflowPositionCount(), 0u);
  EXPECT_TRUE(getCongestionNets().empty());
}

TEST_F(CongestionNetsTest, RadiusExpandsUntilNetFound)
{
  // Route at Chebyshev distance 2 from the overflow cell: only reachable
  // once the search radius expands to 2.
  odb::dbNet* net = makeFrNet("net");
  addRouteH(net, 4, 7, 7);

  overflowH(5, 5);

  const auto nets = getCongestionNets();
  EXPECT_EQ(nets.size(), 1u);
  EXPECT_EQ(nets.count(net), 1u);
}

TEST_F(CongestionNetsTest, NetsBeyondMaxRadiusNotFound)
{
  // Chebyshev distance 6 exceeds the maximum search radius of 4.
  odb::dbNet* far_net = makeFrNet("far_net");
  addRouteH(far_net, 2, 6, 8);

  overflowH(3, 2);

  EXPECT_TRUE(getCongestionNets().empty());
}

TEST_F(CongestionNetsTest, RadiusStopsExpandingOnceNetsFound)
{
  odb::dbNet* near_net = makeFrNet("near_net");
  addRouteH(near_net, 2, 6, 4);
  odb::dbNet* next_net = makeFrNet("next_net");
  addRouteH(next_net, 2, 6, 5);

  overflowH(3, 4);

  // near_net is found at radius 0, so the radius never grows to reach
  // next_net one row away.
  const auto nets = getCongestionNets();
  EXPECT_EQ(nets.size(), 1u);
  EXPECT_EQ(nets.count(near_net), 1u);
}

TEST_F(CongestionNetsTest, AlreadyAddedNetsDontStopRadiusExpansion)
{
  odb::dbNet* seeded_net = makeFrNet("seeded_net");
  addRouteH(seeded_net, 2, 6, 4);
  odb::dbNet* found_net = makeFrNet("found_net");
  addRouteH(found_net, 2, 6, 7);

  overflowH(3, 4);

  // seeded_net crosses the overflow but is already in the set; the radius
  // keeps expanding until found_net (distance 3) is picked up.
  odb::PtrSet<odb::dbNet> nets;
  nets.insert(seeded_net);
  fastroute_->getCongestionNets(nets);
  EXPECT_EQ(nets.size(), 2u);
  EXPECT_EQ(nets.count(found_net), 1u);
}

TEST_F(CongestionNetsTest, AllNetsOnOverflowCellFound)
{
  odb::dbNet* net_a = makeFrNet("net_a");
  addRouteH(net_a, 2, 6, 4);
  odb::dbNet* net_b = makeFrNet("net_b");
  addRouteH(net_b, 3, 7, 4);

  overflowH(4, 4);

  const auto nets = getCongestionNets();
  EXPECT_EQ(nets.size(), 2u);
  EXPECT_EQ(nets.count(net_a), 1u);
  EXPECT_EQ(nets.count(net_b), 1u);
}

}  // namespace
}  // namespace grt
