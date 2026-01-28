// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerArraySpacingRule.h"

#include <cstdint>
#include <cstring>
#include <map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerArraySpacingRule>;

bool _dbTechLayerArraySpacingRule::operator==(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  if (flags_.parallel_overlap != rhs.flags_.parallel_overlap) {
    return false;
  }
  if (flags_.long_array != rhs.flags_.long_array) {
    return false;
  }
  if (flags_.via_width_valid != rhs.flags_.via_width_valid) {
    return false;
  }
  if (flags_.within_valid != rhs.flags_.within_valid) {
    return false;
  }
  if (via_width_ != rhs.via_width_) {
    return false;
  }
  if (cut_spacing_ != rhs.cut_spacing_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (array_width_ != rhs.array_width_) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }

  return true;
}

bool _dbTechLayerArraySpacingRule::operator<(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  return true;
}

_dbTechLayerArraySpacingRule::_dbTechLayerArraySpacingRule(_dbDatabase* db)
{
  flags_ = {};
  via_width_ = 0;
  cut_spacing_ = 0;
  within_ = 0;
  array_width_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.via_width_;
  stream >> obj.cut_spacing_;
  stream >> obj.within_;
  stream >> obj.array_width_;
  stream >> obj.array_spacing_map_;
  stream >> obj.cut_class_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.via_width_;
  stream << obj.cut_spacing_;
  stream << obj.within_;
  stream << obj.array_width_;
  stream << obj.array_spacing_map_;
  stream << obj.cut_class_;
  return stream;
}

void _dbTechLayerArraySpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["array_spacing_map"].add(array_spacing_map_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerArraySpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerArraySpacingRule::setViaWidth(int via_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->via_width_ = via_width;
}

int dbTechLayerArraySpacingRule::getViaWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->via_width_;
}

void dbTechLayerArraySpacingRule::setCutSpacing(int cut_spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_spacing_ = cut_spacing;
}

int dbTechLayerArraySpacingRule::getCutSpacing() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->cut_spacing_;
}

void dbTechLayerArraySpacingRule::setWithin(int within)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerArraySpacingRule::getWithin() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->within_;
}

void dbTechLayerArraySpacingRule::setArrayWidth(int array_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->array_width_ = array_width;
}

int dbTechLayerArraySpacingRule::getArrayWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_width_;
}

void dbTechLayerArraySpacingRule::setCutClass(
    dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

void dbTechLayerArraySpacingRule::setParallelOverlap(bool parallel_overlap)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.parallel_overlap = parallel_overlap;
}

bool dbTechLayerArraySpacingRule::isParallelOverlap() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.parallel_overlap;
}

void dbTechLayerArraySpacingRule::setLongArray(bool long_array)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.long_array = long_array;
}

bool dbTechLayerArraySpacingRule::isLongArray() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.long_array;
}

void dbTechLayerArraySpacingRule::setViaWidthValid(bool via_width_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.via_width_valid = via_width_valid;
}

bool dbTechLayerArraySpacingRule::isViaWidthValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.via_width_valid;
}

void dbTechLayerArraySpacingRule::setWithinValid(bool within_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.within_valid = within_valid;
}

bool dbTechLayerArraySpacingRule::isWithinValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.within_valid;
}

// User Code Begin dbTechLayerArraySpacingRulePublicMethods

void dbTechLayerArraySpacingRule::setCutsArraySpacing(int num_cuts, int spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  obj->array_spacing_map_[num_cuts] = spacing;
}

const std::map<int, int>& dbTechLayerArraySpacingRule::getCutsArraySpacing()
    const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_spacing_map_;
}

dbTechLayerCutClassRule* dbTechLayerArraySpacingRule::getCutClass() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  if (!obj->cut_class_.isValid()) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

dbTechLayerArraySpacingRule* dbTechLayerArraySpacingRule::create(
    dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerArraySpacingRule* newrule
      = layer->array_spacing_rules_tbl_->create();
  return ((dbTechLayerArraySpacingRule*) newrule);
}

dbTechLayerArraySpacingRule*
dbTechLayerArraySpacingRule::getTechLayerArraySpacingRule(dbTechLayer* inly,
                                                          uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return ((dbTechLayerArraySpacingRule*)
              layer->array_spacing_rules_tbl_->getPtr(dbid));
}

void dbTechLayerArraySpacingRule::destroy(dbTechLayerArraySpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->array_spacing_rules_tbl_->destroy(
      (_dbTechLayerArraySpacingRule*) rule);
}

// User Code End dbTechLayerArraySpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
