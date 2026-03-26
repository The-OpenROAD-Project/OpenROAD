// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipPath.h"

#include <cstdlib>
#include <tuple>
#include <vector>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes

#include <utility>

#include "dbCommon.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipPath>;

bool _dbChipPath::operator==(const _dbChipPath& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbChipPath::operator<(const _dbChipPath& rhs) const
{
  return true;
}

_dbChipPath::_dbChipPath(_dbDatabase* db)
{
  name_ = nullptr;
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
  info.children["name"].add(name_);
  info.children["entries"].add(entries_);
  // User Code End collectMemInfo
}

_dbChipPath::~_dbChipPath()
{
  if (name_) {
    free((void*) name_);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbChipPath - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbChipPath::getName() const
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

std::vector<dbChipPath::Entry> dbChipPath::getEntries() const
{
  _dbChipPath* obj = (_dbChipPath*) this;
  _dbChip* chip = (_dbChip*) obj->getOwner();
  _dbDatabase* db = (_dbDatabase*) chip->getOwner();

  // Translate internal tuple storage to public Entry objects
  std::vector<Entry> entries;
  entries.reserve(obj->entries_.size());
  for (const auto& [path_ids, region_id, neg] : obj->entries_) {
    Entry entry;
    entry.chip_inst_path.reserve(path_ids.size());
    for (const dbId<_dbChipInst>& inst_id : path_ids) {
      entry.chip_inst_path.push_back(
          (dbChipInst*) db->chip_inst_tbl_->getPtr(inst_id));
    }
    entry.region
        = (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr(region_id);
    entry.negated = neg;
    entries.push_back(std::move(entry));
  }
  return entries;
}

void dbChipPath::addEntry(const std::vector<dbChipInst*>& chip_inst_path,
                          dbChipRegionInst* region,
                          bool negated)
{
  _dbChipPath* obj = (_dbChipPath*) this;
  _dbChipRegionInst* _region = (_dbChipRegionInst*) region;

  std::vector<dbId<_dbChipInst>> path_ids;
  path_ids.reserve(chip_inst_path.size());
  for (dbChipInst* inst : chip_inst_path) {
    path_ids.emplace_back(((_dbChipInst*) inst)->getOID());
  }
  obj->entries_.emplace_back(std::move(path_ids), _region->getOID(), negated);
}

dbChipPath* dbChipPath::create(dbChip* chip, const char* name)
{
  _dbChip* _chip = (_dbChip*) chip;
  // Reject duplicate path names within the same chip
  if (_chip->chip_path_hash_.hasMember(name)) {
    _dbDatabase* db = (_dbDatabase*) _chip->getOwner();
    db->getLogger()->error(utl::ODB,
                           533,
                           "ChipPath {} already exists in chip {}",
                           name,
                           chip->getName());
  }
  _dbChipPath* chip_path = _chip->chip_path_tbl_->create();
  chip_path->name_ = safe_strdup(name);
  _chip->chip_path_hash_.insert(chip_path);
  return (dbChipPath*) chip_path;
}

void dbChipPath::destroy(dbChipPath* path)
{
  _dbChipPath* _path = (_dbChipPath*) path;
  _dbChip* chip = (_dbChip*) _path->getOwner();
  chip->chip_path_hash_.remove(_path);
  chip->chip_path_tbl_->destroy(_path);
}
// User Code End dbChipPathPublicMethods
}  // namespace odb
// Generator Code End Cpp
