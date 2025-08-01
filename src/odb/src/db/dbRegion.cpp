// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRegion.h"

#include <string>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbInst.h"
#include "dbRegionGroupItr.h"
#include "dbRegionInstItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace odb {

template class dbTable<_dbRegion>;

_dbRegion::_dbRegion(_dbDatabase*)
{
  _flags._type = dbRegionType::INCLUSIVE;
  _flags._invalid = false;
  _flags._spare_bits = false;
  _name = nullptr;
}

_dbRegion::_dbRegion(_dbDatabase*, const _dbRegion& r)
    : _flags(r._flags),
      _name(nullptr),
      _insts(r._insts),
      _boxes(r._boxes),
      groups_(r.groups_)
{
  if (r._name) {
    _name = strdup(r._name);
  }
}

_dbRegion::~_dbRegion()
{
  if (_name) {
    free((void*) _name);
  }
}

bool _dbRegion::operator==(const _dbRegion& rhs) const
{
  if (_flags._type != rhs._flags._type) {
    return false;
  }

  if (_flags._invalid != rhs._flags._invalid) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_insts != rhs._insts) {
    return false;
  }

  if (groups_ != rhs.groups_) {
    return false;
  }

  return true;
}

bool _dbRegion::operator<(const _dbRegion& rhs) const
{
  if (_flags._type < rhs._flags._type) {
    return false;
  }

  if (_flags._type > rhs._flags._type) {
    return true;
  }

  if (_flags._invalid < rhs._flags._invalid) {
    return false;
  }

  if (_flags._invalid > rhs._flags._invalid) {
    return true;
  }

  if (_insts < rhs._insts) {
    return true;
  }

  if (groups_ < rhs.groups_) {
    return true;
  }
  return _boxes < rhs._boxes;
}

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r)
{
  uint* bit_field = (uint*) &r._flags;
  stream << *bit_field;
  stream << r._name;
  stream << r._insts;
  stream << r._boxes;
  stream << r.groups_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbRegion& r)
{
  uint* bit_field = (uint*) &r._flags;
  stream >> *bit_field;
  stream >> r._name;
  stream >> r._insts;
  stream >> r._boxes;
  stream >> r.groups_;

  return stream;
}

std::string dbRegion::getName()
{
  _dbRegion* region = (_dbRegion*) this;
  return region->_name;
}

dbRegionType dbRegion::getRegionType()
{
  _dbRegion* region = (_dbRegion*) this;
  dbRegionType t(region->_flags._type);
  return t;
}

void dbRegion::setInvalid(bool v)
{
  _dbRegion* region = (_dbRegion*) this;
  region->_flags._invalid = v;
}

bool dbRegion::isInvalid()
{
  _dbRegion* region = (_dbRegion*) this;
  return region->_flags._invalid == 1;
}

void dbRegion::setRegionType(dbRegionType type)
{
  _dbRegion* region = (_dbRegion*) this;
  region->_flags._type = type;
}

dbSet<dbInst> dbRegion::getRegionInsts()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  return dbSet<dbInst>(region, block->_region_inst_itr);
}

dbSet<dbBox> dbRegion::getBoundaries()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  return dbSet<dbBox>(region, block->_box_itr);
}

void dbRegion::addInst(dbInst* inst_)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  if (inst->_region != 0) {
    dbRegion* r = dbRegion::getRegion((dbBlock*) block, inst->_region);
    r->removeInst(inst_);
  }

  inst->_region = region->getOID();

  if (region->_insts != 0) {
    _dbInst* tail = block->_inst_tbl->getPtr(region->_insts);
    inst->_region_next = region->_insts;
    inst->_region_prev = 0;
    tail->_region_prev = inst->getOID();
  } else {
    inst->_region_next = 0;
    inst->_region_prev = 0;
  }

  region->_insts = inst->getOID();
}

