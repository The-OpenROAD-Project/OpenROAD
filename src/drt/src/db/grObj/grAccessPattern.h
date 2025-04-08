// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

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
