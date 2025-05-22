// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModNet.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbITerm.h"
#include "dbJournal.h"
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

_dbModNet::_dbModNet(_dbDatabase* db)
{
  _name = nullptr;
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
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(db_schema_db_remove_hash)) {
    dbDatabase* db = (dbDatabase*) (obj.getDatabase());
    _dbBlock* block = (_dbBlock*) (db->getChip()->getBlock());
    _dbModule* module = block->_module_tbl->getPtr(obj._parent);
    if (obj._name) {
      module->_modnet_hash[obj._name] = dbId<_dbModNet>(obj.getId());
    }
  }
  // User Code End >>
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

void _dbModNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  // User Code End collectMemInfo
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

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbModNetObj);
    block->_journal->pushParam(name);
    block->_journal->pushParam(modnet->getId());
    block->_journal->pushParam(parent->getId());
    block->_journal->endAction();
  }

  return (dbModNet*) modnet;
}

void dbModNet::destroy(dbModNet* mod_net)
{
  _dbModNet* _modnet = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) _modnet->getOwner();
  _dbModule* module = block->_module_tbl->getPtr(_modnet->_parent);

  // journalling
  if (block->_journal) {
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbModNetObj);
    block->_journal->pushParam(mod_net->getName());
    block->_journal->pushParam(mod_net->getId());
    block->_journal->pushParam(module->getId());
    block->_journal->endAction();
  }

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

dbSet<dbModNet>::iterator dbModNet::destroy(dbSet<dbModNet>::iterator& itr)
{
  dbModNet* modnet = *itr;
  dbSet<dbModNet>::iterator next = ++itr;
  destroy(modnet);
  return next;
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

unsigned dbModNet::connectionCount()
{
  return (getITerms().size() + getBTerms().size() + getModITerms().size());
}
// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
