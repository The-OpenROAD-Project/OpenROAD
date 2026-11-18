// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "db/drObj/drAccessPattern.h"

#include "db/drObj/drBlockObject.h"
#include "distributed/frArchive.h"
#include "frBaseTypes.h"
namespace drt {

bool drAccessPattern::hasValidAccess(const frDirEnum& dir)
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
    case (frDirEnum::UNKNOWN):
      return false;
  }
  return false;
}

bool drAccessPattern::nextAccessViaDef(const frDirEnum& dir)
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

bool drAccessPattern::prevAccessViaDef(const frDirEnum& dir)
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

template <class Archive>
void drAccessPattern::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drBlockObject>(*this);
  (ar) & beginPoint_;
  (ar) & beginLayerNum_;
  (ar) & beginArea_;
  (ar) & mazeIdx_;
  (ar) & pin_;
  (ar) & validAccess_;
  (ar) & vUIdx_;
  (ar) & vDIdx_;
  (ar) & onTrackX_;
  (ar) & onTrackY_;
  (ar) & pinCost_;
  // vU_ and vD_ are initialized at init and never used in end.(No Need to
  // serialize)
}

// Explicit instantiations
template void drAccessPattern::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void drAccessPattern::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

}  // namespace drt
