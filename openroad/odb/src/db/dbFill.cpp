// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbFill.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbFill>;

bool _dbFill::operator==(const _dbFill& rhs) const
{
  if (flags_.opc != rhs.flags_.opc) {
    return false;
  }

  if (flags_.mask_id != rhs.flags_.mask_id) {
    return false;
  }

  if (flags_.layer_id != rhs.flags_.layer_id) {
    return false;
  }

  if (rect_ != rhs.rect_) {
    return false;
  }

  return true;
}

bool _dbFill::operator<(const _dbFill& rhs) const
{
  if (rect_ < rhs.rect_) {
    return true;
  }

  if (rect_ > rhs.rect_) {
    return false;
  }

  if (flags_.opc < rhs.flags_.opc) {
    return true;
  }

  if (flags_.opc > rhs.flags_.opc) {
    return false;
  }

  if (flags_.mask_id < rhs.flags_.mask_id) {
    return true;
  }

  if (flags_.mask_id > rhs.flags_.mask_id) {
    return false;
  }

  if (flags_.layer_id < rhs.flags_.layer_id) {
    return true;
  }

  if (flags_.layer_id > rhs.flags_.layer_id) {
    return false;
  }

  return false;
}

_dbTechLayer* _dbFill::getTechLayer() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbTech* tech = block->getTech();
  return tech->layer_tbl_->getPtr(flags_.layer_id);
}

////////////////////////////////////////////////////////////////////
//
// dbFill - Methods
//
////////////////////////////////////////////////////////////////////

void dbFill::getRect(Rect& rect)
{
  _dbFill* fill = (_dbFill*) this;
  rect = fill->rect_;
}

bool dbFill::needsOPC()
{
  _dbFill* fill = (_dbFill*) this;
  return fill->flags_.opc;
}

uint32_t dbFill::maskNumber()
{
  _dbFill* fill = (_dbFill*) this;
  return fill->flags_.mask_id;
}

dbTechLayer* dbFill::getTechLayer()
{
  _dbFill* fill = (_dbFill*) this;
  return (dbTechLayer*) fill->getTechLayer();
}

dbFill* dbFill::create(dbBlock* block,
                       bool needs_opc,
                       uint32_t mask_number,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2)
{
  _dbBlock* block_internal = (_dbBlock*) block;
  _dbFill* fill = block_internal->fill_tbl_->create();
  fill->flags_.opc = needs_opc;
  fill->flags_.mask_id = mask_number;
  fill->flags_.layer_id = layer->getImpl()->getOID();
  fill->rect_.init(x1, y1, x2, y2);

  for (auto cb : block_internal->callbacks_) {
    cb->inDbFillCreate((dbFill*) fill);
  }

  return (dbFill*) fill;
}

void dbFill::destroy(dbFill* fill_)
{
  _dbFill* fill = (_dbFill*) fill_;
  _dbBlock* block = (_dbBlock*) fill->getOwner();
  dbProperty::destroyProperties(fill);
  block->fill_tbl_->destroy(fill);
}

dbSet<dbFill>::iterator dbFill::destroy(dbSet<dbFill>::iterator& itr)
{
  dbFill* r = *itr;
  dbSet<dbFill>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbFill* dbFill::getFill(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbFill*) block->fill_tbl_->getPtr(dbid_);
}

void _dbFill::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
