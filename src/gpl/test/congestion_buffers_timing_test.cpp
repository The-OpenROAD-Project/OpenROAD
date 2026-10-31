// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "est/EstimateParasitics.h"
#include "gpl/Replace.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/gpl/src/graphicsNone.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/nesterovPlace.h"
#include "src/gpl/src/placerBase.h"
#include "src/gpl/src/routeBase.h"
#include "src/gpl/src/timingBase.h"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "tst/IntegratedFixture.h"

namespace gpl {

// Friend access only sets the iteration's overflow and invokes the production
// boundary. TimingBase, resizer, callbacks and updateGCellState all run for
// real.
class TimingRepairIntegrationTest : public tst::IntegratedFixture
{
 protected:
  TimingRepairIntegrationTest() : IntegratedFixture(Technology::kNangate45, "")
  {
  }

  void SetUp() override
  {
    block_ = readDef(getFilePath("_main/src/gpl/test/simple01-td-repair.def"),
                     {lib_});
    ASSERT_NE(block_, nullptr);
    int index = 0;
    const auto core = block_->getCoreArea();
    for (auto* inst : block_->getInsts()) {
      const int x = core.xMin() + 2000 + (index % 12) * 2000;
      const int y = core.yMin() + 2000 + (index / 12) * 2000;
      inst->setLocation(x, y);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      ++index;
    }
    makeFanoutNet();
    configureDesign();
    sta_->postReadDb(db_.get());
    sta_->postReadDef(block_);
    ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e5, 2.0e-10);
    ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e5, 2.0e-10);
    initStaDefaultSdc();
    auto* top = db_network_->cell(db_network_->topInstance());
    sta_->setFanoutLimit(top, sta::MinMax::max(), 6.0f, sta_->cmdMode()->sdc());
    resizer_.resizeSlackPreamble();
    for (auto* inst : block_->getInsts()) {
      initial_names_.insert(inst->getName());
    }
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    grt_.getRudy()->setWireWidth(1000000);
    // Reserve filler area for an ordinary pass followed by a real timing
    // repair; inflation may not raise density to fund its added area.
    options_.density = 0.78f;
    options_.timingDrivenMode = true;
    options_.routabilityDrivenMode = true;
    options_.keepResizeBelowOverflow = 0.3f;
  }

  virtual void configureDesign() {}

  void makeFanoutNet()
  {
    auto* input = block_->findBTerm("req_val")->getNet();
    auto* fanout = odb::dbNet::create(block_, "forced_fanout");
    auto* driver = odb::dbInst::create(
        block_, lib_->findMaster("BUF_X1"), "fanout_driver");
    driver->setLocation(10000, 10000);
    driver->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    driver->findITerm("A")->connect(input);
    driver->findITerm("Z")->connect(fanout);
    auto* layer = db_->getTech()->findRoutingLayer(3);
    for (int i = 0; i < 64; ++i) {
      const std::string suffix = std::to_string(i);
      auto* sink = odb::dbInst::create(
          block_, lib_->findMaster("NAND2_X1"), ("sink_" + suffix).c_str());
      const int x = 16000 + (i % 8) * 400;
      const int y = 16000 + (i / 8) * 400;
      sink->setLocation(x, y);
      sink->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      sink->findITerm("A1")->connect(fanout);
      sink->findITerm("A2")->connect(input);
      auto* output = odb::dbNet::create(block_, ("output_" + suffix).c_str());
      sink->findITerm("ZN")->connect(output);
      auto* term = odb::dbBTerm::create(output, ("output_" + suffix).c_str());
      term->setIoType(odb::dbIoType::OUTPUT);
      auto* pin = odb::dbBPin::create(term);
      pin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      odb::dbBox::create(pin, layer, x, y, x + 100, y + 100);
    }
    ASSERT_EQ(fanout->getITermCount(), 65);
  }

  void makePlacer(bool routability = true,
                  bool timing = true,
                  const std::vector<odb::dbGroup*>& groups = {nullptr},
                  bool check_density = false)
  {
    options_.routabilityDrivenMode = routability;
    options_.timingDrivenMode = timing;
    pbc_ = std::make_shared<PlacerBaseCommon>(
        db_.get(), PlacerBaseVars(options_), &logger_);
    for (auto* group : groups) {
      pb_vec_.push_back(std::make_shared<PlacerBase>(
          db_.get(), pbc_, &logger_, check_density, group));
    }
    const NesterovBaseVars nb_vars(options_);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, 1, Clusters{});
    for (const auto& pb : pb_vec_) {
      nb_vec_.push_back(
          std::make_shared<NesterovBase>(nb_vars, pb, nbc_, &logger_));
    }
    rb_ = std::make_shared<RouteBase>(
        RouteBaseVars(options_), db_.get(), &grt_, nbc_, nb_vec_, &logger_);
    tb_ = std::make_shared<TimingBase>(nbc_, &grt_, &resizer_, &logger_);
    tb_->setTimingNetWeightOverflows({100});
    tb_->setTimingNetsPercentage(100);
    np_ = std::make_unique<NesterovPlace>(
        NesterovPlaceVars(options_, nbc_->getHpwl()),
        pbc_,
        nbc_,
        pb_vec_,
        nb_vec_,
        rb_,
        tb_,
        nullptr,
        nullptr,
        std::make_unique<GraphicsNone>(),
        &logger_);
  }

  bool routabilityNeeded() const { return np_->is_routability_need_; }

  int64_t inflateCellForTest(GCell* cell,
                             float ratio,
                             std::optional<int64_t> area_limit = std::nullopt)
  {
    return RouteBase::inflateCell(cell, ratio, area_limit);
  }

  std::pair<int, int> retainedInsertionCounts() const
  {
    int inserted = 0;
    int inflated = 0;
    for (const auto& nb : nb_vec_) {
      for (const auto& cell : nb->getGCells()) {
        if (cell->isStdInstance()
            && !initial_names_.contains(
                cell->insts().front()->dbInst()->getName())) {
          ++inserted;
          inflated += cell->changeType() == GCell::GCellChange::kRoutability;
        }
      }
    }
    return {inserted, inflated};
  }

  GCell* liveCellFor(odb::dbInst* inst) const
  {
    // dbToNb() resolves through the construction-time PlacerBase map, which
    // does not contain resizer-created instances. Handles resolve live storage
    // after callback creation, deletion, reindexing and vector growth.
    for (const auto& nb : nb_vec_) {
      for (const auto& cell : nb->getGCells()) {
        if (cell->isInstance() && cell->contains(inst)) {
          return nbc_->getGCellByIndex(cell.getStorageIndex());
        }
      }
    }
    return nullptr;
  }

  int64_t liveInflationArea() const
  {
    int64_t area = 0;
    for (const auto& nb : nb_vec_) {
      area += nb->getInflationArea();
    }
    return area;
  }

  void checkDensityAccounting()
  {
    for (const auto& nb : nb_vec_) {
      int64_t standard_area = 0;
      for (const auto& cell : nb->getGCells()) {
        if (cell->isStdInstance()) {
          standard_area += static_cast<int64_t>(cell->dx()) * cell->dy();
        }
      }
      const int64_t available
          = nb->getWhiteSpaceArea() - nb->getPb()->macroInstsArea();
      const float expected
          = static_cast<float>(standard_area + nb->getTotalFillerArea())
            / static_cast<float>(available);
      EXPECT_FLOAT_EQ(nb->getTargetDensity(), expected);
      for (const auto& bin : nb->getBins()) {
        EXPECT_FLOAT_EQ(bin.getTargetDensity(), expected);
      }
      EXPECT_EQ(nb->getMovableArea(),
                nb->getNesterovInstsArea() + nb->getTotalFillerArea());
    }
  }

  void runOrdinaryPass(int wire_width, float overflow = 0.2f)
  {
    // Supply settled motion to the real controller, then execute its ordinary
    // congestion checks. Completion is not set by hand in this fixture.
    grt_.getRudy()->setWireWidth(wire_width);
    np_->average_overflow_unscaled_ = overflow;
    for (const auto& nb : nb_vec_) {
      nb->peak_coordi_distance_ = 1.0f;
      nb->coordiDistance_ = 0.0f;
    }
    float current_a = 1.0f;
    const bool capture_snapshot
        = !np_->is_routability_snapshot_saved_
          && np_->npVars_.routability_driven_mode
          && overflow <= np_->npVars_.routability_snapshot_overflow;
    const float current_wl_x = np_->wireLengthCoefX_;
    const float current_wl_y = np_->wireLengthCoefY_;
    np_->routabilitySnapshot(0, current_a, "", ordinary_iterations_, 0);
    if (capture_snapshot) {
      ASSERT_TRUE(np_->is_routability_snapshot_saved_);
      EXPECT_FLOAT_EQ(np_->route_snapshot_wl_coef_x_, current_wl_x);
      EXPECT_FLOAT_EQ(np_->route_snapshot_wl_coef_y_, current_wl_y);
      EXPECT_FLOAT_EQ(np_->route_snapshot_a_, current_a);
      ASSERT_TRUE(std::isfinite(current_wl_x));
      ASSERT_TRUE(std::isfinite(current_wl_y));
      ASSERT_GT(current_wl_x, 0.0f);
      ASSERT_GT(current_wl_y, 0.0f);
    }
    ASSERT_TRUE(np_->isPlacementSettled());
    const int previous_iterations = ordinary_iterations_;
    np_->runRoutability(0, 0, "", ordinary_iterations_, current_a);
    if (ordinary_iterations_ > previous_iterations
        && (routabilityNeeded()
            || rb_->getRudyAverage() >= rb_->rbVars_.targetRC)) {
      ASSERT_TRUE(np_->is_routability_snapshot_saved_);
      EXPECT_FLOAT_EQ(np_->wireLengthCoefX_, np_->route_snapshot_wl_coef_x_);
      EXPECT_FLOAT_EQ(np_->wireLengthCoefY_, np_->route_snapshot_wl_coef_y_);
      EXPECT_FLOAT_EQ(current_a, np_->route_snapshot_a_);
      EXPECT_GT(current_a, 0.0f);
    }
  }

  void completeRoutability(bool inflate_ordinary_cells = false)
  {
    if (inflate_ordinary_cells) {
      runOrdinaryPass(1000000);
      ASSERT_GT(rb_->getTotalInflation(), 0);
      ASSERT_TRUE(np_->is_routability_need_);
    }
    runOrdinaryPass(1);
    ASSERT_FALSE(np_->is_routability_need_);
    ASSERT_GT(ordinary_iterations_, 0);
    grt_.getRudy()->setWireWidth(1000000);
  }

  odb::dbInst* inflatedNewBuffer()
  {
    for (const auto& cell : nb_vec_[0]->getGCells()) {
      if (cell->isStdInstance() && cell->getInflationArea() > 0
          && !initial_names_.contains(
              cell->insts().front()->dbInst()->getName())) {
        return cell->insts().front()->dbInst();
      }
    }
    return nullptr;
  }

  void runTiming(float overflow, bool routability_iteration = false)
  {
    np_->average_overflow_unscaled_ = overflow;
    int virtual_cts = 0;
    np_->runTimingDriven(0,
                         "",
                         0,
                         timing_iterations_,
                         delta_area_,
                         routability_iteration,
                         virtual_cts);
  }

  void checkRetainedInsertions(bool expect_inflation)
  {
    int new_cells = 0;
    int inflated_cells = 0;
    for (const auto& cell : nb_vec_[0]->getGCells()) {
      if (!cell->isStdInstance()
          || initial_names_.contains(
              cell->insts().front()->dbInst()->getName())) {
        continue;
      }
      ++new_cells;
      EXPECT_GE(cell->gPins().size(), 2);
      for (const auto* pin : cell->gPins()) {
        EXPECT_NE(pin->getGNet(), nullptr);
        ASSERT_NE(pin->getPbPin()->getDbITerm(), nullptr);
        EXPECT_NE(pin->getPbPin()->getDbITerm()->getNet(), nullptr);
      }
      if (cell->changeType() == GCell::GCellChange::kRoutability) {
        ++inflated_cells;
        EXPECT_GT(static_cast<int64_t>(cell->dx()) * cell->dy(),
                  cell->insts().front()->getArea());
        const double density_area = static_cast<double>(cell->dDx())
                                    * cell->dDy() * cell->getDensityScale();
        EXPECT_NEAR(density_area,
                    static_cast<double>(cell->dx()) * cell->dy(),
                    density_area * 0.01);
      }
    }
    EXPECT_GT(new_cells, 0)
        << "the real timing repair must retain connected insertions";
    EXPECT_EQ(nb_vec_[0]->getNewInstances().size(), 0)
        << "updateGCellState must consume the new instances";
    if (expect_inflation) {
      EXPECT_GT(inflated_cells, 0)
          << "inflation must run before updateGCellState clears its candidates";
      EXPECT_GT(liveInflationArea(), 0);
    } else {
      EXPECT_EQ(inflated_cells, 0);
      EXPECT_EQ(liveInflationArea(), 0);
    }
    const auto& nb = nb_vec_[0];
    if (options_.routabilityDrivenMode && !routabilityNeeded()) {
      EXPECT_EQ(nb->getMovableArea(),
                nb->getNesterovInstsArea() + nb->getTotalFillerArea());
    } else {
      // The ordinary path retains the legacy float multiply and rounding,
      // rather than publishing the new live-area sum as its movable area.
      EXPECT_EQ(nb->getMovableArea(),
                static_cast<int64_t>(nb->getTargetDensity()
                                     * nb->getWhiteSpaceArea()));
    }
  }

  PlaceOptions options_;
  std::unordered_set<std::string> initial_names_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pb_vec_;
  std::vector<std::shared_ptr<NesterovBase>> nb_vec_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  std::unique_ptr<NesterovPlace> np_;
  int timing_iterations_ = 0;
  int ordinary_iterations_ = 0;
  int64_t delta_area_ = 0;
};

