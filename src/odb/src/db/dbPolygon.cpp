///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbPolygon.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
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
  if (flags_.owner_type_ != rhs.flags_.owner_type_) {
    return false;
  }
  if (flags_.layer_id_ != rhs.flags_.layer_id_) {
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

void _dbPolygon::differences(dbDiff& diff,
                             const char* field,
                             const _dbPolygon& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.owner_type_);
  DIFF_FIELD(flags_.layer_id_);
  DIFF_FIELD(polygon_);
  DIFF_FIELD(design_rule_width_);
  DIFF_FIELD(owner_);
  DIFF_FIELD(next_pbox_);
  DIFF_FIELD(boxes_);
  DIFF_END
}

void _dbPolygon::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.owner_type_);
  DIFF_OUT_FIELD(flags_.layer_id_);
  DIFF_OUT_FIELD(polygon_);
  DIFF_OUT_FIELD(design_rule_width_);
  DIFF_OUT_FIELD(owner_);
  DIFF_OUT_FIELD(next_pbox_);
  DIFF_OUT_FIELD(boxes_);

  DIFF_END
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

_dbPolygon::_dbPolygon(_dbDatabase* db, const _dbPolygon& r)
{
  flags_.owner_type_ = r.flags_.owner_type_;
  flags_.layer_id_ = r.flags_.layer_id_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  polygon_ = r.polygon_;
  design_rule_width_ = r.design_rule_width_;
  owner_ = r.owner_;
  next_pbox_ = r.next_pbox_;
  boxes_ = r.boxes_;
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
  return (dbTechLayer*) tech->_layer_tbl->getPtr(box->flags_.layer_id_);
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
  _dbPolygon* box = master->_poly_box_tbl->create();

  box->flags_.owner_type_ = dbBoxOwner::MASTER;
  box->owner_ = master->getOID();

  box->flags_.layer_id_ = layer_->getImpl()->getOID();
  box->polygon_ = poly;

  // link box to master
  box->next_pbox_ = master->_poly_obstructions;
  master->_poly_obstructions = box->getOID();

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
  _dbPolygon* box = master->_poly_box_tbl->create();

  box->flags_.owner_type_ = dbBoxOwner::MPIN;
  box->owner_ = pin->getOID();

  box->flags_.layer_id_ = layer_->getImpl()->getOID();
  box->polygon_ = poly;

  // link box to box
  box->next_pbox_ = pin->_poly_geoms;
  pin->_poly_geoms = box->getOID();

  // decompose polygon
  box->decompose();

  return (dbPolygon*) box;
}

Polygon _dbPolygon::checkPolygon(std::vector<Point> polygon)
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

  std::vector<Rect>::iterator itr;

  for (itr = rects.begin(); itr != rects.end(); ++itr) {
    Rect& r = *itr;

    dbBox::create((dbPolygon*) this, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  dbPolygon* pbox = (dbPolygon*) this;
  dbSet<dbBox> geoms = pbox->getGeometry();

  // Reverse the stored order to match the created order.
  if (geoms.reversible() && geoms.orderReversed()) {
    geoms.reverse();
  }
}

dbSet<dbBox> dbPolygon::getGeometry()
{
  _dbPolygon* pbox = (_dbPolygon*) this;
  _dbMaster* master = (_dbMaster*) pbox->getOwner();
  return dbSet<dbBox>(pbox, master->_pbox_box_itr);
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