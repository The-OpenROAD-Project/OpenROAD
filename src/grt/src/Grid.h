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

#include <boost/icl/interval.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

#include "RoutingTracks.h"
#include "odb/db.h"

using boost::icl::interval;

namespace grt {

class Grid
{
 public:
  Grid() = default;
  ~Grid() = default;

  void init(const odb::Rect& die_area,
            const int tile_size,
            const int x_grids,
            const int y_grids,
            const bool perfect_regular_x,
            const bool perfectR_rgular_y,
            const int num_layers);

  void clear();

  int getXMin() const { return die_area_.xMin(); }
  int getYMin() const { return die_area_.yMin(); }

  void setXMin(int x) { die_area_.set_xlo(x); }
  void setYMin(int y) { die_area_.set_ylo(y); }

  int getXMax() const { return die_area_.xMax(); }
  int getYMax() const { return die_area_.yMax(); }

  int getTileSize() const { return tile_size_; }

  int getXGrids() const { return x_grids_; }
  int getYGrids() const { return y_grids_; }

  void setXGrids(int x_grids) { x_grids_ = x_grids; }
  void setYGrids(int y_grids) { y_grids_ = y_grids; }

  bool isPerfectRegularX() const { return perfect_regular_x_; }
  bool isPerfectRegularY() const { return perfect_regular_y_; }

  int getNumLayers() const { return num_layers_; }

  void setPitchesInTile(const int pitches_in_tile)
  {
    pitches_in_tile_ = pitches_in_tile;
  }

  int getPitchesInTile() const { return pitches_in_tile_; }

  const std::vector<int>& getTrackPitches() const { return track_pitches_; }

  void addTrackPitch(int value, int layer) { track_pitches_[layer] = value; }

  const std::vector<int>& getHorizontalEdgesCapacities()
  {
    return horizontal_edges_capacities_;
  };

  const std::vector<int>& getVerticalEdgesCapacities()
  {
    return vertical_edges_capacities_;
  };

  void setHorizontalCapacity(int capacity, int layer)
  {
    horizontal_edges_capacities_[layer] = capacity;
  }
  void setVerticalCapacity(int capacity, int layer)
  {
    vertical_edges_capacities_[layer] = capacity;
  }

  odb::Point getPositionOnGrid(const odb::Point& position);

  void getBlockedTiles(const odb::Rect& obstruction,
                       odb::Rect& first_tile_bds,
                       odb::Rect& last_tile_bds,
                       odb::Point& first_tile,
                       odb::Point& last_tile);

  int computeTileReduce(const odb::Rect& obs,
                        const odb::Rect& tile,
                        double track_space,
                        bool first,
                        odb::dbTechLayerDir direction);

  interval<int>::type computeTileReduceInterval(
      const odb::Rect& obs,
      const odb::Rect& tile,
      int track_space,
      bool first,
      const odb::dbTechLayerDir& direction,
      int layer_cap,
      bool is_macro);

  odb::Point getMiddle();
  const odb::Rect& getGridArea() const;

 private:
  odb::Rect die_area_;
  int tile_size_;
  int x_grids_;
  int y_grids_;
  bool perfect_regular_x_;
  bool perfect_regular_y_;
  int num_layers_;
  int pitches_in_tile_ = 15;
  std::vector<int> track_pitches_;
  std::vector<int> horizontal_edges_capacities_;
  std::vector<int> vertical_edges_capacities_;
};

}  // namespace grt
