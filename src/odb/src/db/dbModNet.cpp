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
#include "odb/dbBlockCallBackObj.h"
#include "utl/Logger.h"
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
  stream << obj._name;
  stream << obj._parent;
  stream << obj._next_entry;
  stream << obj._prev_entry;
  stream << obj._moditerms;
  stream << obj._modbterms;
  stream << obj._iterms;
  stream << obj._bterms;
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

std::string dbModNet::getName() const
{
  _dbModNet* obj = (_dbModNet*) this;
  return obj->_name;
}

const char* dbModNet::getConstName() const
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
  if (strcmp(obj->_name, new_name) == 0) {
    return;
  }

  _dbBlock* block = (_dbBlock*) obj->getOwner();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: mod_net {}, rename to {}",
               getId(),
               new_name);
    block->_journal->updateField(this, _dbModNet::NAME, obj->_name, new_name);
  }

  _dbModule* parent = block->_module_tbl->getPtr(obj->_parent);
  parent->_modnet_hash.erase(obj->_name);
  free((void*) (obj->_name));
  obj->_name = safe_strdup(new_name);
  parent->_modnet_hash[new_name] = obj->getOID();
}

void dbModNet::disconnectAllTerms()
{
  // Disconnect all terminals.
  // - The loops are structured this way to handle the modification of the dbSet
  // during iteration.
  while (!getITerms().empty()) {
    getITerms().begin()->disconnectDbModNet();
  }

  while (!getBTerms().empty()) {
    getBTerms().begin()->disconnectDbModNet();
  }

  while (!getModITerms().empty()) {
    getModITerms().begin()->disconnect();
  }

  while (!getModBTerms().empty()) {
    getModBTerms().begin()->disconnect();
  }
}

void dbModNet::dump() const
{
  utl::Logger* logger = getImpl()->getLogger();
  logger->report("--------------------------------------------------");
  logger->report("dbModNet: {} (id={})", getName(), getId());
  logger->report("  Parent Module: {} (id={})",
                 getParent()->getName(),
                 getParent()->getId());

  logger->report("  ModITerms ({}):", getModITerms().size());
  for (dbModITerm* term : getModITerms()) {
    // For dbModITerm, get types from child dbModBTerm
    dbModBTerm* child_bterm = term->getChildModBTerm();
    if (child_bterm) {
      logger->report("    - {} ({}, {}, id={})",
                     term->getName(),
                     child_bterm->getSigType().getString(),
                     child_bterm->getIoType().getString(),
                     term->getId());
    } else {
      logger->report(
          "    - {} (no child bterm, id={})", term->getName(), term->getId());
    }
  }

  logger->report("  ModBTerms ({}):", getModBTerms().size());
  for (dbModBTerm* term : getModBTerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }

  logger->report("  ITerms ({}):", getITerms().size());
  for (dbITerm* term : getITerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }

  logger->report("  BTerms ({}):", getBTerms().size());
  for (dbBTerm* term : getBTerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }
  logger->report("--------------------------------------------------");
}

dbModNet* dbModNet::getModNet(dbBlock* block, uint id)
{
  _dbBlock* block_ = (_dbBlock*) block;
  _dbModNet* ret = block_->_modnet_tbl->getPtr(id);
  return (dbModNet*) ret;
}

dbModNet* dbModNet::create(dbModule* parentModule, const char* base_name)
{
  assert(parentModule->getModNet(base_name) == nullptr);

  // give illusion of scoping.
  _dbModule* parent = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) parent->getOwner();
  _dbModNet* modnet = block->_modnet_tbl->create();
  // defaults
  modnet->_name = strdup(base_name);
  modnet->_parent = parent->getOID();  // dbmodule
  modnet->_next_entry = parent->_modnets;
  modnet->_prev_entry = 0;
  if (parent->_modnets != 0) {
    _dbModNet* new_next = block->_modnet_tbl->getPtr(parent->_modnets);
    new_next->_prev_entry = modnet->getOID();
  }
  parent->_modnets = modnet->getOID();
  parent->_modnet_hash[base_name] = modnet->getOID();

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbModNetObj);
    block->_journal->pushParam(base_name);
    block->_journal->pushParam(modnet->getId());
    block->_journal->pushParam(parent->getId());
    block->_journal->endAction();
  }

  for (auto cb : block->_callbacks) {
    cb->inDbModNetCreate((dbModNet*) modnet);
  }

  return (dbModNet*) modnet;
}

