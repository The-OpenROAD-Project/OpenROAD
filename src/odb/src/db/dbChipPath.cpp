// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipPath.h"

#include <string>
#include <utility>
#include <vector>

#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbChipPath>;

bool _dbChipPath::operator==(const _dbChipPath& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }

  return true;
}

bool _dbChipPath::operator<(const _dbChipPath& rhs) const
{
  return true;
}

_dbChipPath::_dbChipPath(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipPath& obj)
{
  stream >> obj.name_;
  stream >> obj.entries_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipPath& obj)
{
  stream << obj.name_;
  stream << obj.entries_;
  return stream;
}

void _dbChipPath::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.size += name_.capacity();
  info.size += entries_.capacity() * sizeof(std::pair<std::string, bool>);
  for (const auto& [region, _] : entries_) {
    info.size += region.capacity();
  }
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbChipPath - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChipPath::getName() const
{
  _dbChipPath* obj = (_dbChipPath*) this;
  return obj->name_;
}

// User Code Begin dbChipPathPublicMethods
dbChip* dbChipPath::getChip() const
{
  _dbChipPath* obj = (_dbChipPath*) this;
  return (dbChip*) obj->getOwner();
}

const std::vector<std::pair<std::string, bool>>& dbChipPath::getEntries() const
{
  _dbChipPath* obj = (_dbChipPath*) this;
  return obj->entries_;
}

void dbChipPath::addEntry(const std::string& region, bool negated)
{
  _dbChipPath* obj = (_dbChipPath*) this;
  obj->entries_.emplace_back(region, negated);
}

dbChipPath* dbChipPath::create(dbChip* chip, const std::string& name)
{
  _dbChip* _chip = (_dbChip*) chip;

  _dbChipPath* chip_path = _chip->chip_path_tbl_->create();
  chip_path->name_ = name;

  return (dbChipPath*) chip_path;
}

void dbChipPath::destroy(dbChipPath* path)
{
  _dbChipPath* _path = (_dbChipPath*) path;
  _dbChip* chip = (_dbChip*) _path->getOwner();
  chip->chip_path_tbl_->destroy(_path);
}
// User Code End dbChipPathPublicMethods
}  // namespace odb
// Generator Code End Cpp
