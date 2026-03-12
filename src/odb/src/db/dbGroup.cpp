// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroup.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbGroupInstItr.h"
#include "dbGroupItr.h"
#include "dbGroupModInstItr.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModInst.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbCommon.h"
#include "dbGroupGroundNetItr.h"
#include "dbGroupPowerNetItr.h"
#include "dbRegion.h"
#include "dbVector.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGroup>;

bool _dbGroup::operator==(const _dbGroup& rhs) const
{
  if (flags_.type != rhs.flags_.type) {
    return false;
  }
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (group_next_ != rhs.group_next_) {
    return false;
  }
  if (parent_group_ != rhs.parent_group_) {
    return false;
  }
  if (insts_ != rhs.insts_) {
    return false;
  }
  if (modinsts_ != rhs.modinsts_) {
    return false;
  }
  if (groups_ != rhs.groups_) {
    return false;
  }
  if (region_next_ != rhs.region_next_) {
    return false;
  }
  if (region_prev_ != rhs.region_prev_) {
    return false;
  }
  if (region_ != rhs.region_) {
    return false;
  }

  // User Code Begin ==
  if (power_nets_ != rhs.power_nets_) {
    return false;
  }

  if (ground_nets_ != rhs.ground_nets_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbGroup::operator<(const _dbGroup& rhs) const
{
  // User Code Begin <
  if (strcmp(name_, rhs.name_) >= 0) {
    return false;
  }
  if (flags_.type >= rhs.flags_.type) {
    return false;
  }
  // User Code End <
  return true;
}

_dbGroup::_dbGroup(_dbDatabase* db)
{
  flags_ = {};
  name_ = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbGroup& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.group_next_;
  stream >> obj.parent_group_;
  stream >> obj.insts_;
  stream >> obj.modinsts_;
  stream >> obj.groups_;
  stream >> obj.power_nets_;
  stream >> obj.ground_nets_;
  stream >> obj.region_next_;
  stream >> obj.region_prev_;
  stream >> obj.region_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGroup& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.group_next_;
  stream << obj.parent_group_;
  stream << obj.insts_;
  stream << obj.modinsts_;
  stream << obj.groups_;
  stream << obj.power_nets_;
  stream << obj.ground_nets_;
  stream << obj.region_next_;
  stream << obj.region_prev_;
  stream << obj.region_;
  return stream;
}

void _dbGroup::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["power_nets"].add(power_nets_);
  info.children["ground_nets"].add(ground_nets_);
  // User Code End collectMemInfo
}

_dbGroup::~_dbGroup()
{
  if (name_) {
    free((void*) name_);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbGroup - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbGroup::getName() const
{
  _dbGroup* obj = (_dbGroup*) this;
  return obj->name_;
}

dbGroup* dbGroup::getParentGroup() const
{
  _dbGroup* obj = (_dbGroup*) this;
  if (obj->parent_group_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->group_tbl_->getPtr(obj->parent_group_);
}

dbRegion* dbGroup::getRegion() const
{
  _dbGroup* obj = (_dbGroup*) this;
  if (obj->region_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbRegion*) par->region_tbl_->getPtr(obj->region_);
}

// User Code Begin dbGroupPublicMethods
void dbGroup::setType(dbGroupType type)
{
  _dbGroup* obj = (_dbGroup*) this;

  obj->flags_.type = (uint32_t) type;
}

dbGroupType dbGroup::getType() const
{
  _dbGroup* obj = (_dbGroup*) this;

  return (dbGroupType::Value) obj->flags_.type;
}

void dbGroup::addModInst(dbModInst* modinst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->group_ != 0) {
    modinst->getGroup()->removeModInst(modinst);
  }
  _modinst->group_ = _group->getOID();
  _modinst->group_next_ = _group->modinsts_;
  _group->modinsts_ = _modinst->getOID();
}

void dbGroup::removeModInst(dbModInst* modinst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->group_ != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint32_t id = _modinst->getOID();
  _dbModInst* prev = nullptr;
  uint32_t cur = _group->modinsts_;
  while (cur) {
    _dbModInst* c = _block->modinst_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->modinsts_ = _modinst->group_next_;
      } else {
        prev->group_next_ = _modinst->group_next_;
      }
      break;
    }
    prev = c;
    cur = c->group_next_;
  }
  _modinst->group_ = 0;
  _modinst->group_next_ = 0;
}

dbSet<dbModInst> dbGroup::getModInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbModInst>(_group, block->group_modinst_itr_);
}

void dbGroup::addInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->group_ != 0) {
    inst->getGroup()->removeInst(inst);
  }
  _inst->group_ = _group->getOID();
  _inst->group_next_ = _group->insts_;
  _group->insts_ = _inst->getOID();
}

void dbGroup::removeInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->group_ != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint32_t id = _inst->getOID();
  _dbInst* prev = nullptr;
  uint32_t cur = _group->insts_;
  while (cur) {
    _dbInst* c = _block->inst_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->insts_ = _inst->group_next_;
      } else {
        prev->group_next_ = _inst->group_next_;
      }
      break;
    }
    prev = c;
    cur = c->group_next_;
  }
  _inst->group_ = 0;
  _inst->group_next_ = 0;
}

