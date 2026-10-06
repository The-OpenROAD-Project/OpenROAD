// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/infra/frPoint.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

class frBox3D
{
 public:
  frBox3D() = default;
  frBox3D(int llx, int lly, int urx, int ury, int zl, int zh)
      : rect_(odb::Rect(llx, lly, urx, ury)), zl_(zl), zh_(zh)
  {
  }
  frBox3D(const frBox3D& in) = default;

  int xMin() const { return rect_.xMin(); }
  int yMin() const { return rect_.yMin(); }
  int xMax() const { return rect_.xMax(); }
  int yMax() const { return rect_.yMax(); }

  bool contains(int x,
                int y,
                int z,
                int bloatX = 0,
                int bloatY = 0,
                int bloatZ = 0) const
  {
    return zl_ - bloatZ <= z && zh_ + bloatZ >= z && xMin() - bloatX <= x
           && xMax() + bloatX >= x && yMin() - bloatY <= y
           && yMax() + bloatY >= y;
  }
  int zLow() const { return zl_; }
  int zHigh() const { return zh_; }

 private:
  odb::Rect rect_;
  int zl_{0};
  int zh_{0};
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & rect_;
    (ar) & zl_;
    (ar) & zh_;
  }

  friend class boost::serialization::access;
};
}  // namespace drt
