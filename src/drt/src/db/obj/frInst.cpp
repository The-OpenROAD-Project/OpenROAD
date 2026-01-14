// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frInst.h"

#include "db/obj/frBlock.h"
#include "db/obj/frMaster.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace drt {

odb::Rect frInst::getBBox() const
{
  odb::Rect box = getMaster()->getBBox();
  odb::dbTransform xform = getDBTransform();
  xform.apply(box);
  return box;
}

odb::Rect frInst::getBoundaryBBox() const
{
  odb::Rect box = getMaster()->getDieBox();
  odb::dbTransform xform = getDBTransform();
  xform.apply(box);
  return box;
}

odb::dbTransform frInst::getNoRotationTransform() const
{
  odb::dbTransform xfm = getTransform();
  xfm.setOrient(odb::dbOrientType(odb::dbOrientType::R0));
  return xfm;
}

frInstTerm* frInst::getInstTerm(const int index)
{
  return instTerms_.at(index).get();
}

}  // namespace drt
