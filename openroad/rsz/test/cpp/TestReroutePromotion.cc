// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RerouteGenerator.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/GraphClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/SdcClass.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {
namespace {

class ReroutePromotionTest : public tst::IntegratedFixture
{
 protected:
  ReroutePromotionTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/openroad/rsz/test/")
  {
  }

  void loadAndRouteGcd(const bool use_cugr = false,
                       const float upper_layer_adjustment = 0.0f,
                       const bool obstruct_upper_layers = false)
  {
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::defin def_reader(db_.get(), &logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    ASSERT_TRUE(def_reader.readChip(
        search_libs,
        getFilePath("_main/openroad/rsz/test/gcd_nangate45_placed.def").c_str(),
        chip));
    block_ = db_->getChip()->getBlock();
    sta_->postReadDef(block_);

    odb::dbTech* tech = block_->getTech();
    const odb::Rect die = block_->getDieArea();
    for (int l = 1; l <= 6; ++l) {
      odb::dbTechLayer* layer = tech->findRoutingLayer(l);
      ASSERT_NE(layer, nullptr);
      if (block_->findTrackGrid(layer) == nullptr) {
        const int pitch = std::max(layer->getPitch(), 280);
        odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, layer);
        grid->addGridPatternX(pitch / 2, die.dx() / pitch, pitch);
        grid->addGridPatternY(pitch / 2, die.dy() / pitch, pitch);
      }
    }

    // Create a tight clock so setup paths have negative slack.
    sta::Cell* top_cell = db_network_->cell(db_network_->topInstance());
    ASSERT_NE(top_cell, nullptr);
    sta::Port* clk_port = db_network_->findPort(top_cell, "clk");
    ASSERT_NE(clk_port, nullptr);
    sta::Pin* clk_pin
        = db_network_->findPin(db_network_->topInstance(), clk_port);
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(clk_pin);
    const double period = sta_->units()->timeUnit()->userToSta(0.5);
    sta::FloatSeq waveform{0.0f, static_cast<float>(period / 2.0)};
    sta_->makeClock(
        "clk", clk_pins, false, period, waveform, "", sta_->cmdMode());

    // Configure calibrated layer RC: M2..M4 high resistance, M5..M6 10x lower.
    sta::Scene* scene = sta_->scenes().front();
    ep_.setHWireSignalRC(tech, scene, 1.0e8, 1.0e-10);
    ep_.setVWireSignalRC(tech, scene, 1.0e8, 1.0e-10);
    ep_.setHWireClkRC(tech, scene, 1.0e7, 1.0e-10);
    ep_.setVWireClkRC(tech, scene, 1.0e7, 1.0e-10);
    for (int l = 1; l <= 6; ++l) {
      odb::dbTechLayer* layer = tech->findRoutingLayer(l);
      ASSERT_NE(layer, nullptr);
      const double res = (l <= 4) ? 1.0e8 : 1.0e7;
      ep_.setLayerRC(layer, scene, res, 1.0e-10);
      if (odb::dbTechLayer* cut = layer->getUpperLayer()) {
        ep_.setLayerRC(cut, scene, 1.0, 0.0);
      }
    }

    sta_->ensureGraph();
    sta_->ensureLevelized();
    resizer_.initBlock();

    grt_.setUseCUGR(use_cugr);
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    if (obstruct_upper_layers) {
      const int obs_x_max = die.xMin() + (die.dx() * 7) / 10;
      const int obs_y_max = die.yMin() + (die.dy() * 7) / 10;
      for (int l = 5; l <= 6; ++l) {
        odb::dbObstruction::create(block_,
                                   tech->findRoutingLayer(l),
                                   die.xMin(),
                                   die.yMin(),
                                   obs_x_max,
                                   obs_y_max);
      }
    }
    if (upper_layer_adjustment > 0.0f) {
      grt_.addLayerAdjustment(5, upper_layer_adjustment);
      grt_.addLayerAdjustment(6, upper_layer_adjustment);
    }
    grt_.globalRoute(/*save_guides=*/true);
    ep_.estimateParasitics(est::ParasiticsSrc::kGlobalRouting);
    sta_->updateTiming(true);
  }

  static double avgWireLayer(const grt::GRoute& route)
  {
    double weighted = 0.0;
    double total = 0.0;
    for (const grt::GSegment& seg : route) {
      if (!seg.isVia() && seg.length() > 0) {
        weighted += static_cast<double>(seg.init_layer) * seg.length();
        total += seg.length();
      }
    }
    return total > 0.0 ? weighted / total : 0.0;
  }

