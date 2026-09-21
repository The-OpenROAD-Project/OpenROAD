// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "FastRoute.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/db_fixture.h"
#include "utl/ServiceRegistry.h"

namespace grt {

class Maze3DTestPeer
{
 public:
  static void setViaCost(FastRouteCore& router, int cost)
  {
    router.via_cost_ = cost;
  }

  static void setTree(FastRouteCore& router, int net_id, StTree tree)
  {
    router.sttrees_[net_id] = std::move(tree);
  }

  static StTree getTree(const FastRouteCore& router, int net_id)
  {
    return router.sttrees_[net_id];
  }

  static void route(FastRouteCore& router, const std::vector<int>& net_ids)
  {
    router.tree_order_pv_.clear();
    for (int net_id : net_ids) {
      OrderNetPin order{};
      order.treeIndex = net_id;
      router.tree_order_pv_.push_back(order);
    }
    router.mazeRouteMSMDOrder3D(
        /*expand=*/3, /*ripupTHlb=*/0, /*ripupTHub=*/10);
  }

  static std::vector<int> getUsage(const FastRouteCore& router)
  {
    std::vector<int> result;
    for (int y = 0; y < router.y_grid_; y++) {
      for (int x = 0; x < router.x_grid_ - 1; x++) {
        result.push_back(router.graph2d_.getUsageH(x, y));
        for (int layer = 0; layer < router.num_layers_; layer++) {
          result.push_back(router.h_edges_3D_[layer][y][x].usage);
        }
      }
    }
    for (int y = 0; y < router.y_grid_ - 1; y++) {
      for (int x = 0; x < router.x_grid_; x++) {
        result.push_back(router.graph2d_.getUsageV(x, y));
        for (int layer = 0; layer < router.num_layers_; layer++) {
          result.push_back(router.v_edges_3D_[layer][y][x].usage);
        }
      }
    }
    return result;
  }

  struct SearchState
  {
    bool source_empty;
    bool destination_marks_clear;
    bool region_clear;
  };

  static SearchState getSearchState(const FastRouteCore& router)
  {
    const auto marked = [](bool value) { return value; };
    return {
        router.src_heap_3D_.empty(),
        !std::any_of(
            router.pop_heap2_3D_.begin(), router.pop_heap2_3D_.end(), marked),
        !std::any_of(
            router.in_region_.data(),
            router.in_region_.data() + router.in_region_.num_elements(),
            marked)};
  }
};

namespace {

struct RoutedEdge
{
  int n1;
  int n2;
  std::vector<GPoint3D> grids;
};

// Exercise the 3D search directly with synthetic routed trees.
class Maze3DTest : public tst::DbFixture
{
 protected:
  static constexpr int kXGrid = 3;
  static constexpr int kYGrid = 4;
  static constexpr int kNumLayers = 2;

  void SetUp() override
  {
    auto* tech = odb::dbTech::create(db_.get(), "tech");
    auto* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    router_ = std::make_unique<FastRouteCore>(
        db_.get(), &logger_, &registry_, &stt_builder_, nullptr);
    router_->setTileSize(100);
    router_->setGridsAndLayers(kXGrid, kYGrid, kNumLayers);
    router_->addLayerDirection(0, odb::dbTechLayerDir::HORIZONTAL);
    router_->addLayerDirection(1, odb::dbTechLayerDir::VERTICAL);
    router_->initEdges();
    router_->initAuxVar();
    router_->setIncrementalGrt(true);
    Maze3DTestPeer::setViaCost(*router_, 1);
    for (int y = 0; y < kYGrid; y++) {
      for (int x = 0; x < kXGrid - 1; x++) {
        router_->setEdgeCapacity(x, y, x + 1, y, 1, 10);
      }
    }
    for (int y = 0; y < kYGrid - 1; y++) {
      for (int x = 0; x < kXGrid; x++) {
        router_->setEdgeCapacity(x, y, x, y + 1, 2, 10);
      }
    }
    skipped_net_ = addNet("skipped", {});
  }

  int addNet(const char* name, const std::vector<GPoint3D>& grids)
  {
    if (grids.empty()) {
      return addTreeNet(name, {}, {}, {});
    }
    return addTreeNet(name, {grids.front(), grids.back()}, {}, {{0, 1, grids}});
  }

