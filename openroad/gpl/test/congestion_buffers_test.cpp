// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "gpl/Replace.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "openroad/gpl/src/nesterovBase.h"
#include "openroad/gpl/src/nesterovPlace.h"
#include "openroad/gpl/src/placerBase.h"
#include "openroad/gpl/src/routeBase.h"
#include "tst/IntegratedFixture.h"
#include "utl/Logger.h"

namespace gpl {
namespace {

int64_t cellArea(const GCell* cell)
{
  return static_cast<int64_t>(cell->dx()) * cell->dy();
}

TEST(TileGridTest, DefaultRudyRebuildResetsTileStateAndReferences)
{
  utl::Logger logger;
  TileGrid grid;
  grid.setLogger(&logger);
  grid.setLx(-20);
  grid.setLy(30);
  grid.setTileSize(10, 15);
  grid.setTileCnt(3, 2);
  const PlaceOptions options;
  ASSERT_TRUE(options.routabilityUseRudy);

  for (int rebuild = 0; rebuild < 20; ++rebuild) {
    grid.initTiles(options.routabilityUseRudy);
    ASSERT_EQ(grid.tiles().size(), 6u);
    std::unordered_set<const Tile*> references;
    for (size_t index = 0; index < grid.tiles().size(); ++index) {
      const int x = static_cast<int>(index % 3);
      const int y = static_cast<int>(index / 3);
      auto* tile = grid.tiles()[index];
      ASSERT_NE(tile, nullptr);
      EXPECT_TRUE(references.insert(tile).second);
      EXPECT_EQ(tile->x(), x);
      EXPECT_EQ(tile->y(), y);
      EXPECT_EQ(tile->lx(), -20 + x * 10);
      EXPECT_EQ(tile->ly(), 30 + y * 15);
      EXPECT_EQ(tile->ux(), tile->lx() + 10);
      EXPECT_EQ(tile->uy(), tile->ly() + 15);
      EXPECT_FLOAT_EQ(tile->inflationRatio(), 1.0f);
      EXPECT_FLOAT_EQ(tile->inflatedRatio(), 0.0f);
      EXPECT_TRUE(tile->isActionable());
      tile->setInflationRatio(2.0f);
      tile->setInflatedRatio(3.0f);
      tile->setActionable(false);
    }
  }
}

TEST(TileGridTest, DefaultRudyRebuildHandlesGrowthClearAndShrink)
{
  utl::Logger logger;
  TileGrid grid;
  grid.setLogger(&logger);
  const PlaceOptions options;
  for (const auto count : {odb::Point(2, 3),
                           odb::Point(40, 30),
                           odb::Point(0, 0),
                           odb::Point(1, 2),
                           odb::Point(3, 1)}) {
    grid.setLx(-70);
    grid.setLy(-90);
    grid.setTileSize(7, 11);
    grid.setTileCnt(count.x(), count.y());
    grid.initTiles(options.routabilityUseRudy);
    ASSERT_EQ(grid.tiles().size(), static_cast<size_t>(count.x()) * count.y());
    std::unordered_set<const Tile*> references;
    for (size_t index = 0; index < grid.tiles().size(); ++index) {
      const int x = static_cast<int>(index % count.x());
      const int y = static_cast<int>(index / count.x());
      auto* tile = grid.tiles()[index];
      ASSERT_NE(tile, nullptr);
      EXPECT_TRUE(references.insert(tile).second);
      EXPECT_EQ(tile->x(), x);
      EXPECT_EQ(tile->y(), y);
      EXPECT_EQ(tile->lx(), -70 + tile->x() * 7);
      EXPECT_EQ(tile->ly(), -90 + tile->y() * 11);
      EXPECT_EQ(tile->ux(), tile->lx() + 7);
      EXPECT_EQ(tile->uy(), tile->ly() + 11);
      EXPECT_FLOAT_EQ(tile->inflationRatio(), 1.0f);
      EXPECT_FLOAT_EQ(tile->inflatedRatio(), 0.0f);
      EXPECT_TRUE(tile->isActionable());
      tile->setInflationRatio(2.0f);
      tile->setInflatedRatio(3.0f);
      tile->setActionable(false);
    }
  }
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
    block_ = readDef(getFilePath("_main/openroad/gpl/test/gcd.def"), {lib_});
    ASSERT_NE(block_, nullptr);
    std::vector<odb::dbNet*> nets;
    for (auto* net : block_->getNets()) {
      nets.push_back(net);
    }
    for (auto* net : nets) {
      odb::dbNet::destroy(net);
    }
    makeRoutingSeedNet();
    for (auto* inst : block_->getInsts()) {
      inst->setLocation(40000, 40000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
    configureDesign();
    sta_->postReadDb(db_.get());
    sta_->postReadDef(block_);
    options_.density = 0.7f;
    options_.timingDrivenMode = true;
    options_.routabilityDrivenMode = true;
    const PlacerBaseVars pb_vars(options_);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    const NesterovBaseVars nb_vars(options_);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, 1, Clusters{});
    for (auto* group : placementGroups()) {
      auto pb = std::make_shared<PlacerBase>(
          db_.get(), pbc_, &logger_, checkInitialDensity(), group);
      nb_vec_.push_back(
          std::make_shared<NesterovBase>(nb_vars, pb, nbc_, &logger_));
    }
    nb_ = nb_vec_.front();
    np_vars_ = std::make_unique<NesterovPlaceVars>(options_, nbc_->getHpwl());
    for (const auto& nb : nb_vec_) {
      nb->setNpVars(np_vars_.get());
      nb->initDensity1();
    }
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    grt_.getRudy()->setWireWidth(1000000);
    makeRouteBase();
  }

  virtual void configureDesign() {}
  virtual bool checkInitialDensity() const { return false; }
  virtual std::vector<odb::dbGroup*> placementGroups() { return {nullptr}; }

  void makeRoutingSeedNet()
  {
    // GRT needs a real routing net to initialize capacities. Keep this small
    // port-to-port net away from the cells and the synthetic congestion tiles.
    auto* net = odb::dbNet::create(block_, "routing_seed");
    auto* layer = db_->getTech()->findRoutingLayer(3);
    for (int i = 0; i < 2; ++i) {
      const int coordinate = 57000 + i * 500;
      auto* term
          = odb::dbBTerm::create(net, ("seed_" + std::to_string(i)).c_str());
      term->setIoType(i == 0 ? odb::dbIoType::INPUT : odb::dbIoType::OUTPUT);
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

  void makeRouteBase()
  {
    rb_ = std::make_unique<RouteBase>(
        RouteBaseVars(options_), db_.get(), &grt_, nbc_, nb_vec_, &logger_);
  }

  void addDemand(const char* name = "congestion",
                 int lower = 10000,
                 int upper = 22000)
  {
    auto* net = odb::dbNet::create(block_, name);
    auto* layer = db_->getTech()->findRoutingLayer(3);
    for (const int coordinate : {lower, upper}) {
      auto* term = odb::dbBTerm::create(
          net,
          (std::string(name) + "_port_" + std::to_string(coordinate)).c_str());
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

  odb::dbInst* insertBuffer(const char* name,
                            int coordinate = 15000,
                            odb::dbGroup* group = nullptr)
  {
    auto* inst = odb::dbInst::create(block_, lib_->findMaster("BUF_X1"), name);
    inst->setLocation(coordinate, coordinate);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    if (group != nullptr) {
      group->addInst(inst);
      group->getRegion()->addInst(inst);
    }
    const size_t index = nbc_->createCbkGCell(inst);
    for (const auto& nb : nb_vec_) {
      if (nb->getGroup() == group) {
        nb->createCbkGCell(inst, index);
      }
    }
    nbc_->fixPointers();
    return inst;
  }

  GCell* cellFor(odb::dbInst* inst)
  {
    for (const auto& nb : nb_vec_) {
      for (const auto& handle : nb->getGCells()) {
        if (handle->contains(inst)) {
          return nbc_->getGCellByIndex(handle.getStorageIndex());
        }
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
                                    * handle->dDy() * handle->getDensitySmoothing();
      EXPECT_NEAR(effective_area, cellArea(handle), cellArea(handle) * 0.01);
    }
    nb_->checkConsistency();
  }

  int64_t liveInflationArea() const
  {
    int64_t area = 0;
    for (const auto& nb : nb_vec_) {
      area += nb->getInflationArea();
    }
    return area;
  }

  PlaceOptions options_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::shared_ptr<NesterovBase> nb_;
  std::vector<std::shared_ptr<NesterovBase>> nb_vec_;
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
  const int64_t existing_inflation = existing->getInflationArea();
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
  EXPECT_EQ(liveInflationArea(), existing_inflation + added);
  EXPECT_EQ(rb_->getTotalInflation(), 0);
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

TEST_F(CongestionBuffersTest, RepeatedRudyRefreshKeepsTileCountAndMetricsStable)
{
  insertBuffer("refresh");
  addDemand();
  rb_->calculateRudyTiles();
  rb_->updateRudyAverage(false);
  int x, y;
  grt_.getGridSiz