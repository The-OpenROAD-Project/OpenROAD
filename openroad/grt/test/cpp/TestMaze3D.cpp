// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "FastRoute.h"
#include "grt/GRoute.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/db_fixture.h"
#include "tst/fixture.h"
#include "tst/physical_builder.h"
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

  static void route(FastRouteCore& router,
                    const std::vector<int>& net_ids,
                    int expand = 3,
                    int ripup_th_lb = 0,
                    int ripup_th_ub = 10)
  {
    int max_nodes = 0;
    for (int net_id : net_ids) {
      max_nodes = std::max(max_nodes, router.sttrees_[net_id].num_nodes());
    }
    if (static_cast<int>(router.xcor_.size()) < 2 * max_nodes) {
      router.xcor_.resize(2 * max_nodes);
      router.ycor_.resize(2 * max_nodes);
      router.dcor_.resize(2 * max_nodes);
    }
    router.getOverflow3D();
    router.tree_order_pv_.clear();
    for (int net_id : net_ids) {
      OrderNetPin order{};
      order.treeIndex = net_id;
      router.tree_order_pv_.push_back(order);
    }
    router.mazeRouteMSMDOrder3D(expand, ripup_th_lb, ripup_th_ub);
    router.getOverflow3D();
  }

  static void preProcessTechLayers(FastRouteCore& router)
  {
    router.preProcessTechLayers();
  }

  static float getMazeCost(FastRouteCore& router,
                           int net_id,
                           int from_l,
                           int to_l,
                           int from_x,
                           int from_y,
                           int to_x,
                           int to_y,
                           bool is_via)
  {
    return router.getMazeRouteCost3D(
        net_id, from_l, to_l, from_x, from_y, to_x, to_y, is_via);
  }

  static void setActiveResistanceAware(FastRouteCore& router, bool active)
  {
    router.resistance_aware_ = active;
  }

  static size_t getLastMaze3DPops(const FastRouteCore& router)
  {
    return router.last_maze3d_pops_;
  }

  static bool isD1Clean(const FastRouteCore& router)
  {
    return router.d1_3D_clean_;
  }

  static bool isTouchedEmpty(const FastRouteCore& router)
  {
    return router.touched_3D_.empty();
  }

  static bool isD1AllBigInt(const FastRouteCore& router)
  {
    const int* data = router.d1_3D_.data();
    const size_t n = router.d1_3D_.num_elements();
    return std::all_of(
        data, data + n, [](int v) { return v == FastRouteCore::BIG_INT; });
  }

  static std::unique_ptr<FastRouteCore> buildWorker(const FastRouteCore& router)
  {
    return router.buildSnapshotBatchWorker();
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
    logger_.setDebugLevel(utl::GRT, "overflowcheck", 1);
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

using Via = std::tuple<int, int, int, int>;

// Lists the vias of a route by position and layer pair, whichever way each
// via points. A via listed twice is a parallel resistor in the parasitics.
std::multiset<Via> getVias(const GRoute& route)
{
  std::multiset<Via> vias;
  for (const GSegment& segment : route) {
    if (segment.isVia() && segment.init_layer != segment.final_layer) {
      vias.emplace(segment.init_x,
                   segment.init_y,
                   std::min(segment.init_layer, segment.final_layer),
                   std::max(segment.init_layer, segment.final_layer));
    }
  }
  return vias;
}

TEST_F(Maze3DTest, Get3DRouteAddsSinkPinViaOnce)
{
  // The sink pin is below the edge, so its via stack is walked down to the
  // pin and back up.
  addTreeNet("sink_below_edge",
             {{1, 0, 0}, {1, 2, 0}},
             {},
             {{0, 1, {{1, 0, 1}, {1, 1, 1}, {1, 2, 1}}}});

  GRoute route;
  router_->get3DRoute(block_->findNet("sink_below_edge"), route);

  const std::multiset<Via> expected{{150, 50, 1, 2}, {150, 250, 1, 2}};
  EXPECT_EQ(getVias(route), expected);
  EXPECT_EQ(route.size(), 4);
}

TEST_F(Maze3DTest, Get3DRouteAddsColocatedPinViaOnce)
{
  // Two sink pins share a gcell on different layers. The routed edge climbs
  // to the upper pin, and the zero-length edge between the pins walks the
  // same via back down.
  addTreeNet("colocated_sinks",
             {{0, 1, 0}, {2, 1, 1}, {2, 1, 0}},
             {},
             {{0, 1, {{0, 1, 0}, {1, 1, 0}, {2, 1, 0}}},
              {1, 2, {{2, 1, 1}, {2, 1, 0}}}});

  GRoute route;
  router_->get3DRoute(block_->findNet("colocated_sinks"), route);

  const std::multiset<Via> expected{{250, 150, 1, 2}};
  EXPECT_EQ(getVias(route), expected);
  EXPECT_EQ(route.size(), 3);
}

TEST_F(Maze3DTest, PlanarRouteAddsCrossingViaOnce)
{
  // The edges cross at (1, 1) with opposite turns: the first turns from the
  // vertical to the horizontal layer, the second arrives horizontally and is
  // bridged up before it turns.
  addTreeNet("crossing_edges",
             {{1, 0, 1}, {2, 1, 0}, {0, 1, 0}, {1, 2, 1}},
             {},
             {{0, 1, {{1, 0, 1}, {1, 1, 1}, {2, 1, 0}}},
              {2, 3, {{0, 1, 0}, {1, 1, 0}, {1, 2, 1}}}});

  GRoute route;
  router_->getPlanarRoute(block_->findNet("crossing_edges"), route);

  const std::multiset<Via> expected{{150, 150, 1, 2}};
  EXPECT_EQ(getVias(route), expected);
  EXPECT_EQ(route.size(), 5);
}

// Compute exact 3D Dijkstra shortest-path distance from every grid cell to a
// set of destination cells on an unconstrained-capacity 3D grid.
std::vector<int> computeReverseDijkstra3D(
    int x_grid,
    int y_grid,
    int num_layers,
    int min_layer,
    int max_layer,
    const std::vector<odb::dbTechLayerDir>& layer_dirs,
    const std::vector<float>& wire_cost,
    const std::vector<float>& down_via_cost,
    const std::vector<float>& up_via_cost,
    const std::vector<GPoint3D>& dests)
{
  const int layer_stride = x_grid * y_grid;
  const int total = num_layers * layer_stride;
  std::vector<int> dist(total, Maze3DHeuristic::kBigInt);
  using Entry = std::pair<int, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

  auto indexOf = [x_grid, layer_stride](int l, int x, int y) {
    return l * layer_stride + y * x_grid + x;
  };
  for (const auto& d : dests) {
    const int idx = indexOf(d.layer, d.x, d.y);
    dist[idx] = 0;
    pq.emplace(0, idx);
  }

  while (!pq.empty()) {
    const auto [cur_d, idx] = pq.top();
    pq.pop();
    if (cur_d != dist[idx]) {
      continue;
    }
    const int l = idx / layer_stride;
    const int rem = idx % layer_stride;
    const int x = rem % x_grid;
    const int y = rem / x_grid;

    auto relax = [&](int prev_l, int prev_x, int prev_y, int step_cost) {
      const int prev_idx = indexOf(prev_l, prev_x, prev_y);
      const int nd = cur_d + step_cost;
      if (nd < dist[prev_idx]) {
        dist[prev_idx] = nd;
        pq.emplace(nd, prev_idx);
      }
    };

    // Reverse of planar moves on layer l (if allowed).
    if (l >= min_layer && l <= max_layer) {
      const int wc = static_cast<int>(wire_cost[l]);
      if (layer_dirs[l] == odb::dbTechLayerDir::HORIZONTAL) {
        if (x > 0) {
          relax(l, x - 1, y, wc);
        }
        if (x + 1 < x_grid) {
          relax(l, x + 1, y, wc);
        }
      } else {
        if (y > 0) {
          relax(l, x, y - 1, wc);
        }
        if (y + 1 < y_grid) {
          relax(l, x, y + 1, wc);
        }
      }
    }
    // Reverse of down via: prev is l + 1 moving down to l.
    if (l + 1 < num_layers) {
      relax(l + 1, x, y, static_cast<int>(down_via_cost[l + 1]));
    }
    // Reverse of up via: prev is l - 1 moving up to l.
    if (l > 0) {
      relax(l - 1, x, y, static_cast<int>(up_via_cost[l - 1]));
    }
  }
  return dist;
}

void verifyHeuristicAdmissibilityAndConsistency(
    int x_grid,
    int y_grid,
    int num_layers,
    int min_layer,
    int max_layer,
    bool resistance_aware,
    const std::vector<odb::dbTechLayerDir>& layer_dirs,
    const std::vector<float>& wire_cost,
    const std::vector<float>& down_via_cost,
    const std::vector<float>& up_via_cost,
    const std::vector<GPoint3D>& dests)
{
  Maze3DHeuristic heuristic;
  heuristic.prepareNet(num_layers,
                       min_layer,
                       max_layer,
                       resistance_aware,
                       layer_dirs,
                       wire_cost,
                       down_via_cost,
                       up_via_cost);
  int dest_x1 = x_grid;
  int dest_x2 = -1;
  int dest_y1 = y_grid;
  int dest_y2 = -1;
  int dest_l1 = num_layers;
  int dest_l2 = -1;
  for (const auto& d : dests) {
    dest_x1 = std::min<int>(dest_x1, d.x);
    dest_x2 = std::max<int>(dest_x2, d.x);
    dest_y1 = std::min<int>(dest_y1, d.y);
    dest_y2 = std::max<int>(dest_y2, d.y);
    dest_l1 = std::min<int>(dest_l1, d.layer);
    dest_l2 = std::max<int>(dest_l2, d.layer);
  }
  heuristic.prepareDestination(dest_l1, dest_l2);

  const std::vector<int> exact = computeReverseDijkstra3D(x_grid,
                                                          y_grid,
                                                          num_layers,
                                                          min_layer,
                                                          max_layer,
                                                          layer_dirs,
                                                          wire_cost,
                                                          down_via_cost,
                                                          up_via_cost,
                                                          dests);
  const int layer_stride = x_grid * y_grid;
  auto evalAt = [&](int l, int x, int y) {
    const int dx = Maze3DHeuristic::distToBox(x, dest_x1, dest_x2);
    const int dy = Maze3DHeuristic::distToBox(y, dest_y1, dest_y2);
    return heuristic.eval(l, dx, dy);
  };

  for (const auto& d : dests) {
    EXPECT_EQ(evalAt(d.layer, d.x, d.y), 0);
  }

  for (int l = 0; l < num_layers; ++l) {
    for (int y = 0; y < y_grid; ++y) {
      for (int x = 0; x < x_grid; ++x) {
        const int hu = evalAt(l, x, y);
        const int exact_d = exact[l * layer_stride + y * x_grid + x];
        EXPECT_GE(hu, 0);
        // Admissibility: h(u) <= dist*(u, dest).
        EXPECT_LE(hu, exact_d) << "l=" << l << " x=" << x << " y=" << y;

        // Consistency: h(u) <= cost(u, v) + h(v) for every legal edge (u, v).
        if (l >= min_layer && l <= max_layer) {
          const int wc = static_cast<int>(wire_cost[l]);
          if (layer_dirs[l] == odb::dbTechLayerDir::HORIZONTAL) {
            if (x > 0) {
              EXPECT_LE(hu, wc + evalAt(l, x - 1, y));
            }
            if (x + 1 < x_grid) {
              EXPECT_LE(hu, wc + evalAt(l, x + 1, y));
            }
          } else {
            if (y > 0) {
              EXPECT_LE(hu, wc + evalAt(l, x, y - 1));
            }
            if (y + 1 < y_grid) {
              EXPECT_LE(hu, wc + evalAt(l, x, y + 1));
            }
          }
        }
        if (l > 0) {
          EXPECT_LE(hu,
                    static_cast<int>(down_via_cost[l]) + evalAt(l - 1, x, y));
        }
        if (l + 1 < num_layers) {
          EXPECT_LE(hu, static_cast<int>(up_via_cost[l]) + evalAt(l + 1, x, y));
        }
      }
    }
  }
}

TEST(Maze3DHeuristicTest, NonResAwareAdmissibleAndConsistent)
{
  const std::vector<odb::dbTechLayerDir> dirs = {
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
  };
  const std::vector<float> wire_cost = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  const std::vector<float> down_via = {0.0f, 3.0f, 2.0f, 4.0f, 2.0f, 5.0f};
  const std::vector<float> up_via = {3.0f, 2.0f, 4.0f, 2.0f, 5.0f, 0.0f};

  verifyHeuristicAdmissibilityAndConsistency(
      /*x_grid=*/6,
      /*y_grid=*/6,
      /*num_layers=*/6,
      /*min_layer=*/1,
      /*max_layer=*/4,
      /*resistance_aware=*/false,
      dirs,
      wire_cost,
      down_via,
      up_via,
      /*dests=*/{{4, 3, 0}, {5, 4, 2}});
}

TEST(Maze3DHeuristicTest, ResAwareParetoAdmissibleConsistentAndTight)
{
  const std::vector<odb::dbTechLayerDir> dirs = {
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
  };
  // Upper metal layers have lower wire resistance; vias have varied costs.
  const std::vector<float> wire_cost = {20.0f, 18.0f, 8.0f, 7.0f, 2.0f, 2.0f};
  const std::vector<float> down_via = {0.0f, 6.0f, 6.0f, 4.0f, 8.0f, 10.0f};
  const std::vector<float> up_via = {6.0f, 6.0f, 4.0f, 8.0f, 10.0f, 0.0f};

  // Single-layer pin destination below min_layer.
  verifyHeuristicAdmissibilityAndConsistency(
      /*x_grid=*/6,
      /*y_grid=*/6,
      /*num_layers=*/6,
      /*min_layer=*/1,
      /*max_layer=*/5,
      /*resistance_aware=*/true,
      dirs,
      wire_cost,
      down_via,
      up_via,
      /*dests=*/{{4, 4, 0}});

  // Multi-layer subtree destination.
  verifyHeuristicAdmissibilityAndConsistency(
      /*x_grid=*/6,
      /*y_grid=*/6,
      /*num_layers=*/6,
      /*min_layer=*/0,
      /*max_layer=*/5,
      /*resistance_aware=*/true,
      dirs,
      wire_cost,
      down_via,
      up_via,
      /*dests=*/{{2, 1, 0}, {4, 3, 3}, {5, 2, 1}});

  // Single-direction-only layer constraint (only horizontal layer 2 allowed).
  verifyHeuristicAdmissibilityAndConsistency(
      /*x_grid=*/4,
      /*y_grid=*/4,
      /*num_layers=*/6,
      /*min_layer=*/2,
      /*max_layer=*/2,
      /*resistance_aware=*/true,
      dirs,
      wire_cost,
      down_via,
      up_via,
      /*dests=*/{{3, 2, 0}});

  // Verify Pareto trade-off tightness: from layer 0 to dest layer 0 with
  // dx = 1, dy = 0, climbing all the way to layer 4 costs 48 in vias + 2 wire
  // = 50, whereas staying on layer 0 costs 20 (strictly tighter than naive
  // dx * min_all_h = 2).
  Maze3DHeuristic heuristic;
  heuristic.prepareNet(6, 0, 5, true, dirs, wire_cost, down_via, up_via);
  heuristic.prepareDestination(0, 0);
  EXPECT_EQ(heuristic.eval(0, 1, 0), 20);
  // For dx = 10, climbing to layer 2 (vias=24, wire=80 -> 104) vs layer 4
  // (vias=48, wire=20 -> 68) beats staying on layer 0 (200).
  EXPECT_EQ(heuristic.eval(0, 10, 0), 68);
  // Changing destination layer range invalidates cached Pareto terms.
  heuristic.prepareDestination(4, 4);
  EXPECT_EQ(heuristic.eval(4, 1, 0), 2);
  EXPECT_EQ(heuristic.eval(0, 0, 0), 24);
}

TEST(MinHeap3DTest, RekeyAndReopenAfterPopMaintainHeapOrder)
{
  MinHeap3D heap;
  heap.resize(16);
  heap.pushSeed(2);
  heap.pushSeed(5);
  heap.pushSeed(9);

  heap.rekeyAndHeapify([](int idx) {
    return MinHeap3D::Key{.primary = static_cast<uint64_t>(20 - idx), .tie = 0};
  });
  EXPECT_EQ(heap.topIdx(), 9);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 5);

  // Re-opening a popped node via decreaseKey re-inserts it cleanly.
  heap.decreaseKey(9, 1);
  EXPECT_EQ(heap.topIdx(), 9);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 5);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 2);
  heap.removeMin();
  EXPECT_TRUE(heap.empty());
}

