// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbFill.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace odb {

template class dbTable<_dbFill>;

bool _dbFill::operator==(const _dbFill& rhs) const
{
  if (_flags._opc != rhs._flags._opc) {
    return false;
  }

  if (_flags._mask_id != rhs._flags._mask_id) {
    return false;
  }

  if (_flags._layer_id != rhs._flags._layer_id) {
    return false;
  }

  if (_rect != rhs._rect) {
    return false;
  }

  return true;
}

bool _dbFill::operator<(const _dbFill& rhs) const
{
  if (_rect < rhs._rect) {
    return true;
  }

  if (_rect > rhs._rect) {
    return false;
  }

  if (_flags._opc < rhs._flags._opc) {
    return true;
  }

  if (_flags._opc > rhs._flags._opc) {
    return false;
  }

  if (_flags._mask_id < rhs._flags._mask_id) {
    return true;
  }

  if (_flags._mask_id > rhs._flags._mask_id) {
    return false;
  }

  if (_flags._layer_id < rhs._flags._layer_id) {
    return true;
  }

  if (_flags._layer_id > rhs._flags._layer_id) {
    return false;
  }

  return false;
}

_dbTechLayer* _dbFill::getTechLayer() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbTech* tech = block->getTech();
  return tech->_layer_tbl->getPtr(_flags._layer_id);
}

////////////////////////////////////////////////////////////////////
//
// dbFill - Methods
//
////////////////////////////////////////////////////////////////////

void dbFill::getRect(Rect& rect)
{
  _dbFill* fill = (_dbFill*) this;
  rect = fill->_rect;
}

bool dbFill::needsOPC()
{
  _dbFill* fill = (_dbFill*) this;
  return fill->_flags._opc;
}

uint dbFill::maskNumber()
{
  _dbFill* fill = (_dbFill*) this;
  return fill->_flags._mask_id;
}

dbTechLayer* dbFill::getTechLayer()
{
  _dbFill* fill = (_dbFill*) this;
  return (dbTechLayer*) fill->getTechLayer();
}

dbFill* dbFill::create(dbBlock* block,
                       bool needs_opc,
                       uint mask_number,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2)
{
  _dbBlock* block_internal = (_dbBlock*) block;
  _dbFill* fill = block_internal->_fill_tbl->create();
  fill->_flags._opc = needs_opc;
  fill->_flags._mask_id = mask_number;
  fill->_flags._layer_id = layer->getImpl()->getOID();
  fill->_rect.init(x1, y1, x2, y2);

  for (auto cb : block_internal->_callbacks) {
    cb->inDbFillCreate((dbFill*) fill);
  }

  return (dbFill*) fill;
}

void dbFill::destroy(dbFill* fill_)
{
  _dbFill* fill = (_dbFill*) fill_;
  _dbBlock* block = (_dbBlock*) fill->getOwner();
  dbProperty::destroyProperties(fill);
  block->_fill_tbl->destroy(fill);
}

dbSet<dbFill>::iterator dbFill::destroy(dbSet<dbFill>::iterator& itr)
{
  dbFill* r = *itr;
  dbSet<dbFill>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbFill* dbFill::getFill(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbFill*) block->_fill_tbl->getPtr(dbid_);
}

void _dbFill::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
