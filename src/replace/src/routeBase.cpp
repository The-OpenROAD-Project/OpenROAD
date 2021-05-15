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

#include "grt/GlobalRouter.h"
#include "nesterovBase.h"
#include "opendb/db.h"
#include "utl/Logger.h"

using grt::GlobalRouter;
using std::make_pair;
using std::pair;
using std::sort;
using std::string;
using std::vector;

using utl::GPL;

namespace gpl {

Tile::Tile()
    : x_(0),
      y_(0),
      lx_(0),
      ly_(0),
      ux_(0),
      uy_(0),
      inflationRatio_(1.0),
      inflatedRatio_(0)
{
}

Tile::Tile(int x, int y, int lx, int ly, int ux, int uy, int layers) : Tile()
{
  x_ = x;
  y_ = y;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

Tile::~Tile()
{
  reset();
}

void Tile::reset()
{
  x_ = y_ = lx_ = ly_ = ux_ = uy_ = 0;
  inflationRatio_ = 1.0;

  inflatedRatio_ = 0;
}

float Tile::inflationRatio() const
{
  return inflationRatio_;
}

float Tile::inflatedRatio() const
{
  return inflatedRatio_;
}

void Tile::setInflationRatio(float val)
{
  inflationRatio_ = val;
}

void Tile::setInflatedRatio(float ratio)
{
  inflatedRatio_ = ratio;
}

TileGrid::TileGrid()
    : lx_(0),
      ly_(0),
      tileCntX_(0),
      tileCntY_(0),
      tileSizeX_(0),
      tileSizeY_(0),
      numRoutingLayers_(0)
{
}

TileGrid::~TileGrid()
{
  reset();
}

void TileGrid::reset()
{
  lx_ = ly_ = 0;
  tileCntX_ = tileCntY_ = 0;
  tileSizeX_ = tileSizeY_ = 0;
  numRoutingLayers_ = 0;

  tileStor_.clear();
  tiles_.clear();

  tileStor_.shrink_to_fit();
  tiles_.shrink_to_fit();
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

void TileGrid::initTiles()
{
  log_->info(GPL, 36, "TileLxLy: {} {}", lx_, ly_);
  log_->info(GPL, 37, "TileSize: {} {}", tileSizeX_, tileSizeY_);
  log_->info(GPL, 38, "TileCnt: {} {}", tileCntX_, tileCntY_);
  log_->info(GPL, 39, "numRoutingLayers: {}", numRoutingLayers_);

  // 2D tile grid structure init
  int x = lx_, y = ly_;
  int idxX = 0, idxY = 0;
  tileStor_.resize(tileCntX_ * tileCntY_);
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

/////////////////////////////////////////////
// RouteBaseVars

RouteBaseVars::RouteBaseVars()
{
  reset();
}

void RouteBaseVars::reset()
{
  inflationRatioCoef = 2.5;
  maxInflationRatio = 2.5;
  maxDensity = 0.90;
  targetRC = 1.25;
  ignoreEdgeRatio = 0.8;
  minInflationRatio = 1.01;
  rcK1 = rcK2 = 1.0;
  rcK3 = rcK4 = 0.0;
  maxBloatIter = 1;
  maxInflationIter = 4;
}

/////////////////////////////////////////////
// RouteBase

RouteBase::RouteBase()
    : rbVars_(),
      db_(nullptr),
      grouter_(nullptr),
      nb_(nullptr),
      log_(nullptr),
      inflatedAreaDelta_(0),
      bloatIterCnt_(0),
      inflationIterCnt_(0),
      numCall_(0),
      minRc_(1e30),
      minRcTargetDensity_(0),
      minRcViolatedCnt_(0)
{
}

RouteBase::RouteBase(RouteBaseVars rbVars,
                     odb::dbDatabase* db,
                     grt::GlobalRouter* grouter,
                     std::shared_ptr<NesterovBase> nb,
                     utl::Logger* log)
    : RouteBase()
{
  rbVars_ = rbVars;
  db_ = db;
  grouter_ = grouter;
  nb_ = nb;
  log_ = log;

  init();
}

RouteBase::~RouteBase()
{
}

void RouteBase::reset()
{
  rbVars_.reset();
  db_ = nullptr;
  nb_ = nullptr;
  log_ = nullptr;

  bloatIterCnt_ = inflationIterCnt_ = 0;
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

  grouter_->clear();
  tg_.reset();
}

void RouteBase::init()
{
  // tg_ init
  std::unique_ptr<TileGrid> tg(new TileGrid());
  tg_ = std::move(tg);

  tg_->setLogger(log_);
  minRcCellSize_.resize(nb_->gCells().size(), std::make_pair(0, 0));
}

void RouteBase::getGlobalRouterResult()
{
  // update gCells' location to DB for GR
  nb_->updateDbGCells();

  // these two options must be on
  grouter_->setAllowOverflow(true);
  grouter_->setOverflowIterations(0);

  grouter_->run();

  updateRoute();
}

int64_t RouteBase::inflatedAreaDelta() const
{
  return inflatedAreaDelta_;
}

int RouteBase::numCall() const
{
  return numCall_;
}

int RouteBase::bloatIterCnt() const
{
  return bloatIterCnt_;
}

int RouteBase::inflationIterCnt() const
{
  return inflationIterCnt_;
}

static float getUsageCapacityRatio(Tile* tile,
                                   odb::dbTechLayer* layer,
                                   odb::dbGCellGrid* gGrid,
                                   float ignoreEdgeRatio)
{
  unsigned int capH = 0, capV = 0, capU = 0;
  unsigned int useH = 0, useV = 0, useU = 0;
  unsigned int blockH = 0, blockV = 0, blockU = 0;
  gGrid->getCapacity(layer, tile->x(), tile->y(), capH, capV, capU);
  gGrid->getUsage(layer, tile->x(), tile->y(), useH, useV, useU);
  gGrid->getBlockage(layer, tile->x(), tile->y(), blockH, blockV, blockU);

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
  unsigned int curCap = (isHorizontal) ? capH : capV;
  unsigned int curUse = (isHorizontal) ? useH : useV;
  unsigned int blockage = (isHorizontal) ? blockH : blockV;

  // escape tile ratio cals when capacity = 0
  if (curCap == 0) {
    return -1 * FLT_MAX;
  }

  // ignore if blockage is too huge in current tile
  float blockageRatio = static_cast<float>(blockage) / curCap;
  if (blockageRatio >= ignoreEdgeRatio) {
    return -1 * FLT_MAX;
  }

  // return usage (used routing track + blockage) / total capacity
  return static_cast<float>(curUse) / curCap;
}

// fill
//
// TileGrids'
// lx_ ly_
// tileCntX_ tileCntY_
// tileSizeX_ tileSizeY_
//
void RouteBase::updateRoute()
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
  tg_->initTiles();

  for (int i = 1; i <= numLayers; i++) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(i);
    bool isHorizontalLayer
        = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

    for (auto& tile : tg_->tiles()) {
      // Check left and down tile
      // and set the minimum usage/cap vals for
      // TileGrid setup.

      // first extract current tiles' usage
      float ratio
          = getUsageCapacityRatio(tile, layer, gGrid, rbVars_.ignoreEdgeRatio);

      // if horizontal layer (i.e., vertical edges)
      // should consider LEFT tile's RIGHT edge == current 'tile's LEFT edge
      // (current 'ratio' points to RIGHT edges usage)
      if (isHorizontalLayer && tile->x() >= 1) {
        Tile* leftTile
            = tg_->tiles()[tile->y() * tg_->tileCntX() + tile->x() - 1];
        float leftRatio = getUsageCapacityRatio(
            leftTile, layer, gGrid, rbVars_.ignoreEdgeRatio);
        ratio = fmax(leftRatio, ratio);
      }

      // if vertical layer (i.e., horizontal edges)
      // should consider DOWN tile's UP edge == current 'tile's DOWN edge
      // (current 'ratio' points to UP edges usage)
      if (!isHorizontalLayer && tile->y() >= 1) {
        Tile* downTile
            = tg_->tiles()[(tile->y() - 1) * tg_->tileCntX() + tile->x()];
        float downRatio = getUsageCapacityRatio(
            downTile, layer, gGrid, rbVars_.ignoreEdgeRatio);
        ratio = fmax(downRatio, ratio);
      }

      ratio = fmax(ratio, 0.0f);

      // update inflation Ratio
      if (ratio >= rbVars_.minInflationRatio) {
        float inflationRatio = pow(ratio, rbVars_.inflationRatioCoef);
        inflationRatio = fmin(inflationRatio, rbVars_.maxInflationRatio);
        tile->setInflationRatio(inflationRatio);
      }
    }
  }

  // debug print
  for (auto& tile : tg_->tiles()) {
    if (tile->inflationRatio() > 1.0) {
      debugPrint(log_,
                 GPL,
                 "replace",
                 5,
                 "updateInflationRatio: xy: {} {}",
                 tile->x(),
                 tile->y());
      debugPrint(log_,
                 GPL,
                 "replace",
                 5,
                 "updateInflationRatio: minxy: {} {}",
                 tile->lx(),
                 tile->ly());
      debugPrint(log_,
                 GPL,
                 "replace",
                 5,
                 "updateInflationRatio: maxxy: {} {}",
                 tile->ux(),
                 tile->uy());
      debugPrint(log_,
                 GPL,
                 "replace",
                 5,
                 "updateInflationRatio: calcInflationRatio: {}",
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

  getGlobalRouterResult();

  // no need routing if RC is lower than targetRC val
  float curRc = getRC();

  if (curRc < rbVars_.targetRC) {
    resetRoutabilityResources();
    return make_pair(false, false);
  }

  //
  // saving solutions when minRc happen.
  // I hope to get lower Rc gradually as RD goes on
  //
  if (minRc_ > curRc) {
    minRc_ = curRc;
    minRcTargetDensity_ = nb_->targetDensity();
    minRcViolatedCnt_ = 0;

    // save cell size info
    for (auto& gCell : nb_->gCells()) {
      if (!gCell->isStdInstance()) {
        continue;
      }

      minRcCellSize_[&gCell - &nb_->gCells()[0]]
          = std::make_pair(gCell->dx(), gCell->dy());
    }
  } else {
    minRcViolatedCnt_++;
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
  for (auto& gCell : nb_->gCells()) {
    // only care about "standard cell"
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idxX = (gCell->dCx() - tg_->lx()) / tg_->tileSizeX();
    int idxY = (gCell->dCy() - tg_->ly()) / tg_->tileSizeY();

    Tile* tile = tg_->tiles()[idxY * tg_->tileCntX() + idxX];

    // Don't care when inflRatio <= 1
    if (tile->inflatedRatio() <= 1.0) {
      continue;
    }

    int64_t prevCellArea
        = static_cast<int64_t>(gCell->dx()) * static_cast<int64_t>(gCell->dy());

    // bloat
    gCell->setSize(
        static_cast<int>(round(gCell->dx() * sqrt(tile->inflatedRatio()))),
        static_cast<int>(round(gCell->dy() * sqrt(tile->inflatedRatio()))));

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
  if (inflatedAreaDelta_
      > targetInflationDeltaAreaRatio
            * (nb_->whiteSpaceArea()
               - (nb_->nesterovInstsArea() + nb_->totalFillerArea()))) {
    // TODO dynamic inflation procedure?
  }

  log_->info(GPL, 45, "InflatedAreaDelta: {}", inflatedAreaDelta_);
  log_->info(GPL, 46, "TargetDensity: {}", nb_->targetDensity());

  int64_t totalGCellArea
      = inflatedAreaDelta_ + nb_->nesterovInstsArea() + nb_->totalFillerArea();

  float prevDensity = nb_->targetDensity();

  // newly set Density
  nb_->setTargetDensity(static_cast<float>(totalGCellArea)
                        / static_cast<float>(nb_->whiteSpaceArea()));

  //
  // max density detection or,
  // rc not improvement detection -- (not improved the RC values 3 times in a
  // row)
  //
  if (nb_->targetDensity() > rbVars_.maxDensity || minRcViolatedCnt_ >= 3) {
    log_->report("Revert Routability Procedure");
    log_->info(GPL, 47, "SavedMinRC: {}", minRc_);
    log_->info(GPL, 48, "SavedTargetDensity: {}", minRcTargetDensity_);

    nb_->setTargetDensity(minRcTargetDensity_);

    revertGCellSizeToMinRc();

    nb_->updateDensitySize();
    resetRoutabilityResources();

    return make_pair(false, true);
  }

  log_->info(GPL, 49, "WhiteSpaceArea: {}", nb_->whiteSpaceArea());
  log_->info(GPL, 50, "NesterovInstsArea: {}", nb_->nesterovInstsArea());
  log_->info(GPL, 51, "TotalFillerArea: {}", nb_->totalFillerArea());
  log_->info(GPL,
             52,
             "TotalGCellsArea: {}",
             nb_->nesterovInstsArea() + nb_->totalFillerArea());
  log_->info(
      GPL,
      53,
      "ExpectedTotalGCellsArea: {}",
      inflatedAreaDelta_ + nb_->nesterovInstsArea() + nb_->totalFillerArea());

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
  nb_->updateAreas();

  log_->info(GPL, 54, "NewTargetDensity: {}", nb_->targetDensity());
  log_->info(GPL, 55, "NewWhiteSpaceArea: {}", nb_->whiteSpaceArea());
  log_->info(GPL, 56, "MovableArea: {}", nb_->movableArea());
  log_->info(GPL, 57, "NewNesterovInstsArea: {}", nb_->nesterovInstsArea());
  log_->info(GPL, 58, "NewTotalFillerArea: {}", nb_->totalFillerArea());
  log_->info(GPL,
             59,
             "NewTotalGCellsArea: {}",
             nb_->nesterovInstsArea() + nb_->totalFillerArea());

  // update densitySizes for all gCell
  nb_->updateDensitySize();

  // reset
  resetRoutabilityResources();

  return make_pair(true, true);
}

void RouteBase::revertGCellSizeToMinRc()
{
  // revert back the gcell sizes
  for (auto& gCell : nb_->gCells()) {
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idx = &gCell - &nb_->gCells()[0];

    gCell->setSize(minRcCellSize_[idx].first, minRcCellSize_[idx].second);
  }
}

// extract RC values
float RouteBase::getRC() const
{
  double totalRouteOverflowH2 = 0;
  double totalRouteOverflowV2 = 0;
  int overflowTileCnt2 = 0;

  std::vector<double> horEdgeCongArray;
  std::vector<double> verEdgeCongArray;

  odb::dbGCellGrid* gGrid = db_->getChip()->getBlock()->getGCellGrid();
  for (auto& tile : tg_->tiles()) {
    for (int i = 1; i <= tg_->numRoutingLayers(); i++) {
      odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
      bool isHorizontalLayer
          = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

      // extract the ratio in the same way as inflation ratio cals
      float ratio
          = getUsageCapacityRatio(tile, layer, gGrid, rbVars_.ignoreEdgeRatio);

      // escape the case when blockageRatio is too huge
      if (ratio >= 0.0f) {
        if (isHorizontalLayer) {
          totalRouteOverflowH2 += fmax(0.0, -1 + ratio);
          horEdgeCongArray.push_back(ratio);
        } else {
          totalRouteOverflowV2 += fmax(0.0, -1 + ratio);
          verEdgeCongArray.push_back(ratio);
        }

        if (ratio > 1.0) {
          overflowTileCnt2++;
        }
      }
    }
  }

  log_->info(GPL, 63, "TotalRouteOverflowH2: {}", totalRouteOverflowH2);
  log_->info(GPL, 64, "TotalRouteOverflowV2: {}", totalRouteOverflowV2);
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

  log_->info(GPL, 66, "0.5%RC: {}", fmax(horAvg005RC, verAvg005RC));
  log_->info(GPL, 67, "1.0%RC: {}", fmax(horAvg010RC, verAvg010RC));
  log_->info(GPL, 68, "2.0%RC: {}", fmax(horAvg020RC, verAvg020RC));
  log_->info(GPL, 69, "5.0%RC: {}", fmax(horAvg050RC, verAvg050RC));

  log_->info(GPL, 70, "0.5rcK: {}", rbVars_.rcK1);
  log_->info(GPL, 71, "1.0rcK: {}", rbVars_.rcK2);
  log_->info(GPL, 72, "2.0rcK: {}", rbVars_.rcK3);
  log_->info(GPL, 73, "5.0rcK: {}", rbVars_.rcK4);

  float finalRC = (rbVars_.rcK1 * fmax(horAvg005RC, verAvg005RC)
                   + rbVars_.rcK2 * fmax(horAvg010RC, verAvg010RC)
                   + rbVars_.rcK3 * fmax(horAvg020RC, verAvg020RC)
                   + rbVars_.rcK4 * fmax(horAvg050RC, verAvg050RC))
                  / (rbVars_.rcK1 + rbVars_.rcK2 + rbVars_.rcK3 + rbVars_.rcK4);

  log_->info(GPL, 74, "FinalRC: {}", finalRC);
  return finalRC;
}

void RouteBase::increaseCounter()
{
  numCall_++;
  inflationIterCnt_++;
  if (inflationIterCnt_ > rbVars_.maxInflationIter) {
    inflationIterCnt_ = 0;
    bloatIterCnt_++;
  }

  log_->info(GPL,
             75,
             "Routability numCall: {} inflationIterCnt: {} bloatIterCnt: {}",
             numCall_,
             inflationIterCnt_,
             bloatIterCnt_);
}

}  // namespace gpl
