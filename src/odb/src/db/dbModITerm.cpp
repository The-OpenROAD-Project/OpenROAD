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
#include "dbModITerm.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbModITerm>;

bool _dbModITerm::operator==(const _dbModITerm& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_child_modbterm != rhs._child_modbterm) {
    return false;
  }
  if (_mod_net != rhs._mod_net) {
    return false;
  }
  if (_next_net_moditerm != rhs._next_net_moditerm) {
    return false;
  }
  if (_prev_net_moditerm != rhs._prev_net_moditerm) {
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

bool _dbModITerm::operator<(const _dbModITerm& rhs) const
{
  return true;
}

void _dbModITerm::differences(dbDiff& diff,
                              const char* field,
                              const _dbModITerm& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_child_modbterm);
  DIFF_FIELD(_mod_net);
  DIFF_FIELD(_next_net_moditerm);
  DIFF_FIELD(_prev_net_moditerm);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_prev_entry);
  DIFF_END
}

void _dbModITerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_child_modbterm);
  DIFF_OUT_FIELD(_mod_net);
  DIFF_OUT_FIELD(_next_net_moditerm);
  DIFF_OUT_FIELD(_prev_net_moditerm);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_prev_entry);

  DIFF_END
}

_dbModITerm::_dbModITerm(_dbDatabase* db)
{
  _name = nullptr;
}

_dbModITerm::_dbModITerm(_dbDatabase* db, const _dbModITerm& r)
{
  _name = r._name;
  _parent = r._parent;
  _child_modbterm = r._child_modbterm;
  _mod_net = r._mod_net;
  _next_net_moditerm = r._next_net_moditerm;
  _prev_net_moditerm = r._prev_net_moditerm;
  _next_entry = r._next_entry;
  _prev_entry = r._prev_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbModITerm& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._child_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._mod_net;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._next_net_moditerm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._prev_net_moditerm;
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
    _dbModInst* mod_inst = block->_modinst_tbl->getPtr(obj._parent);
    if (obj._name) {
      mod_inst->_moditerm_hash[obj._name] = dbId<_dbModITerm>(obj.getId());
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModITerm& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._child_modbterm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._mod_net;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._next_net_moditerm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._prev_net_moditerm;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._next_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream << obj._prev_entry;
  }
  return stream;
}

_dbModITerm::~_dbModITerm()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModITerm - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModITerm::getName() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  return obj->_name;
}

dbModInst* dbModITerm::getParent() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->_modinst_tbl->getPtr(obj->_parent);
}

// User Code Begin dbModITermPublicMethods

void dbModITerm::setModNet(dbModNet* modNet)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_mod_net = modNet->getImpl()->getOID();
}

dbModNet* dbModITerm::getModNet() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_mod_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModNet*) par->_modnet_tbl->getPtr(obj->_mod_net);
}

void dbModITerm::setChildModBTerm(dbModBTerm* child_port)
{
  _dbModITerm* obj = (_dbModITerm*) this;
  obj->_child_modbterm = child_port->getImpl()->getOID();
}

dbModBTerm* dbModITerm::getChildModBTerm() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_child_modbterm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_child_modbterm);
}

dbModITerm* dbModITerm::create(dbModInst* parentInstance, const char* name)
{
  _dbModInst* parent = (_dbModInst*) parentInstance;
  _dbBlock* block = (_dbBlock*) parent->getOwner();
  _dbModITerm* moditerm = block->_moditerm_tbl->create();
  // defaults
  moditerm->_mod_net = 0;
  moditerm->_next_net_moditerm = 0;
  moditerm->_prev_net_moditerm = 0;

  moditerm->_name = strdup(name);
  ZALLOCATED(moditerm->_name);
  moditerm->_parent = parent->getOID();
  moditerm->_next_entry = parent->_moditerms;
  moditerm->_prev_entry = 0;
  if (parent->_moditerms != 0) {
    _dbModITerm* new_next = block->_moditerm_tbl->getPtr(parent->_moditerms);
    new_next->_prev_entry = moditerm->getOID();
  }
  parent->_moditerms = moditerm->getOID();
  parent->_moditerm_hash[name] = dbId<_dbModITerm>(moditerm->getOID());

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbModITermObj);
    block->_journal->pushParam(moditerm->getId());
    block->_journal->endAction();
  }

  return (dbModITerm*) moditerm;
}

