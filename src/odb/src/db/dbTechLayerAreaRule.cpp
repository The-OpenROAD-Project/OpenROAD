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

// Generator Code Begin Cpp
#include "dbTechLayerAreaRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerAreaRule>;

bool _dbTechLayerAreaRule::operator==(const _dbTechLayerAreaRule& rhs) const
{
  if (flags_.except_rectangle_ != rhs.flags_.except_rectangle_) {
    return false;
  }
  if (flags_.overlap_ != rhs.flags_.overlap_) {
    return false;
  }
  if (area_ != rhs.area_) {
    return false;
  }
  if (except_min_width_ != rhs.except_min_width_) {
    return false;
  }
  if (except_edge_length_ != rhs.except_edge_length_) {
    return false;
  }
  if (trim_layer_ != rhs.trim_layer_) {
    return false;
  }
  if (mask_ != rhs.mask_) {
    return false;
  }
  if (rect_width_ != rhs.rect_width_) {
    return false;
  }

  return true;
}

bool _dbTechLayerAreaRule::operator<(const _dbTechLayerAreaRule& rhs) const
{
  return true;
}

void _dbTechLayerAreaRule::differences(dbDiff& diff,
                                       const char* field,
                                       const _dbTechLayerAreaRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.except_rectangle_);
  DIFF_FIELD(flags_.overlap_);
  DIFF_FIELD(area_);
  DIFF_FIELD(except_min_width_);
  DIFF_FIELD(except_edge_length_);
  DIFF_FIELD(trim_layer_);
  DIFF_FIELD(mask_);
  DIFF_FIELD(rect_width_);
  DIFF_END
}

void _dbTechLayerAreaRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.except_rectangle_);
  DIFF_OUT_FIELD(flags_.overlap_);
  DIFF_OUT_FIELD(area_);
  DIFF_OUT_FIELD(except_min_width_);
  DIFF_OUT_FIELD(except_edge_length_);
  DIFF_OUT_FIELD(trim_layer_);
  DIFF_OUT_FIELD(mask_);
  DIFF_OUT_FIELD(rect_width_);

  DIFF_END
}

_dbTechLayerAreaRule::_dbTechLayerAreaRule(_dbDatabase* db)
{
  flags_ = {};
  area_ = 0;
  except_min_width_ = 0;
  except_edge_length_ = 0;
  mask_ = 0;
  rect_width_ = 0;
}

_dbTechLayerAreaRule::_dbTechLayerAreaRule(_dbDatabase* db,
                                           const _dbTechLayerAreaRule& r)
{
  flags_.except_rectangle_ = r.flags_.except_rectangle_;
  flags_.overlap_ = r.flags_.overlap_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  area_ = r.area_;
  except_min_width_ = r.except_min_width_;
  except_edge_length_ = r.except_edge_length_;
  trim_layer_ = r.trim_layer_;
  mask_ = r.mask_;
  rect_width_ = r.rect_width_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerAreaRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.area_;
  stream >> obj.except_min_width_;
  stream >> obj.except_edge_length_;
  stream >> obj.except_edge_lengths_;
  stream >> obj.except_min_size_;
  stream >> obj.except_step_;
  stream >> obj.trim_layer_;
  stream >> obj.mask_;
  stream >> obj.rect_width_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAreaRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.area_;
  stream << obj.except_min_width_;
  stream << obj.except_edge_length_;
  stream << obj.except_edge_lengths_;
  stream << obj.except_min_size_;
  stream << obj.except_step_;
  stream << obj.trim_layer_;
  stream << obj.mask_;
  stream << obj.rect_width_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerAreaRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerAreaRule::setArea(int area)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->area_ = area;
}

int dbTechLayerAreaRule::getArea() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->area_;
}

void dbTechLayerAreaRule::setExceptMinWidth(int except_min_width)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->except_min_width_ = except_min_width;
}

int dbTechLayerAreaRule::getExceptMinWidth() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->except_min_width_;
}

void dbTechLayerAreaRule::setExceptEdgeLength(int except_edge_length)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->except_edge_length_ = except_edge_length;
}

int dbTechLayerAreaRule::getExceptEdgeLength() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->except_edge_length_;
}

void dbTechLayerAreaRule::setExceptEdgeLengths(
    const std::pair<int, int>& except_edge_lengths)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->except_edge_lengths_ = except_edge_lengths;
}

std::pair<int, int> dbTechLayerAreaRule::getExceptEdgeLengths() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->except_edge_lengths_;
}

void dbTechLayerAreaRule::setExceptMinSize(
    const std::pair<int, int>& except_min_size)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->except_min_size_ = except_min_size;
}

std::pair<int, int> dbTechLayerAreaRule::getExceptMinSize() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->except_min_size_;
}

void dbTechLayerAreaRule::setExceptStep(const std::pair<int, int>& except_step)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->except_step_ = except_step;
}

std::pair<int, int> dbTechLayerAreaRule::getExceptStep() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->except_step_;
}

void dbTechLayerAreaRule::setMask(int mask)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->mask_ = mask;
}

int dbTechLayerAreaRule::getMask() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->mask_;
}

void dbTechLayerAreaRule::setRectWidth(int rect_width)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->rect_width_ = rect_width;
}

int dbTechLayerAreaRule::getRectWidth() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  return obj->rect_width_;
}

void dbTechLayerAreaRule::setExceptRectangle(bool except_rectangle)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->flags_.except_rectangle_ = except_rectangle;
}

bool dbTechLayerAreaRule::isExceptRectangle() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  return obj->flags_.except_rectangle_;
}

void dbTechLayerAreaRule::setOverlap(uint overlap)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->flags_.overlap_ = overlap;
}

uint dbTechLayerAreaRule::getOverlap() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  return obj->flags_.overlap_;
}

// User Code Begin dbTechLayerAreaRulePublicMethods

dbTechLayerAreaRule* dbTechLayerAreaRule::create(dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerAreaRule* newrule = layer->area_rules_tbl_->create();
  newrule->area_ = 0;
  newrule->except_min_width_ = 0;
  newrule->except_edge_length_ = 0;
  newrule->except_edge_lengths_ = std::pair<int, int>(0, 0);
  newrule->except_min_size_ = std::pair<int, int>(0, 0);
  newrule->except_step_ = std::pair<int, int>(0, 0);
  newrule->mask_ = 0;
  newrule->rect_width_ = 0;
  newrule->flags_.except_rectangle_ = false;
  newrule->flags_.overlap_ = 0;
  return ((dbTechLayerAreaRule*) newrule);
}

void dbTechLayerAreaRule::setTrimLayer(dbTechLayer* trim_layer)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->trim_layer_ = trim_layer->getImpl()->getOID();
}

dbTechLayer* dbTechLayerAreaRule::getTrimLayer() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;
  odb::dbTech* tech = getDb()->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->trim_layer_);
}

void dbTechLayerAreaRule::destroy(dbTechLayerAreaRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->area_rules_tbl_->destroy((_dbTechLayerAreaRule*) rule);
}

// User Code End dbTechLayerAreaRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
