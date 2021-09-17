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

#include "dbObstruction.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbBox.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbInst.h"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"

namespace odb {

template class dbTable<_dbObstruction>;

_dbObstruction::_dbObstruction(_dbDatabase*, const _dbObstruction& o)
    : _flags(o._flags),
      _inst(o._inst),
      _bbox(o._bbox),
      _min_spacing(o._min_spacing),
      _effective_width(o._effective_width)
{
}

_dbObstruction::_dbObstruction(_dbDatabase*)
{
  _flags._slot_obs = 0;
  _flags._fill_obs = 0;
  _flags._pushed_down = 0;
  _flags._has_min_spacing = 0;
  _flags._has_effective_width = 0;
  _flags._spare_bits = 0;
  _min_spacing = 0;
  _effective_width = 0;
}

_dbObstruction::~_dbObstruction()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbObstruction& obs)
{
  uint* bit_field = (uint*) &obs._flags;
  stream << *bit_field;
  stream << obs._inst;
  stream << obs._bbox;
  stream << obs._min_spacing;
  stream << obs._effective_width;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbObstruction& obs)
{
  uint* bit_field = (uint*) &obs._flags;
  stream >> *bit_field;
  stream >> obs._inst;
  stream >> obs._bbox;
  stream >> obs._min_spacing;
  stream >> obs._effective_width;

  return stream;
}

bool _dbObstruction::operator==(const _dbObstruction& rhs) const
{
  if (_flags._slot_obs != rhs._flags._slot_obs)
    return false;

  if (_flags._fill_obs != rhs._flags._fill_obs)
    return false;

  if (_flags._pushed_down != rhs._flags._pushed_down)
    return false;

  if (_flags._has_min_spacing != rhs._flags._has_min_spacing)
    return false;

  if (_flags._has_effective_width != rhs._flags._has_effective_width)
    return false;

  if (_inst != rhs._inst)
    return false;

  if (_bbox != rhs._bbox)
    return false;

  return true;
}

void _dbObstruction::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbObstruction& rhs) const
{
  _dbBlock* lhs_blk = (_dbBlock*) getOwner();
  _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();

  DIFF_BEGIN
  DIFF_FIELD(_flags._slot_obs);
  DIFF_FIELD(_flags._fill_obs);
  DIFF_FIELD(_flags._pushed_down);
  DIFF_FIELD(_flags._has_min_spacing);
  DIFF_FIELD(_flags._has_effective_width);
  DIFF_FIELD(_min_spacing);
  DIFF_FIELD(_effective_width);

  if (!diff.deepDiff()) {
    DIFF_FIELD(_inst);
  } else {
    if (_inst && rhs._inst) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
      _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
      diff.diff("_inst", lhs_inst->_name, rhs_inst->_name);
    } else if (_inst) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
      diff.out(dbDiff::LEFT, "_inst", lhs_inst->_name);
    } else if (rhs._inst) {
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
      diff.out(dbDiff::RIGHT, "_inst", rhs_inst->_name);
    }
  }

  DIFF_OBJECT(_bbox, lhs_blk->_box_tbl, rhs_blk->_box_tbl);
  DIFF_END
}

void _dbObstruction::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* blk = (_dbBlock*) getOwner();

  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._slot_obs);
  DIFF_OUT_FIELD(_flags._fill_obs);
  DIFF_OUT_FIELD(_flags._pushed_down);
  DIFF_OUT_FIELD(_flags._has_min_spacing);
  DIFF_OUT_FIELD(_flags._has_effective_width);
  DIFF_OUT_FIELD(_min_spacing);
  DIFF_OUT_FIELD(_effective_width);

  if (!diff.deepDiff()) {
    DIFF_OUT_FIELD(_inst);
  } else {
    if (_inst) {
      _dbBlock* blk = (_dbBlock*) getOwner();
      _dbInst* inst = blk->_inst_tbl->getPtr(_inst);
      diff.out(side, "_inst", inst->_name);
    } else {
      diff.out(side, "_inst", "(NULL)");
    }
  }

  DIFF_OUT_OBJECT(_bbox, blk->_box_tbl);
  DIFF_END
}

