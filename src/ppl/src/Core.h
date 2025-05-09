// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

#include "odb/geom.h"

namespace ppl {

using odb::Point;
using odb::Rect;

using LayerToVector = std::map<int, std::vector<int>>;

class Core
{
 public:
  Core();
  Core(const Rect& boundary,
       const LayerToVector& min_dst_pins_x,
       const LayerToVector& min_dst_pins_y,
       const LayerToVector& init_tracks_x,
       const LayerToVector& init_tracks_y,
       const LayerToVector& num_tracks_x,
       const LayerToVector& num_tracks_y,
       const std::map<int, int>& min_area_x,
       const std::map<int, int>& min_area_y,
       const std::map<int, int>& min_width_x,
       const std::map<int, int>& min_width_y,
       const int& database_unit)
      : boundary_(boundary),
        min_dst_pins_x_(min_dst_pins_x),
        min_dst_pins_y_(min_dst_pins_y),
        init_tracks_x_(init_tracks_x),
        init_tracks_y_(init_tracks_y),
        num_tracks_x_(num_tracks_x),
        num_tracks_y_(num_tracks_y),
        min_area_x_(min_area_x),
        min_area_y_(min_area_y),
        min_width_x_(min_width_x),
        min_width_y_(min_width_y),
        database_unit_(database_unit)
  {
  }

  Rect getBoundary() const { return boundary_; }
  const LayerToVector& getMinDstPinsX() const { return min_dst_pins_x_; }
  const LayerToVector& getMinDstPinsY() const { return min_dst_pins_y_; }
  const LayerToVector& getInitTracksX() const { return init_tracks_x_; }
  const LayerToVector& getInitTracksY() const { return init_tracks_y_; }
  const LayerToVector& getNumTracksX() const { return num_tracks_x_; }
  const LayerToVector& getNumTracksY() const { return num_tracks_y_; }
  std::map<int, int> getMinAreaX() const { return min_area_x_; }
  std::map<int, int> getMinAreaY() const { return min_area_y_; }
  std::map<int, int> getMinWidthX() const { return min_width_x_; }
  std::map<int, int> getMinWidthY() const { return min_width_y_; }
  int getDatabaseUnit() const { return database_unit_; }
  int getPerimeter() const;
  odb::Point getMirroredPosition(const odb::Point& position) const;

 private:
  Rect boundary_;
  LayerToVector min_dst_pins_x_;
  LayerToVector min_dst_pins_y_;
  LayerToVector init_tracks_x_;
  LayerToVector init_tracks_y_;
  LayerToVector num_tracks_x_;
  LayerToVector num_tracks_y_;
  std::map<int, int> min_area_x_;
  std::map<int, int> min_area_y_;
  std::map<int, int> min_width_x_;
  std::map<int, int> min_width_y_;
  int database_unit_;
};

}  // namespace ppl
