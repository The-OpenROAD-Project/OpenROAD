// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {
class frGCellPattern : public frBlockObject
{
 public:
  // constructors
  frGCellPattern(bool horizontal,
                 frCoord start_coord,
                 frUInt4 spacing,
                 frUInt4 count)
      : horizontal_(horizontal),
        startCoord_(start_coord),
        spacing_(spacing),
        count_(count)
  {
  }
  // getters
  bool isHorizontal() const { return horizontal_; }
  frCoord getStartCoord() const { return startCoord_; }
  frUInt4 getSpacing() const { return spacing_; }
  frUInt4 getCount() const { return count_; }
  // others
  frBlockObjectEnum typeId() const override { return frcGCellPattern; }

 private:
  bool horizontal_{false};
  frCoord startCoord_{0};
  frUInt4 spacing_{0};
  frUInt4 count_{0};
};
}  // namespace drt