  static bool hasWireOnLayerAtLeast(const grt::GRoute& route,
                                    const int min_layer)
  {
    for (const grt::GSegment& seg : route) {
      if (!seg.isVia() && seg.length() > 0 && seg.init_layer >= min_layer) {
        return true;
      }
    }
    return false;
  }

  static int totalWireLength(const grt::GRoute& route)
  {
    int len = 0;
    for (const grt::GSegment& seg : route) {
      if (!seg.isVia()) {
        len += seg.length();
      }
    }
    return len;
  }

  odb::dbNet* findLongLowerLayerSignalNet()
  {
    odb::dbNet* best_net = nullptr;
    int best_len = 0;
    for (const auto& [db_net, route] : grt_.getRoutes()) {
      if (db_net->isSpecial() || db_net->getSigType().isSupply()
          || db_net->getSigType() == odb::dbSigType::CLOCK
          || grt_.isNonLeafClock(db_net) || route.empty()) {
        continue;
      }
      if (avgWireLayer(route) >= 4.0) {
        continue;
      }
      const int len = totalWireLength(route);
      if (len > best_len) {
        best_len = len;
        best_net = db_net;
      }
    }
    return best_net;
  }

  sta::Slack worstSetupSlack()
  {
    sta::Slack worst = 0.0f;
    sta::Vertex* worst_v = nullptr;
    sta_->worstSlack(sta::MinMax::max(), worst, worst_v);
    return worst;
  }
};

// Verifies that RerouteMove unblocks nets already marked isNetResAware during
// initial resistance-aware GRT, promotes them onto lower-resistance upper
// layers to improve setup slack, and clears promoted_min_layers_ cleanly when
// the incremental session ends.
TEST_F(ReroutePromotionTest, PromotesAlreadyResAwareCriticalNetToUpperLayer)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  // Mark every routed signal net as already resistance-aware, reproducing the
  // lockout that previously starved RerouteMove after initial res-aware GRT.
  std::vector<odb::dbNet*> lower_layer_nets;
  for (const auto& [db_net, route] : grt_.getRoutes()) {
    if (db_net->isSpecial() || db_net->getSigType().isSupply()
        || db_net->getSigType() == odb::dbSigType::CLOCK
        || grt_.isNonLeafClock(db_net) || route.empty()) {
      continue;
    }
    grt_.setNetIsResAware(db_net, true);
    ASSERT_TRUE(grt_.isNetResAware(db_net));
    if (avgWireLayer(route) < 4.0) {
      lower_layer_nets.push_back(db_net);
    }
  }
  ASSERT_FALSE(lower_layer_nets.empty());

  const sta::Slack wns_before = worstSetupSlack();
  ASSERT_LT(wns_before, 0.0f);

  resizer_.repairSetup(/*setup_margin=*/0.0,
                       /*repair_tns_end_percent=*/0.2,
                       /*max_passes=*/50,
                       /*max_iterations=*/-1,
                       /*max_repairs_per_pass=*/1,
                       /*match_cell_footprint=*/false,
                       /*verbose=*/false,
                       /*sequence=*/{},
                       /*phases=*/"REROUTE",
                       /*path_group=*/"",
                       /*skip_pin_swap=*/true,
                       /*skip_gate_cloning=*/true,
                       /*skip_size_down_fanout=*/true,
                       /*skip_buffering=*/true,
                       /*skip_buffer_removal=*/true,
                       /*skip_buffer_to_inverters=*/true,
                       /*skip_last_gasp=*/true,
                       /*skip_vt_swap=*/true,
                       /*skip_crit_vt_swap=*/true);

  const sta::Slack wns_after = worstSetupSlack();
  EXPECT_GT(wns_after, wns_before);

  int promoted_count = 0;
  for (odb::dbNet* db_net : lower_layer_nets) {
    const auto it = grt_.getRoutes().find(db_net);
    if (it != grt_.getRoutes().end() && hasWireOnLayerAtLeast(it->second, 5)) {
      ++promoted_count;
    }
    // endIncremental() must clear promoted_min_layers_ so nothing leaks.
    EXPECT_EQ(grt_.getNetPromotedMinLayer(db_net), 0);
  }
  EXPECT_GT(promoted_count, 0);
}

