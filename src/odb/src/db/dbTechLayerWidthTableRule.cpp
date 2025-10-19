// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerWidthTableRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerWidthTableRule>;

bool _dbTechLayerWidthTableRule::operator==(
    const _dbTechLayerWidthTableRule& rhs) const
{
  if (flags_.wrong_direction_ != rhs.flags_.wrong_direction_) {
    return false;
  }
  if (flags_.orthogonal_ != rhs.flags_.orthogonal_) {
    return false;
  }

  return true;
}

bool _dbTechLayerWidthTableRule::operator<(
    const _dbTechLayerWidthTableRule& rhs) const
{
  return true;
}

_dbTechLayerWidthTableRule::_dbTechLayerWidthTableRule(_dbDatabase* db)
{
  flags_ = {};
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerWidthTableRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.width_tbl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerWidthTableRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.width_tbl_;
  return stream;
}

void _dbTechLayerWidthTableRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["width_tbl"].add(width_tbl_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerWidthTableRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerWidthTableRule::setWrongDirection(bool wrong_direction)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  obj->flags_.wrong_direction_ = wrong_direction;
}

bool dbTechLayerWidthTableRule::isWrongDirection() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  return obj->flags_.wrong_direction_;
}

void dbTechLayerWidthTableRule::setOrthogonal(bool orthogonal)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  obj->flags_.orthogonal_ = orthogonal;
}

bool dbTechLayerWidthTableRule::isOrthogonal() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  return obj->flags_.orthogonal_;
}

// User Code Begin dbTechLayerWidthTableRulePublicMethods

void dbTechLayerWidthTableRule::addWidth(int width)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;
  obj->width_tbl_.push_back(width);
}

std::vector<int> dbTechLayerWidthTableRule::getWidthTable() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;
  return obj->width_tbl_;
}

dbTechLayerWidthTableRule* dbTechLayerWidthTableRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerWidthTableRule* newrule = layer->width_table_rules_tbl_->create();
  return ((dbTechLayerWidthTableRule*) newrule);
}

dbTechLayerWidthTableRule*
dbTechLayerWidthTableRule::getTechLayerWidthTableRule(dbTechLayer* inly,
                                                      uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (
      (dbTechLayerWidthTableRule*) layer->width_table_rules_tbl_->getPtr(dbid));
}

void dbTechLayerWidthTableRule::destroy(dbTechLayerWidthTableRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  if (rule->isWrongDirection()) {
    // reset wrong way width
    layer->wrong_way_width_ = layer->_width;
  }
  dbProperty::destroyProperties(rule);
  layer->width_table_rules_tbl_->destroy((_dbTechLayerWidthTableRule*) rule);
}

// User Code End dbTechLayerWidthTableRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
