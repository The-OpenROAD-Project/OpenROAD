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

#include "dbSBox.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbDatabase.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"

namespace odb {

template class dbTable<_dbSBox>;

bool _dbSBox::operator==(const _dbSBox& rhs) const
{
  if (_sflags._wire_type != rhs._sflags._wire_type)
    return false;

  if (_dbBox::operator!=(rhs))
    return false;

  return true;
}

int _dbSBox::equal(const _dbSBox& rhs) const
{
  if (_sflags._wire_type != rhs._sflags._wire_type)
    return false;

  return _dbBox::equal(rhs);
}

bool _dbSBox::operator<(const _dbSBox& rhs) const
{
  if (_sflags._wire_type < rhs._sflags._wire_type)
    return true;

  if (_sflags._wire_type > rhs._sflags._wire_type)
    return false;

  return _dbBox::operator<(rhs);
}

void _dbSBox::differences(dbDiff& diff,
                          const char* field,
                          const _dbSBox& rhs) const
{
  if (diff.deepDiff())
    return;

  DIFF_BEGIN
  DIFF_FIELD(_sflags._wire_type);
  DIFF_FIELD(_flags._owner_type);
  DIFF_FIELD(_flags._is_tech_via);
  DIFF_FIELD(_flags._is_block_via);
  DIFF_FIELD(_flags._layer_id);
  DIFF_FIELD(_flags._via_id);
  if (!isOct())
    DIFF_FIELD(_shape._rect);
  DIFF_FIELD_NO_DEEP(_owner);
  DIFF_FIELD_NO_DEEP(_next_box);
  DIFF_END
}

void _dbSBox::out(dbDiff& diff, char side, const char* field) const
{
  if (!diff.deepDiff()) {
    DIFF_OUT_BEGIN
    DIFF_OUT_FIELD(_sflags._wire_type);
    DIFF_OUT_FIELD(_flags._owner_type);
    DIFF_OUT_FIELD(_flags._is_tech_via);
    DIFF_OUT_FIELD(_flags._is_block_via);
    DIFF_OUT_FIELD(_flags._layer_id);
    DIFF_OUT_FIELD(_flags._via_id);
    if (!isOct())
      DIFF_OUT_FIELD(_shape._rect);
    DIFF_OUT_FIELD(_owner);
    DIFF_END
  } else {
    DIFF_OUT_BEGIN
    DIFF_OUT_FIELD(_sflags._wire_type);

    switch (getType()) {
      case BLOCK_VIA: {
        int x, y;
        getViaXY(x, y);
        _dbVia* via = getBlockVia();
        diff.report("%c BLOCK-VIA %s (%d %d)\n", side, via->_name, x, y);
        break;
      }

      case TECH_VIA: {
        int x, y;
        getViaXY(x, y);
        _dbTechVia* via = getTechVia();
        diff.report("%c TECH-VIA %s (%d %d)\n", side, via->_name, x, y);
        break;
      }

      case BOX: {
        //_dbTechLayer * lay = getTechLayer();
        diff.report("%c BOX (%d %d) (%d %d)\n",
                    side,
                    _shape._rect.xMin(),
                    _shape._rect.yMin(),
                    _shape._rect.xMax(),
                    _shape._rect.yMax());
        break;
      }
    }

    DIFF_END
  }
}

////////////////////////////////////////////////////////////////////
//
// dbSBox - Methods
//
////////////////////////////////////////////////////////////////////

dbWireShapeType dbSBox::getWireShapeType()
{
  _dbSBox* box = (_dbSBox*) this;
  return dbWireShapeType(box->_sflags._wire_type);
}

dbSBox::Direction dbSBox::getDirection()
{
  _dbSBox* box = (_dbSBox*) this;
  return (dbSBox::Direction) box->_sflags._direction;
}

dbSWire* dbSBox::getSWire()
{
  return (dbSWire*) getBoxOwner();
}

Oct dbSBox::getOct()
{
  _dbSBox* box = (_dbSBox*) this;
  return box->_shape._oct;
}
dbSBox* dbSBox::create(dbSWire* wire_,
                       dbTechLayer* layer_,
                       int x1,
                       int y1,
                       int x2,
                       int y2,
                       dbWireShapeType type,
                       Direction dir,
                       int width)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbSBox* box = block->_sbox_tbl->create();