// Verifies that evaluateReroutePromotion skips congested upper layer pairs
// (>70% usage) and selects the lower-resistance upper pair when underutilized.
TEST_F(ReroutePromotionTest, EvaluateReroutePromotionRespectsLayerUsage)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  const std::vector<float> uncongested
      = {0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.2f, 0.2f};
  const PromotedLayerEval ok_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(ok_eval.promoted_min_layer, 5);
  EXPECT_GE(ok_eval.reduction_ratio, 0.10f);

  const std::vector<float> congested
      = {0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.85f, 0.85f};
  const PromotedLayerEval skip_eval
      = evaluateReroutePromotion(resizer_, target_net, congested);
  EXPECT_EQ(skip_eval.promoted_min_layer, 0);

  // Missing/out-of-range layer usage entries default conservatively to 1.0f.
  const PromotedLayerEval missing_eval
      = evaluateReroutePromotion(resizer_, target_net, {});
  EXPECT_EQ(missing_eval.promoted_min_layer, 0);

  // Mixed-layer net (75% M6 + 25% M3, so avg_curr_layer == 5 == max_layer - 1)
  // must still evaluate promotion onto M5/M6 rather than getting locked out.
  grt::GRoute& route = grt_.getRoutes()[target_net];
  const grt::GRoute orig_route = route;
  route.clear();
  route.emplace_back(0, 0, 6, 30000, 0, 6);
  route.emplace_back(0, 0, 3, 0, 10000, 3);
  const PromotedLayerEval mixed_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(mixed_eval.promoted_min_layer, 5);
  EXPECT_GE(mixed_eval.reduction_ratio, 0.10f);
  route = orig_route;

  // Multi-track NDR nets are skipped by promotion evaluation.
  odb::dbTechNonDefaultRule* ndr
      = odb::dbTechNonDefaultRule::create(block_, "test_ndr");
  ASSERT_NE(ndr, nullptr);
  target_net->setNonDefaultRule(ndr);
  const PromotedLayerEval ndr_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(ndr_eval.promoted_min_layer, 0);
}

// Verifies that evaluateReroutePromotion accepts realistic 10%-50% total
// wire+via resistance reductions (which fall below the legacy 50% wire-only
// heuristic) and rejects reductions below 10%.
TEST_F(ReroutePromotionTest,
       EvaluateReroutePromotionUsesTotalResistanceThreshold)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  odb::dbTech* tech = block_->getTech();
  sta::Scene* scene = sta_->scenes().front();
  const std::vector<float> uncongested
      = {0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.2f, 0.2f};

  // 30% lower wire R on M5/M6 -> ~25-30% total wire+via R reduction.
  for (int l = 5; l <= 6; ++l) {
    ep_.setLayerRC(tech->findRoutingLayer(l), scene, 7.0e7, 1.0e-10);
  }
  const PromotedLayerEval moderate_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(moderate_eval.promoted_min_layer, 5);
  EXPECT_GE(moderate_eval.reduction_ratio, 0.10f);
  EXPECT_LT(moderate_eval.reduction_ratio, 0.50f);

  // Only 5% lower wire R on M5/M6 -> below the 10% promotion threshold.
  for (int l = 5; l <= 6; ++l) {
    ep_.setLayerRC(tech->findRoutingLayer(l), scene, 9.5e7, 1.0e-10);
  }
  const PromotedLayerEval tiny_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(tiny_eval.promoted_min_layer, 0);
}

// Verifies that promoting a net onto fully congested upper layers falls back
// cleanly via fixEdgeAssignment() without crashing or causing overflow, and
// that rolling back via undoEco restores guides, routes, promotion state, and
// STA timing cleanly.
TEST_F(ReroutePromotionTest, CongestionFallbackAndRollback)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/1.0f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  const sta::Slack wns_before = worstSetupSlack();
  const grt::GRoute orig_route = grt_.getRoutes().at(target_net);

  {
    est::IncrementalParasiticsGuard guard(&ep_);
    odb::dbDatabase::beginEco(block_);

    grt_.setNetPromotedMinLayer(target_net, 5);
    grt_.setResistanceAware(true);
    grt_.addDirtyNet(target_net);
    grt_.setNetIsResAware(target_net, true);
    ep_.parasiticsInvalid(target_net);
    ep_.updateParasitics();

    // M5/M6 have zero capacity, so FastRoute must fall back to M2..M4 cleanly.
    ASSERT_TRUE(grt_.getRoutes().contains(target_net));
    EXPECT_FALSE(grt_.getRoutes().at(target_net).empty());
    EXPECT_FALSE(hasWireOnLayerAtLeast(grt_.getRoutes().at(target_net), 5));
    EXPECT_FALSE(grt_.hasCongestion());

    // Undo restores guides, clears promotion, and refreshes STA parasitics.
    odb::dbDatabase::undoEco(block_);
    EXPECT_EQ(grt_.getNetPromotedMinLayer(target_net), 0);
    EXPECT_TRUE(ep_.hasParasiticsInvalid());
    ep_.updateParasitics();
    sta_->updateTiming(false);
    EXPECT_FALSE(ep_.hasParasiticsInvalid());
    EXPECT_NEAR(worstSetupSlack(), wns_before, 1e-12f);
    // Restored routes are split per guide, so compare wirelength and avg layer.
    const grt::GRoute& restored = grt_.getRoutes().at(target_net);
    EXPECT_EQ(totalWireLength(restored), totalWireLength(orig_route));
    EXPECT_NEAR(avgWireLayer(restored), avgWireLayer(orig_route), 1e-6);
  }
}

