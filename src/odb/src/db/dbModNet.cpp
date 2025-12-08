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
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "dbCommon.h"
#include "dbModuleModNetBTermItr.h"
#include "dbModuleModNetITermItr.h"
#include "dbModuleModNetModBTermItr.h"
#include "dbModuleModNetModITermItr.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbUtil.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModNet>;

bool _dbModNet::operator==(const _dbModNet& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (prev_entry_ != rhs.prev_entry_) {
    return false;
  }
  if (moditerms_ != rhs.moditerms_) {
    return false;
  }
  if (modbterms_ != rhs.modbterms_) {
    return false;
  }
  if (iterms_ != rhs.iterms_) {
    return false;
  }
  if (bterms_ != rhs.bterms_) {
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
  name_ = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbModNet& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.parent_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.next_entry_;
  }
  if (obj.getDatabase()->isSchema(db_schema_hier_port_removal)) {
    stream >> obj.prev_entry_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.moditerms_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.modbterms_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.iterms_;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj.bterms_;
  }
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(db_schema_db_remove_hash)) {
    dbDatabase* db = (dbDatabase*) (obj.getDatabase());
    _dbBlock* block = (_dbBlock*) (db->getChip()->getBlock());
    _dbModule* module = block->module_tbl_->getPtr(obj.parent_);
    if (obj.name_) {
      module->modnet_hash_[obj.name_] = dbId<_dbModNet>(obj.getId());
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModNet& obj)
{
  stream << obj.name_;
  stream << obj.parent_;
  stream << obj.next_entry_;
  stream << obj.prev_entry_;
  stream << obj.moditerms_;
  stream << obj.modbterms_;
  stream << obj.iterms_;
  stream << obj.bterms_;
  return stream;
}

void _dbModNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(name_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbModNet - Methods
//
////////////////////////////////////////////////////////////////////

dbModule* dbModNet::getParent() const
{
  _dbModNet* obj = (_dbModNet*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->parent_);
}

// User Code Begin dbModNetPublicMethods

std::string dbModNet::getName() const
{
  _dbModNet* obj = (_dbModNet*) this;
  return obj->name_;
}

const char* dbModNet::getConstName() const
{
  _dbModNet* obj = (_dbModNet*) this;
  return obj->name_;
}

std::string dbModNet::getHierarchicalName() const
{
  dbModule* parent = getParent();
  if (parent == nullptr) {
    return getName();
  }

  dbBlock* block = parent->getOwner();
  if (parent == block->getTopModule()) {
    return getName();
  }

  return fmt::format("{}{}{}",
                     parent->getModInst()->getHierarchicalName(),
                     block->getHierarchyDelimiter(),
                     getName());
}

//
// Support for renaming hierarchical nets
//
void dbModNet::rename(const char* new_name)
{
  _dbModNet* obj = (_dbModNet*) this;
  if (strcmp(obj->name_, new_name) == 0) {
    return;
  }

  _dbBlock* block = (_dbBlock*) obj->getOwner();

  if (block->journal_) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbModNet({} {:p}) '{}', rename to '{}'",
               getId(),
               static_cast<void*>(this),
               getHierarchicalName(),
               new_name);
    block->journal_->updateField(this, _dbModNet::kName, obj->name_, new_name);
  }

  _dbModule* parent = block->module_tbl_->getPtr(obj->parent_);
  parent->modnet_hash_.erase(obj->name_);
  free((void*) (obj->name_));
  obj->name_ = safe_strdup(new_name);
  parent->modnet_hash_[new_name] = obj->getOID();
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
  _dbModNet* ret = block_->modnet_tbl_->getPtr(id);
  return (dbModNet*) ret;
}

dbModNet* dbModNet::create(dbModule* parentModule, const char* base_name)
{
  assert(parentModule->getModNet(base_name) == nullptr);

  // give illusion of scoping.
  _dbModule* parent = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) parent->getOwner();
  _dbModNet* modnet = block->modnet_tbl_->create();
  // defaults
  modnet->name_ = safe_strdup(base_name);
  modnet->parent_ = parent->getOID();  // dbmodule
  modnet->next_entry_ = parent->modnets_;
  modnet->prev_entry_ = 0;
  if (parent->modnets_ != 0) {
    _dbModNet* new_next = block->modnet_tbl_->getPtr(parent->modnets_);
    new_next->prev_entry_ = modnet->getOID();
  }
  parent->modnets_ = modnet->getOID();
  parent->modnet_hash_[base_name] = modnet->getOID();

  if (block->journal_) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: create dbModNet {} at id {}",
               base_name,
               modnet->getId());
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbModNetObj);
    block->journal_->pushParam(base_name);
    block->journal_->pushParam(modnet->getId());
    block->journal_->pushParam(parent->getId());
    block->journal_->endAction();
  }

  for (auto cb : block->callbacks_) {
    cb->inDbModNetCreate((dbModNet*) modnet);
  }

  return (dbModNet*) modnet;
}

