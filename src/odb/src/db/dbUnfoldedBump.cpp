// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedBump.h"

#include "dbChipBump.h"
#include "dbChipBumpInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "dbUnfoldedChip.h"
#include "dbUnfoldedRegion.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbUnfoldedBump>;

bool _dbUnfoldedBump::operator==(const _dbUnfoldedBump& rhs) const
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

bool _dbUnfoldedBump::operator<(const _dbUnfoldedBump& rhs) const
{
  return true;
}

_dbUnfoldedBump::_dbUnfoldedBump(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedBump& obj)
{
  stream >> obj.chip_bump_inst_;
  stream >> obj.parent_region_;
  stream >> obj.region_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedBump& obj)
{
  stream << obj.chip_bump_inst_;
  stream << obj.parent_region_;
  stream << obj.region_next_;
  return stream;
}

void _dbUnfoldedBump::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedBump - Methods
//
////////////////////////////////////////////////////////////////////

dbChipBumpInst* dbUnfoldedBump::getChipBumpInst() const
{
  _dbUnfoldedBump* obj = (_dbUnfoldedBump*) this;
  if (obj->chip_bump_inst_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipBumpInst*) par->chip_bump_inst_tbl_->getPtr(
      obj->chip_bump_inst_);
}

dbUnfoldedRegion* dbUnfoldedBump::getParentRegion() const
{
  _dbUnfoldedBump* obj = (_dbUnfoldedBump*) this;
  if (obj->parent_region_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedRegion*) par->unfolded_region_tbl_->getPtr(
      obj->parent_region_);
}

// User Code Begin dbUnfoldedBumpPublicMethods
Point3D dbUnfoldedBump::getGlobalPosition() const
{
  _dbUnfoldedBump* obj = (_dbUnfoldedBump*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipBumpInst* bump_inst
      = (dbChipBumpInst*) db->chip_bump_inst_tbl_->getPtr(obj->chip_bump_inst_);
  dbInst* inst = bump_inst->getChipBump()->getInst();
  if (inst == nullptr) {
    return Point3D();
  }
  Point pt = inst->getBBox()->getBox().center();
  dbUnfoldedRegion* region
      = (dbUnfoldedRegion*) db->unfolded_region_tbl_->getPtr(
          obj->parent_region_);
  _dbUnfoldedRegion* _region = (_dbUnfoldedRegion*) region;
  _dbUnfoldedChip* parent_chip
      = db->unfolded_chip_tbl_->getPtr(_region->parent_chip_);
  parent_chip->transform_.apply(pt);
  return Point3D(pt.x(), pt.y(), region->getSurfaceZ());
}
// User Code End dbUnfoldedBumpPublicMethods
}  // namespace odb
// Generator Code End Cpp