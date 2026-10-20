// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gpl/Replace.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "sta/MinMax.hh"
#include "tst/IntegratedFixture.h"
#include "tst/logger_checks.h"

namespace gpl {
namespace {

class IncrementalPlacementTest : public tst::IntegratedFixture
{
 public:
  IncrementalPlacementTest()
      : IntegratedFixture(Technology::kNangate45, "_main/src/gpl/test/")
  {
  }

 protected:
  using Placement
      = std::map<std::string, std::tuple<int, int, odb::dbPlacementStatus>>;

  void SetUp() override
  {
    db_network_->disableHierarchy();
    auto chip = odb::dbChip::create(db_.get(), lib_->getTech());
    std::vector<odb::dbLib*> libs{lib_};
    odb::defin reader(db_.get(), &logger_);
    const auto file = getFilePath("_main/src/gpl/test/simple01-td.def");
    ASSERT_TRUE(reader.readChip(libs, file.c_str(), chip));
    block_ = chip->getBlock();
    sta_->postReadDef(block_);
    new_inst_ = *block_->getInsts().begin();
    resetPlacement();
    for (auto scene : sta_->scenes()) {
      ep_.setHWireSignalRC(nullptr, scene, 1e8, 2e-10);
      ep_.setVWireSignalRC(nullptr, scene, 1e8, 2e-10);
      ep_.setHWireClkRC(nullptr, scene, 1e8, 2e-10);
      ep_.setVWireClkRC(nullptr, scene, 1e8, 2e-10);
    }
    initStaDefaultSdc();
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
  }