// Verifies that getLayerUsageRatios() falls back to dbGCellGrid when the
// router engine is not initialized (resumed-ODB flows), preserving blocked
// upper layers as usage 1.0f instead of returning fake 0.0f.
TEST_F(ReroutePromotionTest, UninitializedUsageFallsBackToGCellGrid)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/1.0f);

  const std::vector<float> init_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(init_ratios.size(), 7u);
  EXPECT_FLOAT_EQ(init_ratios[5], 1.0f);
  EXPECT_FLOAT_EQ(init_ratios[6], 1.0f);
  EXPECT_LT(init_ratios[2], 0.70f);

  // Simulate resumed ODB: router engine cleared, dbGCellGrid preserved in ODB.
  grt_.clear();
  ASSERT_FALSE(grt_.isInitialized());

  const std::vector<float> odb_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(odb_ratios.size(), 7u);
  EXPECT_GT(odb_ratios[5], 0.70f);
  EXPECT_GT(odb_ratios[6], 0.70f);
  EXPECT_LT(odb_ratios[2], 0.70f);

  // A single pre-routed net (e.g. CTS clock or fixed wire) must NOT disable
  // dbGCellGrid fallback before loadGuidesFromDB() (empty routes_) OR
  // FastRoute edge-usage reporting after loadGuidesFromDB().
  odb::dbNet* routed_net = nullptr;
  for (odb::dbNet* db_net : block_->getNets()) {
    if (!db_net->isSpecial() && !db_net->getGuides().empty()) {
      routed_net = db_net;
      break;
    }
  }
  ASSERT_NE(routed_net, nullptr);
  odb::dbWire* wire = odb::dbWire::create(routed_net);
  ASSERT_NE(wire, nullptr);
  routed_net->setWireType(odb::dbWireType::ROUTED);

  const std::vector<float> pre_load_single_drt = grt_.getLayerUsageRatios();
  ASSERT_GE(pre_load_single_drt.size(), 7u);
  EXPECT_LT(pre_load_single_drt[2], 0.70f);
  EXPECT_GT(pre_load_single_drt[5], 0.70f);

  grt_.loadGuidesFromDB();
  const std::vector<float> single_drt_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(single_drt_ratios.size(), 7u);
  EXPECT_LT(single_drt_ratios[2], 0.70f);
  EXPECT_FLOAT_EQ(single_drt_ratios[5], 1.0f);

  // Full post-DRT designs (detailed-routed nets dominate) must fall back
  // conservatively to 1.0f instead of reporting 0.0f edge usage.
  for (const auto& [db_net, route] : grt_.getRoutes()) {
    if (!db_net->isSpecial() && db_net->getWire() == nullptr) {
      odb::dbWire::create(db_net);
      db_net->setWireType(odb::dbWireType::ROUTED);
    }
  }
  const std::vector<float> drt_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(drt_ratios.size(), 7u);
  for (int l = 2; l <= 6; ++l) {
    EXPECT_FLOAT_EQ(drt_ratios[l], 1.0f);
  }
}

