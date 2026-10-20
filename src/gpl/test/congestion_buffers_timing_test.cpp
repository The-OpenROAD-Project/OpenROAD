// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
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
    // repair; the experimental policy may not raise density to fund inflation.
    options_.density = 0.78f;
    options_.timingDrivenMode = true;
    options_.routabilityDrivenMode = true;
    options_.keepResizeBelowOverflow = 0.3f;
  }

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
                  bool inflation = true)
  {
    options_.routabilityDrivenMode = routability;
    options_.timingDrivenMode = timing;
    options_.timingDrivenCongestionInflation = inflation;
    pbc_ = std::make_shared<PlacerBaseCommon>(
        db_.get(), PlacerBaseVars(options_), &logger_);
    pb_vec_.push_back(
        std::make_shared<PlacerBase>(db_.get(), pbc_, &logger_, false));
    const NesterovBaseVars nb_vars(options_);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, 1, Clusters{});
    nb_vec_.push_back(
        std::make_shared<NesterovBase>(nb_vars, pb_vec_[0], nbc_, &logger_));
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
        std::make_unique<GraphicsNone>(),
        &logger_);
  }

  bool routabilityNeeded() const { return np_->is_routability_need_; }

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

  void runOrdinaryPass(int wire_width)
  {
    // Supply settled motion to the real controller, then execute its ordinary
    // congestion checks. Completion is not set by hand in this fixture.
    grt_.getRudy()->setWireWidth(wire_width);
    np_->average_overflow_unscaled_ = 0.2f;
    for (const auto& nb : nb_vec_) {
      nb->peak_coordi_distance_ = 1.0f;
      nb->coordiDistance_ = 0.0f;
      nb->saveSnapshot();
    }
    float current_a = 1.0f;
    np_->runRoutability(0,
                        0,
                        "",
                        1.0f,
                        np_->wireLengthCoefX_,
                        np_->wireLengthCoefY_,
                        ordinary_iterations_,
                        current_a);
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
      if (cell->isStdInstance() && cell->inflationArea() > 0
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
    EXPECT_EQ(
        nb_vec_[0]->getMovableArea(),
        nb_vec_[0]->getNesterovInstsArea() + nb_vec_[0]->getTotalFillerArea());
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
       DefaultOffKeepsCompletedRoutabilityUnmodified)
{
  ASSERT_FALSE(PlaceOptions{}.timingDrivenCongestionInflation);
  makePlacer(true, true, false);
  completeRoutability();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(false);
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
       DefaultOffRetainsOrdinaryBudgetAfterPhysicalResizeAndDeletion)
{
  makePlacer(true, true, false);
  completeRoutability(true);
  const int64_t historical_charge = rb_->getTotalInflation();
  const int64_t live_before = nb_vec_[0]->getInflationArea();
  ASSERT_GT(historical_charge, 0);
  ASSERT_EQ(live_before, historical_charge);
  auto* driver = block_->findInst("fanout_driver");
  ASSERT_GT(nbc_->dbToNb(driver)->inflationArea(), 0);
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
    EXPECT_EQ(nbc_->dbToNb(driver)->inflationArea(), 0);
    EXPECT_TRUE(ep_.hasParasiticsInvalid());
    odb::dbInst* deleted = nullptr;
    for (const auto* cell : nbc_->getGCells()) {
      if (cell->isStdInstance() && cell->inflationArea() > 0
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
  nb_vec_[0]->updateGCellState(np_->getWireLengthCoefX(),
                               np_->getWireLengthCoefY());
  nb_vec_[0]->updateTimingDrivenDensity();
  ASSERT_LT(nb_vec_[0]->getInflationArea(), live_before);
  EXPECT_EQ(rb_->getTotalInflation(), historical_charge);
  // Also cross the actual retained timing boundary with ordinary inflation
  // still present. The default path must not turn its charge into live area.
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  EXPECT_EQ(rb_->getTotalInflation(), historical_charge);
  EXPECT_EQ(inflatedNewBuffer(), nullptr);
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
  auto* driver = block_->findInst("fanout_driver");
  ASSERT_TRUE(driver->swapMaster(lib_->findMaster("BUF_X32")));
  makePlacer();
  const int64_t original = nb_vec_[0]->getNesterovInstsArea();
  runOrdinaryPass(1000000);
  ASSERT_GT(nbc_->dbToNb(driver)->inflationArea(), 0);
  ASSERT_TRUE(routabilityNeeded());
  const int saved_dx = nbc_->dbToNb(driver)->dx();
  const int saved_dy = nbc_->dbToNb(driver)->dy();
  // A genuine RC improvement saves the already-inflated dimensions in the
  // production min-RC snapshot before the next ordinary inflation pass.
  runOrdinaryPass(500000);
  ASSERT_TRUE(rb_->isMinRc());
  ASSERT_TRUE(routabilityNeeded());
  ASSERT_GT(static_cast<int64_t>(nbc_->dbToNb(driver)->dx())
                * nbc_->dbToNb(driver)->dy(),
            static_cast<int64_t>(saved_dx) * saved_dy);
  // Finish through a genuine min-RC revert, before this extension is eligible.
  ordinary_iterations_ = 50;
  runOrdinaryPass(1000000);
  ASSERT_FALSE(rb_->isMinRc());
  ASSERT_FALSE(routabilityNeeded());
  ASSERT_EQ(nbc_->dbToNb(driver)->dx(), saved_dx);
  ASSERT_EQ(nbc_->dbToNb(driver)->dy(), saved_dy);
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
  auto* cell = nbc_->dbToNb(driver);
  ASSERT_EQ(cell->changeType(), GCell::GCellChange::kDownsize);
  ASSERT_EQ(cell->inflationArea(), 0);
  const int physical_dx = cell->dx();
  const int physical_dy = cell->dy();
  ASSERT_LT(static_cast<int64_t>(physical_dx) * physical_dy,
            static_cast<int64_t>(saved_dx) * saved_dy);
  nb_vec_[0]->updateGCellState(np_->getWireLengthCoefX(),
                               np_->getWireLengthCoefY());
  nb_vec_[0]->updateTimingDrivenDensity();
  checkDensityAccounting();
  // An attempted ordinary pass after completion cannot restore the old
  // master's inflated snapshot or its obsolete target density.
  runOrdinaryPass(1000000);
  EXPECT_EQ(ordinary_iterations_, completed_iterations);
  EXPECT_EQ(rb_->getRevertCount(), completed_reverts);
  cell = nbc_->dbToNb(driver);
  EXPECT_EQ(cell->dx(), physical_dx);
  EXPECT_EQ(cell->dy(), physical_dy);
  EXPECT_EQ(cell->inflationArea(), 0);
  checkDensityAccounting();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
  checkDensityAccounting();
  auto* buffer = inflatedNewBuffer();
  ASSERT_NE(buffer, nullptr);
  const int buffer_dx = nbc_->dbToNb(buffer)->dx();
  const int buffer_dy = nbc_->dbToNb(buffer)->dy();
  const int64_t fillers = nb_vec_[0]->getTotalFillerArea();
  for (int attempt = 0; attempt < 3; ++attempt) {
    runOrdinaryPass(1000000);
    EXPECT_EQ(ordinary_iterations_, completed_iterations);
    EXPECT_EQ(rb_->getRevertCount(), completed_reverts);
    EXPECT_EQ(nbc_->dbToNb(buffer)->dx(), buffer_dx);
    EXPECT_EQ(nbc_->dbToNb(buffer)->dy(), buffer_dy);
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
  ASSERT_GT(nbc_->dbToNb(buffer)->dx(), master->getWidth());
  // Exercise inDbPostMoveInst, not a fabricated geometry or baseline reset.
  buffer->setLocation(16000, 16000);
  buffer->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  buffer->findITerm("A")->connect(block_->findBTerm("req_val")->getNet());
  auto* output = odb::dbNet::create(block_, "padded_moved_output");
  buffer->findITerm("Z")->connect(output);
  block_->findInst("sink_0")->findITerm("A1")->connect(output);
  nbc_->fixPointers();
  auto* cell = nbc_->dbToNb(buffer);
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
    EXPECT_EQ(cell->inflationArea(), 0);
    EXPECT_EQ(liveInflationArea(), 0);
  }
  rb_->updateRudyAverage(false);
  EXPECT_GT(rb_->getOverflowedTilesCount(), 0);
  nb_vec_[0]->updateGCellState(np_->getWireLengthCoefX(),
                               np_->getWireLengthCoefY());
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
  const int64_t released = cell->inflationArea();
  const int64_t before = liveInflationArea();
  const char* replacement
      = buffer->getMaster()->getName() == std::string("BUF_X32") ? "BUF_X16"
                                                                 : "BUF_X32";
  ASSERT_TRUE(buffer->swapMaster(lib_->findMaster(replacement)));
  nbc_->fixPointers();
  EXPECT_EQ(cell->inflationArea(), 0);
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
      released = cell->inflationArea();
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
  nb_vec_[0]->updateGCellState(np_->getWireLengthCoefX(),
                               np_->getWireLengthCoefY());
  nb_vec_[0]->updateTimingDrivenDensity();
  EXPECT_TRUE(nb_vec_[0]->getNewInstances().empty());
}

}  // namespace gpl
