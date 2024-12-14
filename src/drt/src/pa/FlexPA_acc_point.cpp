/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <omp.h>
// #include <chrono>
// #include <fstream>
// #include <iostream>
// #include <sstream>

#include "FlexPA.h"
#include "FlexPA_graphics.h"
// #include "db/infra/frTime.h"
// #include "distributed/PinAccessJobDescription.h"
// #include "distributed/frArchive.h"
// #include "dst/Distributed.h"
// #include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
// #include "serialization.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

/**
 *
 * @details This follows the Tao of PAO paper cost structure.
 * On track and half track are the preffered access points,
 * this function is responsible for generating them.
 * It iterates over every track coord in the range [low, high]
 * and inserts one of its coordinates on the coords map.
 * if use_nearby_grid is true it changes the access point cost to it.
 *
 * TODO:
 * This function doesn't seem to be getting the best access point.
 * it iterates through every track contained between low and high
 * and takes the first one (closest to low) not the best one (lowest cost).
 * note that std::map.insert() will not override and entry.
 * it should prioritize OnGrid access points
 */
void FlexPA::genAPOnTrack(
    std::map<frCoord, frAccessPointEnum>& coords,
    const std::map<frCoord, frAccessPointEnum>& track_coords,
    const frCoord low,
    const frCoord high,
    const bool use_nearby_grid)
{
  for (auto it = track_coords.lower_bound(low); it != track_coords.end();
       it++) {
    auto& [coord, cost] = *it;
    if (coord > high) {
      break;
    }
    if (use_nearby_grid) {
      coords.insert({coord, frAccessPointEnum::NearbyGrid});
    } else {
      coords.insert(*it);
    }
  }
}

// will not generate center for wider edge
/**
 * @details This follows the Tao of PAO paper cost structure.
 * First it iterates through the range [low, high] to check if there are
 * at least 3 possible OnTrack access points as those take priority.
 * If false it created and access points in the middle point between [low, high]
 */

void FlexPA::genAPCentered(std::map<frCoord, frAccessPointEnum>& coords,
                           const frLayerNum layer_num,
                           const frCoord low,
                           const frCoord high)
{
  // if touching two tracks, then no center??
  int candidates_on_grid = 0;
  for (auto it = coords.lower_bound(low); it != coords.end(); it++) {
    auto& [coordinate, cost] = *it;
    if (coordinate > high) {
      break;
    }
    if (cost == frAccessPointEnum::OnGrid) {
      candidates_on_grid++;
    }
  }
  if (candidates_on_grid >= 3) {
    return;
  }

  // If there are less than 3 coords OnGrid will create a Centered Access Point
  frCoord manu_grid = getDesign()->getTech()->getManufacturingGrid();
  frCoord coord = (low + high) / 2 / manu_grid * manu_grid;

  if (coords.find(coord) == coords.end()) {
    coords.insert(std::make_pair(coord, frAccessPointEnum::Center));
  } else {
    coords[coord] = std::min(coords[coord], frAccessPointEnum::Center);
  }
}

/**
 * @details This follows the Tao of PAO paper cost structure.
 * Enclosed Boundary APs satisfy via-in-pin requirement.
 * This is the worst access point adressed in the paper
 */

void FlexPA::genAPEnclosedBoundary(std::map<frCoord, frAccessPointEnum>& coords,
                                   const gtl::rectangle_data<frCoord>& rect,
                                   const frLayerNum layer_num,
                                   const bool is_curr_layer_horz)
{
  const auto rect_width = gtl::delta(rect, gtl::HORIZONTAL);
  const auto rect_height = gtl::delta(rect, gtl::VERTICAL);
  const int max_num_via_trial = 2;
  if (layer_num + 1 > getDesign()->getTech()->getTopLayerNum()) {
    return;
  }
  // hardcode first two single vias
  std::vector<frViaDef*> via_defs;
  int cnt = 0;
  for (auto& [tup, via] : layer_num_to_via_defs_[layer_num + 1][1]) {
    via_defs.push_back(via);
    cnt++;
    if (cnt >= max_num_via_trial) {
      break;
    }
  }
  for (auto& via_def : via_defs) {
    frVia via(via_def);
    const Rect box = via.getLayer1BBox();
    const auto via_width = box.dx();
    const auto via_height = box.dy();
    if (via_width > rect_width || via_height > rect_height) {
      continue;
    }
    const int coord_top = is_curr_layer_horz ? gtl::yh(rect) - box.yMax()
                                             : gtl::xh(rect) - box.xMax();
    const int coord_low = is_curr_layer_horz ? gtl::yl(rect) - box.yMin()
                                             : gtl::xl(rect) - box.xMin();
    for (const int coord : {coord_top, coord_low}) {
      if (coords.find(coord) == coords.end()) {
        coords.insert(std::make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
    }
  }
}

void FlexPA::genAPCosted(
    const frAccessPointEnum cost,
    std::map<frCoord, frAccessPointEnum>& coords,
    const std::map<frCoord, frAccessPointEnum>& track_coords,
    const frLayerNum base_layer_num,
    const frLayerNum layer_num,
    const gtl::rectangle_data<frCoord>& rect,
    const bool is_curr_layer_horz,
    const int offset)
{
  auto layer = getDesign()->getTech()->getLayer(layer_num);
  const auto min_width_layer = layer->getMinWidth();
  const int rect_min = is_curr_layer_horz ? gtl::yl(rect) : gtl::xl(rect);
  const int rect_max = is_curr_layer_horz ? gtl::yh(rect) : gtl::xh(rect);

  switch (cost) {
    case (frAccessPointEnum::OnGrid):
      genAPOnTrack(coords, track_coords, rect_min + offset, rect_max - offset);
      break;

      // frAccessPointEnum::Halfgrid not defined

    case (frAccessPointEnum::Center):
      genAPCentered(
          coords, base_layer_num, rect_min + offset, rect_max - offset);
      break;

    case (frAccessPointEnum::EncOpt):
      genAPEnclosedBoundary(coords, rect, base_layer_num, is_curr_layer_horz);
      break;

    case (frAccessPointEnum::NearbyGrid):
      genAPOnTrack(
          coords, track_coords, rect_min - min_width_layer, rect_min, true);
      genAPOnTrack(
          coords, track_coords, rect_max, rect_max + min_width_layer, true);
      break;

    default:
      logger_->error(DRT, 257, "Invalid frAccessPointEnum type");
  }
}

}