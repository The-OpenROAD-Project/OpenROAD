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

#include "dbInst.h"

#include <algorithm>

#include "db.h"
#include "dbArrayTable.h"
#include "dbArrayTable.hpp"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbBox.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbGroup.h"
#include "dbHier.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInstHdr.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModule.h"
#include "dbNet.h"
#include "dbNullIterator.h"
#include "dbRegion.h"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTransform.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbInst>;

class sortMTerm
{
 public:
  bool operator()(_dbMTerm* m1, _dbMTerm* m2)
  {
    return strcmp(m1->_name, m2->_name) < 0;
  }
};

class sortITerm
{
  _dbBlock* _block;

 public:
  sortITerm(_dbBlock* block) { _block = block; }

  bool operator()(uint it1, uint it2)
  {
    _dbITerm* iterm1 = _block->_iterm_tbl->getPtr(it1);
    _dbITerm* iterm2 = _block->_iterm_tbl->getPtr(it2);
    return iterm1->_flags._mterm_idx < iterm2->_flags._mterm_idx;
  }
};

void _dbInst::setInstBBox(_dbInst* inst)
{
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* box = block->_box_tbl->getPtr(inst->_bbox);
  block->remove_rect(box->_shape._rect);

  dbMaster* master = ((dbInst*) inst)->getMaster();
  master->getPlacementBoundary(box->_shape._rect);
  dbTransform transform(inst->_flags._orient, Point(inst->_x, inst->_y));
  transform.apply(box->_shape._rect);
  block->add_rect(box->_shape._rect);
}

_dbInst::_dbInst(_dbDatabase*)
{
  _flags._orient = dbOrientType::R0;
  _flags._status = dbPlacementStatus::NONE;
  _flags._user_flag_1 = 0;
  _flags._user_flag_2 = 0;
  _flags._user_flag_3 = 0;
  _flags._size_only = 0;
  _flags._dont_touch = 0;
  _flags._dont_size = 0;
  _flags._eco_create = 0;
  _flags._eco_destroy = 0;
  _flags._eco_modify = 0;
  _flags._source = dbSourceType::NONE;
  //_flags._spare_bits = 0;
  _flags._level = 0;
  _flags._input_cone = 0;
  _flags._inside_cone = 0;
  _name = 0;
  _x = 0;
  _y = 0;
  _weight = 0;
}

_dbInst::_dbInst(_dbDatabase*, const _dbInst& i)
    : _flags(i._flags),
      _name(NULL),
      _x(i._x),
      _y(i._y),
      _weight(i._weight),
      _next_entry(i._next_entry),
      _inst_hdr(i._inst_hdr),
      _bbox(i._bbox),
      _region(i._region),
      _module(i._module),
      _group(i._group),
      _region_next(i._region_next),
      _module_next(i._module_next),
      _group_next(i._group_next),
      _region_prev(i._region_prev),
      _hierarchy(i._hierarchy),
      _iterms(i._iterms),
      _halo(i._halo)
{
  if (i._name) {
    _name = strdup(i._name);
    ZALLOCATED(_name);
  }
}

_dbInst::~_dbInst()
{
  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst)
{
  uint* bit_field = (uint*) &inst._flags;
  stream << *bit_field;
  stream << inst._name;
  stream << inst._x;
  stream << inst._y;
  stream << inst._weight;
  stream << inst._next_entry;
  stream << inst._inst_hdr;
  stream << inst._bbox;
  stream << inst._region;
  stream << inst._module;
  stream << inst._group;
  stream << inst._region_next;
  stream << inst._module_next;
  stream << inst._group_next;
  stream << inst._region_prev;
  stream << inst._hierarchy;
  stream << inst._iterms;
  stream << inst._halo;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbInst& inst)
{
  uint* bit_field = (uint*) &inst._flags;
  stream >> *bit_field;
  stream >> inst._name;
  stream >> inst._x;
  stream >> inst._y;
  stream >> inst._weight;
  stream >> inst._next_entry;
  stream >> inst._inst_hdr;
  stream >> inst._bbox;
  stream >> inst._region;
  stream >> inst._module;
  stream >> inst._group;
  stream >> inst._region_next;
  stream >> inst._module_next;
  stream >> inst._group_next;
  stream >> inst._region_prev;
  stream >> inst._hierarchy;
  stream >> inst._iterms;
  stream >> inst._halo;

  return stream;
}

