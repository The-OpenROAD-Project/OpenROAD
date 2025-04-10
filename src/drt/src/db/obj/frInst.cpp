// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frInst.h"

#include "frBlock.h"
#include "frMaster.h"
namespace drt {

Rect frInst::getBBox() const
{
  Rect box = getMaster()->getBBox();
  dbTransform xform = getDBTransform();
  xform.apply(box);
  return box;
}

Rect frInst::getBoundaryBBox() const
{
  Rect box = getMaster()->getDieBox();
  dbTransform xform = getDBTransform();
  xform.apply(box);
  return box;
}

dbTransform frInst::getNoRotationTransform() const
{
  dbTransform xfm = getTransform();
  xfm.setOrient(dbOrientType(dbOrientType::R0));
  return xfm;
}

frInstTerm* frInst::getInstTerm(const int index)
{
  return instTerms_.at(index).get();
}

}  // namespace drt
