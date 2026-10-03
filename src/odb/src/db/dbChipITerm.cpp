// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipITerm.h"

#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbChipBTerm.h"
#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipITerm>;

bool _dbChipITerm::operator==(const _dbChipITerm& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_bterm_ != rhs.chip_bterm_) {
    return false;
  }
  if (chip_region_inst_ != rhs.chip_region_inst_) {
    return false;
  }
  if (region_next_ != rhs.region_next_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbChipITerm::operator<(const _dbChipITerm& rhs) const
{
  return true;
}

_dbChipITerm::_dbChipITerm(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipITerm& obj)
{
  stream >> obj.chip_bterm_;
  stream >> obj.chip_region_inst_;
  stream >> obj.region_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipITerm& obj)
{
  stream << obj.chip_bterm_;
  stream << obj.chip_region_inst_;
  stream << obj.region_next_;
  return stream;
}

void _dbChipITerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipITerm - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbChipITermPublicMethods

dbChipBTerm* dbChipITerm::getChipBTerm() const
{
  _dbChipITerm* obj = (_dbChipITerm*) this;
  dbChipRegionInst* chip_region_inst = getChipRegionInst();
  _dbChipRegion* chip_region
      = (_dbChipRegion*) chip_region_inst->getChipRegion();
  return (dbChipBTerm*) chip_region->chip_bterm_tbl_->getPtr(obj->chip_bterm_);
}

dbChipRegionInst* dbChipITerm::getChipRegionInst() const
{
  _dbChipITerm* obj = (_dbChipITerm*) this;
  _dbDatabase* db = obj->getDatabase();
  return (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr(
      obj->chip_region_inst_);
}

// User Code End dbChipITermPublicMethods
}  // namespace odb
// Generator Code End Cpp