// Verifies that partial layer adjustments (e.g. 65% on upper layers) do not
// falsely inflate usage above 70% in either the read-only dbGCellGrid fallback
// (without mutating unpopulated layers like M1) or resumed-ODB flows where
// guides are loaded into routes_ before the engine is initialized.
TEST_F(ReroutePromotionTest, PartialLayerAdjustmentDoesNotStarveResumedOdbUsage)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.65f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  const std::vector<float> init_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(init_ratios.size(), 7u);
  EXPECT_LT(init_ratios[5], 0.70f);
  EXPECT_LT(init_ratios[6], 0.70f);

  // Clear router state: !initialized_ && routes_.empty(), dbGCellGrid intact.
  grt_.clear();
  ASSERT_FALSE(grt_.isInitialized());
  ASSERT_TRUE(grt_.getRoutes().empty());

  odb::dbGCellGrid* gcell_grid = block_->getGCellGrid();
  ASSERT_NE(gcell_grid, nullptr);
  odb::dbTechLayer* m1 = block_->getTech()->findRoutingLayer(1);
  ASSERT_NE(m1, nullptr);
  const int m1_rows_before = gcell_grid->getLayerCongestionMap(m1).numRows();

  const std::vector<float> odb_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(odb_ratios.size(), 7u);
  EXPECT_EQ(gcell_grid->getLayerCongestionMap(m1).numRows(), m1_rows_before);
  EXPECT_LT(odb_ratios[5], 0.70f);
  EXPECT_LT(odb_ratios[6], 0.70f);

  // Resumed-ODB flow: guides loaded into routes_, engine not yet initialized.
  // getLayerUsageRatios() must stay read-only (does not initialize the engine)
  // while reading FastRoute's 3D edge grids populated by loadGuidesFromDB().
  grt_.loadGuidesFromDB();
  ASSERT_FALSE(grt_.isInitialized());
  ASSERT_FALSE(grt_.getRoutes().empty());

  const std::vector<float> resumed_ratios = grt_.getLayerUsageRatios();
  EXPECT_FALSE(grt_.isInitialized());
  ASSERT_GE(resumed_ratios.size(), 7u);
  EXPECT_LT(resumed_ratios[5], 0.70f);
  EXPECT_LT(resumed_ratios[6], 0.70f);

  const PromotedLayerEval eval
      = evaluateReroutePromotion(resizer_, target_net, resumed_ratios);
  EXPECT_EQ(eval.promoted_min_layer, 5);
}

// Verifies that on a resumed-ODB design with heavy routing obstructions
// (blockages covering 70% of upper layers plus 50% layer adjustment),
// getLayerUsageRatios() stays read-only and reads FastRoute's post-obstruction
// 3D edges from loadGuidesFromDB() instead of inflating empty obstructed layers
// above 70% via lossy dbGCellGrid inversion.
TEST_F(ReroutePromotionTest, ResumedOdbUsageHandlesRoutingObstructionsReadOnly)
{
  loadAndRouteGcd(/*use_cugr=*/false,
                  /*upper_layer_adjustment=*/0.50f,
                  /*obstruct_upper_layers=*/true);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  const std::vector<float> live_ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(live_ratios.size(), 7u);
  EXPECT_LT(live_ratios[5], 0.70f);
  EXPECT_LT(live_ratios[6], 0.70f);

  // Simulate resumed ODB: clear engine, load DB guides (populates 3D edges
  // with obstructions and adjustments without initializing the full router).
  grt_.clear();
  grt_.loadGuidesFromDB();
  ASSERT_FALSE(grt_.isInitialized());

  const std::vector<float> resumed_ratios = grt_.getLayerUsageRatios();
  EXPECT_FALSE(grt_.isInitialized());
  ASSERT_GE(resumed_ratios.size(), 7u);
  EXPECT_LT(resumed_ratios[5], 0.70f);
  EXPECT_LT(resumed_ratios[6], 0.70f);

  const PromotedLayerEval eval
      = evaluateReroutePromotion(resizer_, target_net, resumed_ratios);
  EXPECT_EQ(eval.promoted_min_layer, 5);
}

// Verifies that evaluateReroutePromotion uses the explicitly provided active
// corner's layerRC table rather than always querying sta()->cmdScene().
TEST_F(ReroutePromotionTest, EvaluateReroutePromotionUsesProvidedCornerRC)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  sta_->makeScene("fast", "default", {}, {}, "", "");
  sta_->makeScene("slow", "default", {}, {}, "", "");
  sta::Scene* fast_scene = sta_->findScene("fast");
  sta::Scene* slow_scene = sta_->findScene("slow");
  ASSERT_NE(fast_scene, nullptr);
  ASSERT_NE(slow_scene, nullptr);
  ASSERT_NE(fast_scene, slow_scene);

  sta::Scene* cmd_scene = sta_->cmdScene();
  sta::Scene* alt_scene = (cmd_scene == fast_scene) ? slow_scene : fast_scene;
  ASSERT_NE(alt_scene, cmd_scene);

  // On cmd_scene, all layers have equal resistance -> no reduction, no promote.
  // On alt_scene, M5/M6 are 10x lower resistance -> promotes to M5.
  odb::dbTech* tech = block_->getTech();
  for (int l = 1; l <= 6; ++l) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(l);
    ASSERT_NE(layer, nullptr);
    const double alt_res = (l <= 4) ? 1.0e8 : 1.0e7;
    ep_.setLayerRC(layer, cmd_scene, 1.0e8, 1.0e-10);
    ep_.setLayerRC(layer, alt_scene, alt_res, 1.0e-10);
    if (odb::dbTechLayer* cut = layer->getUpperLayer()) {
      ep_.setLayerRC(cut, cmd_scene, 1.0, 0.0);
      ep_.setLayerRC(cut, alt_scene, 1.0, 0.0);
    }
  }

  const std::vector<float> uncongested
      = {0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.2f, 0.2f};
  const PromotedLayerEval default_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested);
  EXPECT_EQ(default_eval.promoted_min_layer, 0);

  const PromotedLayerEval alt_eval
      = evaluateReroutePromotion(resizer_, target_net, uncongested, alt_scene);
  EXPECT_EQ(alt_eval.promoted_min_layer, 5);
}