bool _dbInst::operator<(const _dbInst& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbInst::operator==(const _dbInst& rhs) const
{
  if (_flags._orient != rhs._flags._orient)
    return false;

  if (_flags._status != rhs._flags._status)
    return false;

  if (_flags._user_flag_1 != rhs._flags._user_flag_1)
    return false;

  if (_flags._user_flag_2 != rhs._flags._user_flag_2)
    return false;

  if (_flags._user_flag_3 != rhs._flags._user_flag_3)
    return false;

  if (_flags._size_only != rhs._flags._size_only)
    return false;

  if (_flags._dont_size != rhs._flags._dont_size)
    return false;

  if (_flags._dont_touch != rhs._flags._dont_touch)
    return false;

  if (_flags._source != rhs._flags._source)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_x != rhs._x)
    return false;

  if (_y != rhs._y)
    return false;

  if (_weight != rhs._weight)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_inst_hdr != rhs._inst_hdr)
    return false;

  if (_bbox != rhs._bbox)
    return false;

  if (_region != rhs._region)
    return false;

  if (_module != rhs._module)
    return false;

  if (_group != rhs._group)
    return false;

  if (_region_next != rhs._region_next)
    return false;

  if (_module_next != rhs._module_next)
    return false;

  if (_group_next != rhs._group_next)
    return false;

  if (_region_prev != rhs._region_prev)
    return false;

  if (_hierarchy != rhs._hierarchy)
    return false;

  if (_iterms != rhs._iterms)
    return false;

  if (_halo != rhs._halo)
    return false;

  return true;
}

void _dbInst::differences(dbDiff& diff,
                          const char* field,
                          const _dbInst& rhs) const
{
  _dbBlock* lhs_blk = (_dbBlock*) getOwner();
  _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();

  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._orient);
  DIFF_FIELD(_flags._status);
  DIFF_FIELD(_flags._user_flag_1);
  DIFF_FIELD(_flags._user_flag_2);
  DIFF_FIELD(_flags._user_flag_3);
  DIFF_FIELD(_flags._size_only);
  DIFF_FIELD(_flags._dont_touch);
  DIFF_FIELD(_flags._dont_size);
  DIFF_FIELD(_flags._source);
  DIFF_FIELD(_x);
  DIFF_FIELD(_y);
  DIFF_FIELD(_weight);
  DIFF_FIELD_NO_DEEP(_next_entry);
  DIFF_FIELD_NO_DEEP(_inst_hdr);
  DIFF_OBJECT(_bbox, lhs_blk->_box_tbl, rhs_blk->_box_tbl);
  DIFF_FIELD(_region);
  DIFF_FIELD(_module);
  DIFF_FIELD(_group);
  DIFF_FIELD(_region_next);
  DIFF_FIELD(_module_next);
  DIFF_FIELD(_group_next);
  DIFF_FIELD(_region_prev);
  DIFF_FIELD(_hierarchy);
  DIFF_OBJECT(_halo, lhs_blk->_box_tbl, rhs_blk->_box_tbl);

  if (!diff.deepDiff()) {
    DIFF_VECTOR(_iterms);
  } else {
    dbSet<_dbITerm>::iterator itr;

    dbSet<_dbITerm> lhs_set((dbObject*) this, lhs_blk->_inst_iterm_itr);
    std::vector<_dbITerm*> lhs_vec;

    for (itr = lhs_set.begin(); itr != lhs_set.end(); ++itr)
      lhs_vec.push_back(*itr);

    dbSet<_dbITerm> rhs_set((dbObject*) &rhs, rhs_blk->_inst_iterm_itr);
    std::vector<_dbITerm*> rhs_vec;

    for (itr = rhs_set.begin(); itr != rhs_set.end(); ++itr)
      rhs_vec.push_back(*itr);

#ifndef WIN32  // This line cause a compiler error in visual stdio 6.0
    set_symmetric_diff(diff, "_iterms", lhs_vec, rhs_vec);
#endif
  }

  DIFF_END
}

