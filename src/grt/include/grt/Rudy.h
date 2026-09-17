// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <cassert>
#include <optional>
#include <set>
#include <utility>

#include "odb/PtrSetMap.h"
// #define _CRTDBG_MAP_ALLOC

#pragma once

#include <vector>

#include "odb/db.h"
#include "odb/geom.h"

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

    void addDemand(float demand);
    void setBlockage(float blockage);
    void clear();

    // Wire demand the nets deposit in this tile, as a percentage of the
    // tile's nominal routing capacity.
    float getDemand() const { return demand_; }
    // Fraction of the tile's nominal capacity taken away by obstructions,
    // blockages and layer adjustments. 1.0 means nothing is left to route on.
    // This depends only on the floorplan, not on where the cells are.
    float getBlockage() const { return blockage_; }
    // Demand and blockage added together, as a percentage. Reads as "how full
    // is this tile" for display purposes. Consumers that act on congestion
    // want the two terms separately: only the demand term responds to
    // placement.
    float getRudy() const { return demand_ + blockage_ * 100.0f; }

   private:
    odb::Rect rect_;
    float demand_ = 0;
    float blockage_ = 0;
  };

  explicit Rudy(odb::dbBlock* block, grt::GlobalRouter* grouter);

  /**
   * \pre we need to call this function after `setGridConfig` and
   * `setWireWidth`.
   * */
  void calculateRudy(std::optional<odb::PtrSet<odb::dbNet>*> selection
                     = std::nullopt);

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

  /**
   * How much demand each signal net deposits into the selected tiles, using
   * the same deposit model as calculateRudy(). `selected` is indexed
   * x * tile_cnt_y + y; nets that deposit nothing are left out.
   *
   * A tile's demand comes from every net whose terminal bounding box covers
   * it, which is mostly nets with terminals somewhere else entirely. This is
   * the inverse relation: given the tiles that are congested, which nets put
   * the wire there.
   * */
  std::vector<std::pair<odb::dbNet*, float>> getNetDemandInTiles(
      const std::vector<bool>& selected) const;

 private:
  /**
   * \pre This function should be called after `setGridConfig`
   * */
  void makeGrid();
  void getResourceReductions();
  Tile& getEditableTile(int x, int y) { return grid_.at(x).at(y); }
  void processNet(odb::dbNet* net);
  void processIntersectionSignalNet(odb::Rect net_rect);

  // Call `visit(x, y, demand)` for each tile the net's terminal bounding box
  // covers, with the demand the net deposits there. The single definition of
  // the deposit model; both the accumulated grid and the per-net query go
  // through it.
  template <typename Visitor>
  void visitNetTiles(const odb::Rect& net_rect, Visitor&& visit) const
  {
    if (net_rect.isInverted()) {
      return;
    }
    const auto net_area = net_rect.area();
    if (net_area == 0) {
      // TODO: handle nets with 0 area from getTermBBox()
      return;
    }
    const auto hpwl = static_cast<float>(net_rect.dx() + net_rect.dy());
    const auto net_congestion = (hpwl * wire_width_) / net_area;

    const int min_x_index
        = std::max(0, (net_rect.xMin() - grid_block_.xMin()) / tile_size_);
    const int max_x_index = std::min(
        tile_cnt_x_ - 1, (net_rect.xMax() - grid_block_.xMin()) / tile_size_);
    const int min_y_index
        = std::max(0, (net_rect.yMin() - grid_block_.yMin()) / tile_size_);
    const int max_y_index = std::min(
        tile_cnt_y_ - 1, (net_rect.yMax() - grid_block_.yMin()) / tile_size_);

    for (int x = min_x_index; x <= max_x_index; ++x) {
      for (int y = min_y_index; y <= max_y_index; ++y) {
        const auto tile_box = grid_[x][y].getRect();
        if (!net_rect.overlaps(tile_box)) {
          continue;
        }
        const auto intersect_area = net_rect.intersect(tile_box).area();
        const auto tile_net_box_ratio = static_cast<float>(intersect_area)
                                        / static_cast<float>(tile_box.area());
        visit(x, y, net_congestion * tile_net_box_ratio * 100);
      }
    }
  }

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
