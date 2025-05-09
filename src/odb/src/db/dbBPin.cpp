// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBPin.h"

#include <iostream>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

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
      _effective_width(p._effective_width),
      aps_(p.aps_)
{
}

bool _dbBPin::operator==(const _dbBPin& rhs) const
{
  if (_flags._status != rhs._flags._status) {
    return false;
  }

  if (_flags._has_min_spacing != rhs._flags._has_min_spacing) {
    return false;
  }

  if (_flags._has_effective_width != rhs._flags._has_effective_width) {
    return false;
  }

  if (_bterm != rhs._bterm) {
    return false;
  }

  if (_boxes != rhs._boxes) {
    return false;
  }

  if (_next_bpin != rhs._next_bpin) {
    return false;
  }

  if (_min_spacing != rhs._min_spacing) {
    return false;
  }

  if (_effective_width != rhs._effective_width) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
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
  stream << bpin.aps_;

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
  stream >> bpin.aps_;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbBPin - Methods
//
////////////////////////////////////////////////////////////////////

dbBTerm* dbBPin::getBTerm() const
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
    Rect rect = box->getBox();
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

std::vector<dbAccessPoint*> dbBPin::getAccessPoints() const
{
  _dbBPin* bpin = (_dbBPin*) this;
  _dbBlock* block = (_dbBlock*) getBTerm()->getBlock();
  std::vector<dbAccessPoint*> aps;
  for (const auto& ap : bpin->aps_) {
    aps.push_back((dbAccessPoint*) block->ap_tbl_->getPtr(ap));
  }
  return aps;
}

dbBPin* dbBPin::create(dbBTerm* bterm_)
{
  _dbBTerm* bterm = (_dbBTerm*) bterm_;
  _dbBlock* block = (_dbBlock*) bterm->getOwner();
  _dbBPin* bpin = block->_bpin_tbl->create();
  bpin->_bterm = bterm->getOID();
  bpin->_next_bpin = bterm->_bpins;
  bterm->_bpins = bpin->getOID();
  for (auto callback : block->_callbacks) {
    callback->inDbBPinCreate((dbBPin*) bpin);
  }
  return (dbBPin*) bpin;
}

void dbBPin::destroy(dbBPin* bpin_)
{
  _dbBPin* bpin = (_dbBPin*) bpin_;
  _dbBlock* block = (_dbBlock*) bpin->getOwner();
  _dbBTerm* bterm = (_dbBTerm*) bpin_->getBTerm();
  for (auto callback : block->_callbacks) {
    callback->inDbBPinDestroy(bpin_);
  }
  // unlink bpin from bterm
  uint id = bpin->getOID();
  _dbBPin* prev = nullptr;
  uint cur = bterm->_bpins;
  while (cur) {
    _dbBPin* c = block->_bpin_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        bterm->_bpins = bpin->_next_bpin;
      } else {
        prev->_next_bpin = bpin->_next_bpin;
      }
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

  for (auto ap : bpin_->getAccessPoints()) {
    odb::dbAccessPoint::destroy(ap);
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

void _dbBPin::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["ap"].add(aps_);
}

}  // namespace odb