void dbRegion::removeInst(dbInst* inst_)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  uint id = inst->getOID();

  if (region->_insts == id) {
    region->_insts = inst->_region_next;

    if (region->_insts != 0) {
      _dbInst* t = block->_inst_tbl->getPtr(region->_insts);
      t->_region_prev = 0;
    }
  } else {
    if (inst->_region_next != 0) {
      _dbInst* next = block->_inst_tbl->getPtr(inst->_region_next);
      next->_region_prev = inst->_region_prev;
    }

    if (inst->_region_prev != 0) {
      _dbInst* prev = block->_inst_tbl->getPtr(inst->_region_prev);
      prev->_region_next = inst->_region_next;
    }
  }

  inst->_region = 0;
}
void dbRegion::removeGroup(dbGroup* group)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbGroup* _group = (_dbGroup*) group;
  if (_group->region_ != region->getOID()) {
    return;
  }
  _dbBlock* block = (_dbBlock*) region->getOwner();

  uint id = _group->getOID();

  if (region->groups_ == id) {
    region->groups_ = _group->region_next_;

    if (region->groups_ != 0) {
      _dbGroup* t = block->_group_tbl->getPtr(region->groups_);
      t->region_prev_ = 0;
    }
  } else {
    if (_group->region_next_ != 0) {
      _dbGroup* next = block->_group_tbl->getPtr(_group->region_next_);
      next->region_prev_ = _group->region_prev_;
    }

    if (_group->region_prev_ != 0) {
      _dbGroup* prev = block->_group_tbl->getPtr(_group->region_prev_);
      prev->region_next_ = _group->region_next_;
    }
  }

  _group->region_ = 0;
}

dbBlock* dbRegion::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbRegion::addGroup(dbGroup* group)
{
  _dbRegion* _region = (_dbRegion*) this;
  _dbGroup* _group = (_dbGroup*) group;
  if (_group->region_) {
    return;
  }
  if (_region->groups_.isValid()) {
    _dbGroup* prev_group = (_dbGroup*) dbGroup::getGroup(
        (dbBlock*) _region->getOwner(), _region->groups_);
    prev_group->region_prev_ = _group->getOID();
  }
  _group->region_ = _region->getOID();
  _group->region_next_ = _region->groups_;
  _region->groups_ = _group->getOID();
}

dbSet<dbGroup> dbRegion::getGroups()
{
  _dbRegion* _region = (_dbRegion*) this;
  _dbBlock* _block = (_dbBlock*) _region->getOwner();

  dbSet<dbGroup> groups(_region, _block->_region_group_itr);
  return groups;
}

dbRegion* dbRegion::create(dbBlock* block_, const char* name)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block_->findRegion(name)) {
    return nullptr;
  }

  _dbRegion* region = block->_region_tbl->create();
  region->_name = safe_strdup(name);
  for (auto callback : block->_callbacks) {
    callback->inDbRegionCreate((dbRegion*) region);
  }
  return (dbRegion*) region;
}

void dbRegion::destroy(dbRegion* region_)
{
  _dbRegion* region = (_dbRegion*) region_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  for (auto callback : block->_callbacks) {
    callback->inDbRegionDestroy((dbRegion*) region);
  }

  dbSet<dbInst> insts = region_->getRegionInsts();
  dbSet<dbInst>::iterator iitr;

  for (iitr = insts.begin(); iitr != insts.end(); iitr = insts.begin()) {
    dbInst* inst = *iitr;
    region_->removeInst(inst);
  }

  dbSet<dbBox> boxes = region_->getBoundaries();
  dbSet<dbBox>::iterator bitr;

  for (bitr = boxes.begin(); bitr != boxes.end();) {
    dbBox* box = *bitr;
    dbSet<dbBox>::iterator next = ++bitr;
    dbProperty::destroyProperties(box);
    block->_box_tbl->destroy((_dbBox*) box);
    bitr = next;
  }

  dbSet<dbGroup> groups = region_->getGroups();
  dbSet<dbGroup>::iterator gitr;

  for (gitr = groups.begin(); gitr != groups.end(); gitr = groups.begin()) {
    _dbGroup* _group = (_dbGroup*) *gitr;
    _group->region_ = 0;
    _group->region_next_ = 0;
    _group->region_prev_ = 0;
  }

  dbProperty::destroyProperties(region);
  block->_region_tbl->destroy(region);
}

dbSet<dbRegion>::iterator dbRegion::destroy(dbSet<dbRegion>::iterator& itr)
{
  dbRegion* r = *itr;
  dbSet<dbRegion>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbRegion* dbRegion::getRegion(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRegion*) block->_region_tbl->getPtr(dbid_);
}

void _dbRegion::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
}

}  // namespace odb