TEST(MinHeap3DTest, ClearResetsPositionsForSubsequentDecreaseKey)
{
  MinHeap3D heap;
  heap.resize(16);
  heap.insert(3, 10);
  heap.insert(7, 20);
  heap.insert(11, 30);

  // Clearing a non-empty heap must reset pos_ slots for all active entries so
  // decreaseKey() on any previously active index safely re-inserts it.
  heap.clear();
  EXPECT_TRUE(heap.empty());

  heap.decreaseKey(7, 15);
  heap.decreaseKey(3, 5);
  heap.decreaseKey(11, 25);

  EXPECT_EQ(heap.topIdx(), 3);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 7);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 11);
  heap.removeMin();
  EXPECT_TRUE(heap.empty());
}

TEST(MinHeap3DTest, ResizeClearsEntriesAndResetsPositions)
{
  MinHeap3D heap;
  heap.resize(32);
  heap.insert(5, 30);
  heap.insert(20, 10);
  heap.insert(31, 20);

  // Shrinking a non-empty heap must discard stale high-index entries so
  // subsequent clear() and decreaseKey() never index past pos_.size().
  heap.resize(8);
  EXPECT_TRUE(heap.empty());
  heap.clear();
  EXPECT_TRUE(heap.empty());

  heap.insert(6, 25);
  heap.decreaseKey(2, 10);
  heap.decreaseKey(4, 15);
  heap.decreaseKey(6, 5);

  EXPECT_EQ(heap.topIdx(), 6);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 2);
  heap.removeMin();
  EXPECT_EQ(heap.topIdx(), 4);
  heap.removeMin();
  EXPECT_TRUE(heap.empty());

  // Growing a non-empty heap must likewise keep entries_ and pos_ consistent.
  heap.insert(3, 50);
  heap.resize(16);
  EXPECT_TRUE(heap.empty());
  heap.decreaseKey(3, 40);
  EXPECT_EQ(heap.topIdx(), 3);
  heap.removeMin();
  EXPECT_TRUE(heap.empty());
}

