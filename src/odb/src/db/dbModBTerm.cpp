// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModBTerm.h"

#include <cstdint>
#include <cstdlib>

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <string>

#include "dbCommon.h"
#include "dbCore.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModBTerm>;

bool _dbModBTerm::operator==(const _dbModBTerm& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (flags_ != rhs.flags_) {
    return false;
  }
  if (parent_moditerm_ != rhs.parent_moditerm_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (modnet_ != rhs.modnet_) {
    return false;
  }
  if (next_net_modbterm_ != rhs.next_net_modbterm_) {
    return false;
  }
  if (prev_net_modbterm_ != rhs.prev_net_modbterm_) {
    return false;
  }
  if (busPort_ != rhs.busPort_) {
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

bool _dbModBTerm::operator<(const _dbModBTerm& rhs) const
{
  return true;
}

_dbModBTerm::_dbModBTerm(_dbDatabase* db)
{
  name_ = nullptr;
  flags_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj)
{
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.flags_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.parent_moditerm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.parent_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.modnet_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.next_net_modbterm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.prev_net_modbterm_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOdbBusport)) {
    stream >> obj.busPort_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.next_entry_;
  }
  if (obj.getDatabase()->isSchema(kSchemaHierPortRemoval)) {
    stream >> obj.prev_entry_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj)
{
  stream << obj.name_;
  stream << obj.flags_;
  stream << obj.parent_moditerm_;
  stream << obj.parent_;
  stream << obj.modnet_;
  stream << obj.next_net_modbterm_;
  stream << obj.prev_net_modbterm_;
  stream << obj.busPort_;
  stream << obj.next_entry_;
  stream << obj.prev_entry_;
  return stream;
}

void _dbModBTerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  // User Code End collectMemInfo
}

_dbModBTerm::~_dbModBTerm()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

dbModule* dbModBTerm::getParent() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->parent_);
}

// User Code Begin dbModBTermPublicMethods
std::string dbModBTerm::getHierarchicalName() const
{
  dbModule* parent = getParent();
  if (parent == nullptr) {
    return getName();
  }

  dbBlock* block = parent->getOwner();
  if (parent == block->getTopModule()) {
    return getName();
  }

  return fmt::format("{}{}{}",  // NOLINT(misc-include-cleaner)
                     parent->getModInst()->getHierarchicalName(),
                     block->getHierarchyDelimiter(),
                     getName());
}

dbModInst* dbModBTerm::getModInst() const
{
  if (dbModITerm* parent_iterm = getParentModITerm()) {
    return parent_iterm->getParent();
  }
  return nullptr;
}

void dbModBTerm::setModNet(dbModNet* modNet)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->modnet_ = modNet->getImpl()->getOID();
}

dbModNet* dbModBTerm::getModNet() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->modnet_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModNet*) par->modnet_tbl_->getPtr(obj->modnet_);
}

void dbModBTerm::setParentModITerm(dbModITerm* parent_pin)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  obj->parent_moditerm_ = parent_pin->getImpl()->getOID();
}

dbModITerm* dbModBTerm::getParentModITerm() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->parent_moditerm_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModITerm*) par->moditerm_tbl_->getPtr(obj->parent_moditerm_);
}

struct dbModBTermFlags_str
{
  dbIoType::Value iotype : 4;
  dbSigType::Value sigtype : 4;
  uint32_t spare_bits : 24;
};

union dbModBTermFlags
{
  struct dbModBTermFlags_str flags;
  uint32_t uint_val;
};

void dbModBTerm::setSigType(const dbSigType& type)
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  cur_flags.flags.sigtype = type.getValue();
  _dbmodbterm->flags_ = cur_flags.uint_val;
}

dbSigType dbModBTerm::getSigType() const
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  return dbSigType(cur_flags.flags.sigtype);
}

void dbModBTerm::setIoType(const dbIoType& type)
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  cur_flags.flags.iotype = type.getValue();
  _dbmodbterm->flags_ = cur_flags.uint_val;
}

dbIoType dbModBTerm::getIoType() const
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  return dbIoType(cur_flags.flags.iotype);
}

