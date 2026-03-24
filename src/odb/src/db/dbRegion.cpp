// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRegion.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbInst.h"
#include "dbRegionGroupItr.h"
#include "dbRegionInstItr.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {

template class dbTable<_dbRegion>;

_dbRegion::_dbRegion(_dbDatabase*)
{
  flags_.type = dbRegionType::INCLUSIVE;
  flags_.invalid = false;
  flags_.spare_bits = false;
  name_ = nullptr;
}

_dbRegion::_dbRegion(_dbDatabase*, const _dbRegion& r)
    : flags_(r.flags_),
      name_(nullptr),
      insts_(r.insts_),
      boxes_(r.boxes_),
      groups_(r.groups_)
{
  if (r.name_) {
    name_ = strdup(r.name_);
  }
}

_dbRegion::~_dbRegion()
{
  if (name_) {
    free((void*) name_);
  }
}

bool _dbRegion::operator==(const _dbRegion& rhs) const
{
  if (flags_.type != rhs.flags_.type) {
    return false;
  }

  if (flags_.invalid != rhs.flags_.invalid) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (insts_ != rhs.insts_) {
    return false;
  }

  if (groups_ != rhs.groups_) {
    return false;
  }

  return true;
}

bool _dbRegion::operator<(const _dbRegion& rhs) const
{
  if (flags_.type < rhs.flags_.type) {
    return false;
  }

  if (flags_.type > rhs.flags_.type) {
    return true;
  }

  if (flags_.invalid < rhs.flags_.invalid) {
    return false;
  }

  if (flags_.invalid > rhs.flags_.invalid) {
    return true;
  }

  if (insts_ < rhs.insts_) {
    return true;
  }

  if (groups_ < rhs.groups_) {
    return true;
  }
  return boxes_ < rhs.boxes_;
}

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r)
{
  uint32_t* bit_field = (uint32_t*) &r.flags_;
  stream << *bit_field;
  stream << r.name_;
  stream << r.insts_;
  stream << r.boxes_;
  stream << r.groups_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbRegion& r)
{
  uint32_t* bit_field = (uint32_t*) &r.flags_;
  stream >> *bit_field;
  stream >> r.name_;
  stream >> r.insts_;
  stream >> r.boxes_;
  stream >> r.groups_;

  return stream;
}

std::string dbRegion::getName()
{
  _dbRegion* region = (_dbRegion*) this;
  return region->name_;
}

dbRegionType dbRegion::getRegionType()
{
  _dbRegion* region = (_dbRegion*) this;
  dbRegionType t(region->flags_.type);
  return t;
}

void dbRegion::setInvalid(bool v)
{
  _dbRegion* region = (_dbRegion*) this;
  region->flags_.invalid = v;
}

bool dbRegion::isInvalid()
{
  _dbRegion* region = (_dbRegion*) this;
  return region->flags_.invalid == 1;
}

void dbRegion::setRegionType(dbRegionType type)
{
  _dbRegion* region = (_dbRegion*) this;
  region->flags_.type = type;
}

dbSet<dbInst> dbRegion::getRegionInsts()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  return dbSet<dbInst>(region, block->region_inst_itr_);
}

dbSet<dbBox> dbRegion::getBoundaries()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  return dbSet<dbBox>(region, block->box_itr_);
}

void dbRegion::addInst(dbInst* inst_)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  if (inst->region_ != 0) {
    dbRegion* r = dbRegion::getRegion((dbBlock*) block, inst->region_);
    r->removeInst(inst_);
  }

  inst->region_ = region->getOID();

  if (region->insts_ != 0) {
    _dbInst* tail = block->inst_tbl_->getPtr(region->insts_);
    inst->region_next_ = region->insts_;
    inst->region_prev_ = 0;
    tail->region_prev_ = inst->getOID();
  } else {
    inst->region_next_ = 0;
    inst->region_prev_ = 0;
  }

  region->insts_ = inst->getOID();
}

void dbRegion::removeInst(dbInst* inst_)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  uint32_t id = inst->getOID();

  if (region->insts_ == id) {
    region->insts_ = inst->region_next_;

    if (region->insts_ != 0) {
      _dbInst* t = block->inst_tbl_->getPtr(region->insts_);
      t->region_prev_ = 0;
    }
  } else {
    if (inst->region_next_ != 0) {
      _dbInst* next = block->inst_tbl_->getPtr(inst->region_next_);
      next->region_prev_ = inst->region_prev_;
    }

    if (inst->region_prev_ != 0) {
      _dbInst* prev = block->inst_tbl_->getPtr(inst->region_prev_);
      prev->region_next_ = inst->region_next_;
    }
  }

  inst->region_ = 0;
}
void dbRegion::removeGroup(dbGroup* group)
{
  _dbRegion* region = (_dbRegion*) this;
  _dbGroup* _group = (_dbGroup*) group;
  if (_group->region_ != region->getOID()) {
    return;
  }
  _dbBlock* block = (_dbBlock*) region->getOwner();

  uint32_t id = _group->getOID();

  if (region->groups_ == id) {
    region->groups_ = _group->region_next_;

    if (region->groups_ != 0) {
      _dbGroup* t = block->group_tbl_->getPtr(region->groups_);
      t->region_prev_ = 0;
    }
  } else {
    if (_group->region_next_ != 0) {
      _dbGroup* next = block->group_tbl_->getPtr(_group->region_next_);
      next->region_prev_ = _group->region_prev_;
    }

    if (_group->region_prev_ != 0) {
      _dbGroup* prev = block->group_tbl_->getPtr(_group->region_prev_);
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

  dbSet<dbGroup> groups(_region, _block->region_group_itr_);
  return groups;
}

dbRegion* dbRegion::create(dbBlock* block_, const char* name)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block_->findRegion(name)) {
    return nullptr;
  }

  _dbRegion* region = block->region_tbl_->create();
  region->name_ = safe_strdup(name);
  for (auto callback : block->callbacks_) {
    callback->inDbRegionCreate((dbRegion*) region);
  }
  return (dbRegion*) region;
}

void dbRegion::destroy(dbRegion* region_)
{
  _dbRegion* region = (_dbRegion*) region_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  for (auto callback : block->callbacks_) {
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
    block->box_tbl_->destroy((_dbBox*) box);
    bitr = next;
  }

  dbSet<dbGroup> groups = region_->getGroups();
  dbSet<dbGroup>::iterator gitr;

  for (gitr = groups.begin(); gitr != groups.end(); gitr = groups.begin()) {
    dbGroup* group = *gitr;
    region_->removeGroup(group);
  }

  dbProperty::destroyProperties(region);
  block->region_tbl_->destroy(region);
}

dbSet<dbRegion>::iterator dbRegion::destroy(dbSet<dbRegion>::iterator& itr)
{
  dbRegion* r = *itr;
  dbSet<dbRegion>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbRegion* dbRegion::getRegion(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRegion*) block->region_tbl_->getPtr(dbid_);
}

void _dbRegion::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
}

}  // namespace odb
