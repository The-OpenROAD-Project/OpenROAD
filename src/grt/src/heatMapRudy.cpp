//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Precision Innovations Inc.
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "heatMapRudy.h"

#include "odb/db.h"

namespace grt {

RUDYDataSource::RUDYDataSource(utl::Logger* logger,
                               grt::GlobalRouter* grouter,
                               odb::dbDatabase* db)
    : GlobalRoutingDataSource(logger,
                              "Estimated Congestion (RUDY)",
                              "RUDY",
                              "RUDY")
{
  grouter_ = grouter;
  db_ = db;
  rudy_ = nullptr;
}

void RUDYDataSource::combineMapData(bool base_has_value,
                                    double& base,
                                    const double new_data,
                                    const double data_area,
                                    const double intersection_area,
                                    const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

void RUDYDataSource::populateXYGrid()
{
  if (getBlock() == nullptr) {
    gui::GlobalRoutingDataSource::populateXYGrid();
    return;
  }

  try {
    rudy_ = grouter_->getRudy();
  } catch (const std::runtime_error& e) {
    gui::GlobalRoutingDataSource::populateXYGrid();
    return;
  }
  int tile_size = rudy_->getTileSize();

  const odb::Rect& bounds = getBounds();
  const int x_grid = std::floor(bounds.dx() / static_cast<double>(tile_size));
  const int y_grid = std::floor(bounds.dy() / static_cast<double>(tile_size));

  std::vector<int> x_grid_set, y_grid_set;
  for (int x = 0; x < x_grid; x++) {
    const int xMin = bounds.xMin() + x * tile_size;
    const int xMax = std::min(xMin + tile_size, bounds.xMax());
    if (x == 0) {
      x_grid_set.push_back(xMin);
    }
    if (x == x_grid - 1) {
      x_grid_set.push_back(bounds.xMax());
    } else {
      x_grid_set.push_back(xMax);
    }
  }
  for (int y = 0; y < y_grid; y++) {
    const int yMin = bounds.yMin() + y * tile_size;
    const int yMax = std::min(yMin + tile_size, bounds.yMax());
    if (y == 0) {
      y_grid_set.push_back(yMin);
    }
    if (y == y_grid - 1) {
      y_grid_set.push_back(bounds.yMax());
    } else {
      y_grid_set.push_back(yMax);
    }
  }

  setXYMapGrid(x_grid_set, y_grid_set);
}

bool RUDYDataSource::populateMap()
{
  if (!getBlock()) {
    return false;
  }

  if (rudy_ == nullptr) {
    return false;
  }

  for (odb::dbInst* inst : getBlock()->getInsts()) {
    if (!inst->isPlaced()) {
      getLogger()->warn(
          utl::GRT, 120, "Instance {} is not placed.", inst->getName());
      return false;
    }
  }

  const auto& [x_grid_size, y_grid_size] = rudy_->getGridSize();
  if (x_grid_size == 0 || y_grid_size == 0) {
    return false;
  }

  rudy_->calculateRudy();

  for (int x = 0; x < x_grid_size; ++x) {
    for (int y = 0; y < y_grid_size; ++y) {
      const auto& tile = rudy_->getTile(x, y);
      const auto& box = tile.getRect();
      const double value = tile.getRudy();
      addToMap(box, value);
    }
  }
  return true;
}

void RUDYDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void RUDYDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void RUDYDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  destroyMap();
}

void RUDYDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbITermPostDisconnect(odb::dbITerm*, odb::dbNet*)
{
  destroyMap();
}

void RUDYDataSource::inDbITermPostConnect(odb::dbITerm*)
{
  destroyMap();
}

void RUDYDataSource::inDbBTermPostConnect(odb::dbBTerm*)
{
  destroyMap();
}

void RUDYDataSource::inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*)
{
  destroyMap();
}

}  // namespace grt
