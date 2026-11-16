// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "gmock/gmock.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/ServiceRegistry.h"

namespace grt {
namespace {

using Via = std::tuple<int, int, int, int>;

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

// x, y, layer, is horizontal
using WireEdge = std::tuple<int, int, int, bool>;

// The wires of a route as unit GCell edges, so two routes compare equal however
// their segments are split; a route restored from guides is split per guide.
std::set<WireEdge> getWireEdges(const GRoute& route,
                                const int gcell_width,
                                const int gcell_height)
{
  std::set<WireEdge> edges;
  for (const GSegment& segment : route) {
    if (segment.isVia() || segment.init_layer != segment.final_layer) {
      continue;
    }
    const auto [x_min, x_max] = std::minmax(segment.init_x, segment.final_x);
    const auto [y_min, y_max] = std::minmax(segment.init_y, segment.final_y);
    for (int x = x_min; x < x_max; x += gcell_width) {
      edges.emplace(x, y_min, segment.init_layer, true);
    }
    for (int y = y_min; y < y_max; y += gcell_height) {
      edges.emplace(x_min, y, segment.init_layer, false);
    }
  }
  return edges;
}

class GuideRestoreFixture : public tst::Fixture
{
 protected:
  void readNangate45Design(const std::string& def_name)
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    odb::dbLib* lib = loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath("_main/src/grt/test/" + def_name);
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    sta_->postReadDef(block_);
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_{&logger_};
  ant::AntennaChecker ant_{db_.get(), &logger_};
  dpl::Opendp dpl_{db_.get(), &logger_};
  GlobalRouter
      router_{&logger_, &registry_, &stt_, db_.get(), sta_.get(), &ant_, &dpl_};
};

class GuideRestoreTest : public GuideRestoreFixture,
                         public testing::WithParamInterface<bool>
{
 protected:
  void SetUp() override
  {
    ASSERT_NO_FATAL_FAILURE(readNangate45Design("remove_buffers1.def"));
    router_.setUseCUGR(GetParam());
    router_.setMinRoutingLayer(2);
    router_.setMaxRoutingLayer(8);
    router_.globalRoute(/*save_guides=*/true);
  }
};

TEST_P(GuideRestoreTest, EcoUndoPreservesVias)
{
  const NetRouteMap original_routes = router_.getRoutes();
  ASSERT_FALSE(original_routes.empty());

  odb::dbGuide* via_guide = nullptr;
  for (const auto& [net, route] : original_routes) {
    const auto vias = getVias(route);
    const std::set<Via> unique_vias(vias.begin(), vias.end());
    EXPECT_EQ(vias.size(), unique_vias.size());
    for (odb::dbGuide* guide : net->getGuides()) {
      if (guide->getViaLayer() != nullptr
          && guide->getLayer() != guide->getViaLayer()) {
        via_guide = guide;
      }
    }
  }
  ASSERT_NE(via_guide, nullptr);

  // Model a pin-covering via saved as guides on both layers.
  odb::dbGuide::create(via_guide->getNet(),
                       via_guide->getViaLayer(),
                       via_guide->getLayer(),
                       via_guide->getBox(),
                       via_guide->isCongested());

  // Undo dispatches the guide-restore callback for each affected net.
  router_.startIncremental();
  odb::dbDatabase::beginEco(block_);
  for (const auto& [net, route] : original_routes) {
    net->clearGuides();
  }
  odb::dbDatabase::undoEco(block_);
  router_.endIncremental();

  const NetRouteMap& restored_routes = router_.getRoutes();
  ASSERT_EQ(restored_routes.size(), original_routes.size());
  for (const auto& [net, route] : original_routes) {
    SCOPED_TRACE(net->getName());
    const auto restored = restored_routes.find(net);
    ASSERT_NE(restored, restored_routes.end());
    EXPECT_EQ(getVias(restored->second), getVias(route));
  }
}

INSTANTIATE_TEST_SUITE_P(RoutingEngines,
                         GuideRestoreTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "CUGR" : "FastRoute";
                         });

// An ECO undo must also restore the nets FastRoute's congestion recovery
// rerouted beyond the dirty ones; left on their new routes, they hold tracks
// the restored dirty nets come back to.
class CongestionRecoveryUndoTest : public GuideRestoreFixture
{
 protected:
  void SetUp() override
  {
    ASSERT_NO_FATAL_FAILURE(readNangate45Design("gcd.def"));
    // Tight enough that the reroute below overflows and the recovery pulls in
    // more nets, loose enough that the recovery then clears the overflow.
    router_.setMinRoutingLayer(2);
    router_.setMaxRoutingLayer(4);
    router_.setAdjustment(0.6);
    router_.globalRoute(/*save_guides=*/true);
  }
};

TEST_F(CongestionRecoveryUndoTest, EcoUndoRestoresRecoveryReroutes)
{
  ASSERT_FALSE(router_.isCongested());
  std::vector<int> grid_x;
  std::vector<int> grid_y;
  block_->getGCellGrid()->getGridX(grid_x);
  block_->getGCellGrid()->getGridY(grid_y);
  ASSERT_GE(grid_x.size(), 2);
  ASSERT_GE(grid_y.size(), 2);
  const int gcell_width = grid_x[1] - grid_x[0];
  const int gcell_height = grid_y[1] - grid_y[0];
  auto wire_edges = [&](odb::dbNet* net) {
    return getWireEdges(router_.getRoutes().at(net), gcell_width, gcell_height);
  };

  odb::PtrMap<odb::dbNet, std::set<WireEdge>> original_edges;
  for (const auto& [net, route] : router_.getRoutes()) {
    original_edges[net] = wire_edges(net);
  }

  IncrementalGRoute incremental(&router_, block_);
  odb::dbDatabase::beginEco(block_);
  // Stack cells on one spot, as rebuffering does before legalizing.
  const odb::Rect die = block_->getDieArea();
  int stacked = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    // Fillers and tap cells are FIRM and stay put.
    if (stacked < 10
        && inst->getPlacementStatus() == odb::dbPlacementStatus::PLACED) {
      inst->setLocation(die.xCenter(), die.yCenter());
      stacked++;
    }
  }
  const odb::PtrSet<odb::dbNet> dirty_nets = router_.getDirtyNets();
  int recovery_moved = 0;
  for (odb::dbNet* net : incremental.updateRoutes()) {
    if (!dirty_nets.contains(net)
        && wire_edges(net) != original_edges.at(net)) {
      recovery_moved++;
    }
  }
  ASSERT_GT(recovery_moved, 0)
      << "the recovery no longer moves nets the stacked cells do not own; "
         "retune the scenario";

  odb::dbDatabase::undoEco(block_);
  incremental.updateRoutes();

  std::vector<std::string> off_original_route;
  for (const auto& [net, edges] : original_edges) {
    if (!router_.getRoutes().contains(net) || wire_edges(net) != edges) {
      off_original_route.push_back(net->getName());
    }
  }
  EXPECT_THAT(off_original_route, testing::IsEmpty());
}

}  // namespace
}  // namespace grt
