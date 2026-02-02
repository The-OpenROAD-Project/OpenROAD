// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSBox.h"

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbDatabase.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbSBox>;

bool _dbSBox::operator==(const _dbSBox& rhs) const
{
  if (sflags_.wire_type != rhs.sflags_.wire_type) {
    return false;
  }

  if (sflags_.direction != rhs.sflags_.direction) {
    return false;
  }

  if (sflags_.via_bottom_mask != rhs.sflags_.via_bottom_mask) {
    return false;
  }

  if (sflags_.via_cut_mask != rhs.sflags_.via_cut_mask) {
    return false;
  }

  if (sflags_.via_top_mask != rhs.sflags_.via_top_mask) {
    return false;
  }

  if (_dbBox::operator!=(rhs)) {
    return false;
  }

  return true;
}

int _dbSBox::equal(const _dbSBox& rhs) const
{
  if (sflags_.wire_type != rhs.sflags_.wire_type) {
    return false;
  }

  if (sflags_.direction != rhs.sflags_.direction) {
    return false;
  }

  if (sflags_.via_bottom_mask != rhs.sflags_.via_bottom_mask) {
    return false;
  }

  if (sflags_.via_cut_mask != rhs.sflags_.via_cut_mask) {
    return false;
  }

  if (sflags_.via_top_mask != rhs.sflags_.via_top_mask) {
    return false;
  }

  return _dbBox::equal(rhs);
}

bool _dbSBox::operator<(const _dbSBox& rhs) const
{
  if (sflags_.wire_type < rhs.sflags_.wire_type) {
    return true;
  }

  if (sflags_.direction < rhs.sflags_.direction) {
    return true;
  }

  if (sflags_.via_bottom_mask < rhs.sflags_.via_bottom_mask) {
    return true;
  }

  if (sflags_.via_cut_mask < rhs.sflags_.via_cut_mask) {
    return true;
  }

  if (sflags_.via_top_mask < rhs.sflags_.via_top_mask) {
    return true;
  }

  if (sflags_.wire_type > rhs.sflags_.wire_type) {
    return false;
  }

  if (sflags_.direction > rhs.sflags_.direction) {
    return false;
  }

  if (sflags_.via_bottom_mask > rhs.sflags_.via_bottom_mask) {
    return false;
  }

  if (sflags_.via_cut_mask > rhs.sflags_.via_cut_mask) {
    return false;
  }

  if (sflags_.via_top_mask > rhs.sflags_.via_top_mask) {
    return false;
  }

  return _dbBox::operator<(rhs);
}

////////////////////////////////////////////////////////////////////
//
// dbSBox - Methods
//
////////////////////////////////////////////////////////////////////

dbWireShapeType dbSBox::getWireShapeType() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return dbWireShapeType(box->sflags_.wire_type);
}

dbSBox::Direction dbSBox::getDirection() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return (dbSBox::Direction) box->sflags_.direction;
}

dbSWire* dbSBox::getSWire() const
{
  return (dbSWire*) getBoxOwner();
}

Oct dbSBox::getOct() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return box->shape_.oct;
}

uint32_t dbSBox::getViaBottomLayerMask() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return box->sflags_.via_bottom_mask;
}

uint32_t dbSBox::getViaCutLayerMask() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return box->sflags_.via_cut_mask;
}

uint32_t dbSBox::getViaTopLayerMask() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return box->sflags_.via_top_mask;
}

bool dbSBox::hasViaLayerMasks() const
{
  const _dbSBox* box = (const _dbSBox*) this;
  return box->sflags_.via_bottom_mask != 0 || box->sflags_.via_cut_mask != 0
         || box->sflags_.via_top_mask != 0;
}

void dbSBox::setViaLayerMask(const uint32_t bottom,
                             const uint32_t cut,
                             const uint32_t top)
{
  _dbSBox* box = (_dbSBox*) this;
  box->checkMask(bottom);
  box->checkMask(cut);
  box->checkMask(top);

  box->sflags_.via_bottom_mask = bottom;
  box->sflags_.via_cut_mask = cut;
  box->sflags_.via_top_mask = top;
}

dbSBox* dbSBox::create(dbSWire* wire_,
                       dbTechLayer* layer_,
                       const int x1,
                       const int y1,
                       const int x2,
                       const int y2,
                       const dbWireShapeType type,
                       const Direction dir,
                       const int width)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbSBox* box = block->sbox_tbl_->create();

  const uint32_t dx = std::abs(x2 - x1);
  const uint32_t dy = std::abs(y2 - y1);

  switch (dir) {
    case UNDEFINED:
      if ((dx & 1) && (dy & 1)) {  // both odd
        return nullptr;
      }

      break;

    case HORIZONTAL:
      if (dy & 1) {  // dy odd
        return nullptr;
      }
      break;

    case VERTICAL:
      if (dx & 1) {  // dy odd
        return nullptr;
      }
      break;
    case OCTILINEAR:
      if (dx != dy) {
        return nullptr;
      }
      break;
  }

  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->flags_.owner_type = dbBoxOwner::SWIRE;
  if (dir == OCTILINEAR) {
    const Point p1(x1, y1);
    const Point p2(x2, y2);
    new (&box->shape_.oct) Oct();
    box->shape_.oct.init(p1, p2, width);
    box->flags_.octilinear = true;
    block->add_oct(box->shape_.oct);
  } else {
    box->shape_.rect.init(x1, y1, x2, y2);
    box->flags_.octilinear = false;
    block->add_rect(box->shape_.rect);
  }

  box->sflags_.wire_type = type.getValue();
  box->sflags_.direction = dir;

  wire->addSBox(box);

  return (dbSBox*) box;
}