  int addTreeNet(const char* name,
                 const std::vector<GPoint3D>& pins,
                 const std::vector<GPoint3D>& steiner_points,
                 const std::vector<RoutedEdge>& edges)
  {
    auto* db_net = odb::dbNet::create(block_, name);
    FrNet* net = router_->addNet(db_net,
                                 /*is_clock=*/false,
                                 /*is_local=*/false,
                                 /*driver_idx=*/0,
                                 /*cost=*/1,
                                 /*min_layer=*/0,
                                 /*max_layer=*/kNumLayers - 1,
                                 /*slack=*/0,
                                 /*edge_cost_per_layer=*/nullptr);
    int net_id;
    bool exists;
    router_->getNetId(db_net, net_id, exists);
    EXPECT_TRUE(exists);
    for (const auto& pin : pins) {
      net->addPin(pin.x, pin.y, pin.layer);
    }

    StTree tree;
    tree.num_terminals = pins.size();
    std::vector<GPoint3D> points = pins;
    points.insert(points.end(), steiner_points.begin(), steiner_points.end());
    const int num_points = static_cast<int>(points.size());
    for (int i = 0; i < num_points; i++) {
      const auto& point = points[i];
      if (i < tree.num_terminals) {
        tree.node_to_pin_idx[i] = i;
      }
      TreeNode node{};
      node.x = point.x;
      node.y = point.y;
      node.stackAlias = i;
      node.assigned = true;
      std::ranges::fill(node.nbr, -1);
      std::ranges::fill(node.edge, -1);
      node.botL = node.topL = point.layer;
      tree.nodes.push_back(node);
    }
    for (const auto& routed_edge : edges) {
      const auto& grids = routed_edge.grids;
      const int edge_id = tree.edges.size();
      TreeEdge edge;
      edge.n1 = edge.n1a = routed_edge.n1;
      edge.n2 = edge.n2a = routed_edge.n2;
      edge.assigned = true;
      edge.len = std::abs(grids.front().x - grids.back().x)
                 + std::abs(grids.front().y - grids.back().y);
      edge.route.type = RouteType::MazeRoute;
      edge.route.grids = grids;
      edge.route.routelen = grids.size() - 1;
      for (int node_id : {edge.n1, edge.n2}) {
        auto& node = tree.nodes[node_id];
        const int layer
            = node_id == edge.n1 ? grids.front().layer : grids.back().layer;
        node.nbr[node.nbr_count] = node_id == edge.n1 ? edge.n2 : edge.n1;
        node.edge[node.nbr_count++] = edge_id;
        node.eID[node.conCNT] = edge_id;
        node.heights[node.conCNT++] = layer;
        node.botL = std::min<int>(node.botL, layer);
        node.topL = std::max<int>(node.topL, layer);
      }
      for (int i = 0; i < edge.route.routelen; i++) {
        const auto& from = grids[i];
        const auto& to = grids[i + 1];
        if (from.layer == to.layer) {
          router_->updateEdge2DAnd3DUsage(std::min(from.x, to.x),
                                          std::min(from.y, to.y),
                                          std::max(from.x, to.x),
                                          std::max(from.y, to.y),
                                          from.layer + 1,
                                          1,
                                          db_net);
        }
      }
      tree.edges.push_back(std::move(edge));
    }
    Maze3DTestPeer::setTree(*router_, net_id, std::move(tree));
    return net_id;
  }

  int addBlockedNet(const char* name = "blocked")
  {
    const int net_id = addNet(name,
                              {{0, 0, 0},
                               {1, 0, 0},
                               {1, 0, 1},
                               {1, 1, 1},
                               {1, 2, 1},
                               {1, 2, 0},
                               {2, 2, 0}});
    // Block both planar exits while allowing layer changes at the source.
    router_->addAdjustment(0, 0, 1, 0, 1, 0, true);
    router_->addAdjustment(0, 0, 0, 1, 2, 0, true);
    return net_id;
  }

  void route(const std::vector<int>& net_ids)
  {
    // The non-resistance-aware pass processes only the first 90% of nets.
    auto order = net_ids;
    order.push_back(skipped_net_);
    Maze3DTestPeer::route(*router_, order);
  }

