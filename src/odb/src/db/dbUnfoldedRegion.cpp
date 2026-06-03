// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedRegion.h"

#include <cstdint>

#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedChip.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbUnfoldedBumpItr.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbUnfoldedRegion>;

bool _dbUnfoldedRegion::operator==(const _dbUnfoldedRegion& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_region_inst_ != rhs.chip_region_inst_) {
    return false;
  }
  if (effective_side_ != rhs.effective_side_) {
    return false;
  }
  if (parent_chip_ != rhs.parent_chip_) {
    return false;
  }
  if (chip_next_ != rhs.chip_next_) {
    return false;
  }
  if (bump_ != rhs.bump_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedRegion::operator<(const _dbUnfoldedRegion& rhs) const
{
  return true;
}

_dbUnfoldedRegion::_dbUnfoldedRegion(_dbDatabase* db)
{
  effective_side_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedRegion& obj)
{
  stream >> obj.chip_region_inst_;
  stream >> obj.effective_side_;
  stream >> obj.parent_chip_;
  stream >> obj.chip_next_;
  stream >> obj.bump_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedRegion& obj)
{
  stream << obj.chip_region_inst_;
  stream << obj.effective_side_;
  stream << obj.parent_chip_;
  stream << obj.chip_next_;
  stream << obj.bump_;
  return stream;
}

void _dbUnfoldedRegion::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedRegion - Methods
//
////////////////////////////////////////////////////////////////////

dbChipRegionInst* dbUnfoldedRegion::getChipRegionInst() const
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  if (obj->chip_region_inst_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipRegionInst*) par->chip_region_inst_tbl_->getPtr(
      obj->chip_region_inst_);
}

dbUnfoldedChip* dbUnfoldedRegion::getParentChip() const
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  if (obj->parent_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedChip*) par->unfolded_chip_tbl_->getPtr(obj->parent_chip_);
}

// User Code Begin dbUnfoldedRegionPublicMethods
Cuboid dbUnfoldedRegion::getCuboid() const
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipRegionInst* reg_inst
      = (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr(
          obj->chip_region_inst_);
  Cuboid c = reg_inst->getChipRegion()->getCuboid();
  _dbUnfoldedChip* parent = db->unfolded_chip_tbl_->getPtr(obj->parent_chip_);
  parent->transform_.apply(c);
  return c;
}

dbUnfoldedRegion::EffectiveSide dbUnfoldedRegion::getEffectiveSide() const
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  return static_cast<EffectiveSide>(obj->effective_side_);
}

void dbUnfoldedRegion::setEffectiveSide(EffectiveSide side)
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  obj->effective_side_ = static_cast<uint32_t>(side);
}

bool dbUnfoldedRegion::isTop() const
{
  return getEffectiveSide() == EffectiveSide::TOP;
}

bool dbUnfoldedRegion::isBottom() const
{
  return getEffectiveSide() == EffectiveSide::BOTTOM;
}

bool dbUnfoldedRegion::isInternal() const
{
  return getEffectiveSide() == EffectiveSide::INTERNAL;
}

bool dbUnfoldedRegion::isInternalExt() const
{
  return getEffectiveSide() == EffectiveSide::INTERNAL_EXT;
}

int dbUnfoldedRegion::getSurfaceZ() const
{
  Cuboid c = getCuboid();
  if (isTop()) {
    return c.zMax();
  }
  if (isBottom()) {
    return c.zMin();
  }
  return c.zCenter();
}

dbSet<dbUnfoldedBump> dbUnfoldedRegion::getBumps() const
{
  _dbUnfoldedRegion* obj = (_dbUnfoldedRegion*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return dbSet<dbUnfoldedBump>(obj, db->unfolded_bump_itr_);
}
// User Code End dbUnfoldedRegionPublicMethods
}  // namespace odb
// Generator Code End Cpp