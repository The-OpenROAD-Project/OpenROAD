// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frRPin.h"

#include <iostream>

#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {

odb::Rect frRPin::getBBox()
{
  odb::Point pt;

  switch (term_->typeId()) {
    case frcInstTerm: {
      auto inst = static_cast<frInstTerm*>(term_)->getInst();
      odb::dbTransform shiftXform = inst->getNoRotationTransform();

      pt = accessPoint_->getPoint();
      shiftXform.apply(pt);
      break;
    }
    case frcBTerm:
      pt = accessPoint_->getPoint();
      break;
    default:
      std::cout << "ERROR: Invalid term type in frRPin.\n";
      break;
  }

  return odb::Rect(pt.x(), pt.y(), pt.x(), pt.y());
}

frLayerNum frRPin::getLayerNum()
{
  return accessPoint_->getLayerNum();
}

}  // namespace drt
