///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbNameCache.h"

#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbName.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbSet.h"

namespace odb {

template class dbHashTable<_dbName>;
template class dbTable<_dbName>;

/////////////////
// _dbName
/////////////////
_dbName::_dbName(_dbDatabase*, const _dbName& n)
    : _name(nullptr), _next_entry(n._next_entry), _ref_cnt(n._ref_cnt)
{
  if (n._name) {
    _name = strdup(n._name);
    ZALLOCATED(_name);
  }
}

_dbName::_dbName(_dbDatabase*)
{
  _name = nullptr;
  _ref_cnt = 0;
}

_dbName::~_dbName()
{
  if (_name) {
    free((void*) _name);
  }
}

bool _dbName::operator==(const _dbName& rhs) const
{
  if (strcmp(_name, rhs._name) != 0) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_ref_cnt != rhs._ref_cnt) {
    return false;
  }

  return true;
}

bool _dbName::operator<(const _dbName& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

void _dbName::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
}

dbOStream& operator<<(dbOStream& stream, const _dbName& name)
{
  stream << name._name;
  stream << name._next_entry;
  stream << name._ref_cnt;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbName& name)
{
  stream >> name._name;
  stream >> name._next_entry;
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
    n->_name = strdup(name);
    ZALLOCATED(n->_name);
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
  return n->_name;
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
