//////////////////////////////////////////////////////////////////////////////
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
#include "dbModNet.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbITerm.h"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbModuleModNetBTermItr.h"
#include "dbModuleModNetITermItr.h"
#include "dbModuleModNetModBTermItr.h"
#include "dbModuleModNetModITermItr.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModNet>;

bool _dbModNet::operator==(const _dbModNet& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_prev_entry != rhs._prev_entry) {
    return false;
  }
  if (_moditerms != rhs._moditerms) {
    return false;
  }
  if (_modbterms != rhs._modbterms) {
    return false;
  }
  if (_iterms != rhs._iterms) {
    return false;
  }
  if (_bterms != rhs._bterms) {
    return false;
  }

  return true;
}

bool _dbModNet::operator<(const _dbModNet& rhs) const
{
  return true;
}

void _dbModNet::differences(dbDiff& diff,
                            const char* field,
                            const _dbModNet& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_prev_entry);
  DIFF_FIELD(_moditerms);
  DIFF_FIELD(_modbterms);
  DIFF_FIELD(_iterms);
  DIFF_FIELD(_bterms);
  DIFF_END
}

void _dbModNet::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_prev_entry);
  DIFF_OUT_FIELD(_moditerms);
  DIFF_OUT_FIELD(_modbterms);
  DIFF_OUT_FIELD(_iterms);
  DIFF_OUT_FIELD(_bterms);

  DIFF_END
}

_dbModNet::_dbModNet(_dbDatabase* db)
{
  _name = nullptr;
}

_dbModNet::_dbModNet(_dbDatabase* db, const _dbModNet& r)
{
  _name = r._name;
  _parent = r._parent;
  _next_entry = r._next_entry;
  _prev_entry = r._prev_entry;
  _moditerms = r._moditerms;
  _modbterms = r._modbterms;
  _iterms = r._iterms;
  _bterms = r._bterms;
}

dbIStream& operator>>(dbIStream& stream, _dbModNet& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._next_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream >> obj._prev_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._moditerms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modbterms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._iterms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._bterms;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModNet& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._name;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._parent;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._next_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream << obj._prev_entry;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._moditerms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modbterms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._iterms;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._bterms;
  }
  return stream;
}

_dbModNet::~_dbModNet()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModNet - Methods
//
////////////////////////////////////////////////////////////////////

dbModule* dbModNet::getParent() const
{
  _dbModNet* obj = (_dbModNet*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

// User Code Begin dbModNetPublicMethods

const char* dbModNet::getName() const
{
  _dbModNet* obj = (_dbModNet*) this;
  return obj->_name;
}

//
// Support for renaming hierarchical nets
//
void dbModNet::rename(const char* new_name)
{
  _dbModNet* obj = (_dbModNet*) this;
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  _dbModule* parent = block->_module_tbl->getPtr(obj->_parent);
  parent->_modnet_hash.erase(obj->_name);
  free((void*) (obj->_name));
  obj->_name = strdup(new_name);
  ZALLOCATED(obj->_name);
  parent->_modnet_hash[new_name] = obj->getOID();
}

dbModNet* dbModNet::getModNet(dbBlock* block, uint id)
{
  _dbBlock* block_ = (_dbBlock*) block;
  _dbModNet* ret = block_->_modnet_tbl->getPtr(id);
  return (dbModNet*) ret;
}

dbModNet* dbModNet::create(dbModule* parentModule, const char* name)
{
  // give illusion of scoping.
  _dbModule* parent = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) parent->getOwner();
  _dbModNet* modnet = block->_modnet_tbl->create();
  // defaults
  modnet->_name = strdup(name);
  modnet->_parent = parent->getOID();  // dbmodule
  modnet->_next_entry = parent->_modnets;
  modnet->_prev_entry = 0;
  if (parent->_modnets != 0) {
    _dbModNet* new_next = block->_modnet_tbl->getPtr(parent->_modnets);
    new_next->_prev_entry = modnet->getOID();
  }
  parent->_modnets = modnet->getOID();
  parent->_modnet_hash[name] = modnet->getOID();
  return (dbModNet*) modnet;
}

void dbModNet::destroy(dbModNet* mod_net)
{
  _dbModNet* _modnet = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) _modnet->getOwner();
  _dbModule* module = block->_module_tbl->getPtr(_modnet->_parent);

  uint prev = _modnet->_prev_entry;
  uint next = _modnet->_next_entry;
  if (prev == 0) {
    module->_modnets = next;
  } else {
    _dbModNet* prev_modnet = block->_modnet_tbl->getPtr(prev);
    prev_modnet->_next_entry = next;
  }
  if (next != 0) {
    _dbModNet* next_modnet = block->_modnet_tbl->getPtr(next);
    next_modnet->_prev_entry = prev;
  }
  _modnet->_prev_entry = 0;
  _modnet->_next_entry = 0;
  module->_modnet_hash.erase(mod_net->getName());
  block->_modnet_tbl->destroy(_modnet);
}

dbSet<dbModITerm> dbModNet::getModITerms()
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbModITerm>(_mod_net, _block->_module_modnet_moditerm_itr);
}

dbSet<dbModBTerm> dbModNet::getModBTerms()
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbModBTerm>(_mod_net, _block->_module_modnet_modbterm_itr);
}

dbSet<dbBTerm> dbModNet::getBTerms()
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbBTerm>(_mod_net, _block->_module_modnet_bterm_itr);
}

dbSet<dbITerm> dbModNet::getITerms()
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbITerm>(_mod_net, _block->_module_modnet_iterm_itr);
}
// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
