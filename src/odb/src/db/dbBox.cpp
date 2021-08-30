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

#include "dbBox.h"

#include "db.h"
#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockage.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbObstruction.h"
#include "dbRegion.h"
#include "dbSWire.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"

namespace odb {

template class dbTable<_dbBox>;

bool _dbBox::isOct() const
{
  return _octilinear;
  // return _flags._owner_type==dbBoxOwner::SWIRE &&( (_dbSBox*)this
  // )->_sflags._direction==3;
}

bool _dbBox::operator==(const _dbBox& rhs) const
{
  if (_flags._owner_type != rhs._flags._owner_type)
    return false;

  if (_flags._is_tech_via != rhs._flags._is_tech_via)
    return false;

  if (_flags._is_block_via != rhs._flags._is_block_via)
    return false;

  if (_flags._layer_id != rhs._flags._layer_id)
    return false;

  if (_flags._via_id != rhs._flags._via_id)
    return false;
  if (_octilinear != rhs._octilinear)
    return false;
  if (isOct() && _shape._oct != _shape._oct)
    return false;
  else if (_shape._rect != _shape._rect)
    return false;

  if (_owner != rhs._owner)
    return false;

  if (_next_box != rhs._next_box)
    return false;
  if (_octilinear != rhs._octilinear)
    return false;
  if (design_rule_width_ != rhs.design_rule_width_)
    return false;
  return true;
}

int _dbBox::equal(const _dbBox& rhs) const
{
  Type lhs_type = getType();
  Type rhs_type = rhs.getType();

  if (lhs_type != rhs_type)
    return false;

  switch (lhs_type) {
    case BLOCK_VIA: {
      _dbVia* lhs_via = getBlockVia();
      _dbVia* rhs_via = rhs.getBlockVia();

      if (strcmp(lhs_via->_name, rhs_via->_name) != 0)
        return false;
      break;
    }

    case TECH_VIA: {
      _dbTechVia* lhs_via = getTechVia();
      _dbTechVia* rhs_via = rhs.getTechVia();

      if (strcmp(lhs_via->_name, rhs_via->_name) != 0)
        return false;
      break;
    }

    case BOX: {
      _dbTechLayer* lhs_lay = getTechLayer();
      _dbTechLayer* rhs_lay = rhs.getTechLayer();

      if (strcmp(lhs_lay->_name, rhs_lay->_name) != 0)
        return false;
      break;
    }
  }
  if (_octilinear != rhs._octilinear)
    return false;
  if (design_rule_width_ != rhs.design_rule_width_)
    return false;
  if (isOct() && _shape._oct != _shape._oct)
    return false;
  else if (_shape._rect != _shape._rect)
    return false;

  return true;
}

bool _dbBox::operator<(const _dbBox& rhs) const
{
  Type lhs_type = getType();
  Type rhs_type = rhs.getType();

  if (lhs_type < rhs_type)
    return true;

  if (lhs_type > rhs_type)
    return false;

  switch (lhs_type) {
    case BLOCK_VIA: {
      _dbVia* lhs_via = getBlockVia();
      _dbVia* rhs_via = rhs.getBlockVia();
      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
      break;
    }

    case TECH_VIA: {
      _dbTechVia* lhs_via = getTechVia();
      _dbTechVia* rhs_via = rhs.getTechVia();
      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
      break;
    }

    case BOX: {
      if ((_flags._layer_id != 0) && (rhs._flags._layer_id != 0)) {
        _dbTechLayer* lhs_lay = getTechLayer();
        _dbTechLayer* rhs_lay = rhs.getTechLayer();
        int r = strcmp(lhs_lay->_name, rhs_lay->_name);

        if (r < 0)
          return true;

        if (r > 0)
          return false;
      }

      else if (_flags._layer_id != 0)
        return true;
      else if (rhs._flags._layer_id != 0)
        return false;

      break;
    }
  }
  if (!isOct() && !rhs.isOct())
    return _shape._rect < rhs._shape._rect;
  if (design_rule_width_ >= rhs.design_rule_width_)
    return false;
  return false;
}

void _dbBox::differences(dbDiff& diff,
                         const char* field,
                         const _dbBox& rhs) const
{
  if (diff.deepDiff())
    return;

  DIFF_BEGIN
  DIFF_FIELD(_flags._owner_type);
  DIFF_FIELD(_flags._is_tech_via);
  DIFF_FIELD(_flags._is_block_via);
  DIFF_FIELD(_flags._layer_id);
  DIFF_FIELD(_flags._via_id);
  DIFF_FIELD(_octilinear);

  if (isOct()) {
    DIFF_FIELD(_shape._oct);
  } else {
    DIFF_FIELD(_shape._rect);
  }
  DIFF_FIELD(_owner);
  DIFF_FIELD(_next_box);
  DIFF_FIELD(design_rule_width_);
  DIFF_END
}

void _dbBox::out(dbDiff& diff, char side, const char* field) const
{
  if (!diff.deepDiff()) {
    DIFF_OUT_BEGIN
    DIFF_OUT_FIELD(_flags._owner_type);
    DIFF_OUT_FIELD(_flags._is_tech_via);
    DIFF_OUT_FIELD(_flags._is_block_via);
    DIFF_OUT_FIELD(_flags._layer_id);
    DIFF_OUT_FIELD(_flags._via_id);
    DIFF_OUT_FIELD(_octilinear);
    if (isOct()) {
      DIFF_OUT_FIELD(_shape._oct);
    } else {
      DIFF_OUT_FIELD(_shape._rect);
    }
    DIFF_OUT_FIELD(_owner);
    DIFF_OUT_FIELD(_next_box);
    DIFF_OUT_FIELD(design_rule_width_);
    DIFF_END
  } else {
    DIFF_OUT_BEGIN

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
        if (_flags._layer_id != 0) {
          _dbTechLayer* lay = getTechLayer();
          diff.report("%c BOX %s (%d %d) (%d %d)\n",
                      side,
                      lay->_name,
                      _shape._rect.xMin(),
                      _shape._rect.yMin(),
                      _shape._rect.xMax(),
                      _shape._rect.yMax());
        } else {
          diff.report("%c BOX (%d %d) (%d %d)\n",
                      side,
                      _shape._rect.xMin(),
                      _shape._rect.yMin(),
                      _shape._rect.xMax(),
                      _shape._rect.yMax());
        }

        break;
      }
    }