// Verifies that net topology edits (disconnect, connect, merge) clear
// promoted_min_layers_ so split/shortened stubs are not forced onto upper
// metal, while master swaps (e.g. VT/sizing swaps) preserve promotion.
TEST_F(ReroutePromotionTest, TopologyChangeClearsPromotedMinLayer)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  odb::dbNet* target_net = findLongLowerLayerSignalNet();
  ASSERT_NE(target_net, nullptr);

  odb::dbITerm* load_iterm = nullptr;
  odb::dbInst* swappable_inst = nullptr;
  odb::dbMaster* alt_master = nullptr;
  for (odb::dbITerm* iterm : target_net->getITerms()) {
    if (load_iterm == nullptr && iterm->getIoType() == odb::dbIoType::INPUT) {
      load_iterm = iterm;
    }
    if (swappable_inst == nullptr) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* orig = inst->getMaster();
      const std::string name = orig->getName();
      if (const size_t pos = name.rfind("_X1");
          pos != std::string::npos && pos + 3 == name.size()) {
        if (odb::dbMaster* m
            = orig->getLib()->findMaster((name.substr(0, pos) + "_X2").c_str());
            m != nullptr && m != orig) {
          swappable_inst = inst;
          alt_master = m;
        }
      }
    }
  }
  ASSERT_NE(load_iterm, nullptr);
  ASSERT_NE(swappable_inst, nullptr);
  ASSERT_NE(alt_master, nullptr);

  est::IncrementalParasiticsGuard guard(&ep_);

  // Master swap preserves promotion (e.g. CRIT_VT_SWAP does not demote).
  grt_.setNetPromotedMinLayer(target_net, 5);
  ASSERT_EQ(grt_.getNetPromotedMinLayer(target_net), 5);
  swappable_inst->swapMaster(alt_master);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(target_net), 5);

  // Disconnecting a load ITerm clears promotion.
  load_iterm->disconnect();
  EXPECT_EQ(grt_.getNetPromotedMinLayer(target_net), 0);

  // Reconnecting an ITerm clears promotion.
  grt_.setNetPromotedMinLayer(target_net, 5);
  ASSERT_EQ(grt_.getNetPromotedMinLayer(target_net), 5);
  load_iterm->connect(target_net);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(target_net), 0);

  // Merging nets clears promotion on both preserved and removed nets.
  // Keep another live net promoted so promoted_min_layers_ stays non-empty and
  // any dangling entry for the destroyed tmp_net would be caught by lookup.
  odb::dbNet* other_net = nullptr;
  for (const auto& [db_net, route] : grt_.getRoutes()) {
    if (db_net != target_net && !db_net->isSpecial()
        && !db_net->getSigType().isSupply()
        && db_net->getSigType() != odb::dbSigType::CLOCK && !route.empty()) {
      other_net = db_net;
      break;
    }
  }
  ASSERT_NE(other_net, nullptr);
  odb::dbNet* tmp_net = odb::dbNet::create(block_, "tmp_merge_net");
  ASSERT_NE(tmp_net, nullptr);
  grt_.setNetPromotedMinLayer(other_net, 5);
  grt_.setNetPromotedMinLayer(target_net, 5);
  grt_.setNetPromotedMinLayer(tmp_net, 5);
  target_net->mergeNet(tmp_net);
  EXPECT_EQ(block_->findNet("tmp_merge_net"), nullptr);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(target_net), 0);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(other_net), 5);
}

