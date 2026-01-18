// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frFig.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {
class frRef : public frPinFig
{
 public:
  // getters
  virtual odb::Point getOrigin() const = 0;

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
