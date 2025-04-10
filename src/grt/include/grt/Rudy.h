// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <cassert>
#include <utility>
// #define _CRTDBG_MAP_ALLOC

#pragma once

#include <vector>

#include "odb/db.h"

namespace grt {

class GlobalRouter;

class Rudy
{
 public:
  class Tile
  {
   public:
    odb::Rect getRect() const { return rect_; }
    void setRect(int lx, int ly, int ux, int uy);
    void addRudy(float rudy);
    float getRudy() const { return rudy_; }
    void clearRudy() { rudy_ = 0.0; }

   private:
    odb::Rect rect_;
    float rudy_ = 0;
  };

  explicit Rudy(odb::dbBlock* block, grt::GlobalRouter* grouter);

  /**
   * \pre we need to call this function after `setGridConfig` and
   * `setWireWidth`.
   * */
  void calculateRudy();

  /**
   * Set the grid area and grid numbers.
   * Default value will be the die area of block and (40, 40), respectively.
   * */
  void setGridConfig(odb::Rect block, int tile_cnt_x, int tile_cnt_y);

  /**
   * Set the wire length for calculate Rudy.
   * If the layer which name is metal1 and it has getWidth value, then this
   * function will not applied, but it will apply that information.
   * */
  void setWireWidth(int wire_width) { wire_width_ = wire_width; }

  const Tile& getTile(int x, int y) const { return grid_.at(x).at(y); }
  std::pair<int, int> getGridSize() const;
  int getTileSize() const { return tile_size_; }

 private:
  /**
   * \pre This function should be called after `setGridConfig`
   * */
  void makeGrid();
  void getResourceReductions();
  Tile& getEditableTile(int x, int y) { return grid_.at(x).at(y); }
  void processIntersectionSignalNet(odb::Rect net_rect);

  odb::dbBlock* block_;
  odb::Rect grid_block_;
  GlobalRouter* grouter_;
  int tile_cnt_x_ = 40;
  int tile_cnt_y_ = 40;
  int wire_width_ = 100;
  int tile_size_ = 0;
  std::vector<std::vector<Tile>> grid_;
};

}  // namespace grt
