// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbSupplySet.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbHashTable.hpp"
#include "dbModInst.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbSupplySet>;

bool _dbSupplySet::operator==(const _dbSupplySet& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_power_nets != rhs._power_nets) {
    return false;
  }
  if (_nwellnets != rhs._nwellnets) {
    return false;
  }
  if (_pwellnets != rhs._pwellnets) {
    return false;
  }
  if (_group != rhs._group) {
    return false;
  }

  return true;
}

bool _dbSupplySet::operator<(const _dbSupplySet& rhs) const
{
  return true;
}

_dbSupplySet::_dbSupplySet(_dbDatabase* db)
{
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbSupplySet& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._power_nets;
  stream >> obj._ground_nets;
  stream >> obj._nwellnets;
  stream >> obj._pwellnets;
  stream >> obj._group;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbSupplySet& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._power_nets;
  stream << obj._ground_nets;
  stream << obj._nwellnets;
  stream << obj._pwellnets;
  stream << obj._group;
  return stream;
}

void _dbSupplySet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbSupplySet::~_dbSupplySet()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbSupplySet - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbSupplySet::getName() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  return obj->_name;
}

dbGroup* dbSupplySet::getGroup() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  if (obj->_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_group);
}

// User Code Begin dbSupplySetPublicMethods

dbSupplySet* dbSupplySet::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_supplyset_hash.hasMember(name)) {
    return nullptr;
  }
  _dbSupplySet* obj = _block->_supplyset_tbl->create();
  obj->_name = strdup(name);
  obj->_group = 0;
  ZALLOCATED(obj->_name);

  _block->_supplyset_hash.insert(obj);
  return (dbSupplySet*) obj;
}

// User Code End dbSupplySetPublicMethods
}  // namespace odb
   // Generator Code End Cpp