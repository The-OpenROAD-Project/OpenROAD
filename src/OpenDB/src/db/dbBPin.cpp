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

#include "dbBPin.h"

#include <iostream>

#include "db.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
namespace odb {

template class dbTable<_dbBPin>;

_dbBPin::_dbBPin(_dbDatabase*)
{
  _flags._status = dbPlacementStatus::NONE;
  _flags._has_min_spacing = 0;
  _flags._has_effective_width = 0;
  _flags._spare_bits = 0;
  _min_spacing = 0;
  _effective_width = 0;
}

_dbBPin::_dbBPin(_dbDatabase*, const _dbBPin& p)
    : _flags(p._flags),
      _bterm(p._bterm),
      _boxes(p._boxes),
      _next_bpin(p._next_bpin),
      _min_spacing(p._min_spacing),
      _effective_width(p._effective_width)
{
}

_dbBPin::~_dbBPin()
{
}

bool _dbBPin::operator==(const _dbBPin& rhs) const
{
  if (_flags._status != rhs._flags._status)
    return false;

  if (_flags._has_min_spacing != rhs._flags._has_min_spacing)
    return false;

  if (_flags._has_effective_width != rhs._flags._has_effective_width)
    return false;

  if (_bterm != rhs._bterm)
    return false;

  if (_boxes != rhs._boxes)
    return false;

  if (_next_bpin != rhs._next_bpin)
    return false;

  if (_min_spacing != rhs._min_spacing)
    return false;

  if (_effective_width != rhs._effective_width)
    return false;

  return true;
}

void _dbBPin::differences(dbDiff& diff,
                          const char* field,
                          const _dbBPin& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._status);
  DIFF_FIELD(_flags._has_min_spacing);
  DIFF_FIELD(_flags._has_effective_width);
  DIFF_FIELD(_bterm);
  DIFF_FIELD(_boxes);
  DIFF_FIELD(_next_bpin);
  DIFF_FIELD(_min_spacing);
  DIFF_FIELD(_effective_width);
  DIFF_END
}

void _dbBPin::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._status);
  DIFF_OUT_FIELD(_flags._has_min_spacing);
  DIFF_OUT_FIELD(_flags._has_effective_width);
  DIFF_OUT_FIELD(_bterm);
  DIFF_OUT_FIELD(_boxes);
  DIFF_OUT_FIELD(_next_bpin);
  DIFF_OUT_FIELD(_min_spacing);
  DIFF_OUT_FIELD(_effective_width);
  DIFF_END
}

dbOStream& operator<<(dbOStream& stream, const _dbBPin& bpin)
{
  uint* bit_field = (uint*) &bpin._flags;
  stream << *bit_field;
  stream << bpin._bterm;
  stream << bpin._boxes;
  stream << bpin._next_bpin;
  stream << bpin._min_spacing;
  stream << bpin._effective_width;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBPin& bpin)
{
  uint* bit_field = (uint*) &bpin._flags;
  stream >> *bit_field;
  stream >> bpin._bterm;
  stream >> bpin._boxes;
  stream >> bpin._next_bpin;
  stream >> bpin._min_spacing;
  stream >> bpin._effective_width;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbBPin - Methods
//
////////////////////////////////////////////////////////////////////

dbBTerm* dbBPin::getBTerm()
{
  _dbBPin* pin = (_dbBPin*) this;
  _dbBlock* block = (_dbBlock*) pin->getOwner();
  return (dbBTerm*) block->_bterm_tbl->getPtr(pin->_bterm);
}

dbSet<dbBox> dbBPin::getBoxes()
{
  _dbBPin* pin = (_dbBPin*) this;

  _dbBlock* block = (_dbBlock*) pin->getOwner();
  return dbSet<dbBox>(pin, block->_box_itr);
}

Rect dbBPin::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (dbBox* box : getBoxes()) {
    Rect rect;
    box->getBox(rect);
    bbox.merge(rect);
  }
  return bbox;
}

dbPlacementStatus dbBPin::getPlacementStatus()
{
  _dbBPin* bpin = (_dbBPin*) this;
  return dbPlacementStatus(bpin->_flags._status);
}

void dbBPin::setPlacementStatus(dbPlacementStatus status)
{
  _dbBPin* bpin = (_dbBPin*) this;
  bpin->_flags._status = status.getValue();
  _dbBlock* block = (_dbBlock*) bpin->getOwner();
  block->_flags._valid_bbox = 0;
}

bool dbBPin::hasEffectiveWidth()
{
  _dbBPin* bpin = (_dbBPin*) this;
  return bpin->_flags._has_effective_width == 1U;
}

void dbBPin::setEffectiveWidth(int w)
{
  _dbBPin* bpin = (_dbBPin*) this;
  bpin->_flags._has_effective_width = 1U;
  bpin->_effective_width = w;
}

int dbBPin::getEffectiveWidth()
{
  _dbBPin* bpin = (_dbBPin*) this;
  return bpin->_effective_width;
}

bool dbBPin::hasMinSpacing()
{
  _dbBPin* bpin = (_dbBPin*) this;
  return bpin->_flags._has_min_spacing == 1U;
}

void dbBPin::setMinSpacing(int w)
{
  _dbBPin* bpin = (_dbBPin*) this;
  bpin->_flags._has_min_spacing = 1U;
  bpin->_min_spacing = w;
}

int dbBPin::getMinSpacing()
{
  _dbBPin* bpin = (_dbBPin*) this;
  return bpin->_min_spacing;
}

dbBPin* dbBPin::create(dbBTerm* bterm_)
{
  _dbBTerm* bterm = (_dbBTerm*) bterm_;
  _dbBlock* block = (_dbBlock*) bterm->getOwner();
  _dbBPin* bpin = block->_bpin_tbl->create();
  bpin->_bterm = bterm->getOID();
  bpin->_next_bpin = bterm->_bpins;
  bterm->_bpins = bpin->getOID();
  for (auto callback : block->_callbacks)
    callback->inDbBPinCreate((dbBPin*) bpin);
  return (dbBPin*) bpin;
}

void dbBPin::destroy(dbBPin* bpin_)
{
  _dbBPin* bpin = (_dbBPin*) bpin_;
  _dbBlock* block = (_dbBlock*) bpin->getOwner();
  _dbBTerm* bterm = (_dbBTerm*) bpin_->getBTerm();
  for (auto callback : block->_callbacks)
    callback->inDbBPinDestroy(bpin_);
  // unlink bpin from bterm
  uint id = bpin->getOID();
  _dbBPin* prev = NULL;
  uint cur = bterm->_bpins;
  while (cur) {
    _dbBPin* c = block->_bpin_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        bterm->_bpins = bpin->_next_bpin;
      else
        prev->_next_bpin = bpin->_next_bpin;
      break;
    }
    prev = c;
    cur = c->_next_bpin;
  }

  dbId<_dbBox> nextBox = bpin->_boxes;
  while (nextBox) {
    _dbBox* b = block->_box_tbl->getPtr(nextBox);
    nextBox = b->_next_box;
    dbProperty::destroyProperties(b);
    block->remove_rect(b->_shape._rect);
    block->_box_tbl->destroy(b);
  }

  dbProperty::destroyProperties(bpin);
  block->_bpin_tbl->destroy(bpin);
}

dbSet<dbBPin>::iterator dbBPin::destroy(dbSet<dbBPin>::iterator& itr)
{
  dbBPin* bt = *itr;
  dbSet<dbBPin>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbBPin* dbBPin::getBPin(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBPin*) block->_bpin_tbl->getPtr(dbid_);
}

}  // namespace odb