// Verifies that when an open journal containing speculative reroute passes
// 1..N is rolled back, pass 1 stays blacklisted in attempted_nets_ to avoid
// fanout-cone thrashing while speculative passes 2..N are un-blacklisted so
// later endpoints can still attempt them, whereas committed nets stay
// blacklisted.
TEST_F(ReroutePromotionTest, RollbackUnblacklistsSpeculativeRerouteNets)
{
  loadAndRouteGcd(/*use_cugr=*/false, /*upper_layer_adjustment=*/0.0f);

  std::vector<odb::dbNet*> candidates;
  const std::vector<float> ratios = grt_.getLayerUsageRatios();
  for (const auto& [db_net, route] : grt_.getRoutes()) {
    if (db_net->isSpecial() || db_net->getSigType().isSupply()
        || db_net->getSigType() == odb::dbSigType::CLOCK
        || grt_.isNonLeafClock(db_net) || route.empty()) {
      continue;
    }
    if (db_net->getFirstOutput() == nullptr) {
      continue;
    }
    if (evaluateReroutePromotion(resizer_, db_net, ratios).promoted_min_layer
        > 0) {
      candidates.push_back(db_net);
      if (candidates.size() == 4) {
        break;
      }
    }
  }
  ASSERT_EQ(candidates.size(), 4u);

  sta::Slack worst = 0.0f;
  sta::Vertex* worst_v = nullptr;
  sta_->worstSlack(sta::MinMax::max(), worst, worst_v);
  ASSERT_NE(worst_v, nullptr);
  const sta::Path* worst_path
      = sta_->vertexWorstSlackPath(worst_v, sta::MinMax::max());
  ASSERT_NE(worst_path, nullptr);

  auto makeTarget = [&](odb::dbNet* db_net) {
    Target target;
    target.views = kPathDriverView;
    target.driver_pin = db_network_->dbToSta(db_net->getFirstOutput());
    target.endpoint_path = worst_path;
    target.driver_path = worst_path;
    target.path_index = 1;
    target.scene = sta_->cmdScene();
    return target;
  };

  const Target target1 = makeTarget(candidates[0]);
  const Target target2 = makeTarget(candidates[1]);
  const Target target3 = makeTarget(candidates[2]);
  const Target target4 = makeTarget(candidates[3]);

  MoveCommitter committer(resizer_);
  committer.init();
  OptimizerRunConfig run_config;
  OptimizationPolicyConfig policy_config;
  GeneratorContext context{.resizer = resizer_,
                           .committer = committer,
                           .run_config = run_config,
                           .policy_config = policy_config};
  RerouteGenerator generator(context);

  est::IncrementalParasiticsGuard guard(&ep_);

  // 1. Pass 1 (net1) followed by speculative pass 2 (net2) in an open journal,
  // with updateParasitics() writing guides before restoreJournal().
  committer.beginJournal();
  std::vector<std::unique_ptr<MoveCandidate>> cands1
      = generator.generate(target1);
  ASSERT_EQ(cands1.size(), 1u);
  EXPECT_TRUE(committer.commit(*cands1[0]).accepted);
  ep_.updateParasitics();

  committer.markSpeculativePass();
  std::vector<std::unique_ptr<MoveCandidate>> cands2
      = generator.generate(target2);
  ASSERT_EQ(cands2.size(), 1u);
  EXPECT_TRUE(committer.commit(*cands2[0]).accepted);
  ep_.updateParasitics();
  EXPECT_EQ(committer.pendingMoves(MoveType::kReroute), 2);

  // Restore journal: guides and promotions roll back; pass 1 stays blacklisted,
  // speculative pass 2 is released.
  committer.restoreJournal();
  EXPECT_EQ(committer.pendingMoves(MoveType::kReroute), 0);
  EXPECT_EQ(committer.committedMoves(MoveType::kReroute), 0);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[0]), 0);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[1]), 0);
  EXPECT_TRUE(generator.generate(target1).empty());

  // 2. Mixed-sequence pre-updateParasitics() rollback with an actual ODB edit:
  // pass 1 moves net2's driver instance (ODB edit in journal), then speculative
  // pass 2 requests reroute on net2 without running updateParasitics(). On
  // restoreJournal(), undoEco reverts the move (marking net2 dirty in GRT),
  // cancelNetReroute() clears net2's pending promotion without dropping GRT's
  // ODB-driven dirty mark, and net2 is released from attempted_nets_.
  odb::dbInst* drvr_inst2 = candidates[1]->getFirstOutput()->getInst();
  ASSERT_NE(drvr_inst2, nullptr);
  const odb::Point orig_loc2 = drvr_inst2->getLocation();
  committer.beginJournal();
  drvr_inst2->setLocation(orig_loc2.x() + 1000, orig_loc2.y());
  committer.markSpeculativePass();
  std::vector<std::unique_ptr<MoveCandidate>> mixed_cands2
      = generator.generate(target2);
  ASSERT_EQ(mixed_cands2.size(), 1u);
  EXPECT_TRUE(committer.commit(*mixed_cands2[0]).accepted);
  EXPECT_GT(grt_.getNetPromotedMinLayer(candidates[1]), 0);
  committer.restoreJournal();
  EXPECT_EQ(drvr_inst2->getLocation(), orig_loc2);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[1]), 0);
  EXPECT_TRUE(grt_.getDirtyNets().empty());

  // Direct invariant check: cancelNetReroute() never drops an ODB-driven
  // dirty mark on the same net.
  grt_.addDirtyNet(candidates[1]);
  grt_.setNetPromotedMinLayer(candidates[1], 5);
  grt_.requestNetReroute(candidates[1]);
  grt_.cancelNetReroute(candidates[1]);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[1]), 0);
  EXPECT_TRUE(grt_.getDirtyNets().contains(candidates[1]));
  ep_.updateParasitics();
  EXPECT_TRUE(grt_.getDirtyNets().empty());

  std::vector<std::unique_ptr<MoveCandidate>> retry2
      = generator.generate(target2);
  ASSERT_EQ(retry2.size(), 1u);

  // 3. Multi-repair pass 1 (max_repairs_per_pass > 1): both pass-1 reroutes
  // roll back promotion cleanly and stay blacklisted after rollback.
  committer.beginJournal();
  std::vector<std::unique_ptr<MoveCandidate>> cands3
      = generator.generate(target3);
  ASSERT_EQ(cands3.size(), 1u);
  EXPECT_TRUE(committer.commit(*cands3[0]).accepted);
  std::vector<std::unique_ptr<MoveCandidate>> cands4
      = generator.generate(target4);
  ASSERT_EQ(cands4.size(), 1u);
  EXPECT_TRUE(committer.commit(*cands4[0]).accepted);
  ep_.updateParasitics();
  committer.restoreJournal();
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[2]), 0);
  EXPECT_EQ(grt_.getNetPromotedMinLayer(candidates[3]), 0);
  EXPECT_TRUE(generator.generate(target3).empty());
  EXPECT_TRUE(generator.generate(target4).empty());

  // 4. Committing net2 keeps it blacklisted on subsequent generate() calls and
  // does not permanently latch grt::Net::is_res_aware_.
  committer.beginJournal();
  EXPECT_TRUE(committer.commit(*retry2[0]).accepted);
  ep_.updateParasitics();
  committer.commitJournal();
  EXPECT_EQ(committer.committedMoves(MoveType::kReroute), 1);
  EXPECT_FALSE(grt_.isNetResAware(candidates[1]));
  EXPECT_TRUE(generator.generate(target2).empty());
}