void _dbInst::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* blk = (_dbBlock*) getOwner();
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._orient);
  DIFF_OUT_FIELD(_flags._status);
  DIFF_OUT_FIELD(_flags._user_flag_1);
  DIFF_OUT_FIELD(_flags._user_flag_2);
  DIFF_OUT_FIELD(_flags._user_flag_3);
  DIFF_OUT_FIELD(_flags._size_only);
  DIFF_OUT_FIELD(_flags._dont_touch);
  DIFF_OUT_FIELD(_flags._dont_size);
  DIFF_OUT_FIELD(_flags._source);
  DIFF_OUT_FIELD(_x);
  DIFF_OUT_FIELD(_y);
  DIFF_OUT_FIELD(_weight);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);
  DIFF_OUT_FIELD_NO_DEEP(_inst_hdr);
  DIFF_OUT_OBJECT(_bbox, blk->_box_tbl);
  DIFF_OUT_FIELD(_region);
  DIFF_OUT_FIELD(_module);
  DIFF_OUT_FIELD(_group);
  DIFF_OUT_FIELD(_region_next);
  DIFF_OUT_FIELD(_module_next);
  DIFF_OUT_FIELD(_group_next);
  DIFF_OUT_FIELD(_region_prev);
  DIFF_OUT_FIELD(_hierarchy);

  if (!diff.deepDiff()) {
    DIFF_OUT_VECTOR(_iterms);
  } else {
    dbSet<_dbITerm>::iterator itr;
    dbSet<_dbITerm> insts((dbObject*) this, blk->_inst_iterm_itr);
    diff.begin_object("%c _iterms\n", side);

    for (itr = insts.begin(); itr != insts.end(); ++itr)
      (*itr)->out(diff, side, "");

    diff.end_object();
  }

  DIFF_OUT_OBJECT(_halo, blk->_box_tbl);

  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbInst - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbInst::getName()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_name;
}

const char* dbInst::getConstName()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_name;
}

bool dbInst::isNamed(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  if (!strcmp(inst->_name, name))
    return true;
  return false;
}

bool dbInst::rename(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (block->_inst_hash.hasMember(name))
    return false;

  block->_inst_hash.remove(inst);
  free((void*) inst->_name);
  inst->_name = strdup(name);
  ZALLOCATED(inst->_name);
  block->_inst_hash.insert(inst);

  return true;
}

void dbInst::getOrigin(int& x, int& y)
{
  _dbInst* inst = (_dbInst*) this;
  x = inst->_x;
  y = inst->_y;
}

void dbInst::setOrigin(int x, int y)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  int prev_x = inst->_x;
  int prev_y = inst->_y;
  // Do Nothin if same origin, But What if uninitialized and x=y=0
  if (prev_x == x && prev_y == y)
    return;
  for (auto callback : block->_callbacks)
    callback->inDbPreMoveInst(this);

  inst->_x = x;
  inst->_y = y;
  _dbInst::setInstBBox(inst);

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setOrigin {}, {}",
               x,
               y);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(inst->getObjectType());
    block->_journal->pushParam(inst->getId());
    block->_journal->pushParam(_dbInst::ORIGIN);
    block->_journal->pushParam(prev_x);
    block->_journal->pushParam(prev_y);
    block->_journal->pushParam(inst->_x);
    block->_journal->pushParam(inst->_y);
    block->_journal->endAction();
  }

  block->_flags._valid_bbox = 0;
  for (auto callback : block->_callbacks)
    callback->inDbPostMoveInst(this);
}

void dbInst::setLocationOrient(dbOrientType orient)
{
  int x, y;
  getLocation(x, y);
  setOrient(orient);
  setLocation(x, y);
}

void dbInst::getLocation(int& x, int& y) const
{
  const _dbInst* inst = (const _dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* bbox = block->_box_tbl->getPtr(inst->_bbox);
  x = bbox->_shape._rect.xMin();
  y = bbox->_shape._rect.yMin();
}

void dbInst::setLocation(int x, int y)
{
  dbMaster* master = getMaster();
  Rect bbox;
  master->getPlacementBoundary(bbox);
  dbTransform t(getOrient());
  t.apply(bbox);
  setOrigin(x - bbox.xMin(), y - bbox.yMin());
}

dbBox* dbInst::getBBox()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  return (dbBox*) block->_box_tbl->getPtr(inst->_bbox);
}

