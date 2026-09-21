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
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
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

class GuideRestoreTest : public tst::Fixture,
                         public testing::WithParamInterface<bool>
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    odb::dbLib* lib = loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def
        = getFilePath("_main/src/grt/test/remove_buffers1.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    sta_->postReadDef(block_);

    router_.setUseCUGR(GetParam());
    router_.setMinRoutingLayer(2);
    router_.setMaxRoutingLayer(8);
    router_.globalRoute(/*save_guides=*/true);
  }

  odb::dbBlock* block_ = nullptr;
  utl::ServiceRegistry registry_{&logger_};
  stt::SteinerTreeBuilder stt_{&logger_};
  ant::AntennaChecker ant_{db_.get(), &logger_};
  dpl::Opendp dpl_{db_.get(), &logger_};
  GlobalRouter
      router_{&logger_, &registry_, &stt_, db_.get(), sta_.get(), &ant_, &dpl_};
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

}  // namespace
}  // namespace grt
