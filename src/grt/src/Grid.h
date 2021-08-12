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

#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <vector>

#include "RoutingTracks.h"
#include "opendb/db.h"

namespace grt {

class Grid
{
 private:
  long lower_left_x_;
  long lower_left_y_;
  long upper_right_x_;
  long upper_right_y_;
  long tile_width_;
  long tile_height_;
  int x_grids_;
  int y_grids_;
  bool perfect_regular_x_;
  bool perfect_regular_y_;
  int num_layers_;
  int pitches_in_tile_ = 15;
  std::vector<int> spacings_;
  std::vector<int> min_widths_;
  std::vector<int> horizontal_edges_capacities_;
  std::vector<int> vertical_edges_capacities_;
  std::map<int, std::vector<odb::Rect>> obstructions_;

 public:
  Grid() = default;
  ~Grid() = default;

  void init(const long lower_left_x,
            const long lower_left_y,
            const long upper_right_x,
            const long upper_right_y,
            const long tile_width,
            const long tile_height,
            const int x_grids,
            const int y_grids,
            const bool perfect_regular_x,
            const bool perfectR_rgular_y,
            const int num_layers,
            const std::vector<int>& spacings,
            const std::vector<int>& min_widths,
            const std::vector<int>& horizontalCapacities,
            const std::vector<int>& verticalCapacities,
            const std::map<int, std::vector<odb::Rect>>& obstructions);

  typedef struct
  {
    int _x;
    int _y;
  } TILE;

  void clear();

  long getLowerLeftX() const { return lower_left_x_; }
  long getLowerLeftY() const { return lower_left_y_; }

  void setLowerLeftX(long x) { lower_left_x_ = x; }
  void setLowerLeftY(long y) { lower_left_y_ = y; }

  long getUpperRightX() const { return upper_right_x_; }
  long getUpperRightY() const { return upper_right_y_; }

  long getTileWidth() const { return tile_width_; }
  long getTileHeight() const { return tile_height_; }

  int getXGrids() const { return x_grids_; }
  int getYGrids() const { return y_grids_; }

  bool isPerfectRegularX() const { return perfect_regular_x_; }
  bool isPerfectRegularY() const { return perfect_regular_y_; }

  int getNumLayers() const { return num_layers_; }

  void setPitchesInTile(const int pitches_in_tile)
  {
    pitches_in_tile_ = pitches_in_tile;
  }
  int getPitchesInTile() const { return pitches_in_tile_; }

  const std::vector<int>& getSpacings() const { return spacings_; }
  const std::vector<int>& getMinWidths() const { return min_widths_; }

  void addSpacing(int value, int layer) { spacings_[layer] = value; }
  void addMinWidth(int value, int layer) { min_widths_[layer] = value; }

  const std::vector<int>& getHorizontalEdgesCapacities()
  {
    return horizontal_edges_capacities_;
  };
  const std::vector<int>& getVerticalEdgesCapacities()
  {
    return vertical_edges_capacities_;
  };

  void addHorizontalCapacity(int value, int layer)
  {
    horizontal_edges_capacities_[layer] = value;
  }
  void addVerticalCapacity(int value, int layer)
  {
    vertical_edges_capacities_[layer] = value;
  }

  void updateHorizontalEdgesCapacities(int layer, int reduction)
  {
    horizontal_edges_capacities_[layer] = reduction;
  };
  void updateVerticalEdgesCapacities(int layer, int reduction)
  {
    vertical_edges_capacities_[layer] = reduction;
  };

  const std::map<int, std::vector<odb::Rect>>& getAllObstructions() const
  {
    return obstructions_;
  }
  void addObstruction(int layer, const odb::Rect& obstruction)
  {
    obstructions_[layer].push_back(obstruction);
  }

  odb::Point getPositionOnGrid(const odb::Point& position);

  std::pair<TILE, TILE> getBlockedTiles(const odb::Rect& obstruction,
                                        odb::Rect& first_tile_bds,
                                        odb::Rect& last_tile_bds);

  int computeTileReduce(const odb::Rect& obs,
                        const odb::Rect& tile,
                        int track_space,
                        bool first,
                        odb::dbTechLayerDir direction);

  odb::Point getMiddle();
  odb::Rect getGridArea() const;
};

}  // namespace grt
