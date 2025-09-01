// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "frRPin.h"

#include <iostream>

#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"

namespace drt {

Rect frRPin::getBBox()
{
  Point pt;

  switch (term_->typeId()) {
    case frcInstTerm: {
      auto inst = static_cast<frInstTerm*>(term_)->getInst();
      dbTransform shiftXform = inst->getNoRotationTransform();

      pt = accessPoint_->getPoint();
      shiftXform.apply(pt);
      break;
    }
    case frcBTerm:
      pt = accessPoint_->getPoint();
      break;
    default:
      std::cout << "ERROR: Invalid term type in frRPin." << std::endl;
      break;
  }

  return Rect(pt.x(), pt.y(), pt.x(), pt.y());
}

frLayerNum frRPin::getLayerNum()
{
  return accessPoint_->getLayerNum();
}

}  // namespace drt
