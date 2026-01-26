// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbPolygon.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbBoxItr.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/poly_decomp.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbPolygon>;

bool _dbPolygon::operator==(const _dbPolygon& rhs) const
{
  if (flags_.owner_type != rhs.flags_.owner_type) {
    return false;
  }
  if (flags_.layer_id != rhs.flags_.layer_id) {
    return false;
  }
  if (polygon_ != rhs.polygon_) {
    return false;
  }
  if (design_rule_width_ != rhs.design_rule_width_) {
    return false;
  }
  if (owner_ != rhs.owner_) {
    return false;
  }
  if (next_pbox_ != rhs.next_pbox_) {
    return false;
  }
  if (boxes_ != rhs.boxes_) {
    return false;
  }

  return true;
}

bool _dbPolygon::operator<(const _dbPolygon& rhs) const
{
  return true;
}

_dbPolygon::_dbPolygon(_dbDatabase* db)
{
  flags_ = {};
  polygon_ = {};
  design_rule_width_ = 0;
  owner_ = 0;
  next_pbox_ = 0;
  boxes_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbPolygon& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.polygon_;
  stream >> obj.design_rule_width_;
  stream >> obj.owner_;
  stream >> obj.next_pbox_;
  stream >> obj.boxes_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbPolygon& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.polygon_;
  stream << obj.design_rule_width_;
  stream << obj.owner_;
  stream << obj.next_pbox_;
  stream << obj.boxes_;
  return stream;
}

void _dbPolygon::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["polygon"].add(polygon_.getPoints());
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbPolygon - Methods
//
////////////////////////////////////////////////////////////////////

Polygon dbPolygon::getPolygon() const
{
  _dbPolygon* obj = (_dbPolygon*) this;
  return obj->polygon_;
}

int dbPolygon::getDesignRuleWidth() const
{
  _dbPolygon* obj = (_dbPolygon*) this;
  return obj->design_rule_width_;
}

// User Code Begin dbPolygonPublicMethods
dbTechLayer* dbPolygon::getTechLayer()
{
  _dbPolygon* box = (_dbPolygon*) this;
  _dbMaster* master = (_dbMaster*) getImpl()->getOwner();
  _dbLib* lib = (_dbLib*) master->getOwner();
  _dbTech* tech = lib->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(box->flags_.layer_id);
}

dbPolygon* dbPolygon::create(dbMaster* master_,
                             dbTechLayer* layer_,
                             const std::vector<Point>& polygon)
{
  const Polygon poly = _dbPolygon::checkPolygon(polygon);
  if (poly.getPoints().empty()) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) master_;
  _dbPolygon* box = master->poly_box_tbl_->create();

  box->flags_.owner_type = dbBoxOwner::MASTER;
  box->owner_ = master->getOID();

  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->polygon_ = poly;

  // link box to master
  box->next_pbox_ = master->poly_obstructions_;
  master->poly_obstructions_ = box->getOID();

  // decompose polygon
  box->decompose();

  return (dbPolygon*) box;
}

dbPolygon* dbPolygon::create(dbMPin* pin_,
                             dbTechLayer* layer_,
                             const std::vector<Point>& polygon)
{
  const Polygon poly = _dbPolygon::checkPolygon(polygon);
  if (poly.getPoints().empty()) {
    return nullptr;
  }

  _dbMPin* pin = (_dbMPin*) pin_;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  _dbPolygon* box = master->poly_box_tbl_->create();

  box->flags_.owner_type = dbBoxOwner::MPIN;
  box->owner_ = pin->getOID();

  box->flags_.layer_id = layer_->getImpl()->getOID();
  box->polygon_ = poly;

  // link box to box
  box->next_pbox_ = pin->poly_geoms_;
  pin->poly_geoms_ = box->getOID();

  // decompose polygon
  box->decompose();

  return (dbPolygon*) box;
}

Polygon _dbPolygon::checkPolygon(const std::vector<Point>& polygon)
{
  if (polygon.size() < 4) {
    return {};
  }

  return polygon;
}

void _dbPolygon::decompose()
{
  std::vector<Rect> rects;
  decompose_polygon(polygon_.getPoints(), rects);

  dbPolygon* polygon = (dbPolygon*) this;
  for (Rect& r : rects) {
    dbBox::create(polygon, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  dbSet<dbBox> geoms = polygon->getGeometry();

  // Reverse the stored order to match the created order.
  if (geoms.reversible() && geoms.orderReversed()) {
    geoms.reverse();
  }
}

dbSet<dbBox> dbPolygon::getGeometry()
{
  _dbPolygon* pbox = (_dbPolygon*) this;
  _dbMaster* master = (_dbMaster*) pbox->getOwner();
  return dbSet<dbBox>(pbox, master->pbox_box_itr_);
}

void dbPolygon::setDesignRuleWidth(int design_rule_width)
{
  _dbPolygon* obj = (_dbPolygon*) this;

  obj->design_rule_width_ = design_rule_width;

  for (dbBox* box : getGeometry()) {
    box->setDesignRuleWidth(design_rule_width);
  }
}

// User Code End dbPolygonPublicMethods
}  // namespace odb
   // Generator Code End Cpp
