// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroup.h"

#include <cstdint>
#include <cstring>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbDatabase.h"
#include "dbGroupInstItr.h"
#include "dbGroupItr.h"
#include "dbGroupModInstItr.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModInst.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include "dbGroupGroundNetItr.h"
#include "dbGroupPowerNetItr.h"
#include "dbRegion.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGroup>;

bool _dbGroup::operator==(const _dbGroup& rhs) const
{
  if (flags_._type != rhs.flags_._type) {
    return false;
  }
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_group_next != rhs._group_next) {
    return false;
  }
  if (_parent_group != rhs._parent_group) {
    return false;
  }
  if (_insts != rhs._insts) {
    return false;
  }
  if (_modinsts != rhs._modinsts) {
    return false;
  }
  if (_groups != rhs._groups) {
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
  if (_power_nets != rhs._power_nets) {
    return false;
  }

  if (_ground_nets != rhs._ground_nets) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbGroup::operator<(const _dbGroup& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0) {
    return false;
  }
  if (flags_._type >= rhs.flags_._type) {
    return false;
  }
  // User Code End <
  return true;
}

_dbGroup::_dbGroup(_dbDatabase* db)
{
  flags_ = {};
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbGroup& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._group_next;
  stream >> obj._parent_group;
  stream >> obj._insts;
  stream >> obj._modinsts;
  stream >> obj._groups;
  stream >> obj._power_nets;
  stream >> obj._ground_nets;
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
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._group_next;
  stream << obj._parent_group;
  stream << obj._insts;
  stream << obj._modinsts;
  stream << obj._groups;
  stream << obj._power_nets;
  stream << obj._ground_nets;
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
  info.children_["name"].add(_name);
  info.children_["power_nets"].add(_power_nets);
  info.children_["ground_nets"].add(_ground_nets);
  // User Code End collectMemInfo
}

_dbGroup::~_dbGroup()
{
  if (_name) {
    free((void*) _name);
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
  return obj->_name;
}

dbGroup* dbGroup::getParentGroup() const
{
  _dbGroup* obj = (_dbGroup*) this;
  if (obj->_parent_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_parent_group);
}

dbRegion* dbGroup::getRegion() const
{
  _dbGroup* obj = (_dbGroup*) this;
  if (obj->region_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbRegion*) par->_region_tbl->getPtr(obj->region_);
}

// User Code Begin dbGroupPublicMethods
void dbGroup::setType(dbGroupType type)
{
  _dbGroup* obj = (_dbGroup*) this;

  obj->flags_._type = (uint) type;
}

dbGroupType dbGroup::getType() const
{
  _dbGroup* obj = (_dbGroup*) this;

  return (dbGroupType::Value) obj->flags_._type;
}

void dbGroup::addModInst(dbModInst* modinst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->_group != 0) {
    modinst->getGroup()->removeModInst(modinst);
  }
  _modinst->_group = _group->getOID();
  _modinst->_group_next = _group->_modinsts;
  _group->_modinsts = _modinst->getOID();
}

void dbGroup::removeModInst(dbModInst* modinst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->_group != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint id = _modinst->getOID();
  _dbModInst* prev = nullptr;
  uint cur = _group->_modinsts;
  while (cur) {
    _dbModInst* c = _block->_modinst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->_modinsts = _modinst->_group_next;
      } else {
        prev->_group_next = _modinst->_group_next;
      }
      break;
    }
    prev = c;
    cur = c->_group_next;
  }
  _modinst->_group = 0;
  _modinst->_group_next = 0;
}

dbSet<dbModInst> dbGroup::getModInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbModInst>(_group, block->_group_modinst_itr);
}

void dbGroup::addInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->_group != 0) {
    inst->getGroup()->removeInst(inst);
  }
  _inst->_group = _group->getOID();
  _inst->_group_next = _group->_insts;
  _group->_insts = _inst->getOID();
}

void dbGroup::removeInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->_group != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint id = _inst->getOID();
  _dbInst* prev = nullptr;
  uint cur = _group->_insts;
  while (cur) {
    _dbInst* c = _block->_inst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->_insts = _inst->_group_next;
      } else {
        prev->_group_next = _inst->_group_next;
      }
      break;
    }
    prev = c;
    cur = c->_group_next;
  }
  _inst->_group = 0;
  _inst->_group_next = 0;
}

dbSet<dbInst> dbGroup::getInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbInst>(_group, block->_group_inst_itr);
}

void dbGroup::addGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->_parent_group != 0) {
    child->getParentGroup()->removeGroup(child);
  }
  _child->_parent_group = _group->getOID();
  _child->_group_next = _group->_groups;
  _group->_groups = _child->getOID();
}

