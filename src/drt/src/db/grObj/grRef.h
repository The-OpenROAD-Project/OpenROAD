// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/grObj/grFig.h"
#include "odb/dbTypes.h"

namespace drt {

class grRef : public grPinFig
{
 public:
  // getters
  virtual dbOrientType getOrient() const = 0;
  virtual Point getOrigin() const = 0;
  virtual dbTransform getTransform() const = 0;
  // setters
  virtual void setOrient(const dbOrientType& tmpOrient) = 0;
  virtual void setOrigin(const Point& tmpPoint) = 0;
  virtual void setTransform(const dbTransform& xform) = 0;
  frBlockObjectEnum typeId() const override { return grcRef; }
};

}  // namespace drt
