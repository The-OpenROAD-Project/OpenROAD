// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBumpInst.h"

#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbChipBump.h"
#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipBumpInst>;

bool _dbChipBumpInst::operator==(const _dbChipBumpInst& rhs) const
{
  if (chip_bump_ != rhs.chip_bump_) {
    return false;
  }
  if (chip_region_inst_ != rhs.chip_region_inst_) {
    return false;
  }
  if (region_next_ != rhs.region_next_) {
    return false;
  }

  return true;
}

bool _dbChipBumpInst::operator<(const _dbChipBumpInst& rhs) const
{
  return true;
}

_dbChipBumpInst::_dbChipBumpInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipBumpInst& obj)
{
  stream >> obj.chip_bump_;
  stream >> obj.chip_region_inst_;
  stream >> obj.region_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipBumpInst& obj)
{
  stream << obj.chip_bump_;
  stream << obj.chip_region_inst_;
  stream << obj.region_next_;
  return stream;
}

void _dbChipBumpInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipBumpInst - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbChipBumpInstPublicMethods

dbChipBump* dbChipBumpInst::getChipBump() const
{
  _dbChipBumpInst* obj = (_dbChipBumpInst*) this;
  dbChipRegionInst* chip_region_inst = getChipRegionInst();
  _dbChipRegion* chip_region
      = (_dbChipRegion*) chip_region_inst->getChipRegion();
  return (dbChipBump*) chip_region->chip_bump_tbl_->getPtr(obj->chip_bump_);
}

dbChipRegionInst* dbChipBumpInst::getChipRegionInst() const
{
  _dbChipBumpInst* obj = (_dbChipBumpInst*) this;
  _dbDatabase* db = obj->getDatabase();
  return (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr(
      obj->chip_region_inst_);
}

// User Code End dbChipBumpInstPublicMethods
}  // namespace odb
// Generator Code End Cpp