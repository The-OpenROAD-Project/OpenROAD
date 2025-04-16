// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Pin.h"

namespace grt {

class RoutingTracks
{
 public:
  RoutingTracks() = default;
  RoutingTracks(const int layer_index,
                const int track_pitch,
                const int line_2_via_pitch_up,
                const int line_2_via_pitch_down,
                const int location,
                const int num_tracks)
      : layer_index_(layer_index),
        track_pitch_(track_pitch),
        line_2_via_pitch_up_(line_2_via_pitch_up),
        line_2_via_pitch_down_(line_2_via_pitch_down),
        location_(location),
        num_tracks_(num_tracks)
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
    return std::max(
        {track_pitch_, line_2_via_pitch_up_, line_2_via_pitch_down_});
  }
  int getLocation() const { return location_; }
  int getNumTracks() const { return num_tracks_; }

 private:
  int layer_index_;
  int track_pitch_;
  int line_2_via_pitch_up_;
  int line_2_via_pitch_down_;
  int location_;
  int num_tracks_;
};

}  // namespace grt
