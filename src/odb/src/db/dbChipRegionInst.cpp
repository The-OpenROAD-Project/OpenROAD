// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegionInst.h"

#include "dbChipRegion.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipBumpInstItr.h"
#include "dbChipInst.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipRegionInst>;

bool _dbChipRegionInst::operator==(const _dbChipRegionInst& rhs) const
{
  if (region_ != rhs.region_) {
    return false;
  }
  if (parent_chipinst_ != rhs.parent_chipinst_) {
    return false;
  }
  if (chip_region_inst_next_ != rhs.chip_region_inst_next_) {
    return false;
  }
  if (chip_bump_insts_ != rhs.chip_bump_insts_) {
    return false;
  }

  return true;
}

bool _dbChipRegionInst::operator<(const _dbChipRegionInst& rhs) const
{
  return true;
}

_dbChipRegionInst::_dbChipRegionInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipRegionInst& obj)
{
  stream >> obj.region_;
  stream >> obj.parent_chipinst_;
  stream >> obj.chip_region_inst_next_;
  stream >> obj.chip_bump_insts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipRegionInst& obj)
{
  stream << obj.region_;
  stream << obj.parent_chipinst_;
  stream << obj.chip_region_inst_next_;
  stream << obj.chip_bump_insts_;
  return stream;
}

void _dbChipRegionInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipRegionInst - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbChipRegionInstPublicMethods

// Returns the region's cuboid transformed into the parent chip's
// coordinate system.
Cuboid dbChipRegionInst::getCuboid() const
{
  Cuboid cuboid = getChipRegion()->getCuboid();
  getChipInst()->getTransform().apply(cuboid);
  return cuboid;
}

dbChipInst* dbChipRegionInst::getChipInst() const
{
  _dbChipRegionInst* obj = (_dbChipRegionInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner()->getImpl();
  return (dbChipInst*) db->chip_inst_tbl_->getPtr(obj->parent_chipinst_);
}

dbChipRegion* dbChipRegionInst::getChipRegion() const
{
  _dbChipRegionInst* obj = (_dbChipRegionInst*) this;
  auto chip_inst = getChipInst();
  _dbChip* chip = (_dbChip*) chip_inst->getMasterChip();
  return (dbChipRegion*) chip->chip_region_tbl_->getPtr(obj->region_);
}

dbSet<dbChipBumpInst> dbChipRegionInst::getChipBumpInsts() const
{
  _dbChipRegionInst* obj = (_dbChipRegionInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return dbSet<dbChipBumpInst>(obj, db->chip_bump_inst_itr_);
}

// User Code End dbChipRegionInstPublicMethods
}  // namespace odb
// Generator Code End Cpp