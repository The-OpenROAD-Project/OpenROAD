// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerVoltageSpacing.h"

#include <cstdint>
#include <cstring>
#include <map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerVoltageSpacing>;

bool _dbTechLayerVoltageSpacing::operator==(
    const _dbTechLayerVoltageSpacing& rhs) const
{
  if (flags_.tocut_above != rhs.flags_.tocut_above) {
    return false;
  }
  if (flags_.tocut_below != rhs.flags_.tocut_below) {
    return false;
  }

  // User Code Begin ==
  if (table_ != rhs.table_) {
    return false;
  }

  // User Code End ==
  return true;
}

bool _dbTechLayerVoltageSpacing::operator<(
    const _dbTechLayerVoltageSpacing& rhs) const
{
  // User Code Begin <
  if (flags_.tocut_above != rhs.flags_.tocut_above) {
    return flags_.tocut_above < rhs.flags_.tocut_above;
  }
  if (flags_.tocut_below != rhs.flags_.tocut_below) {
    return flags_.tocut_below < rhs.flags_.tocut_below;
  }
  if (table_ != rhs.table_) {
    return table_ < rhs.table_;
  }

  // User Code End <
  return true;
}

_dbTechLayerVoltageSpacing::_dbTechLayerVoltageSpacing(_dbDatabase* db)
{
  flags_ = {};
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerVoltageSpacing& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.table_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerVoltageSpacing& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.table_;
  return stream;
}

void _dbTechLayerVoltageSpacing::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerVoltageSpacing - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerVoltageSpacing::setTocutAbove(bool tocut_above)
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;

  obj->flags_.tocut_above = tocut_above;
}

bool dbTechLayerVoltageSpacing::isTocutAbove() const
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;

  return obj->flags_.tocut_above;
}

void dbTechLayerVoltageSpacing::setTocutBelow(bool tocut_below)
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;

  obj->flags_.tocut_below = tocut_below;
}

bool dbTechLayerVoltageSpacing::isTocutBelow() const
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;

  return obj->flags_.tocut_below;
}

// User Code Begin dbTechLayerVoltageSpacingPublicMethods
void dbTechLayerVoltageSpacing::addEntry(float voltage, int spacing)
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;
  obj->table_[voltage] = spacing;
}

const std::map<float, int>& dbTechLayerVoltageSpacing::getTable() const
{
  _dbTechLayerVoltageSpacing* obj = (_dbTechLayerVoltageSpacing*) this;
  return obj->table_;
}

dbTechLayerVoltageSpacing* dbTechLayerVoltageSpacing::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerVoltageSpacing* newrule
      = layer->voltage_spacing_rules_tbl_->create();
  return ((dbTechLayerVoltageSpacing*) newrule);
}

void dbTechLayerVoltageSpacing::destroy(dbTechLayerVoltageSpacing* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->voltage_spacing_rules_tbl_->destroy(
      (_dbTechLayerVoltageSpacing*) rule);
}
// User Code End dbTechLayerVoltageSpacingPublicMethods
}  // namespace odb
// Generator Code End Cpp