  StTree tree(int net_id) const
  {
    return Maze3DTestPeer::getTree(*router_, net_id);
  }

  std::vector<int> usage() const { return Maze3DTestPeer::getUsage(*router_); }

  void expectRoute(int net_id,
                   const std::vector<GPoint3D>& expected,
                   int edge_id = 0) const
  {
    const auto actual = tree(net_id);
    const auto& edge = actual.edges[edge_id];
    EXPECT_EQ(edge.route.type, RouteType::MazeRoute);
    EXPECT_EQ(edge.route.routelen, expected.size() - 1);
    ASSERT_EQ(edge.route.grids.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); i++) {
      SCOPED_TRACE(i);
      EXPECT_EQ(edge.route.grids[i].x, expected[i].x);
      EXPECT_EQ(edge.route.grids[i].y, expected[i].y);
      EXPECT_EQ(edge.route.grids[i].layer, expected[i].layer);
    }
  }

  void expectNodeConnections(int net_id, const StTree& expected) const
  {
    const auto actual = tree(net_id);
    ASSERT_EQ(actual.nodes.size(), expected.nodes.size());
    for (std::size_t i = 0; i < actual.nodes.size(); i++) {
      SCOPED_TRACE(i);
      const auto& node = actual.nodes[i];
      const auto& original = expected.nodes[i];
      EXPECT_TRUE(node.assigned);
      EXPECT_EQ(node.x, original.x);
      EXPECT_EQ(node.y, original.y);
      EXPECT_EQ(node.stackAlias, original.stackAlias);
      EXPECT_EQ(node.nbr_count, original.nbr_count);
      ASSERT_EQ(node.conCNT, original.conCNT);
      EXPECT_EQ(node.botL, original.botL);
      EXPECT_EQ(node.topL, original.topL);
      for (int j = 0; j < 3; j++) {
        EXPECT_EQ(node.nbr[j], original.nbr[j]);
        EXPECT_EQ(node.edge[j], original.edge[j]);
      }
      std::vector<std::pair<int, int>> connections;
      std::vector<std::pair<int, int>> original_connections;
      for (int j = 0; j < node.conCNT; j++) {
        connections.emplace_back(node.eID[j], node.heights[j]);
      }
      for (int j = 0; j < original.conCNT; j++) {
        original_connections.emplace_back(original.eID[j], original.heights[j]);
      }
      std::ranges::sort(connections);
      std::ranges::sort(original_connections);
      EXPECT_EQ(connections, original_connections);
    }
  }