  void resetPlacement()
  {
    for (auto inst : block_->getInsts()) {
      inst->setLocation(20000, 20000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    new_inst_->setPlacementStatus(odb::dbPlacementStatus::NONE);
  }

  Placement placement() const
  {
    Placement result;
    for (auto inst : block_->getInsts()) {
      const auto point = inst->getLocation();
      result.emplace(
          inst->getName(),
          std::make_tuple(point.x(), point.y(), inst->getPlacementStatus()));
    }
    return result;
  }

  void run(int threads, int iterations, bool routing)
  {
    sta_->setThreadCount(threads);
    PlaceOptions options;
    options.nesterovPlaceMaxIter = iterations;
    options.enable_routing_congestion = routing;
    Replace placer(db_.get(), sta_.get(), &resizer_, &grt_, nullptr, &logger_);
    placer.doIncrementalPlace(threads, options);
  }

  std::vector<float> congestion() const
  {
    std::vector<float> result;
    auto grid = block_->getGCellGrid();
    if (grid == nullptr) {
      return result;
    }
    for (auto layer : db_->getTech()->getLayers()) {
      if (layer->getRoutingLevel() == 0) {
        continue;
      }
      const auto map = grid->getLayerCongestionMap(layer);
      result.push_back(map.numRows());
      result.push_back(map.numCols());
      for (int x = 0; x < map.numRows(); ++x) {
        for (int y = 0; y < map.numCols(); ++y) {
          result.push_back(map(x, y).usage);
          result.push_back(map(x, y).capacity);
        }
      }
    }
    return result;
  }

  odb::dbInst* new_inst_ = nullptr;
};

TEST_F(IncrementalPlacementTest,
       RejectionRestoresCheckpointAndRoutesFinalPlacement)
{
  tst::MetricsCapture metrics(logger_);
  // The locked phase has its own 600-iteration budget. Capping the total at
  // 600 independently obtains that checkpoint without an unlocked move.
  for (int threads : {1, 2}) {
    SCOPED_TRACE(threads);
    resetPlacement();
    run(threads, 600, false);
    ASSERT_EQ(metrics.value("gpl__incremental__timing_protected"), "1");
    ASSERT_EQ(metrics.value("gpl__incremental__timing_reverted"), "0");
    const auto checkpoint = placement();
    const float checkpoint_ws = sta_->worstSlack(sta::MinMax::max());
    const float checkpoint_tns = sta_->totalNegativeSlack(sta::MinMax::max());
    ASSERT_EQ(new_inst_->getPlacementStatus(), odb::dbPlacementStatus::PLACED);

    resetPlacement();
    run(threads, 5000, true);
    ASSERT_EQ(metrics.value("gpl__incremental__timing_reverted"), "1");
    ASSERT_TRUE(metrics.value("gpl__incremental__timing_weighted_nets"));
    EXPECT_GT(
        std::stoi(*metrics.value("gpl__incremental__timing_weighted_nets")), 0);
    ASSERT_TRUE(metrics.value("gpl__incremental__timing_max_net_weight"));
    EXPECT_GT(
        std::stof(*metrics.value("gpl__incremental__timing_max_net_weight")),
        1.0f);
    EXPECT_EQ(placement(), checkpoint);
    EXPECT_EQ(new_inst_->getPlacementStatus(), odb::dbPlacementStatus::PLACED);
    EXPECT_NEAR(sta_->worstSlack(sta::MinMax::max()), checkpoint_ws, 1e-13);
    EXPECT_NEAR(
        sta_->totalNegativeSlack(sta::MinMax::max()), checkpoint_tns, 1e-13);
    ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
    EXPECT_NEAR(sta_->worstSlack(sta::MinMax::max()), checkpoint_ws, 1e-13);
    EXPECT_NEAR(
        sta_->totalNegativeSlack(sta::MinMax::max()), checkpoint_tns, 1e-13);

    const grt::NetRouteMap routes = grt_.getRoutes();
    ASSERT_FALSE(routes.empty());
    const auto odb_congestion = congestion();
    ASSERT_FALSE(odb_congestion.empty());
    // A fresh route of the restored coordinates must agree with both the
    // router's routes and the ODB congestion returned by incremental GPL.
    grt_.globalRoute();
    EXPECT_EQ(grt_.getRoutes(), routes);
    EXPECT_EQ(congestion(), odb_congestion);
  }
}

TEST_F(IncrementalPlacementTest, ReuseAfterAcceptAndRejectMatchesFreshPlacement)
{
  tst::MetricsCapture metrics(logger_);
  for (int threads : {1, 2}) {
    SCOPED_TRACE(threads);
    sta_->setThreadCount(threads);
    PlaceOptions regular_options;
    regular_options.nesterovPlaceMaxIter = 20;
    resetPlacement();
    {
      Replace fresh(db_.get(), sta_.get(), &resizer_, &grt_, nullptr, &logger_);
      fresh.doPlace(threads, regular_options);
    }
    const auto fresh_placement = placement();

    for (int incremental_iterations : {600, 5000}) {
      SCOPED_TRACE(incremental_iterations);
      resetPlacement();
      PlaceOptions incremental_options;
      incremental_options.nesterovPlaceMaxIter = incremental_iterations;
      Replace reused(
          db_.get(), sta_.get(), &resizer_, &grt_, nullptr, &logger_);
      reused.doIncrementalPlace(threads, incremental_options);
      ASSERT_EQ(metrics.value("gpl__incremental__timing_protected"), "1");
      ASSERT_EQ(metrics.value("gpl__incremental__timing_reverted"),
                incremental_iterations == 600 ? "0" : "1");
      ASSERT_TRUE(metrics.value("gpl__incremental__timing_weighted_nets"));
      ASSERT_GT(
          std::stoi(*metrics.value("gpl__incremental__timing_weighted_nets")),
          0);

      // No explicit reset: C++ callers can reuse Replace just as Tcl callers
      // can issue another command. Both acceptance and rollback must leave
      // the next ordinary placement independent of incremental timing state.
      resetPlacement();
      reused.doPlace(threads, regular_options);
      EXPECT_EQ(placement(), fresh_placement);
    }
  }
}

}  // namespace
}  // namespace gpl
