// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frInst.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

#include "db/obj/frInstTerm.h"
#include "db/obj/frMaster.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

frInst::frInst(frMaster* master, odb::dbInst* db_inst)
    : master_(master), db_inst_(db_inst)
{
  if (db_inst_ == nullptr) {
    throw std::runtime_error("frInst requires non-null dbInst");
  }
}

frInst::frInst() = default;

frInst::~frInst() = default;

void frInst::addInstTerm(std::unique_ptr<frInstTerm> in)
{
  instTerms_.push_back(std::move(in));
}

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
  return {odb::dbOrientType::R0, db_inst_->getLocation()};
}

frInstTerm* frInst::getInstTerm(const int index)
{
  return instTerms_.at(index).get();
}

}  // namespace drt
