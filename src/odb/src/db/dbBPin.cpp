// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBPin.h"

#include <cstdint>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbBPin>;

_dbBPin::_dbBPin(_dbDatabase*)
{
  flags_.status = dbPlacementStatus::NONE;
  flags_.has_min_spacing = 0;
  flags_.has_effective_width = 0;
  flags_.spare_bits = 0;
  min_spacing_ = 0;
  effective_width_ = 0;
}

_dbBPin::_dbBPin(_dbDatabase*, const _dbBPin& p)
    : flags_(p.flags_),
      bterm_(p.bterm_),
      boxes_(p.boxes_),
      next_bpin_(p.next_bpin_),
      min_spacing_(p.min_spacing_),
      effective_width_(p.effective_width_),
      aps_(p.aps_)
{
}

bool _dbBPin::operator==(const _dbBPin& rhs) const
{
  if (flags_.status != rhs.flags_.status) {
    return false;
  }

  if (flags_.has_min_spacing != rhs.flags_.has_min_spacing) {
    return false;
  }

  if (flags_.has_effective_width != rhs.flags_.has_effective_width) {
    return false;
  }

  if (bterm_ != rhs.bterm_) {
    return false;
  }

  if (boxes_ != rhs.boxes_) {
    return false;
  }

  if (next_bpin_ != rhs.next_bpin_) {
    return false;
  }

  if (min_spacing_ != rhs.min_spacing_) {
    return false;
  }

  if (effective_width_ != rhs.effective_width_) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
}

dbOStream& operator<<(dbOStream& stream, const _dbBPin& bpin)
{
  uint32_t* bit_field = (uint32_t*) &bpin.flags_;
  stream << *bit_field;
  stream << bpin.bterm_;
  stream << bpin.boxes_;
  stream << bpin.next_bpin_;
  stream << bpin.min_spacing_;
  stream << bpin.effective_width_;
  stream << bpin.aps_;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBPin& bpin)
{
  uint32_t* bit_field = (uint32_t*) &bpin.flags_;
  stream >> *bit_field;
  stream >> bpin.bterm_;
  stream >> bpin.boxes_;
  stream >> bpin.next_bpin_;
  stream >> bpin.min_spacing_;
  stream >> bpin.effective_width_;
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
  const _dbBPin* pin = (const _dbBPin*) this;
  _dbBlock* block = (_dbBlock*) pin->getOwner();
  return (dbBTerm*) block->bterm_tbl_->getPtr(pin->bterm_);
}

dbSet<dbBox> dbBPin::getBoxes()
{
  _dbBPin* pin = (_dbBPin*) this;
  _dbBlock* block = (_dbBlock*) pin->getOwner();
  return dbSet<dbBox>(pin, block->box_itr_);
}

Rect dbBPin::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (const dbBox* box : getBoxes()) {
    const Rect rect = box->getBox();
    bbox.merge(rect);
  }
  return bbox;
}

dbPlacementStatus dbBPin::getPlacementStatus() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
  return dbPlacementStatus(bpin->flags_.status);
}

void dbBPin::setPlacementStatus(dbPlacementStatus status)
{
  _dbBPin* bpin = (_dbBPin*) this;

  if (bpin->flags_.status == status) {
    return;
  }

  _dbBlock* block = (_dbBlock*) bpin->getOwner();

  for (auto callback : block->callbacks_) {
    callback->inDbBPinPlacementStatusBefore(this, status);
  }

  bpin->flags_.status = status.getValue();
  block->flags_.valid_bbox = 0;
}

bool dbBPin::hasEffectiveWidth() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
  return bpin->flags_.has_effective_width == 1U;
}

void dbBPin::setEffectiveWidth(int w)
{
  _dbBPin* bpin = (_dbBPin*) this;
  bpin->flags_.has_effective_width = 1U;
  bpin->effective_width_ = w;
}

int dbBPin::getEffectiveWidth() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
  return bpin->effective_width_;
}

bool dbBPin::hasMinSpacing() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
  return bpin->flags_.has_min_spacing == 1U;
}

void dbBPin::setMinSpacing(int w)
{
  _dbBPin* bpin = (_dbBPin*) this;
  bpin->flags_.has_min_spacing = 1U;
  bpin->min_spacing_ = w;
}

int dbBPin::getMinSpacing() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
  return bpin->min_spacing_;
}

std::vector<dbAccessPoint*> dbBPin::getAccessPoints() const
{
  const _dbBPin* bpin = (const _dbBPin*) this;
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
  _dbBPin* bpin = block->bpin_tbl_->create();
  bpin->bterm_ = bterm->getOID();
  bpin->next_bpin_ = bterm->bpins_;
  bterm->bpins_ = bpin->getOID();
  for (auto callback : block->callbacks_) {
    callback->inDbBPinCreate((dbBPin*) bpin);
  }
  return (dbBPin*) bpin;
}

void dbBPin::destroy(dbBPin* bpin_)
{
  _dbBPin* bpin = (_dbBPin*) bpin_;
  _dbBlock* block = (_dbBlock*) bpin->getOwner();
  _dbBTerm* bterm = (_dbBTerm*) bpin_->getBTerm();
  for (auto callback : block->callbacks_) {
    callback->inDbBPinDestroy(bpin_);
  }
  // unlink bpin from bterm
  uint32_t id = bpin->getOID();
  _dbBPin* prev = nullptr;
  uint32_t cur = bterm->bpins_;
  while (cur) {
    _dbBPin* c = block->bpin_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        bterm->bpins_ = bpin->next_bpin_;
      } else {
        prev->next_bpin_ = bpin->next_bpin_;
      }
      break;
    }
    prev = c;
    cur = c->next_bpin_;
  }

  dbId<_dbBox> nextBox = bpin->boxes_;
  while (nextBox) {
    _dbBox* b = block->box_tbl_->getPtr(nextBox);
    nextBox = b->next_box_;
    dbProperty::destroyProperties(b);
    block->remove_rect(b->shape_.rect);
    block->box_tbl_->destroy(b);
  }

  for (auto ap : bpin_->getAccessPoints()) {
    odb::dbAccessPoint::destroy(ap);
  }

  dbProperty::destroyProperties(bpin);
  block->bpin_tbl_->destroy(bpin);
}

dbSet<dbBPin>::iterator dbBPin::destroy(dbSet<dbBPin>::iterator& itr)
{
  dbBPin* bt = *itr;
  dbSet<dbBPin>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbBPin* dbBPin::getBPin(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBPin*) block->bpin_tbl_->getPtr(dbid_);
}

void _dbBPin::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["ap"].add(aps_);
}

void _dbBPin::removeBox(_dbBox* box)
{
  _dbBlock* block = (_dbBlock*) getOwner();

  for (auto callback : block->callbacks_) {
    callback->inDbBPinRemoveBox((dbBox*) box);
  }

  dbId<_dbBox> boxid = box->getOID();
  if (boxid == boxes_) {
    // at head of list, need to move head
    boxes_ = box->next_box_;
  } else {
    // in the middle of the list, need to iterate and relink
    dbId<_dbBox> id = boxes_;
    if (id == 0) {
      return;
    }
    while (id != 0) {
      _dbBox* nbox = block->box_tbl_->getPtr(id);
      dbId<_dbBox> nid = nbox->next_box_;

      if (nid == boxid) {
        nbox->next_box_ = box->next_box_;
        break;
      }

      id = nid;
    }
  }
}

}  // namespace odb
