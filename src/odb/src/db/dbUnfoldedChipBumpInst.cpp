// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipBumpInst.h"

#include "dbChipBump.h"
#include "dbChipBumpInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "dbUnfoldedChipInst.h"
#include "dbUnfoldedChipRegionInst.h"
#include "odb/db.h"
#include "odb/geom.h"
namespace odb {
template class dbTable<_dbUnfoldedChipBumpInst>;

bool _dbUnfoldedChipBumpInst::operator==(
    const _dbUnfoldedChipBumpInst& rhs) const
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

bool _dbUnfoldedChipBumpInst::operator<(
    const _dbUnfoldedChipBumpInst& rhs) const
{
  return true;
}

_dbUnfoldedChipBumpInst::_dbUnfoldedChipBumpInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipBumpInst& obj)
{
  stream >> obj.chip_bump_inst_;
  stream >> obj.parent_region_;
  stream >> obj.region_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipBumpInst& obj)
{
  stream << obj.chip_bump_inst_;
  stream << obj.parent_region_;
  stream << obj.region_next_;
  return stream;
}

void _dbUnfoldedChipBumpInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipBumpInst - Methods
//
////////////////////////////////////////////////////////////////////

dbChipBumpInst* dbUnfoldedChipBumpInst::getChipBumpInst() const
{
  _dbUnfoldedChipBumpInst* obj = (_dbUnfoldedChipBumpInst*) this;
  if (obj->chip_bump_inst_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipBumpInst*) par->chip_bump_inst_tbl_->getPtr(
      obj->chip_bump_inst_);
}

dbUnfoldedChipRegionInst* dbUnfoldedChipBumpInst::getParentRegion() const
{
  _dbUnfoldedChipBumpInst* obj = (_dbUnfoldedChipBumpInst*) this;
  if (obj->parent_region_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedChipRegionInst*)
      par->unfolded_chip_region_inst_tbl_->getPtr(obj->parent_region_);
}

// User Code Begin dbUnfoldedChipBumpInstPublicMethods
Point3D dbUnfoldedChipBumpInst::getGlobalPosition() const
{
  _dbUnfoldedChipBumpInst* obj = (_dbUnfoldedChipBumpInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipBumpInst* bump_inst
      = (dbChipBumpInst*) db->chip_bump_inst_tbl_->getPtr(obj->chip_bump_inst_);
  dbInst* inst = bump_inst->getChipBump()->getInst();
  if (inst == nullptr) {
    return Point3D();
  }
  Point pt = inst->getBBox()->getBox().center();
  dbUnfoldedChipRegionInst* region
      = (dbUnfoldedChipRegionInst*) db->unfolded_chip_region_inst_tbl_->getPtr(
          obj->parent_region_);
  _dbUnfoldedChipRegionInst* _region = (_dbUnfoldedChipRegionInst*) region;
  _dbUnfoldedChipInst* parent_chip
      = db->unfolded_chip_inst_tbl_->getPtr(_region->parent_chip_);
  parent_chip->transform_.apply(pt);
  return Point3D(pt.x(), pt.y(), region->getSurfaceZ());
}
// User Code End dbUnfoldedChipBumpInstPublicMethods
}  // namespace odb
// Generator Code End Cpp