dbOrientType dbInst::getOrient()
{
  _dbInst* inst = (_dbInst*) this;
  return dbOrientType(inst->_flags._orient);
}

void dbInst::setOrient(dbOrientType orient)
{
  if (orient == getOrient())
    return;
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  for (auto callback : block->_callbacks)
    callback->inDbPreMoveInst(this);
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._orient = orient.getValue();
  _dbInst::setInstBBox(inst);

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setOrient {}",
               orient.getValue());
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }

  block->_flags._valid_bbox = 0;
  for (auto callback : block->_callbacks)
    callback->inDbPostMoveInst(this);
}

dbPlacementStatus dbInst::getPlacementStatus()
{
  _dbInst* inst = (_dbInst*) this;
  return dbPlacementStatus(inst->_flags._status);
}

void dbInst::setPlacementStatus(dbPlacementStatus status)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._status = status.getValue();
  block->_flags._valid_bbox = 0;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setPlacementStatus {}",
               status.getValue());
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::getTransform(dbTransform& t)
{
  _dbInst* inst = (_dbInst*) this;
  t = dbTransform(inst->_flags._orient, Point(inst->_x, inst->_y));
  return;
}

void dbInst::setTransform(dbTransform& t)
{
  setOrient(t.getOrient());
  Point offset = t.getOffset();
  setOrigin(offset.x(), offset.y());
  return;
}

static void getParentTransform(dbInst* inst, dbTransform& t)
{
  dbInst* parent = inst->getParent();

  if (parent == NULL)
    t = dbTransform();
  else {
    getParentTransform(parent, t);
    dbTransform x;
    parent->getTransform(x);
    x.concat(t);
    t = x;
  }
}

void dbInst::getHierTransform(dbTransform& t)
{
  getParentTransform(this, t);
  dbTransform x;
  getTransform(x);
  x.concat(t);
  t = x;
  return;
}

int dbInst::getLevel()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_flags._inside_cone > 0)
    return inst->_flags._level;
  if (inst->_flags._input_cone > 0)
    return -inst->_flags._level;

  return 0;
}
void dbInst::setLevel(uint v, bool fromPI)
{
  _dbInst* inst = (_dbInst*) this;
  if (v > 255) {
    getImpl()->getLogger()->info(
        utl::ODB,
        36,
        "setLevel {} greater than 255 is illegal! inst {}",
        v,
        getId());
    return;
  }
  inst->_flags._level = v;
  inst->_flags._input_cone = 0;
  inst->_flags._inside_cone = 0;

  if (fromPI)
    inst->_flags._input_cone = 1;
  else
    inst->_flags._inside_cone = 1;
}
bool dbInst::getEcoCreate()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_create;
}
void dbInst::setEcoCreate(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v)
    inst->_flags._eco_create = 1;
  else
    inst->_flags._eco_create = 0;
}
bool dbInst::getEcoDestroy()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_destroy;
}
void dbInst::setEcoDestroy(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v)
    inst->_flags._eco_destroy = 1;
  else
    inst->_flags._eco_destroy = 0;
}
bool dbInst::getEcoModify()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_modify;
}
void dbInst::setEcoModify(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v)
    inst->_flags._eco_modify = 1;
  else
    inst->_flags._eco_modify = 0;
}
bool dbInst::getUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_1 == 1;
}

void dbInst::setUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_1 = 1;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

void dbInst::clearUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_1 = 0;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

bool dbInst::getUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_2 == 1;
}

void dbInst::setUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_2 = 1;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

void dbInst::clearUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_2 = 0;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

bool dbInst::getUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_3 == 1;
}

void dbInst::setUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_3 = 1;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

void dbInst::clearUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_3 = 0;

  if (block->_journal)
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
}

void dbInst::setSizeOnly(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._size_only = v;
}

bool dbInst::isSizeOnly()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._size_only == 1;
}

void dbInst::setDoNotTouch(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._dont_touch = v;
}

bool dbInst::isDoNotTouch()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._dont_touch == 1;
}

void dbInst::setDoNotSize(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._dont_size = v;
}

bool dbInst::isDoNotSize()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._dont_size == 1;
}

