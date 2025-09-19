// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db/obj/frFig.h"
#include "frBaseTypes.h"

namespace drt {
class frBoundary : public frFig
{
 public:
  // getters
  const std::vector<Point>& getPoints() const { return points_; }
  frUInt4 getNumPoints() const { return points_.size(); }
  // setters
  void setPoints(const std::vector<Point>& pIn) { points_ = pIn; }
  // others
  frBlockObjectEnum typeId() const override { return frcBoundary; }

  Rect getBBox() const override
  {
    frCoord llx = 0;
    frCoord lly = 0;
    frCoord urx = 0;
    frCoord ury = 0;
    if (!points_.empty()) {
      llx = points_.begin()->x();
      urx = points_.begin()->x();
      lly = points_.begin()->y();
      ury = points_.begin()->y();
    }
    for (auto& point : points_) {
      llx = (llx < point.x()) ? llx : point.x();
      lly = (lly < point.y()) ? lly : point.y();
      urx = (urx > point.x()) ? urx : point.x();
      ury = (ury > point.y()) ? ury : point.y();
    }
    return {llx, lly, urx, ury};
  }
  void move(const dbTransform& xform) override
  {
    for (auto& point : points_) {
      xform.apply(point);
    }
  }
  bool intersects(const Rect& box) const override { return false; }

 protected:
  std::vector<Point> points_;
};
}  // namespace drt
