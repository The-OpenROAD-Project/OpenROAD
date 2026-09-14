// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "FastRoute.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/db_fixture.h"
#include "utl/ServiceRegistry.h"

namespace grt {

// Exercise the 3D search directly with synthetic routed trees.
class Maze3DTest : public tst::DbFixture
{
 protected:
  static constexpr int kGridSize = 3;
  static constexpr int kNumLayers = 2;

  void SetUp() override
  {
    auto* tech = odb::dbTech::create(db_.get(), "tech");
    auto* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    router_ = std::make_unique<FastRouteCore>(
        db_.get(), &logger_, &registry_, &stt_builder_, nullptr);
    router_->setTileSize(100);
    router_->setGridsAndLayers(kGridSize, kGridSize, kNumLayers);
    router_->addLayerDirection(0, odb::dbTechLayerDir::HORIZONTAL);
    router_->addLayerDirection(1, odb::dbTechLayerDir::VERTICAL);
    router_->initEdges();
    router_->initAuxVar();
    router_->setIncrementalGrt(true);
    router_->via_cost_ = 1;
    for (int y = 0; y < kGridSize; y++) {
      for (int x = 0; x < kGridSize - 1; x++) {
        router_->setEdgeCapacity(x, y, x + 1, y, 1, 10);
        router_->setEdgeCapacity(y, x, y, x + 1, 2, 10);
      }
    }
    skipped_net_ = addNet("skipped", {});
  }

  int addNet(const char* name, const std::vector<GPoint3D>& grids)
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
    const int net_id = router_->db_net_id_map_.at(db_net);
    if (grids.empty()) {
      return net_id;
    }

    StTree& tree = router_->sttrees_[net_id];
    tree.num_terminals = 2;
    tree.nodes.resize(2);
    tree.edges.resize(1);
    const std::array endpoints{grids.front(), grids.back()};
    for (int i = 0; i < 2; i++) {
      const auto& point = endpoints[i];
      net->addPin(point.x, point.y, point.layer);
      tree.node_to_pin_idx[i] = i;
      TreeNode& node = tree.nodes[i];
      node.x = point.x;
      node.y = point.y;
      node.stackAlias = i;
      node.assigned = true;
      node.nbr_count = 1;
      node.nbr[0] = 1 - i;
      node.edge[0] = 0;
      node.conCNT = 1;
      node.eID[0] = 0;
      node.heights[0] = point.layer;
      node.botL = node.topL = point.layer;
    }
    TreeEdge& edge = tree.edges[0];
    edge.n1 = edge.n1a = 0;
    edge.n2 = edge.n2a = 1;
    edge.assigned = true;
    edge.len = std::abs(endpoints[0].x - endpoints[1].x)
               + std::abs(endpoints[0].y - endpoints[1].y);
    edge.route.type = RouteType::MazeRoute;
    edge.route.grids = grids;
    edge.route.routelen = grids.size() - 1;
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
    return net_id;
  }

  int addBlockedNet()
  {
    const int net_id = addNet("blocked",
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
    router_->tree_order_pv_.clear();
    for (int net_id : net_ids) {
      OrderNetPin order{};
      order.treeIndex = net_id;
      router_->tree_order_pv_.push_back(order);
    }
    // The non-resistance-aware pass processes only the first 90% of nets.
    OrderNetPin skipped{};
    skipped.treeIndex = skipped_net_;
    router_->tree_order_pv_.push_back(skipped);
    router_->mazeRouteMSMDOrder3D(
        /*expand=*/3, /*ripupTHlb=*/0, /*ripupTHub=*/10);
  }

  const StTree& tree(int net_id) const { return router_->sttrees_[net_id]; }

  std::vector<int> usage() const
  {
    std::vector<int> result;
    for (int y = 0; y < kGridSize; y++) {
      for (int x = 0; x < kGridSize - 1; x++) {
        result.push_back(router_->graph2d_.getUsageH(x, y));
        result.push_back(router_->graph2d_.getUsageV(y, x));
        for (int layer = 0; layer < kNumLayers; layer++) {
          result.push_back(router_->h_edges_3D_[layer][y][x].usage);
          result.push_back(router_->v_edges_3D_[layer][x][y].usage);
        }
      }
    }
    return result;
  }

  void expectRoute(int net_id, const std::vector<GPoint3D>& expected) const
  {
    const auto& edge = tree(net_id).edges[0];
    EXPECT_EQ(edge.route.type, RouteType::MazeRoute);
    EXPECT_EQ(edge.route.routelen, expected.size() - 1);
    ASSERT_EQ(edge.route.grids.size(), expected.size());
    for (int i = 0; i < expected.size(); i++) {
      SCOPED_TRACE(i);
      EXPECT_EQ(edge.route.grids[i].x, expected[i].x);
      EXPECT_EQ(edge.route.grids[i].y, expected[i].y);
      EXPECT_EQ(edge.route.grids[i].layer, expected[i].layer);
    }
    for (const auto& node : tree(net_id).nodes) {
      EXPECT_TRUE(node.assigned);
      EXPECT_EQ(node.conCNT, 1);
      EXPECT_EQ(node.eID[0], 0);
      EXPECT_EQ(node.heights[0], 0);
      EXPECT_EQ(node.botL, 0);
      EXPECT_EQ(node.topL, 0);
    }
  }

  void expectSearchExhausted() const
  {
    EXPECT_TRUE(router_->src_heap_3D_.empty());
    EXPECT_FALSE(std::ranges::any_of(router_->pop_heap2_3D_,
                                     [](bool marked) { return marked; }));
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
  const auto original_grids = tree(blocked_net).edges[0].route.grids;
  const auto original_usage = usage();

  // Retry to detect usage leaks and duplicate node connections.
  for (int attempt = 0; attempt < 2; attempt++) {
    SCOPED_TRACE(attempt);
    ASSERT_NO_THROW(route({blocked_net}));
    expectSearchExhausted();
    expectRoute(blocked_net, original_grids);
    EXPECT_EQ(usage(), original_usage);
  }
}

TEST_F(Maze3DTest, ContinuesRoutingAfterHeapUnderflow)
{
  const int blocked_net = addBlockedNet();
  const auto original_grids = tree(blocked_net).edges[0].route.grids;
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

  ASSERT_NO_THROW(route({blocked_net, next_net}));

  expectRoute(blocked_net, original_grids);
  expectRoute(next_net, {{0, 2, 0}, {1, 2, 0}, {2, 2, 0}});
}

}  // namespace grt