// Compute exact reference 3D Dijkstra shortest-path cost on a FastRouteCore's
// current 3D edge capacities and costs for a 2-pin edge.
int referenceDijkstraCost3D(FastRouteCore& router,
                            int net_id,
                            int x_grid,
                            int y_grid,
                            int num_layers,
                            int min_layer,
                            int max_layer,
                            const std::vector<odb::dbTechLayerDir>& dirs,
                            const GPoint3D& src,
                            const GPoint3D& dst,
                            int region_x1,
                            int region_x2,
                            int region_y1,
                            int region_y2)
{
  const int layer_stride = x_grid * y_grid;
  const int total = num_layers * layer_stride;
  std::vector<int> dist(total, Maze3DHeuristic::kBigInt);
  using Entry = std::pair<int, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

  auto indexOf = [x_grid, layer_stride](int l, int x, int y) {
    return l * layer_stride + y * x_grid + x;
  };
  const int src_idx = indexOf(src.layer, src.x, src.y);
  const int dst_idx = indexOf(dst.layer, dst.x, dst.y);
  dist[src_idx] = 0;
  pq.emplace(0, src_idx);

  const auto& h_edges = router.getHorizontalEdges3D();
  const auto& v_edges = router.getVerticalEdges3D();

  while (!pq.empty()) {
    const auto [d, idx] = pq.top();
    pq.pop();
    if (idx == dst_idx) {
      return d;
    }
    if (d != dist[idx]) {
      continue;
    }
    const int l = idx / layer_stride;
    const int rem = idx % layer_stride;
    const int x = rem % x_grid;
    const int y = rem / x_grid;

    auto relax = [&](int nl, int nx, int ny, int step) {
      const int nidx = indexOf(nl, nx, ny);
      if (d + step < dist[nidx]) {
        dist[nidx] = d + step;
        pq.emplace(dist[nidx], nidx);
      }
    };

    if (l >= min_layer && l <= max_layer) {
      const int wc = static_cast<int>(
          Maze3DTestPeer::getMazeCost(router, net_id, l, l, 0, 0, 1, 0, false));
      if (dirs[l] == odb::dbTechLayerDir::HORIZONTAL) {
        if (x > region_x1
            && h_edges[l][y][x - 1].usage + 1 <= h_edges[l][y][x - 1].cap) {
          relax(l, x - 1, y, wc);
        }
        if (x < region_x2
            && h_edges[l][y][x].usage + 1 <= h_edges[l][y][x].cap) {
          relax(l, x + 1, y, wc);
        }
      } else {
        if (y > region_y1
            && v_edges[l][y - 1][x].usage + 1 <= v_edges[l][y - 1][x].cap) {
          relax(l, x, y - 1, wc);
        }
        if (y < region_y2
            && v_edges[l][y][x].usage + 1 <= v_edges[l][y][x].cap) {
          relax(l, x, y + 1, wc);
        }
      }
    }
    if (l > 0) {
      const int dv = static_cast<int>(Maze3DTestPeer::getMazeCost(
          router, net_id, l, l - 1, 0, 0, 0, 0, true));
      relax(l - 1, x, y, dv);
    }
    if (l + 1 < num_layers) {
      const int uv = static_cast<int>(Maze3DTestPeer::getMazeCost(
          router, net_id, l, l + 1, 0, 0, 0, 0, true));
      relax(l + 1, x, y, uv);
    }
  }
  return Maze3DHeuristic::kBigInt;
}

int evaluateRouteCost3D(FastRouteCore& router,
                        int net_id,
                        const std::vector<GPoint3D>& grids,
                        int src_pin_layer,
                        int dst_pin_layer)
{
  int cost = 0;
  // mazeRouteMSMDOrder3D trims vertical pin via stacks at endpoints via
  // headRoom/tailRoom; include endpoint pin-to-route via transitions so total
  // cost matches src_pin -> dst_pin Dijkstra distance.
  auto addViaStack = [&](int from_l, int to_l) {
    while (from_l < to_l) {
      cost += static_cast<int>(Maze3DTestPeer::getMazeCost(
          router, net_id, from_l, from_l + 1, 0, 0, 0, 0, true));
      ++from_l;
    }
    while (from_l > to_l) {
      cost += static_cast<int>(Maze3DTestPeer::getMazeCost(
          router, net_id, from_l, from_l - 1, 0, 0, 0, 0, true));
      --from_l;
    }
  };
  addViaStack(src_pin_layer, grids.front().layer);
  for (size_t i = 0; i + 1 < grids.size(); ++i) {
    const auto& a = grids[i];
    const auto& b = grids[i + 1];
    const bool is_via = (a.layer != b.layer);
    cost += static_cast<int>(Maze3DTestPeer::getMazeCost(
        router, net_id, a.layer, b.layer, a.x, a.y, b.x, b.y, is_via));
  }
  addViaStack(grids.back().layer, dst_pin_layer);
  return cost;
}

class MultiLayerMaze3DTest : public tst::Fixture
{
 protected:
  static constexpr int kXGrid = 14;
  static constexpr int kYGrid = 6;
  static constexpr int kNumLayers = 4;
  static constexpr int kTileSize = 2000;

