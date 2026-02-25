// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbBusPort.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbModuleBusPortModBTermItr.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include "odb/dbSet.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbBusPort>;

bool _dbBusPort::operator==(const _dbBusPort& rhs) const
{
  if (flags_ != rhs.flags_) {
    return false;
  }
  if (from_ != rhs.from_) {
    return false;
  }
  if (to_ != rhs.to_) {
    return false;
  }
  if (port_ != rhs.port_) {
    return false;
  }
  if (members_ != rhs.members_) {
    return false;
  }
  if (last_ != rhs.last_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
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
  flags_ = 0;
  from_ = 0;
  to_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj)
{
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.flags_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.from_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.to_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.port_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.members_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.last_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.parent_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj)
{
  stream << obj.flags_;
  stream << obj.from_;
  stream << obj.to_;
  stream << obj.port_;
  stream << obj.members_;
  stream << obj.last_;
  stream << obj.parent_;
  return stream;
}

void _dbBusPort::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbBusPort::~_dbBusPort()
{
  // User Code Begin Destructor
  delete members_iter_;
  // User Code End Destructor
}

////////////////////////////////////////////////////////////////////
//
// dbBusPort - Methods
//
////////////////////////////////////////////////////////////////////

int dbBusPort::getFrom() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->from_;
}

int dbBusPort::getTo() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  return obj->to_;
}

dbModBTerm* dbBusPort::getPort() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->port_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->modbterm_tbl_->getPtr(obj->port_);
}

void dbBusPort::setMembers(dbModBTerm* members)
{
  _dbBusPort* obj = (_dbBusPort*) this;

  obj->members_ = members->getImpl()->getOID();
}

dbModBTerm* dbBusPort::getMembers() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->members_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->modbterm_tbl_->getPtr(obj->members_);
}

void dbBusPort::setLast(dbModBTerm* last)
{
  _dbBusPort* obj = (_dbBusPort*) this;

  obj->last_ = last->getImpl()->getOID();
}

dbModBTerm* dbBusPort::getLast() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->last_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->modbterm_tbl_->getPtr(obj->last_);
}

dbModule* dbBusPort::getParent() const
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->parent_);
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
    // the flags_ are set to non zero if we cannot
    // count on the order of the modbterms (eg
    // if some have been deleted or added in non-linear way).
    //
    /* This leads to wrong bus member access outside bus port
    if (obj->flags_ == 0U) {
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
  if (obj->to_ >= obj->from_) {
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
  _dbBusPort* busport = block->busport_tbl_->create();
  busport->port_ = port->getId();
  busport->from_ = from_ix;
  busport->to_ = to_ix;
  busport->flags_ = 0U;
  busport->parent_ = module->getOID();
  return (dbBusPort*) busport;
}

dbSet<dbModBTerm> dbBusPort::getBusPortMembers()
{
  _dbBusPort* obj = (_dbBusPort*) this;
  if (obj->members_iter_ == nullptr) {
    _dbBlock* block = (_dbBlock*) obj->getOwner();
    obj->members_iter_ = new dbModuleBusPortModBTermItr(block->modbterm_tbl_);
  }
  return dbSet<dbModBTerm>(this, obj->members_iter_);
}

// User Code End dbBusPortPublicMethods
}  // namespace odb
   // Generator Code End Cpp
