/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "opendb/geom.h"

namespace ppl {

using odb::Point;
using odb::Rect;

class Core
{
 public:
  Core();
  Core(const Rect& boundary,
       const std::vector<int>& min_dst_pins_x,
       const std::vector<int>& min_dst_pins_y,
       const std::vector<int>& init_tracks_x,
       const std::vector<int>& init_tracks_y,
       const std::vector<int>& num_tracks_x,
       const std::vector<int>& num_tracks_y,
       const std::vector<int>& min_area_x,
       const std::vector<int>& min_area_y,
       const std::vector<int>& min_width_x,
       const std::vector<int>& min_width_y,
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
  std::vector<int> getMinDstPinsX() const { return min_dst_pins_x_; }
  std::vector<int> getMinDstPinsY() const { return min_dst_pins_y_; }
  std::vector<int> getInitTracksX() const { return init_tracks_x_; }
  std::vector<int> getInitTracksY() const { return init_tracks_y_; }
  std::vector<int> getNumTracksX() const { return num_tracks_x_; }
  std::vector<int> getNumTracksY() const { return num_tracks_y_; }
  std::vector<int> getMinAreaX() const { return min_area_x_; }
  std::vector<int> getMinAreaY() const { return min_area_y_; }
  std::vector<int> getMinWidthX() const { return min_width_x_; }
  std::vector<int> getMinWidthY() const { return min_width_y_; }
  int getDatabaseUnit() const { return database_unit_; }
  int getPerimeter() const;
  void addPattern(int layer, int x_step, int y_step,
                  int x_ori, int y_ori, int width, int height);

 private:
  Rect boundary_;
  std::vector<int> min_dst_pins_x_;
  std::vector<int> min_dst_pins_y_;
  std::vector<int> init_tracks_x_;
  std::vector<int> init_tracks_y_;
  std::vector<int> num_tracks_x_;
  std::vector<int> num_tracks_y_;
  std::vector<int> min_area_x_;
  std::vector<int> min_area_y_;
  std::vector<int> min_width_x_;
  std::vector<int> min_width_y_;
  int database_unit_;
};

}  // namespace ppl