  void setupRouter(int x_grid,
                   int y_grid,
                   int num_layers,
                   const std::vector<odb::dbTechLayerDir>& dirs_override = {})
  {
    x_grid_ = x_grid;
    y_grid_ = y_grid;
    num_layers_ = num_layers;
    if (block_ == nullptr) {
      auto* tech = tst::makeTech(db_.get());
      block_ = tst::makeBlock(db_.get(), tech);
      auto layers = tst::makeLayerStack(
          tech, kNumLayers, {.width = 1000, .pitch = 2000});
      // Lower metals have high resistance; upper metals have low resistance.
      layers.metal(1)->setResistance(12.0);
      layers.metal(2)->setResistance(12.0);
      layers.metal(3)->setResistance(2.0);
      layers.metal(4)->setResistance(2.0);
      layers.cut(1)->setResistance(4.0);
      layers.cut(2)->setResistance(4.0);
      layers.cut(3)->setResistance(4.0);
      for (int l = 0; l < kNumLayers; ++l) {
        if (l < static_cast<int>(dirs_override.size())) {
          layers.metals[l]->setDirection(dirs_override[l]);
        }
        layer_dirs_.push_back(layers.metals[l]->getDirection());
      }
      router_ = std::make_unique<FastRouteCore>(
          db_.get(), &logger_, &registry_, &stt_builder_, getSta());
    }
    router_->setTileSize(kTileSize);
    router_->setGridsAndLayers(x_grid_, y_grid_, num_layers_);
    for (int l = 0; l < num_layers_; ++l) {
      router_->addLayerDirection(l, layer_dirs_[l]);
    }
    router_->initEdges();
    router_->initAuxVar();
    router_->setIncrementalGrt(true);
    Maze3DTestPeer::preProcessTechLayers(*router_);
    Maze3DTestPeer::setViaCost(*router_, 2);

    for (int l = 0; l < num_layers_; ++l) {
      if (layer_dirs_[l] == odb::dbTechLayerDir::HORIZONTAL) {
        for (int y = 0; y < y_grid_; ++y) {
          for (int x = 0; x < x_grid_ - 1; ++x) {
            router_->setEdgeCapacity(x, y, x + 1, y, l + 1, 10);
          }
        }
      } else {
        for (int y = 0; y < y_grid_ - 1; ++y) {
          for (int x = 0; x < x_grid_; ++x) {
            router_->setEdgeCapacity(x, y, x, y + 1, l + 1, 10);
          }
        }
      }
    }
  }

