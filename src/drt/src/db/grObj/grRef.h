// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/grObj/grFig.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

class grRef : public grPinFig
{
 public:
  // getters
  virtual odb::dbOrientType getOrient() const = 0;
  virtual odb::Point getOrigin() const = 0;
  virtual odb::dbTransform getTransform() const = 0;
  // setters
  virtual void setOrient(const odb::dbOrientType& tmpOrient) = 0;
  virtual void setOrigin(const odb::Point& tmpPoint) = 0;
  virtual void setTransform(const odb::dbTransform& xform) = 0;
  frBlockObjectEnum typeId() const override { return grcRef; }
};

}  // namespace drt
