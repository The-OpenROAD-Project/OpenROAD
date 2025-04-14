// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "db/obj/frInstTerm.h"

#include <vector>

#include "db/obj/frInst.h"

namespace drt {

frString frInstTerm::getName() const
{
  return getInst()->getName() + '/' + getTerm()->getName();
}

frAccessPoint* frInstTerm::getAccessPoint(frCoord x, frCoord y, frLayerNum lNum)
{
  auto inst = getInst();
  dbTransform shiftXform = inst->getTransform();
  Point offset(shiftXform.getOffset());
  x = x - offset.getX();
  y = y - offset.getY();
  return term_->getAccessPoint(x, y, lNum, inst->getPinAccessIdx());
}

bool frInstTerm::hasAccessPoint(frCoord x, frCoord y, frLayerNum lNum)
{
  return getAccessPoint(x, y, lNum) != nullptr;
}

void frInstTerm::getShapes(std::vector<frRect>& outShapes) const
{
  term_->getShapes(outShapes);
  for (auto& shape : outShapes) {
    dbTransform trans = getInst()->getDBTransform();
    shape.move(trans);
  }
}

Rect frInstTerm::getBBox() const
{
  Rect bbox(term_->getBBox());
  dbTransform trans = getInst()->getDBTransform();
  trans.apply(bbox);
  return bbox;
}

}  // namespace drt