void dbModITerm::connect(dbModNet* net)
{
  _dbModITerm* _moditerm = (_dbModITerm*) this;
  _dbModNet* _modnet = (_dbModNet*) net;
  _dbBlock* _block = (_dbBlock*) _moditerm->getOwner();
  // already connected.
  if (_moditerm->_mod_net == _modnet->getId()) {
    return;
  }
  _moditerm->_mod_net = _modnet->getId();
  // append to net moditerms
  if (_modnet->_moditerms != 0) {
    _dbModITerm* head = _block->_moditerm_tbl->getPtr(_modnet->_moditerms);
    // next is old head
    _moditerm->_next_net_moditerm = _modnet->_moditerms;
    head->_prev_net_moditerm = getId();
  } else {
    _moditerm->_next_net_moditerm = 0;
  }
  // set up new head
  _moditerm->_prev_net_moditerm = 0;
  _modnet->_moditerms = getId();

  if (_block->_journal) {
    _block->_journal->beginAction(dbJournal::CONNECT_OBJECT);
    _block->_journal->pushParam(dbModITermObj);
    _block->_journal->pushParam(getId());
    _block->_journal->pushParam(_modnet->getId());
    _block->_journal->endAction();
  }
}

void dbModITerm::disconnect()
{
  _dbModITerm* _moditerm = (_dbModITerm*) this;
  _dbBlock* _block = (_dbBlock*) _moditerm->getOwner();
  if (_moditerm->_mod_net == 0) {
    return;
  }
  _dbModNet* _modnet = _block->_modnet_tbl->getPtr(_moditerm->_mod_net);
  _moditerm->_mod_net = 0;
  _dbModITerm* next_moditerm
      = (_moditerm->_next_net_moditerm != 0)
            ? _block->_moditerm_tbl->getPtr(_moditerm->_next_net_moditerm)
            : nullptr;
  _dbModITerm* prior_moditerm
      = (_moditerm->_prev_net_moditerm != 0)
            ? _block->_moditerm_tbl->getPtr(_moditerm->_prev_net_moditerm)
            : nullptr;
  if (prior_moditerm) {
    prior_moditerm->_next_net_moditerm = _moditerm->_next_net_moditerm;
  } else {
    _modnet->_moditerms = _moditerm->_next_net_moditerm;
  }
  if (next_moditerm) {
    next_moditerm->_prev_net_moditerm = _moditerm->_prev_net_moditerm;
  }
}

dbModITerm* dbModITerm::getModITerm(dbBlock* block, uint dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbModITerm*) (owner->_moditerm_tbl->getPtr(dbid));
}

void dbModITerm::destroy(dbModITerm* val)
{
  _dbModITerm* _moditerm = (_dbModITerm*) val;
  _dbBlock* block = (_dbBlock*) _moditerm->getOwner();

  _dbModInst* mod_inst = block->_modinst_tbl->getPtr(_moditerm->_parent);
  // snip out the mod iterm, from doubly linked list
  uint prev = _moditerm->_prev_entry;
  uint next = _moditerm->_next_entry;
  if (prev == 0) {
    // head of list
    mod_inst->_moditerms = next;
  } else {
    _dbModITerm* prev_moditerm = block->_moditerm_tbl->getPtr(prev);
    prev_moditerm->_next_entry = next;
  }

  if (next != 0) {
    _dbModITerm* next_moditerm = block->_moditerm_tbl->getPtr(next);
    next_moditerm->_prev_entry = prev;
  }
  _moditerm->_prev_entry = 0;
  _moditerm->_next_entry = 0;
  mod_inst->_moditerm_hash.erase(val->getName());
  block->_moditerm_tbl->destroy(_moditerm);
}

// User Code End dbModITermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
