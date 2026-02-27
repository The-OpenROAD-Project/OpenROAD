// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModITerm.h"

#include <cstdlib>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModNet.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include "dbCommon.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModITerm>;

bool _dbModITerm::operator==(const _dbModITerm& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (child_modbterm_ != rhs.child_modbterm_) {
    return false;
  }
  if (mod_net_ != rhs.mod_net_) {
    return false;
  }
  if (next_net_moditerm_ != rhs.next_net_moditerm_) {
    return false;
  }
  if (prev_net_moditerm_ != rhs.prev_net_moditerm_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (prev_entry_ != rhs.prev_entry_) {
    return false;
  }

  return true;
}

bool _dbModITerm::operator<(const _dbModITerm& rhs) const
{
  return true;
}

_dbModITerm::_dbModITerm(_dbDatabase* db)
{
  // For pointer tagging the bottom log_2(8) bits.
  static_assert(alignof(_dbModITerm) % 8 == 0);
  name_ = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbModITerm& obj)
{
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.parent_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.child_modbterm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.mod_net_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.next_net_moditerm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.prev_net_moditerm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.next_entry_;
  }
  if (obj.getDatabase()->isSchema(kSchemaHierPortRemoval)) {
    stream >> obj.prev_entry_;
  }
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(kSchemaDbRemoveHash)) {
    dbDatabase* db = reinterpret_cast<dbDatabase*>(obj.getDatabase());
    _dbBlock* block = reinterpret_cast<_dbBlock*>(db->getChip()->getBlock());
    _dbModInst* mod_inst = block->modinst_tbl_->getPtr(obj.parent_);
    if (obj.name_) {
      mod_inst->moditerm_hash_[obj.name_] = dbId<_dbModITerm>(obj.getId());
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModITerm& obj)
{
  stream << obj.name_;
  stream << obj.parent_;
  stream << obj.child_modbterm_;
  stream << obj.mod_net_;
  stream << obj.next_net_moditerm_;
  stream << obj.prev_net_moditerm_;
  stream << obj.next_entry_;
  stream << obj.prev_entry_;
  return stream;
}

void _dbModITerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  // User Code End collectMemInfo
}

_dbModITerm::~_dbModITerm()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

dbModInst* dbModITerm::getParent() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->modinst_tbl_->getPtr(obj->parent_);
}

// User Code Begin dbModITermPublicMethods
std::string dbModITerm::getHierarchicalName() const
{
  dbModInst* modinst = getParent();
  if (modinst == nullptr) {
    return getName();
  }

  dbModule* module = modinst->getParent();
  if (module == nullptr) {
    return getName();
  }

  dbBlock* block = module->getOwner();
  if (module == block->getTopModule()) {
    return getName();
  }

  return fmt::format("{}{}{}",  // NOLINT(misc-include-cleaner)
                     modinst->getHierarchicalName(),
                     block->getHierarchyDelimiter(),
                     getName());
}

void dbModITerm::setModNet(dbModNet* modNet)
{
  _dbModITerm* obj = reinterpret_cast<_dbModITerm*>(this);

  obj->mod_net_ = modNet->getImpl()->getOID();
}

dbModNet* dbModITerm::getModNet() const
{
  const _dbModITerm* obj = reinterpret_cast<const _dbModITerm*>(this);
  if (obj->mod_net_ == 0) {
    return nullptr;
  }
  _dbBlock* par = static_cast<_dbBlock*>(obj->getOwner());
  return reinterpret_cast<dbModNet*>(par->modnet_tbl_->getPtr(obj->mod_net_));
}

void dbModITerm::setChildModBTerm(dbModBTerm* child_port)
{
  _dbModITerm* obj = reinterpret_cast<_dbModITerm*>(this);
  obj->child_modbterm_ = child_port->getImpl()->getOID();
}

dbModBTerm* dbModITerm::getChildModBTerm() const
{
  const _dbModITerm* obj = reinterpret_cast<const _dbModITerm*>(this);
  if (obj->child_modbterm_ == 0) {
    return nullptr;
  }
  _dbBlock* par = static_cast<_dbBlock*>(obj->getOwner());
  return reinterpret_cast<dbModBTerm*>(
      par->modbterm_tbl_->getPtr(obj->child_modbterm_));
}