void dbModNet::destroy(dbModNet* mod_net)
{
  _dbModNet* _modnet = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) _modnet->getOwner();
  _dbModule* module = block->module_tbl_->getPtr(_modnet->parent_);

  mod_net->disconnectAllTerms();

  // journalling
  if (block->journal_) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: delete dbModNet {} at id {}",
               mod_net->getName(),
               mod_net->getId());
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbModNetObj);
    block->journal_->pushParam(mod_net->getName());
    block->journal_->pushParam(mod_net->getId());
    block->journal_->pushParam(module->getId());
    block->journal_->endAction();
  }

  for (auto cb : block->callbacks_) {
    cb->inDbModNetDestroy(mod_net);
  }

  uint prev = _modnet->prev_entry_;
  uint next = _modnet->next_entry_;
  if (prev == 0) {
    module->modnets_ = next;
  } else {
    _dbModNet* prev_modnet = block->modnet_tbl_->getPtr(prev);
    prev_modnet->next_entry_ = next;
  }
  if (next != 0) {
    _dbModNet* next_modnet = block->modnet_tbl_->getPtr(next);
    next_modnet->prev_entry_ = prev;
  }
  _modnet->prev_entry_ = 0;
  _modnet->next_entry_ = 0;
  module->modnet_hash_.erase(mod_net->getName());
  block->modnet_tbl_->destroy(_modnet);
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
  return dbSet<dbModITerm>(_mod_net, _block->module_modnet_moditerm_itr_);
}

dbSet<dbModBTerm> dbModNet::getModBTerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbModBTerm>(_mod_net, _block->module_modnet_modbterm_itr_);
}

dbSet<dbBTerm> dbModNet::getBTerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbBTerm>(_mod_net, _block->module_modnet_bterm_itr_);
}

dbSet<dbITerm> dbModNet::getITerms() const
{
  _dbModNet* _mod_net = (_dbModNet*) this;
  _dbBlock* _block = (_dbBlock*) _mod_net->getOwner();
  return dbSet<dbITerm>(_mod_net, _block->module_modnet_iterm_itr_);
}

unsigned dbModNet::connectionCount() const
{
  return (getITerms().size() + getBTerms().size() + getModITerms().size()
          + getModBTerms().size());
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

void dbModNet::checkSanity() const
{
  std::vector<std::string> drvr_info_list;
  dbUtil::findBTermDrivers(this, drvr_info_list);
  dbUtil::findITermDrivers(this, drvr_info_list);
  dbUtil::findModBTermDrivers(this, drvr_info_list);
  dbUtil::findModITermDrivers(this, drvr_info_list);

  dbUtil::checkNetSanity(this, drvr_info_list);
}

void dbModNet::mergeModNet(dbModNet* in_modnet)
{
  _dbModNet* net = (_dbModNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  for (auto callback : block->callbacks_) {
    callback->inDbModNetPreMerge(this, in_modnet);
  }

  // 1. Connect all terminals of in_modnet to this modnet.
  // - Create vectors for safe iteration, as connect() can invalidate iterators.
  auto iterms_set = in_modnet->getITerms();
  std::vector<dbITerm*> iterms(iterms_set.begin(), iterms_set.end());
  for (dbITerm* iterm : iterms) {
    iterm->connect(this);
  }

  auto bterms_set = in_modnet->getBTerms();
  std::vector<dbBTerm*> bterms(bterms_set.begin(), bterms_set.end());
  for (dbBTerm* bterm : bterms) {
    bterm->connect(this);
  }

  auto moditerms_set = in_modnet->getModITerms();
  std::vector<dbModITerm*> moditerms(moditerms_set.begin(),
                                     moditerms_set.end());
  for (dbModITerm* moditerm : moditerms) {
    moditerm->connect(this);
  }

  auto modbterms_set = in_modnet->getModBTerms();
  std::vector<dbModBTerm*> modbterms(modbterms_set.begin(),
                                     modbterms_set.end());
  for (dbModBTerm* modbterm : modbterms) {
    modbterm->connect(this);
  }

  // 2. Destroy in_modnet
  destroy(in_modnet);
}

void dbModNet::connectTermsOf(dbNet* in_net)
{
  _dbModNet* net = (_dbModNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  for (auto callback : block->callbacks_) {
    callback->inDbModNetPreConnectTermsOf(this, in_net);
  }

  // Create vectors for safe iteration, as connect() can invalidate iterators.
  auto iterms_set = in_net->getITerms();
  std::vector<dbITerm*> iterms(iterms_set.begin(), iterms_set.end());
  dbModule* modnet_parent = getParent();
  for (dbITerm* iterm : iterms) {
    // Only connect terminals that are in the same module as the modnet
    if (iterm->getInst()->getModule() == modnet_parent) {
      iterm->connect(this);
    }
  }

  auto bterms_set = in_net->getBTerms();
  std::vector<dbBTerm*> bterms(bterms_set.begin(), bterms_set.end());
  for (dbBTerm* bterm : bterms) {
    // Only connect terminals that are in the same module as the modnet
    // BTerms are considered to be in the top module of the block.
    if (bterm->getBlock()->getTopModule() == modnet_parent) {
      bterm->connect(this);
    }
  }
}

bool dbModNet::isConnected(const dbNet* other) const
{
  dbNet* net = findRelatedNet();
  return (net == other);
}

bool dbModNet::isConnected(const dbModNet* other) const
{
  if (other == nullptr) {
    return false;
  }
  dbNet* net = findRelatedNet();
  dbNet* other_net = other->findRelatedNet();
  return (net == other_net);
}

// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