TEST_F(TimingRepairIntegrationTest,
       RetainedRepairInflatesConnectedNewBuffersBeforeStateInitialization)
{
  makePlacer();
  completeRoutability();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
}

class TimingRepairRoutabilityPhaseTest
    : public TimingRepairIntegrationTest,
      public ::testing::WithParamInterface<int>
{
};

TEST_P(TimingRepairRoutabilityPhaseTest,
       CountsRealRetainedInsertionsBeforeDuringAndAfterRoutability)
{
  const int phase = GetParam();
  if (phase == 0) {
    options_.keepResizeBelowOverflow = 0.95f;
    options_.routabilityCheckOverflow = 0.3f;
  }
  makePlacer();
  if (phase == 0) {
    runOrdinaryPass(1, 0.8f);
    ASSERT_EQ(ordinary_iterations_, 0);
    ASSERT_EQ(rb_->getTotalTilesCount(), 0);
  }
  if (phase == 1) {
    runOrdinaryPass(1000000);
    ASSERT_TRUE(routabilityNeeded());
    ASSERT_GT(rb_->getTotalInflation(), 0);
  } else if (phase == 2) {
    completeRoutability();
    ASSERT_FALSE(routabilityNeeded());
  }
  const int64_t ordinary_charge = rb_->getTotalInflation();
  runTiming(phase == 0 ? 0.8f : 0.2f);
  const auto [inserted, inflated] = retainedInsertionCounts();
  RecordProperty("routability_phase", phase);
  RecordProperty("ordinary_passes", ordinary_iterations_);
  RecordProperty("retained_new_cells", inserted);
  RecordProperty("inflated_new_cells", inflated);
  ASSERT_EQ(timing_iterations_, 1);
  ASSERT_GT(inserted, 0);
  EXPECT_EQ(rb_->getTotalInflation(), ordinary_charge);
  EXPECT_TRUE(nb_vec_[0]->getNewInstances().empty());
  if (phase == 2) {
    EXPECT_GT(inflated, 0);
    checkRetainedInsertions(true);
  } else {
    EXPECT_EQ(inflated, 0);
    EXPECT_TRUE(routabilityNeeded());
    EXPECT_EQ(ordinary_iterations_, phase);
    if (phase == 0) {
      checkRetainedInsertions(false);
      EXPECT_EQ(rb_->getTotalTilesCount(), 0);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(RoutabilityPhases,
                         TimingRepairRoutabilityPhaseTest,
                         ::testing::Values(0, 1, 2));

TEST_F(TimingRepairIntegrationTest,
       SynchronousCallbacksPrecedeInflationAndOptimizerInitialization)
{
  makePlacer();
  completeRoutability();
  np_->updateDb();
  tb_->executeTimingDriven(false, true);
  nb_vec_[0]->refreshIoPins();
  // The production executor has delivered callbacks and repaired pointers,
  // but has not yet initialized the new cells' optimizer state.
  const auto& pending = nb_vec_[0]->getNewInstances();
  ASSERT_FALSE(pending.empty());
  const std::unordered_set<odb::dbInst*> created(pending.begin(),
                                                 pending.end());
  int live_creations = 0;
  for (const auto& cell : nb_vec_[0]->getGCells()) {
    if (!cell->isStdInstance()
        || !created.contains(cell->insts().front()->dbInst())) {
      continue;
    }
    ++live_creations;
    EXPECT_GE(cell->gPins().size(), 2);
    for (const auto* pin : cell->gPins()) {
      EXPECT_NE(pin->getGNet(), nullptr);
      ASSERT_NE(pin->getPbPin()->getDbITerm(), nullptr);
      EXPECT_NE(pin->getPbPin()->getDbITerm()->getNet(), nullptr);
    }
  }
  ASSERT_GT(live_creations, 0);
  RecordProperty("queued_creations", static_cast<int>(pending.size()));
  RecordProperty("live_creations_before_state_update", live_creations);

  auto* transient = odb::dbInst::create(
      block_, lib_->findMaster("BUF_X1"), "callback_transient");
  ASSERT_NE(liveCellFor(transient), nullptr);
  EXPECT_TRUE(pending.contains(transient));
  odb::dbInst::destroy(transient);
  EXPECT_EQ(liveCellFor(transient), nullptr);
  EXPECT_FALSE(pending.contains(transient))
      << "deletion must remove the pending identity with the live cell";
  nbc_->fixPointers();
  nb_vec_[0]->refreshIoPins();

  ASSERT_GT(rb_->inflateNewCells(), 0);
  nbc_->rebuildDeviceState();
  nb_vec_[0]->updateGCellState();
  nb_vec_[0]->updateTimingDrivenDensity();
  checkRetainedInsertions(true);
  checkDensityAccounting();
  const int64_t inflated_area = liveInflationArea();
  EXPECT_TRUE(pending.empty());
  EXPECT_EQ(rb_->inflateNewCells(), 0);
  EXPECT_EQ(liveInflationArea(), inflated_area);
}

TEST_F(TimingRepairIntegrationTest,
       SharedInflationPreservesLegacyFloatRoundingAndBoundedArea)
{
  makePlacer();
  auto* cell = liveCellFor(block_->findInst("fanout_driver"));
  ASSERT_NE(cell, nullptr);
  // Float sqrt/multiply rounds the width to 1003.5; double arithmetic stays
  // just below it. The stored even widths therefore differ by two DBUs.
  constexpr float threshold = 1.0070122480392456f;
  const int legacy_width
      = 2 * (static_cast<int>(std::round(1000 * std::sqrt(threshold))) / 2);
  const int double_width
      = 2
        * (static_cast<int>(
               std::round(1000 * std::sqrt(static_cast<double>(threshold))))
           / 2);
  ASSERT_NE(legacy_width, double_width);
  for (const float ratio : {std::nextafter(threshold, 1.0f),
                            threshold,
                            std::nextafter(threshold, 2.0f),
                            1.33f}) {
    for (const auto& [width, height] :
         {std::pair{1000, 800}, std::pair{10, 10}, std::pair{101, 79}}) {
      cell->setAllLocations(0, 0, width, height);
      const int expected_width
          = 2 * (static_cast<int>(std::round(width * std::sqrt(ratio))) / 2);
      const int expected_height
          = 2 * (static_cast<int>(std::round(height * std::sqrt(ratio))) / 2);
      const int64_t old_area = static_cast<int64_t>(width) * height;
      EXPECT_EQ(
          inflateCellForTest(cell, ratio),
          static_cast<int64_t>(expected_width) * expected_height - old_area);
      EXPECT_EQ(cell->dx(), expected_width);
      EXPECT_EQ(cell->dy(), expected_height);
      for (const int64_t limit : {int64_t{0}, int64_t{1}, int64_t{3600}}) {
        cell->setAllLocations(0, 0, width, height);
        const int64_t added = inflateCellForTest(cell, ratio, limit);
        EXPECT_GE(added, 0);
        EXPECT_LE(added, limit);
        EXPECT_EQ(static_cast<int64_t>(cell->dx()) * cell->dy(),
                  old_area + added);
        EXPECT_GE(cell->dx(), width);
        EXPECT_GE(cell->dy(), height);
      }
    }
  }
}

class MacroOnlyTimingRepairIntegrationTest : public TimingRepairIntegrationTest
{
 protected:
  void configureDesign() override
  {
    // Define the master and instances before postReadDb binds LEF cells to STA.
    const auto core = block_->getCoreArea();
    const int macro_width = 6000;
    const int split_x = core.xMin() + 2 * macro_width;
    for (int i = 0; i < 2; ++i) {
      auto* region = odb::dbRegion::create(
          block_, ("repair_region_" + std::to_string(i)).c_str());
      region->setRegionType(odb::dbRegionType::EXCLUSIVE);
      odb::dbBox::create(region,
                         i == 0 ? split_x : core.xMin(),
                         core.yMin(),
                         i == 0 ? core.xMax() : split_x,
                         core.yMax());
      auto* group = odb::dbGroup::create(
          region, ("repair_domain_" + std::to_string(i)).c_str());
      group->setType(odb::dbGroupType::VOLTAGE_DOMAIN);
      groups_.push_back(group);
    }
    for (auto* inst : block_->getInsts()) {
      groups_[0]->addInst(inst);
      groups_[0]->getRegion()->addInst(inst);
    }
    macro_master_ = odb::dbMaster::create(lib_, "repair_macro");
    macro_master_->setType(odb::dbMasterType::BLOCK);
    macro_master_->setWidth(macro_width);
    macro_master_->setHeight(core.dy() / 2);
    macro_master_->setFrozen();
    for (int i = 0; i < 4; ++i) {
      auto* macro = odb::dbInst::create(
          block_, macro_master_, ("repair_macro_" + std::to_string(i)).c_str());
      // Unlike a single region-sized macro, these cells can move. An asymmetric
      // placement gives the real Nesterov initialization nonzero density
      // forces.
      macro->setLocation(core.xMin() + i * 1300, core.yMin() + 3000 + i * 6000);
      macro->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      groups_[1]->addInst(macro);
      groups_[1]->getRegion()->addInst(macro);
    }
  }

  std::vector<odb::dbGroup*> groups_;
  odb::dbMaster* macro_master_ = nullptr;
};

TEST_F(MacroOnlyTimingRepairIntegrationTest,
       RetainedRepairRefreshesUnchangedMacroOnlyRegion)
{
  ASSERT_NE(macro_master_, nullptr);
  ASSERT_NE(db_network_->dbToSta(macro_master_), nullptr);
  makePlacer(false, true, groups_, true);
  const auto& macro_region = nb_vec_[1];
  ASSERT_EQ(macro_region->getWhiteSpaceArea(),
            macro_region->getPb()->macroInstsArea());
  ASSERT_EQ(macro_region->getStdInstArea(), 0);
  ASSERT_EQ(macro_region->getTotalFillerArea(), 0);
  ASSERT_EQ(macro_region->getFillerCnt(), 0);
  ASSERT_GT(macro_region->getDensityGradSum(), 0);
  const float density = macro_region->getTargetDensity();
  ASSERT_TRUE(std::isfinite(density));
  std::vector<int64_t> before_area;
  std::vector<int64_t> before_fillers;
  for (const auto& nb : nb_vec_) {
    before_area.push_back(nb->getNesterovInstsArea());
    before_fillers.push_back(nb->getTotalFillerArea());
  }
  const int64_t before_delta = delta_area_;
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(false);
  const int64_t repair_delta = delta_area_ - before_delta;
  ASSERT_NE(repair_delta, 0);
  for (size_t i = 0; i < nb_vec_.size(); ++i) {
    const auto& nb = nb_vec_[i];
    // Only the standard-cell region changes. Legacy accounting consumes the
    // callback delta there and leaves the macro-only domain unchanged.
    const float expected
        = static_cast<float>((i == 0 ? repair_delta : 0) + before_area[i]
                             + before_fillers[i])
          / static_cast<float>(nb->getWhiteSpaceArea());
    EXPECT_EQ(nb->getTargetDensity(), expected);
    EXPECT_EQ(nb->getMovableArea(),
              static_cast<int64_t>(expected * nb->getWhiteSpaceArea()));
  }
  ASSERT_TRUE(std::isfinite(macro_region->getTargetDensity()));
  EXPECT_EQ(macro_region->getTargetDensity(), density);
  EXPECT_EQ(macro_region->getMovableArea(),
            macro_region->getNesterovInstsArea());
  for (const auto& bin : macro_region->getBins()) {
    EXPECT_EQ(bin.getTargetDensity(), density);
  }
}

class MixedMacroTimingRepairIntegrationTest : public TimingRepairIntegrationTest
{
 protected:
  void configureDesign() override
  {
    const auto core = block_->getCoreArea();
    auto* master = odb::dbMaster::create(lib_, "density_gate_macro");
    master->setType(odb::dbMasterType::BLOCK);
    master->setWidth(core.dx() / 4);
    master->setHeight(core.dy() / 3);
    master->setFrozen();
    macro_ = odb::dbInst::create(block_, master, "density_gate_macro_inst");
    macro_->setLocation(core.xMin() + core.dx() / 2,
                        core.yMin() + core.dy() / 2);
    macro_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    macro_->setDoNotTouch(true);
  }

  void expectLegacyDensityAfterRetainedRepair()
  {
    ASSERT_EQ(nb_vec_.size(), 1u);
    const auto& nb = nb_vec_.front();
    ASSERT_NE(macro_, nullptr);
    auto* macro_cell = liveCellFor(macro_);
    ASSERT_NE(macro_cell, nullptr);
    ASSERT_TRUE(macro_cell->isMacroInstance());
    ASSERT_GT(nb->getStdInstArea(), 0);
    const int64_t macro_area = nb->getPb()->macroInstsArea();
    const int64_t whitespace = nb->getWhiteSpaceArea();
    ASSERT_GT(macro_area, 0);
    ASSERT_GT(whitespace, macro_area);
    ASSERT_EQ(nbc_->getDeltaArea(), 0);
    const int64_t instance_area = nb->getNesterovInstsArea();
    const int64_t fillers = nb->getTotalFillerArea();
    const int64_t accumulated_delta = delta_area_;
    const int macro_dx = macro_cell->dx();
    const int macro_dy = macro_cell->dy();
    const int64_t macro_inflation = macro_cell->getInflationArea();
    const auto* macro_master = macro_->getMaster();
    ASSERT_TRUE(std::isfinite(nb->getTargetDensity()));
    runTiming(0.2f);
    ASSERT_EQ(timing_iterations_, 1);
    const int64_t repair_delta = delta_area_ - accumulated_delta;
    ASSERT_NE(repair_delta, 0)
        << "The real retained repair must change area to distinguish the gates";
    // Reproduce the legacy operation order and float rounding, using the
    // actual callback delta captured by runTimingDriven, not a fabricated edit.
    const float expected
        = static_cast<float>(repair_delta + instance_area + fillers)
          / static_cast<float>(whitespace);
    ASSERT_TRUE(std::isfinite(expected));
    ASSERT_TRUE(std::isfinite(nb->getTargetDensity()));
    EXPECT_EQ(nb->getTargetDensity(), expected);
    EXPECT_EQ(nb->getMovableArea(),
              static_cast<int64_t>(expected * whitespace));
    EXPECT_EQ(nb->getTotalFillerArea(), fillers);
    EXPECT_EQ(nbc_->getDeltaArea(), 0);
    EXPECT_EQ(nbc_->getNewGcellsCount(), 0);
    for (const auto& bin : nb->getBins()) {
      EXPECT_TRUE(std::isfinite(bin.getTargetDensity()));
      EXPECT_EQ(bin.getTargetDensity(), expected);
    }
    const float post_routability_density
        = static_cast<float>(nb->getStdInstArea() + fillers)
          / static_cast<float>(whitespace - macro_area);
    EXPECT_GT(std::abs(expected - post_routability_density), 1e-6f)
        << "Mixed macros must distinguish legacy accounting from the new "
           "helper";
    ASSERT_EQ(macro_->getMaster(), macro_master);
    macro_cell = liveCellFor(macro_);
    ASSERT_NE(macro_cell, nullptr);
    EXPECT_EQ(macro_cell->dx(), macro_dx);
    EXPECT_EQ(macro_cell->dy(), macro_dy);
    EXPECT_EQ(macro_cell->getInflationArea(), macro_inflation);
    EXPECT_EQ(nb->getMacroInstArea(),
              static_cast<int64_t>(macro_dx) * macro_dy);
    const double density_area = static_cast<double>(macro_cell->dDx())
                                * macro_cell->dDy()
                                * macro_cell->getDensityScale();
    EXPECT_TRUE(std::isfinite(density_area));
    EXPECT_NEAR(density_area,
                static_cast<double>(macro_dx) * macro_dy,
                static_cast<double>(macro_dx) * macro_dy * 0.01);
    int new_cells = 0;
    for (const auto& cell : nb->getGCells()) {
      if (!cell->isStdInstance()
          || initial_names_.contains(
              cell->insts().front()->dbInst()->getName())) {
        continue;
      }
      ++new_cells;
      EXPECT_NE(cell->changeType(), GCell::GCellChange::kRoutability);
      EXPECT_EQ(cell->getInflationArea(), 0);
      EXPECT_GE(cell->gPins().size(), 2);
      for (const auto* pin : cell->gPins()) {
        ASSERT_NE(pin->getPbPin()->getDbITerm(), nullptr);
        EXPECT_NE(pin->getPbPin()->getDbITerm()->getNet(), nullptr);
      }
    }
    EXPECT_GT(new_cells, 0) << "A no-op repair cannot qualify this regression";
    EXPECT_TRUE(nb->getNewInstances().empty());
  }

  odb::dbInst* macro_ = nullptr;
};

TEST_F(MixedMacroTimingRepairIntegrationTest,
       CompletedRepairWithZeroBudgetUsesLiveMacroDensity)
{
  options_.routabilityMaxInflationTotal = 0.0f;
  makePlacer();
  completeRoutability();
  ASSERT_FALSE(routabilityNeeded());
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(false);
  checkDensityAccounting();
  EXPECT_EQ(liveInflationArea(), 0);
}

TEST_F(MixedMacroTimingRepairIntegrationTest,
       BeforeFirstRoutabilityPassKeepsLegacyMacroDensity)
{
  makePlacer();
  ASSERT_TRUE(routabilityNeeded());
  ASSERT_EQ(ordinary_iterations_, 0);
  expectLegacyDensityAfterRetainedRepair();
  EXPECT_TRUE(routabilityNeeded());
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
  EXPECT_EQ(liveInflationArea(), 0);
}

TEST_F(MixedMacroTimingRepairIntegrationTest,
       ActiveRoutabilityRepairKeepsLegacyMacroDensity)
{
  makePlacer();
  runOrdinaryPass(1000000);
  ASSERT_TRUE(routabilityNeeded());
  ASSERT_GT(ordinary_iterations_, 0);
  ASSERT_GT(rb_->getTotalInflation(), 0);
  const int64_t ordinary_charge = rb_->getTotalInflation();
  expectLegacyDensityAfterRetainedRepair();
  EXPECT_TRUE(routabilityNeeded());
  EXPECT_EQ(rb_->getTotalInflation(), ordinary_charge);
}

TEST_F(MixedMacroTimingRepairIntegrationTest,
       WithoutRoutabilityKeepsLegacyMacroDensity)
{
  makePlacer(false);
  expectLegacyDensityAfterRetainedRepair();
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
  EXPECT_EQ(liveInflationArea(), 0);
}

TEST_F(MixedMacroTimingRepairIntegrationTest,
       ActiveRoutabilityIterationSkipsMacroDensityAndRepair)
{
  makePlacer();
  const auto& nb = nb_vec_.front();
  const float density = nb->getTargetDensity();
  const int64_t movable_area = nb->getMovableArea();
  auto* macro_cell = liveCellFor(macro_);
  ASSERT_NE(macro_cell, nullptr);
  const int macro_dx = macro_cell->dx();
  const int macro_dy = macro_cell->dy();
  runTiming(0.2f, true);
  EXPECT_EQ(timing_iterations_, 0);
  EXPECT_EQ(block_->getInsts().size(), initial_names_.size());
  EXPECT_TRUE(std::isfinite(nb->getTargetDensity()));
  EXPECT_FLOAT_EQ(nb->getTargetDensity(), density);
  EXPECT_EQ(nb->getMovableArea(), movable_area);
  for (const auto& bin : nb->getBins()) {
    EXPECT_FLOAT_EQ(bin.getTargetDensity(), density);
  }
  macro_cell = liveCellFor(macro_);
  ASSERT_NE(macro_cell, nullptr);
  EXPECT_EQ(macro_cell->dx(), macro_dx);
  EXPECT_EQ(macro_cell->dy(), macro_dy);
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
}

TEST_F(MixedMacroTimingRepairIntegrationTest,
       CompletedRepairUsesLiveMacroDensity)
{
  makePlacer();
  completeRoutability();
  ASSERT_FALSE(routabilityNeeded());
  auto* macro_cell = liveCellFor(macro_);
  ASSERT_NE(macro_cell, nullptr);
  const int macro_dx = macro_cell->dx();
  const int macro_dy = macro_cell->dy();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
  checkDensityAccounting();
  EXPECT_TRUE(std::isfinite(nb_vec_.front()->getTargetDensity()));
  macro_cell = liveCellFor(macro_);
  ASSERT_NE(macro_cell, nullptr);
  EXPECT_EQ(macro_cell->dx(), macro_dx);
  EXPECT_EQ(macro_cell->dy(), macro_dy);
  EXPECT_EQ(macro_cell->getInflationArea(), 0);
}

TEST_F(TimingRepairIntegrationTest,
       VirtualRepairDoesNotRefreshRudyOrRetainBuffers)
{
  makePlacer();
  completeRoutability();
  const int tiles_before = rb_->getTotalTilesCount();
  ASSERT_GT(tiles_before, 0);
  runTiming(0.4f);
  EXPECT_EQ(timing_iterations_, 1);
  EXPECT_EQ(block_->getInsts().size(), initial_names_.size());
  EXPECT_EQ(rb_->getTotalTilesCount(), tiles_before);
  EXPECT_EQ(rb_->getTotalInflation(), 0);
  EXPECT_TRUE(nb_vec_[0]->getNewInstances().empty());
}

TEST_F(TimingRepairIntegrationTest,
       RoutabilityDisabledStillRepairsWithoutInflation)
{
  makePlacer(false);
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(false);
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
}

TEST_F(TimingRepairIntegrationTest, TimingDisabledSkipsRepairAndInflation)
{
  makePlacer(true, false);
  runTiming(0.2f);
  EXPECT_EQ(timing_iterations_, 0);
  EXPECT_EQ(block_->getInsts().size(), initial_names_.size());
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
}

TEST_F(TimingRepairIntegrationTest,
       ActiveRoutabilityIterationDefersTimingRepair)
{
  makePlacer();
  runTiming(0.2f, true);
  EXPECT_EQ(timing_iterations_, 0);
  EXPECT_EQ(block_->getInsts().size(), initial_names_.size());
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
}

TEST_F(TimingRepairIntegrationTest, BeforeFirstRoutabilityPassDefersInflation)
{
  makePlacer();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  EXPECT_EQ(ordinary_iterations_, 0);
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
  checkRetainedInsertions(false);
}

TEST_F(TimingRepairIntegrationTest,
       DefaultCompletedRoutabilityInflatesNewBuffers)
{
  makePlacer();
  completeRoutability();
  runTiming(0.2f);
  const auto [inserted, inflated] = retainedInsertionCounts();
  RecordProperty("retained_new_cells", inserted);
  RecordProperty("inflated_new_cells", inflated);
  RecordProperty("ordinary_passes", ordinary_iterations_);
  RecordProperty("routability_complete", !routabilityNeeded());
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
  checkDensityAccounting();
}

TEST_F(TimingRepairIntegrationTest,
       OrdinaryAndTimingInflationShareLiveAllowance)
{
  makePlacer();
  const int64_t original = nb_vec_[0]->getNesterovInstsArea();
  completeRoutability(true);
  ASSERT_GT(rb_->getTotalInflation(), 0);
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
  EXPECT_LE(liveInflationArea(),
            static_cast<int64_t>(static_cast<double>(original)
                                 * options_.routabilityMaxInflationTotal));
}

TEST_F(TimingRepairIntegrationTest,
       RetainsOrdinaryBudgetAfterPhysicalResizeAndDeletion)
{
  makePlacer();
  completeRoutability(true);
  const int64_t historical_charge = rb_->getTotalInflation();
  const int64_t live_before = nb_vec_[0]->getInflationArea();
  ASSERT_GT(historical_charge, 0);
  ASSERT_EQ(live_before, historical_charge);
  auto* driver = block_->findInst("fanout_driver");
  ASSERT_NE(liveCellFor(driver), nullptr);
  ASSERT_GT(liveCellFor(driver)->getInflationArea(), 0);
  ASSERT_FALSE(ep_.isIncrementalParasiticsEnabled());
  ASSERT_FALSE(ep_.hasParasiticsInvalid());
  ASSERT_FALSE(grt_.haveRoutes());
  // Model retained resizer edits in the same placement-parasitics transaction
  // as findResizeSlacks(), before entering another timing-driven iteration.
  ep_.setParasiticsSrc(est::ParasiticsSrc::kPlacement);
  {
    est::IncrementalParasiticsGuard guard(&ep_);
    ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
    ASSERT_TRUE(driver->swapMaster(lib_->findMaster("BUF_X32")));
    nbc_->fixPointers();
    EXPECT_EQ(liveCellFor(driver)->getInflationArea(), 0);
    EXPECT_TRUE(ep_.hasParasiticsInvalid());
    odb::dbInst* deleted = nullptr;
    for (const auto* cell : nbc_->getGCells()) {
      if (cell->isStdInstance() && cell->getInflationArea() > 0
          && cell->getName().starts_with("sink_")) {
        deleted = cell->insts().front()->dbInst();
        break;
      }
    }
    ASSERT_NE(deleted, nullptr);
    odb::dbInst::destroy(deleted);
    nbc_->fixPointers();
    EXPECT_TRUE(ep_.hasParasiticsInvalid());
  }
  ASSERT_FALSE(ep_.isIncrementalParasiticsEnabled());
  ASSERT_FALSE(ep_.hasParasiticsInvalid());
  nb_vec_[0]->updateGCellState();
  nb_vec_[0]->updateTimingDrivenDensity();
  ASSERT_LT(nb_vec_[0]->getInflationArea(), live_before);
  EXPECT_EQ(rb_->getTotalInflation(), historical_charge);
  // Also cross the actual retained timing boundary with ordinary inflation
  // still present. The default path must not turn its charge into live area.
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  EXPECT_EQ(rb_->getTotalInflation(), historical_charge);
  checkRetainedInsertions(true);
  checkDensityAccounting();
  int new_cells = 0;
  for (auto* inst : block_->getInsts()) {
    new_cells += !initial_names_.contains(inst->getName());
  }
  EXPECT_GT(new_cells, 0);
  EXPECT_TRUE(nb_vec_[0]->getNewInstances().empty());
}

TEST_F(TimingRepairIntegrationTest,
       CompletedMinRcSearchCannotRestoreAfterRetainedDownsize)
{
  // The protected geometry probe must not be a load on req_val: protecting
  // that load would prevent the real repair from buffering its high fanout.
  auto* driver = odb::dbInst::create(
      block_, lib_->findMaster("BUF_X32"), "downsize_probe");
  driver->setLocation(16000, 16000);
  driver->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  auto* layer = db_->getTech()->findRoutingLayer(3);
  for (const char* pin_name : {"A", "Z"}) {
    const std::string name = std::string("downsize_probe_") + pin_name;
    auto* net = odb::dbNet::create(block_, name.c_str());
    auto* iterm = driver->findITerm(pin_name);
    iterm->connect(net);
    auto* term = odb::dbBTerm::create(net, name.c_str());
    term->setIoType(pin_name[0] == 'A' ? odb::dbIoType::INPUT
                                       : odb::dbIoType::OUTPUT);
    auto* pin = odb::dbBPin::create(term);
    pin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    int x = 0;
    int y = 0;
    ASSERT_TRUE(iterm->getAvgXY(&x, &y));
    odb::dbBox::create(pin, layer, x, y, x + 1, y + 1);
    ASSERT_EQ(net->getITermCount(), 1);
  }
  initial_names_.insert(driver->getName());
  ASSERT_NE(driver->findITerm("A")->getNet(),
            block_->findBTerm("req_val")->getNet());
  ASSERT_FALSE(block_->findInst("fanout_driver")->isDoNotTouch());
  makePlacer();
  const int64_t original = nb_vec_[0]->getNesterovInstsArea();
  runOrdinaryPass(1000000);
  ASSERT_NE(liveCellFor(driver), nullptr);
  ASSERT_GT(liveCellFor(driver)->getInflationArea(), 0);
  ASSERT_TRUE(routabilityNeeded());
  const int saved_dx = liveCellFor(driver)->dx();
  const int saved_dy = liveCellFor(driver)->dy();
  // A genuine RC improvement saves the already-inflated dimensions in the
  // production min-RC snapshot before the next ordinary inflation pass.
  runOrdinaryPass(500000);
  ASSERT_TRUE(rb_->isMinRc());
  ASSERT_TRUE(routabilityNeeded());
  ASSERT_GT(static_cast<int64_t>(liveCellFor(driver)->dx())
                * liveCellFor(driver)->dy(),
            static_cast<int64_t>(saved_dx) * saved_dy);
  // Finish through a genuine min-RC revert, before this extension is eligible.
  ordinary_iterations_ = 50;
  runOrdinaryPass(1000000);
  ASSERT_FALSE(rb_->isMinRc());
  ASSERT_FALSE(routabilityNeeded());
  ASSERT_EQ(liveCellFor(driver)->dx(), saved_dx);
  ASSERT_EQ(liveCellFor(driver)->dy(), saved_dy);
  const int completed_iterations = ordinary_iterations_;
  const int completed_reverts = rb_->getRevertCount();
  const int64_t historical_charge = rb_->getTotalInflation();
  ASSERT_FALSE(ep_.isIncrementalParasiticsEnabled());
  ASSERT_FALSE(grt_.haveRoutes());
  // Publish the restored coordinates as runTimingDriven() does. This fixture
  // steered the ordinary controller outside a timing transaction, so refresh
  // the dirty placement through the estimate_parasitics -placement API before
  // entering another incremental guard, which requires clean parasitics.
  np_->updateDb();
  ASSERT_TRUE(ep_.hasParasiticsInvalid());
  ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
  ASSERT_FALSE(ep_.hasParasiticsInvalid());
  // Keep the real GPL resize callback, but also mirror the estimator guard
  // used by production resizer edits. Its destructor processes invalid nets.
  ep_.setParasiticsSrc(est::ParasiticsSrc::kPlacement);
  {
    est::IncrementalParasiticsGuard guard(&ep_);
    ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
    ASSERT_TRUE(driver->swapMaster(lib_->findMaster("BUF_X1")));
    nbc_->fixPointers();
    EXPECT_TRUE(ep_.hasParasiticsInvalid());
  }
  ASSERT_FALSE(ep_.isIncrementalParasiticsEnabled());
  ASSERT_FALSE(ep_.hasParasiticsInvalid());
  auto* cell = liveCellFor(driver);
  ASSERT_NE(cell, nullptr);
  ASSERT_EQ(cell->changeType(), GCell::GCellChange::kDownsize);
  ASSERT_EQ(cell->getInflationArea(), 0);
  const int physical_dx = cell->dx();
  const int physical_dy = cell->dy();
  ASSERT_LT(static_cast<int64_t>(physical_dx) * physical_dy,
            static_cast<int64_t>(saved_dx) * saved_dy);
  nb_vec_[0]->updateGCellState();
  nb_vec_[0]->updateTimingDrivenDensity();
  checkDensityAccounting();
  // An attempted ordinary pass after completion cannot restore the old
  // master's inflated snapshot or its obsolete target density.
  runOrdinaryPass(1000000);
  EXPECT_EQ(ordinary_iterations_, completed_iterations);
  EXPECT_EQ(rb_->getRevertCount(), completed_reverts);
  cell = liveCellFor(driver);
  ASSERT_NE(cell, nullptr);
  EXPECT_EQ(cell->dx(), physical_dx);
  EXPECT_EQ(cell->dy(), physical_dy);
  EXPECT_EQ(cell->getInflationArea(), 0);
  checkDensityAccounting();
  // Keep this downsized target material through the real repair. Do not make
  // the invariant conditional on whether resizer happened to delete it.
  driver->setDoNotTouch(true);
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  ASSERT_EQ(block_->findInst("downsize_probe"), driver);
  EXPECT_EQ(driver->getMaster(), lib_->findMaster("BUF_X1"));
  cell = liveCellFor(driver);
  ASSERT_NE(cell, nullptr);
  EXPECT_EQ(cell->dx(), physical_dx);
  EXPECT_EQ(cell->dy(), physical_dy);
  EXPECT_EQ(cell->getInflationArea(), 0);
  checkRetainedInsertions(true);
  checkDensityAccounting();
  auto* buffer = inflatedNewBuffer();
  ASSERT_NE(buffer, nullptr);
  auto* buffer_cell = liveCellFor(buffer);
  ASSERT_NE(buffer_cell, nullptr);
  const int buffer_dx = buffer_cell->dx();
  const int buffer_dy = buffer_cell->dy();
  const int64_t fillers = nb_vec_[0]->getTotalFillerArea();
  for (int attempt = 0; attempt < 3; ++attempt) {
    runOrdinaryPass(1000000);
    EXPECT_EQ(ordinary_iterations_, completed_iterations);
    EXPECT_EQ(rb_->getRevertCount(), completed_reverts);
    buffer_cell = liveCellFor(buffer);
    ASSERT_NE(buffer_cell, nullptr);
    EXPECT_EQ(buffer_cell->dx(), buffer_dx);
    EXPECT_EQ(buffer_cell->dy(), buffer_dy);
    ASSERT_EQ(block_->findInst("downsize_probe"), driver);
    cell = liveCellFor(driver);
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->dx(), physical_dx);
    EXPECT_EQ(cell->dy(), physical_dy);
    EXPECT_EQ(cell->getInflationArea(), 0);
    EXPECT_EQ(nb_vec_[0]->getTotalFillerArea(), fillers);
    checkDensityAccounting();
  }
  EXPECT_EQ(rb_->getTotalInflation(), historical_charge);
  EXPECT_LE(liveInflationArea(),
            static_cast<int64_t>(static_cast<double>(original)
                                 * options_.routabilityMaxInflationTotal));
}

TEST_F(TimingRepairIntegrationTest,
       PaddedMoveWithZeroLiveAllowanceDoesNotRecreatePadding)
{
  options_.padLeft = 2;
  options_.padRight = 3;
  options_.routabilityMaxInflationTotal = 0;
  makePlacer();
  completeRoutability();
  auto* master = lib_->findMaster("BUF_X1");
  auto* buffer = odb::dbInst::create(block_, master, "padded_moved_buffer");
  ASSERT_NE(liveCellFor(buffer), nullptr);
  ASSERT_GT(liveCellFor(buffer)->dx(), master->getWidth());
  // Exercise inDbPostMoveInst, not a fabricated geometry or baseline reset.
  buffer->setLocation(16000, 16000);
  buffer->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  buffer->findITerm("A")->connect(block_->findBTerm("req_val")->getNet());
  auto* output = odb::dbNet::create(block_, "padded_moved_output");
  buffer->findITerm("Z")->connect(output);
  block_->findInst("sink_0")->findITerm("A1")->connect(output);
  nbc_->fixPointers();
  auto* cell = liveCellFor(buffer);
  ASSERT_NE(cell, nullptr);
  ASSERT_EQ(cell->dx(), buffer->getBBox()->getDX());
  ASSERT_EQ(cell->dy(), buffer->getBBox()->getDY());
  ASSERT_GT(cell->insts().front()->getArea(),
            static_cast<int64_t>(cell->dx()) * cell->dy());
  ASSERT_FALSE(nb_vec_[0]->getNewInstances().empty());
  const int width = cell->dx();
  const int height = cell->dy();
  for (int refresh = 0; refresh < 3; ++refresh) {
    EXPECT_EQ(rb_->inflateNewCells(), 0);
    EXPECT_EQ(cell->dx(), width);
    EXPECT_EQ(cell->dy(), height);
    EXPECT_EQ(cell->getInflationArea(), 0);
    EXPECT_EQ(liveInflationArea(), 0);
  }
  rb_->updateRudyAverage(false);
  EXPECT_GT(rb_->getOverflowedTilesCount(), 0);
  nb_vec_[0]->updateGCellState();
  nb_vec_[0]->updateTimingDrivenDensity();
  checkDensityAccounting();
}

TEST_F(TimingRepairIntegrationTest, PhysicalResizeReclaimsLiveInflation)
{
  makePlacer();
  completeRoutability();
  runTiming(0.2f);
  auto* buffer = inflatedNewBuffer();
  ASSERT_NE(buffer, nullptr);
  GCell* cell = nbc_->getGCells().front();
  for (auto* candidate : nbc_->getGCells()) {
    if (candidate->contains(buffer)) {
      cell = candidate;
      break;
    }
  }
  const int64_t released = cell->getInflationArea();
  const int64_t before = liveInflationArea();
  const char* replacement
      = buffer->getMaster()->getName() == std::string("BUF_X32") ? "BUF_X16"
                                                                 : "BUF_X32";
  ASSERT_TRUE(buffer->swapMaster(lib_->findMaster(replacement)));
  nbc_->fixPointers();
  EXPECT_EQ(cell->getInflationArea(), 0);
  EXPECT_EQ(liveInflationArea(), before - released);
}

TEST_F(TimingRepairIntegrationTest, DeletionReclaimsAllowanceForReplacement)
{
  options_.routabilityMaxInflationTotal = 0.002f;
  makePlacer();
  completeRoutability();
  runTiming(0.2f);
  auto* buffer = inflatedNewBuffer();
  ASSERT_NE(buffer, nullptr);
  int64_t released = 0;
  int center_x = 0, center_y = 0;
  for (auto* cell : nbc_->getGCells()) {
    if (cell->contains(buffer)) {
      released = cell->getInflationArea();
      center_x = cell->dCx();
      center_y = cell->dCy();
    }
  }
  ASSERT_GT(released, 0);
  const int64_t before = liveInflationArea();
  odb::dbInst::destroy(buffer);
  nbc_->fixPointers();
  EXPECT_EQ(liveInflationArea(), before - released);
  auto* master = lib_->findMaster("BUF_X1");
  auto* replacement = odb::dbInst::create(block_, master, "reclaimed_buffer");
  replacement->setLocation(center_x - master->getWidth() / 2,
                           center_y - master->getHeight() / 2);
  replacement->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  replacement->findITerm("A")->connect(block_->findBTerm("req_val")->getNet());
  auto* output = odb::dbNet::create(block_, "reclaimed_output");
  replacement->findITerm("Z")->connect(output);
  block_->findInst("sink_0")->findITerm("A1")->connect(output);
  nbc_->fixPointers();
  ASSERT_GT(rb_->inflateNewCells(), 0);
  nb_vec_[0]->updateGCellState();
  nb_vec_[0]->updateTimingDrivenDensity();
  EXPECT_TRUE(nb_vec_[0]->getNewInstances().empty());
}

}  // namespace gpl
