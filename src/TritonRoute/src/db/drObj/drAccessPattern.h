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

#ifndef _DR_ACCESS_PATTERN_H
#define _DR_ACCESS_PATTERN_H

#include "db/drObj/drBlockObject.h"
#include "db/infra/frPoint.h"
#include "dr/FlexMazeTypes.h"

namespace fr {
class drPin;
class frViaDef;
class drAccessPattern : public drBlockObject
{
 public:
  drAccessPattern()
      : beginPoint_(),
        beginLayerNum_(0),
        beginArea_(0),
        pin_(nullptr),
        validAccess_(std::vector<bool>(6, true)),
        vU_(nullptr),
        vD_(nullptr),
        vUIdx_(0),
        vDIdx_(0),
        onTrackX_(true),
        onTrackY_(true),
        pinCost_(0)
  {
  }
  // getters
  void getPoint(frPoint& bpIn) const { bpIn.set(beginPoint_); }
  frLayerNum getBeginLayerNum() const { return beginLayerNum_; }
  frCoord getBeginArea() const { return beginArea_; }
  drPin* getPin() const { return pin_; }
  bool hasMazeIdx() const { return (!mazeIdx_.empty()); }
  void getMazeIdx(FlexMazeIdx& in) const { in.set(mazeIdx_); }
  bool hasValidAccess(const frDirEnum& dir)
  {
    switch (dir) {
      case (frDirEnum::E):
        return validAccess_[0];
        break;
      case (frDirEnum::S):
        return validAccess_[1];
        break;
      case (frDirEnum::W):
        return validAccess_[2];
        break;
      case (frDirEnum::N):
        return validAccess_[3];
        break;
      case (frDirEnum::U):
        return validAccess_[4];
        break;
      case (frDirEnum::D):
        return validAccess_[5];
        break;
      default:
        return false;
    }
  }
  bool hasAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    if (dir == frDirEnum::U) {
      return !(vU_ == nullptr);
    } else {
      return !(vD_ == nullptr);
    }
  }
  frViaDef* getAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    if (dir == frDirEnum::U) {
      return (*vU_)[vUIdx_];
    } else {
      return (*vD_)[vDIdx_];
    }
  }
  bool nextAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    bool sol = true;
    if (dir == frDirEnum::U) {
      if ((*vU_).size() == 1) {
        sol = false;
      } else {
        ++vUIdx_;
        if (vUIdx_ >= (int) (*vU_).size()) {
          vUIdx_ -= (int) (*vU_).size();
        }
      }
    } else {
      if ((*vD_).size() == 1) {
        sol = false;
      } else {
        ++vDIdx_;
        if (vDIdx_ >= (int) (*vD_).size()) {
          vDIdx_ -= (int) (*vD_).size();
        }
      }
    }
    return sol;
  }
  bool prevAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    bool sol = true;
    if (dir == frDirEnum::U) {
      if ((*vU_).size() == 1) {
        sol = false;
      } else {
        --vUIdx_;
        if (vUIdx_ < 0) {
          vUIdx_ += (int) (*vU_).size();
        }
      }
    } else {
      if ((*vD_).size() == 1) {
        sol = false;
      } else {
        --vDIdx_;
        if (vDIdx_ < 0) {
          vDIdx_ += (int) (*vD_).size();
        }
      }
    }
    return sol;
  }
  bool isOnTrack(bool isX) const { return (isX) ? onTrackX_ : onTrackY_; }
  frUInt4 getPinCost() const { return pinCost_; }
  // setters
  void setPoint(const frPoint& bpIn) { beginPoint_.set(bpIn); }
  void setBeginLayerNum(frLayerNum in) { beginLayerNum_ = in; }
  void setBeginArea(frCoord in) { beginArea_ = in; }
  void setMazeIdx(const FlexMazeIdx& in) { mazeIdx_.set(in); }
  void setPin(drPin* in) { pin_ = in; }
  void setValidAccess(const std::vector<bool>& in) { validAccess_ = in; }

  void setAccessViaDef(const frDirEnum dir, std::vector<frViaDef*>* viaDef)
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
  frPoint beginPoint_;
  frLayerNum beginLayerNum_;
  frCoord beginArea_;
  FlexMazeIdx mazeIdx_;
  drPin* pin_;
  std::vector<bool> validAccess_;
  std::vector<frViaDef*>* vU_;
  std::vector<frViaDef*>* vD_;
  int vUIdx_;
  int vDIdx_;
  bool onTrackX_;    // initialized in initMazeIdx_ap
  bool onTrackY_;    // initialized in initMazeIdx_ap
  frUInt4 pinCost_;  // is preferred ap
};
}  // namespace fr

#endif
