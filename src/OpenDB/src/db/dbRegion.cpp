///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbRegion.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbRegion.h"
#include "dbRegionInstItr.h"
#include "dbRegionItr.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbRegion>;

_dbRegion::_dbRegion(_dbDatabase*)
{
  _flags._type = dbRegionType::INCLUSIVE;
  _flags._invalid = false;
  _name = NULL;
}

_dbRegion::_dbRegion(_dbDatabase*, const _dbRegion& r)
    : _flags(r._flags),
      _name(NULL),
      _insts(r._insts),
      _boxes(r._boxes),
      _parent(r._parent),
      _children(r._children),
      _next_child(r._next_child)
{
  if (r._name)
    _name = strdup(r._name);
}

_dbRegion::~_dbRegion()
{
  if (_name)
    free((void*) _name);
}

bool _dbRegion::operator==(const _dbRegion& rhs) const
{
  if (_flags._type != rhs._flags._type)
    return false;

  if (_flags._invalid != rhs._flags._invalid)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_insts != rhs._insts)
    return false;

  if (_parent != rhs._parent)
    return false;

  if (_children != rhs._children)
    return false;

  if (_next_child != rhs._next_child)
    return false;

  return true;
}

bool _dbRegion::operator<(const _dbRegion& rhs) const
{
  if (_flags._type < rhs._flags._type)
    return false;

  if (_flags._type > rhs._flags._type)
    return true;

  if (_flags._invalid < rhs._flags._invalid)
    return false;

  if (_flags._invalid > rhs._flags._invalid)
    return true;

  if (_insts < rhs._insts)
    return true;

  if (_insts > rhs._insts)
    return false;

  if (_parent < rhs._parent)
    return true;

  if (_parent > rhs._parent)
    return false;

  if (_children < rhs._children)
    return true;

  if (_children > rhs._children)
    return false;

  if (_next_child < rhs._next_child)
    return true;

  if (_next_child > rhs._next_child)
    return false;

  return _boxes < rhs._boxes;
}

void _dbRegion::differences(dbDiff& diff,
                            const char* field,
                            const _dbRegion& rhs) const
{
  if (diff.deepDiff())
    return;

  DIFF_BEGIN
  DIFF_FIELD(_flags._type);
  DIFF_FIELD(_flags._invalid);
  DIFF_FIELD(_name);
  DIFF_FIELD(_insts);
  DIFF_FIELD(_boxes);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_children);
  DIFF_FIELD(_next_child);
  DIFF_END
}

void _dbRegion::out(dbDiff& diff, char side, const char* field) const
{
  if (diff.deepDiff())
    return;

  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._type);
  DIFF_OUT_FIELD(_flags._invalid);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_insts);
  DIFF_OUT_FIELD(_boxes);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_children);
  DIFF_OUT_FIELD(_next_child);
  DIFF_END
}

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r)
{
  uint* bit_field = (uint*) &r._flags;
  stream << *bit_field;
  stream << r._name;
  stream << r._insts;
  stream << r._boxes;
  stream << r._parent;
  stream << r._children;
  stream << r._next_child;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbRegion& r)
{
  uint* bit_field = (uint*) &r._flags;
  stream >> *bit_field;
  stream >> r._name;
  stream >> r._insts;
  stream >> r._boxes;
  stream >> r._parent;
  stream >> r._children;
  stream >> r._next_child;

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

dbRegion* dbRegion::getParent()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  if (region->_parent == 0)
    return NULL;

  _dbRegion* p = block->_region_tbl->getPtr(region->_parent);
  return (dbRegion*) p;
}

dbSet<dbRegion> dbRegion::getChildren()
{
  _dbRegion* region = (_dbRegion*) this;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  dbSet<dbRegion> children(region, block->_region_itr);
  return children;
}

void dbRegion::addChild(dbRegion* child_)
{
  _dbRegion* child = (_dbRegion*) child_;
  _dbRegion* parent = (_dbRegion*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();

  if (child->_parent || (parent == child))
    return;

  child->_parent = parent->getOID();
  child->_next_child = parent->_children;
  parent->_children = child->getOID();
}

dbBlock* dbRegion::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbRegion* dbRegion::create(dbBlock* block_, const char* name)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block_->findRegion(name))
    return NULL;

  _dbRegion* region = block->_region_tbl->create();
  region->_name = strdup(name);
  ZALLOCATED(region->_name);
  for (auto callback : block->_callbacks)
    callback->inDbRegionCreate((dbRegion*) region);
  return (dbRegion*) region;
}

dbRegion* dbRegion::create(dbRegion* parent_, const char* name)
{
  _dbRegion* parent = (_dbRegion*) parent_;
  _dbBlock* block = (_dbBlock*) parent->getOwner();

  if (((dbBlock*) block)->findRegion(name))
    return NULL;

  _dbRegion* region = block->_region_tbl->create();
  region->_name = strdup(name);
  ZALLOCATED(region->_name);
  region->_parent = parent->getOID();
  region->_next_child = parent->_children;
  parent->_children = region->getOID();
  for (auto callback : block->_callbacks)
    callback->inDbRegionCreate((dbRegion*) region);
  return (dbRegion*) region;
}

void dbRegion::destroy(dbRegion* region_)
{
  _dbRegion* region = (_dbRegion*) region_;
  _dbBlock* block = (_dbBlock*) region->getOwner();

  dbSet<dbRegion> children = region_->getChildren();
  dbSet<dbRegion>::iterator childItr;
  for (childItr = children.begin(); childItr != children.end();
       childItr = children.begin()) {
    dbRegion* child = *childItr;
    child->destroy(child);
  }
  for (auto callback : block->_callbacks)
    callback->inDbRegionDestroy((dbRegion*) region);

  dbSet<dbInst> insts = region_->getRegionInsts();
  dbSet<dbInst>::iterator iitr;

  for (iitr = insts.begin(); iitr != insts.end(); iitr = insts.begin()) {
    dbInst* inst = *iitr;
    region_->removeInst(inst);
  }

  dbSet<dbBox> boxes = region_->getBoundaries();
  dbSet<dbBox>::iterator bitr;

  for (bitr = boxes.begin(); bitr != boxes.end();) {
    dbSet<dbBox>::iterator next = ++bitr;
    dbBox* box = *bitr;
    dbProperty::destroyProperties(box);
    block->_box_tbl->destroy((_dbBox*) box);
    bitr = next;
  }

  if (region->_parent) {
    _dbRegion* parent = block->_region_tbl->getPtr(region->_parent);
    _dbRegion* prev = NULL;
    _dbRegion* cur = block->_region_tbl->getPtr(parent->_children);

    while (cur) {
      if (cur == region)
        break;

      prev = cur;
      cur = block->_region_tbl->getPtr(cur->_next_child);
    }

    if (prev == NULL)
      parent->_children = region->_next_child;
    else
      prev->_next_child = region->_next_child;
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

}  // namespace odb
