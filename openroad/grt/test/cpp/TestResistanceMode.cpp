// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "FastRoute.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "tst/physical_builder.h"
#include "utl/ServiceRegistry.h"

namespace grt {

class ResistanceModeTestPeer
{
 public:
  static void initialize(FastRouteCore& router)
  {
    router.preProcessTechLayers();
    router.via_cost_ = 1;
  }

  static bool active(const FastRouteCore& router)
  {
    return router.resistance_aware_;
  }

  static void leaveStaleScope(FastRouteCore& router)
  {
    router.resistance_aware_ = true;
  }

  static int wire(FastRouteCore& router, FrNet* net)
  {
    return router.getWireCost(0, router.tile_size_, net);
  }

  static int via(FastRouteCore& router) { return router.getViaCost(0, 2); }

  static float mazeWire(FastRouteCore& router, int net_id)
  {
    return router.getMazeRouteCost3D(net_id, 0, 0, 0, 0, 1, 0, false);
  }

  static float mazeVia(FastRouteCore& router, int net_id)
  {
    return router.getMazeRouteCost3D(net_id, 0, 1, 0, 0, 0, 0, true);
  }

  static void setTree(FastRouteCore& router, int net_id, StTree tree)
  {
    router.sttrees_[net_id] = std::move(tree);
  }

  static const StTree& tree(const FastRouteCore& router, int net_id)
  {
    return router.sttrees_[net_id];
  }

  static void assign(FastRouteCore& router, int net_id, bool forward)
  {
    router.assignEdge(net_id, 0, forward);
  }

  static std::vector<OrderNetPin> order(FastRouteCore& router)
  {
    router.netpinOrderInc();
    return router.tree_order_pv_;
  }

  static void updateWorst(FastRouteCore& router, FrNet* net)
  {
    router.updateWorstMetrics(net);
  }

  static void resetWorst(FastRouteCore& router) { router.resetWorstMetrics(); }

  static auto worst(const FastRouteCore& router)
  {
    return std::make_tuple(router.worst_net_resistance_,
                           router.worst_slack_,
                           router.worst_net_length_,
                           router.worst_fanout_);
  }

  static void visitMazeScopes(FastRouteCore& router)
  {
    router.tree_order_pv_.clear();
    for (const int id : router.net_ids_) {
      OrderNetPin order{};
      order.treeIndex = id;
      router.tree_order_pv_.push_back(order);
    }
    // Exercise per-net scope entry, without doing a rip-up or maze search.
    router.mazeRouteMSMDOrder3D(
        0, FastRouteCore::BIG_INT, FastRouteCore::BIG_INT);
  }
};

namespace {

class ResistanceModeTest : public tst::Fixture
{
 protected:
  static constexpr int kNumLayers = 3;
  static constexpr int kLength = 12;
  static constexpr int kTileSize = 2000;

  void SetUp() override
  {
    auto* tech = tst::makeTech(db_.get());
    block_ = tst::makeBlock(db_.get(), tech);
    auto layers
        = tst::makeLayerStack(tech, kNumLayers, {.width = 1000, .pitch = 2000});
    for (auto* metal : layers.metals) {
      metal->setResistance(10.0);
    }
    layers.metal(3)->setResistance(2.0);
    for (auto* cut : layers.cuts) {
      cut->setResistance(10.0);
    }

    router_ = std::make_unique<FastRouteCore>(
        db_.get(), &logger_, &registry_, &stt_builder_, getSta());
    router_->setTileSize(kTileSize);
    router_->setGridsAndLayers(kLength + 1, 2, kNumLayers);
    for (int layer = 0; layer < kNumLayers; layer++) {
      router_->addLayerDirection(layer, layers.metals[layer]->getDirection());
    }
    router_->initEdges();
    router_->initAuxVar();
    ResistanceModeTestPeer::initialize(*router_);
    for (int layer = 1; layer <= kNumLayers; layer++) {
      for (int x = 0; x < kLength; x++) {
        router_->setEdgeCapacity(x, 0, x + 1, 0, layer, 10);
      }
    }
  }

