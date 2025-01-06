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

#pragma once

#include "db/drObj/drBlockObject.h"
#include "db/infra/frPoint.h"
#include "dr/FlexMazeTypes.h"

namespace drt {

class drPin;
class frViaDef;

class drAccessPattern : public drBlockObject
{
 public:
  // getters
  Point getPoint() const { return beginPoint_; }
  frLayerNum getBeginLayerNum() const { return beginLayerNum_; }
  frCoord getBeginArea() const { return beginArea_; }
  drPin* getPin() const { return pin_; }
  bool hasMazeIdx() const { return (!mazeIdx_.empty()); }
  FlexMazeIdx getMazeIdx() const { return mazeIdx_; }
  bool hasValidAccess(const frDirEnum& dir);
  bool hasAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    return (dir == frDirEnum::U) ? vU_ : vD_;
  }
  const frViaDef* getAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    return (dir == frDirEnum::U) ? (*vU_)[vUIdx_] : (*vD_)[vDIdx_];
  }
  bool nextAccessViaDef(const frDirEnum& dir = frDirEnum::U);
  bool prevAccessViaDef(const frDirEnum& dir = frDirEnum::U);
  bool isOnTrack(bool isX) const { return (isX) ? onTrackX_ : onTrackY_; }
  frUInt4 getPinCost() const { return pinCost_; }
  // setters
  void setPoint(const Point& bpIn) { beginPoint_ = bpIn; }
  void setBeginLayerNum(frLayerNum in) { beginLayerNum_ = in; }
  void setBeginArea(frCoord in) { beginArea_ = in; }
  void setMazeIdx(const FlexMazeIdx& in) { mazeIdx_.set(in); }
  void setPin(drPin* in) { pin_ = in; }
  void setValidAccess(const std::vector<bool>& in) { validAccess_ = in; }

  void setAccessViaDef(const frDirEnum dir,
                       std::vector<const frViaDef*>* viaDef)
  {
    if (dir == frDirEnum::U) {
      vU_ = viaDef;
    } else {
      vD_ = viaDef;
    }
  }
  void setOnTrack(bool in, bool isX)
  {
    if (isX) {
      onTrackX_ = in;
    } else {
      onTrackY_ = in;
    }
  }
  void setPinCost(frUInt4 in) { pinCost_ = in; }

  // others
  frBlockObjectEnum typeId() const override { return drcAccessPattern; }

 protected:
  Point beginPoint_;
  frLayerNum beginLayerNum_{0};
  frCoord beginArea_{0};
  FlexMazeIdx mazeIdx_;
  drPin* pin_{nullptr};
  std::vector<bool> validAccess_ = std::vector<bool>(6, true);
  std::vector<const frViaDef*>* vU_{nullptr};
  std::vector<const frViaDef*>* vD_{nullptr};
  int vUIdx_{0};
  int vDIdx_{0};
  bool onTrackX_{true};  // initialized in initMazeIdx_ap
  bool onTrackY_{true};  // initialized in initMazeIdx_ap
  frUInt4 pinCost_{0};   // is preferred ap

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};

}  // namespace drt
