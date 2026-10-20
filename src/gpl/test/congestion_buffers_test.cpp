// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/nesterovPlace.h"
#include "src/gpl/src/placerBase.h"
#include "src/gpl/src/routeBase.h"
#include "tst/IntegratedFixture.h"

namespace gpl {
namespace {

int64_t cellArea(const GCell* cell)
{
  return static_cast<int64_t>(cell->dx()) * cell->dy();
}

// Real RUDY, callback-created buffers and the real placement density model.
// A synthetic port net concentrates routing demand in one part of the core;
// a large wire pitch makes the pressure deterministic, independent of STA.
class CongestionBuffersTest : public tst::IntegratedFixture
{
 protected:
  CongestionBuffersTest() : IntegratedFixture(Technology::kNangate45, "") {}

  void SetUp() override
  {
    block_ = readDef(getFilePath("_main/src/gpl/test/simple01.def"), {lib_});
    ASSERT_NE(block_, nullptr);
    std::vector<odb::dbNet*> nets;
    for (auto* net : block_->getNets()) {
      nets.push_back(net);
    }
    for (auto* net : nets) {
      odb::dbNet::destroy(net);
    }
    for (auto* inst : block_->getInsts()) {
      inst->setLocation(40000, 40000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    options_.density = 0.7f;
    options_.timingDrivenMode = true;
    options_.routabilityDrivenMode = true;
    const PlacerBaseVars pb_vars(options_);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    auto pb = std::make_shared<PlacerBase>(db_.get(), pbc_, &logger_, false);
    const NesterovBaseVars nb_vars(options_);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, 1, Clusters{});
    nb_ = std::make_shared<NesterovBase>(nb_vars, pb, nbc_, &logger_);
    np_vars_ = std::make_unique<NesterovPlaceVars>(options_, nbc_->getHpwl());
    nb_->setNpVars(np_vars_.get());
    nb_->initDensity1();
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    grt_.getRudy()->setWireWidth(1000000);
    makeRouteBase();
  }

  void makeRouteBase()
  {
    rb_ = std::make_unique<RouteBase>(RouteBaseVars(options_),
                                      db_.get(),
                                      &grt_,
                                      nbc_,
                                      std::vector{nb_},
                                      &logger_);
  }

  void addDemand()
  {
    auto* net = odb::dbNet::create(block_, "congestion");
    auto* layer = db_->getTech()->findRoutingLayer(3);
    for (const int coordinate : {10000, 22000}) {
      auto* term = odb::dbBTerm::create(
          net, ("port_" + std::to_string(coordinate)).c_str());
      auto* pin = odb::dbBPin::create(term);
      pin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      odb::dbBox::create(pin,
                         layer,
                         coordinate,
                         coordinate,
                         coordinate + 100,
                         coordinate + 100);
    }
  }

  odb::dbInst* insertBuffer(const char* name, int coordinate = 15000)
  {
    auto* inst = odb::dbInst::create(block_, lib_->findMaster("BUF_X1"), name);
    inst->setLocation(coordinate, coordinate);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    const size_t index = nbc_->createCbkGCell(inst);
    nb_->createCbkGCell(inst, index);
    nbc_->fixPointers();
    return inst;
  }

  GCell* cellFor(odb::dbInst* inst)
  {
    for (const auto& handle : nb_->getGCells()) {
      if (handle->contains(inst)) {
        return handle;
      }
    }
    return nullptr;
  }

  int64_t instanceArea() const
  {
    int64_t area = 0;
    for (const auto& handle : nb_->getGCells()) {
      if (handle->isInstance()) {
        area += cellArea(handle);
      }
    }
    return area;
  }

  void checkDensity()
  {
    nb_->updateTimingDrivenDensity();
    const int64_t area = instanceArea();
    EXPECT_EQ(nb_->getNesterovInstsArea(), area);
    EXPECT_EQ(nb_->getMovableArea(), area + nb_->getTotalFillerArea());
    EXPECT_NEAR(nb_->getTargetDensity(),
                static_cast<double>(area + nb_->getTotalFillerArea())
                    / nb_->getWhiteSpaceArea(),
                1e-6);
    EXPECT_TRUE(std::isfinite(nb_->getTargetDensity()));
    for (const auto& handle : nb_->getGCells()) {
      const double effective_area = static_cast<double>(handle->dDx())
                                    * handle->dDy() * handle->getDensityScale();
      EXPECT_NEAR(effective_area, cellArea(handle), cellArea(handle) * 0.01);
    }
    nb_->checkConsistency();
  }

  PlaceOptions options_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::shared_ptr<NesterovBase> nb_;
  std::unique_ptr<NesterovPlaceVars> np_vars_;
  std::unique_ptr<RouteBase> rb_;
};

TEST_F(CongestionBuffersTest, RefreshesRudyAndPreservesExistingInflation)
{
  GCell* existing = nbc_->getGCells().front();
  existing->setSize(existing->dx() + 400,
                    existing->dy() + 400,
                    GCell::GCellChange::kRoutability);
  auto* existing_inst = existing->insts().front()->dbInst();
  const int existing_width = existing->dx();
  const int existing_height = existing->dy();
  auto* hot = insertBuffer("hot");
  auto* cold = insertBuffer("cold", 45000);
  const int64_t hot_area = cellArea(cellFor(hot));
  const int64_t cold_area = cellArea(cellFor(cold));
  rb_->calculateRudyTiles();
  // Demand appears after the old map, as it can during timing repair.
  addDemand();
  const int64_t added = rb_->inflateNewCells();
  ASSERT_GT(added, 0) << "the regression must exercise actual cell inflation";
  EXPECT_EQ(cellArea(cellFor(hot)), hot_area + added);
  EXPECT_EQ(cellArea(cellFor(cold)), cold_area);
  EXPECT_EQ(cellFor(existing_inst)->dx(), existing_width);
  EXPECT_EQ(cellFor(existing_inst)->dy(), existing_height);
  EXPECT_EQ(rb_->getTotalInflation(), added);
  EXPECT_EQ(rb_->getRevertCount(), 0);
  EXPECT_LE(static_cast<double>(cellArea(cellFor(hot))) / hot_area,
            options_.routabilityMaxInflationRatio);
  EXPECT_EQ(rb_->inflateNewCells(), 0) << "do not compound the same insertion";
  checkDensity();
  checkDensity();  // Re-accounting must not count inserted area twice.
}

TEST_F(CongestionBuffersTest, UncongestedBufferKeepsItsGeometry)
{
  auto* buffer = insertBuffer("uncongested");
  const int64_t area = cellArea(cellFor(buffer));
  EXPECT_EQ(rb_->inflateNewCells(), 0);
  EXPECT_EQ(cellArea(cellFor(buffer)), area);
  checkDensity();
}

TEST_F(CongestionBuffersTest, NoCreationsDoNotChangeExistingCells)
{
  addDemand();
  const int64_t area = instanceArea();
  EXPECT_EQ(rb_->inflateNewCells(), 0);
  EXPECT_EQ(instanceArea(), area);
}

TEST_F(CongestionBuffersTest, ResizedNewBufferStillReceivesInflation)
{
  addDemand();
  auto* buffer = insertBuffer("resized");
  ASSERT_TRUE(buffer->swapMaster(lib_->findMaster("BUF_X2")));
  nbc_->resizeGCell(buffer);
  const int64_t area = cellArea(cellFor(buffer));
  ASSERT_EQ(cellFor(buffer)->changeType(), GCell::GCellChange::kUpsize);
  ASSERT_GT(rb_->inflateNewCells(), 0);
  EXPECT_GT(cellArea(cellFor(buffer)), area);
  checkDensity();
}

TEST_F(CongestionBuffersTest, DensityHeadroomLimitsAddedArea)
{
  addDemand();
  auto* buffer = insertBuffer("limited");
  const int64_t area = cellArea(cellFor(buffer));
  const int64_t occupied = instanceArea() + nb_->getTotalFillerArea();
  const int64_t headroom = area / 4;
  options_.routabilityMaxDensity
      = static_cast<double>(occupied + headroom) / nb_->getWhiteSpaceArea();
  makeRouteBase();
  const int64_t added = rb_->inflateNewCells();
  ASSERT_GT(added, 0);
  EXPECT_LE(added, headroom + 512);  // Float density is rounded in DBU area.
  EXPECT_EQ(cellArea(cellFor(buffer)), area + added);
  checkDensity();
  EXPECT_LE(nb_->getTargetDensity(), options_.routabilityMaxDensity + 1e-6);
}

TEST_F(CongestionBuffersTest, OverfullRegionDoesNotGainInflation)
{
  addDemand();
  auto* buffer = insertBuffer("overfull");
  const int64_t area = cellArea(cellFor(buffer));
  options_.routabilityMaxDensity = 0.01f;
  makeRouteBase();
  EXPECT_EQ(rb_->inflateNewCells(), 0);
  EXPECT_EQ(cellArea(cellFor(buffer)), area);
  checkDensity();
}

TEST_F(CongestionBuffersTest, ZeroBudgetDoesNotGainInflation)
{
  addDemand();
  auto* buffer = insertBuffer("zero_budget");
  const int64_t area = cellArea(cellFor(buffer));
  options_.routabilityMaxInflationTotal = 0;
  makeRouteBase();
  EXPECT_EQ(rb_->inflateNewCells(), 0);
  EXPECT_EQ(cellArea(cellFor(buffer)), area);
  checkDensity();
}

TEST_F(CongestionBuffersTest, DeletedCreationAndStorageSwapsAreHandled)
{
  addDemand();
  auto* transient = insertBuffer("transient");
  auto* retained = insertBuffer("retained");
  const int64_t area = cellArea(cellFor(retained));
  auto replacement = nb_->destroyCbkGCell(transient);
  ASSERT_TRUE(replacement.has_value());
  if (replacement->first != nullptr) {
    ASSERT_TRUE(nb_->updateHandle(replacement->first, replacement->second));
  }
  odb::dbInst::destroy(transient);
  nbc_->fixPointers();
  ASSERT_GT(rb_->inflateNewCells(), 0);
  EXPECT_GT(cellArea(cellFor(retained)), area);
  checkDensity();
}

TEST_F(CongestionBuffersTest, BurstOfInsertionsCannotOverdrawBudget)
{
  addDemand();
  options_.routabilityMaxInflationTotal = 0.01f;
  const int64_t original_area = nb_->getNesterovInstsArea();
  makeRouteBase();
  constexpr int kBuffers = 64;
  std::vector<odb::dbInst*> buffers;
  for (int i = 0; i < kBuffers; ++i) {
    buffers.push_back(insertBuffer(("burst_" + std::to_string(i)).c_str()));
  }
  const int64_t buffer_area = cellArea(cellFor(buffers.front()));
  const int64_t added = rb_->inflateNewCells();
  ASSERT_GT(added, 0);
  EXPECT_LE(added,
            static_cast<int64_t>(original_area
                                 * options_.routabilityMaxInflationTotal));
  int inflated_count = 0;
  int64_t measured_added = 0;
  for (auto* buffer : buffers) {
    const int64_t area = cellArea(cellFor(buffer));
    EXPECT_GE(area, buffer_area);
    measured_added += area - buffer_area;
    inflated_count += area > buffer_area;
  }
  EXPECT_GT(inflated_count, 0);
  EXPECT_LT(inflated_count, kBuffers);
  EXPECT_EQ(measured_added, added);
  checkDensity();
  EXPECT_LE(nb_->getTargetDensity(), options_.routabilityMaxDensity);
}

TEST_F(CongestionBuffersTest, AreaAccountingDoesNotApplyOtherRegionsDelta)
{
  auto unaffected = std::make_shared<NesterovBase>(
      NesterovBaseVars(options_), nb_->getPb(), nbc_, &logger_);
  unaffected->setNpVars(np_vars_.get());
  unaffected->initDensity1();
  const int64_t area = unaffected->getNesterovInstsArea();
  const int64_t fillers = unaffected->getTotalFillerArea();
  insertBuffer("other_region");
  ASSERT_GT(nbc_->getDeltaArea(), 0);
  unaffected->updateTimingDrivenDensity();
  EXPECT_EQ(unaffected->getNesterovInstsArea(), area);
  EXPECT_EQ(unaffected->getMovableArea(), area + fillers);
  EXPECT_NEAR(
      unaffected->getTargetDensity(),
      static_cast<double>(area + fillers) / unaffected->getWhiteSpaceArea(),
      1e-6);
}

TEST_F(CongestionBuffersTest, OddDimensionsHaveExactNonshrinkingAreaAccounting)
{
  addDemand();
  auto* buffer = insertBuffer("odd_geometry");
  GCell* cell = cellFor(buffer);
  cell->setAllLocations(cell->lx(), cell->ly(), cell->ux() + 1, cell->uy() + 1);
  const int width = cell->dx();
  const int height = cell->dy();
  const int64_t area = cellArea(cell);
  const int64_t added = rb_->inflateNewCells();
  ASSERT_GT(added, 0);
  EXPECT_GE(cell->dx(), width);
  EXPECT_GE(cell->dy(), height);
  EXPECT_EQ(cellArea(cell), area + added);
  EXPECT_LE(static_cast<double>(cellArea(cell)) / area,
            options_.routabilityMaxInflationRatio);
  checkDensity();
}

}  // namespace
}  // namespace gpl