// Verifies CUGR compatibility: getLayerUsageRatios() works under CUGR and
// RerouteGenerator cleanly skips CUGR without churning routes or crashing.
TEST_F(ReroutePromotionTest, CugrHandlesPromotionApiAndReroutePhase)
{
  loadAndRouteGcd(/*use_cugr=*/true, /*upper_layer_adjustment=*/0.0f);

  const std::vector<float> ratios = grt_.getLayerUsageRatios();
  ASSERT_GE(ratios.size(), 7u);
  for (int l = 2; l <= 6; ++l) {
    EXPECT_GE(ratios[l], 0.0f);
  }

  const sta::Slack wns_before = worstSetupSlack();
  resizer_.repairSetup(/*setup_margin=*/0.0,
                       /*repair_tns_end_percent=*/0.1,
                       /*max_passes=*/5,
                       /*max_iterations=*/-1,
                       /*max_repairs_per_pass=*/1,
                       /*match_cell_footprint=*/false,
                       /*verbose=*/false,
                       /*sequence=*/{},
                       /*phases=*/"REROUTE",
                       /*path_group=*/"",
                       /*skip_pin_swap=*/true,
                       /*skip_gate_cloning=*/true,
                       /*skip_size_down_fanout=*/true,
                       /*skip_buffering=*/true,
                       /*skip_buffer_removal=*/true,
                       /*skip_buffer_to_inverters=*/true,
                       /*skip_last_gasp=*/true,
                       /*skip_vt_swap=*/true,
                       /*skip_crit_vt_swap=*/true);

  EXPECT_FLOAT_EQ(worstSetupSlack(), wns_before);
  for (const auto& [db_net, route] : grt_.getRoutes()) {
    EXPECT_EQ(grt_.getNetPromotedMinLayer(db_net), 0);
  }
}

}  // namespace
}  // namespace rsz
