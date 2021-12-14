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

#ifndef _FR_TRACKPATTERN_H_
#define _FR_TRACKPATTERN_H_

#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace fr {
class frTrackPattern : public frBlockObject
{
 public:
  // constructors
  frTrackPattern()
      : horizontal_(false),
        startCoord_(0),
        numTracks_(0),
        trackSpacing_(0),
        layerNum_(0)
  {
  }
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
  bool horizontal_;
  frCoord startCoord_;
  frUInt4 numTracks_;
  frUInt4 trackSpacing_;
  frLayerNum layerNum_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frBlockObject>(*this);
    (ar) & horizontal_;
    (ar) & startCoord_;
    (ar) & numTracks_;
    (ar) & trackSpacing_;
    (ar) & layerNum_;
  }

  friend class boost::serialization::access;
};

}  // namespace fr

#endif
