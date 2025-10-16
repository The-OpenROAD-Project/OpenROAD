// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "db/grObj/grBlockObject.h"
#include "db/infra/frPoint.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class grPin;
class frViaDef;
class grAccessPattern : public grBlockObject
{
 public:
  // getters
  odb::Point getPoint() const { return beginPoint_; }
  frLayerNum getBeginLayerNum() const { return beginLayerNum_; }
  grPin* getPin() const { return pin_; }
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
    }
    return !(vD_ == nullptr);
  }
  frViaDef* getAccessViaDef(const frDirEnum& dir = frDirEnum::U)
  {
    if (dir == frDirEnum::U) {
      return (*vU_)[vUIdx_];
    }
    return (*vD_)[vDIdx_];
  }

  // setters
  void setPoint(const odb::Point& bpIn) { beginPoint_ = bpIn; }
  void setBeginLayerNum(frLayerNum in) { beginLayerNum_ = in; }
  void setPin(grPin* in) { pin_ = in; }
  void setValidAccess(const std::vector<bool>& in) { validAccess_ = in; }
  void setAccessViaDef(const frDirEnum dir, std::vector<frViaDef*>* viaDef)
  {
    if (dir == frDirEnum::U) {
      vU_ = viaDef;
    } else {
      vD_ = viaDef;
    }
  }

  // others
  frBlockObjectEnum typeId() const override { return grcAccessPattern; }

 protected:
  odb::Point beginPoint_;
  frLayerNum beginLayerNum_{0};
  grPin* pin_{nullptr};
  std::vector<bool> validAccess_ = std::vector<bool>(6, true);
  std::vector<frViaDef*>* vU_{nullptr};
  std::vector<frViaDef*>* vD_{nullptr};
  int vUIdx_{0};
  int vDIdx_{0};
};
}  // namespace drt
