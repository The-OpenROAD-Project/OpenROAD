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

#include "grt/GlobalRouter.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace grt {

Rudy::Rudy(odb::dbBlock* block) : block_(block)
{
  grid_block_ = block_->getDieArea();
  if (grid_block_.area() == 0) {
    return;
  }
  // TODO: Match the wire width with the paper definition
  wire_width_ = block_->getTech()->findRoutingLayer(1)->getWidth();

  int x_grids, y_grids;
  grouter_->getGridSize(x_grids, y_grids);
  const int tile_size = grouter_->getGridTileSize();
  setGridConfig(grid_block_, x_grids, y_grids);
  makeGrid(tile_size);
}

void Rudy::setGridConfig(odb::Rect block, int tile_cnt_x, int tile_cnt_y)
{
  grid_block_ = block;
  tile_cnt_x_ = tile_cnt_x;
  tile_cnt_y_ = tile_cnt_y;
}

void Rudy::makeGrid(const int tile_size)
{
  const int grid_lx = grid_block_.xMin();
  const int grid_ly = grid_block_.yMin();

  grid_.resize(tile_cnt_x_);
  int cur_x = grid_lx;
  for (int x = 0; x < grid_.size(); x++) {
    grid_[x].resize(tile_cnt_y_);
    int cur_y = grid_ly;
    for (int y = 0; y < grid_[x].size(); y++) {
      Tile& grid = grid_[x][y];
      grid.setRect(cur_x, cur_y, cur_x + tile_size, cur_y + tile_size);
      cur_y += tile_size;
    }
    cur_x += tile_size;
  }
}

void Rudy::calculateRudy()
{
  // Clear previous computation
  for (auto& grid_column : grid_) {
    for (auto& tile : grid_column) {
      tile.clearRudy();
    }
  }

  // refer: https://ieeexplore.ieee.org/document/4211973
  const int tile_width = grid_block_.dx() / tile_cnt_x_;
  const int tile_height = grid_block_.dy() / tile_cnt_y_;

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
  const int tile_width = grid_block_.dx() / tile_cnt_x_;
  const int tile_height = grid_block_.dy() / tile_cnt_y_;
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
  const int min_x_index
      = std::max(0, (obstruction_rect.xMin() - grid_block_.xMin()) / tile_width);
  const int max_x_index
      = std::min(tile_cnt_x_ - 1,
                 (obstruction_rect.xMax() - grid_block_.xMin()) / tile_width);
  const int min_y_index = std::max(
      0, (obstruction_rect.yMin() - grid_block_.yMin()) / tile_height);
  const int max_y_index
      = std::min(tile_cnt_y_ - 1,
                 (obstruction_rect.yMax() - grid_block_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = min_x_index; x <= max_x_index; ++x) {
    for (int y = min_y_index; y <= max_y_index; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tile_box = tile.getRect();
      if (obstruction_rect.overlaps(tile_box)) {
        const auto hpwl = static_cast<float>(tile_box.dx() + tile_box.dy());
        const auto wire_area = hpwl * wire_width_;
        const auto tile_area = tile_box.area();
        const auto obstr_congestion = wire_area / tile_area;
        const auto intersect_area = obstruction_rect.intersect(tile_box).area();

        const auto tile_obstr_box_ratio
            = static_cast<float>(intersect_area) / static_cast<float>(tile_area);
        const auto rudy
            = obstr_congestion * tile_obstr_box_ratio * 100 * nets_per_tile;
        tile.addRudy(rudy);
      }
    }
  }
}

void Rudy::processIntersectionSignalNet(const odb::Rect net_rect,
                                                  const int tile_width,
                                                  const int tile_height)
{
  const auto net_area = net_rect.area();
  if (net_area == 0) {
    // TODO: handle nets with 0 area from getTermBBox()
    return;
  }
  const auto hpwl = static_cast<float>(net_rect.dx() + net_rect.dy());
  const auto wire_area = hpwl * wire_width_;
  const auto net_congestion = wire_area / net_area;

  // Calculate the intersection range
  const int min_x_index
      = std::max(0, (net_rect.xMin() - grid_block_.xMin()) / tile_width);
  const int max_x_index = std::min(
      tile_cnt_x_ - 1, (net_rect.xMax() - grid_block_.xMin()) / tile_width);
  const int min_y_index
      = std::max(0, (net_rect.yMin() - grid_block_.yMin()) / tile_height);
  const int max_y_index = std::min(
      tile_cnt_y_ - 1, (net_rect.yMax() - grid_block_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = min_x_index; x <= max_x_index; ++x) {
    for (int y = min_y_index; y <= max_y_index; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tile_box = tile.getRect();
      if (net_rect.overlaps(tile_box)) {
        const auto intersect_area = net_rect.intersect(tile_box).area();
        const auto tile_area = tile_box.area();
        const auto tile_net_box_ratio
            = static_cast<float>(intersect_area) / static_cast<float>(tile_area);
        const auto rudy = net_congestion * tile_net_box_ratio * 100;
        tile.addRudy(rudy);
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

void Rudy::Tile::addRudy(float rudy)
{
  rudy_ += rudy;
}

} // end namespace