dbBlock* dbInst::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbMaster* dbInst::getMaster() const
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbInstHdr* inst_hdr = block->_inst_hdr_tbl->getPtr(inst->_inst_hdr);
  _dbDatabase* db = inst->getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(inst_hdr->_lib);
  return (dbMaster*) lib->_master_tbl->getPtr(inst_hdr->_master);
}

bool dbInst::isBlock() const
{
  return getMaster()->isBlock();
}

bool dbInst::isCore() const
{
  return getMaster()->isCore();
}

bool dbInst::isPad() const
{
  return getMaster()->isPad();
}

bool dbInst::isEndCap() const
{
  return getMaster()->isEndCap();
}

dbITerm* dbInst::getClockedTerm()
{
  dbMaster* m = getMaster();
  if (m->getType() != dbMasterType::CORE)
    return NULL;
  int ii = m->getClockedIndex();
  if (ii < 0)
    return NULL;
  return getITerm(ii);
}
dbITerm* dbInst::getOutputTerm()
{
  dbMaster* m = getMaster();
  int ii = m->getOutputIndex();
  if (ii < 0)
    return NULL;
  return getITerm(ii);
}

dbSet<dbITerm> dbInst::getITerms()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  return dbSet<dbITerm>(inst, block->_inst_iterm_itr);
}

dbITerm* dbInst::findITerm(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  dbMaster* master = getMaster();
  _dbMTerm* mterm = (_dbMTerm*) master->findMTerm((dbBlock*) block, name);

  if (mterm == NULL)
    return NULL;

  return (dbITerm*) block->_iterm_tbl->getPtr(inst->_iterms[mterm->_order_id]);
}

dbRegion* dbInst::getRegion()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_region == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbRegion* r = block->_region_tbl->getPtr(inst->_region);
  return (dbRegion*) r;
}

dbModule* dbInst::getModule()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_module == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbModule* module = block->_module_tbl->getPtr(inst->_module);
  return (dbModule*) module;
}

dbGroup* dbInst::getGroup()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_group == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbGroup* group = block->_group_tbl->getPtr(inst->_group);
  return (dbGroup*) group;
}

dbBox* dbInst::getHalo()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_halo == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* b = block->_box_tbl->getPtr(inst->_halo);
  return (dbBox*) b;
}

void dbInst::getConnectivity(std::vector<dbInst*>& result,
                             dbSigType::Value type)
{
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iterm_itr;

  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    dbITerm* iterm = *iterm_itr;
    dbNet* net = iterm->getNet();

    if (net == NULL)
      continue;

    if ((net != NULL) && (((_dbNet*) net)->_flags._sig_type != type))
      continue;

    dbSet<dbITerm> net_iterms = net->getITerms();
    dbSet<dbITerm>::iterator net_iterm_itr;

    for (net_iterm_itr = net_iterms.begin(); net_iterm_itr != net_iterms.end();
         ++net_iterm_itr) {
      dbITerm* net_iterm = *net_iterm_itr;
      dbInst* inst = net_iterm->getInst();

      if (inst != this)
        result.push_back(inst);
    }
  }

  // remove duplicates
  std::sort(result.begin(), result.end());
  std::vector<dbInst*>::iterator end_itr;
  end_itr = std::unique(result.begin(), result.end());
  result.erase(end_itr, result.end());
}

bool dbInst::resetHierarchy(bool verbose)
{
  _dbInst* inst = (_dbInst*) this;
  //_dbBlock * block = (_dbBlock *) block_;

  if (inst->_hierarchy) {
    if (verbose)
      getImpl()->getLogger()->info(
          utl::ODB, 37, "instance bound to a block {}", inst->_hierarchy.id());
    inst->_hierarchy = 0;
    return true;
  }
  /*
      if ( block->_parent_inst ) {
          warning(0, "block already bound to an instance %d\n",
     block->_parent_inst.id()); if (force) { warning(0, "Forced Initialize to
     0\n"); block->_parent_inst= 0; } else { return false;
          }
      }
  */
  return false;
}
bool dbInst::bindBlock(dbBlock* block_, bool force)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) block_;

  if (inst->_hierarchy) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 38,
                                 "instance already bound to a block {}",
                                 inst->_hierarchy.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 39, "Forced Initialize to 0");
      inst->_hierarchy = 0;
    } else {
      return false;
    }
  }

  if (block->_parent_inst) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 40,
                                 "block already bound to an instance {}",
                                 block->_parent_inst.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 41, "Forced Initialize to 0");
      block->_parent_inst = 0;
    } else {
      return false;
    }
  }

  if (block_->getParent() != getBlock()) {
    getImpl()->getLogger()->warn(
        utl::ODB, 42, "block not a direct child of instance owner");
    return false;
  }

  _dbHier* hier = _dbHier::create(this, block_);

  // _dbHier::create fails if bterms cannot be mapped (1-to-1) to mterms
  if (hier == NULL) {
    getImpl()->getLogger()->warn(utl::ODB, 43, "_dbHier::create fails");
    return false;
  }
  return true;
}