dbSet<dbInst> dbGroup::getInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbInst>(_group, block->group_inst_itr_);
}

void dbGroup::addGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->parent_group_ != 0) {
    child->getParentGroup()->removeGroup(child);
  }
  _child->parent_group_ = _group->getOID();
  _child->group_next_ = _group->groups_;
  _group->groups_ = _child->getOID();
}

void dbGroup::removeGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->parent_group_ != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint32_t id = _child->getOID();
  _dbGroup* prev = nullptr;
  uint32_t cur = _group->groups_;
  while (cur) {
    _dbGroup* c = _block->group_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->groups_ = _child->group_next_;
      } else {
        prev->group_next_ = _child->group_next_;
      }
      break;
    }
    prev = c;
    cur = c->group_next_;
  }
  _child->parent_group_ = 0;
  _child->group_next_ = 0;
}

dbSet<dbGroup> dbGroup::getGroups()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbGroup>(_group, block->group_itr_);
}

void dbGroup::addPowerNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  for (const dbId<_dbNet>& _child : _group->power_nets_) {
    if (_child == _net->getOID()) {
      return;
    }
  }
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->ground_nets_.begin();
       it != _group->ground_nets_.end() && !found;
       it++) {
    if (*it == _net->getOID()) {
      _group->ground_nets_.erase(it--);
      found = true;
    }
  }
  _group->power_nets_.push_back(_net->getOID());
  if (!found) {
    _net->groups_.push_back(_group->getOID());
  }
}

void dbGroup::addGroundNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  for (const dbId<_dbNet>& _child : _group->ground_nets_) {
    if (_child == _net->getOID()) {
      return;
    }
  }
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->power_nets_.begin();
       it != _group->power_nets_.end() && !found;
       it++) {
    if (*it == _net->getOID()) {
      _group->power_nets_.erase(it--);
      found = true;
    }
  }
  _group->ground_nets_.push_back(_net->getOID());
  if (!found) {
    _net->groups_.push_back(_group->getOID());
  }
}

void dbGroup::removeNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator net_itr;
  for (net_itr = _group->power_nets_.begin();
       net_itr != _group->power_nets_.end() && !found;
       net_itr++) {
    if (*net_itr == _net->getOID()) {
      _group->power_nets_.erase(net_itr--);
      found = true;
    }
  }
  for (net_itr = _group->ground_nets_.begin();
       net_itr != _group->ground_nets_.end() && !found;
       net_itr++) {
    if (*net_itr == _net->getOID()) {
      _group->ground_nets_.erase(net_itr--);
      found = true;
    }
  }
  if (found) {
    dbVector<dbId<_dbGroup>>::iterator group_itr;
    for (group_itr = _net->groups_.begin(); group_itr != _net->groups_.end();
         group_itr++) {
      if (*group_itr == _group->getOID()) {
        _net->groups_.erase(group_itr--);
        return;
      }
    }
  }
}

dbSet<dbNet> dbGroup::getPowerNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->group_power_net_itr_);
}

dbSet<dbNet> dbGroup::getGroundNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->group_ground_net_itr_);
}

dbGroup* dbGroup::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->group_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->group_tbl_->create();
  _group->name_ = safe_strdup(name);
  _group->flags_.type = dbGroupType::PHYSICAL_CLUSTER;
  _block->group_hash_.insert(_group);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbGroup* parent, const char* name)
{
  _dbGroup* _parent = (_dbGroup*) parent;
  _dbBlock* _block = (_dbBlock*) _parent->getOwner();
  if (_block->group_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->group_tbl_->create();
  _group->name_ = safe_strdup(name);
  _group->flags_.type = dbGroupType::PHYSICAL_CLUSTER;
  _block->group_hash_.insert(_group);
  parent->addGroup((dbGroup*) _group);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbRegion* region, const char* name)
{
  _dbRegion* _region = (_dbRegion*) region;
  _dbBlock* _block = (_dbBlock*) _region->getOwner();
  if (_block->group_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->group_tbl_->create();
  _group->name_ = safe_strdup(name);
  _group->flags_.type = dbGroupType::PHYSICAL_CLUSTER;
  _block->group_hash_.insert(_group);
  region->addGroup((dbGroup*) _group);
  return (dbGroup*) _group;
}

void dbGroup::destroy(dbGroup* group)
{
  _dbGroup* _group = (_dbGroup*) group;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  while (!group->getInsts().empty()) {
    group->removeInst(*group->getInsts().begin());
  }
  if (_group->region_.isValid()) {
    group->getRegion()->removeGroup(group);
  }
  while (!group->getModInsts().empty()) {
    group->removeModInst(*group->getModInsts().begin());
  }
  while (!group->getGroups().empty()) {
    group->removeGroup(*group->getGroups().begin());
  }
  if (_group->parent_group_.isValid()) {
    group->getParentGroup()->removeGroup(group);
  }
  dbProperty::destroyProperties(_group);
  block->group_hash_.remove(_group);
  block->group_tbl_->destroy(_group);
}

dbGroup* dbGroup::getGroup(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbGroup*) block->group_tbl_->getPtr(dbid_);
}

// User Code End dbGroupPublicMethods
}  // namespace odb
// Generator Code End Cpp
