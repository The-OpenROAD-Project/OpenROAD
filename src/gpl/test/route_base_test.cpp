// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gpl/Replace.h"
#include "grt/Rudy.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/placerBase.h"
#include "src/gpl/src/routeBase.h"
#include "tst/IntegratedFixture.h"
#include "utl/Logger.h"

namespace gpl {
namespace {

TEST(TileGridTest, RepeatedInitializationReplacesTilePointers)
{
  utl::Logger logger;
  TileGrid grid;
  grid.setLogger(&logger);
  grid.setTileCnt(13, 11);
  grid.setTileSize(400, 600);
  for (int refresh = 0; refresh < 4; ++refresh) {
    grid.initTiles(true);
    const auto& tiles = grid.tiles();
    EXPECT_EQ(tiles.size(), 143);
    const std::unordered_set<Tile*> unique(tiles.begin(), tiles.end());
    EXPECT_EQ(unique.size(), tiles.size());
  }
}

TEST(TileGridTest, ReinitializationSupportsGrowingAndShrinkingGrid)
{
  utl::Logger logger;
  TileGrid grid;
  grid.setLogger(&logger);
  grid.setLx(100);
  grid.setLy(200);
  grid.setTileSize(400, 600);
  for (const auto& [columns, rows] :
       {std::pair{2, 2}, std::pair{17, 13}, std::pair{3, 5}}) {
    grid.setTileCnt(columns, rows);
    grid.initTiles(true);
    const auto& tiles = grid.tiles();
    // Check the count before dereferencing potentially stale resized pointers.
    ASSERT_EQ(tiles.size(), columns * rows);
    const std::unordered_set<Tile*> unique(tiles.begin(), tiles.end());
    EXPECT_EQ(unique.size(), tiles.size());
    for (int y = 0; y < rows; ++y) {
      for (int x = 0; x < columns; ++x) {
        const auto* tile = tiles[y * columns + x];
        EXPECT_EQ(tile->x(), x);
        EXPECT_EQ(tile->y(), y);
        EXPECT_EQ(tile->lx(), 100 + x * 400);
        EXPECT_EQ(tile->ly(), 200 + y * 600);
        EXPECT_EQ(tile->ux(), 100 + (x + 1) * 400);
        EXPECT_EQ(tile->uy(), 200 + (y + 1) * 600);
      }
    }
  }
}

class RouteBaseTest : public tst::IntegratedFixture
{
 protected:
  RouteBaseTest() : IntegratedFixture(Technology::kNangate45, "") {}

  void SetUp() override
  {
    block_ = readDef(getFilePath("_main/src/gpl/test/ar02.def"), {lib_});
    ASSERT_NE(block_, nullptr);
    std::vector<odb::dbNet*> nets;
    for (auto* net : block_->getNets()) {
      nets.push_back(net);
    }
    for (auto* net : nets) {
      odb::dbNet::destroy(net);
    }
    addPortNet("routing_seed", 57000, 57000, 57500, 57500);
    sta_->postReadDb(db_.get());
    sta_->postReadDef(block_);
    grt_.setMinRoutingLayer(2);
    grt_.setMaxRoutingLayer(6);
    auto* rudy = grt_.getRudy();
    rudy->setWireWidth(1000000);
    tile_size_ = rudy->getTileSize();
    ASSERT_GT(tile_size_, 0);
    const auto [columns, rows] = rudy->getGridSize();
    const auto core = block_->getCoreArea();

    // One cell per interior tile makes zero-demand tiles actionable too.
    for (int y = 1; y < rows - 1 && centers_.size() < kSamples; ++y) {
      for (int x = 1; x < columns - 1 && centers_.size() < kSamples; ++x) {
        if (!core.contains(rudy->getTile(x, y).getRect())) {
          continue;
        }
        centers_.emplace_back(x * tile_size_ + tile_size_ / 2,
                              y * tile_size_ + tile_size_ / 2);
      }
    }
    ASSERT_EQ(centers_.size(), kSamples);
    const auto [first_x, first_y] = centers_.front();
    for (auto* inst : block_->getInsts()) {
      placeAtCenter(inst, first_x, first_y);
    }
    for (size_t i = 0; i < centers_.size(); ++i) {
      auto* inst = odb::dbInst::create(block_,
                                       lib_->findMaster("BUF_X1"),
                                       ("sample_" + std::to_string(i)).c_str());
      ASSERT_NE(inst, nullptr);
      placeAtCenter(inst, centers_[i].first, centers_[i].second);
    }
    const int inset = tile_size_ / 8;
    addPortNet("single_tile_peak",
               first_x - inset,
               first_y - inset,
               first_x + inset,
               first_y + inset);

    // Leave the feature-free placement policy at its defaults.
    options_.initialPlacePerturbationDist = 0.0f;
    const PlacerBaseVars pb_vars(options_);
    auto pbc = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    const NesterovBaseVars nb_vars(options_);
    auto nbc = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc, &logger_, 1, Clusters{});
    auto pb = std::make_shared<PlacerBase>(
        db_.get(), pbc, &logger_, /* check_density = */ false);
    nb_ = std::make_shared<NesterovBase>(nb_vars, pb, nbc, &logger_);
    np_vars_ = std::make_unique<NesterovPlaceVars>(options_, nbc->getHpwl());
    nb_->setNpVars(np_vars_.get());
    nb_->initDensity1();
    rb_ = std::make_unique<RouteBase>(RouteBaseVars(options_),
                                      db_.get(),
                                      &grt_,
                                      nbc,
                                      std::vector{nb_},
                                      &logger_);
  }

