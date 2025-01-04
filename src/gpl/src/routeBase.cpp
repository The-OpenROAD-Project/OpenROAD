///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "routeBase.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>

#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "utl/Logger.h"

using odb::dbBlock;
using std::make_pair;
using std::pair;
using std::sort;
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
  log_->info(GPL,
             36,
             "{:9} ( {:4} {:4} ) ( {:4} {:4} ) DBU",
             "TileBBox:",
             lx_,
             ly_,
             tileSizeX_,
             tileSizeY_);
  log_->info(GPL, 38, "{:9} {:6} {:4}", "TileCnt:", tileCntX_, tileCntY_);
  if (!use_rudy) {
    log_->info(GPL, 39, "numRoutingLayers: {}", numRoutingLayers_);
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
  log_->info(GPL, 40, "NumTiles: {}", tiles_.size());
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

RouteBaseVars::RouteBaseVars()
{
  reset();
}

void RouteBaseVars::reset()
{
  inflationRatioCoef = 5;
  maxInflationRatio = 8;
  maxDensity = 0.90;
  targetRC = 1.01;
  ignoreEdgeRatio = 0.8;
  minInflationRatio = 1.01;
  rcK1 = rcK2 = 1.0;
  rcK3 = rcK4 = 0.0;
  maxInflationIter = 4;
  useRudy = true;
}

/////////////////////////////////////////////
// RouteBase

RouteBase::RouteBase() = default;

RouteBase::RouteBase(RouteBaseVars rbVars,
                     odb::dbDatabase* db,
                     grt::GlobalRouter* grouter,
                     std::shared_ptr<NesterovBaseCommon> nbc,
                     std::vector<std::shared_ptr<NesterovBase>> nbVec,
                     utl::Logger* log)
{
  rbVars_ = rbVars;
  db_ = db;
  grouter_ = grouter;
  nbc_ = std::move(nbc);
  log_ = log;
  nbVec_ = std::move(nbVec);
  init();
}

RouteBase::~RouteBase() = default;

void RouteBase::reset()
{
  rbVars_.reset();
  db_ = nullptr;
  nbc_ = nullptr;
  log_ = nullptr;

  numCall_ = 0;

  minRc_ = 1e30;
  minRcTargetDensity_ = 0;
  minRcViolatedCnt_ = 0;

  minRcCellSize_.clear();
  minRcCellSize_.shrink_to_fit();

  resetRoutabilityResources();
}

void RouteBase::resetRoutabilityResources()
{
  inflatedAreaDelta_ = 0;

  if (!rbVars_.useRudy) {
    grouter_->clear();
  }
  tg_.reset();
}

void RouteBase::init()
{
  // tg_ init
  std::unique_ptr<TileGrid> tg(new TileGrid());
  tg_ = std::move(tg);

  tg_->setLogger(log_);
  minRcCellSize_.resize(nbc_->gCells().size(), std::make_pair(0, 0));
}

void RouteBase::getRudyResult()
{
  nbc_->updateDbGCells();
  updateRudyRoute();
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

int64_t RouteBase::inflatedAreaDelta() const
{
  return inflatedAreaDelta_;
}

int RouteBase::numCall() const
{
  return numCall_;
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

void RouteBase::updateRudyRoute()
{
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
std::pair<bool, bool> RouteBase::routability()
{
  increaseCounter();

  // create Tile Grid
  std::unique_ptr<TileGrid> tg(new TileGrid());
  tg_ = std::move(tg);
  tg_->setLogger(log_);

  float curRc;
  if (rbVars_.useRudy) {
    getRudyResult();
    curRc = getRudyRC();
  } else {
    getGrtResult();
    curRc = getGrtRC();
  }

  if (curRc < rbVars_.targetRC) {
    log_->info(GPL,
               77,
               "FinalRC lower than targetRC({:.4f}), routability not needed.",
               rbVars_.targetRC);
    resetRoutabilityResources();
    return std::make_pair(false, false);
  }

  //
  // saving solutions when minRc happen.
  // I hope to get lower Rc gradually as RD goes on
  //
  if ((minRc_ - curRc) > 0.001) {
    log_->info(
        GPL, 78, "FinalRC lower than minRC ({:.4f}), min RC updated.", minRc_);
    minRc_ = curRc;
    minRcTargetDensity_ = nbVec_[0]->targetDensity();
    minRcViolatedCnt_ = 0;

    // save cell size info
    for (auto& gCell : nbc_->gCells()) {
      if (!gCell->isStdInstance()) {
        continue;
      }

      minRcCellSize_[&gCell - nbc_->gCells().data()]
          = std::make_pair(gCell->dx(), gCell->dy());
    }
  } else {
    minRcViolatedCnt_++;
    log_->info(GPL,
               79,
               "MinRC ({}) violation occurred, total count: {}.",
               minRc_,
               minRcViolatedCnt_);
  }

  // set inflated ratio
  for (auto& tile : tg_->tiles()) {
    if (tile->inflationRatio() > 1) {
      tile->setInflatedRatio(tile->inflationRatio());
    } else {
      tile->setInflatedRatio(1.0);
    }
  }

  inflatedAreaDelta_ = 0;

  // run bloating and get inflatedAreaDelta_
  for (auto& gCell : nbc_->gCells()) {
    // only care about "standard cell"
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idxX = std::min((gCell->dCx() - tg_->lx()) / tg_->tileSizeX(),
                        tg_->tileCntX() - 1);
    int idxY = std::min((gCell->dCy() - tg_->ly()) / tg_->tileSizeY(),
                        tg_->tileCntY() - 1);

    size_t index = idxY * tg_->tileCntX() + idxX;
    if (index >= tg_->tiles().size()) {
      continue;
    }
    Tile* tile = tg_->tiles()[index];

    // Don't care when inflRatio <= 1
    if (tile->inflatedRatio() <= 1.0) {
      continue;
    }

    int64_t prevCellArea
        = static_cast<int64_t>(gCell->dx()) * static_cast<int64_t>(gCell->dy());

    // bloat
    gCell->setSize(static_cast<int>(std::round(
                       gCell->dx() * std::sqrt(tile->inflatedRatio()))),
                   static_cast<int>(std::round(
                       gCell->dy() * std::sqrt(tile->inflatedRatio()))));

    int64_t newCellArea
        = static_cast<int64_t>(gCell->dx()) * static_cast<int64_t>(gCell->dy());

    // deltaArea is equal to area * deltaRatio
    // both of original and density size will be changed
    inflatedAreaDelta_ += newCellArea - prevCellArea;

    //    inflatedAreaDelta_
    //      = static_cast<int64_t>(round(
    //        static_cast<int64_t>(gCell->dx())
    //        * static_cast<int64_t>(gCell->dy())
    //        * (tile->inflatedRatio() - 1.0)));
  }

  // target ratio
  float targetInflationDeltaAreaRatio
      = 1.0 / static_cast<float>(rbVars_.maxInflationIter);

  // TODO: will be implemented
  if (inflatedAreaDelta_ > targetInflationDeltaAreaRatio
                               * (nbVec_[0]->whiteSpaceArea()
                                  - (nbVec_[0]->nesterovInstsArea()
                                     + nbVec_[0]->totalFillerArea()))) {
    // TODO dynamic inflation procedure?
  }

  dbBlock* block = db_->getChip()->getBlock();
  float inflated_area_delta_microns
      = block->dbuAreaToMicrons(inflatedAreaDelta_);
  float inflated_area_delta_percentage = (static_cast<float>(inflatedAreaDelta_)
                                          / nbVec_[0]->nesterovInstsArea())
                                         * 100.0f;
  log_->info(GPL,
             45,
             "{:20} {:10.3f} um^2 ({:+.2f}%)",
             "InflatedAreaDelta:",
             inflated_area_delta_microns,
             inflated_area_delta_percentage);
  log_->info(
      GPL, 46, "{:20} {:10.3f}", "TargetDensity:", nbVec_[0]->targetDensity());

  int64_t totalGCellArea = inflatedAreaDelta_ + nbVec_[0]->nesterovInstsArea()
                           + nbVec_[0]->totalFillerArea();

  // newly set Density
  nbVec_[0]->setTargetDensity(
      static_cast<float>(totalGCellArea)
      / static_cast<float>(nbVec_[0]->whiteSpaceArea()));

  //
  // max density detection or,
  // rc not improvement detection -- (not improved the RC values 3 times in a
  // row)
  //
  if (nbVec_[0]->targetDensity() > rbVars_.maxDensity
      || minRcViolatedCnt_ >= 3) {
    log_->report(
        "Revert Routability Procedure. Target density higher than max, or "
        "minRC max violations.");
    log_->info(GPL, 80, "minRcViolatedCnt: {}", minRcViolatedCnt_);
    log_->info(GPL, 47, "SavedMinRC: {:.4f}", minRc_);
    log_->info(GPL, 48, "SavedTargetDensity: {:.4f}", minRcTargetDensity_);

    nbVec_[0]->setTargetDensity(minRcTargetDensity_);

    revertGCellSizeToMinRc();

    nbVec_[0]->updateDensitySize();
    resetRoutabilityResources();

    return std::make_pair(false, true);
  }

  log_->info(GPL,
             49,
             "{:20} {:10.3f} um^2",
             "WhiteSpaceArea:",
             block->dbuAreaToMicrons(nbVec_[0]->whiteSpaceArea()));
  log_->info(GPL,
             50,
             "{:20} {:10.3f} um^2",
             "NesterovInstsArea:",
             block->dbuAreaToMicrons(nbVec_[0]->nesterovInstsArea()));
  log_->info(GPL,
             51,
             "{:20} {:10.3f} um^2",
             "TotalFillerArea:",
             block->dbuAreaToMicrons(nbVec_[0]->totalFillerArea()));
  log_->info(GPL,
             52,
             "{:20} {:10.3f} um^2",
             "TotalGCellsArea:",
             block->dbuAreaToMicrons(nbVec_[0]->nesterovInstsArea()
                                     + nbVec_[0]->totalFillerArea()));
  log_->info(GPL,
             53,
             "{:20} {:10.3f} um^2",
             "ExpectedGCellsArea:",
             block->dbuAreaToMicrons(inflatedAreaDelta_
                                     + nbVec_[0]->nesterovInstsArea()
                                     + nbVec_[0]->totalFillerArea()));

  // cut filler cells accordingly
  //  if( nb_->totalFillerArea() > inflatedAreaDelta_ ) {
  //    nb_->cutFillerCells( nb_->totalFillerArea() - inflatedAreaDelta_ );
  //  }
  // routability-driven cannot solve this problem with the given density...
  // return false
  //  else {
  //    return false;
  //  }

  // updateArea
  nbVec_[0]->updateAreas();

  log_->info(GPL,
             54,
             "{:20} {:10.3f}",
             "NewTargetDensity:",
             nbVec_[0]->targetDensity());
  log_->info(GPL,
             55,
             "{:20} {:10.3f} um^2",
             "NewWhiteSpaceArea:",
             block->dbuAreaToMicrons(nbVec_[0]->whiteSpaceArea()));
  log_->info(GPL,
             56,
             "{:20} {:10.3f} um^2",
             "MovableArea:",
             block->dbuAreaToMicrons(nbVec_[0]->movableArea()));
  log_->info(GPL,
             57,
             "{:20} {:10.3f} um^2",
             "NewNesterovInstArea:",
             block->dbuAreaToMicrons(nbVec_[0]->nesterovInstsArea()));
  log_->info(GPL,
             58,
             "{:20} {:10.3f} um^2",
             "NewTotalFillerArea:",
             block->dbuAreaToMicrons(nbVec_[0]->totalFillerArea()));
  log_->info(GPL,
             59,
             "{:20} {:10.3f} um^2",
             "NewTotalGCellsArea:",
             block->dbuAreaToMicrons(nbVec_[0]->nesterovInstsArea()
                                     + nbVec_[0]->totalFillerArea()));

  // update densitySizes for all gCell
  nbVec_[0]->updateDensitySize();

  // reset
  resetRoutabilityResources();

  return std::make_pair(true, true);
}

void RouteBase::revertGCellSizeToMinRc()
{
  // revert back the gcell sizes
  for (auto& gCell : nbc_->gCells()) {
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idx = &gCell - nbc_->gCells().data();

    gCell->setSize(minRcCellSize_[idx].first, minRcCellSize_[idx].second);
  }
}

float RouteBase::getRudyRC() const
{
  grt::Rudy* rudy = grouter_->getRudy();
  double totalRouteOverflow = 0;
  int overflowTileCnt = 0;
  std::vector<double> edgeCongArray;

  for (auto& tile : tg_->tiles()) {
    float ratio = rudy->getTile(tile->x(), tile->y()).getRudy() / 100.0;
    // Escape the case when blockage ratio is too huge
    if (ratio >= 0.0f) {
      totalRouteOverflow += std::fmax(0.0, -1 + ratio);
      edgeCongArray.push_back(ratio);

      if (ratio > 1.0) {
        overflowTileCnt++;
      }
    }
  }

  log_->info(GPL, 81, "TotalRouteOverflow: {:.4f}", totalRouteOverflow);
  log_->info(GPL, 82, "OverflowTileCnt: {}", overflowTileCnt);

  int arraySize = edgeCongArray.size();
  std::sort(edgeCongArray.rbegin(), edgeCongArray.rend());

  double avg005RC = 0;
  double avg010RC = 0;
  double avg020RC = 0;
  double avg050RC = 0;

  for (int i = 0; i < arraySize; ++i) {
    if (i < 0.005 * arraySize) {
      avg005RC += edgeCongArray[i];
    }
    if (i < 0.01 * arraySize) {
      avg010RC += edgeCongArray[i];
    }
    if (i < 0.02 * arraySize) {
      avg020RC += edgeCongArray[i];
    }
    if (i < 0.05 * arraySize) {
      avg050RC += edgeCongArray[i];
    }
  }

  avg005RC /= ceil(0.005 * arraySize);
  avg010RC /= ceil(0.010 * arraySize);
  avg020RC /= ceil(0.020 * arraySize);
  avg050RC /= ceil(0.050 * arraySize);

  log_->info(GPL, 83, "0.5%RC: {:.4f}", avg005RC);
  log_->info(GPL, 84, "1.0%RC: {:.4f}", avg010RC);
  log_->info(GPL, 85, "2.0%RC: {:.4f}", avg020RC);
  log_->info(GPL, 86, "5.0%RC: {:.4f}", avg050RC);

  float finalRC = (rbVars_.rcK1 * avg005RC + rbVars_.rcK2 * avg010RC
                   + rbVars_.rcK3 * avg020RC + rbVars_.rcK4 * avg050RC)
                  / (rbVars_.rcK1 + rbVars_.rcK2 + rbVars_.rcK3 + rbVars_.rcK4);

  log_->info(GPL, 87, "FinalRC: {}", finalRC);
  return finalRC;
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

  log_->info(GPL, 63, "TotalRouteOverflowH2: {:.4f}", totalRouteOverflowH2);
  log_->info(GPL, 64, "TotalRouteOverflowV2: {:.4f}", totalRouteOverflowV2);
  log_->info(GPL, 65, "OverflowTileCnt2: {}", overflowTileCnt2);

  int horArraySize = horEdgeCongArray.size();
  int verArraySize = verEdgeCongArray.size();

  std::sort(horEdgeCongArray.rbegin(), horEdgeCongArray.rend());
  std::sort(verEdgeCongArray.rbegin(), verEdgeCongArray.rend());

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

  log_->info(GPL, 66, "0.5%RC: {:.4f}", std::fmax(horAvg005RC, verAvg005RC));
  log_->info(GPL, 67, "1.0%RC: {:.4f}", std::fmax(horAvg010RC, verAvg010RC));
  log_->info(GPL, 68, "2.0%RC: {:.4f}", std::fmax(horAvg020RC, verAvg020RC));
  log_->info(GPL, 69, "5.0%RC: {:.4f}", std::fmax(horAvg050RC, verAvg050RC));

  log_->info(GPL, 70, "0.5rcK: {:.2f}", rbVars_.rcK1);
  log_->info(GPL, 71, "1.0rcK: {:.2f}", rbVars_.rcK2);
  log_->info(GPL, 72, "2.0rcK: {:.2f}", rbVars_.rcK3);
  log_->info(GPL, 73, "5.0rcK: {:.2f}", rbVars_.rcK4);

  float finalRC = (rbVars_.rcK1 * std::fmax(horAvg005RC, verAvg005RC)
                   + rbVars_.rcK2 * std::fmax(horAvg010RC, verAvg010RC)
                   + rbVars_.rcK3 * std::fmax(horAvg020RC, verAvg020RC)
                   + rbVars_.rcK4 * std::fmax(horAvg050RC, verAvg050RC))
                  / (rbVars_.rcK1 + rbVars_.rcK2 + rbVars_.rcK3 + rbVars_.rcK4);

  log_->info(GPL, 74, "FinalRC: {}", finalRC);
  return finalRC;
}

void RouteBase::increaseCounter()
{
  numCall_++;

  log_->info(GPL, 75, "Routability iteration: {}", numCall_);
}

}  // namespace gpl
