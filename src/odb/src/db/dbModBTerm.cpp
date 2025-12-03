// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModBTerm.h"

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdlib>
#include <string>

#include "dbCommon.h"
#include "odb/dbBlockCallBackObj.h"
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
  if (next_entry_ != rhs.next_entry_) {
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

_dbModBTerm::_dbModBTerm(_dbDatabase* db)
{
  name_ = nullptr;
  flags_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.flags_;
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
    stream >> obj.next_entry_;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream >> obj._prev_entry;
  }
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(db_schema_db_remove_hash)) {
    dbDatabase* db = (dbDatabase*) (obj.getDatabase());
    _dbBlock* block = (_dbBlock*) (db->getChip()->getBlock());
    _dbModule* module = block->module_tbl_->getPtr(obj._parent);
    if (obj.name_) {
      module->_modbterm_hash[obj.name_] = obj.getId();
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj)
{
  stream << obj.name_;
  stream << obj.flags_;
  stream << obj._parent_moditerm;
  stream << obj._parent;
  stream << obj._modnet;
  stream << obj._next_net_modbterm;
  stream << obj._prev_net_modbterm;
  stream << obj._busPort;
  stream << obj.next_entry_;
  stream << obj._prev_entry;
  return stream;
}

void _dbModBTerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(name_);
  // User Code End collectMemInfo
}

_dbModBTerm::~_dbModBTerm()
{
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
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->_parent);
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
  return (dbModNet*) par->modnet_tbl_->getPtr(obj->_modnet);
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
  return (dbModITerm*) par->moditerm_tbl_->getPtr(obj->_parent_moditerm);
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
  cur_flags.uint_val = _dbmodbterm->flags_;
  cur_flags.flags._sigtype = type.getValue();
  _dbmodbterm->flags_ = cur_flags.uint_val;
}

dbSigType dbModBTerm::getSigType() const
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  return dbSigType(cur_flags.flags._sigtype);
}

void dbModBTerm::setIoType(const dbIoType& type)
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
  cur_flags.flags._iotype = type.getValue();
  _dbmodbterm->flags_ = cur_flags.uint_val;
}