void dbGroup::removeGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->_parent_group != _group->getOID()) {
    return;
  }
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint id = _child->getOID();
  _dbGroup* prev = nullptr;
  uint cur = _group->_groups;
  while (cur) {
    _dbGroup* c = _block->_group_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _group->_groups = _child->_group_next;
      } else {
        prev->_group_next = _child->_group_next;
      }
      break;
    }
    prev = c;
    cur = c->_group_next;
  }
  _child->_parent_group = 0;
  _child->_group_next = 0;
}

dbSet<dbGroup> dbGroup::getGroups()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  return dbSet<dbGroup>(_group, block->_group_itr);
}

void dbGroup::addPowerNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  for (const dbId<_dbNet>& _child : _group->_power_nets) {
    if (_child == _net->getOID()) {
      return;
    }
  }
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->_ground_nets.begin();
       it != _group->_ground_nets.end() && !found;
       it++) {
    if (*it == _net->getOID()) {
      _group->_ground_nets.erase(it--);
      found = true;
    }
  }
  _group->_power_nets.push_back(_net->getOID());
  if (!found) {
    _net->_groups.push_back(_group->getOID());
  }
}

void dbGroup::addGroundNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  for (const dbId<_dbNet>& _child : _group->_ground_nets) {
    if (_child == _net->getOID()) {
      return;
    }
  }
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->_power_nets.begin();
       it != _group->_power_nets.end() && !found;
       it++) {
    if (*it == _net->getOID()) {
      _group->_power_nets.erase(it--);
      found = true;
    }
  }
  _group->_ground_nets.push_back(_net->getOID());
  if (!found) {
    _net->_groups.push_back(_group->getOID());
  }
}

void dbGroup::removeNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet* _net = (_dbNet*) net;
  bool found = false;
  dbVector<dbId<_dbNet>>::iterator net_itr;
  for (net_itr = _group->_power_nets.begin();
       net_itr != _group->_power_nets.end() && !found;
       net_itr++) {
    if (*net_itr == _net->getOID()) {
      _group->_power_nets.erase(net_itr--);
      found = true;
    }
  }
  for (net_itr = _group->_ground_nets.begin();
       net_itr != _group->_ground_nets.end() && !found;
       net_itr++) {
    if (*net_itr == _net->getOID()) {
      _group->_ground_nets.erase(net_itr--);
      found = true;
    }
  }
  if (found) {
    dbVector<dbId<_dbGroup>>::iterator group_itr;
    for (group_itr = _net->_groups.begin(); group_itr != _net->_groups.end();
         group_itr++) {
      if (*group_itr == _group->getOID()) {
        _net->_groups.erase(group_itr--);
        return;
      }
    }
  }
}

dbSet<dbNet> dbGroup::getPowerNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->_group_power_net_itr);
}

dbSet<dbNet> dbGroup::getGroundNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->_group_ground_net_itr);
}

dbGroup* dbGroup::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_group_hash.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = dbGroupType::PHYSICAL_CLUSTER;
  _block->_group_hash.insert(_group);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbGroup* parent, const char* name)
{
  _dbGroup* _parent = (_dbGroup*) parent;
  _dbBlock* _block = (_dbBlock*) _parent->getOwner();
  if (_block->_group_hash.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = dbGroupType::PHYSICAL_CLUSTER;
  _block->_group_hash.insert(_group);
  parent->addGroup((dbGroup*) _group);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbRegion* region, const char* name)
{
  _dbRegion* _region = (_dbRegion*) region;
  _dbBlock* _block = (_dbBlock*) _region->getOwner();
  if (_block->_group_hash.hasMember(name)) {
    return nullptr;
  }
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = dbGroupType::PHYSICAL_CLUSTER;
  _block->_group_hash.insert(_group);
  region->addGroup((dbGroup*) _group);
  return (dbGroup*) _group;
}

void dbGroup::destroy(dbGroup* group)
{
  _dbGroup* _group = (_dbGroup*) group;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  for (auto inst : group->getInsts()) {
    group->removeInst(inst);
  }
  if (_group->region_.isValid()) {
    group->getRegion()->removeGroup(group);
  }
  for (auto modinst : group->getModInsts()) {
    group->removeModInst(modinst);
  }
  for (auto child : group->getGroups()) {
    group->removeGroup(child);
  }
  if (_group->_parent_group.isValid()) {
    group->getParentGroup()->removeGroup(group);
  }
  dbProperty::destroyProperties(_group);
  block->_group_hash.remove(_group);
  block->_group_tbl->destroy(_group);
}

dbGroup* dbGroup::getGroup(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbGroup*) block->_group_tbl->getPtr(dbid_);
}

// User Code End dbGroupPublicMethods
}  // namespace odb
// Generator Code End Cpp
