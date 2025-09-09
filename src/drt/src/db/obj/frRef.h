// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frFig.h"
#include "odb/dbTypes.h"

namespace drt {
class frRef : public frPinFig
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

 protected:
  // constructors
  frRef() = default;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frPinFig>(*this);
  }

  friend class boost::serialization::access;
};
}  // namespace drt
