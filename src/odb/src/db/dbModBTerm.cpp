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
#include "dbModBTerm.h"

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbModBTerm>;

bool _dbModBTerm::operator==(const _dbModBTerm& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_flags != rhs._flags) {
    return false;
  }
  if (_parent_moditerm != rhs._parent_moditerm) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_modnet != rhs._modnet) {
    return false;
  }
  if (_next_net_modbterm != rhs._next_net_modbterm) {
    return false;
  }
  if (_prev_net_modbterm != rhs._prev_net_modbterm) {
    return false;
  }
  if (_busPort != rhs._busPort) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_prev_entry != rhs._prev_entry) {
    return false;
  }

  return true;
}

bool _dbModBTerm::operator<(const _dbModBTerm& rhs) const
{
  return true;
}

void _dbModBTerm::differences(dbDiff& diff,
                              const char* field,
                              const _dbModBTerm& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags);
  DIFF_FIELD(_parent_moditerm);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_modnet);
  DIFF_FIELD(_next_net_modbterm);
  DIFF_FIELD(_prev_net_modbterm);
  DIFF_FIELD(_busPort);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_prev_entry);
  DIFF_END
}

void _dbModBTerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags);
  DIFF_OUT_FIELD(_parent_moditerm);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_modnet);
  DIFF_OUT_FIELD(_next_net_modbterm);
  DIFF_OUT_FIELD(_prev_net_modbterm);
  DIFF_OUT_FIELD(_busPort);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_prev_entry);

  DIFF_END
}

_dbModBTerm::_dbModBTerm(_dbDatabase* db)
{
  _name = nullptr;
  _flags = 0;
}

_dbModBTerm::_dbModBTerm(_dbDatabase* db, const _dbModBTerm& r)
{
  _name = r._name;
  _flags = r._flags;
  _parent_moditerm = r._parent_moditerm;
  _parent = r._parent;
  _modnet = r._modnet;
  _next_net_modbterm = r._next_net_modbterm;
  _prev_net_modbterm = r._prev_net_modbterm;
  _busPort = r._busPort;
  _next_entry = r._next_entry;
  _prev_entry = r._prev_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._flags;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._parent_moditerm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modnet;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._next_net_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._prev_net_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream >> obj._busPort;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._next_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream >> obj._prev_entry;
  }
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(db_schema_db_remove_hash)) {
    dbDatabase* db = (dbDatabase*) (obj.getDatabase());
    _dbBlock* block = (_dbBlock*) (db->getChip()->getBlock());
    _dbModule* module = block->_module_tbl->getPtr(obj._parent);
    if (obj._name) {
      module->_modbterm_hash[obj._name] = obj.getId();
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._flags;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._parent_moditerm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modnet;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._next_net_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._prev_net_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_odb_busport)) {
    stream << obj._busPort;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._next_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream << obj._prev_entry;
  }
  return stream;
}

_dbModBTerm::~_dbModBTerm()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModBTerm - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModBTerm::getName() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  return obj->_name;
}

dbModule* dbModBTerm::getParent() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

// User Code Begin dbModBTermPublicMethods

void dbModBTerm::setModNet(dbModNet* modNet)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_modnet = modNet->getImpl()->getOID();
}

dbModNet* dbModBTerm::getModNet() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_modnet == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModNet*) par->_modnet_tbl->getPtr(obj->_modnet);
}

void dbModBTerm::setParentModITerm(dbModITerm* parent_pin)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  obj->_parent_moditerm = parent_pin->getImpl()->getOID();
}

dbModITerm* dbModBTerm::getParentModITerm() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_parent_moditerm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModITerm*) par->_moditerm_tbl->getPtr(obj->_parent_moditerm);
}

struct dbModBTermFlags_str
{
  dbIoType::Value _iotype : 4;
  dbSigType::Value _sigtype : 4;
  uint _spare_bits : 24;
};

union dbModBTermFlags
{
  struct dbModBTermFlags_str flags;
  uint uint_val;
};

void dbModBTerm::setSigType(const dbSigType& type)
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->_flags;
  cur_flags.flags._sigtype = type.getValue();
  _dbmodbterm->_flags = cur_flags.uint_val;
}

dbSigType dbModBTerm::getSigType()
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->_flags;
  return dbSigType(cur_flags.flags._sigtype);
}

void dbModBTerm::setIoType(const dbIoType& type)
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->_flags;
  cur_flags.flags._iotype = type.getValue();
  _dbmodbterm->_flags = cur_flags.uint_val;
}

dbIoType dbModBTerm::getIoType()
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->_flags;
  return dbIoType(cur_flags.flags._iotype);
}