  void placeAtCenter(odb::dbInst* inst, int x, int y)
  {
    inst->setLocation(x - inst->getMaster()->getWidth() / 2,
                      y - inst->getMaster()->getHeight() / 2);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  void addPortNet(const std::string& name, int lx, int ly, int ux, int uy)
  {
    auto* net = odb::dbNet::create(block_, name.c_str());
    auto* layer = db_->getTech()->findRoutingLayer(3);
    int index = 0;
    for (const auto& [x, y] : {std::pair{lx, ly}, std::pair{ux, uy}}) {
      auto* term = odb::dbBTerm::create(
          net, (name + "_" + std::to_string(index)).c_str());
      term->setIoType(index++ == 0 ? odb::dbIoType::INPUT
                                   : odb::dbIoType::OUTPUT);
      auto* pin = odb::dbBPin::create(term);
      pin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      odb::dbBox::create(pin, layer, x, y, x + 100, y + 100);
    }
  }

  static float weightedCongestion(std::vector<float> samples)
  {
    std::sort(samples.begin(), samples.end(), std::greater<float>());
    const auto half_percent
        = static_cast<size_t>(std::ceil(0.005 * samples.size()));
    const auto one_percent
        = static_cast<size_t>(std::ceil(0.01 * samples.size()));
    return (std::accumulate(
                samples.begin(), samples.begin() + half_percent, 0.0)
                / half_percent
            + std::accumulate(
                  samples.begin(), samples.begin() + one_percent, 0.0)
                  / one_percent)
           / 2.0;
  }

  static constexpr size_t kSamples = 104;
  static_assert(kSamples > 100);
  int tile_size_ = 0;
  std::vector<std::pair<int, int>> centers_;
  PlaceOptions options_;
  std::shared_ptr<NesterovBase> nb_;
  std::unique_ptr<NesterovPlaceVars> np_vars_;
  std::unique_ptr<RouteBase> rb_;
};

TEST_F(RouteBaseTest, RepeatedRudyRefreshPreservesNonuniformWeightedCongestion)
{
  ASSERT_EQ(options_.routabilityRcK1, 1.0f);
  ASSERT_EQ(options_.routabilityRcK2, 1.0f);
  ASSERT_EQ(options_.routabilityRcK3, 0.0f);
  ASSERT_EQ(options_.routabilityRcK4, 0.0f);
  rb_->calculateRudyTiles();
  rb_->updateRudyAverage(false);
  const int tile_count = rb_->getTotalTilesCount();
  std::set<std::pair<int, int>> occupied;
  for (const auto& cell : nb_->getGCells()) {
    if (cell->isStdInstance()) {
      occupied.emplace(cell->dCx() / tile_size_, cell->dCy() / tile_size_);
    }
  }
  ASSERT_EQ(occupied.size(), kSamples);
  std::vector<float> samples;
  auto* rudy = grt_.getRudy();
  for (const auto& [x, y] : occupied) {
    const auto& tile = rudy->getTile(x, y);
    ASSERT_LT(tile.getBlockage(), RouteBaseVars(options_).ignoreEdgeRatio);
    samples.push_back((tile.getDemand() / 100.0f)
                      / (1.0f - tile.getBlockage()));
  }
  const float expected = weightedCongestion(samples);
  ASSERT_GT(expected, 0.0f);
  auto duplicated = samples;
  duplicated.insert(duplicated.end(), samples.begin(), samples.end());
  const float duplicate_metric = weightedCongestion(duplicated);
  ASSERT_NE(expected, duplicate_metric)
      << "The nonuniform fixture must expose duplicate percentile sampling.";
  RecordProperty("unique_weighted_congestion", std::to_string(expected));
  RecordProperty("duplicated_weighted_congestion",
                 std::to_string(duplicate_metric));
  EXPECT_EQ(rb_->getRudyAverage(), expected);

  for (int refresh = 0; refresh < 3; ++refresh) {
    rb_->calculateRudyTiles();
    rb_->updateRudyAverage(false);
    EXPECT_EQ(rb_->getTotalTilesCount(), tile_count);
    EXPECT_EQ(rb_->getRudyAverage(), expected);
  }
}

}  // namespace
}  // namespace gpl
