// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/drObj/drFig.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

class drRef : public drPinFig
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

 protected:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drPinFig>(*this);
  }

  friend class boost::serialization::access;
};

}  // namespace drt
