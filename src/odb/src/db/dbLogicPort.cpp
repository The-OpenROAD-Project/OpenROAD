// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbLogicPort.h"

#include <cstdlib>
#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbIsolation.h"
#include "dbModInst.h"
#include "dbPowerSwitch.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbCommon.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbLogicPort>;

bool _dbLogicPort::operator==(const _dbLogicPort& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
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
  name_ = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbLogicPort& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.direction;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbLogicPort& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.direction;
  return stream;
}

void _dbLogicPort::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["direction"].add(direction);
  // User Code End collectMemInfo
}

_dbLogicPort::~_dbLogicPort()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
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
  if (_block->logicport_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbLogicPort* lp = _block->logicport_tbl_->create();
  lp->name_ = safe_strdup(name);

  if (direction.empty()) {
    lp->direction = "in";
  } else {
    lp->direction = direction;
  }

  _block->logicport_hash_.insert(lp);
  return (dbLogicPort*) lp;
}

void dbLogicPort::destroy(dbLogicPort* lp)
{
  // TODO
}

// User Code End dbLogicPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp
