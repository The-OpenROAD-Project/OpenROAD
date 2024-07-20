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
#include "dbBusPort.h"

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbBusPort>;

bool _dbBusPort::operator==(const _dbBusPort& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_start_ix != rhs._start_ix) {
    return false;
  }
  if (_size != rhs._size) {
    return false;
  }
  if (_updown != rhs._updown) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_firstmember != rhs._firstmember) {
    return false;
  }

  return true;
}

bool _dbBusPort::operator<(const _dbBusPort& rhs) const
{
  return true;
}

void _dbBusPort::differences(dbDiff& diff,
                             const char* field,
                             const _dbBusPort& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_start_ix);
  DIFF_FIELD(_size);
  DIFF_FIELD(_updown);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_firstmember);
  DIFF_END
}

void _dbBusPort::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_start_ix);
  DIFF_OUT_FIELD(_size);
  DIFF_OUT_FIELD(_updown);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_firstmember);

  DIFF_END
}

_dbBusPort::_dbBusPort(_dbDatabase* db)
{
  _name = nullptr;
  _start_ix = 0;
  _size = 0;
  _updown = false;
  _firstmember = 0;
}

_dbBusPort::_dbBusPort(_dbDatabase* db, const _dbBusPort& r)
{
  _name = r._name;
  _start_ix = r._start_ix;
  _size = r._size;
  _updown = r._updown;
  _parent = r._parent;
  _firstmember = r._firstmember;
}

dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._start_ix;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._size;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._updown;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._firstmember;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._start_ix;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._size;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._updown;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._firstmember;
  }
  return stream;
}

_dbBusPort::~_dbBusPort()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbBusPort - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbBusPort::getName() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_name;
}

int dbBusPort::getStartIx() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_start_ix;
}

int dbBusPort::getSize() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_size;
}

bool dbBusPort::isUpdown() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_updown;
}

dbModule* dbBusPort::getParent() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

dbModBTerm* dbBusPort::getFirstmember() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_firstmember == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_firstmember);
}

// User Code Begin dbBusPortPublicMethods
dbBusPort* dbBusPort::create(dbModule* parentModule,
                             const char* name,
                             int from_index,
                             bool updown,
                             int size)
{
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  std::string str_name(name);
  _dbBusPort* busport = block->_busport_tbl->create();
  busport->_start_ix = from_index;
  busport->_size = size;
  busport->_updown = updown;
  busport->_firstmember = 0;
  busport->_name = strdup(name);
  busport->_parent = module->getOID();
  ZALLOCATED(busport->_name);
  return (dbBusPort*) busport;
}

// note on connectivity model
// we assume a fully connected model at the bit level
// so a bus port is a partition on the bit level model.
// eg
// for ports: {a,bus[3:0],j}
// then the fully connected model is a,bus[3],bus[2],bus[1],bus[0],j
// and to get the index element 1 from busPort we use bus[3]+1 = bus[2].
//

dbModBTerm* dbBusPort::fetchIndexedPort(int offset)
{
  _dbBusPort* obj = (_dbBusPort*) this;
  _dbBlock* block_ = (_dbBlock*) obj->getOwner();
  if (obj->_firstmember == 0 || offset >= obj->_size)
    return nullptr;
  else {
    int id = obj->_firstmember;
    _dbModBTerm* element = block_->_modbterm_tbl->getPtr(id);
    for (int posn = 0; (element != nullptr) && (posn != offset); posn++) {
      id = element->_next_entry;
      element = block_->_modbterm_tbl->getPtr(id);
    }
    return (dbModBTerm*) element;
  }
  return nullptr;
}

// User Code End dbBusPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp
