// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "db/drObj/drBlockObject.h"
#include "db/infra/frPoint.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {

class drPin;
class frViaDef;

class drAccessPattern : public drBlockObject
{
 public:
  // getters
  odb::Point getPoint() const { return beginPoint_; }
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
  const frViaDef* getAccessViaDef()
  {
    if (hasAccessViaDef(frDirEnum::U)) {
      return (*vU_)[vUIdx_];
    }
    if (hasAccessViaDef(frDirEnum::D)) {
      return (*vD_)[vDIdx_];
    }
    return nullptr;
  }
  bool nextAccessViaDef(const frDirEnum& dir = frDirEnum::U);
  bool prevAccessViaDef(const frDirEnum& dir = frDirEnum::U);
  bool isOnTrack(bool isX) const { return (isX) ? onTrackX_ : onTrackY_; }
  frUInt4 getPinCost() const { return pinCost_; }
  // setters
  void setPoint(const odb::Point& bpIn) { beginPoint_ = bpIn; }
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
  odb::Point beginPoint_;
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