dbModBTerm* dbModBTerm::create(dbModule* parentModule, const char* name)
{
  dbModBTerm* ret = parentModule->findModBTerm(name);
  if (ret) {
    return ret;
  }
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  _dbModBTerm* modbterm = block->modbterm_tbl_->create();
  // defaults
  modbterm->flags_ = 0U;
  ((dbModBTerm*) modbterm)->setIoType(dbIoType::INPUT);
  ((dbModBTerm*) modbterm)->setSigType(dbSigType::SIGNAL);
  modbterm->modnet_ = 0;
  modbterm->next_net_modbterm_ = 0;
  modbterm->prev_net_modbterm_ = 0;
  modbterm->busPort_ = 0;
  modbterm->name_ = safe_strdup(name);
  modbterm->parent_ = module->getOID();
  modbterm->next_entry_ = module->modbterms_;
  modbterm->prev_entry_ = 0;
  if (module->modbterms_ != 0) {
    _dbModBTerm* new_next = block->modbterm_tbl_->getPtr(module->modbterms_);
    new_next->prev_entry_ = modbterm->getOID();
  }
  module->modbterms_ = modbterm->getOID();
  module->modbterm_hash_[name] = dbId<_dbModBTerm>(modbterm->getOID());

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             modbterm->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbModBTermObj);
    block->journal_->pushParam(name);
    block->journal_->pushParam(modbterm->getId());
    block->journal_->pushParam(module->getId());
    block->journal_->endAction();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermCreate((dbModBTerm*) modbterm);
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
  if (_modbterm->modnet_ == net->getId()) {
    return;
  }

  // If the modbterm is already connected to a different modnet, disconnect it
  // first.
  if (_modbterm->modnet_ != 0) {
    disconnect();
  }

  for (auto callback : _block->callbacks_) {
    callback->inDbModBTermPreConnect(this, net);
  }
  _modbterm->modnet_ = net->getId();
  // append to net mod bterms. Do this by pushing onto head of list.
  if (_modnet->modbterms_ != 0) {
    _dbModBTerm* head = _block->modbterm_tbl_->getPtr(_modnet->modbterms_);
    // next is old head
    _modbterm->next_net_modbterm_ = _modnet->modbterms_;
    // previous for old head is this one
    head->prev_net_modbterm_ = getId();
  } else {
    _modbterm->next_net_modbterm_ = 0;  // only element
  }
  _modbterm->prev_net_modbterm_ = 0;  // previous of head always zero
  _modnet->modbterms_ = getId();      // set new head

  debugPrint(_modbterm->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             _modbterm->getDebugName(),
             net->getDebugName());

  if (_block->journal_) {
    _block->journal_->beginAction(dbJournal::kConnectObject);
    _block->journal_->pushParam(dbModBTermObj);
    _block->journal_->pushParam(getId());
    _block->journal_->pushParam(net->getId());
    _block->journal_->endAction();
  }
  for (auto callback : _block->callbacks_) {
    callback->inDbModBTermPostConnect(this);
  }
}

void dbModBTerm::disconnect()
{
  _dbModule* module = (_dbModule*) getParent();
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  if (_modbterm->modnet_ == 0) {
    return;
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermPreDisconnect(this);
  }
  _dbModNet* mod_net = block->modnet_tbl_->getPtr(_modbterm->modnet_);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: disconnect {} from {}",
             _modbterm->getDebugName(),
             mod_net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDisconnectObject);
    block->journal_->pushParam(dbModBTermObj);
    block->journal_->pushParam(_modbterm->getId());
    block->journal_->pushParam(_modbterm->modnet_);
    block->journal_->endAction();
  }

  if (_modbterm->prev_net_modbterm_ == 0) {
    // degenerate case, head element, need to update net starting point
    // and if next is null then make generate empty list
    mod_net->modbterms_ = _modbterm->next_net_modbterm_;
  } else {
    _dbModBTerm* prev_modbterm
        = block->modbterm_tbl_->getPtr(_modbterm->prev_net_modbterm_);
    prev_modbterm->next_net_modbterm_
        = _modbterm->next_net_modbterm_;  // short out this element
  }
  if (_modbterm->next_net_modbterm_ != 0) {
    _dbModBTerm* next_modbterm
        = block->modbterm_tbl_->getPtr(_modbterm->next_net_modbterm_);
    next_modbterm->prev_net_modbterm_ = _modbterm->prev_net_modbterm_;
  }
  //
  // zero out this element for garbage collection
  // Note we can never rely on sequential order of modbterms for offsets.
  //
  _modbterm->next_net_modbterm_ = 0;
  _modbterm->prev_net_modbterm_ = 0;
  _modbterm->modnet_ = 0;

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermPostDisConnect(this, (dbModNet*) mod_net);
  }
}

bool dbModBTerm::isBusPort() const
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  return (_modbterm->busPort_ != 0);
}

dbBusPort* dbModBTerm::getBusPort() const
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  if (_modbterm->busPort_ != 0) {
    _dbBlock* block = (_dbBlock*) _modbterm->getOwner();
    return (dbBusPort*) block->busport_tbl_->getPtr(_modbterm->busPort_);
  }
  return nullptr;
}

void dbModBTerm::setBusPort(dbBusPort* bus_port)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  _modbterm->busPort_ = bus_port->getId();
}

dbModBTerm* dbModBTerm::getModBTerm(dbBlock* block, uint32_t dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbModBTerm*) (owner->modbterm_tbl_->getPtr(dbid));
}

void dbModBTerm::destroy(dbModBTerm* val)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) val;
  _dbBlock* block = (_dbBlock*) (_modbterm->getOwner());

  _dbModule* module = block->module_tbl_->getPtr(_modbterm->parent_);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             val->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbModBTermObj);
    block->journal_->pushParam(val->getName());
    block->journal_->pushParam(val->getId());
    block->journal_->pushParam(module->getId());
    block->journal_->endAction();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermDestroy(val);
  }

  uint32_t prev = _modbterm->prev_entry_;
  uint32_t next = _modbterm->next_entry_;
  if (prev == 0) {
    // head of list
    module->modbterms_ = next;
  } else {
    _dbModBTerm* prev_modbterm = block->modbterm_tbl_->getPtr(prev);
    prev_modbterm->next_entry_ = next;
  }

  if (next != 0) {
    _dbModBTerm* next_modbterm = block->modbterm_tbl_->getPtr(next);
    next_modbterm->prev_entry_ = prev;
  }
  _modbterm->prev_entry_ = 0;
  _modbterm->next_entry_ = 0;

  module->modbterm_hash_.erase(val->getName());
  block->modbterm_tbl_->destroy(_modbterm);
}

dbSet<dbModBTerm>::iterator dbModBTerm::destroy(
    dbSet<dbModBTerm>::iterator& itr)
{
  dbModBTerm* modbterm = *itr;
  dbSet<dbModBTerm>::iterator next = ++itr;
  destroy(modbterm);
  return next;
}

// User Code End dbModBTermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
