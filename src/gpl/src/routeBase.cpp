// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "routeBase.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "gpl/Replace.h"
#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

using odb::dbBlock;
using std::pair;
using std::string;
using std::vector;

using utl::GPL;

namespace gpl {

Tile::Tile() = default;

Tile::Tile(int x, int y, int lx, int ly, int ux, int uy, int layers)
{
  x_ = x;
  y_ = y;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

int64_t Tile::area() const
{
  return static_cast<int64_t>(ux_ - lx_) * static_cast<int64_t>(uy_ - ly_);
}

float Tile::inflationRatio() const
{
  return inflationRatio_;
}

float Tile::inflatedRatio() const
{
  return inflatedRatio_;
}

void Tile::setInflationRatio(float ratio)
{
  inflationRatio_ = ratio;
}

void Tile::setInflatedRatio(float ratio)
{
  inflatedRatio_ = ratio;
}

void TileGrid::setLogger(utl::Logger* log)
{
  log_ = log;
}

void TileGrid::setTileCnt(int tileCntX, int tileCntY)
{
  tileCntX_ = tileCntX;
  tileCntY_ = tileCntY;
}

void TileGrid::setTileCntX(int tileCntX)
{
  tileCntX_ = tileCntX;
}

void TileGrid::setTileCntY(int tileCntY)
{
  tileCntY_ = tileCntY;
}

void TileGrid::setTileSize(int tileSizeX, int tileSizeY)
{
  tileSizeX_ = tileSizeX;
  tileSizeY_ = tileSizeY;
}

void TileGrid::setTileSizeX(int tileSizeX)
{
  tileSizeX_ = tileSizeX;
}

void TileGrid::setTileSizeY(int tileSizeY)
{
  tileSizeY_ = tileSizeY;
}

void TileGrid::setNumRoutingLayers(int num)
{
  numRoutingLayers_ = num;
}

void TileGrid::setLx(int lx)
{
  lx_ = lx;
}

void TileGrid::setLy(int ly)
{
  ly_ = ly;
}

void TileGrid::initTiles(bool use_rudy)
{
  debugPrint(log_,
             GPL,
             "rudy",
             1,
             "{:9} ( {:4} {:4} ) ( {:4} {:4} ) DBU",
             "TileBBox:",
             lx_,
             ly_,
             tileSizeX_,
             tileSizeY_);
  debugPrint(
      log_, GPL, "rudy", 1, "{:9} {:6} {:4}", "TileCnt:", tileCntX_, tileCntY_);
  if (!use_rudy) {
    log_->info(GPL, 39, "Number of routing layers: {}", numRoutingLayers_);
  }

  // 2D tile grid structure init
  int x = lx_, y = ly_;
  int idxX = 0, idxY = 0;
  tileStor_.resize(static_cast<uint64_t>(tileCntX_)
                   * static_cast<uint64_t>(tileCntY_));
  for (auto& tile : tileStor_) {
    tile = Tile(
        idxX, idxY, x, y, x + tileSizeX_, y + tileSizeY_, numRoutingLayers_);

    x += tileSizeX_;
    idxX += 1;
    if (x >= ux()) {
      y += tileSizeY_;
      x = lx_;

      idxY++;
      idxX = 0;
    }

    tiles_.push_back(&tile);
  }
  debugPrint(log_, GPL, "rudy", 1, "NumTiles: {}", tiles_.size());
}

int TileGrid::lx() const
{
  return lx_;
}

int TileGrid::ly() const
{
  return ly_;
}

// this is points
int TileGrid::ux() const
{
  return lx_ + tileCntX_ * tileSizeX_;
}
int TileGrid::uy() const
{
  return ly_ + tileCntY_ * tileSizeY_;
}

int TileGrid::tileCntX() const
{
  return tileCntX_;
}

int TileGrid::tileCntY() const
{
  return tileCntY_;
}

int TileGrid::tileSizeX() const
{
  return tileSizeX_;
}

int TileGrid::tileSizeY() const
{
  return tileSizeY_;
}

int TileGrid::numRoutingLayers() const
{
  return numRoutingLayers_;
}

const std::vector<Tile*>& TileGrid::tiles() const
{
  return tiles_;
}

/////////////////////////////////////////////
// RouteBaseVars

RouteBaseVars::RouteBaseVars(const PlaceOptions& options)
    : useRudy(options.routabilityUseRudy),
      targetRC(options.routabilityTargetRcMetric),
      inflationRatioCoef(options.routabilityInflationRatioCoef),
      maxInflationRatio(options.routabilityMaxInflationRatio),
      maxDensity(options.routabilityMaxDensity),
      ignoreEdgeRatio(0.8),
      minInflationRatio(1.01),
      rcK1(options.routabilityRcK1),
      rcK2(options.routabilityRcK2),
      rcK3(options.routabilityRcK3),
      rcK4(options.routabilityRcK4)
{
}

/////////////////////////////////////////////
// RouteBase

RouteBase::RouteBase(RouteBaseVars rbVars,
                     odb::dbDatabase* db,
                     grt::GlobalRouter* grouter,
                     std::shared_ptr<NesterovBaseCommon> nbc,
                     std::vector<std::shared_ptr<NesterovBase>> nbVec,
                     utl::Logger* log)
    : rbVars_(rbVars)
{
  db_ = db;
  grouter_ = grouter;
  nbc_ = std::move(nbc);
  log_ = log;
  nbVec_ = std::move(nbVec);
  minRcTargetDensity_.resize(nbVec_.size(), 0);
  inflatedAreaDelta_.resize(nbVec_.size(), 0);
  accumulatedInflatedAreaDelta_.resize(nbVec_.size(), 0);
  minRcInflatedAreaDelta_.resize(nbVec_.size(), 0);
  init();
}

RouteBase::~RouteBase() = default;

void RouteBase::resetRoutabilityResources()
{
  for (auto& area : inflatedAreaDelta_) {
    area = 0;
  }
}

void RouteBase::revertToMinCongestion()
{
  log_->info(GPL,
             55,
             "Reverting inflation values and target density from the "
             "iteration with "
             "minimum observed routing congestion.");
  log_->info(GPL, 56, "Minimum observed routing congestion: {:.4f}", minRc_);

  // revert
  nbc_->revertGCellSizeToMinRc();
  for (int j = 0; j < nbVec_.size(); j++) {
    log_->info(GPL,
               57,
               "Target density at minimum routing congestion: {:.4f}{}",
               minRcTargetDensity_[j],
               nbVec_[j]->getGroup()
                   ? " (" + std::string(nbVec_[j]->getGroup()->getName()) + ")"
                   : "");

    nbVec_[j]->setTargetDensity(minRcTargetDensity_[j]);
    nbVec_[j]->restoreRemovedFillers();
    nbVec_[j]->updateDensitySize();
    accumulatedInflatedAreaDelta_[j] = minRcInflatedAreaDelta_[j];
  }
  resetRoutabilityResources();
}

void RouteBase::init()
{
  // tg_ init
  std::unique_ptr<TileGrid> tg(new TileGrid());
  tg_ = std::move(tg);

  tg_->setLogger(log_);
  nbc_->resizeMinRcCellSize();
}

void RouteBase::getGrtResult()
{
  // update gCells' location to DB for GR
  nbc_->updateDbGCells();

  // these two options must be on
  grouter_->setAllowCongestion(true);
  grouter_->setCongestionIterations(0);

  // this option must be off
  grouter_->setCriticalNetsPercentage(0);

  grouter_->globalRoute();

  updateGrtRoute();
}

void RouteBase::loadGrt()
{
  grouter_->setAllowCongestion(true);
  grouter_->setCongestionIterations(0);
  grouter_->setCriticalNetsPercentage(0);
  grouter_->globalRoute();
}

std::vector<int64_t> RouteBase::inflatedAreaDelta() const
{
  return inflatedAreaDelta_;
}

int64_t RouteBase::getTotalInflation() const
{
  int64_t totalInflation = 0;
  for (auto inflation : accumulatedInflatedAreaDelta_) {
    totalInflation += inflation;
  }
  return totalInflation;
}

int RouteBase::getRevertCount() const
{
  return revert_count_;
}

static float getUsageCapacityRatio(Tile* tile,
                                   odb::dbTechLayer* layer,
                                   odb::dbGCellGrid* gGrid,
                                   grt::GlobalRouter* grouter,
                                   float ignoreEdgeRatio)
{
  uint8_t blockH = 0, blockV = 0;
  grouter->getBlockage(layer, tile->x(), tile->y(), blockH, blockV);

  bool isHorizontal
      = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

  // from the dbGCellGrid discussion in PR,
  // 'usage' contains (blockage + wire consumption).
  // where blockage is unavailable wires due to obstruction.
  //
  // 'capacity' contains (total capacity)
  // (i.e., total number of wires in the current tile)
  //
  // RePlAce/RC metric need 'blockage' because
  // if blockage ratio is larger than certain ratio,
  // need to skip.
  uint8_t curCap = gGrid->getCapacity(layer, tile->x(), tile->y());
  uint8_t curUse = gGrid->getUsage(layer, tile->x(), tile->y());
  uint8_t blockage = (isHorizontal) ? blockH : blockV;

  // escape tile ratio cals when capacity = 0
  if (curCap == 0) {
    return std::numeric_limits<float>::lowest();
  }

  // ignore if blockage is too huge in current tile
  float blockageRatio = static_cast<float>(blockage) / curCap;
  if (blockageRatio >= ignoreEdgeRatio) {
    return std::numeric_limits<float>::lowest();
  }

  // return usage (used routing track + blockage) / total capacity
  return static_cast<float>(curUse) / curCap;
}

void RouteBase::calculateRudyTiles()
{
  nbc_->updateDbGCells();
  grt::Rudy* rudy = grouter_->getRudy();
  rudy->calculateRudy();
  tg_->setNumRoutingLayers(0);

  // update grid tile info
  tg_->setLx(0);
  tg_->setLy(0);
  tg_->setTileSize(rudy->getTileSize(), rudy->getTileSize());
  int x_grids, y_grids;
  grouter_->getGridSize(x_grids, y_grids);
  tg_->setTileCnt(x_grids, y_grids);
  tg_->initTiles(rbVars_.useRudy);

  for (auto& tile : tg_->tiles()) {
    float ratio = rudy->getTile(tile->x(), tile->y()).getRudy() / 100.0;

    // update inflation Ratio
    if (ratio >= rbVars_.minInflationRatio) {
      float inflationRatio = std::pow(ratio, rbVars_.inflationRatioCoef);
      inflationRatio = std::fmin(inflationRatio, rbVars_.maxInflationRatio);
      tile->setInflationRatio(inflationRatio);
    }
  }

  if (log_->debugCheck(GPL, "updateInflationRatio", 1)) {
    auto log = [this](auto... param) {
      log_->debug(GPL, "updateInflationRatio", param...);
    };
    for (auto& tile : tg_->tiles()) {
      if (tile->inflationRatio() > 1.0) {
        log("xy: {} {}", tile->x(), tile->y());
        log("minxy: {} {}", tile->lx(), tile->ly());
        log("maxxy: {} {}", tile->ux(), tile->uy());
        log("calcInflationRatio: {}", tile->inflationRatio());
      }
    }
  }
}

// fill
//
// TileGrids'
// lx_ ly_
// tileCntX_ tileCntY_
// tileSizeX_ tileSizeY_
//
void RouteBase::updateGrtRoute()
{
  odb::dbGCellGrid* gGrid = db_->getChip()->getBlock()->getGCellGrid();
  std::vector<int> gridX, gridY;
  gGrid->getGridX(gridX);
  gGrid->getGridY(gridY);

  // retrieve routing Layer Count from odb
  odb::dbTech* tech = db_->getTech();
  int numLayers = tech->getRoutingLayerCount();
  tg_->setNumRoutingLayers(numLayers);

  // update grid tile info
  tg_->setLx(gridX[0]);
  tg_->setLy(gridY[0]);
  tg_->setTileSize(gridX[1] - gridX[0], gridY[1] - gridY[0]);
  tg_->setTileCnt(gridX.size(), gridY.size());
  tg_->initTiles(rbVars_.useRudy);

  int min_routing_layer, max_routing_layer;
  grouter_->getMinMaxLayer(min_routing_layer, max_routing_layer);
  for (int i = 1; i <= numLayers; i++) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(i);
    bool isHorizontalLayer
        = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

    for (auto& tile : tg_->tiles()) {
      float ratio;
      if (i >= min_routing_layer && i <= max_routing_layer) {
        // Check left and down tile
        // and set the minimum usage/cap vals for
        // TileGrid setup.

        // first extract current tiles' usage
        ratio = getUsageCapacityRatio(
            tile, layer, gGrid, grouter_, rbVars_.ignoreEdgeRatio);

        // if horizontal layer (i.e., vertical edges)
        // should consider LEFT tile's RIGHT edge == current 'tile's LEFT edge
        // (current 'ratio' points to RIGHT edges usage)
        if (isHorizontalLayer && tile->x() >= 1) {
          Tile* leftTile
              = tg_->tiles()[tile->y() * tg_->tileCntX() + tile->x() - 1];
          float leftRatio = getUsageCapacityRatio(
              leftTile, layer, gGrid, grouter_, rbVars_.ignoreEdgeRatio);
          ratio = std::fmax(leftRatio, ratio);
        }

        // if vertical layer (i.e., horizontal edges)
        // should consider DOWN tile's UP edge == current 'tile's DOWN edge
        // (current 'ratio' points to UP edges usage)
        if (!isHorizontalLayer && tile->y() >= 1) {
          Tile* downTile
              = tg_->tiles()[(tile->y() - 1) * tg_->tileCntX() + tile->x()];
          float downRatio = getUsageCapacityRatio(
              downTile, layer, gGrid, grouter_, rbVars_.ignoreEdgeRatio);
          ratio = std::fmax(downRatio, ratio);
        }

        ratio = std::fmax(ratio, 0.0f);
      } else {
        ratio = 0.0;
      }
      //  update inflation Ratio
      if (ratio >= rbVars_.minInflationRatio) {
        float inflationRatio = std::pow(ratio, rbVars_.inflationRatioCoef);
        inflationRatio = std::fmin(inflationRatio, rbVars_.maxInflationRatio);
        tile->setInflationRatio(inflationRatio);
      }
    }
  }

  // debug print
  for (auto& tile : tg_->tiles()) {
    if (tile->inflationRatio() > 1.0) {
      debugPrint(log_,
                 GPL,
                 "updateInflationRatio",
                 1,
                 "xy: {} {}",
                 tile->x(),
                 tile->y());
      debugPrint(log_,
                 GPL,
                 "updateInflationRatio",
                 1,
                 "minxy: {} {}",
                 tile->lx(),
                 tile->ly());
      debugPrint(log_,
                 GPL,
                 "updateInflationRatio",
                 1,
                 "maxxy: {} {}",
                 tile->ux(),
                 tile->uy());
      debugPrint(log_,
                 GPL,
                 "updateInflationRatio",
                 1,
                 "calcInflationRatio: {}",
                 tile->inflationRatio());
    }
  }
}

// first: is Routability Need
// second: reverting procedure init need
//          (e.g. calling NesterovPlace's init())
std::pair<bool, bool> RouteBase::routability(
    int routability_driven_revert_count)
{
  increaseCounter();

  // create Tile Grid
  std::unique_ptr<TileGrid> tg(new TileGrid());
  tg_ = std::move(tg);
  tg_->setLogger(log_);

  float curRc;
  if (rbVars_.useRudy) {
    calculateRudyTiles();
    updateRudyAverage(true);
    curRc = getRudyAverage();
  } else {
    getGrtResult();
    curRc = getGrtRC();
  }

  if (curRc < rbVars_.targetRC) {
    log_->info(GPL,
               50,
               "Weighted routing congestion is lower than target routing "
               "congestion({:.4f}), "
               "end routability optimization.",
               rbVars_.targetRC);
    resetRoutabilityResources();
    return std::make_pair(false, false);
  }

  // saving solutions when minRc happen.
  if ((minRc_ - curRc) > 0.001) {
    is_min_rc_ = true;
    log_->info(GPL,
               48,
               "Routing congestion ({:.4f}) lower than previous minimum "
               "({:.4g}). Updating minimum.",
               curRc,
               minRc_);
    minRc_ = curRc;
    min_RC_violated_cnt_ = 0;
    for (int i = 0; i < nbVec_.size(); i++) {
      minRcTargetDensity_[i] = nbVec_[i]->getTargetDensity();
      nbVec_[i]->clearRemovedFillers();
    }

    // save cell size info
    nbc_->updateMinRcCellSize();
    minRcInflatedAreaDelta_ = accumulatedInflatedAreaDelta_;
  } else {
    is_min_rc_ = false;
    min_RC_violated_cnt_++;
    log_->info(GPL,
               49,
               "Routing congestion ({:.4f}) higher than minimum ({:.4f}). "
               "Consecutive non-improvement count: {}.",
               curRc,
               minRc_,
               min_RC_violated_cnt_);
  }

  // set inflated ratio
  for (auto& tile : tg_->tiles()) {
    if (tile->inflationRatio() > 1) {
      tile->setInflatedRatio(tile->inflationRatio());
    } else {
      tile->setInflatedRatio(1.0);
    }
  }

  // inflate cells and remove fillers
  std::vector<double> prev_white_space_area(nbVec_.size());
  std::vector<double> prev_movable_area(nbVec_.size());
  std::vector<double> prev_total_filler_area(nbVec_.size());
  std::vector<double> prev_total_gcells_area(nbVec_.size());
  std::vector<double> prev_expected_gcells_area(nbVec_.size());
  dbBlock* block = db_->getChip()->getBlock();
  for (int i = 0; i < nbVec_.size(); i++) {
    inflatedAreaDelta_[i] = 0;
    // run bloating and get inflatedAreaDelta_
    for (auto& gCellHandle : nbVec_[i]->getGCells()) {
      // only care about "standard cell"
      if (!gCellHandle->isStdInstance()) {
        continue;
      }
      if (!gCellHandle.isNesterovBaseCommon()) {
        log_->error(GPL,
                    159,
                    "Gcell {} from group {} is a Std instance, but is not "
                    "from NesterovBaseCommon. This shouldn't happen.",
                    gCellHandle->getName(),
                    nbVec_[i]->getGroup()->getName());
      }
      auto gCell = nbc_->getGCellByIndex(gCellHandle.getStorageIndex());

      int idxX = std::min((gCell->dCx() - tg_->lx()) / tg_->tileSizeX(),
                          tg_->tileCntX() - 1);
      int idxY = std::min((gCell->dCy() - tg_->ly()) / tg_->tileSizeY(),
                          tg_->tileCntY() - 1);

      size_t index = (idxY * tg_->tileCntX()) + idxX;
      if (index >= tg_->tiles().size()) {
        continue;
      }
      Tile* tile = tg_->tiles()[index];

      // Don't care when inflRatio <= 1
      if (tile->inflatedRatio() <= 1.0) {
        continue;
      }

      int64_t prevCellArea = static_cast<int64_t>(gCell->dx())
                             * static_cast<int64_t>(gCell->dy());

      // bloat
      gCell->setSize(static_cast<int>(std::round(
                         gCell->dx() * std::sqrt(tile->inflatedRatio()))),
                     static_cast<int>(std::round(
                         gCell->dy() * std::sqrt(tile->inflatedRatio()))),
                     GCell::GCellChange::kRoutability);

      int64_t newCellArea = static_cast<int64_t>(gCell->dx())
                            * static_cast<int64_t>(gCell->dy());

      // deltaArea is equal to area * deltaRatio
      // both of original and density size will be changed
      inflatedAreaDelta_[i] += newCellArea - prevCellArea;
    }
    accumulatedInflatedAreaDelta_[i] += inflatedAreaDelta_[i];

    float inflated_area_delta_microns
        = block->dbuAreaToMicrons(inflatedAreaDelta_[i]);
    float inflated_area_delta_percentage
        = (static_cast<float>(inflatedAreaDelta_[i])
           / nbVec_[i]->getNesterovInstsArea())
          * 100.0f;
    log_->info(GPL,
               51,
               format_label_um2_with_delta,
               "Inflated area:",
               inflated_area_delta_microns,
               inflated_area_delta_percentage);
    log_->info(GPL,
               52,
               format_label_float,
               "Placement target density:",
               nbVec_[i]->getTargetDensity());

    prev_white_space_area[i] = nbVec_[i]->getWhiteSpaceArea();
    prev_movable_area[i] = nbVec_[i]->getMovableArea();
    prev_total_filler_area[i] = nbVec_[i]->getTotalFillerArea();
    prev_total_gcells_area[i]
        = nbVec_[i]->getNesterovInstsArea() + nbVec_[i]->getTotalFillerArea();
    prev_expected_gcells_area[i]
        = inflatedAreaDelta_[i] + prev_total_gcells_area[i];

    nbVec_[i]->cutFillerCells(inflatedAreaDelta_[i]);

    // max density detection
    if (nbVec_[i]->getTargetDensity() > rbVars_.maxDensity) {
      log_->info(GPL,
                 53,
                 "Target density {:.4f} exceeds the maximum allowed {:.4f}{}.",
                 nbVec_[i]->getTargetDensity(),
                 rbVars_.maxDensity,
                 nbVec_[i]->getGroup()
                     ? " in group " + string(nbVec_[i]->getGroup()->getName())
                     : "");

      revertToMinCongestion();
      return std::make_pair(false, true);
    }
  }

  if (routability_driven_revert_count >= max_routability_revert_) {
    log_->info(GPL,
               91,
               "Routability mode reached the maximum allowed reverts {}",
               routability_driven_revert_count);

    revertToMinCongestion();
    return std::make_pair(false, true);
  }
  // rc not improvement detection -- (not improved the RC values 3 times in a
  // row)
  if (min_RC_violated_cnt_ >= max_routability_no_improvement_) {
    log_->info(GPL,
               54,
               "No improvement in routing congestion for {} consecutive "
               "iterations (limit is {}).",
               min_RC_violated_cnt_,
               max_routability_no_improvement_);

    revertToMinCongestion();
    return std::make_pair(false, true);
  }

  // updateArea
  for (int i = 0; i < nbVec_.size(); i++) {
    nbVec_[i]->updateAreas();
    nbVec_[i]->checkConsistency();

    double new_total_gcells_area
        = nbVec_[i]->getNesterovInstsArea() + nbVec_[i]->getTotalFillerArea();
    double new_expected_gcells_area
        = inflatedAreaDelta_[i] + new_total_gcells_area;

    auto percentDiff = [](double old_value, double new_value) -> double {
      if (old_value == 0.0) {
        return 0.0;
      }
      return ((new_value - old_value) / old_value) * 100.0;
    };

    log_->info(
        GPL,
        58,
        format_label_um2_with_delta,
        "White space area:",
        block->dbuAreaToMicrons(nbVec_[i]->getWhiteSpaceArea()),
        percentDiff(prev_white_space_area[i], nbVec_[i]->getWhiteSpaceArea()));

    log_->info(GPL,
               59,
               format_label_um2_with_delta,
               "Movable instances area:",
               block->dbuAreaToMicrons(nbVec_[i]->getMovableArea()),
               percentDiff(prev_movable_area[i], nbVec_[i]->getMovableArea()));

    log_->info(GPL,
               60,
               format_label_um2_with_delta,
               "Total filler area:",
               block->dbuAreaToMicrons(nbVec_[i]->getTotalFillerArea()),
               percentDiff(prev_total_filler_area[i],
                           nbVec_[i]->getTotalFillerArea()));

    log_->info(GPL,
               61,
               format_label_um2_with_delta,
               "Total non-inflated area:",
               block->dbuAreaToMicrons(new_total_gcells_area),
               percentDiff(prev_total_gcells_area[i], new_total_gcells_area));

    log_->info(
        GPL,
        62,
        format_label_um2_with_delta,
        "Total inflated area:",
        block->dbuAreaToMicrons(new_expected_gcells_area),
        percentDiff(prev_expected_gcells_area[i], new_expected_gcells_area));

    log_->info(GPL,
               63,
               format_label_float,
               "New Target Density:",
               nbVec_[i]->getTargetDensity());

    // update densitySizes for all gCell
    nbVec_[i]->updateDensitySize();
  }

  // reset
  resetRoutabilityResources();

  return std::make_pair(true, true);
}

void RouteBase::updateRudyAverage(bool verbose)
{
  grt::Rudy* rudy = grouter_->getRudy();
  total_route_overflow_ = 0.0;
  overflowed_tiles_count_ = 0;
  std::vector<double> edge_cong_array;

  for (auto& tile : tg_->tiles()) {
    float ratio = rudy->getTile(tile->x(), tile->y()).getRudy() / 100.0;
    // Escape the case when blockage ratio is too huge
    if (ratio >= 0.0f) {
      total_route_overflow_ += std::fmax(0.0, -1 + ratio);
      edge_cong_array.push_back(ratio);

      if (ratio > 1.0) {
        overflowed_tiles_count_++;
      }
    }
  }

  if (verbose) {
    log_->info(
        GPL, 41, "Total routing overflow: {:.4f}", total_route_overflow_);
    log_->info(
        GPL,
        42,
        "Number of overflowed tiles: {} ({:.2f}%)",
        overflowed_tiles_count_,
        (static_cast<double>(overflowed_tiles_count_) / tg_->tiles().size())
            * 100);
  }

  int arraySize = edge_cong_array.size();
  std::ranges::sort(edge_cong_array, std::greater<double>());

  double avg005RC = 0;
  double avg010RC = 0;
  double avg020RC = 0;
  double avg050RC = 0;

  for (int i = 0; i < arraySize; ++i) {
    if (i < 0.005 * arraySize) {
      avg005RC += edge_cong_array[i];
    }
    if (i < 0.01 * arraySize) {
      avg010RC += edge_cong_array[i];
    }
    if (i < 0.02 * arraySize) {
      avg020RC += edge_cong_array[i];
    }
    if (i < 0.05 * arraySize) {
      avg050RC += edge_cong_array[i];
    }
  }

  avg005RC /= ceil(0.005 * arraySize);
  avg010RC /= ceil(0.010 * arraySize);
  avg020RC /= ceil(0.020 * arraySize);
  avg050RC /= ceil(0.050 * arraySize);
  final_average_rc_
      = (rbVars_.rcK1 * avg005RC + rbVars_.rcK2 * avg010RC
         + rbVars_.rcK3 * avg020RC + rbVars_.rcK4 * avg050RC)
        / (rbVars_.rcK1 + rbVars_.rcK2 + rbVars_.rcK3 + rbVars_.rcK4);

  if (verbose) {
    log_->info(
        GPL, 43, "Average top 0.5% routing congestion: {:.4f}", avg005RC);
    log_->info(
        GPL, 44, "Average top 1.0% routing congestion: {:.4f}", avg010RC);
    log_->info(
        GPL, 45, "Average top 2.0% routing congestion: {:.4f}", avg020RC);
    log_->info(
        GPL, 46, "Average top 5.0% routing congestion: {:.4f}", avg050RC);
    log_->info(GPL,
               47,
               "Routability iteration weighted routing congestion: {:.4f}",
               final_average_rc_);
  }
}

// extract RC values
float RouteBase::getGrtRC() const
{
  double totalRouteOverflowH2 = 0;
  double totalRouteOverflowV2 = 0;
  int overflowTileCnt2 = 0;

  std::vector<double> horEdgeCongArray;
  std::vector<double> verEdgeCongArray;

  odb::dbGCellGrid* gGrid = db_->getChip()->getBlock()->getGCellGrid();
  for (auto& tile : tg_->tiles()) {
    int min_routing_layer, max_routing_layer;
    grouter_->getMinMaxLayer(min_routing_layer, max_routing_layer);
    for (int i = 1; i <= tg_->numRoutingLayers(); i++) {
      odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
      bool isHorizontalLayer
          = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

      // extract the ratio in the same way as inflation ratio cals
      float ratio = getUsageCapacityRatio(
          tile, layer, gGrid, grouter_, rbVars_.ignoreEdgeRatio);
      if (i < min_routing_layer || i > max_routing_layer) {
        ratio = 0.0;
      }
      // escape the case when blockageRatio is too huge
      if (ratio >= 0.0f) {
        if (isHorizontalLayer) {
          totalRouteOverflowH2 += std::fmax(0.0, -1 + ratio);
          horEdgeCongArray.push_back(ratio);
        } else {
          totalRouteOverflowV2 += std::fmax(0.0, -1 + ratio);
          verEdgeCongArray.push_back(ratio);
        }

        if (ratio > 1.0) {
          overflowTileCnt2++;
        }
      }
    }
  }

  log_->info(GPL, 64, "TotalRouteOverflowH2: {:.4f}", totalRouteOverflowH2);
  log_->info(GPL, 65, "TotalRouteOverflowV2: {:.4f}", totalRouteOverflowV2);
  log_->info(GPL, 66, "OverflowTileCnt2: {}", overflowTileCnt2);

  int horArraySize = horEdgeCongArray.size();
  int verArraySize = verEdgeCongArray.size();

  std::ranges::sort(std::ranges::reverse_view(horEdgeCongArray));
  std::ranges::sort(std::ranges::reverse_view(verEdgeCongArray));

  double horAvg005RC = 0;
  double horAvg010RC = 0;
  double horAvg020RC = 0;
  double horAvg050RC = 0;
  for (int i = 0; i < horArraySize; ++i) {
    if (i < 0.005 * horArraySize) {
      horAvg005RC += horEdgeCongArray[i];
    }
    if (i < 0.01 * horArraySize) {
      horAvg010RC += horEdgeCongArray[i];
    }
    if (i < 0.02 * horArraySize) {
      horAvg020RC += horEdgeCongArray[i];
    }
    if (i < 0.05 * horArraySize) {
      horAvg050RC += horEdgeCongArray[i];
    }
  }

  horAvg005RC /= ceil(0.005 * horArraySize);
  horAvg010RC /= ceil(0.010 * horArraySize);
  horAvg020RC /= ceil(0.020 * horArraySize);
  horAvg050RC /= ceil(0.050 * horArraySize);

  double verAvg005RC = 0;
  double verAvg010RC = 0;
  double verAvg020RC = 0;
  double verAvg050RC = 0;
  for (int i = 0; i < verArraySize; ++i) {
    if (i < 0.005 * verArraySize) {
      verAvg005RC += verEdgeCongArray[i];
    }
    if (i < 0.01 * verArraySize) {
      verAvg010RC += verEdgeCongArray[i];
    }
    if (i < 0.02 * verArraySize) {
      verAvg020RC += verEdgeCongArray[i];
    }
    if (i < 0.05 * verArraySize) {
      verAvg050RC += verEdgeCongArray[i];
    }
  }
  verAvg005RC /= ceil(0.005 * verArraySize);
  verAvg010RC /= ceil(0.010 * verArraySize);
  verAvg020RC /= ceil(0.020 * verArraySize);
  verAvg050RC /= ceil(0.050 * verArraySize);

  log_->info(GPL, 67, "0.5%RC: {:.4f}", std::fmax(horAvg005RC, verAvg005RC));
  log_->info(GPL, 68, "1.0%RC: {:.4f}", std::fmax(horAvg010RC, verAvg010RC));
  log_->info(GPL, 69, "2.0%RC: {:.4f}", std::fmax(horAvg020RC, verAvg020RC));
  log_->info(GPL, 70, "5.0%RC: {:.4f}", std::fmax(horAvg050RC, verAvg050RC));

  log_->info(GPL, 71, "0.5rcK: {:.2f}", rbVars_.rcK1);
  log_->info(GPL, 72, "1.0rcK: {:.2f}", rbVars_.rcK2);
  log_->info(GPL, 73, "2.0rcK: {:.2f}", rbVars_.rcK3);
  log_->info(GPL, 74, "5.0rcK: {:.2f}", rbVars_.rcK4);

  float finalRC = (rbVars_.rcK1 * std::fmax(horAvg005RC, verAvg005RC)
                   + rbVars_.rcK2 * std::fmax(horAvg010RC, verAvg010RC)
                   + rbVars_.rcK3 * std::fmax(horAvg020RC, verAvg020RC)
                   + rbVars_.rcK4 * std::fmax(horAvg050RC, verAvg050RC))
                  / (rbVars_.rcK1 + rbVars_.rcK2 + rbVars_.rcK3 + rbVars_.rcK4);

  log_->info(GPL, 75, "Final routing congestion: {}", finalRC);
  return finalRC;
}

void RouteBase::increaseCounter()
{
  revert_count_++;

  log_->info(GPL, 40, "Routability iteration: {}", revert_count_);
}

}  // namespace gpl
