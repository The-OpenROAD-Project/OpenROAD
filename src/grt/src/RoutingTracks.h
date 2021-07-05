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

#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Pin.h"

namespace grt {

class RoutingTracks
{
 private:
  int layer_index_;
  int track_pitch_;
  int line_2_via_pitch_up_;
  int line_2_via_pitch_down_;
  int location_;
  int num_tracks_;
  bool orientation_;

 public:
  RoutingTracks() = default;
  RoutingTracks(const int layer_index,
                const int track_pitch,
                const int line_2_via_pitch_up,
                const int line_2_via_pitch_down,
                const int location,
                const int num_tracks,
                const bool orientation)
      : layer_index_(layer_index),
        track_pitch_(track_pitch),
        line_2_via_pitch_up_(line_2_via_pitch_up),
        line_2_via_pitch_down_(line_2_via_pitch_down),
        location_(location),
        num_tracks_(num_tracks),
        orientation_(orientation)
  {
  }

  int getLayerIndex() const { return layer_index_; }
  int getTrackPitch() const { return track_pitch_; }
  int getLineToViaPitch() const
  {
    return std::max(line_2_via_pitch_up_, line_2_via_pitch_down_);
  }
  int getUsePitch() const
  {
    return std::max({track_pitch_, line_2_via_pitch_up_, line_2_via_pitch_down_});
  }
  int getLocation() const { return location_; }
  int getNumTracks() const { return num_tracks_; }
  bool getOrientation() const { return orientation_; }
};

}  // namespace grt