void dbInst::unbindBlock()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_hierarchy) {
    _dbBlock* block = (_dbBlock*) inst->getOwner();
    _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);
    _dbHier::destroy(hier);
  }
}

dbBlock* dbInst::getChild()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_hierarchy == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);
  _dbBlock* child = chip->_block_tbl->getPtr(hier->_child_block);
  return (dbBlock*) child;
}

bool dbInst::isHierarchical()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_hierarchy != 0;
}

dbInst* dbInst::getParent()
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  return block->getParentInst();
}

dbSet<dbInst> dbInst::getChildren()
{
  dbBlock* child = getChild();

  if (child == NULL)
    return dbSet<dbInst>(child, &dbNullIterator::null_iterator);

  return child->getInsts();
}

int dbInst::getWeight()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_weight;
}

void dbInst::setWeight(int weight)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_weight = weight;
}

dbSourceType dbInst::getSourceType()
{
  _dbInst* inst = (_dbInst*) this;
  dbSourceType t(inst->_flags._source);
  return t;
}

void dbInst::setSourceType(dbSourceType type)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._source = type;
}

dbITerm* dbInst::getITerm(dbMTerm* mterm_)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[mterm->_order_id]);
  return (dbITerm*) iterm;
}
dbITerm* dbInst::getITerm(uint mterm_order_id)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[mterm_order_id]);
  return (dbITerm*) iterm;
}
bool dbInst::swapMaster(dbMaster* new_master_)
{
  const char* newMasterName = new_master_->getConstName();
  const char* oldMasterName = this->getMaster()->getConstName();

  /*
  if (strcmp(this->getConstName(), "BW1_BUF440357")==0) {
      notice(0, "Trying to swapMaster FROM %s TO %s %s\n",
  new_master_->getConstName(), this->getMaster()->getConstName(),
          new_master_->getConstName(),
          this->getConstName());
  }
  */
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  dbMaster* old_master_ = getMaster();

  if (inst->_hierarchy) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 44,
                                 "Failed(_hierarchy) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  if (block->_journal) {
    debugPrint(
        getImpl()->getLogger(), utl::ODB, "DB_ECO", 1, "ECO: swapMaster");
    dbLib* old_lib = old_master_->getLib();
    dbLib* new_lib = new_master_->getLib();
    block->_journal->beginAction(dbJournal::SWAP_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(inst->getId());
    block->_journal->pushParam(old_lib->getId());
    block->_journal->pushParam(old_master_->getId());
    block->_journal->pushParam(new_lib->getId());
    block->_journal->pushParam(new_master_->getId());
    block->_journal->endAction();
  }

  // Notification - payam 01/18/2006
  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr)
    (**cbitr)().inDbInstSwapMasterBefore(
        this, new_master_);  // client ECO initialization - payam

  //
  // Ensure the mterms are equivalent
  //
  dbSet<dbMTerm>::iterator itr;
  dbSet<dbMTerm> mterms = new_master_->getMTerms();
  std::vector<_dbMTerm*> new_terms;

  for (itr = mterms.begin(); itr != mterms.end(); ++itr)
    new_terms.push_back((_dbMTerm*) *itr);

  mterms = old_master_->getMTerms();
  std::vector<_dbMTerm*> old_terms;

  for (itr = mterms.begin(); itr != mterms.end(); ++itr)
    old_terms.push_back((_dbMTerm*) *itr);

  if (old_terms.size() != new_terms.size()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 45,
                                 "Failed(termSize) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  std::vector<uint> idx_map(old_terms.size());
  std::sort(new_terms.begin(), new_terms.end(), sortMTerm());
  std::sort(old_terms.begin(), old_terms.end(), sortMTerm());
  std::vector<_dbMTerm*>::iterator i1 = new_terms.begin();
  std::vector<_dbMTerm*>::iterator i2 = old_terms.begin();

  for (; i1 != new_terms.end() && i2 != old_terms.end(); ++i1, ++i2) {
    _dbMTerm* t1 = *i1;
    _dbMTerm* t2 = *i2;

    if (strcmp(t1->_name, t2->_name) != 0)
      break;

    idx_map[t2->_order_id] = t1->_order_id;
  }

  // mterms are not equivalent
  if (i1 != new_terms.end() || i2 != old_terms.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 46,
                                 "Failed(mtermEquiv) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  // remove reference to inst_hdr
  _dbInstHdr* old_inst_hdr
      = block->_inst_hdr_hash.find(((_dbMaster*) old_master_)->_id);
  old_inst_hdr->_inst_cnt--;

  // delete the old-inst-hdr if required
  if (old_inst_hdr->_inst_cnt == 0)
    dbInstHdr::destroy((dbInstHdr*) old_inst_hdr);

  // add reference to new inst_hdr
  _dbInstHdr* new_inst_hdr
      = block->_inst_hdr_hash.find(((_dbMaster*) new_master_)->_id);

  // create a new inst-hdr if needed
  if (new_inst_hdr == NULL) {
    new_inst_hdr = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block,
                                                   (dbMaster*) new_master_);
    ZASSERT(new_inst_hdr);
  }

  new_inst_hdr->_inst_cnt++;
  inst->_inst_hdr = new_inst_hdr->getOID();

  // The next two steps invalidates any dbSet<dbITerm> iterators.

  // 1) update the iterm-mterm-idx
  uint cnt = inst->_iterms.size();

  uint i;
  for (i = 0; i < cnt; ++i) {
    _dbITerm* it = block->_iterm_tbl->getPtr(inst->_iterms[i]);
    uint old_idx = it->_flags._mterm_idx;
    it->_flags._mterm_idx = idx_map[old_idx];
  }

  // 2) reorder the iterms vector
  sortITerm itermCmp(block);
  std::sort(inst->_iterms.begin(), inst->_iterms.end(), itermCmp);

  // Notification - payam 01/18/2006
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr)
    (*cbitr)->inDbInstSwapMasterAfter(this);

  // notice(0, "dbSwap_END %s -> %s %s\n", old_master_->getConstName(),
  // this->getMaster()->getConstName(), this->getConstName());

  return true;
}

