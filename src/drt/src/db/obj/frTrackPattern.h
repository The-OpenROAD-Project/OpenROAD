// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {
class frTrackPattern : public frBlockObject
{
 public:
  // constructors
  frTrackPattern(bool horizontal,
                 frCoord start_coord,
                 frUInt4 num_tracks,
                 frUInt4 track_spacing,
                 frLayerNum layer_num)
      : horizontal_(horizontal),
        startCoord_(start_coord),
        numTracks_(num_tracks),
        trackSpacing_(track_spacing),
        layerNum_(layer_num)
  {
  }
  // getters
  // vertical track has horizontal = true;
  bool isHorizontal() const { return horizontal_; }
  frCoord getStartCoord() const { return startCoord_; }
  frUInt4 getNumTracks() const { return numTracks_; }
  frUInt4 getTrackSpacing() const { return trackSpacing_; }
  frLayerNum getLayerNum() const { return layerNum_; }
  frBlockObjectEnum typeId() const override { return frcTrackPattern; }

 private:
  bool horizontal_;
  frCoord startCoord_;
  frUInt4 numTracks_;
  frUInt4 trackSpacing_;
  frLayerNum layerNum_;
};

}  // namespace drt
