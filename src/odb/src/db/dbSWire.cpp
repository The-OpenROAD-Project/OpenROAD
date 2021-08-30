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

#include "dbSWire.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbDiff.hpp"
#include "dbNet.h"
#include "dbSBox.h"
#include "dbSBoxItr.h"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbSWire>;

bool _dbSWire::operator==(const _dbSWire& rhs) const
{
  if (_flags._wire_type != rhs._flags._wire_type)
    return false;

  if (_net != rhs._net)
    return false;

  if (_shield != rhs._shield)
    return false;

  if (_wires != rhs._wires)
    return false;

  if (_next_swire != rhs._next_swire)
    return false;

  return true;
}

bool _dbSWire::operator<(const _dbSWire& rhs) const
{
  if (_flags._wire_type < rhs._flags._wire_type)
    return true;

  if (_flags._wire_type > rhs._flags._wire_type)
    return false;

  if ((_shield != 0) && (rhs._shield != 0)) {
    _dbBlock* lhs_blk = (_dbBlock*) getOwner();
    _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
    _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
    _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);
    int r = strcmp(lhs_net->_name, rhs_net->_name);

    if (r < 0)
      return true;

    if (r > 0)
      return false;
  } else if (_shield != 0) {
    return false;
  } else if (rhs._shield != 0) {
    return true;
  }

  return false;
}

void _dbSWire::differences(dbDiff& diff,
                           const char* field,
                           const _dbSWire& rhs) const
{
  _dbBlock* lhs_block = (_dbBlock*) getOwner();
  _dbBlock* rhs_block = (_dbBlock*) rhs.getOwner();

  DIFF_BEGIN
  DIFF_FIELD(_flags._wire_type);
  DIFF_FIELD_NO_DEEP(_net);

  if (!diff.deepDiff()) {
    DIFF_FIELD(_shield);
  } else {
    if ((_shield != 0) && (rhs._shield != 0)) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
      _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);
      diff.diff("_shield", lhs_net->_name, rhs_net->_name);
    } else if (_shield != 0) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
      diff.out(dbDiff::LEFT, "_shield", lhs_net->_name);
    } else if (rhs._shield != 0) {
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);
      diff.out(dbDiff::RIGHT, "_shield", rhs_net->_name);
    }
  }

  DIFF_SET(_wires, lhs_block->_sbox_itr, rhs_block->_sbox_itr);
  DIFF_FIELD_NO_DEEP(_next_swire);
  DIFF_END
}

void _dbSWire::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._wire_type);
  DIFF_OUT_FIELD_NO_DEEP(_net);

  if (!diff.deepDiff()) {
    DIFF_OUT_FIELD(_shield);
  } else {
    if (_shield != 0) {
      _dbBlock* blk = (_dbBlock*) getOwner();
      _dbNet* net = blk->_net_tbl->getPtr(_net);
      diff.out(side, "_shield", net->_name);
    }
  }

  DIFF_OUT_SET(_wires, block->_sbox_itr);
  DIFF_OUT_FIELD_NO_DEEP(_next_swire);
  DIFF_END
}

dbBlock* dbSWire::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbNet* dbSWire::getNet()
{
  _dbSWire* wire = (_dbSWire*) this;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(wire->_net);
}

dbWireType dbSWire::getWireType()
{
  _dbSWire* wire = (_dbSWire*) this;
  return wire->_flags._wire_type;
}

dbNet* dbSWire::getShield()
{
  _dbSWire* wire = (_dbSWire*) this;

  if (wire->_shield == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(wire->_shield);
}

dbSet<dbSBox> dbSWire::getWires()
{
  _dbSWire* wire = (_dbSWire*) this;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return dbSet<dbSBox>(wire, block->_sbox_itr);
}

dbSWire* dbSWire::create(dbNet* net_, dbWireType type, dbNet* shield_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbNet* shield = (_dbNet*) shield_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  _dbSWire* wire = block->_swire_tbl->create();
  wire->_flags._wire_type = type.getValue();
  wire->_net = net->getOID();
  wire->_next_swire = net->_swires;
  net->_swires = wire->getOID();

  if (shield)
    wire->_shield = shield->getOID();
  for (auto callback : block->_callbacks)
    callback->inDbSWireCreate((dbSWire*) wire);
  return (dbSWire*) wire;
}

static void destroySBoxes(_dbSWire* wire)
{
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  dbId<_dbSBox> id = wire->_wires;
  if (id == 0)
    return;
  for (auto callback : block->_callbacks)
    callback->inDbSWirePreDestroySBoxes((dbSWire*) wire);
  while (id != 0) {
    _dbSBox* box = block->_sbox_tbl->getPtr(id);
    uint nid = box->_next_box;
    dbProperty::destroyProperties(box);
    block->_sbox_tbl->destroy(box);
    id = nid;
  }
  for (auto callback : block->_callbacks)
    callback->inDbSWirePostDestroySBoxes((dbSWire*) wire);
}

void dbSWire::destroy(dbSWire* wire_)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbNet* net = block->_net_tbl->getPtr(wire->_net);
  _dbSWire* prev = NULL;
  dbId<_dbSWire> id;
  // destroy the sboxes
  destroySBoxes(wire);
  for (auto callback : block->_callbacks)
    callback->inDbSWireDestroy(wire_);
  // unlink the swire
  for (id = net->_swires; id != 0; id = prev->_next_swire) {
    _dbSWire* w = block->_swire_tbl->getPtr(id);
    if (w == wire) {
      if (prev == NULL)
        net->_swires = w->_next_swire;
      else
        prev->_next_swire = w->_next_swire;
      break;
    }
    prev = w;
  }
  ZASSERT(id != 0);
  // destroy the wire
  dbProperty::destroyProperties(wire);
  block->_swire_tbl->destroy(wire);
}

dbSet<dbSWire>::iterator dbSWire::destroy(dbSet<dbSWire>::iterator& itr)
{
  dbSWire* w = *itr;
  dbSet<dbSWire>::iterator next = ++itr;
  destroy(w);
  return next;
}

dbSWire* dbSWire::getSWire(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbSWire*) block->_swire_tbl->getPtr(dbid_);
}

}  // namespace odb
