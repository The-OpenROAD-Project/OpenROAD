// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNameCache.h"

#include <cstdlib>
#include <cstring>

#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbHashTable.hpp"
#include "dbName.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"
#include "odb/dbSet.h"

namespace odb {

template class dbHashTable<_dbName>;
template class dbTable<_dbName>;

/////////////////
// _dbName
/////////////////
_dbName::_dbName(_dbDatabase*, const _dbName& n)
    : name_(nullptr), next_entry_(n.next_entry_), _ref_cnt(n._ref_cnt)
{
  if (n.name_) {
    name_ = safe_strdup(n.name_);
  }
}

_dbName::_dbName(_dbDatabase*)
{
  name_ = nullptr;
  _ref_cnt = 0;
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

  if (_ref_cnt != rhs._ref_cnt) {
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

  info.children_["name"].add(name_);
}

dbOStream& operator<<(dbOStream& stream, const _dbName& name)
{
  stream << name.name_;
  stream << name.next_entry_;
  stream << name._ref_cnt;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbName& name)
{
  stream >> name.name_;
  stream >> name.next_entry_;
  stream >> name._ref_cnt;
  return stream;
}

/////////////////
// _dbNameCache
/////////////////

_dbNameCache::_dbNameCache(_dbDatabase* db,
                           dbObject* owner,
                           dbObjectTable* (dbObject::*m)(dbObjectType))
{
  _name_tbl = new dbTable<_dbName>(db, owner, m, dbNameObj);

  _name_hash.setTable(_name_tbl);
}

_dbNameCache::~_dbNameCache()
{
  delete _name_tbl;
}

bool _dbNameCache::operator==(const _dbNameCache& rhs) const
{
  if (_name_hash != rhs._name_hash) {
    return false;
  }

  if (*_name_tbl != *rhs._name_tbl) {
    return false;
  }

  return true;
}

uint _dbNameCache::findName(const char* name)
{
  _dbName* n = _name_hash.find(name);

  if (n) {
    return n->getOID();
  }

  return 0U;
}

uint _dbNameCache::addName(const char* name)
{
  _dbName* n = _name_hash.find(name);

  if (n == nullptr) {
    n = _name_tbl->create();
    n->name_ = safe_strdup(name);
    _name_hash.insert(n);
  }

  ++n->_ref_cnt;
  return n->getOID();
}

void _dbNameCache::removeName(uint id)
{
  _dbName* n = _name_tbl->getPtr(id);
  --n->_ref_cnt;

  if (n->_ref_cnt == 0) {
    _name_hash.remove(n);
    _name_tbl->destroy(n);
  }
}

const char* _dbNameCache::getName(uint id)
{
  _dbName* n = _name_tbl->getPtr(id);
  return n->name_;
}

dbOStream& operator<<(dbOStream& stream, const _dbNameCache& cache)
{
  stream << cache._name_hash;
  stream << *cache._name_tbl;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNameCache& cache)
{
  stream >> cache._name_hash;
  stream >> *cache._name_tbl;
  return stream;
}

void _dbNameCache::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  _name_tbl->collectMemInfo(info.children_["name_tbl"]);
  info.children_["name_hash"].add(_name_hash);
}

}  // namespace odb
