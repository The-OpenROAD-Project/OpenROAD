// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "grt/Rudy.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>

#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace grt {

Rudy::Rudy(odb::dbBlock* block, grt::GlobalRouter* grouter)
    : block_(block), grouter_(grouter)
{
  grid_block_ = block_->getDieArea();
  if (grid_block_.area() == 0) {
    return;
  }

  if (!grouter_->isInitialized()) {
    int min_layer, max_layer;
    grouter_->setDbBlock(block);
    grouter_->getMinMaxLayer(min_layer, max_layer);
    // Designs with invalid pin placement can still be evaluated by RUDY.
    grouter_->initFastRoute(min_layer, max_layer, false);
  }

  // The wire width is the harmonic average pitch divided by the number of
  // routing layers.
  double pitch_terms = 0;
  const int min_routing_layer = grouter->getMinRoutingLayer();
  const int max_routing_layer = grouter->getMaxRoutingLayer();
  const auto tech = block_->getTech();
  for (int layer_idx = min_routing_layer; layer_idx <= max_routing_layer;
       ++layer_idx) {
    const auto layer = tech->findRoutingLayer(layer_idx);
    int pitch = layer->getPitch();
    if (pitch == 0) {
      pitch = layer->getWidth() + layer->getSpacing();
    }
    pitch_terms += 1.0 / pitch;
  }

  if (pitch_terms != 0) {
    wire_width_
        = (1 / pitch_terms) * 1.25;  // = harm. mean / num_routing_layers
  }

  int x_grids, y_grids;
  grouter_->getGridSize(x_grids, y_grids);
  tile_size_ = grouter_->getGridTileSize();
  setGridConfig(grid_block_, x_grids, y_grids);
  makeGrid();
}

void Rudy::setGridConfig(odb::Rect block, int tile_cnt_x, int tile_cnt_y)
{
  grid_block_ = block;
  tile_cnt_x_ = tile_cnt_x;
  tile_cnt_y_ = tile_cnt_y;
}

void Rudy::makeGrid()
{
  const int grid_lx = grid_block_.xMin();
  const int grid_ly = grid_block_.yMin();

  odb::Point upper_die_bounds(grid_block_.xMax(), grid_block_.yMax());
  odb::Point upper_grid_bounds(tile_cnt_x_ * tile_size_,
                               tile_cnt_y_ * tile_size_);
  int x_extra = upper_die_bounds.x() - upper_grid_bounds.x();
  int y_extra = upper_die_bounds.y() - upper_grid_bounds.y();

  grid_.resize(tile_cnt_x_);
  int cur_x = grid_lx;
  for (int x = 0; x < grid_.size(); x++) {
    grid_[x].resize(tile_cnt_y_);
    int cur_y = grid_ly;
    for (int y = 0; y < grid_[x].size(); y++) {
      Tile& grid = grid_[x][y];
      int x_ext = x == grid_.size() - 1 ? x_extra : 0;
      int y_ext = y == grid_[x].size() - 1 ? y_extra : 0;
      grid.setRect(
          cur_x, cur_y, cur_x + tile_size_ + x_ext, cur_y + tile_size_ + y_ext);
      cur_y += tile_size_;
    }
    cur_x += tile_size_;
  }
}

void Rudy::getResourceReductions()
{
  CapacityReductionData cap_usage_data;
  grouter_->getCapacityReductionData(cap_usage_data);
  for (int x = 0; x < grid_.size(); x++) {
    for (int y = 0; y < grid_[x].size(); y++) {
      Tile& tile = getEditableTile(x, y);
      uint8_t tile_cap = cap_usage_data[x][y].capacity;
      float tile_reduction = cap_usage_data[x][y].reduction;
      float cap_usage_data = tile_reduction / tile_cap;
      tile.addRudy(cap_usage_data * 100);
    }
  }
}

void Rudy::calculateRudy(std::optional<std::set<odb::dbNet*>*> selection)
{
  // Clear previous computation
  for (auto& grid_column : grid_) {
    for (auto& tile : grid_column) {
      tile.clearRudy();
    }
  }

  getResourceReductions();

  if (selection.has_value()) {
    for (auto net : *selection.value()) {
      processNet(net);
    }
  } else {
    for (auto net : block_->getNets()) {
      processNet(net);
    }
  }
}

void Rudy::processNet(odb::dbNet* net)
{
  // refer: https://ieeexplore.ieee.org/document/4211973
  if (!net->getSigType().isSupply()) {
    const auto net_rect = net->getTermBBox();
    processIntersectionSignalNet(net_rect);
  }
}

void Rudy::processIntersectionSignalNet(const odb::Rect net_rect)
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
      = std::max(0, (net_rect.xMin() - grid_block_.xMin()) / tile_size_);
  const int max_x_index = std::min(
      tile_cnt_x_ - 1, (net_rect.xMax() - grid_block_.xMin()) / tile_size_);
  const int min_y_index
      = std::max(0, (net_rect.yMin() - grid_block_.yMin()) / tile_size_);
  const int max_y_index = std::min(
      tile_cnt_y_ - 1, (net_rect.yMax() - grid_block_.yMin()) / tile_size_);

  // Iterate over the tiles in the calculated range
  for (int x = min_x_index; x <= max_x_index; ++x) {
    for (int y = min_y_index; y <= max_y_index; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tile_box = tile.getRect();
      if (net_rect.overlaps(tile_box)) {
        const auto intersect_area = net_rect.intersect(tile_box).area();
        const auto tile_area = tile_box.area();
        const auto tile_net_box_ratio = static_cast<float>(intersect_area)
                                        / static_cast<float>(tile_area);
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

}  // namespace grt
