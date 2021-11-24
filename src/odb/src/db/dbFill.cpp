///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "dbFill.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"

namespace odb {

template class dbTable<_dbFill>;

bool _dbFill::operator==(const _dbFill& rhs) const
{
  if (_flags._opc != rhs._flags._opc)
    return false;

  if (_flags._mask_id != rhs._flags._mask_id)
    return false;

  if (_flags._layer_id != rhs._flags._layer_id)
    return false;

  if (_rect != rhs._rect)
    return false;

  return true;
}

bool _dbFill::operator<(const _dbFill& rhs) const
{
  if (_rect < rhs._rect)
    return true;

  if (_rect > rhs._rect)
    return false;

  if (_flags._opc < rhs._flags._opc)
    return true;

  if (_flags._opc > rhs._flags._opc)
    return false;

  if (_flags._mask_id < rhs._flags._mask_id)
    return true;

  if (_flags._mask_id > rhs._flags._mask_id)
    return false;

  if (_flags._layer_id < rhs._flags._layer_id)
    return true;

  if (_flags._layer_id > rhs._flags._layer_id)
    return false;

  return false;
}

void _dbFill::differences(dbDiff& diff,
                          const char* field,
                          const _dbFill& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._opc);
  DIFF_FIELD(_flags._mask_id);
  DIFF_FIELD(_flags._layer_id);
  DIFF_FIELD(_rect);
  DIFF_END
}

void _dbFill::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._opc);
  DIFF_OUT_FIELD(_flags._mask_id);
  DIFF_OUT_FIELD(_flags._layer_id);
  DIFF_OUT_FIELD(_rect);
  DIFF_END
}

_dbTechLayer* _dbFill::getTechLayer() const
{
  _dbDatabase* db = (_dbDatabase*) getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
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

  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block_internal->_callbacks.begin();
       cbitr != block_internal->_callbacks.end();
       ++cbitr) {
    (*cbitr)->inDbFillCreate((dbFill*) fill);
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

}  // namespace odb
