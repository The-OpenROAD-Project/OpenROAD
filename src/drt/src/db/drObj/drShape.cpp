// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/drObj/drShape.h"

#include <tuple>

#include "db/drObj/drNet.h"
#include "db/obj/frShape.h"
#include "odb/geom.h"

namespace drt {

odb::Rect drPathSeg::getBBox() const
{
  bool isHorizontal = true;
  if (begin_.x() == end_.x()) {
    isHorizontal = false;
  }
  const auto width = style_.getWidth();
  const auto beginExt = style_.getBeginExt();
  const auto endExt = style_.getEndExt();
  if (isHorizontal) {
    return odb::Rect(begin_.x() - beginExt,
                     begin_.y() - width / 2,
                     end_.x() + endExt,
                     end_.y() + width / 2);
  }
  return odb::Rect(begin_.x() - width / 2,
                   begin_.y() - beginExt,
                   end_.x() + width / 2,
                   end_.y() + endExt);
}

drPathSeg::drPathSeg(const frPathSeg& in) : layer_(in.getLayerNum())
{
  std::tie(begin_, end_) = in.getPoints();
  style_ = in.getStyle();
  setTapered(in.isTapered());
}

drPatchWire::drPatchWire(const frPatchWire& in) : layer_(in.getLayerNum())
{
  offsetBox_ = in.getOffsetBox();
  origin_ = in.getOrigin();
}

}  // namespace drt
