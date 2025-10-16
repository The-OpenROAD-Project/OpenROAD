// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {

class drBlockObject : public frBlockObject
{
 protected:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frBlockObject>(*this);
  }

  friend class boost::serialization::access;
};

}  // namespace drt