dbModITerm* dbModITerm::create(dbModInst* parentInstance,
                               const char* name,
                               dbModBTerm* modbterm)
{
  assert(parentInstance->findModITerm(name) == nullptr);

  _dbModInst* parent = reinterpret_cast<_dbModInst*>(parentInstance);
  _dbBlock* block = static_cast<_dbBlock*>(parent->getOwner());
  assert(strchr(name, block->hier_delimiter_) == nullptr);
  _dbModITerm* moditerm = block->moditerm_tbl_->create();

  // defaults
  moditerm->mod_net_ = 0;
  moditerm->next_net_moditerm_ = 0;
  moditerm->prev_net_moditerm_ = 0;

  moditerm->name_ = safe_strdup(name);
  moditerm->parent_ = parent->getOID();
  moditerm->next_entry_ = parent->moditerms_;
  moditerm->prev_entry_ = 0;
  if (parent->moditerms_ != 0) {
    _dbModITerm* new_next = block->moditerm_tbl_->getPtr(parent->moditerms_);
    new_next->prev_entry_ = moditerm->getOID();
  }
  parent->moditerms_ = moditerm->getOID();
  parent->moditerm_hash_[name] = dbId<_dbModITerm>(moditerm->getOID());

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             moditerm->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbModITermObj);
    block->journal_->pushParam(name);
    block->journal_->pushParam(moditerm->getId());
    if (modbterm) {
      block->journal_->pushParam(modbterm->getId());
    } else {
      block->journal_->pushParam(0U);
    }
    block->journal_->pushParam(parent->getId());
    block->journal_->endAction();
  }

  if (modbterm) {
    reinterpret_cast<dbModITerm*>(moditerm)->setChildModBTerm(modbterm);
    modbterm->setParentModITerm(reinterpret_cast<dbModITerm*>(moditerm));
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModITermCreate(reinterpret_cast<dbModITerm*>(moditerm));
  }

  return reinterpret_cast<dbModITerm*>(moditerm);
}

void dbModITerm::connect(dbModNet* net)
{
  _dbModITerm* _moditerm = reinterpret_cast<_dbModITerm*>(this);
  _dbModNet* _modnet = reinterpret_cast<_dbModNet*>(net);
  _dbBlock* _block = static_cast<_dbBlock*>(_moditerm->getOwner());
  // already connected.
  if (_moditerm->mod_net_ == _modnet->getId()) {
    return;
  }

  // If the moditerm is already connected to a different modnet, disconnect it
  // first.
  if (_moditerm->mod_net_ != 0) {
    disconnect();
  }

  for (auto callback : _block->callbacks_) {
    callback->inDbModITermPreConnect(this, net);
  }
  _moditerm->mod_net_ = _modnet->getId();
  // append to net moditerms
  if (_modnet->moditerms_ != 0) {
    _dbModITerm* head = _block->moditerm_tbl_->getPtr(_modnet->moditerms_);
    // next is old head
    _moditerm->next_net_moditerm_ = _modnet->moditerms_;
    head->prev_net_moditerm_ = getId();
  } else {
    _moditerm->next_net_moditerm_ = 0;
  }
  // set up new head
  _moditerm->prev_net_moditerm_ = 0;
  _modnet->moditerms_ = getId();

  debugPrint(_block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             _moditerm->getDebugName(),
             _modnet->getDebugName());

  if (_block->journal_) {
    _block->journal_->beginAction(dbJournal::kConnectObject);
    _block->journal_->pushParam(dbModITermObj);
    _block->journal_->pushParam(getId());
    _block->journal_->pushParam(_modnet->getId());
    _block->journal_->endAction();
  }
  for (auto callback : _block->callbacks_) {
    callback->inDbModITermPostConnect(this);
  }
}