void dbModNet::destroy(dbModNet* mod_net)
{
  _dbModNet* _modnet = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) _modnet->getOwner();
  _dbModule* module = block->_module_tbl->getPtr(_modnet->_parent);

  mod_net->disconnectAllTerms();

  // journalling
  if (block->_journal) {
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbModNetObj);
    block->_journal->pushParam(mod_net->getName());
    block->_journal->pushParam(mod_net->getId());
    block->_journal->pushParam(module->getId());
    block->_journal->endAction();
  }

  for (auto cb : block->_callbacks) {
    cb->inDbModNetDestroy(mod_net);
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

dbSet<dbModITerm> dbModNet::getModITerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbModITerm>(_mod_net, _block->_module_modnet_moditerm_itr);
}

dbSet<dbModBTerm> dbModNet::getModBTerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbModBTerm>(_mod_net, _block->_module_modnet_modbterm_itr);
}

dbSet<dbBTerm> dbModNet::getBTerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbBTerm>(_mod_net, _block->_module_modnet_bterm_itr);
}

dbSet<dbITerm> dbModNet::getITerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbITerm>(_mod_net, _block->_module_modnet_iterm_itr);
}

unsigned dbModNet::connectionCount()
{
  return (getITerms().size() + getBTerms().size() + getModITerms().size());
}

dbNet* dbModNet::findRelatedNet() const
{
  // Helper to find a flat net with ITerms or BTerms connected to this modnet.
  auto findNetOfTerms = [](const dbModNet* modnet) -> dbNet* {
    for (dbITerm* iterm : modnet->getITerms()) {
      if (dbNet* net = iterm->getNet()) {
        return net;
      }
    }
    for (dbBTerm* bterm : modnet->getBTerms()) {
      if (dbNet* net = bterm->getNet()) {
        return net;
      }
    }
    return nullptr;
  };

  // Fast path: check this modnet's iterms and bterms first.
  if (dbNet* net = findNetOfTerms(this)) {
    return net;
  }

  //
  // Slow path: traverse hierarchy
  //
  std::vector<const dbModNet*> modnets_to_visit;
  std::set<const dbModNet*> visited_modnets;

  // Helper to add a modnet to the visit queue if it's new.
  auto visitIfNew = [&](const dbModNet* modnet) {
    if (modnet && visited_modnets.insert(modnet).second) {
      modnets_to_visit.push_back(modnet);
    }
  };

  const dbModNet* current_modnet = this;
  while (current_modnet != nullptr) {
    visited_modnets.insert(current_modnet);

    // Expand search to connected hierarchical nets (dbModNet).

    // Traverse down the hierarchy.
    for (dbModITerm* mod_iterm : current_modnet->getModITerms()) {
      if (dbModBTerm* child_bterm = mod_iterm->getChildModBTerm()) {
        visitIfNew(child_bterm->getModNet());
      }
    }
    // Traverse up the hierarchy.
    for (dbModBTerm* mod_bterm : current_modnet->getModBTerms()) {
      if (dbModITerm* parent_iterm = mod_bterm->getParentModITerm()) {
        visitIfNew(parent_iterm->getModNet());
      }
    }

    // Move to the next modnet to visit.
    if (modnets_to_visit.empty()) {
      break;  // No more modnets to visit.
    }

    current_modnet = modnets_to_visit.back();
    modnets_to_visit.pop_back();

    // Check for a direct connection to a dbNet (flat net).
    if (dbNet* net = findNetOfTerms(current_modnet)) {
      return net;
    }
  }

  // No related dbNet found
  return nullptr;
}

// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