dbInst* dbInst::create(dbBlock* block_, dbMaster* master_, const char* name_)
{
  return create(block_, master_, name_, NULL);
}

dbInst* dbInst::create(dbBlock* block_,
                       dbMaster* master_,
                       const char* name_,
                       dbRegion* region)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbMaster* master = (_dbMaster*) master_;
  _dbInstHdr* inst_hdr = block->_inst_hdr_hash.find(master->_id);
  if (inst_hdr == NULL) {
    inst_hdr
        = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block, (dbMaster*) master);
    ZASSERT(inst_hdr);
  }

  if (block->_inst_hash.hasMember(name_))
    return NULL;

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbInst:create");
    dbLib* lib = master_->getLib();
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(lib->getId());
    block->_journal->pushParam(master_->getId());
    block->_journal->pushParam(name_);
    block->_journal->endAction();
  }

  _dbInst* inst = block->_inst_tbl->create();
  inst->_name = strdup(name_);
  ZALLOCATED(inst->_name);
  inst->_inst_hdr = inst_hdr->getOID();
  block->_inst_hash.insert(inst);
  inst_hdr->_inst_cnt++;

  // create the iterms
  uint mterm_cnt = inst_hdr->_mterms.size();
  inst->_iterms.resize(mterm_cnt);

  uint i;
  for (i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->_iterm_tbl->create();
    inst->_iterms[i] = iterm->getOID();
    iterm->_flags._mterm_idx = i;
    iterm->_inst = inst->getOID();
  }

  _dbBox* box = block->_box_tbl->create();
  box->_shape._rect.init(0, 0, master->_width, master->_height);
  box->_flags._owner_type = dbBoxOwner::INST;
  box->_owner = inst->getOID();
  inst->_bbox = box->getOID();

  block->add_rect(box->_shape._rect);

  if (region) {
    region->addInst((dbInst*) inst);
    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr)
      (**cbitr)().inDbInstCreate((dbInst*) inst,
                                 region);  // client ECO initialization - payam
  } else {
    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr)
      (**cbitr)().inDbInstCreate(
          (dbInst*) inst);  // client ECO initialization - payam
  }

  for (i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[i]);
    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr)
      (**cbitr)().inDbITermCreate(
          (dbITerm*) iterm);  // client ECO initialization - payam
  }

  return (dbInst*) inst;
}

