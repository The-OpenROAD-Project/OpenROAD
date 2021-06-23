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
  int _layerIndex;
  int _trackPitch;
  int _line2ViaPitchUp;
  int _line2ViaPitchDown;
  int _location;
  int _numTracks;
  bool _orientation;

 public:
  RoutingTracks() = default;
  RoutingTracks(const int layerIndex,
                const int trackPitch,
                const int line2ViaPitchUp,
                const int line2ViaPitchDown,
                const int location,
                const int numTracks,
                const bool orientation)
      : _layerIndex(layerIndex),
        _trackPitch(trackPitch),
        _line2ViaPitchUp(line2ViaPitchUp),
        _line2ViaPitchDown(line2ViaPitchDown),
        _location(location),
        _numTracks(numTracks),
        _orientation(orientation)
  {
  }

  int getLayerIndex() const { return _layerIndex; }
  int getTrackPitch() const { return _trackPitch; }
  int getLineToViaPitch() const
  {
    return std::max(_line2ViaPitchUp, _line2ViaPitchDown);
  }
  int getUsePitch() const
  {
    return std::max({_trackPitch, _line2ViaPitchUp, _line2ViaPitchDown});
  }
  int getLocation() const { return _location; }
  int getNumTracks() const { return _numTracks; }
  bool getOrientation() const { return _orientation; }
};

}  // namespace grt
