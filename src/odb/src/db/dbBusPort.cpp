// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbBusPort.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbModuleBusPortModBTermItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbBusPort>;

bool _dbBusPort::operator==(const _dbBusPort& rhs) const
{
  if (_flags != rhs._flags) {
    return false;
  }
  if (_from != rhs._from) {
    return false;
  }
  if (_to != rhs._to) {
    return false;
  }
  if (_port != rhs._port) {
    return false;
  }
  if (_members != rhs._members) {
    return false;
  }
  if (_last != rhs._last) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }

  return true;
}

bool _dbBusPort::operator<(const _dbBusPort& rhs) const
{
  return true;
}

_dbBusPort::_dbBusPort(_dbDatabase* db)
{
  _flags = 0;
  _from = 0;
  _to = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._flags;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._from;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._to;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._port;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._members;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._last;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._parent;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._flags;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._from;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._to;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._port;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._members;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._last;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._parent;
  }
  return stream;
}

void _dbBusPort::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbBusPort::~_dbBusPort()
{
}

////////////////////////////////////////////////////////////////////
//
// dbBusPort - Methods
//
////////////////////////////////////////////////////////////////////

int dbBusPort::getFrom() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_from;
}

int dbBusPort::getTo() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->_to;
}

dbModBTerm* dbBusPort::getPort() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_port == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_port);
}

void dbBusPort::setMembers(dbModBTerm* members)
{
  _dbBusPort* obj = (_dbBusPort*) this;

  obj->_members = members->getImpl()->getOID();
}

dbModBTerm* dbBusPort::getMembers() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_members == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_members);
}

void dbBusPort::setLast(dbModBTerm* last)
{
  _dbBusPort* obj = (_dbBusPort*) this;

  obj->_last = last->getImpl()->getOID();
}

dbModBTerm* dbBusPort::getLast() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_last == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_last);
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

// User Code Begin dbBusPortPublicMethods

/*
  Get the indexed element. eg b[2] gets the element in the sequence
  at index 2. Note that depending on the from/to the offset
  of this index will change. eg
  b[1:5]
  element is at offset 1 in the array
  b[5:1] element is at offset 4 in the array
 */
dbModBTerm* dbBusPort::getBusIndexedElement(int index)
{
  int offset;
  if (getUpdown()) {
    offset = index - getFrom();
  } else {
    offset = getFrom() - index;
  }
  if (offset < getSize()) {
    //
    // TODO. Future pull request: support for making vector of objects unclean.
    // if we cannot count on the order, skip to the dbModBterm
    //
    // the _flags are set to non zero if we cannot
    // count on the order of the modbterms (eg
    // if some have been deleted or added in non-linear way).
    //
    /* This leads to wrong bus member access outside bus port
    if (obj->_flags == 0U) {
      _dbBlock* block_ = (_dbBlock*) obj->getOwner();
      _dbBusPort* obj = (_dbBusPort*) this;
      return (dbModBTerm*) (block_->_modbterm_tbl->getPtr(obj->getId() + offset
                                                          + 1));
    }
    */
    int i = 0;
    dbSet<dbModBTerm> busport_members = getBusPortMembers();
    for (auto cur : busport_members) {
      if (i == offset) {
        return cur;
      }
      i++;
    }
  }
  return nullptr;
}

int dbBusPort::getSize() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  // we keep how the size is computed in the
  // database low level object so that if
  // we need to use it during low level
  // iterators we can.
  return obj->size();
}

bool dbBusPort::getUpdown() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_to >= obj->_from) {
    return true;
  }
  return false;
}

dbBusPort* dbBusPort::create(dbModule* parentModule,
                             dbModBTerm* port,
                             int from_ix,
                             int to_ix)
{
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbBusPort* busport = block->_busport_tbl->create();
  busport->_port = port->getId();
  busport->_from = from_ix;
  busport->_to = to_ix;
  busport->_flags = 0U;
  busport->_parent = module->getOID();
  return (dbBusPort*) busport;
}

dbSet<dbModBTerm> dbBusPort::getBusPortMembers()
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->_members_iter == nullptr) {
    _dbBlock* block = (_dbBlock*) obj->getOwner();
    obj->_members_iter = new dbModuleBusPortModBTermItr(block->_modbterm_tbl);
  }
  return dbSet<dbModBTerm>(this, obj->_members_iter);
}

// User Code End dbBusPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp
