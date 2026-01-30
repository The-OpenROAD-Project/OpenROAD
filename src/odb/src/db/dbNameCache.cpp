// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNameCache.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbHashTable.hpp"
#include "dbName.h"
#include "dbTable.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"

namespace odb {

template class dbHashTable<_dbName>;
template class dbTable<_dbName>;

/////////////////
// _dbName
/////////////////
_dbName::_dbName(_dbDatabase*, const _dbName& n)
    : name_(nullptr), next_entry_(n.next_entry_), ref_cnt_(n.ref_cnt_)
{
  if (n.name_) {
    name_ = safe_strdup(n.name_);
  }
}

_dbName::_dbName(_dbDatabase*)
{
  name_ = nullptr;
  ref_cnt_ = 0;
}

_dbName::~_dbName()
{
  if (name_) {
    free((void*) name_);
  }
}

bool _dbName::operator==(const _dbName& rhs) const
{
  if (strcmp(name_, rhs.name_) != 0) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (ref_cnt_ != rhs.ref_cnt_) {
    return false;
  }

  return true;
}

bool _dbName::operator<(const _dbName& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

void _dbName::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
}

dbOStream& operator<<(dbOStream& stream, const _dbName& name)
{
  stream << name.name_;
  stream << name.next_entry_;
  stream << name.ref_cnt_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbName& name)
{
  stream >> name.name_;
  stream >> name.next_entry_;
  stream >> name.ref_cnt_;
  return stream;
}

/////////////////
// _dbNameCache
/////////////////

_dbNameCache::_dbNameCache(_dbDatabase* db,
                           dbObject* owner,
                           dbObjectTable* (dbObject::*m)(dbObjectType))
{
  name_tbl_ = new dbTable<_dbName>(db, owner, m, dbNameObj);

  name_hash_.setTable(name_tbl_);
}

_dbNameCache::~_dbNameCache()
{
  delete name_tbl_;
}

bool _dbNameCache::operator==(const _dbNameCache& rhs) const
{
  if (name_hash_ != rhs.name_hash_) {
    return false;
  }

  if (*name_tbl_ != *rhs.name_tbl_) {
    return false;
  }

  return true;
}

uint32_t _dbNameCache::findName(const char* name)
{
  _dbName* n = name_hash_.find(name);

  if (n) {
    return n->getOID();
  }

  return 0U;
}

uint32_t _dbNameCache::addName(const char* name)
{
  _dbName* n = name_hash_.find(name);

  if (n == nullptr) {
    n = name_tbl_->create();
    n->name_ = safe_strdup(name);
    name_hash_.insert(n);
  }

  ++n->ref_cnt_;
  return n->getOID();
}

void _dbNameCache::removeName(uint32_t id)
{
  _dbName* n = name_tbl_->getPtr(id);
  --n->ref_cnt_;

  if (n->ref_cnt_ == 0) {
    name_hash_.remove(n);
    name_tbl_->destroy(n);
  }
}

const char* _dbNameCache::getName(uint32_t id)
{
  _dbName* n = name_tbl_->getPtr(id);
  return n->name_;
}

dbOStream& operator<<(dbOStream& stream, const _dbNameCache& cache)
{
  stream << cache.name_hash_;
  stream << *cache.name_tbl_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNameCache& cache)
{
  stream >> cache.name_hash_;
  stream >> *cache.name_tbl_;
  return stream;
}

void _dbNameCache::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  name_tbl_->collectMemInfo(info.children["name_tbl"]);
  info.children["name_hash"].add(name_hash_);
}

}  // namespace odb
