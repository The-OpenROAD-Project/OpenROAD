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

#include "db/grObj/grBlockObject.h"
#include "db/infra/frPoint.h"

namespace drt {
class grPin;
class frViaDef;
class grAccessPattern : public grBlockObject
{
 public:
  // getters
  Point getPoint() const { return beginPoint; }
  frLayerNum getBeginLayerNum() const { return beginLayerNum; }
  grPin* getPin() const { return pin; }
  bool hasValidAccess(const frDirEnum& dir)
  {
    switch (dir) {
      case (frDirEnum::E):
        return validAccess[0];
        break;
      case (frDirEnum::S):
        return validAccess[1];
        break;
      case (frDirEnum::W):
        return validAccess[2];
        break;
      case (frDirEnum::N):
        return validAccess[3];
        break;
      case (frDirEnum::U):
        return validAccess[4];
        break;
      case (frDirEnum::D):
        return validAccess[5];
        break;
      default:
        return false;
    }
  }
  bool hasAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    if (dir == frDirEnum::U) {
      return !(vU == nullptr);
    }
    return !(vD == nullptr);
  }
  frViaDef* getAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    if (dir == frDirEnum::U) {
      return (*vU)[vUIdx];
    }
    return (*vD)[vDIdx];
  }

  // setters
  void setPoint(const Point& bpIn) { beginPoint = bpIn; }
  void setBeginLayerNum(frLayerNum in) { beginLayerNum = in; }
  void setPin(grPin* in) { pin = in; }
  void setValidAccess(const std::vector<bool>& in) { validAccess = in; }
  void setAccessViaDef(const frDirEnum dir, std::vector<frViaDef*>* viaDef)
  {
    if (dir == frDirEnum::U) {
      vU = viaDef;
    } else {
      vD = viaDef;
    }
  }

  // others
  frBlockObjectEnum typeId() const override { return grcAccessPattern; }

 protected:
  Point beginPoint;
  frLayerNum beginLayerNum{0};
  grPin* pin{nullptr};
  std::vector<bool> validAccess = std::vector<bool>(6, true);
  std::vector<frViaDef*>* vU{nullptr};
  std::vector<frViaDef*>* vD{nullptr};
  int vUIdx{0};
  int vDIdx{0};
};
}  // namespace drt
