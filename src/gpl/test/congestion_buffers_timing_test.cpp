// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

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
    options_.density = 0.7f;
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

  void makePlacer(bool routability = true, bool timing = true)
  {
    options_.routabilityDrivenMode = routability;
    options_.timingDrivenMode = timing;
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
      EXPECT_GT(rb_->getTotalInflation(), 0);
    } else {
      EXPECT_EQ(inflated_cells, 0);
      EXPECT_EQ(rb_->getTotalInflation(), 0);
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
  int64_t delta_area_ = 0;
};

TEST_F(TimingRepairIntegrationTest,
       RetainedRepairInflatesConnectedNewBuffersBeforeStateInitialization)
{
  makePlacer();
  runTiming(0.2f);
  ASSERT_EQ(timing_iterations_, 1);
  checkRetainedInsertions(true);
}

TEST_F(TimingRepairIntegrationTest,
       VirtualRepairDoesNotRefreshRudyOrRetainBuffers)
{
  makePlacer();
  runTiming(0.4f);
  EXPECT_EQ(timing_iterations_, 1);
  EXPECT_EQ(block_->getInsts().size(), initial_names_.size());
  EXPECT_EQ(rb_->getTotalTilesCount(), 0);
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

}  // namespace gpl