dbSBox* dbSBox::create(dbSWire* wire_,
                       dbVia* via_,
                       const int x,
                       const int y,
                       const dbWireShapeType type)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbVia* via = (_dbVia*) via_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();

  if (via->bbox_ == 0) {
    return nullptr;
  }

  _dbBox* vbbox = block->box_tbl_->getPtr(via->bbox_);
  const int xmin = vbbox->shape_.rect.xMin() + x;
  const int ymin = vbbox->shape_.rect.yMin() + y;
  const int xmax = vbbox->shape_.rect.xMax() + x;
  const int ymax = vbbox->shape_.rect.yMax() + y;
  _dbSBox* box = block->sbox_tbl_->create();
  box->flags_.owner_type = dbBoxOwner::SWIRE;
  box->shape_.rect.init(xmin, ymin, xmax, ymax);
  box->flags_.is_block_via = 1;
  box->flags_.via_id = via->getOID();
  box->flags_.octilinear = false;
  box->sflags_.wire_type = type.getValue();

  wire->addSBox(box);

  block->add_rect(box->shape_.rect);
  return (dbSBox*) box;
}

dbSBox* dbSBox::create(dbSWire* wire_,
                       dbTechVia* via_,
                       const int x,
                       const int y,
                       const dbWireShapeType type)
{
  _dbSWire* wire = (_dbSWire*) wire_;
  _dbTechVia* via = (_dbTechVia*) via_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();

  if (via->bbox_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbBox* vbbox = tech->box_tbl_->getPtr(via->bbox_);
  const int xmin = vbbox->shape_.rect.xMin() + x;
  const int ymin = vbbox->shape_.rect.yMin() + y;
  const int xmax = vbbox->shape_.rect.xMax() + x;
  const int ymax = vbbox->shape_.rect.yMax() + y;
  _dbSBox* box = block->sbox_tbl_->create();
  box->flags_.owner_type = dbBoxOwner::SWIRE;
  box->shape_.rect.init(xmin, ymin, xmax, ymax);
  box->flags_.is_tech_via = 1;
  box->flags_.via_id = via->getOID();
  box->flags_.octilinear = false;
  box->sflags_.wire_type = type.getValue();

  wire->addSBox(box);

  block->add_rect(box->shape_.rect);
  return (dbSBox*) box;
}

dbSBox* dbSBox::getSBox(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbSBox*) block->sbox_tbl_->getPtr(dbid_);
}

void dbSBox::destroy(dbSBox* box_)
{
  _dbSWire* wire = (_dbSWire*) box_->getSWire();
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbSBox* box = (_dbSBox*) box_;

  wire->removeSBox(box);

  block->remove_rect(box->shape_.rect);
  dbProperty::destroyProperties(box);
  block->sbox_tbl_->destroy(box);
}

std::vector<dbSBox*> dbSBox::smashVia()
{
  if (!isVia()) {
    return {};
  }

  auto* block_via = getBlockVia();

  if (block_via == nullptr) {
    return {};
  }

  if (block_via->getTechVia() != nullptr) {
    return {};
  }

  auto params = block_via->getViaParams();

  if (params.getNumCutCols() == 1 && params.getNumCutRows() == 1) {
    // nothing to do
    return {};
  }

  const std::string name = block_via->getName() + "_smashed";

  _dbSWire* wire = (_dbSWire*) getSWire();
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbSWire* swire = (dbSWire*) wire;

  odb::dbVia* new_block_via = block->findVia(name.c_str());

  if (new_block_via == nullptr) {
    new_block_via = odb::dbVia::create(block, name.c_str());

    params.setNumCutCols(1);
    params.setNumCutRows(1);

    new_block_via->setViaParams(params);
  }

  std::vector<dbSBox*> new_boxes;

  std::vector<odb::dbShape> via_boxes;
  getViaBoxes(via_boxes);
  for (const auto& via_box : via_boxes) {
    auto* layer = via_box.getTechLayer();
    if (layer->getType() != odb::dbTechLayerType::CUT) {
      continue;
    }

    const auto& box = via_box.getBox();
    auto* sbox_via = odb::dbSBox::create(
        swire, new_block_via, box.xCenter(), box.yCenter(), getWireShapeType());
    new_boxes.push_back(sbox_via);

    if (hasViaLayerMasks()) {
      sbox_via->setViaLayerMask(
          getViaBottomLayerMask(), getViaCutLayerMask(), getViaTopLayerMask());
    }
  }

  return new_boxes;
}

_dbSBox::_dbSBox(_dbDatabase* db, const _dbSBox& b)
    : _dbBox(db, b), sflags_(b.sflags_)
{
}

_dbSBox::_dbSBox(_dbDatabase* db) : _dbBox(db)
{
  sflags_.wire_type = dbWireShapeType::COREWIRE;
  sflags_.direction = 0;
  sflags_.via_bottom_mask = 0;
  sflags_.via_cut_mask = 0;
  sflags_.via_top_mask = 0;
  sflags_.spare_bits = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbSBox& box)
{
  stream << (_dbBox&) box;
  uint32_t* bit_field = (uint32_t*) &box.sflags_;
  stream << *bit_field;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbSBox& box)
{
  stream >> (_dbBox&) box;
  uint32_t* bit_field = (uint32_t*) &box.sflags_;
  stream >> *bit_field;
  return stream;
}

}  // namespace odb
