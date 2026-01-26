// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSWire.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbNet.h"
#include "dbSBox.h"
#include "dbSBoxItr.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {

template class dbTable<_dbSWire>;

bool _dbSWire::operator==(const _dbSWire& rhs) const
{
  if (flags_.wire_type != rhs.flags_.wire_type) {
    return false;
  }

  if (net_ != rhs.net_) {
    return false;
  }

  if (shield_ != rhs.shield_) {
    return false;
  }

  if (wires_ != rhs.wires_) {
    return false;
  }

  if (next_swire_ != rhs.next_swire_) {
    return false;
  }

  return true;
}

bool _dbSWire::operator<(const _dbSWire& rhs) const
{
  if (flags_.wire_type < rhs.flags_.wire_type) {
    return true;
  }

  if (flags_.wire_type > rhs.flags_.wire_type) {
    return false;
  }

  if ((shield_ != 0) && (rhs.shield_ != 0)) {
    _dbBlock* lhs_blk = (_dbBlock*) getOwner();
    _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
    _dbNet* lhs_net = lhs_blk->net_tbl_->getPtr(net_);
    _dbNet* rhs_net = rhs_blk->net_tbl_->getPtr(rhs.net_);
    int r = strcmp(lhs_net->name_, rhs_net->name_);

    if (r < 0) {
      return true;
    }

    if (r > 0) {
      return false;
    }
  } else if (shield_ != 0) {
    return false;
  } else if (rhs.shield_ != 0) {
    return true;
  }

  return false;
}

void _dbSWire::addSBox(_dbSBox* box)
{
  box->owner_ = getOID();
  box->next_box_ = (uint32_t) wires_;
  wires_ = box->getOID();
  _dbBlock* block = (_dbBlock*) getOwner();
  for (auto callback : block->callbacks_) {
    callback->inDbSWireAddSBox((dbSBox*) box);
  }
}

void _dbSWire::removeSBox(_dbSBox* box)
{
  _dbBlock* block = (_dbBlock*) getOwner();
  uint32_t boxid = box->getOID();
  if (boxid == wires_) {
    // at head of list, need to move head
    wires_ = (uint32_t) box->next_box_;
  } else {
    // in the middle of the list, need to iterate and relink
    dbId<_dbSBox> id = wires_;
    if (id == 0) {
      return;
    }
    while (id != 0) {
      _dbSBox* nbox = block->sbox_tbl_->getPtr(id);
      uint32_t nid = nbox->next_box_;

      if (nid == boxid) {
        nbox->next_box_ = box->next_box_;
        break;
      }

      id = nid;
    }
  }

  for (auto callback : block->callbacks_) {
    callback->inDbSWireRemoveSBox((dbSBox*) box);
  }
}

dbBlock* dbSWire::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbNet* dbSWire::getNet()
{
  _dbSWire* wire = (_dbSWire*) this;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->net_tbl_->getPtr(wire->net_);
}

dbWireType dbSWire::getWireType()
{
  _dbSWire* wire = (_dbSWire*) this;
  return wire->flags_.wire_type;
}

dbNet* dbSWire::getShield()
{
  _dbSWire* wire = (_dbSWire*) this;

  if (wire->shield_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->net_tbl_->getPtr(wire->shield_);
}

dbSet<dbSBox> dbSWire::getWires()
{
  _dbSWire* wire = (_dbSWire*) this;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return dbSet<dbSBox>(wire, block->sbox_itr_);
}

dbSWire* dbSWire::create(dbNet* net_, dbWireType type, dbNet* shield_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbNet* shield = (_dbNet*) shield_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  _dbSWire* wire = block->swire_tbl_->create();
  wire->flags_.wire_type = type.getValue();
  wire->net_ = net->getOID();
  wire->next_swire_ = net->swires_;
  net->swires_ = wire->getOID();

  if (shield) {
    wire->shield_ = shield->getOID();
  }
  for (auto callback : block->callbacks_) {
    callback->inDbSWireCreate((dbSWire*) wire);
  }
  return (dbSWire*) wire;
}

static void destroySBoxes(_dbSWire* wire)
{
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  dbId<_dbSBox> id = wire->wires_;
  if (id == 0) {
    return;
  }
  for (auto callback : block->callbacks_) {
    callback->inDbSWirePreDestroySBoxes((dbSWire*) wire);
  }
  while (id != 0) {
    _dbSBox* box = block->sbox_tbl_->getPtr(id);
    uint32_t nid = box->next_box_;
    dbProperty::destroyProperties(box);
    block->sbox_tbl_->destroy(box);
    id = nid;
  }
  for (auto callback : block->callbacks_) {
    callback->inDbSWirePostDestroySBoxes((dbSWire*) wire);
  }
}

void dbSWire::destroy(dbSWire* wire_)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbNet* net = block->net_tbl_->getPtr(wire->net_);
  _dbSWire* prev = nullptr;
  dbId<_dbSWire> id;
  // destroy the sboxes
  destroySBoxes(wire);
  for (auto callback : block->callbacks_) {
    callback->inDbSWireDestroy(wire_);
  }
  // unlink the swire
  for (id = net->swires_; id != 0; id = prev->next_swire_) {
    _dbSWire* w = block->swire_tbl_->getPtr(id);
    if (w == wire) {
      if (prev == nullptr) {
        net->swires_ = w->next_swire_;
      } else {
        prev->next_swire_ = w->next_swire_;
      }
      break;
    }
    prev = w;
  }
  assert(id != 0);
  // destroy the wire
  dbProperty::destroyProperties(wire);
  block->swire_tbl_->destroy(wire);
}

dbSet<dbSWire>::iterator dbSWire::destroy(dbSet<dbSWire>::iterator& itr)
{
  dbSWire* w = *itr;
  dbSet<dbSWire>::iterator next = ++itr;
  destroy(w);
  return next;
}

dbSWire* dbSWire::getSWire(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbSWire*) block->swire_tbl_->getPtr(dbid_);
}

void _dbSWire::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
