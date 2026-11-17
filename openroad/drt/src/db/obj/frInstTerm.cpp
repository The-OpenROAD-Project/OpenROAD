// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "db/obj/frInstTerm.h"

#include <vector>

#include "db/obj/frInst.h"
#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {

frString frInstTerm::getName() const
{
  return getInst()->getName() + '/' + getTerm()->getName();
}

frAccessPoint* frInstTerm::getAccessPoint(frCoord x, frCoord y, frLayerNum lNum)
{
  auto inst = getInst();
  odb::dbTransform shiftXform = inst->getTransform();
  odb::Point offset(shiftXform.getOffset());
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
    odb::dbTransform trans = getInst()->getDBTransform();
    shape.move(trans);
  }
}

odb::Rect frInstTerm::getBBox() const
{
  odb::Rect bbox(term_->getBBox());
  odb::dbTransform trans = getInst()->getDBTransform();
  trans.apply(bbox);
  return bbox;
}

}  // namespace drt