bool _dbObstruction::operator<(const _dbObstruction& rhs) const
{
  _dbBlock* lhs_block = (_dbBlock*) getOwner();
  _dbBlock* rhs_block = (_dbBlock*) rhs.getOwner();
  _dbBox* lhs_box = lhs_block->_box_tbl->getPtr(_bbox);
  _dbBox* rhs_box = rhs_block->_box_tbl->getPtr(rhs._bbox);

  if (*lhs_box < *rhs_box)
    return true;

  if (lhs_box->equal(*rhs_box)) {
    if (_inst && rhs._inst) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
      _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
      int r = strcmp(lhs_inst->_name, rhs_inst->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
    } else if (_inst) {
      return false;
    } else if (rhs._inst) {
      return true;
    }

    if (_flags._slot_obs < rhs._flags._slot_obs)
      return true;

    if (_flags._slot_obs > rhs._flags._slot_obs)
      return false;

    if (_flags._fill_obs < rhs._flags._fill_obs)
      return true;

    if (_flags._fill_obs > rhs._flags._fill_obs)
      return false;

    if (_flags._pushed_down < rhs._flags._pushed_down)
      return true;

    if (_flags._pushed_down > rhs._flags._pushed_down)
      return false;

    if (_flags._has_min_spacing < rhs._flags._has_min_spacing)
      return true;

    if (_flags._has_min_spacing > rhs._flags._has_min_spacing)
      return false;

    if (_flags._has_effective_width < rhs._flags._has_effective_width)
      return true;

    if (_flags._has_effective_width > rhs._flags._has_effective_width)
      return false;

    if (_min_spacing < rhs._min_spacing)
      return true;

    if (_min_spacing > rhs._min_spacing)
      return false;

    if (_effective_width < rhs._effective_width)
      return true;

    if (_effective_width > rhs._effective_width)
      return false;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//
// dbObstruction - Methods
//
////////////////////////////////////////////////////////////////////

dbBox* dbObstruction::getBBox()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  _dbBlock* block = (_dbBlock*) obs->getOwner();
  return (dbBox*) block->_box_tbl->getPtr(obs->_bbox);
}

dbInst* dbObstruction::getInstance()
{
  _dbObstruction* obs = (_dbObstruction*) this;

  if (obs->_inst == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) obs->getOwner();
  return (dbInst*) block->_inst_tbl->getPtr(obs->_inst);
}

void dbObstruction::setSlotObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->_flags._slot_obs = 1;
}

bool dbObstruction::isSlotObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_flags._slot_obs == 1;
}

void dbObstruction::setFillObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->_flags._fill_obs = 1;
}

bool dbObstruction::isFillObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_flags._fill_obs == 1;
}

void dbObstruction::setPushedDown()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->_flags._pushed_down = 1;
}

bool dbObstruction::isPushedDown()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_flags._pushed_down == 1;
}

bool dbObstruction::hasEffectiveWidth()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_flags._has_effective_width == 1U;
}

void dbObstruction::setEffectiveWidth(int w)
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->_flags._has_effective_width = 1U;
  obs->_effective_width = w;
}

int dbObstruction::getEffectiveWidth()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_effective_width;
}

bool dbObstruction::hasMinSpacing()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_flags._has_min_spacing == 1U;
}

void dbObstruction::setMinSpacing(int w)
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->_flags._has_min_spacing = 1U;
  obs->_min_spacing = w;
}

int dbObstruction::getMinSpacing()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->_min_spacing;
}

dbBlock* dbObstruction::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbObstruction* dbObstruction::create(dbBlock* block_,
                                     dbTechLayer* layer_,
                                     int x1,
                                     int y1,
                                     int x2,
                                     int y2,
                                     dbInst* inst_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbTechLayer* layer = (_dbTechLayer*) layer_;
  _dbInst* inst = (_dbInst*) inst_;

  _dbObstruction* obs = block->_obstruction_tbl->create();

  if (inst)
    obs->_inst = inst->getOID();

  _dbBox* box = block->_box_tbl->create();
  box->_shape._rect.init(x1, y1, x2, y2);
  box->_flags._owner_type = dbBoxOwner::OBSTRUCTION;
  box->_owner = obs->getOID();
  box->_flags._layer_id = layer->getOID();
  obs->_bbox = box->getOID();

  // Update bounding box of block
  block->add_rect(box->_shape._rect);
  for (auto callback : block->_callbacks)
    callback->inDbObstructionCreate((dbObstruction*) obs);
  return (dbObstruction*) obs;
}

void dbObstruction::destroy(dbObstruction* obstruction)
{
  _dbObstruction* obs = (_dbObstruction*) obstruction;
  _dbBlock* block = (_dbBlock*) obs->getOwner();
  for (auto callback : block->_callbacks)
    callback->inDbObstructionDestroy(obstruction);
  dbProperty::destroyProperties(obs);
  block->_obstruction_tbl->destroy(obs);
}

dbObstruction* dbObstruction::getObstruction(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbObstruction*) block->_obstruction_tbl->getPtr(dbid_);
}

}  // namespace odb