void dbInst::destroy(dbInst* inst_)
{
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  dbRegion* region = inst_->getRegion();

  if (region)
    region->removeInst(inst_);

  dbModule* module = inst_->getModule();
  if (module)
    module->removeInst(inst_);

  if (inst->_group)
    inst_->getGroup()->removeInst(inst_);

  uint i;
  uint n = inst->_iterms.size();

  for (i = 0; i < n; ++i) {
    dbId<_dbITerm> id = inst->_iterms[i];
    _dbITerm* it = block->_iterm_tbl->getPtr(id);
    dbITerm::disconnect((dbITerm*) it);

    // Bugzilla #7: notify when pins are deleted (assumption: pins
    // are destroyed only when the related instance is destroyed)
    // payam 01/10/2006
    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr)
      (**cbitr)().inDbITermDestroy(
          (dbITerm*) it);  // client ECO optimization - payam

    dbProperty::destroyProperties(it);
    block->_iterm_tbl->destroy(it);
  }

  //    Move this part after inDbInstDestroy
  //    ----------------------------------------
  //    _dbMaster * master = (_dbMaster *) inst_->getMaster();
  //    _dbInstHdr * inst_hdr = block->_inst_hdr_hash.find(master->_id);
  //    inst_hdr->_inst_cnt--;
  //
  //    if ( inst_hdr->_inst_cnt == 0 )
  //        dbInstHdr::destroy( (dbInstHdr *) inst_hdr );

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbInst:destroy");
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(inst->getId());
    block->_journal->endAction();
  }

  // Bugzilla #7: The notification of the the instance destruction must
  // be done after pin manipulation is completed. The notification is
  // now after the pin disconnection - payam 01/10/2006
  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr)
    (**cbitr)().inDbInstDestroy(inst_);  // client ECO optimization - payam

  _dbMaster* master = (_dbMaster*) inst_->getMaster();
  _dbInstHdr* inst_hdr = block->_inst_hdr_hash.find(master->_id);
  inst_hdr->_inst_cnt--;

  if (inst_hdr->_inst_cnt == 0)
    dbInstHdr::destroy((dbInstHdr*) inst_hdr);

  if (inst->_halo) {
    _dbBox* halo = block->_box_tbl->getPtr(inst->_halo);
    dbProperty::destroyProperties(halo);
    block->_box_tbl->destroy(halo);
  }

  _dbBox* box = block->_box_tbl->getPtr(inst->_bbox);
  block->remove_rect(box->_shape._rect);
  block->_inst_hash.remove(inst);
  dbProperty::destroyProperties(inst);
  block->_inst_tbl->destroy(inst);
  dbProperty::destroyProperties(box);
  block->_box_tbl->destroy(box);
}

dbSet<dbInst>::iterator dbInst::destroy(dbSet<dbInst>::iterator& itr)
{
  dbInst* bt = *itr;
  dbSet<dbInst>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbInst* dbInst::getInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbInst*) block->_inst_tbl->getPtr(dbid_);
}

dbInst* dbInst::getValidInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->_inst_tbl->validId(dbid_))
    return NULL;
  return (dbInst*) block->_inst_tbl->getPtr(dbid_);
}
dbITerm* dbInst::getFirstOutput()
{
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* tr = *iitr;

    if ((tr->getSigType() == dbSigType::GROUND)
        || (tr->getSigType() == dbSigType::POWER))
      continue;

    if (tr->getIoType() != dbIoType::OUTPUT)
      continue;

    return tr;
  }
  getImpl()->getLogger()->warn(
      utl::ODB, 47, "instance {} has no output pin", getConstName());
  return NULL;
}

}  // namespace odb
