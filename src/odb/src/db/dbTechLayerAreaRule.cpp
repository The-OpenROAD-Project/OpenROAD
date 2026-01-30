// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerAreaRule.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerAreaRule>;

bool _dbTechLayerAreaRule::operator==(const _dbTechLayerAreaRule& rhs) const
{
  if (flags_.except_rectangle != rhs.flags_.except_rectangle) {
    return false;
  }
  if (flags_.overlap != rhs.flags_.overlap) {
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

_dbTechLayerAreaRule::_dbTechLayerAreaRule(_dbDatabase* db)
{
  flags_ = {};
  area_ = 0;
  except_min_width_ = 0;
  except_edge_length_ = 0;
  mask_ = 0;
  rect_width_ = 0;
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

void _dbTechLayerAreaRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
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

  obj->flags_.except_rectangle = except_rectangle;
}

bool dbTechLayerAreaRule::isExceptRectangle() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  return obj->flags_.except_rectangle;
}

void dbTechLayerAreaRule::setOverlap(uint32_t overlap)
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  obj->flags_.overlap = overlap;
}

uint32_t dbTechLayerAreaRule::getOverlap() const
{
  _dbTechLayerAreaRule* obj = (_dbTechLayerAreaRule*) this;

  return obj->flags_.overlap;
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
  newrule->flags_.except_rectangle = false;
  newrule->flags_.overlap = 0;
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
