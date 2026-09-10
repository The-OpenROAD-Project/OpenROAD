// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipRegionInst.h"

#include <cstdint>
#include <cstring>

#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedChipInst.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
// User Code Begin Includes
#include "dbUnfoldedChipBumpInstItr.h"  // IWYU pragma: keep
// User Code End Includes
namespace odb {
template class dbTable<_dbUnfoldedChipRegionInst>;

bool _dbUnfoldedChipRegionInst::operator==(
    const _dbUnfoldedChipRegionInst& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.effective_side_ != rhs.flags_.effective_side_) {
    return false;
  }
  if (chip_region_inst_ != rhs.chip_region_inst_) {
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

bool _dbUnfoldedChipRegionInst::operator<(
    const _dbUnfoldedChipRegionInst& rhs) const
{
  return true;
}

_dbUnfoldedChipRegionInst::_dbUnfoldedChipRegionInst(_dbDatabase* db)
{
  flags_ = {};
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipRegionInst& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.chip_region_inst_;
  stream >> obj.parent_chip_;
  stream >> obj.chip_next_;
  stream >> obj.bump_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipRegionInst& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.chip_region_inst_;
  stream << obj.parent_chip_;
  stream << obj.chip_next_;
  stream << obj.bump_;
  return stream;
}

void _dbUnfoldedChipRegionInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipRegionInst - Methods
//
////////////////////////////////////////////////////////////////////

dbChipRegionInst* dbUnfoldedChipRegionInst::getChipRegionInst() const
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  if (obj->chip_region_inst_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipRegionInst*) par->chip_region_inst_tbl_->getPtr(
      obj->chip_region_inst_);
}

dbUnfoldedChipInst* dbUnfoldedChipRegionInst::getParentChip() const
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  if (obj->parent_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedChipInst*) par->unfolded_chip_inst_tbl_->getPtr(
      obj->parent_chip_);
}

// User Code Begin dbUnfoldedChipRegionInstPublicMethods
Cuboid dbUnfoldedChipRegionInst::getCuboid() const
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipRegionInst* reg_inst
      = (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr(
          obj->chip_region_inst_);
  Cuboid c = reg_inst->getChipRegion()->getCuboid();
  _dbUnfoldedChipInst* parent
      = db->unfolded_chip_inst_tbl_->getPtr(obj->parent_chip_);
  parent->transform_.apply(c);
  return c;
}

dbUnfoldedChipRegionInst::EffectiveSide
dbUnfoldedChipRegionInst::getEffectiveSide() const
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  return static_cast<EffectiveSide>(obj->flags_.effective_side_);
}

void dbUnfoldedChipRegionInst::setEffectiveSide(EffectiveSide side)
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  obj->flags_.effective_side_ = static_cast<uint32_t>(side);
}

bool dbUnfoldedChipRegionInst::isTop() const
{
  return getEffectiveSide() == EffectiveSide::TOP;
}

bool dbUnfoldedChipRegionInst::isBottom() const
{
  return getEffectiveSide() == EffectiveSide::BOTTOM;
}

bool dbUnfoldedChipRegionInst::isInternal() const
{
  return getEffectiveSide() == EffectiveSide::INTERNAL;
}

bool dbUnfoldedChipRegionInst::isInternalExt() const
{
  return getEffectiveSide() == EffectiveSide::INTERNAL_EXT;
}

int dbUnfoldedChipRegionInst::getSurfaceZ() const
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

dbSet<dbUnfoldedChipBumpInst> dbUnfoldedChipRegionInst::getBumps() const
{
  _dbUnfoldedChipRegionInst* obj = (_dbUnfoldedChipRegionInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return dbSet<dbUnfoldedChipBumpInst>(obj, db->unfolded_bump_itr_);
}
// User Code End dbUnfoldedChipRegionInstPublicMethods
}  // namespace odb
// Generator Code End Cpp