  void expectSearchExhausted() const
  {
    const auto state = Maze3DTestPeer::getSearchState(*router_);
    EXPECT_TRUE(state.source_empty);
    EXPECT_TRUE(state.destination_marks_clear);
    EXPECT_TRUE(state.region_clear);
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_builder_{&logger_};
  std::unique_ptr<FastRouteCore> router_;
  int skipped_net_ = -1;
};

TEST_F(Maze3DTest, RestoresRouteAndUsageAfterHeapUnderflow)
{
  const int blocked_net = addBlockedNet();
  const auto original_tree = tree(blocked_net);
  const auto original_usage = usage();

  // Retry to detect usage leaks and duplicate node connections.
  for (int attempt = 0; attempt < 2; attempt++) {
    SCOPED_TRACE(attempt);
    ASSERT_NO_THROW(route({blocked_net}));
    expectSearchExhausted();
    expectRoute(blocked_net, original_tree.edges[0].route.grids);
    expectNodeConnections(blocked_net, original_tree);
    EXPECT_EQ(usage(), original_usage);
    EXPECT_EQ(logger_.getWarningCount(), attempt + 1);
  }
}

TEST_F(Maze3DTest, ContinuesRoutingAfterHeapUnderflow)
{
  const int blocked_net = addBlockedNet();
  const auto blocked_tree = tree(blocked_net);
  const int next_net = addNet("next",
                              {{0, 2, 0},
                               {0, 2, 1},
                               {0, 1, 1},
                               {0, 1, 0},
                               {1, 1, 0},
                               {2, 1, 0},
                               {2, 1, 1},
                               {2, 2, 1},
                               {2, 2, 0}});
  const auto next_tree = tree(next_net);

  ASSERT_NO_THROW(route({blocked_net, next_net}));

  expectRoute(blocked_net, blocked_tree.edges[0].route.grids);
  expectNodeConnections(blocked_net, blocked_tree);
  expectRoute(next_net, {{0, 2, 0}, {1, 2, 0}, {2, 2, 0}});
  expectNodeConnections(next_net, next_tree);
}

TEST_F(Maze3DTest, WarnsOnceWithRecoveredNetCount)
{
  const int first_net = addBlockedNet("first");
  const int second_net = addBlockedNet("second");

  logger_.redirectStringBegin();
  EXPECT_NO_THROW(route({first_net, second_net}));
  const std::string output = logger_.redirectStringEnd();

  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(output.find("[WARNING GRT-0183]"), std::string::npos);
  EXPECT_NE(output.find("edges of 2 nets"), std::string::npos);
}

TEST_F(Maze3DTest, DoesNotWarnWhenNoRecoveryIsNeeded)
{
  const std::vector<GPoint3D> grids{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
  const int net = addNet("routable", grids);
  const auto original_tree = tree(net);

  ASSERT_NO_THROW(route({net}));

  expectRoute(net, grids);
  expectNodeConnections(net, original_tree);
  EXPECT_EQ(logger_.getWarningCount(), 0);
}

TEST_F(Maze3DTest, RestoresUpperLayerPinConnectionsAfterHeapUnderflow)
{
  const std::vector<GPoint3D> grids{{0, 0, 1}, {0, 1, 1}, {0, 2, 1}};
  const int net = addNet("upper_layer", grids);
  const auto original_tree = tree(net);
  const auto original_usage = usage();
  router_->addAdjustment(0, 0, 1, 0, 1, 0, true);
  router_->addAdjustment(0, 0, 0, 1, 2, 0, true);

  ASSERT_NO_THROW(route({net}));

  expectSearchExhausted();
  expectRoute(net, grids);
  expectNodeConnections(net, original_tree);
  EXPECT_EQ(usage(), original_usage);
  EXPECT_EQ(logger_.getWarningCount(), 1);
}

TEST_F(Maze3DTest, RecoversMultipleEdgesOfOneMultiPinNet)
{
  const int net = addTreeNet(
      "three_pin",
      {{0, 1, 1}, {2, 0, 1}, {2, 3, 1}},
      {{1, 2, 0}},
      {{0, 3, {{0, 1, 1}, {0, 2, 1}, {0, 2, 0}, {1, 2, 0}}},
       {3, 1, {{1, 2, 0}, {2, 2, 0}, {2, 2, 1}, {2, 1, 1}, {2, 0, 1}}},
       {3,
        2,
        {{1, 2, 0}, {1, 2, 1}, {1, 3, 1}, {1, 3, 0}, {2, 3, 0}, {2, 3, 1}}}});
  const auto original_tree = tree(net);
  const auto original_usage = usage();
  for (int y = 0; y < kYGrid; y++) {
    for (int x = 0; x < kXGrid - 1; x++) {
      router_->addAdjustment(x, y, x + 1, y, 1, 0, true);
    }
  }
  for (int y = 0; y < kYGrid - 1; y++) {
    for (int x = 0; x < kXGrid; x++) {
      router_->addAdjustment(x, y, x, y + 1, 2, 0, true);
    }
  }

  logger_.setDebugLevel(utl::GRT, "maze_3d", 1);
  logger_.redirectStringBegin();
  EXPECT_NO_THROW(route({net}));
  const std::string output = logger_.redirectStringEnd();

  expectSearchExhausted();
  const int num_edges = static_cast<int>(original_tree.edges.size());
  for (int edge_id = 0; edge_id < num_edges; edge_id++) {
    SCOPED_TRACE(edge_id);
    expectRoute(net, original_tree.edges[edge_id].route.grids, edge_id);
    const std::string diagnostic
        = "no 3D maze path found for edge " + std::to_string(edge_id) + ";";
    EXPECT_NE(output.find(diagnostic), std::string::npos);
  }
  expectNodeConnections(net, original_tree);
  EXPECT_EQ(usage(), original_usage);
  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(output.find("[WARNING GRT-0183]"), std::string::npos);
  EXPECT_NE(output.find("edges of 1 nets"), std::string::npos);
}

}  // namespace
}  // namespace grt
