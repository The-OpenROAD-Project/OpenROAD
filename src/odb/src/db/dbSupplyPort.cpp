// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbSupplyPort.h"

#include <string>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbPowerDomain.h"
#include "dbSupplyNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbSupplyPort>;

bool _dbSupplyPort::operator==(const _dbSupplyPort& rhs) const
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
  if (_domain != rhs._domain) {
    return false;
  }
  if (_supplynet != rhs._supplynet) {
    return false;
  }

  return true;
}

bool _dbSupplyPort::operator<(const _dbSupplyPort& rhs) const
{
  return true;
}

_dbSupplyPort::_dbSupplyPort(_dbDatabase* db)
{
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbSupplyPort& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._direction;
  stream >> obj._domain;
  stream >> obj._supplynet;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbSupplyPort& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._direction;
  stream << obj._domain;
  stream << obj._supplynet;
  return stream;
}

void _dbSupplyPort::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbSupplyPort::~_dbSupplyPort()
{
  if (_name) {
    free((void*) _name);
  }
  // User Code Begin Destructor
  // delete _domain;
  // delete _direction;
  // User Code End Destructor
}

////////////////////////////////////////////////////////////////////
//
// dbSupplyPort - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbSupplyPort::getName() const
{
  _dbSupplyPort* obj = (_dbSupplyPort*) this;
  return obj->_name;
}

std::string dbSupplyPort::getDirection() const
{
  _dbSupplyPort* obj = (_dbSupplyPort*) this;
  return obj->_direction;
}

dbPowerDomain* dbSupplyPort::getDomain() const
{
  _dbSupplyPort* obj = (_dbSupplyPort*) this;
  if (obj->_domain == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->_powerdomain_tbl->getPtr(obj->_domain);
}

// User Code Begin dbSupplyPortPublicMethods
dbSupplyPort* dbSupplyPort::create(dbBlock* block,
                                   const char* direction,
                                   dbPowerDomain* pd,
                                   const char* supplyport)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_supplyport_hash.hasMember(supplyport)) {
    return nullptr;
  }

  _dbSupplyPort* sp = _block->_supplyport_tbl->create();
  sp->_name = strdup(supplyport);
  ZALLOCATED(sp->_name);
  sp->_domain = pd->getImpl()->getOID();
  sp->_direction = strdup(direction);
  // ZALLOCATED(sp->_direction);
  _block->_supplyport_hash.insert(sp);
  return (dbSupplyPort*) sp;
}

bool dbSupplyPort::connectPort(dbSupplyNet* supply_net)
{
  _dbSupplyNet* sn = (_dbSupplyNet*) supply_net;
  _dbSupplyPort* sp = (_dbSupplyPort*) this;

  // if (sn->_in.isValid()) {
  if (!sn->_in.isValid()) {
    sn->_in = sp->getImpl()->getOID();
  } else {
    return false;  // already connected
  }
  sp->_supplynet = sn->getImpl()->getOID();

  return true;
}

dbSupplyNet* dbSupplyPort::getConnectedSupplyNet() const
{
  _dbSupplyPort* obj = (_dbSupplyPort*) this;
  if (!obj->_supplynet.isValid()) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  return (odb::dbSupplyNet*) par->_supplynet_tbl->getPtr(obj->_supplynet);
}
// User Code End dbSupplyPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp