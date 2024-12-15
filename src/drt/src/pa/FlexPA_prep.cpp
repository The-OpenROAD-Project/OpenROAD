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

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "serialization.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

bool FlexPA::enclosesOnTrackPlanarAccess(
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layer_num)
{
  frCoord low, high;
  frLayer* layer = getDesign()->getTech()->getLayer(layer_num);
  if (layer->isHorizontal()) {
    low = gtl::yl(rect);
    high = gtl::yh(rect);
  } else if (layer->isVertical()) {
    low = gtl::xl(rect);
    high = gtl::xh(rect);
  } else {
    logger_->error(
        DRT,
        1003,
        "enclosesPlanarAccess: layer is neither vertical or horizontal");
  }
  const auto& tracks = track_coords_[layer_num];
  const auto low_track = tracks.lower_bound(low);
  if (low_track == tracks.end()) {
    logger_->error(DRT, 1004, "enclosesPlanarAccess: low track not found");
  }
  if (low_track->first > high) {
    return false;
  }
  auto high_track = tracks.lower_bound(high);
  if (high_track != tracks.end()) {
    if (high_track->first > high) {
      high_track--;
    }
  } else {
    logger_->error(DRT, 1005, "enclosesPlanarAccess: high track not found");
  }
  if (high_track->first - low_track->first > (int) layer->getPitch()) {
    return true;
  }
  if (low_track->first - (int) layer->getWidth() / 2 < low) {
    return false;
  }
  if (high_track->first + (int) layer->getWidth() / 2 > high) {
    return false;
  }
  return true;
}

}  // namespace drt
