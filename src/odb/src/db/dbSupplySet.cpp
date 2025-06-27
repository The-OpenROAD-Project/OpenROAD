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
#include "dbSupplyNet.h"
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
  if (_power_net != rhs._power_net) {
    return false;
  }
  if (_ground_net != rhs._ground_net) {
    return false;
  }
  if (_nwell_net != rhs._nwell_net) {
    return false;
  }
  if (_pwell_net != rhs._pwell_net) {
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
  stream >> obj._power_net;
  stream >> obj._ground_net;
  stream >> obj._nwell_net;
  stream >> obj._pwell_net;
  stream >> obj._group;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbSupplySet& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._power_net;
  stream << obj._ground_net;
  stream << obj._nwell_net;
  stream << obj._pwell_net;
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

void dbSupplySet::SetsupplyPwrNet(dbSupplyNet* net)
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  obj->_power_net = net->getImpl()->getOID();
}

dbSupplyNet* dbSupplySet::getsupplyPwrNet() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  if (obj->_power_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  return (dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_power_net);
}

void dbSupplySet::SetsupplyGndNet(dbSupplyNet* net)
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  obj->_ground_net = net->getImpl()->getOID();
}

dbSupplyNet* dbSupplySet::getsupplyGndNet() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  if (obj->_ground_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  return (dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_ground_net);
}
void dbSupplySet::SetsupplyNwellNet(dbSupplyNet* net)
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  obj->_nwell_net = net->getImpl()->getOID();
}

dbSupplyNet* dbSupplySet::getsupplyNwellNet() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  if (obj->_nwell_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  return (dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_nwell_net);
}
void dbSupplySet::SetsupplyPwellNet(dbSupplyNet* net)
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  obj->_pwell_net = net->getImpl()->getOID();
}

dbSupplyNet* dbSupplySet::getsupplyPwellNet() const
{
  _dbSupplySet* obj = (_dbSupplySet*) this;
  if (obj->_pwell_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  return (dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_pwell_net);
}

// User Code End dbSupplySetPublicMethods
}  // namespace odb
   // Generator Code End Cpp