  FrNet* addNet(const char* name,
                bool selected,
                int length = kLength,
                int min_x = 0,
                bool clock = false)
  {
    auto* net = router_->addNet(odb::dbNet::create(block_, name),
                                clock,
                                /*is_local=*/false,
                                /*driver_idx=*/0,
                                /*cost=*/1,
                                /*min_layer=*/0,
                                /*max_layer=*/kNumLayers - 1,
                                /*slack=*/-10,
                                /*edge_cost_per_layer=*/nullptr);
    net->setIsResAware(selected);
    net->setResistance(10.0);
    net->setNetLength(length);
    net->addPin(min_x, 0, 0);
    net->addPin(min_x + length, 0, 0);

    StTree tree;
    tree.num_terminals = 2;
    for (int id = 0; id < 2; id++) {
      TreeNode node{};
      node.x = min_x + id * length;
      node.y = 0;
      node.botL = node.topL = 0;
      node.stackAlias = id;
      node.assigned = true;
      tree.nodes.push_back(node);
      tree.node_to_pin_idx[id] = id;
    }
    TreeEdge edge{};
    edge.n1 = edge.n1a = 0;
    edge.n2 = edge.n2a = 1;
    edge.len = edge.route.routelen = length;
    edge.route.type = RouteType::MazeRoute;
    for (int x = min_x; x <= min_x + length; x++) {
      edge.route.grids.push_back({static_cast<int16_t>(x), 0, 0});
    }
    tree.edges.push_back(std::move(edge));
    ResistanceModeTestPeer::setTree(*router_, netId(net), std::move(tree));
    ResistanceModeTestPeer::updateWorst(*router_, net);
    return net;
  }

  int netId(FrNet* net)
  {
    int id = -1;
    bool exists = false;
    router_->getNetId(net->getDbNet(), id, exists);
    EXPECT_TRUE(exists);
    return id;
  }

  void expectAssignment(FrNet* net, int layer, int lower_usage, int upper_usage)
  {
    const auto& tree = ResistanceModeTestPeer::tree(*router_, netId(net));
    ASSERT_EQ(tree.edges.size(), 1);
    const auto& edge = tree.edges.front();
    ASSERT_TRUE(edge.assigned);
    ASSERT_EQ(edge.route.grids.size(), kLength + 1);
    for (int x = 0; x <= kLength; x++) {
      SCOPED_TRACE(x);
      EXPECT_EQ(edge.route.grids[x].x, x);
      EXPECT_EQ(edge.route.grids[x].y, 0);
      EXPECT_EQ(edge.route.grids[x].layer, layer);
    }
    for (int x = 0; x < kLength; x++) {
      EXPECT_EQ(router_->getAvailableResources(x, 0, x + 1, 0, 1),
                10 - lower_usage);
      EXPECT_EQ(router_->getAvailableResources(x, 0, x + 1, 0, 3),
                10 - upper_usage);
    }
  }

  void expectOrder(FrNet* first, FrNet* second, bool zero_scores)
  {
    const auto order = ResistanceModeTestPeer::order(*router_);
    ASSERT_EQ(order.size(), 2);
    EXPECT_EQ(order[0].treeIndex, netId(first));
    EXPECT_EQ(order[1].treeIndex, netId(second));
    for (const auto& entry : order) {
      EXPECT_TRUE(std::isfinite(entry.res_aware_score));
      if (zero_scores) {
        EXPECT_FLOAT_EQ(entry.res_aware_score, 0.0f);
      }
    }
  }

  void expectInactiveCosts(FrNet* net)
  {
    EXPECT_FALSE(ResistanceModeTestPeer::active(*router_));
    EXPECT_EQ(ResistanceModeTestPeer::wire(*router_, net), 0);
    EXPECT_EQ(ResistanceModeTestPeer::via(*router_), 0);
    EXPECT_FLOAT_EQ(ResistanceModeTestPeer::mazeWire(*router_, netId(net)), 1);
    EXPECT_FLOAT_EQ(ResistanceModeTestPeer::mazeVia(*router_, netId(net)), 1);
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_builder_{&logger_};
  std::unique_ptr<FastRouteCore> router_;
};

TEST_F(ResistanceModeTest, InitialDisabledModeHasNoActiveCost)
{
  auto* net = addNet("selected", true);
  expectInactiveCosts(net);
  router_->setResistanceAware(true);
  expectInactiveCosts(net);
}

TEST_F(ResistanceModeTest, DisabledStaleSelectionKeepsLegacyOrder)
{
  auto* short_net = addNet("short", false, 2);
  auto* selected = addNet("selected", true);
  router_->setResistanceAware(false);
  expectOrder(short_net, selected, true);
  EXPECT_TRUE(selected->isResAware());
}

TEST_F(ResistanceModeTest,
       DisabledStaleSelectionWithZeroWorstMetricsHasFiniteScores)
{
  auto* short_net = addNet("short", true, 2);
  auto* selected = addNet("selected", true);
  short_net->setResistance(0.0);
  selected->setResistance(0.0);
  ResistanceModeTestPeer::resetWorst(*router_);
  const auto worst = ResistanceModeTestPeer::worst(*router_);
  router_->setResistanceAware(false);
  expectOrder(short_net, selected, true);
  EXPECT_EQ(ResistanceModeTestPeer::worst(*router_), worst);
}

TEST_F(ResistanceModeTest, PublicModeChangesRestoreLegacyOrderOnReuse)
{
  auto* short_net = addNet("short", false, 2);
  auto* selected = addNet("selected", true);
  router_->setResistanceAware(true);
  expectOrder(selected, short_net, false);
  router_->setResistanceAware(false);
  expectOrder(short_net, selected, true);
  router_->setResistanceAware(true);
  expectOrder(selected, short_net, false);
  router_->setResistanceAware(false);
  expectOrder(short_net, selected, true);
}

TEST_F(ResistanceModeTest, EnabledUnselectedNetHasNoResistanceScore)
{
  auto* short_net = addNet("short", false, 2);
  auto* long_net = addNet("long", false);
  ResistanceModeTestPeer::resetWorst(*router_);
  router_->setResistanceAware(true);
  expectOrder(short_net, long_net, true);
}

TEST_F(ResistanceModeTest, DisabledClockKeepsLegacyOrder)
{
  auto* short_net = addNet("short", false, 2);
  auto* clock = addNet("clock", false, kLength, 0, true);
  router_->setResistanceAware(false);
  expectOrder(short_net, clock, true);
  router_->setResistanceAware(true);
  expectOrder(clock, short_net, true);
}

TEST_F(ResistanceModeTest, DisabledModeRetainsNdrPriority)
{
  auto* short_net = addNet("short", false, 2);
  auto* ndr_net = addNet("ndr", false);
  auto* ndr = odb::dbTechNonDefaultRule::create(block_, "wide");
  ndr_net->getDbNet()->setNonDefaultRule(ndr);
  router_->setResistanceAware(false);
  expectOrder(ndr_net, short_net, true);
}

TEST_F(ResistanceModeTest, DisabledTiesUseTreeId)
{
  auto* first = addNet("first", true, 2);
  auto* second = addNet("second", true, 2);
  second->setResistance(100.0);
  ResistanceModeTestPeer::updateWorst(*router_, second);
  router_->setResistanceAware(false);
  expectOrder(first, second, true);
}

TEST_F(ResistanceModeTest, DisabledLengthTiesUseMinX)
{
  auto* right = addNet("right", true, 2, 4);
  auto* left = addNet("left", false, 2);
  router_->setResistanceAware(false);
  expectOrder(left, right, true);
}

TEST_F(ResistanceModeTest, MazeClearsStaleScopeWhileDisabled)
{
  auto* first = addNet("first", true);
  addNet("last", true);
  router_->setResistanceAware(false);
  // Model an interrupted prior selected-net scope at the maze entry boundary.
  ResistanceModeTestPeer::leaveStaleScope(*router_);
  ResistanceModeTestPeer::visitMazeScopes(*router_);
  expectInactiveCosts(first);
}

TEST_F(ResistanceModeTest, MazeUsesSelectedAndUnselectedScopesOnReuse)
{
  auto* first = addNet("first", true);
  auto* last = addNet("last", true);
  router_->setResistanceAware(true);
  ResistanceModeTestPeer::visitMazeScopes(*router_);
  ASSERT_TRUE(ResistanceModeTestPeer::active(*router_));
  router_->setResistanceAware(false);
  expectInactiveCosts(first);

  first->setIsResAware(false);
  last->setIsResAware(false);
  router_->setResistanceAware(true);
  ResistanceModeTestPeer::visitMazeScopes(*router_);
  expectInactiveCosts(last);

  first->setIsResAware(true);
  last->setIsResAware(true);
  ResistanceModeTestPeer::visitMazeScopes(*router_);
  EXPECT_TRUE(ResistanceModeTestPeer::active(*router_));
}

class ResistanceModeAssignmentTest : public ResistanceModeTest,
                                     public testing::WithParamInterface<bool>
{
};

TEST_P(ResistanceModeAssignmentTest,
       SelectedAssignmentUsesExistingResistanceCosts)
{
  auto* net = addNet("selected", true);
  router_->setResistanceAware(true);
  // Existing costs: M1 costs 12*3=36; M3 costs 12*2+6+4=34.
  ResistanceModeTestPeer::assign(*router_, netId(net), GetParam());
  expectAssignment(net, 2, 0, 1);
  EXPECT_TRUE(ResistanceModeTestPeer::active(*router_));
  EXPECT_GT(ResistanceModeTestPeer::wire(*router_, net), 0);
  EXPECT_GT(ResistanceModeTestPeer::via(*router_), 0);
}

TEST_P(ResistanceModeAssignmentTest, UnselectedAssignmentKeepsLegacyLayer)
{
  auto* net = addNet("unselected", false);
  router_->setResistanceAware(true);
  ResistanceModeTestPeer::assign(*router_, netId(net), GetParam());
  expectAssignment(net, 0, 1, 0);
  expectInactiveCosts(net);
}

TEST_P(ResistanceModeAssignmentTest, DisabledSelectedAssignmentKeepsLegacyLayer)
{
  auto* net = addNet("selected", true);
  router_->setResistanceAware(false);
  ResistanceModeTestPeer::assign(*router_, netId(net), GetParam());
  expectAssignment(net, 0, 1, 0);
  expectInactiveCosts(net);
}

TEST_P(ResistanceModeAssignmentTest, AssignmentClearsStaleScopeWhenDisabled)
{
  auto* net = addNet("selected", true);
  router_->setResistanceAware(false);
  ResistanceModeTestPeer::leaveStaleScope(*router_);
  ResistanceModeTestPeer::assign(*router_, netId(net), GetParam());
  expectAssignment(net, 0, 1, 0);
  expectInactiveCosts(net);
}

TEST_P(ResistanceModeAssignmentTest, PublicModesRespectSelectionOnReuse)
{
  auto* selected = addNet("selected", true);
  router_->setResistanceAware(true);
  ResistanceModeTestPeer::assign(*router_, netId(selected), GetParam());
  expectAssignment(selected, 2, 0, 1);
  ASSERT_TRUE(ResistanceModeTestPeer::active(*router_));

  auto* ordinary = addNet("ordinary", false);
  ResistanceModeTestPeer::assign(*router_, netId(ordinary), GetParam());
  expectAssignment(ordinary, 0, 1, 1);
  expectInactiveCosts(ordinary);

  auto* selected_again = addNet("selected_again", true);
  ResistanceModeTestPeer::assign(*router_, netId(selected_again), GetParam());
  expectAssignment(selected_again, 2, 1, 2);
  ASSERT_TRUE(ResistanceModeTestPeer::active(*router_));
  router_->setResistanceAware(false);
  EXPECT_TRUE(selected_again->isResAware());
  expectInactiveCosts(selected_again);
  router_->setResistanceAware(false);
  expectInactiveCosts(selected_again);

  auto* stale_selected = addNet("disabled_selected", true);
  ResistanceModeTestPeer::assign(*router_, netId(stale_selected), GetParam());
  expectAssignment(stale_selected, 0, 2, 2);
  expectInactiveCosts(stale_selected);

  router_->setResistanceAware(true);
  expectInactiveCosts(stale_selected);
  auto* unselected_after_enable = addNet("unselected_after_enable", false);
  ResistanceModeTestPeer::assign(
      *router_, netId(unselected_after_enable), GetParam());
  expectAssignment(unselected_after_enable, 0, 3, 2);
  expectInactiveCosts(unselected_after_enable);

  auto* selected_after_enable = addNet("selected_after_enable", true);
  ResistanceModeTestPeer::assign(
      *router_, netId(selected_after_enable), GetParam());
  expectAssignment(selected_after_enable, 2, 3, 3);
  EXPECT_TRUE(ResistanceModeTestPeer::active(*router_));
}

INSTANTIATE_TEST_SUITE_P(BothDirections,
                         ResistanceModeAssignmentTest,
                         testing::Bool());

}  // namespace
}  // namespace grt