void dbModITerm::disconnect()
{
  _dbModITerm* _moditerm = reinterpret_cast<_dbModITerm*>(this);
  _dbBlock* _block = static_cast<_dbBlock*>(_moditerm->getOwner());
  if (_moditerm->mod_net_ == 0) {
    return;
  }
  for (auto callback : _block->callbacks_) {
    callback->inDbModITermPreDisconnect(this);
  }
  _dbModNet* _modnet = _block->modnet_tbl_->getPtr(_moditerm->mod_net_);

  debugPrint(_block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: disconnect {} from {}",
             _moditerm->getDebugName(),
             _modnet->getDebugName());

  if (_block->journal_) {
    _block->journal_->beginAction(dbJournal::kDisconnectObject);
    _block->journal_->pushParam(dbModITermObj);
    _block->journal_->pushParam(_moditerm->getId());
    _block->journal_->pushParam(_moditerm->mod_net_);
    _block->journal_->endAction();
  }

  _dbModITerm* next_moditerm
      = (_moditerm->next_net_moditerm_ != 0)
            ? _block->moditerm_tbl_->getPtr(_moditerm->next_net_moditerm_)
            : nullptr;
  _dbModITerm* prior_moditerm
      = (_moditerm->prev_net_moditerm_ != 0)
            ? _block->moditerm_tbl_->getPtr(_moditerm->prev_net_moditerm_)
            : nullptr;
  if (prior_moditerm) {
    prior_moditerm->next_net_moditerm_ = _moditerm->next_net_moditerm_;
  } else {
    _modnet->moditerms_ = _moditerm->next_net_moditerm_;
  }
  if (next_moditerm) {
    next_moditerm->prev_net_moditerm_ = _moditerm->prev_net_moditerm_;
  }

  _moditerm->next_net_moditerm_ = 0;
  _moditerm->prev_net_moditerm_ = 0;
  _moditerm->mod_net_ = 0;

  for (auto callback : _block->callbacks_) {
    callback->inDbModITermPostDisconnect(this,
                                         reinterpret_cast<dbModNet*>(_modnet));
  }
}

dbModITerm* dbModITerm::getModITerm(dbBlock* block, uint32_t dbid)
{
  _dbBlock* owner = reinterpret_cast<_dbBlock*>(block);
  return reinterpret_cast<dbModITerm*>(owner->moditerm_tbl_->getPtr(dbid));
}

void dbModITerm::destroy(dbModITerm* val)
{
  _dbModITerm* _moditerm = reinterpret_cast<_dbModITerm*>(val);
  _dbBlock* block = static_cast<_dbBlock*>(_moditerm->getOwner());
  _dbModInst* mod_inst = block->modinst_tbl_->getPtr(_moditerm->parent_);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             _moditerm->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbModITermObj);
    block->journal_->pushParam(val->getName());
    block->journal_->pushParam(val->getId());
    block->journal_->pushParam(_moditerm->child_modbterm_);
    block->journal_->pushParam(_moditerm->parent_);
    block->journal_->endAction();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModITermDestroy(val);
  }

  // Clear the parent moditerm from the child modbterm
  if (_moditerm->child_modbterm_ != 0) {
    if (_dbModBTerm* child_modbterm
        = block->modbterm_tbl_->getPtr(_moditerm->child_modbterm_)) {
      child_modbterm->parent_moditerm_ = 0;
    }
  }

  // snip out the mod iterm, from doubly linked list
  uint32_t prev = _moditerm->prev_entry_;
  uint32_t next = _moditerm->next_entry_;
  if (prev == 0) {
    // head of list
    mod_inst->moditerms_ = next;
  } else {
    _dbModITerm* prev_moditerm = block->moditerm_tbl_->getPtr(prev);
    prev_moditerm->next_entry_ = next;
  }

  if (next != 0) {
    _dbModITerm* next_moditerm = block->moditerm_tbl_->getPtr(next);
    next_moditerm->prev_entry_ = prev;
  }
  _moditerm->prev_entry_ = 0;
  _moditerm->next_entry_ = 0;
  mod_inst->moditerm_hash_.erase(val->getName());
  block->moditerm_tbl_->destroy(_moditerm);
}

dbSet<dbModITerm>::iterator dbModITerm::destroy(
    dbSet<dbModITerm>::iterator& itr)
{
  dbModITerm* moditerm = *itr;
  dbSet<dbModITerm>::iterator next = ++itr;
  destroy(moditerm);
  return next;
}

// User Code End dbModITermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