    DIFF_END
  }
}

_dbTechLayer* _dbBox::getTechLayer() const
{
  if (_flags._layer_id == 0)
    return NULL;

  switch (_flags._owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::REGION:
      return NULL;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::SWIRE: {
      _dbDatabase* db = (_dbDatabase*) getDatabase();
      _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
      return tech->_layer_tbl->getPtr(_flags._layer_id);
    }

    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN: {
      _dbDatabase* db = (_dbDatabase*) getDatabase();
      _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
      return tech->_layer_tbl->getPtr(_flags._layer_id);
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) getOwner();
      return tech->_layer_tbl->getPtr(_flags._layer_id);
    }
  }

  ZASSERT(0);
  return NULL;
}

_dbTechVia* _dbBox::getTechVia() const
{
  if (_flags._is_tech_via == 0)
    return NULL;

  switch (_flags._owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::REGION:
      return NULL;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) getOwner();
      _dbDatabase* db = (_dbDatabase*) block->getDatabase();
      _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
      return tech->_via_tbl->getPtr(_flags._via_id);
    }

    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN: {
      _dbMaster* master = (_dbMaster*) getOwner();
      _dbDatabase* db = (_dbDatabase*) master->getDatabase();
      _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
      return tech->_via_tbl->getPtr(_flags._via_id);
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) getOwner();
      return tech->_via_tbl->getPtr(_flags._via_id);
    }
  }

  return NULL;
}

_dbVia* _dbBox::getBlockVia() const
{
  if (_flags._is_block_via == 0)
    return NULL;

  switch (_flags._owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::REGION:
      return NULL;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) getOwner();
      return block->_via_tbl->getPtr(_flags._via_id);
    }

    // There are no block-vias on these objects
    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN:
    case dbBoxOwner::TECH_VIA:
      break;
  }

  return NULL;
}

void _dbBox::getViaXY(int& x, int& y) const
{
  switch (getType()) {
    case BLOCK_VIA: {
      dbVia* v = (dbVia*) getBlockVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = _shape._rect.xMin() - xmin;
      y = _shape._rect.yMin() - ymin;
      break;
    }

    case TECH_VIA: {
      dbTechVia* v = (dbTechVia*) getTechVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = _shape._rect.xMin() - xmin;
      y = _shape._rect.yMin() - ymin;
      break;
    }

    default:
      break;
  }
}

////////////////////////////////////////////////////////////////////
//
// dbBox - Methods
//
////////////////////////////////////////////////////////////////////

GeomShape* dbBox::getGeomShape()
{
  _dbBox* box = (_dbBox*) this;
  return (GeomShape*) &box->_shape;
}

int dbBox::xMin()
{
  return getGeomShape()->xMin();
}

int dbBox::yMin()
{
  return getGeomShape()->yMin();
}

int dbBox::xMax()
{
  return getGeomShape()->xMax();
}

int dbBox::yMax()
{
  return getGeomShape()->yMax();
}

bool dbBox::isVia()
{
  _dbBox* box = (_dbBox*) this;
  return box->_flags._via_id != 0;
}

dbTechVia* dbBox::getTechVia()
{
  _dbBox* box = (_dbBox*) this;
  return (dbTechVia*) box->getTechVia();
}

dbVia* dbBox::getBlockVia()
{
  _dbBox* box = (_dbBox*) this;
  return (dbVia*) box->getBlockVia();
}

void dbBox::getBox(Rect& rect)
{
  _dbBox* box = (_dbBox*) this;
  if (box->isOct()) {
    GeomShape* interface = getGeomShape();
    rect = Rect(interface->xMin(),
                interface->yMin(),
                interface->xMax(),
                interface->yMax());
  } else
    rect = box->_shape._rect;
}

void dbBox::getViaBoxes(std::vector<dbShape>& shapes)
{
  _dbBox* box = (_dbBox*) this;

  int x = 0;
  int y = 0;
  box->getViaXY(x, y);

  dbSet<dbBox> boxes;

  if (box->_flags._is_tech_via) {
    boxes = getTechVia()->getBoxes();
  } else if (box->_flags._is_block_via) {
    boxes = getBlockVia()->getBoxes();
  } else {
    throw ZException("getViaBoxes called with non-via");
  }

  shapes.clear();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    dbBox* b = *itr;
    int xmin = b->xMin() + x;
    int ymin = b->yMin() + y;
    int xmax = b->xMax() + x;
    int ymax = b->yMax() + y;
    Rect r(xmin, ymin, xmax, ymax);
    dbShape shape(b->getTechLayer(), r);
    shapes.push_back(shape);
  }
}

int dbBox::getDir()
{
  Rect rect;
  getBox(rect);
  return rect.getDir();
}

uint dbBox::getDX()
{
  return getGeomShape()->dx();
}

uint dbBox::getDY()
{
  return getGeomShape()->dy();
}
uint dbBox::getWidth(uint dir)
{
  if (dir == 1)  // horizontal
    return getDY();
  else
    return getDX();
}

int dbBox::getDesignRuleWidth() const
{
  _dbBox* box = (_dbBox*) this;
  return box->design_rule_width_;
}

void dbBox::setDesignRuleWidth(int width)
{
  _dbBox* box = (_dbBox*) this;
  box->design_rule_width_ = width;
}

uint dbBox::getLength(uint dir)
{
  if (dir == 1)  // horizontal
    return getDX();
  else
    return getDY();
}

void dbBox::getViaXY(int& x, int& y)
{
  _dbBox* box = (_dbBox*) this;
  ZASSERT(box->_flags._is_tech_via || box->_flags._is_block_via);
  box->getViaXY(x, y);
}

dbObject* dbBox::getBoxOwner()
{
  _dbBox* box = (_dbBox*) this;

  dbObject* owner = getImpl()->getOwner();

  switch (box->_flags._owner_type) {
    case dbBoxOwner::UNKNOWN:
      return NULL;

    case dbBoxOwner::BLOCK: {
      return owner;
    }

    case dbBoxOwner::INST: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_inst_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::BTERM: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_bterm_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::BPIN: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_bpin_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::VIA: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_via_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::OBSTRUCTION: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_obstruction_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::BLOCKAGE: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_blockage_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_swire_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::MASTER: {
      return owner;
    }

    case dbBoxOwner::MPIN: {
      _dbMaster* master = (_dbMaster*) owner;
      return master->_mpin_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) owner;
      return tech->_via_tbl->getPtr(box->_owner);
    }

    case dbBoxOwner::REGION: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_swire_tbl->getPtr(box->_owner);
    }
  }

  ZASSERT(0);
  return NULL;
}

dbBoxOwner dbBox::getOwnerType()
{
  _dbBox* box = (_dbBox*) this;
  return dbBoxOwner(box->_flags._owner_type);
}

dbTechLayer* dbBox::getTechLayer()
{
  _dbBox* box = (_dbBox*) this;
  return (dbTechLayer*) box->getTechLayer();
}

dbBox* dbBox::create(dbBPin* bpin_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2)
{
  _dbBPin* bpin = (_dbBPin*) bpin_;
  _dbBlock* block = (_dbBlock*) bpin->getOwner();

  _dbBox* box = block->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::BPIN;
  box->_owner = bpin->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  box->_next_box = bpin->_boxes;
  bpin->_boxes = box->getOID();

  block->add_rect(box->_shape._rect);
  return (dbBox*) box;
}

dbBox* dbBox::create(dbVia* via_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2)
{
  _dbVia* via = (_dbVia*) via_;
  _dbBlock* block = (_dbBlock*) via->getOwner();
  _dbBox* box = block->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::VIA;
  box->_owner = via->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  // update via bbox
  if (via->_bbox == 0) {
    _dbBox* vbbox = block->_box_tbl->create();
    vbbox->_flags._owner_type = dbBoxOwner::VIA;
    vbbox->_owner = via->getOID();
    vbbox->_shape._rect.init(x1, y1, x2, y2);
    via->_bbox = vbbox->getOID();
  } else {
    _dbBox* vbbox = block->_box_tbl->getPtr(via->_bbox);
    vbbox->_shape._rect.merge(box->_shape._rect);
  }

  // Update the top-bottom layer of this via
  if (via->_top == 0) {
    via->_top = box->_flags._layer_id;
    via->_bottom = box->_flags._layer_id;
  } else {
    _dbTechLayer* layer = (_dbTechLayer*) layer_;
    _dbTech* tech = (_dbTech*) layer->getOwner();
    _dbTechLayer* top = tech->_layer_tbl->getPtr(via->_top);
    _dbTechLayer* bottom = tech->_layer_tbl->getPtr(via->_bottom);

    if (layer->_number > top->_number)
      via->_top = layer->getOID();

    if (layer->_number < bottom->_number)
      via->_bottom = layer->getOID();
  }

  // link box to via
  box->_next_box = via->_boxes;
  via->_boxes = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMaster* master_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2)
{
  _dbMaster* master = (_dbMaster*) master_;
  _dbBox* box = master->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::MASTER;
  box->_owner = master->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  // link box to master
  box->_next_box = master->_obstructions;
  master->_obstructions = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMaster* master_, dbTechVia* via_, int x, int y)
{
  _dbMaster* master = (_dbMaster*) master_;
  _dbTechVia* via = (_dbTechVia*) via_;

  if (via->_bbox == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->_shape._rect.xMin() + x;
  int ymin = vbbox->_shape._rect.yMin() + y;
  int xmax = vbbox->_shape._rect.xMax() + x;
  int ymax = vbbox->_shape._rect.yMax() + y;
  _dbBox* box = master->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._owner_type = dbBoxOwner::MASTER;
  box->_owner = master->getOID();
  box->_shape._rect.init(xmin, ymin, xmax, ymax);
  box->_flags._is_tech_via = 1;
  box->_flags._via_id = via->getOID();

  // link box to master
  box->_next_box = master->_obstructions;
  master->_obstructions = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMPin* pin_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2)
{
  _dbMPin* pin = (_dbMPin*) pin_;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  _dbBox* box = master->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::MPIN;
  box->_owner = pin->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  // link box to pin
  box->_next_box = pin->_geoms;
  pin->_geoms = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMPin* pin_, dbTechVia* via_, int x, int y)
{
  _dbMPin* pin = (_dbMPin*) pin_;
  _dbTechVia* via = (_dbTechVia*) via_;

  if (via->_bbox == 0)
    return NULL;

  _dbMaster* master = (_dbMaster*) pin->getOwner();
  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->_shape._rect.xMin() + x;
  int ymin = vbbox->_shape._rect.yMin() + y;
  int xmax = vbbox->_shape._rect.xMax() + x;
  int ymax = vbbox->_shape._rect.yMax() + y;
  _dbBox* box = master->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._owner_type = dbBoxOwner::MPIN;
  box->_owner = pin->getOID();
  box->_shape._rect.init(xmin, ymin, xmax, ymax);
  box->_flags._is_tech_via = 1;
  box->_flags._via_id = via->getOID();

  // link box to pin
  box->_next_box = pin->_geoms;
  pin->_geoms = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbTechVia* via_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2)
{
  _dbTechVia* via = (_dbTechVia*) via_;
  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* box = tech->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._layer_id = layer_->getImpl()->getOID();
  box->_flags._owner_type = dbBoxOwner::TECH_VIA;
  box->_owner = via->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  // update via bbox
  if (via->_bbox == 0) {
    _dbBox* vbbox = tech->_box_tbl->create();
    // 10302012D via group POWER Extraction
    // vbbox->_flags._is_tech_via = 1;
    vbbox->_flags._owner_type = dbBoxOwner::TECH_VIA;
    vbbox->_owner = via->getOID();
    vbbox->_shape._rect.init(x1, y1, x2, y2);
    via->_bbox = vbbox->getOID();
  } else {
    _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
    vbbox->_shape._rect.merge(box->_shape._rect);
  }

  // Update the top-bottom layer of this via
  if (via->_top == 0) {
    via->_top = box->_flags._layer_id;
    via->_bottom = box->_flags._layer_id;
  } else {
    _dbTechLayer* layer = (_dbTechLayer*) layer_;
    _dbTechLayer* top = tech->_layer_tbl->getPtr(via->_top);
    _dbTechLayer* bottom = tech->_layer_tbl->getPtr(via->_bottom);

    if (layer->_number > top->_number)
      via->_top = layer_->getImpl()->getOID();

    if (layer->_number < bottom->_number)
      via->_bottom = layer_->getImpl()->getOID();
  }

  // link box to via
  box->_next_box = via->_boxes;
  via->_boxes = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbRegion* region_, int x1, int y1, int x2, int y2)
{
  _dbRegion* region = (_dbRegion*) region_;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  _dbBox* box = block->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._owner_type = dbBoxOwner::REGION;
  box->_owner = region->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);

  // link box to region
  box->_next_box = region->_boxes;
  region->_boxes = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbInst* inst_, int x1, int y1, int x2, int y2)
{
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (inst->_halo)
    return NULL;

  _dbBox* box = block->_box_tbl->create();
  box->_octilinear = false;
  box->_flags._owner_type = dbBoxOwner::INST;
  box->_owner = inst->getOID();
  box->_shape._rect.init(x1, y1, x2, y2);
  inst->_halo = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::getBox(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBox*) block->_box_tbl->getPtr(dbid_);
}

dbBox* dbBox::getBox(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbBox*) tech->_box_tbl->getPtr(dbid_);
}

dbBox* dbBox::getBox(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbBox*) master->_box_tbl->getPtr(dbid_);
}

bool dbBox::isVisited()
{
  _dbBox* box = (_dbBox*) this;
  return box->_flags._visited == 1;
}
void dbBox::setVisited(bool value)
{
  /*
          if (getId()==333485)
  {
                  fprintf(stdout, "setVisited=%d\n", value);
  }
  */
  _dbBox* box = (_dbBox*) this;
  box->_flags._visited = (value == true) ? 1 : 0;
}
bool dbBox::isMarked()
{
  _dbBox* box = (_dbBox*) this;
  return box->_flags._mark == 1;
}
void dbBox::setMarked(bool value)
{
  _dbBox* box = (_dbBox*) this;
  box->_flags._mark = (value == true) ? 1 : 0;
}

}  // namespace odb
