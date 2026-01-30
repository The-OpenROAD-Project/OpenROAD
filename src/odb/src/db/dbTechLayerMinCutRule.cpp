// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerMinCutRule.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerMinCutRule>;

bool _dbTechLayerMinCutRule::operator==(const _dbTechLayerMinCutRule& rhs) const
{
  if (flags_.per_cut_class != rhs.flags_.per_cut_class) {
    return false;
  }
  if (flags_.within_cut_dist_valid != rhs.flags_.within_cut_dist_valid) {
    return false;
  }
  if (flags_.from_above != rhs.flags_.from_above) {
    return false;
  }
  if (flags_.from_below != rhs.flags_.from_below) {
    return false;
  }
  if (flags_.length_valid != rhs.flags_.length_valid) {
    return false;
  }
  if (flags_.area_valid != rhs.flags_.area_valid) {
    return false;
  }
  if (flags_.area_within_dist_valid != rhs.flags_.area_within_dist_valid) {
    return false;
  }
  if (flags_.same_metal_overlap != rhs.flags_.same_metal_overlap) {
    return false;
  }
  if (flags_.fully_enclosed != rhs.flags_.fully_enclosed) {
    return false;
  }
  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (within_cut_dist != rhs.within_cut_dist) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (length_within_dist_ != rhs.length_within_dist_) {
    return false;
  }
  if (area_ != rhs.area_) {
    return false;
  }
  if (area_within_dist_ != rhs.area_within_dist_) {
    return false;
  }

  return true;
}

bool _dbTechLayerMinCutRule::operator<(const _dbTechLayerMinCutRule& rhs) const
{
  return true;
}

_dbTechLayerMinCutRule::_dbTechLayerMinCutRule(_dbDatabase* db)
{
  flags_ = {};
  num_cuts_ = 0;
  width_ = 0;
  within_cut_dist = 0;
  length_ = 0;
  length_within_dist_ = 0;
  area_ = 0;
  area_within_dist_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinCutRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.num_cuts_;
  stream >> obj.cut_class_cuts_map_;
  stream >> obj.width_;
  stream >> obj.within_cut_dist;
  stream >> obj.length_;
  stream >> obj.length_within_dist_;
  stream >> obj.area_;
  stream >> obj.area_within_dist_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinCutRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.num_cuts_;
  stream << obj.cut_class_cuts_map_;
  stream << obj.width_;
  stream << obj.within_cut_dist;
  stream << obj.length_;
  stream << obj.length_within_dist_;
  stream << obj.area_;
  stream << obj.area_within_dist_;
  return stream;
}

void _dbTechLayerMinCutRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["cut_class_cuts_map"].add(cut_class_cuts_map_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMinCutRule::setNumCuts(int num_cuts)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->num_cuts_ = num_cuts;
}

int dbTechLayerMinCutRule::getNumCuts() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->num_cuts_;
}

std::map<std::string, int> dbTechLayerMinCutRule::getCutClassCutsMap() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->cut_class_cuts_map_;
}

void dbTechLayerMinCutRule::setWidth(int width)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->width_ = width;
}

int dbTechLayerMinCutRule::getWidth() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->width_;
}

void dbTechLayerMinCutRule::setWithinCutDist(int within_cut_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->within_cut_dist = within_cut_dist;
}

int dbTechLayerMinCutRule::getWithinCutDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->within_cut_dist;
}

void dbTechLayerMinCutRule::setLength(int length)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->length_ = length;
}

int dbTechLayerMinCutRule::getLength() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->length_;
}

void dbTechLayerMinCutRule::setLengthWithinDist(int length_within_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->length_within_dist_ = length_within_dist;
}

int dbTechLayerMinCutRule::getLengthWithinDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->length_within_dist_;
}

void dbTechLayerMinCutRule::setArea(int area)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->area_ = area;
}

int dbTechLayerMinCutRule::getArea() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->area_;
}

void dbTechLayerMinCutRule::setAreaWithinDist(int area_within_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->area_within_dist_ = area_within_dist;
}

int dbTechLayerMinCutRule::getAreaWithinDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->area_within_dist_;
}

void dbTechLayerMinCutRule::setPerCutClass(bool per_cut_class)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.per_cut_class = per_cut_class;
}

bool dbTechLayerMinCutRule::isPerCutClass() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.per_cut_class;
}

void dbTechLayerMinCutRule::setWithinCutDistValid(bool within_cut_dist_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.within_cut_dist_valid = within_cut_dist_valid;
}

bool dbTechLayerMinCutRule::isWithinCutDistValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.within_cut_dist_valid;
}

void dbTechLayerMinCutRule::setFromAbove(bool from_above)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.from_above = from_above;
}

bool dbTechLayerMinCutRule::isFromAbove() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.from_above;
}

void dbTechLayerMinCutRule::setFromBelow(bool from_below)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.from_below = from_below;
}

bool dbTechLayerMinCutRule::isFromBelow() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.from_below;
}

void dbTechLayerMinCutRule::setLengthValid(bool length_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.length_valid = length_valid;
}

bool dbTechLayerMinCutRule::isLengthValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.length_valid;
}

void dbTechLayerMinCutRule::setAreaValid(bool area_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.area_valid = area_valid;
}

bool dbTechLayerMinCutRule::isAreaValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.area_valid;
}

void dbTechLayerMinCutRule::setAreaWithinDistValid(bool area_within_dist_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.area_within_dist_valid = area_within_dist_valid;
}

bool dbTechLayerMinCutRule::isAreaWithinDistValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.area_within_dist_valid;
}

void dbTechLayerMinCutRule::setSameMetalOverlap(bool same_metal_overlap)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.same_metal_overlap = same_metal_overlap;
}

bool dbTechLayerMinCutRule::isSameMetalOverlap() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.same_metal_overlap;
}

void dbTechLayerMinCutRule::setFullyEnclosed(bool fully_enclosed)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.fully_enclosed = fully_enclosed;
}

bool dbTechLayerMinCutRule::isFullyEnclosed() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.fully_enclosed;
}

// User Code Begin dbTechLayerMinCutRulePublicMethods

void dbTechLayerMinCutRule::setCutsPerCutClass(const std::string& cut_class,
                                               int num_cuts)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  obj->cut_class_cuts_map_[cut_class] = num_cuts;
}

dbTechLayerMinCutRule* dbTechLayerMinCutRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerMinCutRule* newrule = layer->min_cuts_rules_tbl_->create();
  return ((dbTechLayerMinCutRule*) newrule);
}

dbTechLayerMinCutRule* dbTechLayerMinCutRule::getTechLayerMinCutRule(
    dbTechLayer* inly,
    uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return ((dbTechLayerMinCutRule*) layer->min_cuts_rules_tbl_->getPtr(dbid));
}

void dbTechLayerMinCutRule::destroy(dbTechLayerMinCutRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->min_cuts_rules_tbl_->destroy((_dbTechLayerMinCutRule*) rule);
}

// User Code End dbTechLayerMinCutRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
