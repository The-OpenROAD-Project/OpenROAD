///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

#include "Rudy.h"

#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace grt {

Rudy::Rudy(odb::dbBlock* block) : block_(block)
{
  gridBlock_ = block_->getDieArea();
  if (gridBlock_.area() == 0) {
    return;
  }
  // TODO: Match the wire width with the paper definition
  wireWidth_ = block_->getTech()->findRoutingLayer(1)->getWidth();

  odb::dbTechLayer* tech_layer = block_->getTech()->findRoutingLayer(3);
  odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
  if (track_grid == nullptr) {
    return;
  }
  int track_spacing, track_init, num_tracks;
  track_grid->getAverageTrackSpacing(track_spacing, track_init, num_tracks);
  int upper_rightX = gridBlock_.xMax();
  int upper_rightY = gridBlock_.yMax();
  int tile_size = pitches_in_tile_ * track_spacing;
  int x_grids = upper_rightX / tile_size;
  int y_grids = upper_rightY / tile_size;
  setGridConfig(gridBlock_, x_grids, y_grids);
}

void Rudy::setGridConfig(odb::Rect block, int tileCntX, int tileCntY)
{
  gridBlock_ = block;
  tileCntX_ = tileCntX;
  tileCntY_ = tileCntY;
  makeGrid();
}

void Rudy::makeGrid()
{
  const int block_width = gridBlock_.dx();
  const int block_height = gridBlock_.dy();
  const int gridLx = gridBlock_.xMin();
  const int gridLy = gridBlock_.yMin();
  const int tile_width = block_width / tileCntX_;
  const int tile_height = block_height / tileCntY_;

  grid_.resize(tileCntX_);
  int curX = gridLx;
  for (auto& gridColumn : grid_) {
    gridColumn.resize(tileCntY_);
    int curY = gridLy;
    for (auto& grid : gridColumn) {
      grid.setRect(curX, curY, curX + tile_width, curY + tile_height);
      curY += tile_height;
    }
    curX += tile_width;
  }
}

void Rudy::calculateRUDY()
{
  // Clear previous computation
  for (auto& gridColumn : grid_) {
    for (auto& tile : gridColumn) {
      tile.clearRUDY();
    }
  }

  // refer: https://ieeexplore.ieee.org/document/4211973
  const int tile_width = gridBlock_.dx() / tileCntX_;
  const int tile_height = gridBlock_.dy() / tileCntY_;

  for (auto net : block_->getNets()) {
    if (!net->getSigType().isSupply()) {
      const auto net_rect = net->getTermBBox();
      processIntersectionSignalNet(net_rect, tile_width, tile_height);
    } else {
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            continue;
          }
          odb::Rect wire_rect = s->getBox();
          processIntersectionGenericObstruction(
              wire_rect, tile_width, tile_height, 1);
        }
      }
    }
  }

  for (odb::dbInst* instance : block_->getInsts()) {
    odb::dbMaster* master = instance->getMaster();
    if (master->isBlock()) {
      processMacroObstruction(master, instance);
    }
  }
}

void Rudy::processMacroObstruction(odb::dbMaster* macro,
                                             odb::dbInst* instance)
{
  const int tile_width = gridBlock_.dx() / tileCntX_;
  const int tile_height = gridBlock_.dy() / tileCntY_;
  for (odb::dbBox* obstr_box : macro->getObstructions()) {
    const odb::Point origin = instance->getOrigin();
    odb::dbTransform transform(instance->getOrient(), origin);
    odb::Rect macro_obstruction = obstr_box->getBox();
    transform.apply(macro_obstruction);
    const auto obstr_area = macro_obstruction.area();
    if (obstr_area == 0) {
      continue;
    }
    processIntersectionGenericObstruction(
        macro_obstruction, tile_width, tile_height, 2);
  }
}

void Rudy::processIntersectionGenericObstruction(
    odb::Rect obstruction_rect,
    const int tile_width,
    const int tile_height,
    const int nets_per_tile)
{
  // Calculate the intersection range
  const int minXIndex
      = std::max(0, (obstruction_rect.xMin() - gridBlock_.xMin()) / tile_width);
  const int maxXIndex
      = std::min(tileCntX_ - 1,
                 (obstruction_rect.xMax() - gridBlock_.xMin()) / tile_width);
  const int minYIndex = std::max(
      0, (obstruction_rect.yMin() - gridBlock_.yMin()) / tile_height);
  const int maxYIndex
      = std::min(tileCntY_ - 1,
                 (obstruction_rect.yMax() - gridBlock_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = minXIndex; x <= maxXIndex; ++x) {
    for (int y = minYIndex; y <= maxYIndex; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tileBox = tile.getRect();
      if (obstruction_rect.overlaps(tileBox)) {
        const auto hpwl = static_cast<float>(tileBox.dx() + tileBox.dy());
        const auto wireArea = hpwl * wireWidth_;
        const auto tileArea = tileBox.area();
        const auto obstr_congestion = wireArea / tileArea;
        const auto intersectArea = obstruction_rect.intersect(tileBox).area();

        const auto tileObstrBoxRatio
            = static_cast<float>(intersectArea) / static_cast<float>(tileArea);
        const auto rudy
            = obstr_congestion * tileObstrBoxRatio * 100 * nets_per_tile;
        tile.addRUDY(rudy);
      }
    }
  }
}

void Rudy::processIntersectionSignalNet(const odb::Rect net_rect,
                                                  const int tile_width,
                                                  const int tile_height)
{
  const auto netArea = net_rect.area();
  if (netArea == 0) {
    // TODO: handle nets with 0 area from getTermBBox()
    return;
  }
  const auto hpwl = static_cast<float>(net_rect.dx() + net_rect.dy());
  const auto wireArea = hpwl * wireWidth_;
  const auto netCongestion = wireArea / netArea;

  // Calculate the intersection range
  const int minXIndex
      = std::max(0, (net_rect.xMin() - gridBlock_.xMin()) / tile_width);
  const int maxXIndex = std::min(
      tileCntX_ - 1, (net_rect.xMax() - gridBlock_.xMin()) / tile_width);
  const int minYIndex
      = std::max(0, (net_rect.yMin() - gridBlock_.yMin()) / tile_height);
  const int maxYIndex = std::min(
      tileCntY_ - 1, (net_rect.yMax() - gridBlock_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = minXIndex; x <= maxXIndex; ++x) {
    for (int y = minYIndex; y <= maxYIndex; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tileBox = tile.getRect();
      if (net_rect.overlaps(tileBox)) {
        const auto intersectArea = net_rect.intersect(tileBox).area();
        const auto tileArea = tileBox.area();
        const auto tileNetBoxRatio
            = static_cast<float>(intersectArea) / static_cast<float>(tileArea);
        const auto rudy = netCongestion * tileNetBoxRatio * 100;
        tile.addRUDY(rudy);
      }
    }
  }
}

std::pair<int, int> Rudy::getGridSize() const
{
  if (grid_.empty()) {
    return {0, 0};
  }
  return {grid_.size(), grid_.at(0).size()};
}

void Rudy::Tile::setRect(int lx, int ly, int ux, int uy)
{
  rect_ = odb::Rect(lx, ly, ux, uy);
}

void Rudy::Tile::addRUDY(float rudy)
{
  rudy_ += rudy;
}

} // end namespace
