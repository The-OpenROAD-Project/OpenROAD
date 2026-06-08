// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedBumpInst.h"

#include "dbChipBump.h"
#include "dbChipBumpInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "dbUnfoldedChipInst.h"
#include "dbUnfoldedRegionInst.h"
#include "odb/db.h"
#include "odb/geom.h"
namespace odb {
template class dbTable<_dbUnfoldedBumpInst>;

bool _dbUnfoldedBumpInst::operator==(const _dbUnfoldedBumpInst& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_bump_inst_ != rhs.chip_bump_inst_) {
    return false;
  }
  if (parent_region_ != rhs.parent_region_) {
    return false;
  }
  if (region_next_ != rhs.region_next_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedBumpInst::operator<(const _dbUnfoldedBumpInst& rhs) const
{
  return true;
}

_dbUnfoldedBumpInst::_dbUnfoldedBumpInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedBumpInst& obj)
{
  stream >> obj.chip_bump_inst_;
  stream >> obj.parent_region_;
  stream >> obj.region_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedBumpInst& obj)
{
  stream << obj.chip_bump_inst_;
  stream << obj.parent_region_;
  stream << obj.region_next_;
  return stream;
}

void _dbUnfoldedBumpInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedBumpInst - Methods
//
////////////////////////////////////////////////////////////////////

dbChipBumpInst* dbUnfoldedBumpInst::getChipBumpInst() const
{
  _dbUnfoldedBumpInst* obj = (_dbUnfoldedBumpInst*) this;
  if (obj->chip_bump_inst_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipBumpInst*) par->chip_bump_inst_tbl_->getPtr(
      obj->chip_bump_inst_);
}

dbUnfoldedRegionInst* dbUnfoldedBumpInst::getParentRegion() const
{
  _dbUnfoldedBumpInst* obj = (_dbUnfoldedBumpInst*) this;
  if (obj->parent_region_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedRegionInst*) par->unfolded_region_inst_tbl_->getPtr(
      obj->parent_region_);
}

// User Code Begin dbUnfoldedBumpInstPublicMethods
Point3D dbUnfoldedBumpInst::getGlobalPosition() const
{
  _dbUnfoldedBumpInst* obj = (_dbUnfoldedBumpInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipBumpInst* bump_inst = (dbChipBumpInst*) db->chip_bump_inst_tbl_->getPtr(
      obj->chip_bump_inst_);
  dbInst* inst = bump_inst->getChipBump()->getInst();
  if (inst == nullptr) {
    return Point3D();
  }
  Point pt = inst->getBBox()->getBox().center();
  dbUnfoldedRegionInst* region
      = (dbUnfoldedRegionInst*) db->unfolded_region_inst_tbl_->getPtr(
          obj->parent_region_);
  _dbUnfoldedRegionInst* _region = (_dbUnfoldedRegionInst*) region;
  _dbUnfoldedChipInst* parent_chip
      = db->unfolded_chip_inst_tbl_->getPtr(_region->parent_chip_);
  parent_chip->transform_.apply(pt);
  return Point3D(pt.x(), pt.y(), region->getSurfaceZ());
}
// User Code End dbUnfoldedBumpInstPublicMethods
}  // namespace odb
// Generator Code End Cpp