// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frBlockObject.h"

namespace drt {
class frGCellPattern : public frBlockObject
{
 public:
  // getters
  bool isHorizontal() const { return horizontal_; }
  frCoord getStartCoord() const { return startCoord_; }
  frUInt4 getSpacing() const { return spacing_; }
  frUInt4 getCount() const { return count_; }
  // setters
  void setHorizontal(bool isH) { horizontal_ = isH; }
  void setStartCoord(frCoord scIn) { startCoord_ = scIn; }
  void setSpacing(frUInt4 sIn) { spacing_ = sIn; }
  void setCount(frUInt4 cIn) { count_ = cIn; }
  // others
  frBlockObjectEnum typeId() const override { return frcGCellPattern; }

 private:
  bool horizontal_{false};
  frCoord startCoord_{0};
  frUInt4 spacing_{0};
  frUInt4 count_{0};
};
}  // namespace drt
