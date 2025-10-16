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
  frTrackPattern() = default;
  frTrackPattern(bool tmpIsH,
                 frCoord tmpSC,
                 frUInt4 tmpNT,
                 frUInt4 tmpTS,
                 frLayerNum tmpLN)
      : horizontal_(tmpIsH),
        startCoord_(tmpSC),
        numTracks_(tmpNT),
        trackSpacing_(tmpTS),
        layerNum_(tmpLN)
  {
  }
  // getters
  // vertical track has horizontal = true;
  bool isHorizontal() const { return horizontal_; }
  frCoord getStartCoord() const { return startCoord_; }
  frUInt4 getNumTracks() const { return numTracks_; }
  frUInt4 getTrackSpacing() const { return trackSpacing_; }
  frLayerNum getLayerNum() const { return layerNum_; }
  // setters
  // vertical track has horizontal = true;
  void setHorizontal(bool tmpIsHorizontal) { horizontal_ = tmpIsHorizontal; }
  void setStartCoord(frCoord tmpStartCoord) { startCoord_ = tmpStartCoord; }
  void setNumTracks(frUInt4 tmpNumTracks) { numTracks_ = tmpNumTracks; }
  void setTrackSpacing(frUInt4 tmpTrackSpacing)
  {
    trackSpacing_ = tmpTrackSpacing;
  }
  void setLayerNum(frLayerNum tmpLayerNum) { layerNum_ = tmpLayerNum; }
  frBlockObjectEnum typeId() const override { return frcTrackPattern; }

 private:
  bool horizontal_{false};
  frCoord startCoord_{0};
  frUInt4 numTracks_{0};
  frUInt4 trackSpacing_{0};
  frLayerNum layerNum_{0};
};

}  // namespace drt