dbIoType dbModBTerm::getIoType() const
{
  _dbModBTerm* _dbmodbterm = (_dbModBTerm*) this;
  dbModBTermFlags cur_flags;
  cur_flags.uint_val = _dbmodbterm->flags_;
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

  _dbModBTerm* modbterm = block->modbterm_tbl_->create();
  // defaults
  modbterm->flags_ = 0U;
  ((dbModBTerm*) modbterm)->setIoType(dbIoType::INPUT);
  ((dbModBTerm*) modbterm)->setSigType(dbSigType::SIGNAL);
  modbterm->_modnet = 0;
  modbterm->_next_net_modbterm = 0;
  modbterm->_prev_net_modbterm = 0;
  modbterm->_busPort = 0;
  modbterm->name_ = safe_strdup(name);
  modbterm->_parent = module->getOID();
  modbterm->next_entry_ = module->_modbterms;
  modbterm->_prev_entry = 0;
  if (module->_modbterms != 0) {
    _dbModBTerm* new_next = block->modbterm_tbl_->getPtr(module->_modbterms);
    new_next->_prev_entry = modbterm->getOID();
  }
  module->_modbterms = modbterm->getOID();
  module->_modbterm_hash[name] = dbId<_dbModBTerm>(modbterm->getOID());

  if (block->journal_) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: create dbModBTerm {} at id {}",
               name,
               modbterm->getId());
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
  if (_modbterm->_modnet == net->getId()) {
    return;
  }

  // If the modbterm is already connected to a different modnet, disconnect it
  // first.
  if (_modbterm->_modnet != 0) {
    disconnect();
  }

  for (auto callback : _block->callbacks_) {
    callback->inDbModBTermPreConnect(this, net);
  }
  _modbterm->_modnet = net->getId();
  // append to net mod bterms. Do this by pushing onto head of list.
  if (_modnet->_modbterms != 0) {
    _dbModBTerm* head = _block->modbterm_tbl_->getPtr(_modnet->_modbterms);
    // next is old head
    _modbterm->_next_net_modbterm = _modnet->_modbterms;
    // previous for old head is this one
    head->_prev_net_modbterm = getId();
  } else {
    _modbterm->_next_net_modbterm = 0;  // only element
  }
  _modbterm->_prev_net_modbterm = 0;  // previous of head always zero
  _modnet->_modbterms = getId();      // set new head

  if (_block->journal_) {
    debugPrint(_modbterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: connect modBterm ({} {:p}) '{}' to modnet ({} {:p}) '{}'",
               getId(),
               static_cast<void*>(this),
               getHierarchicalName(),
               net->getId(),
               static_cast<void*>(net),
               net->getHierarchicalName());
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
  if (_modbterm->_modnet == 0) {
    return;
  }

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermPreDisconnect(this);
  }
  _dbModNet* mod_net = block->modnet_tbl_->getPtr(_modbterm->_modnet);

  if (block->journal_) {
    debugPrint(
        block->getImpl()->getLogger(),
        utl::ODB,
        "DB_ECO",
        1,
        "ECO: disconnect dbModBTerm {} at id {} from dbModNet {} at id {}",
        getName(),
        getId(),
        mod_net->name_,
        mod_net->getId());
    block->journal_->beginAction(dbJournal::kDisconnectObject);
    block->journal_->pushParam(dbModBTermObj);
    block->journal_->pushParam(_modbterm->getId());
    block->journal_->pushParam(_modbterm->_modnet);
    block->journal_->endAction();
  }

  if (_modbterm->_prev_net_modbterm == 0) {
    // degenerate case, head element, need to update net starting point
    // and if next is null then make generate empty list
    mod_net->_modbterms = _modbterm->_next_net_modbterm;
  } else {
    _dbModBTerm* prev_modbterm
        = block->modbterm_tbl_->getPtr(_modbterm->_prev_net_modbterm);
    prev_modbterm->_next_net_modbterm
        = _modbterm->_next_net_modbterm;  // short out this element
  }
  if (_modbterm->_next_net_modbterm != 0) {
    _dbModBTerm* next_modbterm
        = block->modbterm_tbl_->getPtr(_modbterm->_next_net_modbterm);
    next_modbterm->_prev_net_modbterm = _modbterm->_prev_net_modbterm;
  }
  //
  // zero out this element for garbage collection
  // Note we can never rely on sequential order of modbterms for offsets.
  //
  _modbterm->_next_net_modbterm = 0;
  _modbterm->_prev_net_modbterm = 0;
  _modbterm->_modnet = 0;

  for (auto callback : block->callbacks_) {
    callback->inDbModBTermPostDisConnect(this, (dbModNet*) mod_net);
  }
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
    return (dbBusPort*) block->busport_tbl_->getPtr(_modbterm->_busPort);
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
  return (dbModBTerm*) (owner->modbterm_tbl_->getPtr(dbid));
}

void dbModBTerm::destroy(dbModBTerm* val)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) val;
  _dbBlock* block = (_dbBlock*) (_modbterm->getOwner());

  _dbModule* module = block->module_tbl_->getPtr(_modbterm->_parent);

  if (block->journal_) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: delete dbModBTerm {} at id {}",
               val->getName(),
               val->getId());
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

  uint prev = _modbterm->_prev_entry;
  uint next = _modbterm->next_entry_;
  if (prev == 0) {
    // head of list
    module->_modbterms = next;
  } else {
    _dbModBTerm* prev_modbterm = block->modbterm_tbl_->getPtr(prev);
    prev_modbterm->next_entry_ = next;
  }

  if (next != 0) {
    _dbModBTerm* next_modbterm = block->modbterm_tbl_->getPtr(next);
    next_modbterm->_prev_entry = prev;
  }
  _modbterm->_prev_entry = 0;
  _modbterm->next_entry_ = 0;

  module->_modbterm_hash.erase(val->getName());
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
