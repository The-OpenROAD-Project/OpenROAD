// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModNet.h"

#include <cstdlib>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "boost/container/small_vector.hpp"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbModuleModNetBTermItr.h"
#include "dbModuleModNetITermItr.h"
#include "dbModuleModNetModBTermItr.h"
#include "dbModuleModNetModITermItr.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
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
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.parent_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.next_entry_;
  }
  if (obj.getDatabase()->isSchema(kSchemaHierPortRemoval)) {
    stream >> obj.prev_entry_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.moditerms_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.modbterms_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.iterms_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.bterms_;
  }
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
  info.children["name"].add(name_);
  // User Code End collectMemInfo
}

_dbModNet::~_dbModNet()
{
  if (name_) {
    free((void*) name_);
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

  if (getParent()->getModNet(new_name) != nullptr) {
    block->getLogger()->error(
        utl::ODB,
        495,
        "dbModNet::rename(): Found a duplicate dbModNet name '{}' in the "
        "module '{}'. Review the name creation logic.",
        new_name,
        getParent()->getName());
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: {}, rename to '{}'",
             obj->getDebugName(),
             new_name);

  if (block->journal_) {
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
  logger->report("dbModNet: {} (id={})", getHierarchicalName(), getId());
  logger->report("  Parent Module: {} (id={})",
                 getParent()->getName(),
                 getParent()->getId());

  logger->report("  ModITerms ({}):", getModITerms().size());
  for (dbModITerm* moditerm : getModITerms()) {
    // For dbModITerm, get types from child dbModBTerm
    dbModBTerm* child_modbterm = moditerm->getChildModBTerm();
    if (child_modbterm) {
      logger->report("    - {} ({}, {}, id={})",
                     child_modbterm->getHierarchicalName(),
                     child_modbterm->getSigType().getString(),
                     child_modbterm->getIoType().getString(),
                     moditerm->getId());
    } else {
      logger->report("    - {} (no child bterm, id={})",
                     moditerm->getName(),
                     moditerm->getId());
    }
  }

  logger->report("  ModBTerms ({}):", getModBTerms().size());
  for (dbModBTerm* term : getModBTerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getHierarchicalName(),
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

dbModNet* dbModNet::getModNet(dbBlock* block, uint32_t id)
{
  _dbBlock* block_ = (_dbBlock*) block;
  _dbModNet* ret = block_->modnet_tbl_->getPtr(id);
  return (dbModNet*) ret;
}

dbModNet* dbModNet::create(dbModule* parent_module, const char* base_name)
{
  _dbModule* parent = (_dbModule*) parent_module;
  _dbBlock* block = (_dbBlock*) parent->getOwner();

  if (parent_module->getModNet(base_name) != nullptr) {
    block->getLogger()->error(
        utl::ODB,
        492,
        "dbModNet::create(): Found a duplicate dbModNet name '{}' in the "
        "module '{}'. Review the name creation logic.",
        base_name,
        parent_module->getHierarchicalName());
  }

  // give illusion of scoping.
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

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             modnet->getDebugName());

  if (block->journal_) {
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

dbModNet* dbModNet::create(dbModule* parent_module,
                           const char* base_name,
                           const dbNameUniquifyType& uniquify,
                           dbNet* corresponding_flat_net)
{
  dbBlock* block = parent_module->getOwner();
  std::string net_name = block->makeNewModNetName(
      parent_module, base_name, uniquify, corresponding_flat_net);
  return create(parent_module, block->getBaseName(net_name.c_str()));
}

void dbModNet::destroy(dbModNet* mod_net)
{
  _dbModNet* _modnet = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) _modnet->getOwner();
  _dbModule* module = block->module_tbl_->getPtr(_modnet->parent_);

  mod_net->disconnectAllTerms();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             mod_net->getDebugName());

  // journalling
  if (block->journal_) {
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

  uint32_t prev = _modnet->prev_entry_;
  uint32_t next = _modnet->next_entry_;
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
  std::set<const dbModNet*> visited_modnets;
  boost::container::small_vector<const dbModNet*, 16> modnets_to_visit;

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
  boost::container::small_vector<dbITerm*, 16> iterms(iterms_set.begin(),
                                                      iterms_set.end());
  for (dbITerm* iterm : iterms) {
    iterm->connect(this);
  }

  auto bterms_set = in_modnet->getBTerms();
  boost::container::small_vector<dbBTerm*, 16> bterms(bterms_set.begin(),
                                                      bterms_set.end());
  for (dbBTerm* bterm : bterms) {
    bterm->connect(this);
  }

  auto moditerms_set = in_modnet->getModITerms();
  boost::container::small_vector<dbModITerm*, 16> moditerms(
      moditerms_set.begin(), moditerms_set.end());
  for (dbModITerm* moditerm : moditerms) {
    moditerm->connect(this);
  }

  auto modbterms_set = in_modnet->getModBTerms();
  boost::container::small_vector<dbModBTerm*, 16> modbterms(
      modbterms_set.begin(), modbterms_set.end());
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
  boost::container::small_vector<dbITerm*, 16> iterms(iterms_set.begin(),
                                                      iterms_set.end());
  dbModule* modnet_parent = getParent();
  for (dbITerm* iterm : iterms) {
    // Only connect terminals that are in the same module as the modnet
    if (iterm->getInst()->getModule() == modnet_parent) {
      iterm->connect(this);
    }
  }

  auto bterms_set = in_net->getBTerms();
  boost::container::small_vector<dbBTerm*, 16> bterms(bterms_set.begin(),
                                                      bterms_set.end());
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

std::vector<dbModNet*> dbModNet::getNextModNetsInFanin() const
{
  std::vector<dbModNet*> next_modnets;

  // 1. Upward: This module's input port -> parent's instance pin -> parent's
  // net
  for (dbModBTerm* modbterm : getModBTerms()) {
    if (modbterm->getIoType() != dbIoType::INPUT
        && modbterm->getIoType() != dbIoType::INOUT) {
      continue;
    }

    dbModITerm* parent_moditerm = modbterm->getParentModITerm();
    if (parent_moditerm == nullptr) {
      continue;
    }

    if (dbModNet* modnet = parent_moditerm->getModNet()) {
      next_modnets.push_back(modnet);
    }
  }

  // 2. Downward: Child's instance pin -> child's output port -> child's net
  for (dbModITerm* moditerm : getModITerms()) {
    dbModBTerm* child_modbterm = moditerm->getChildModBTerm();
    if (child_modbterm == nullptr) {
      continue;
    }

    if (child_modbterm->getIoType() != dbIoType::OUTPUT
        && child_modbterm->getIoType() != dbIoType::INOUT) {
      continue;
    }

    if (dbModNet* modnet = child_modbterm->getModNet()) {
      next_modnets.push_back(modnet);
    }
  }

  return next_modnets;
}

std::vector<dbModNet*> dbModNet::getNextModNetsInFanout() const
{
  std::vector<dbModNet*> next_modnets;

  // 1. Downward: This net -> child's instance pin -> child's input port ->
  // child's net
  for (dbModITerm* moditerm : getModITerms()) {
    dbModBTerm* child_modbterm = moditerm->getChildModBTerm();
    if (child_modbterm == nullptr) {
      continue;
    }

    // Child's INPUT bterm receives signal from this modnet (fanout)
    if (child_modbterm->getIoType() != dbIoType::INPUT
        && child_modbterm->getIoType() != dbIoType::INOUT) {
      continue;
    }

    if (dbModNet* modnet = child_modbterm->getModNet()) {
      next_modnets.push_back(modnet);
    }
  }

  // 2. Upward: This net -> current module's output port -> parent's instance
  // pin -> parent's net
  for (dbModBTerm* modbterm : getModBTerms()) {
    if (modbterm->getIoType() != dbIoType::OUTPUT
        && modbterm->getIoType() != dbIoType::INOUT) {
      continue;
    }

    dbModITerm* parent_moditerm = modbterm->getParentModITerm();
    if (parent_moditerm == nullptr) {
      continue;
    }

    if (dbModNet* modnet = parent_moditerm->getModNet()) {
      next_modnets.push_back(modnet);
    }
  }

  return next_modnets;
}

dbModNet* dbModNet::findInHierarchy(
    const std::function<bool(dbModNet*)>& condition,
    dbHierSearchDir dir) const
{
  boost::container::small_vector<dbModNet*, 16> worklist;
  std::set<dbModNet*> visited;
  worklist.push_back(const_cast<dbModNet*>(this));
  visited.insert(const_cast<dbModNet*>(this));

  for (size_t i = 0; i < worklist.size(); ++i) {
    dbModNet* curr = worklist[i];

    // Return if the condition is met
    if (condition(curr)) {
      return curr;
    }

    std::vector<dbModNet*> next_nets = (dir == dbHierSearchDir::FANOUT)
                                           ? curr->getNextModNetsInFanout()
                                           : curr->getNextModNetsInFanin();
    for (dbModNet* next : next_nets) {
      if (visited.insert(next).second) {
        worklist.push_back(next);
      }
    }
  }

  return nullptr;
}

// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
