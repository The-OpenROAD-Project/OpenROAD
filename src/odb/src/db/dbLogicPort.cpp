///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbLogicPort.h"

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
