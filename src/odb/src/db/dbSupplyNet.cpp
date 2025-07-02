// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbSupplyNet.h"

#include <string>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbSupplyPort.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbGroup.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbSupplyNet>;

bool _dbSupplyNet::operator==(const _dbSupplyNet& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_direction != rhs._direction) {
    return false;
  }
  if (_group != rhs._group) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_in != rhs._in) {
    return false;
  }
  if (_out != rhs._out) {
    return false;
  }

  return true;
}

bool _dbSupplyNet::operator<(const _dbSupplyNet& rhs) const
{
  return true;
}

_dbSupplyNet::_dbSupplyNet(_dbDatabase* db)
{
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbSupplyNet& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._direction;
  stream >> obj._group;
  stream >> obj._parent;
  stream >> obj._in;
  stream >> obj._out;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbSupplyNet& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._direction;
  stream << obj._group;
  stream << obj._parent;
  stream << obj._in;
  stream << obj._out;
  return stream;
}

void _dbSupplyNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbSupplyNet::~_dbSupplyNet()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbSupplyNet - Methods
//
////////////////////////////////////////////////////////////////////

char* dbSupplyNet::getName() const
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;
  return obj->_name;
}

std::string dbSupplyNet::getDirection() const
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;
  return obj->_direction;
}

void dbSupplyNet::setGroup(dbGroup* group)
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;

  obj->_group = group->getImpl()->getOID();
}

dbGroup* dbSupplyNet::getGroup() const
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;
  if (obj->_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_group);
}

void dbSupplyNet::setParent(dbSupplyNet* parent)
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;

  obj->_parent = parent->getImpl()->getOID();
}

dbSupplyNet* dbSupplyNet::getParent() const
{
  _dbSupplyNet* obj = (_dbSupplyNet*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_parent);
}

// User Code Begin dbSupplyNetPublicMethods

dbSupplyNet* dbSupplyNet::create(dbBlock* block,
                                 const char* direction,
                                 const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_supplynet_hash.hasMember(name)) {
    return nullptr;
  }
  _dbSupplyNet* sn = _block->_supplynet_tbl->create();
  sn->_name = safe_strdup(name);
 

  _block->_supplynet_hash.insert(sn);

  return (dbSupplyNet*) sn;
}

// User Code End dbSupplyNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp