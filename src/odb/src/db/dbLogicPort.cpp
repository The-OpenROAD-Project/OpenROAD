// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbLogicPort.h"

#include <string>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbIsolation.h"
#include "dbModInst.h"
#include "dbPowerSwitch.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbLogicPort>;

bool _dbLogicPort::operator==(const _dbLogicPort& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (direction != rhs.direction) {
    return false;
  }

  return true;
}

bool _dbLogicPort::operator<(const _dbLogicPort& rhs) const
{
  return true;
}

_dbLogicPort::_dbLogicPort(_dbDatabase* db)
{
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbLogicPort& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj.direction;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbLogicPort& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj.direction;
  return stream;
}

void _dbLogicPort::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  info.children_["direction"].add(direction);
  // User Code End collectMemInfo
}

_dbLogicPort::~_dbLogicPort()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbLogicPort - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbLogicPort::getName() const
{
  _dbLogicPort* obj = (_dbLogicPort*) this;
  return obj->_name;
}

std::string dbLogicPort::getDirection() const
{
  _dbLogicPort* obj = (_dbLogicPort*) this;
  return obj->direction;
}

// User Code Begin dbLogicPortPublicMethods

dbLogicPort* dbLogicPort::create(dbBlock* block,
                                 const char* name,
                                 const std::string& direction)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_logicport_hash.hasMember(name)) {
    return nullptr;
  }
  _dbLogicPort* lp = _block->_logicport_tbl->create();
  lp->_name = strdup(name);
  ZALLOCATED(lp->_name);

  if (direction.empty()) {
    lp->direction = "in";
  } else {
    lp->direction = direction;
  }

  _block->_logicport_hash.insert(lp);
  return (dbLogicPort*) lp;
}

void dbLogicPort::destroy(dbLogicPort* lp)
{
  // TODO
}

// User Code End dbLogicPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp
