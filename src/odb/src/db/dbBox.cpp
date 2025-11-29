// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBox.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockage.h"
#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbObstruction.h"
#include "dbPolygon.h"
#include "dbRegion.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "odb/ZException.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "odb/odb.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbBox>;

bool _dbBox::isOct() const
{
  return flags_.octilinear;
}

bool _dbBox::operator==(const _dbBox& rhs) const
{
  if (flags_.owner_type != rhs.flags_.owner_type) {
    return false;
  }

  if (flags_.is_tech_via != rhs.flags_.is_tech_via) {
    return false;
  }

  if (flags_.is_block_via != rhs.flags_.is_block_via) {
    return false;
  }

  if (flags_.layer_id != rhs.flags_.layer_id) {
    return false;
  }

  if (flags_.layer_mask != rhs.flags_.layer_mask) {
    return false;
  }

  if (flags_.via_id != rhs.flags_.via_id) {
    return false;
  }
  if (flags_.octilinear != rhs.flags_.octilinear) {
    return false;
  }
  if (isOct() && shape_.oct != rhs.shape_.oct) {
    return false;
  }
  if (shape_.rect != rhs.shape_.rect) {
    return false;
  }

  if (owner_ != rhs.owner_) {
    return false;
  }

  if (next_box_ != rhs.next_box_) {
    return false;
  }
  if (design_rule_width_ != rhs.design_rule_width_) {
    return false;
  }
  return true;
}

int _dbBox::equal(const _dbBox& rhs) const
{
  const Type lhs_type = getType();
  const Type rhs_type = rhs.getType();

  if (lhs_type != rhs_type) {
    return false;
  }

  switch (lhs_type) {
    case kBlockVia: {
      _dbVia* lhs_via = getBlockVia();
      _dbVia* rhs_via = rhs.getBlockVia();

      if (strcmp(lhs_via->_name, rhs_via->_name) != 0) {
        return false;
      }
      break;
    }

    case kTechVia: {
      _dbTechVia* lhs_via = getTechVia();
      _dbTechVia* rhs_via = rhs.getTechVia();

      if (strcmp(lhs_via->_name, rhs_via->_name) != 0) {
        return false;
      }
      break;
    }

    case kBox: {
      _dbTechLayer* lhs_lay = getTechLayer();
      _dbTechLayer* rhs_lay = rhs.getTechLayer();

      if (strcmp(lhs_lay->_name, rhs_lay->_name) != 0) {
        return false;
      }
      break;
    }
  }
  if (flags_.octilinear != rhs.flags_.octilinear) {
    return false;
  }
  if (flags_.layer_mask != rhs.flags_.layer_mask) {
    return false;
  }
  if (design_rule_width_ != rhs.design_rule_width_) {
    return false;
  }
  if (isOct() && shape_.oct != rhs.shape_.oct) {
    return false;
  }
  if (shape_.rect != rhs.shape_.rect) {
    return false;
  }

  return true;
}

bool _dbBox::operator<(const _dbBox& rhs) const
{
  const Type lhs_type = getType();
  const Type rhs_type = rhs.getType();

  if (lhs_type < rhs_type) {
    return true;
  }

  if (lhs_type > rhs_type) {
    return false;
  }

  switch (lhs_type) {
    case kBlockVia: {
      _dbVia* lhs_via = getBlockVia();
      _dbVia* rhs_via = rhs.getBlockVia();
      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case kTechVia: {
      _dbTechVia* lhs_via = getTechVia();
      _dbTechVia* rhs_via = rhs.getTechVia();
      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case kBox: {
      if ((flags_.layer_id != 0) && (rhs.flags_.layer_id != 0)) {
        _dbTechLayer* lhs_lay = getTechLayer();
        _dbTechLayer* rhs_lay = rhs.getTechLayer();
        int r = strcmp(lhs_lay->_name, rhs_lay->_name);

        if (r < 0) {
          return true;
        }

        if (r > 0) {
          return false;
        }
      }

      else if (flags_.layer_id != 0) {
        return true;
      } else if (rhs.flags_.layer_id != 0) {
        return false;
      }

      break;
    }
  }
  if (!isOct() && !rhs.isOct()) {
    return shape_.rect < rhs.shape_.rect;
  }
  if (design_rule_width_ >= rhs.design_rule_width_) {
    return false;
  }
  if (flags_.layer_mask >= rhs.flags_.layer_mask) {
    return false;
  }
  return false;
}

_dbBox::Type _dbBox::getType() const
{
  if (flags_.is_tech_via) {
    return kTechVia;
  }

  if (flags_.is_block_via) {
    return kBlockVia;
  }

  return kBox;
}

_dbTechLayer* _dbBox::getTechLayer() const
{
  if (flags_.layer_id == 0) {
    return nullptr;
  }

  switch (flags_.owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::REGION:
      return nullptr;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) getOwner();
      _dbTech* tech = block->getTech();
      return tech->_layer_tbl->getPtr(flags_.layer_id);
    }

    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN:
    case dbBoxOwner::PBOX: {
      _dbMaster* master = (_dbMaster*) getOwner();
      _dbLib* lib = (_dbLib*) master->getOwner();
      _dbTech* tech = lib->getTech();
      return tech->_layer_tbl->getPtr(flags_.layer_id);
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) getOwner();
      return tech->_layer_tbl->getPtr(flags_.layer_id);
    }
  }

  ZASSERT(0);
  return nullptr;
}

_dbTechVia* _dbBox::getTechVia() const
{
  if (flags_.is_tech_via == 0) {
    return nullptr;
  }

  switch (flags_.owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::REGION:
    case dbBoxOwner::PBOX:
      return nullptr;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) getOwner();
      _dbTech* tech = block->getTech();
      return tech->_via_tbl->getPtr(flags_.via_id);
    }

    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN: {
      _dbMaster* master = (_dbMaster*) getOwner();
      _dbLib* lib = (_dbLib*) master->getOwner();
      _dbDatabase* db = (_dbDatabase*) master->getDatabase();
      _dbTech* tech = db->_tech_tbl->getPtr(lib->_tech);
      return tech->_via_tbl->getPtr(flags_.via_id);
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) getOwner();
      return tech->_via_tbl->getPtr(flags_.via_id);
    }
  }

  return nullptr;
}

_dbVia* _dbBox::getBlockVia() const
{
  if (flags_.is_block_via == 0) {
    return nullptr;
  }

  switch (flags_.owner_type) {
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::REGION:
    case dbBoxOwner::PBOX:
      return nullptr;

    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::BPIN:
    case dbBoxOwner::VIA:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) getOwner();
      return block->_via_tbl->getPtr(flags_.via_id);
    }

    // There are no block-vias on these objects
    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN:
    case dbBoxOwner::TECH_VIA:
      break;
  }

  return nullptr;
}

Point _dbBox::getViaXY() const
{
  switch (getType()) {
    case kBlockVia: {
      dbVia* v = (dbVia*) getBlockVia();
      dbBox* b = v->getBBox();
      const int xmin = b->xMin();
      const int ymin = b->yMin();
      return {shape_.rect.xMin() - xmin, shape_.rect.yMin() - ymin};
    }

    case kTechVia: {
      dbTechVia* v = (dbTechVia*) getTechVia();
      dbBox* b = v->getBBox();
      const int xmin = b->xMin();
      const int ymin = b->yMin();
      return {shape_.rect.xMin() - xmin, shape_.rect.yMin() - ymin};
    }

    default:
      break;
  }
  return {};
}

void _dbBox::checkMask(const int mask) const
{
  if (mask >= 4) {
    getImpl()->getLogger()->error(
        utl::ODB, 434, "Mask must be between 0 and 3.");
  }
}

////////////////////////////////////////////////////////////////////
//
// dbBox - Methods
//
////////////////////////////////////////////////////////////////////

int dbBox::xMin() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.xMin();
  }
  return box->shape_.rect.xMin();
}

int dbBox::yMin() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.yMin();
  }
  return box->shape_.rect.yMin();
}

int dbBox::xMax() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.xMax();
  }
  return box->shape_.rect.xMax();
}

int dbBox::yMax() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.yMax();
  }
  return box->shape_.rect.yMax();
}

bool dbBox::isVia() const
{
  const _dbBox* box = (const _dbBox*) this;
  return box->flags_.via_id != 0;
}

dbTechVia* dbBox::getTechVia() const
{
  const _dbBox* box = (const _dbBox*) this;
  return (dbTechVia*) box->getTechVia();
}

dbVia* dbBox::getBlockVia() const
{
  const _dbBox* box = (const _dbBox*) this;
  return (dbVia*) box->getBlockVia();
}

Rect dbBox::getBox() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->isOct()) {
    const Oct oct = box->shape_.oct;
    return Rect(oct.xMin(), oct.yMin(), oct.xMax(), oct.yMax());
  }
  return box->shape_.rect;
}

void dbBox::getViaBoxes(std::vector<dbShape>& shapes) const
{
  const _dbBox* box = (const _dbBox*) this;

  const Point pt = box->getViaXY();

  dbSet<dbBox> boxes;

  if (box->flags_.is_tech_via) {
    boxes = getTechVia()->getBoxes();
  } else if (box->flags_.is_block_via) {
    boxes = getBlockVia()->getBoxes();
  } else {
    throw std::runtime_error("getViaBoxes called with non-via");
  }

  shapes.clear();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    dbBox* b = *itr;
    const int xmin = b->xMin() + pt.getX();
    const int ymin = b->yMin() + pt.getY();
    const int xmax = b->xMax() + pt.getX();
    const int ymax = b->yMax() + pt.getY();
    const Rect r(xmin, ymin, xmax, ymax);
    const dbShape shape(b->getTechLayer(), r);
    shapes.push_back(shape);
  }
}

void dbBox::getViaLayerBoxes(dbTechLayer* layer,
                             std::vector<dbShape>& shapes) const
{
  const _dbBox* box = (const _dbBox*) this;

  const Point pt = box->getViaXY();

  dbSet<dbBox> boxes;

  if (box->flags_.is_tech_via) {
    boxes = getTechVia()->getBoxes();
  } else if (box->flags_.is_block_via) {
    boxes = getBlockVia()->getBoxes();
  } else {
    throw std::runtime_error("getViaBoxes called with non-via");
  }

  shapes.clear();

  for (dbBox* b : boxes) {
    dbTechLayer* box_layer = b->getTechLayer();
    if (box_layer == layer) {
      const int xmin = b->xMin() + pt.getX();
      const int ymin = b->yMin() + pt.getY();
      const int xmax = b->xMax() + pt.getX();
      const int ymax = b->yMax() + pt.getY();
      Rect r(xmin, ymin, xmax, ymax);
      dbShape shape(box_layer, r);
      shapes.push_back(shape);
    }
  }
}

Orientation2D dbBox::getDir() const
{
  Rect rect = getBox();
  return rect.getDir();
}

uint dbBox::getDX() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.dx();
  }
  return box->shape_.rect.dx();
}

uint dbBox::getDY() const
{
  const _dbBox* box = (const _dbBox*) this;
  if (box->flags_.octilinear) {
    return box->shape_.oct.dy();
  }
  return box->shape_.rect.dy();
}

int dbBox::getDesignRuleWidth() const
{
  const _dbBox* box = (const _dbBox*) this;
  return box->design_rule_width_;
}

void dbBox::setDesignRuleWidth(const int width)
{
  _dbBox* box = (_dbBox*) this;
  box->design_rule_width_ = width;
}

Point dbBox::getViaXY() const
{
  const _dbBox* box = (const _dbBox*) this;
  return box->getViaXY();
}

dbObject* dbBox::getBoxOwner() const
{
  const _dbBox* box = (const _dbBox*) this;

  dbObject* owner = getImpl()->getOwner();

  switch (box->flags_.owner_type) {
    case dbBoxOwner::UNKNOWN:
      return nullptr;

    case dbBoxOwner::BLOCK: {
      return owner;
    }

    case dbBoxOwner::INST: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_inst_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::BTERM: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_bterm_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::BPIN: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_bpin_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::VIA: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_via_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::OBSTRUCTION: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_obstruction_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::BLOCKAGE: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_blockage_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::SWIRE: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_swire_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::MASTER: {
      return owner;
    }

    case dbBoxOwner::MPIN: {
      _dbMaster* master = (_dbMaster*) owner;
      return master->_mpin_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::PBOX: {
      return owner;
    }

    case dbBoxOwner::TECH_VIA: {
      _dbTech* tech = (_dbTech*) owner;
      return tech->_via_tbl->getPtr(box->owner_);
    }

    case dbBoxOwner::REGION: {
      _dbBlock* block = (_dbBlock*) owner;
      return block->_swire_tbl->getPtr(box->owner_);
    }
  }

  ZASSERT(0);
  return nullptr;
}

dbBoxOwner dbBox::getOwnerType() const
{
  const _dbBox* box = (const _dbBox*) this;
  return dbBoxOwner(box->flags_.owner_type);
}

dbTechLayer* dbBox::getTechLayer() const
{
  const _dbBox* box = (const _dbBox*) this;
  return (dbTechLayer*) box->getTechLayer();
}

uint dbBox::getLayerMask() const
{
  const _dbBox* box = (const _dbBox*) this;
  return box->flags_.layer_mask;
}

void dbBox::setLayerMask(const uint mask)
{
  _dbBox* box = (_dbBox*) this;
  box->checkMask(mask);

  if (box->flags_.layer_id == 0 && mask != 0) {
    getImpl()->getLogger()->error(
        utl::ODB, 435, "Mask must be 0 when no layer is provided.");
  }

  box->flags_.layer_mask = mask;
}

dbBox* dbBox::create(dbBPin* bpin_,
                     dbTechLayer* layer_,
                     int x1,
                     int y1,
                     int x2,
                     int y2,
                     uint mask)
{
  _dbBPin* bpin = (_dbBPin*) bpin_;
  _dbBlock* block = (_dbBlock*) bpin->getOwner();

  _dbBox* box = block->_box_tbl->create();
  box->flags_.octilinear = false;
  const auto layer_id = layer_->getImpl()->getOID();
  if (layer_id >= (1 << 9)) {
    bpin->getLogger()->error(
        utl::ODB,
        430,
        "Layer {} has index {} which is too large to be stored",
        layer_->getName(),
        layer_id);
  }
  box->flags_.layer_id = layer_id;
  box->flags_.owner_type = dbBoxOwner::BPIN;
  box->owner_ = bpin->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  dbBox* dbbox = (dbBox*) box;
  dbbox->setLayerMask(mask);

  box->next_box_ = bpin->_boxes;
  bpin->_boxes = box->getOID();

  block->add_rect(box->shape_.rect);
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
  box->flags_.octilinear = false;
  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->flags_.owner_type = dbBoxOwner::VIA;
  box->owner_ = via->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // update via bbox
  if (via->_bbox == 0) {
    _dbBox* vbbox = block->_box_tbl->create();
    vbbox->flags_.owner_type = dbBoxOwner::VIA;
    vbbox->owner_ = via->getOID();
    vbbox->shape_.rect.init(x1, y1, x2, y2);
    via->_bbox = vbbox->getOID();
  } else {
    _dbBox* vbbox = block->_box_tbl->getPtr(via->_bbox);
    vbbox->shape_.rect.merge(box->shape_.rect);
  }

  // Update the top-bottom layer of this via
  if (via->_top == 0) {
    via->_top = box->flags_.layer_id;
    via->_bottom = box->flags_.layer_id;
  } else {
    _dbTechLayer* layer = (_dbTechLayer*) layer_;
    _dbTech* tech = (_dbTech*) layer->getOwner();
    _dbTechLayer* top = tech->_layer_tbl->getPtr(via->_top);
    _dbTechLayer* bottom = tech->_layer_tbl->getPtr(via->_bottom);

    if (layer->_number > top->_number) {
      via->_top = layer->getOID();
    }

    if (layer->_number < bottom->_number) {
      via->_bottom = layer->getOID();
    }
  }

  // link box to via
  box->next_box_ = via->_boxes;
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
  box->flags_.octilinear = false;
  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->flags_.owner_type = dbBoxOwner::MASTER;
  box->owner_ = master->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // link box to master
  box->next_box_ = master->_obstructions;
  master->_obstructions = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMaster* master_, dbTechVia* via_, int x, int y)
{
  _dbMaster* master = (_dbMaster*) master_;
  _dbTechVia* via = (_dbTechVia*) via_;

  if (via->_bbox == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->shape_.rect.xMin() + x;
  int ymin = vbbox->shape_.rect.yMin() + y;
  int xmax = vbbox->shape_.rect.xMax() + x;
  int ymax = vbbox->shape_.rect.yMax() + y;
  _dbBox* box = master->_box_tbl->create();
  box->flags_.octilinear = false;
  box->flags_.owner_type = dbBoxOwner::MASTER;
  box->owner_ = master->getOID();
  box->shape_.rect.init(xmin, ymin, xmax, ymax);
  box->flags_.is_tech_via = 1;
  box->flags_.via_id = via->getOID();

  // link box to master
  box->next_box_ = master->_obstructions;
  master->_obstructions = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbPolygon* pbox, int x1, int y1, int x2, int y2)
{
  _dbPolygon* pbox_ = (_dbPolygon*) pbox;
  _dbMaster* master = (_dbMaster*) pbox_->getOwner();
  _dbBox* box = master->_box_tbl->create();
  box->flags_.octilinear = false;
  box->flags_.layer_id = pbox_->flags_.layer_id_;
  box->flags_.owner_type = dbBoxOwner::PBOX;
  box->owner_ = pbox_->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // link box to pin
  box->next_box_ = pbox_->boxes_;
  pbox_->boxes_ = box->getOID();
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
  box->flags_.octilinear = false;
  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->flags_.owner_type = dbBoxOwner::MPIN;
  box->owner_ = pin->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // link box to pin
  box->next_box_ = pin->_geoms;
  pin->_geoms = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbMPin* pin_, dbTechVia* via_, int x, int y)
{
  _dbMPin* pin = (_dbMPin*) pin_;
  _dbTechVia* via = (_dbTechVia*) via_;

  if (via->_bbox == 0) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) pin->getOwner();
  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
  int xmin = vbbox->shape_.rect.xMin() + x;
  int ymin = vbbox->shape_.rect.yMin() + y;
  int xmax = vbbox->shape_.rect.xMax() + x;
  int ymax = vbbox->shape_.rect.yMax() + y;
  _dbBox* box = master->_box_tbl->create();
  box->flags_.octilinear = false;
  box->flags_.owner_type = dbBoxOwner::MPIN;
  box->owner_ = pin->getOID();
  box->shape_.rect.init(xmin, ymin, xmax, ymax);
  box->flags_.is_tech_via = 1;
  box->flags_.via_id = via->getOID();

  // link box to pin
  box->next_box_ = pin->_geoms;
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
  box->flags_.octilinear = false;
  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->flags_.owner_type = dbBoxOwner::TECH_VIA;
  box->owner_ = via->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // update via bbox
  if (via->_bbox == 0) {
    _dbBox* vbbox = tech->_box_tbl->create();
    // 10302012D via group POWER Extraction
    // vbbox->flags_.is_tech_via = 1;
    vbbox->flags_.owner_type = dbBoxOwner::TECH_VIA;
    vbbox->owner_ = via->getOID();
    vbbox->shape_.rect.init(x1, y1, x2, y2);
    via->_bbox = vbbox->getOID();
  } else {
    _dbBox* vbbox = tech->_box_tbl->getPtr(via->_bbox);
    vbbox->shape_.rect.merge(box->shape_.rect);
  }

  // Update the top-bottom layer of this via
  if (via->_top == 0) {
    via->_top = box->flags_.layer_id;
    via->_bottom = box->flags_.layer_id;
  } else {
    _dbTechLayer* layer = (_dbTechLayer*) layer_;
    _dbTechLayer* top = tech->_layer_tbl->getPtr(via->_top);
    _dbTechLayer* bottom = tech->_layer_tbl->getPtr(via->_bottom);

    if (layer->_number > top->_number) {
      via->_top = layer_->getImpl()->getOID();
    }

    if (layer->_number < bottom->_number) {
      via->_bottom = layer_->getImpl()->getOID();
    }
  }

  // link box to via
  box->next_box_ = via->_boxes;
  via->_boxes = box->getOID();
  return (dbBox*) box;
}

dbBox* dbBox::create(dbRegion* region_, int x1, int y1, int x2, int y2)
{
  _dbRegion* region = (_dbRegion*) region_;
  _dbBlock* block = (_dbBlock*) region->getOwner();
  _dbBox* box = block->_box_tbl->create();
  box->flags_.octilinear = false;
  box->flags_.owner_type = dbBoxOwner::REGION;
  box->owner_ = region->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);

  // link box to region
  box->next_box_ = region->_boxes;
  region->_boxes = box->getOID();
  for (auto callback : block->_callbacks) {
    callback->inDbRegionAddBox(region_, (dbBox*) box);
  }
  return (dbBox*) box;
}

