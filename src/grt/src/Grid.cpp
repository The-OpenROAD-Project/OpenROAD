/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "Grid.h"

#include <complex>

namespace grt {

void Grid::init(const odb::Rect& die_area,
                const int tile_width,
                const int tile_height,
                const int x_grids,
                const int y_grids,
                const bool perfect_regular_x,
                const bool perfect_regular_y,
                const int num_layers,
                const std::vector<int>& spacings,
                const std::vector<int>& min_widths,
                const std::vector<int>& horizontal_capacities,
                const std::vector<int>& vertical_capacities,
                const std::map<int, std::vector<odb::Rect>>& obstructions)
{
  die_area_ = die_area;
  tile_width_ = tile_width;
  tile_height_ = tile_height;
  x_grids_ = x_grids;
  y_grids_ = y_grids;
  perfect_regular_x_ = perfect_regular_x;
  perfect_regular_y_ = perfect_regular_y;
  num_layers_ = num_layers;
  spacings_ = spacings;
  min_widths_ = min_widths;
  horizontal_edges_capacities_ = horizontal_capacities;
  vertical_edges_capacities_ = vertical_capacities;
  obstructions_ = obstructions;
}

void Grid::clear()
{
  spacings_.clear();
  min_widths_.clear();
  horizontal_edges_capacities_.clear();
  vertical_edges_capacities_.clear();
  obstructions_.clear();
}

void Grid::addObstruction(int layer, const odb::Rect& obstruction)
{
  // compute the intersection between obstruction and the die area
  // only when they are overlapping to avoid assert error during
  // intersect() function
  if (die_area_.overlaps(obstruction)) {
    odb::Rect obs_inside_die = die_area_.intersect(obstruction);
    if (!obs_inside_die.isInverted()) {
      obstructions_[layer].push_back(obs_inside_die);
    }
  }
}

odb::Point Grid::getPositionOnGrid(const odb::Point& position)
{
  int x = position.x();
  int y = position.y();

  // Computing x and y center:
  int gcell_id_x = floor((float) ((x - die_area_.xMin()) / tile_width_));
  int gcell_id_y = floor((float) ((y - die_area_.yMin()) / tile_height_));

  if (gcell_id_x >= x_grids_)
    gcell_id_x--;

  if (gcell_id_y >= y_grids_)
    gcell_id_y--;

  int center_x = (gcell_id_x * tile_width_) + (tile_width_ / 2) + die_area_.xMin();
  int center_y
      = (gcell_id_y * tile_height_) + (tile_height_ / 2) + die_area_.yMin();

  return odb::Point(center_x, center_y);
}

std::pair<Grid::TILE, Grid::TILE> Grid::getBlockedTiles(
    const odb::Rect& obstruction,
    odb::Rect& first_tile_bds,
    odb::Rect& last_tile_bds)
{
  std::pair<TILE, TILE> tiles;
  TILE first_tile;
  TILE last_tile;

  odb::Point lower = obstruction.ll();  // lower bound of obstruction
  odb::Point upper = obstruction.ur();  // upper bound of obstruction

  lower
      = getPositionOnGrid(lower);  // translate lower bound of obstruction to
                                   // the center of the tile where it is inside
  upper
      = getPositionOnGrid(upper);  // translate upper bound of obstruction to
                                   // the center of the tile where it is inside

  // Get x and y indices of first blocked tile
  first_tile._x = (lower.x() - (getTileWidth() / 2)) / getTileWidth();
  first_tile._y = (lower.y() - (getTileHeight() / 2)) / getTileHeight();

  // Get x and y indices of last blocked tile
  last_tile._x = (upper.x() - (getTileWidth() / 2)) / getTileWidth();
  last_tile._y = (upper.y() - (getTileHeight() / 2)) / getTileHeight();

  tiles = std::make_pair(first_tile, last_tile);

  odb::Point ll_first_tile = odb::Point(lower.x() - (getTileWidth() / 2),
                                        lower.y() - (getTileHeight() / 2));
  odb::Point ur_first_tile = odb::Point(lower.x() + (getTileWidth() / 2),
                                        lower.y() + (getTileHeight() / 2));

  odb::Point ll_last_tile = odb::Point(upper.x() - (getTileWidth() / 2),
                                       upper.y() - (getTileHeight() / 2));
  odb::Point ur_last_tile = odb::Point(upper.x() + (getTileWidth() / 2),
                                       upper.y() + (getTileHeight() / 2));

  if ((die_area_.xMax() - ur_last_tile.x()) / getTileWidth() < 1) {
    ur_last_tile.setX(die_area_.xMax());
  }
  if ((die_area_.yMax() - ur_last_tile.y()) / getTileHeight() < 1) {
    ur_last_tile.setY(die_area_.yMax());
  }

  first_tile_bds = odb::Rect(ll_first_tile, ur_first_tile);
  last_tile_bds = odb::Rect(ll_last_tile, ur_last_tile);

  return tiles;
}

int Grid::computeTileReduce(const odb::Rect& obs,
                            const odb::Rect& tile,
                            int track_space,
                            bool first,
                            odb::dbTechLayerDir direction)
{
  int reduce = -1;
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    if (obs.xMin() >= tile.xMin() && obs.xMax() <= tile.xMax()) {
      reduce = ceil(std::abs(obs.xMax() - obs.xMin()) / track_space);
    } else if (first) {
      reduce = ceil(std::abs(tile.xMax() - obs.xMin()) / track_space);
    } else {
      reduce = ceil(std::abs(obs.xMax() - tile.xMin()) / track_space);
    }
  } else {
    if (obs.yMin() >= tile.yMin() && obs.yMax() <= tile.yMax()) {
      reduce = ceil(std::abs(obs.yMax() - obs.yMin()) / track_space);
    } else if (first) {
      reduce = ceil(std::abs(tile.yMax() - obs.yMin()) / track_space);
    } else {
      reduce = ceil(std::abs(obs.yMax() - tile.yMin()) / track_space);
    }
  }

  return reduce;
}

odb::Point Grid::getMiddle()
{
  return odb::Point((die_area_.xMin() + (die_area_.dx() / 2.0)),
                    (die_area_.yMin() + (die_area_.dy() / 2.0)));
}

const odb::Rect& Grid::getGridArea() const
{
  return die_area_;
}

}  // namespace grt
