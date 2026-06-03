// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChip.h"

#include <string>

#include "dbChipInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbUnfoldedRegionItr.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbUnfoldedChip>;

bool _dbUnfoldedChip::operator==(const _dbUnfoldedChip& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (name_ != rhs.name_) {
    return false;
  }
  if (region_ != rhs.region_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedChip::operator<(const _dbUnfoldedChip& rhs) const
{
  return true;
}

_dbUnfoldedChip::_dbUnfoldedChip(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChip& obj)
{
  stream >> obj.name_;
  stream >> obj.chip_inst_path_;
  stream >> obj.transform_;
  stream >> obj.region_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChip& obj)
{
  stream << obj.name_;
  stream << obj.chip_inst_path_;
  stream << obj.transform_;
  stream << obj.region_;
  return stream;
}

void _dbUnfoldedChip::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["chip_inst_path"].add(chip_inst_path_);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChip - Methods
//
////////////////////////////////////////////////////////////////////

const std::string& dbUnfoldedChip::getName() const
{
  _dbUnfoldedChip* obj = (_dbUnfoldedChip*) this;
  return obj->name_;
}

dbTransform dbUnfoldedChip::getTransform() const
{
  _dbUnfoldedChip* obj = (_dbUnfoldedChip*) this;
  return obj->transform_;
}

// User Code Begin dbUnfoldedChipPublicMethods
Cuboid dbUnfoldedChip::getCuboid() const
{
  _dbUnfoldedChip* obj = (_dbUnfoldedChip*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipInst* leaf
      = (dbChipInst*) db->chip_inst_tbl_->getPtr(obj->chip_inst_path_.back());
  Cuboid c = leaf->getMasterChip()->getCuboid();
  obj->transform_.apply(c);
  return c;
}

dbSet<dbUnfoldedRegion> dbUnfoldedChip::getRegions() const
{
  _dbUnfoldedChip* obj = (_dbUnfoldedChip*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return dbSet<dbUnfoldedRegion>(obj, db->unfolded_region_itr_);
}

dbUnfoldedRegion* dbUnfoldedChip::findRegion(dbChipRegionInst* source) const
{
  for (dbUnfoldedRegion* region : getRegions()) {
    if (region->getChipRegionInst() == source) {
      return region;
    }
  }
  return nullptr;
}

std::vector<dbChipInst*> dbUnfoldedChip::getChipInstPath() const
{
  _dbUnfoldedChip* obj = (_dbUnfoldedChip*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> path;
  path.reserve(obj->chip_inst_path_.size());
  for (const auto& id : obj->chip_inst_path_) {
    path.push_back((dbChipInst*) db->chip_inst_tbl_->getPtr(id));
  }
  return path;
}
// User Code End dbUnfoldedChipPublicMethods
}  // namespace odb
// Generator Code End Cpp