  uint dx;
  if (x2 > x1)
    dx = x2 - x1;
  else
    dx = x1 - x2;

  uint dy;
  if (y2 > y1)
    dy = y2 - y1;
  else
    dy = y1 - y2;

  switch (dir) {
    case UNDEFINED:
      if ((dx & 1) && (dy & 1))  // both odd
        return NULL;

      break;

    case HORIZONTAL:
      if (dy & 1)  // dy odd
        return NULL;
      break;

    case VERTICAL:
      if (dx & 1)  // dy odd
        return NULL;
      break;
    case OCTILINEAR:
      if (dx != dy)
        return NULL;
      break;
  }

  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::SWIRE;
  box->_owner = wire->getOID();
  GeomShape* _geomshape;
  if (dir == OCTILINEAR) {
    Point p1(x1, y1);
    Point p2(x2, y2);
    new (&box->_shape._oct) Oct();
    box->_shape._oct.init(p1, p2, width);
    box->_octilinear = true;
    _geomshape = (GeomShape*) &(box->_shape._oct);
  } else {
    box->_shape._rect.init(x1, y1, x2, y2);
    box->_octilinear = false;
    _geomshape = (GeomShape*) &(box->_shape._rect);
  }

  box->_sflags._wire_type = type.getValue();
  box->_sflags._direction = dir;

  // link box to wire
  box->_next_box = (uint) wire->_wires;
  wire->_wires = box->getOID();

  block->add_geom_shape(_geomshape);

  return (dbSBox*) box;
}

dbSBox* dbSBox::create(dbSWire* wire_,
                       dbVia* via_,
                       int x,
                       int y,
                       dbWireShapeType type)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbVia* via = (_dbVia*) via_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();

  if (via->_bbox == 0)
    return NULL;

  _dbBox* vbbox = block->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->_shape._rect.xMin() + x;
  int ymin = vbbox->_shape._rect.yMin() + y;
  int xmax = vbbox->_shape._rect.xMax() + x;
  int ymax = vbbox->_shape._rect.yMax() + y;
  _dbSBox* box = block->_sbox_tbl->create();
  box->_flags._owner_type = dbBoxOwner::SWIRE;
  box->_owner = wire->getOID();
  box->_shape._rect.init(xmin, ymin, xmax, ymax);
  box->_flags._is_block_via = 1;
  box->_flags._via_id = via->getOID();
  box->_sflags._wire_type = type.getValue();
  box->_octilinear = false;
  // link box to wire
  box->_next_box = (uint) wire->_wires;
  wire->_wires = box->getOID();

  block->add_rect(box->_shape._rect);
  return (dbSBox*) box;
}

dbSBox* dbSBox::create(dbSWire* wire_,
                       dbTechVia* via_,
                       int x,
                       int y,
                       dbWireShapeType type)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbTechVia* via = (_dbTechVia*) via_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();

  if (via->_bbox == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->_shape._rect.xMin() + x;
  int ymin = vbbox->_shape._rect.yMin() + y;
  int xmax = vbbox->_shape._rect.xMax() + x;
  int ymax = vbbox->_shape._rect.yMax() + y;
  _dbSBox* box = block->_sbox_tbl->create();
  box->_flags._owner_type = dbBoxOwner::SWIRE;
  box->_owner = wire->getOID();
  box->_shape._rect.init(xmin, ymin, xmax, ymax);
  box->_flags._is_tech_via = 1;
  box->_flags._via_id = via->getOID();
  box->_sflags._wire_type = type.getValue();
  box->_octilinear = false;
  // link box to wire
  box->_next_box = (uint) wire->_wires;
  wire->_wires = box->getOID();

  block->add_rect(box->_shape._rect);
  return (dbSBox*) box;
}

dbSBox* dbSBox::getSBox(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbSBox*) block->_sbox_tbl->getPtr(dbid_);
}

}  // namespace odb
