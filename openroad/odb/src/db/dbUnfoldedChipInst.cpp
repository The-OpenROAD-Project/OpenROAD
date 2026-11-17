// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipInst.h"

#include <string>

#include "dbChipInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
// User Code Begin Includes
#include <vector>

#include "dbUnfoldedChipRegionInstItr.h"  // IWYU pragma: keep
// User Code End Includes
namespace odb {
template class dbTable<_dbUnfoldedChipInst>;

bool _dbUnfoldedChipInst::operator==(const _dbUnfoldedChipInst& rhs) const
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

bool _dbUnfoldedChipInst::operator<(const _dbUnfoldedChipInst& rhs) const
{
  return true;
}

_dbUnfoldedChipInst::_dbUnfoldedChipInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipInst& obj)
{
  stream >> obj.name_;
  stream >> obj.chip_inst_path_;
  stream >> obj.transform_;
  stream >> obj.region_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipInst& obj)
{
  stream << obj.name_;
  stream << obj.chip_inst_path_;
  stream << obj.transform_;
  stream << obj.region_;
  return stream;
}

void _dbUnfoldedChipInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["chip_inst_path"].add(chip_inst_path_);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipInst - Methods
//
////////////////////////////////////////////////////////////////////

const std::string& dbUnfoldedChipInst::getName() const
{
  _dbUnfoldedChipInst* obj = (_dbUnfoldedChipInst*) this;
  return obj->name_;
}

dbTransform dbUnfoldedChipInst::getTransform() const
{
  _dbUnfoldedChipInst* obj = (_dbUnfoldedChipInst*) this;
  return obj->transform_;
}

// User Code Begin dbUnfoldedChipInstPublicMethods
Cuboid dbUnfoldedChipInst::getCuboid() const
{
  _dbUnfoldedChipInst* obj = (_dbUnfoldedChipInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  dbChipInst* leaf
      = (dbChipInst*) db->chip_inst_tbl_->getPtr(obj->chip_inst_path_.back());
  Cuboid c = leaf->getMasterChip()->getCuboid();
  obj->transform_.apply(c);
  return c;
}

dbSet<dbUnfoldedChipRegionInst> dbUnfoldedChipInst::getRegions() const
{
  _dbUnfoldedChipInst* obj = (_dbUnfoldedChipInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return dbSet<dbUnfoldedChipRegionInst>(obj, db->unfolded_region_itr_);
}

dbUnfoldedChipRegionInst* dbUnfoldedChipInst::findRegion(
    dbChipRegionInst* source) const
{
  for (dbUnfoldedChipRegionInst* region : getRegions()) {
    if (region->getChipRegionInst() == source) {
      return region;
    }
  }
  return nullptr;
}

std::vector<dbChipInst*> dbUnfoldedChipInst::getChipInstPath() const
{
  _dbUnfoldedChipInst* obj = (_dbUnfoldedChipInst*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> path;
  path.reserve(obj->chip_inst_path_.size());
  for (const auto& id : obj->chip_inst_path_) {
    path.push_back((dbChipInst*) db->chip_inst_tbl_->getPtr(id));
  }
  return path;
}
// User Code End dbUnfoldedChipInstPublicMethods
}  // namespace odb
// Generator Code End Cpp