  int addTwoPinNet(const char* name,
                   const std::vector<GPoint3D>& init_grids,
                   bool res_aware,
                   int min_layer = 0,
                   int max_layer = -1)
  {
    if (max_layer < 0) {
      max_layer = num_layers_ - 1;
    }
    auto* db_net = odb::dbNet::create(block_, name);
    FrNet* net = router_->addNet(db_net,
                                 /*is_clock=*/false,
                                 /*is_local=*/false,
                                 /*driver_idx=*/0,
                                 /*cost=*/1,
                                 min_layer,
                                 max_layer,
                                 /*slack=*/-5.0f,
                                 /*edge_cost_per_layer=*/nullptr);
    net->setIsResAware(res_aware);
    net->addPin(
        init_grids.front().x, init_grids.front().y, init_grids.front().layer);
    net->addPin(
        init_grids.back().x, init_grids.back().y, init_grids.back().layer);

    int net_id = -1;
    bool exists = false;
    router_->getNetId(db_net, net_id, exists);
    EXPECT_TRUE(exists);

    StTree tree;
    tree.num_terminals = 2;
    const GPoint3D endpoints[2] = {init_grids.front(), init_grids.back()};
    for (int i = 0; i < 2; ++i) {
      tree.node_to_pin_idx[i] = i;
      TreeNode node{};
      node.x = endpoints[i].x;
      node.y = endpoints[i].y;
      node.stackAlias = i;
      node.assigned = true;
      std::ranges::fill(node.nbr, -1);
      std::ranges::fill(node.edge, -1);
      node.botL = node.topL = endpoints[i].layer;
      node.nbr[0] = 1 - i;
      node.edge[0] = 0;
      node.nbr_count = 1;
      node.eID[0] = 0;
      node.heights[0] = endpoints[i].layer;
      node.conCNT = 1;
      tree.nodes.push_back(node);
    }
    TreeEdge edge{};
    edge.n1 = edge.n1a = 0;
    edge.n2 = edge.n2a = 1;
    edge.assigned = true;
    edge.len = std::abs(endpoints[0].x - endpoints[1].x)
               + std::abs(endpoints[0].y - endpoints[1].y);
    edge.route.type = RouteType::MazeRoute;
    edge.route.grids = init_grids;
    edge.route.routelen = static_cast<int>(init_grids.size()) - 1;
    for (int i = 0; i < edge.route.routelen; ++i) {
      const auto& a = init_grids[i];
      const auto& b = init_grids[i + 1];
      if (a.layer == b.layer) {
        router_->updateEdge2DAnd3DUsage(std::min(a.x, b.x),
                                        std::min(a.y, b.y),
                                        std::max(a.x, b.x),
                                        std::max(a.y, b.y),
                                        a.layer + 1,
                                        1,
                                        db_net);
      }
    }
    tree.edges.push_back(std::move(edge));
    Maze3DTestPeer::setTree(*router_, net_id, std::move(tree));
    return net_id;
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_builder_{&logger_};
  std::unique_ptr<FastRouteCore> router_;
  std::vector<odb::dbTechLayerDir> layer_dirs_;
  int x_grid_ = 0;
  int y_grid_ = 0;
  int num_layers_ = 0;
};

TEST_F(MultiLayerMaze3DTest, NonResAwareAStarMatchesDijkstraWithObstacles)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 3);

  // Initial route from (0, 1, M1) to (6, 4, M1).
  std::vector<GPoint3D> init_route;
  for (int x = 0; x <= 6; ++x) {
    init_route.push_back({static_cast<int16_t>(x), 1, 0});
  }
  init_route.push_back({6, 1, 1});
  for (int y = 2; y <= 4; ++y) {
    init_route.push_back({6, static_cast<int16_t>(y), 1});
  }
  init_route.push_back({6, 4, 0});

  const int net_id = addTwoPinNet("n_nonres", init_route, /*res_aware=*/false);
  const int dummy_id
      = addTwoPinNet("dummy", {{0, 0, 0}, {1, 0, 0}}, /*res_aware=*/false);

  // Block horizontal edges at x=3 on M1 for y=0..3 to force a detour or climb.
  for (int y = 0; y <= 3; ++y) {
    router_->addAdjustment(3, y, 4, y, 1, 0, true);
  }

  // Compute reference Dijkstra cost after ripping up net's own usage (the
  // blocked edges have cap=0; unblocked edges have cap=10 >> 1).
  const int expected_cost
      = referenceDijkstraCost3D(*router_,
                                net_id,
                                kXGrid,
                                kYGrid,
                                kNumLayers,
                                /*min_layer=*/0,
                                /*max_layer=*/kNumLayers - 1,
                                layer_dirs_,
                                {0, 1, 0},
                                {6, 4, 0},
                                /*region_x1=*/0,
                                /*region_x2=*/kXGrid - 1,
                                /*region_y1=*/0,
                                /*region_y2=*/kYGrid - 1);

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/10, 0, 100);

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  const int actual_cost = evaluateRouteCost3D(
      *router_, net_id, routed.edges[0].route.grids, 0, 0);
  EXPECT_EQ(actual_cost, expected_cost);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, ResAwareAStarMatchesDijkstraAndUsesUpperLayer)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  router_->setResistanceAware(true);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Long horizontal run from (0, 2, M1) to (12, 2, M1): M1 costs 12 * 3 = 36,
  // while climbing to M3 costs 12 * 2 + 4 * 2 = 32.
  std::vector<GPoint3D> init_route;
  for (int x = 0; x <= 12; ++x) {
    init_route.push_back({static_cast<int16_t>(x), 2, 0});
  }
  const int net_id = addTwoPinNet("n_res", init_route, /*res_aware=*/true);

  Maze3DTestPeer::setActiveResistanceAware(*router_, true);
  const int unblocked_expected
      = referenceDijkstraCost3D(*router_,
                                net_id,
                                kXGrid,
                                kYGrid,
                                kNumLayers,
                                /*min_layer=*/0,
                                /*max_layer=*/kNumLayers - 1,
                                layer_dirs_,
                                {0, 2, 0},
                                {12, 2, 0},
                                /*region_x1=*/0,
                                /*region_x2=*/kXGrid - 1,
                                /*region_y1=*/0,
                                /*region_y2=*/kYGrid - 1);
  Maze3DTestPeer::setActiveResistanceAware(*router_, false);

  Maze3DTestPeer::route(*router_, {net_id}, /*expand=*/10, 0, 100);

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  const auto& grids = routed.edges[0].route.grids;
  Maze3DTestPeer::setActiveResistanceAware(*router_, true);
  const int unblocked_actual
      = evaluateRouteCost3D(*router_, net_id, grids, 0, 0);
  Maze3DTestPeer::setActiveResistanceAware(*router_, false);

  // Route must climb to low-resistance layer M3 (layer index 2) and match exact
  // 3D Dijkstra optimal cost.
  const bool uses_upper_layer
      = std::any_of(grids.begin(), grids.end(), [](const GPoint3D& p) {
          return p.layer >= 2;
        });
  EXPECT_TRUE(uses_upper_layer);
  EXPECT_EQ(unblocked_actual, unblocked_expected);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));

  // Now block M3 at (6, 2) -> (7, 2) while leaving M1 open: A* must fall back
  // cleanly without losing optimality or dirtying d1_3D_.
  router_->addAdjustment(6, 2, 7, 2, 3, 0, true);
  Maze3DTestPeer::setActiveResistanceAware(*router_, true);
  const int blocked_lower_bound = referenceDijkstraCost3D(*router_,
                                                          net_id,
                                                          kXGrid,
                                                          kYGrid,
                                                          kNumLayers,
                                                          0,
                                                          kNumLayers - 1,
                                                          layer_dirs_,
                                                          {0, 2, 0},
                                                          {12, 2, 0},
                                                          0,
                                                          kXGrid - 1,
                                                          0,
                                                          kYGrid - 1);
  Maze3DTestPeer::setActiveResistanceAware(*router_, false);

  Maze3DTestPeer::route(*router_, {net_id}, /*expand=*/10, 0, 100);
  const auto routed_blocked = Maze3DTestPeer::getTree(*router_, net_id);
  Maze3DTestPeer::setActiveResistanceAware(*router_, true);
  const int blocked_actual = evaluateRouteCost3D(
      *router_, net_id, routed_blocked.edges[0].route.grids, 0, 0);
  Maze3DTestPeer::setActiveResistanceAware(*router_, false);
  // Straight M1 route has no detour so exact cost equals Dijkstra lower bound.
  EXPECT_EQ(blocked_actual, blocked_lower_bound);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest,
       SparseResetPersistsAcrossCallsResizeClearAndWorkers)
{
  setupRouter(6, 5, 4);
  EXPECT_FALSE(Maze3DTestPeer::isD1Clean(*router_));

  const int net_id = addTwoPinNet(
      "repeat_net",
      {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 0, 1}, {2, 1, 1}, {2, 2, 1}},
      /*res_aware=*/false);
  const int dummy_id = addTwoPinNet(
      "repeat_dummy", {{0, 4, 0}, {1, 4, 0}}, /*res_aware=*/false);

  // Repeated calls reuse the clean d1_3D_ grid and yield identical routes.
  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/4, 0, 100);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
  const auto first_grids
      = Maze3DTestPeer::getTree(*router_, net_id).edges[0].route.grids;

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/4, 0, 100);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
  const auto second_grids
      = Maze3DTestPeer::getTree(*router_, net_id).edges[0].route.grids;
  ASSERT_EQ(first_grids.size(), second_grids.size());
  for (size_t i = 0; i < first_grids.size(); ++i) {
    EXPECT_EQ(first_grids[i].x, second_grids[i].x);
    EXPECT_EQ(first_grids[i].y, second_grids[i].y);
    EXPECT_EQ(first_grids[i].layer, second_grids[i].layer);
  }

  // Snapshot batch worker gets independent clean-tracking state.
  auto worker = Maze3DTestPeer::buildWorker(*router_);
  EXPECT_FALSE(Maze3DTestPeer::isD1Clean(*worker));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*worker));

  // clear() invalidates d1_3D_clean_.
  router_->clear();
  EXPECT_FALSE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));

  // Re-initializing with a larger grid invalidates and cleanly rebuilds d1_3D_.
  setupRouter(8, 6, 4);
  EXPECT_FALSE(Maze3DTestPeer::isD1Clean(*router_));
  const int resized_net
      = addTwoPinNet("resized_net",
                     {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}},
                     /*res_aware=*/false);
  const int resized_dummy = addTwoPinNet(
      "resized_dummy", {{0, 5, 0}, {1, 5, 0}}, /*res_aware=*/false);
  Maze3DTestPeer::route(
      *router_, {resized_net, resized_dummy}, /*expand=*/4, 0, 100);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));

  // Shrinking the grid without an intervening clear() must discard stale
  // frontier entries from src_heap_3D_ before the next route call.
  setupRouter(4, 3, 2);
  EXPECT_FALSE(Maze3DTestPeer::isD1Clean(*router_));
  const int shrunk_net = addTwoPinNet(
      "shrunk_net", {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}, /*res_aware=*/false);
  const int shrunk_dummy = addTwoPinNet(
      "shrunk_dummy", {{0, 2, 0}, {1, 2, 0}}, /*res_aware=*/false);
  Maze3DTestPeer::route(
      *router_, {shrunk_net, shrunk_dummy}, /*expand=*/4, 0, 100);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, EqualFTieBreakPreservesUpperLayer)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Source at (0, 0, M4 [layer 3, vertical]) to destination at (4, 3, M1
  // [layer 0, horizontal]). All monotone descending paths share the exact same
  // total cost f = 3 vias + 7 wire steps = 10, est_vias = 3, and est_wl = 7.
  // Equal-f tie-breaking on (f, est_vias, est_wl, h, dir_guarded_layer) keeps
  // planar routing on upper layers (M4/M3) rather than dropping immediately to
  // M2/M1 at the source.
  const std::vector<GPoint3D> init_route{
      {0, 0, 3},
      {0, 0, 2},
      {0, 0, 1},
      {0, 1, 1},
      {0, 2, 1},
      {0, 3, 1},
      {0, 3, 0},
      {1, 3, 0},
      {2, 3, 0},
      {3, 3, 0},
      {4, 3, 0},
  };
  const int net_id = addTwoPinNet("upper_src", init_route, /*res_aware=*/false);
  const int dummy_id = addTwoPinNet(
      "upper_dummy", {{0, 5, 0}, {1, 5, 0}}, /*res_aware=*/false);

  const int expected_cost = referenceDijkstraCost3D(*router_,
                                                    net_id,
                                                    kXGrid,
                                                    kYGrid,
                                                    kNumLayers,
                                                    0,
                                                    kNumLayers - 1,
                                                    layer_dirs_,
                                                    {0, 0, 3},
                                                    {4, 3, 0},
                                                    0,
                                                    kXGrid - 1,
                                                    0,
                                                    kYGrid - 1);

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/4, 0, 100);

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  const auto& grids = routed.edges[0].route.grids;
  EXPECT_EQ(evaluateRouteCost3D(*router_, net_id, grids, 3, 0), expected_cost);
  // First segment must continue on M4 (layer 3) instead of dropping early.
  ASSERT_GE(grids.size(), 2);
  EXPECT_EQ(grids[0].layer, 3);
  EXPECT_EQ(grids[1].layer, 3);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, EqualFTieBreakAscendingUsesUpperLayerBeforeDest)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Symmetric ascending counterpart to EqualFTieBreakPreservesUpperLayer:
  // source at (0, 0, M1 [layer 0, horizontal]) to destination at (4, 3, M4
  // [layer 3, vertical]). With symmetric (f, est_vias, est_wl, h, layer_pref)
  // keys, the route must climb to upper metal before reaching the destination
  // pin rather than pinning the entire run onto M1/M2 until (4, 3).
  const std::vector<GPoint3D> init_route{
      {0, 0, 0},
      {1, 0, 0},
      {2, 0, 0},
      {3, 0, 0},
      {4, 0, 0},
      {4, 0, 1},
      {4, 1, 1},
      {4, 2, 1},
      {4, 3, 1},
      {4, 3, 2},
      {4, 3, 3},
  };
  const int net_id = addTwoPinNet("asc_src", init_route, /*res_aware=*/false);
  const int dummy_id
      = addTwoPinNet("asc_dummy", {{0, 5, 0}, {1, 5, 0}}, /*res_aware=*/false);

  const int expected_cost = referenceDijkstraCost3D(*router_,
                                                    net_id,
                                                    kXGrid,
                                                    kYGrid,
                                                    kNumLayers,
                                                    0,
                                                    kNumLayers - 1,
                                                    layer_dirs_,
                                                    {0, 0, 0},
                                                    {4, 3, 3},
                                                    0,
                                                    kXGrid - 1,
                                                    0,
                                                    kYGrid - 1);

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/4, 0, 100);

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  const auto& grids = routed.edges[0].route.grids;
  EXPECT_EQ(evaluateRouteCost3D(*router_, net_id, grids, 0, 3), expected_cost);
  ASSERT_GE(grids.size(), 2);
  // Route must climb onto upper metal (M3/M4) for planar routing instead of
  // staying pinned on M1/M2 until the destination pin.
  const bool uses_upper_planar = [&]() {
    for (size_t i = 0; i + 1 < grids.size(); ++i) {
      if (grids[i].layer == grids[i + 1].layer && grids[i].layer >= 2) {
        return true;
      }
    }
    return false;
  }();
  EXPECT_TRUE(uses_upper_planar);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, RoutableMultiPinNetRoutesOptimallyAndCleansState)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Build a routable 3-pin Steiner tree with a suboptimal detoured branch so
  // 3D A* re-routes multi-seed src/dest subtrees and shifts cleanly.
  auto* db_net = odb::dbNet::create(block_, "multi_pin");
  FrNet* net = router_->addNet(db_net,
                               /*is_clock=*/false,
                               /*is_local=*/false,
                               /*driver_idx=*/0,
                               /*cost=*/1,
                               /*min_layer=*/0,
                               /*max_layer=*/kNumLayers - 1,
                               /*slack=*/0.0f,
                               /*edge_cost_per_layer=*/nullptr);
  int net_id = -1;
  bool exists = false;
  router_->getNetId(db_net, net_id, exists);
  ASSERT_TRUE(exists);

  const std::vector<GPoint3D> pins{{0, 1, 0}, {6, 1, 0}, {3, 4, 1}};
  const std::vector<GPoint3D> steiner{{3, 1, 0}};
  for (const auto& pin : pins) {
    net->addPin(pin.x, pin.y, pin.layer);
  }

  StTree tree;
  tree.num_terminals = static_cast<int>(pins.size());
  std::vector<GPoint3D> points = pins;
  points.insert(points.end(), steiner.begin(), steiner.end());
  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    if (i < tree.num_terminals) {
      tree.node_to_pin_idx[i] = i;
    }
    TreeNode node{};
    node.x = points[i].x;
    node.y = points[i].y;
    node.stackAlias = i;
    node.assigned = true;
    std::ranges::fill(node.nbr, -1);
    std::ranges::fill(node.edge, -1);
    node.botL = node.topL = points[i].layer;
    tree.nodes.push_back(node);
  }

  const std::vector<RoutedEdge> edges{
      {0, 3, {{0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {3, 1, 0}}},
      {3, 1, {{3, 1, 0}, {4, 1, 0}, {5, 1, 0}, {6, 1, 0}}},
      // Detoured branch from (3, 1, 0) to (3, 4, 1) going via x=4.
      {3,
       2,
       {{3, 1, 0},
        {4, 1, 0},
        {4, 1, 1},
        {4, 2, 1},
        {4, 3, 1},
        {4, 4, 1},
        {4, 4, 0},
        {3, 4, 0},
        {3, 4, 1}}},
  };
  for (const auto& re : edges) {
    const int edge_id = static_cast<int>(tree.edges.size());
    TreeEdge edge{};
    edge.n1 = edge.n1a = re.n1;
    edge.n2 = edge.n2a = re.n2;
    edge.assigned = true;
    edge.len = std::abs(re.grids.front().x - re.grids.back().x)
               + std::abs(re.grids.front().y - re.grids.back().y);
    edge.route.type = RouteType::MazeRoute;
    edge.route.grids = re.grids;
    edge.route.routelen = static_cast<int>(re.grids.size()) - 1;
    for (int node_id : {edge.n1, edge.n2}) {
      auto& node = tree.nodes[node_id];
      const int layer = (node_id == edge.n1) ? re.grids.front().layer
                                             : re.grids.back().layer;
      node.nbr[node.nbr_count] = (node_id == edge.n1) ? edge.n2 : edge.n1;
      node.edge[node.nbr_count++] = edge_id;
      node.eID[node.conCNT] = edge_id;
      node.heights[node.conCNT++] = layer;
      node.botL = std::min<int>(node.botL, layer);
      node.topL = std::max<int>(node.topL, layer);
    }
    for (int i = 0; i < edge.route.routelen; ++i) {
      const auto& a = re.grids[i];
      const auto& b = re.grids[i + 1];
      if (a.layer == b.layer) {
        router_->updateEdge2DAnd3DUsage(std::min(a.x, b.x),
                                        std::min(a.y, b.y),
                                        std::max(a.x, b.x),
                                        std::max(a.y, b.y),
                                        a.layer + 1,
                                        1,
                                        db_net);
      }
    }
    tree.edges.push_back(std::move(edge));
  }
  Maze3DTestPeer::setTree(*router_, net_id, std::move(tree));
  const int dummy_id = addTwoPinNet(
      "multi_dummy", {{0, 5, 0}, {1, 5, 0}}, /*res_aware=*/false);

  ASSERT_NO_THROW(Maze3DTestPeer::route(
      *router_, {net_id, dummy_id}, /*expand=*/4, 0, 100));

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  ASSERT_EQ(routed.edges.size(), 3);
  // The detoured vertical branch (edge 2) should straighten to length 3 (x=3,
  // y=1..4 on M2) instead of the original detoured 8-step route.
  EXPECT_EQ(routed.edges[2].route.routelen, 3);
  EXPECT_EQ(logger_.getWarningCount(), 0);
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, EqualFTieBreakPrefersFewerTotalVias)
{
  setupRouter(kXGrid, kYGrid, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Build a 3-pin tree whose first edge connects (0, 1, M1) to (2, 1, M3) via
  // M3 so the source subtree contains both (0, 1, M1) and (2, 1, M3), while
  // the second edge connects to (3, 1, M1). From the source subtree to
  // (3, 1, M1), both paths have identical total cost f = 3:
  //   - From (0, 1, M1): 3 wires + 0 vias (est_vias = 0, est_wl = 3)
  //   - From (2, 1, M3): 1 wire + 2 vias (est_vias = 2, est_wl = 1)
  // Ordering est_vias ahead of est_wl aligns with FastRoute's wirelength +
  // 3*vias objective and selects the 0-via path.
  auto* db_net = odb::dbNet::create(block_, "fewer_total_vias");
  FrNet* net = router_->addNet(db_net,
                               /*is_clock=*/false,
                               /*is_local=*/false,
                               /*driver_idx=*/0,
                               /*cost=*/1,
                               /*min_layer=*/0,
                               /*max_layer=*/kNumLayers - 1,
                               /*slack=*/0.0f,
                               /*edge_cost_per_layer=*/nullptr);
  int net_id = -1;
  bool exists = false;
  router_->getNetId(db_net, net_id, exists);
  ASSERT_TRUE(exists);

  const std::vector<GPoint3D> pins{{0, 1, 0}, {2, 1, 2}, {3, 1, 0}};
  for (const auto& pin : pins) {
    net->addPin(pin.x, pin.y, pin.layer);
  }

  StTree tree;
  tree.num_terminals = static_cast<int>(pins.size());
  for (int i = 0; i < tree.num_terminals; ++i) {
    tree.node_to_pin_idx[i] = i;
    TreeNode node{};
    node.x = pins[i].x;
    node.y = pins[i].y;
    node.stackAlias = i;
    node.assigned = true;
    std::ranges::fill(node.nbr, -1);
    std::ranges::fill(node.edge, -1);
    node.botL = node.topL = pins[i].layer;
    tree.nodes.push_back(node);
  }

  // Route the detoured edge to pin 2 first while the (0,1,M1)->(2,1,M3) branch
  // is intact in the source subtree. Ordering est_vias ahead of est_wl connects
  // pin 2 to (0,1,M1) with 0 vias along M1, after which the M3 pin connects
  // vertically at (2,1) — reducing total tree vias from 4 to 2.
  const std::vector<RoutedEdge> edges{
      // Edge 0 (routed first): initial detoured edge from (2,1,2) to (3,1,0).
      {1,
       2,
       {{2, 1, 2},
        {2, 1, 1},
        {2, 2, 1},
        {2, 2, 0},
        {3, 2, 0},
        {3, 2, 1},
        {3, 1, 1},
        {3, 1, 0}}},
      // Edge 1: intact branch supplying both (0,1,M1) and (2,1,M3) seeds.
      {0, 1, {{0, 1, 0}, {0, 1, 1}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}}},
  };
  for (const auto& re : edges) {
    const int edge_id = static_cast<int>(tree.edges.size());
    TreeEdge edge{};
    edge.n1 = edge.n1a = re.n1;
    edge.n2 = edge.n2a = re.n2;
    edge.assigned = true;
    edge.len = std::abs(re.grids.front().x - re.grids.back().x)
               + std::abs(re.grids.front().y - re.grids.back().y);
    edge.route.type = RouteType::MazeRoute;
    edge.route.grids = re.grids;
    edge.route.routelen = static_cast<int>(re.grids.size()) - 1;
    for (int node_id : {edge.n1, edge.n2}) {
      auto& node = tree.nodes[node_id];
      const int layer = (node_id == edge.n1) ? re.grids.front().layer
                                             : re.grids.back().layer;
      node.nbr[node.nbr_count] = (node_id == edge.n1) ? edge.n2 : edge.n1;
      node.edge[node.nbr_count++] = edge_id;
      node.eID[node.conCNT] = edge_id;
      node.heights[node.conCNT++] = layer;
      node.botL = std::min<int>(node.botL, layer);
      node.topL = std::max<int>(node.topL, layer);
    }
    for (int i = 0; i < edge.route.routelen; ++i) {
      const auto& a = re.grids[i];
      const auto& b = re.grids[i + 1];
      if (a.layer == b.layer) {
        router_->updateEdge2DAnd3DUsage(std::min(a.x, b.x),
                                        std::min(a.y, b.y),
                                        std::max(a.x, b.x),
                                        std::max(a.y, b.y),
                                        a.layer + 1,
                                        1,
                                        db_net);
      }
    }
    tree.edges.push_back(std::move(edge));
  }
  Maze3DTestPeer::setTree(*router_, net_id, std::move(tree));
  const int dummy_id = addTwoPinNet(
      "no_detour_dummy", {{0, 5, 0}, {1, 5, 0}}, /*res_aware=*/false);

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/4, 0, 100);

  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  // Pin 2 at (3, 1, M1) must attach on M1 with 0 vias along its routed edge
  // (connecting from (0, 1, M1)), leaving only the single vertical pin-1 via
  // stack on the tree.
  bool found_pin2_edge = false;
  for (const auto& e : routed.edges) {
    if (e.len <= 0 || e.route.grids.empty()) {
      continue;
    }
    const auto& g = e.route.grids;
    const bool touches_pin2 = (g.front().x == 3 && g.front().y == 1)
                              || (g.back().x == 3 && g.back().y == 1);
    if (!touches_pin2) {
      continue;
    }
    found_pin2_edge = true;
    int via_steps = 0;
    for (size_t i = 0; i + 1 < g.size(); ++i) {
      if (g[i].layer != g[i + 1].layer) {
        ++via_steps;
      }
    }
    EXPECT_EQ(via_steps, 0);
    EXPECT_EQ(g.front().layer, 0);
    EXPECT_EQ(g.back().layer, 0);
  }
  EXPECT_TRUE(found_pin2_edge);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, AStarExpandsLinearNodesOnEqualFPlateau)
{
  // On a 32x32x4 grid, tie-breaking by smaller h (with direction-guarded upper
  // layer preference) must expand Theta(W + H + L) nodes instead of exploring
  // the Theta(W * H * L) optimal bounding box.
  constexpr int kW = 32;
  constexpr int kH = 32;
  setupRouter(kW, kH, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Case 1: Ascending (0, 0, M1) -> (30, 30, M4) (optimal f* = 63).
  std::vector<GPoint3D> asc_init;
  for (int x = 0; x <= 30; ++x) {
    asc_init.push_back({static_cast<int16_t>(x), 0, 0});
  }
  asc_init.push_back({30, 0, 1});
  for (int y = 1; y <= 30; ++y) {
    asc_init.push_back({30, static_cast<int16_t>(y), 1});
  }
  asc_init.push_back({30, 30, 2});
  asc_init.push_back({30, 30, 3});
  const int asc_id = addTwoPinNet("plateau_asc", asc_init, /*res_aware=*/false);
  const int dummy_id = addTwoPinNet(
      "plateau_dummy", {{0, 31, 0}, {1, 31, 0}}, /*res_aware=*/false);

  Maze3DTestPeer::route(*router_, {asc_id, dummy_id}, /*expand=*/1, 0, 100);
  const size_t asc_pops = Maze3DTestPeer::getLastMaze3DPops(*router_);
  // Optimal path length is 63; Theta(W*H*L) BFS would exceed 2,000 pops.
  EXPECT_LE(asc_pops, 80u);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));

  // Case 2: Same-layer (0, 0, M1) -> (30, 30, M1) (optimal f* = 62).
  router_->clear();
  setupRouter(kW, kH, kNumLayers);
  Maze3DTestPeer::setViaCost(*router_, 1);
  std::vector<GPoint3D> same_init;
  for (int x = 0; x <= 30; ++x) {
    same_init.push_back({static_cast<int16_t>(x), 0, 0});
  }
  same_init.push_back({30, 0, 1});
  for (int y = 1; y <= 30; ++y) {
    same_init.push_back({30, static_cast<int16_t>(y), 1});
  }
  same_init.push_back({30, 30, 0});
  const int same_id
      = addTwoPinNet("plateau_same", same_init, /*res_aware=*/false);
  const int dummy2_id = addTwoPinNet(
      "plateau_dummy2", {{0, 31, 0}, {1, 31, 0}}, /*res_aware=*/false);

  Maze3DTestPeer::route(*router_, {same_id, dummy2_id}, /*expand=*/1, 0, 100);
  const size_t same_pops = Maze3DTestPeer::getLastMaze3DPops(*router_);
  EXPECT_LE(same_pops, 80u);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, ResAwareDetourPenaltyTriggersAndAvoidsDetour)
{
  constexpr int kWideXGrid = 18;
  setupRouter(kWideXGrid, kYGrid, kNumLayers);
  router_->setResistanceAware(true);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Straight horizontal run from (1, 1, M1) to (16, 1, M1) has
  // original_len = 15. With M1/M2 step = 3, M3/M4 step = 2, and via = 2:
  //   - Direct M1 (0 detour): 15 * 3 = 45.
  //   - Midpoint M3 block at (8, 1)-(9, 1) rules out any cheaper straight
  //     M1+M3 hybrid (best straight hybrid >= 46).
  //   - Detoured M3 via y = 2 (17 wire steps = 2 detour steps):
  //     unpenalized = 4 * 2 + 2 * 3 + 15 * 2 = 44 < 45,
  //     with kLowDetourPenalty (5) = 44 + 2 * 5 = 54 > 45.
  std::vector<GPoint3D> init_route;
  for (int x = 1; x <= 16; ++x) {
    init_route.push_back({static_cast<int16_t>(x), 1, 0});
  }
  const int net_id
      = addTwoPinNet("n_detour_pen", init_route, /*res_aware=*/true);

  router_->addAdjustment(8, 1, 9, 1, 3, 0, true);

  // Confirm unpenalized reference Dijkstra prefers the detoured M3 path (44)
  // over direct M1 (45), proving detour_penalty_ is what flips the choice.
  Maze3DTestPeer::setActiveResistanceAware(*router_, true);
  const int unpenalized_cost = referenceDijkstraCost3D(*router_,
                                                       net_id,
                                                       kWideXGrid,
                                                       kYGrid,
                                                       kNumLayers,
                                                       0,
                                                       kNumLayers - 1,
                                                       layer_dirs_,
                                                       {1, 1, 0},
                                                       {16, 1, 0},
                                                       0,
                                                       kWideXGrid - 1,
                                                       0,
                                                       kYGrid - 1);
  EXPECT_EQ(unpenalized_cost, 44);

  Maze3DTestPeer::route(*router_, {net_id}, /*expand=*/4, 0, 100);

  const auto direct = Maze3DTestPeer::getTree(*router_, net_id);
  int direct_planar = 0;
  for (size_t i = 0; i + 1 < direct.edges[0].route.grids.size(); ++i) {
    if (direct.edges[0].route.grids[i].layer
        == direct.edges[0].route.grids[i + 1].layer) {
      ++direct_planar;
    }
  }
  EXPECT_EQ(direct_planar, 15);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));

  // Now also block M1 at (8, 1)-(9, 1) so every legal path must detour
  // (wire_len = 17 > original_len = 15), triggering detour_penalty_ end-to-end.
  router_->addAdjustment(8, 1, 9, 1, 1, 0, true);
  Maze3DTestPeer::route(*router_, {net_id}, /*expand=*/4, 0, 100);

  const auto detoured = Maze3DTestPeer::getTree(*router_, net_id);
  int detoured_planar = 0;
  for (size_t i = 0; i + 1 < detoured.edges[0].route.grids.size(); ++i) {
    if (detoured.edges[0].route.grids[i].layer
        == detoured.edges[0].route.grids[i + 1].layer) {
      ++detoured_planar;
    }
  }
  EXPECT_EQ(detoured_planar, 17);
  EXPECT_EQ(logger_.getWarningCount(), 0);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest, LayerZeroValidSpanBeatsInvalidSpanSentinel)
{
  // Build a 4-layer stack where M1 (layer 0) is HORIZONTAL while M2 (layer 1)
  // and M3 (layer 2) are both VERTICAL, and M4 (layer 3) is HORIZONTAL.
  // For destination (6, 1, M2 [layer 1]) with dx = 5, dy = 0:
  //   - Seed (1, 1, M3 [layer 2]): dl = 1, span [1, 2] has no H layer ->
  //     valid_span = false (sentinel layer_pref = 63).
  //   - Seed (1, 1, M1 [layer 0]): dl = 1, span [0, 1] contains M1 (H) ->
  //     valid_span = true (layer_pref = 62 < 63).
  // Both seeds have identical (f=6, est_vias=1, est_wl=5, h=6). Fixing
  // layer_pref = 62 - clamp(layer, 0, 62) ensures M1 strictly beats the
  // invalid-span M3 seed regardless of seed insertion order.
  const std::vector<odb::dbTechLayerDir> custom_dirs{
      odb::dbTechLayerDir::HORIZONTAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::VERTICAL,
      odb::dbTechLayerDir::HORIZONTAL,
  };
  setupRouter(10, 6, 4, custom_dirs);
  Maze3DTestPeer::setViaCost(*router_, 1);

  auto* db_net = odb::dbNet::create(block_, "m1_vs_invalid_span");
  FrNet* net = router_->addNet(db_net,
                               /*is_clock=*/false,
                               /*is_local=*/false,
                               /*driver_idx=*/0,
                               /*cost=*/1,
                               /*min_layer=*/0,
                               /*max_layer=*/3,
                               /*slack=*/0.0f,
                               /*edge_cost_per_layer=*/nullptr);
  int net_id = -1;
  bool exists = false;
  router_->getNetId(db_net, net_id, exists);
  ASSERT_TRUE(exists);

  // Pin 0 at (1, 1, M3), Pin 1 at (1, 1, M1), Pin 2 at (6, 1, M2).
  // Edge 0 connects Pin 0 (M3) -> Pin 2 (M2) and is routed while Edge 1
  // keeps the vertical stack (1, 1, M3) -> (1, 1, M2) -> (1, 1, M1) intact
  // in the source subtree. Place M3 seed first via Pin 0 so M1 must beat it
  // by key order (layer_pref 62 < 63).
  const std::vector<GPoint3D> pins{{1, 1, 2}, {1, 1, 0}, {6, 1, 1}};
  for (const auto& pin : pins) {
    net->addPin(pin.x, pin.y, pin.layer);
  }

  StTree tree;
  tree.num_terminals = static_cast<int>(pins.size());
  for (int i = 0; i < tree.num_terminals; ++i) {
    tree.node_to_pin_idx[i] = i;
    TreeNode node{};
    node.x = pins[i].x;
    node.y = pins[i].y;
    node.stackAlias = i;
    node.assigned = true;
    std::ranges::fill(node.nbr, -1);
    std::ranges::fill(node.edge, -1);
    node.botL = node.topL = pins[i].layer;
    tree.nodes.push_back(node);
  }

  const std::vector<RoutedEdge> edges{
      // Edge 0 (routed): (1, 1, M3) -> (6, 1, M2) with initial detoured route.
      {0,
       2,
       {{1, 1, 2},
        {1, 1, 3},
        {2, 1, 3},
        {3, 1, 3},
        {4, 1, 3},
        {5, 1, 3},
        {6, 1, 3},
        {6, 1, 2},
        {6, 1, 1}}},
      // Edge 1 (zero-len vertical stack): supplies (1, 1, M3) and (1, 1, M1)
      // in the source subtree (edge.len == 0 so it is not ripped up).
      {0, 1, {{1, 1, 2}, {1, 1, 1}, {1, 1, 0}}},
  };
  for (const auto& re : edges) {
    const int edge_id = static_cast<int>(tree.edges.size());
    TreeEdge edge{};
    edge.n1 = edge.n1a = re.n1;
    edge.n2 = edge.n2a = re.n2;
    edge.assigned = true;
    edge.len = std::abs(re.grids.front().x - re.grids.back().x)
               + std::abs(re.grids.front().y - re.grids.back().y);
    edge.route.type = RouteType::MazeRoute;
    edge.route.grids = re.grids;
    edge.route.routelen = static_cast<int>(re.grids.size()) - 1;
    for (int node_id : {edge.n1, edge.n2}) {
      auto& node = tree.nodes[node_id];
      const int layer = (node_id == edge.n1) ? re.grids.front().layer
                                             : re.grids.back().layer;
      node.nbr[node.nbr_count] = (node_id == edge.n1) ? edge.n2 : edge.n1;
      node.edge[node.nbr_count++] = edge_id;
      node.eID[node.conCNT] = edge_id;
      node.heights[node.conCNT++] = layer;
      node.botL = std::min<int>(node.botL, layer);
      node.topL = std::max<int>(node.topL, layer);
    }
    for (int i = 0; i < edge.route.routelen; ++i) {
      const auto& a = re.grids[i];
      const auto& b = re.grids[i + 1];
      if (a.layer == b.layer) {
        router_->updateEdge2DAnd3DUsage(std::min(a.x, b.x),
                                        std::min(a.y, b.y),
                                        std::max(a.x, b.x),
                                        std::max(a.y, b.y),
                                        a.layer + 1,
                                        1,
                                        db_net);
      }
    }
    tree.edges.push_back(std::move(edge));
  }
  Maze3DTestPeer::setTree(*router_, net_id, std::move(tree));
  const int dummy_id
      = addTwoPinNet("m1_sentinel_dummy", {{0, 5, 0}, {1, 5, 0}}, false);

  Maze3DTestPeer::route(*router_, {net_id, dummy_id}, /*expand=*/2, 0, 100);

  // With layer_pref = 62 < 63, (1, 1, M1) pops before dead-end (1, 1, M3)
  // and routes straight along M1 to (6, 1, M1) -> (6, 1, M2):
  //   1 pop for (1, 1, M2) (f = 5, dead-end) + 6 pops along M1 = 7 pops total.
  // If layer == 0 collided with invalid-span 63u, dead-end (1, 1, M3) would
  // pop first and add an extra wasted expansion (8 pops).
  const size_t pops = Maze3DTestPeer::getLastMaze3DPops(*router_);
  EXPECT_EQ(pops, 7u);
  const auto routed = Maze3DTestPeer::getTree(*router_, net_id);
  const auto& g = routed.edges[0].route.grids;
  ASSERT_GE(g.size(), 2u);
  EXPECT_EQ(g.front().layer, 0);
  EXPECT_EQ(g.back().layer, 0);
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

TEST_F(MultiLayerMaze3DTest,
       SingleDirectionLayerConstraintHandlesBigIntHeuristic)
{
  setupRouter(6, 5, 4);
  Maze3DTestPeer::setViaCost(*router_, 1);

  // Restrict net to horizontal M1 only (min_layer == max_layer == 0) while the
  // destination is at dy > 0, so Maze3DHeuristic::eval() returns BIG_INT and
  // packKey exercises uint32_t::max() clamping during relaxation.
  const int net_id = addTwoPinNet(
      "single_dir",
      {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {1, 2, 1}, {1, 2, 0}, {2, 2, 0}},
      /*res_aware=*/false,
      /*min_layer=*/0,
      /*max_layer=*/0);
  const int dummy_id
      = addTwoPinNet("single_dir_dummy", {{0, 4, 0}, {1, 4, 0}}, false);

  ASSERT_NO_THROW(Maze3DTestPeer::route(
      *router_, {net_id, dummy_id}, /*expand=*/2, 0, 100));
  EXPECT_TRUE(Maze3DTestPeer::isD1Clean(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isTouchedEmpty(*router_));
  EXPECT_TRUE(Maze3DTestPeer::isD1AllBigInt(*router_));
}

}  // namespace
}  // namespace grt