dbBox* dbBox::create(dbInst* inst_, int x1, int y1, int x2, int y2)
{
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (inst->_halo) {
    return nullptr;
  }

  _dbBox* box = block->_box_tbl->create();
  box->flags_.octilinear = false;
  box->flags_.owner_type = dbBoxOwner::INST;
  box->owner_ = inst->getOID();
  box->shape_.rect.init(x1, y1, x2, y2);
  inst->_halo = box->getOID();
  return (dbBox*) box;
}

void dbBox::destroy(dbBox* box)
{
  _dbBox* db_box = (_dbBox*) box;
  switch (db_box->flags_.owner_type) {
    case dbBoxOwner::BPIN: {
      _dbBPin* pin = (_dbBPin*) box->getBoxOwner();
      pin->removeBox(db_box);
      _dbBlock* block = (_dbBlock*) pin->getOwner();
      block->remove_rect(db_box->shape_.rect);
      block->_box_tbl->destroy(db_box);
      break;
    }
    case dbBoxOwner::UNKNOWN:
    case dbBoxOwner::BLOCK:
    case dbBoxOwner::INST:
    case dbBoxOwner::BTERM:
    case dbBoxOwner::VIA:
    case dbBoxOwner::OBSTRUCTION:
    case dbBoxOwner::SWIRE:
    case dbBoxOwner::BLOCKAGE:
    case dbBoxOwner::MASTER:
    case dbBoxOwner::MPIN:
    case dbBoxOwner::TECH_VIA:
    case dbBoxOwner::REGION:
    case dbBoxOwner::PBOX:
      return;
  }
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

bool dbBox::isVisited() const
{
  const _dbBox* box = (const _dbBox*) this;
  return box->flags_.visited == 1;
}
void dbBox::setVisited(const bool value)
{
  _dbBox* box = (_dbBox*) this;
  box->flags_.visited = (value == true) ? 1 : 0;
}

void _dbBox::collectMemInfo(MemInfo& info) const
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbBox::_dbBox(_dbDatabase*)
{
  flags_.owner_type = dbBoxOwner::UNKNOWN;
  flags_.is_tech_via = 0;
  flags_.is_block_via = 0;
  flags_.layer_id = 0;
  flags_.layer_mask = 0;
  flags_.via_id = 0;
  flags_.visited = 0;
  flags_.octilinear = false;
  owner_ = 0;
  design_rule_width_ = -1;
}

_dbBox::_dbBox(_dbDatabase*, const _dbBox& b)
    : flags_(b.flags_),
      owner_(b.owner_),
      next_box_(b.next_box_),
      design_rule_width_(b.design_rule_width_)
{
  if (b.isOct()) {
    new (&shape_.oct) Oct();
    shape_.oct = b.shape_.oct;
  } else {
    new (&shape_.rect) Rect();
    shape_.rect = b.shape_.rect;
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbBox& box)
{
  uint* bit_field = (uint*) &box.flags_;
  stream << *bit_field;
  if (box.isOct()) {
    stream << box.shape_.oct;
  } else {
    stream << box.shape_.rect;
  }
  stream << box.owner_;
  stream << box.next_box_;
  stream << box.design_rule_width_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBox& box)
{
  if (box.getDatabase()->isSchema(db_schema_dbbox_mask)) {
    uint* bit_field = (uint*) &box.flags_;
    stream >> *bit_field;
  } else if (box.getDatabase()->isSchema(db_schema_box_layer_bits)) {
    _dbBoxFlagsWithoutMask old;
    uint* bit_field = (uint*) &old;
    stream >> *bit_field;
    box.flags_.owner_type = old.owner_type;
    box.flags_.visited = old.visited;
    box.flags_.octilinear = old.octilinear;
    box.flags_.is_tech_via = old.is_tech_via;
    box.flags_.is_block_via = old.is_block_via;
    box.flags_.layer_id = old.layer_id;
    box.flags_.via_id = old.via_id;
    box.flags_.layer_mask = 0;
  } else {
    _dbBoxFlagsBackwardCompatability old;
    uint* bit_field = (uint*) &old;
    stream >> *bit_field;
    box.flags_.owner_type = old.owner_type;
    box.flags_.visited = old.visited;
    box.flags_.octilinear = old.octilinear;
    box.flags_.is_tech_via = old.is_tech_via;
    box.flags_.is_block_via = old.is_block_via;
    box.flags_.layer_id = old.layer_id;
    box.flags_.via_id = old.via_id;
    box.flags_.layer_mask = 0;
  }

  if (box.isOct()) {
    new (&box.shape_.oct) Oct();
    stream >> box.shape_.oct;
  } else {
    stream >> box.shape_.rect;
  }
  stream >> box.owner_;
  stream >> box.next_box_;
  stream >> box.design_rule_width_;
  return stream;
}

}  // namespace odb