dbModBTerm* dbModBTerm::create(dbModule* parentModule, const char* name)
{
  dbModBTerm* ret = parentModule->findModBTerm(name);
  if (ret) {
    return ret;
  }

  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  std::string str_name(name);
  _dbModBTerm* modbterm = block->_modbterm_tbl->create();
  // defaults
  modbterm->_flags = 0U;
  ((dbModBTerm*) modbterm)->setIoType(dbIoType::INPUT);
  ((dbModBTerm*) modbterm)->setSigType(dbSigType::SIGNAL);
  modbterm->_modnet = 0;
  modbterm->_next_net_modbterm = 0;
  modbterm->_prev_net_modbterm = 0;
  modbterm->_busPort = 0;
  modbterm->_name = strdup(name);
  ZALLOCATED(modbterm->_name);
  modbterm->_parent = module->getOID();
  modbterm->_next_entry = module->_modbterms;
  modbterm->_prev_entry = 0;
  if (module->_modbterms != 0) {
    _dbModBTerm* new_next = block->_modbterm_tbl->getPtr(module->_modbterms);
    new_next->_prev_entry = modbterm->getOID();
  }
  module->_modbterms = modbterm->getOID();
  module->_modbterm_hash[name] = dbId<_dbModBTerm>(modbterm->getOID());

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbModBTermObj);
    block->_journal->pushParam(modbterm->getId());
    block->_journal->endAction();
  }

  return (dbModBTerm*) modbterm;
}

void dbModBTerm::connect(dbModNet* net)
{
  _dbModule* _module = (_dbModule*) (net->getParent());
  _dbBlock* _block = (_dbBlock*) _module->getOwner();
  _dbModNet* _modnet = (_dbModNet*) net;
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  // already connected
  if (_modbterm->_modnet == net->getId()) {
    return;
  }
  _modbterm->_modnet = net->getId();
  // append to net mod bterms. Do this by pushing onto head of list.
  if (_modnet->_modbterms != 0) {
    _dbModBTerm* head = _block->_modbterm_tbl->getPtr(_modnet->_modbterms);
    // next is old head
    _modbterm->_next_net_modbterm = _modnet->_modbterms;
    // previous for old head is this one
    head->_prev_net_modbterm = getId();
  } else {
    _modbterm->_next_net_modbterm = 0;  // only element
  }
  _modbterm->_prev_net_modbterm = 0;  // previous of head always zero
  _modnet->_modbterms = getId();      // set new head

  if (_block->_journal) {
    _block->_journal->beginAction(dbJournal::CONNECT_OBJECT);
    _block->_journal->pushParam(dbModBTermObj);
    _block->_journal->pushParam(getId());
    _block->_journal->pushParam(net->getId());
    _block->_journal->endAction();
  }
}

void dbModBTerm::disconnect()
{
  _dbModule* module = (_dbModule*) getParent();
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  if (_modbterm->_modnet == 0) {
    return;
  }
  _dbModNet* mod_net = block->_modnet_tbl->getPtr(_modbterm->_modnet);

  if (_modbterm->_prev_net_modbterm == 0) {
    // degenerate case, head element, need to update net starting point
    // and if next is null then make generate empty list
    mod_net->_modbterms = _modbterm->_next_net_modbterm;
  } else {
    _dbModBTerm* prev_modbterm
        = block->_modbterm_tbl->getPtr(_modbterm->_prev_net_modbterm);
    prev_modbterm->_next_net_modbterm
        = _modbterm->_next_net_modbterm;  // short out this element
  }
  if (_modbterm->_next_net_modbterm != 0) {
    _dbModBTerm* next_modbterm
        = block->_modbterm_tbl->getPtr(_modbterm->_next_net_modbterm);
    next_modbterm->_prev_net_modbterm = _modbterm->_prev_net_modbterm;
  }
  //
  // zero out this element for garbage collection
  // Note we can never rely on sequential order of modbterms for offsets.
  //
  _modbterm->_next_net_modbterm = 0;
  _modbterm->_prev_net_modbterm = 0;
}

bool dbModBTerm::isBusPort() const
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  return (_modbterm->_busPort != 0);
}

dbBusPort* dbModBTerm::getBusPort() const
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  if (_modbterm->_busPort != 0) {
    _dbBlock* block = (_dbBlock*) _modbterm->getOwner();
    return (dbBusPort*) block->_busport_tbl->getPtr(_modbterm->_busPort);
  }
  return nullptr;
}

void dbModBTerm::setBusPort(dbBusPort* bus_port)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  _modbterm->_busPort = bus_port->getId();
}

dbModBTerm* dbModBTerm::getModBTerm(dbBlock* block, uint dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbModBTerm*) (owner->_modbterm_tbl->getPtr(dbid));
}

void dbModBTerm::destroy(dbModBTerm* val)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) val;
  _dbBlock* block = (_dbBlock*) (_modbterm->getOwner());
  _dbModule* module = block->_module_tbl->getPtr(_modbterm->_parent);
  uint prev = _modbterm->_prev_entry;
  uint next = _modbterm->_next_entry;
  if (prev == 0) {
    // head of list
    module->_modbterms = next;
  } else {
    _dbModBTerm* prev_modbterm = block->_modbterm_tbl->getPtr(prev);
    prev_modbterm->_next_entry = next;
  }

  if (next != 0) {
    _dbModBTerm* next_modbterm = block->_modbterm_tbl->getPtr(next);
    next_modbterm->_prev_entry = prev;
  }
  _modbterm->_prev_entry = 0;
  _modbterm->_next_entry = 0;
  module->_modbterm_hash.erase(val->getName());
  block->_modbterm_tbl->destroy(_modbterm);
}

// User Code End dbModBTermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
