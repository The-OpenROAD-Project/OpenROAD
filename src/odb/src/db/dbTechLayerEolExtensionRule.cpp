// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerEolExtensionRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerEolExtensionRule>;

bool _dbTechLayerEolExtensionRule::operator==(
    const _dbTechLayerEolExtensionRule& rhs) const
{
  if (flags_.parallel_only != rhs.flags_.parallel_only) {
    return false;
  }
  if (spacing_ != rhs.spacing_) {
    return false;
  }

  return true;
}

bool _dbTechLayerEolExtensionRule::operator<(
    const _dbTechLayerEolExtensionRule& rhs) const
{
  // User Code Begin <
  if (spacing_ >= rhs.spacing_) {
    return false;
  }
  // User Code End <
  return true;
}

_dbTechLayerEolExtensionRule::_dbTechLayerEolExtensionRule(_dbDatabase* db)
{
  flags_ = {};
  spacing_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolExtensionRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.spacing_;
  stream >> obj.extension_tbl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerEolExtensionRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.spacing_;
  stream << obj.extension_tbl_;
  return stream;
}

void _dbTechLayerEolExtensionRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["extension_tbl"].add(extension_tbl_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerEolExtensionRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerEolExtensionRule::setSpacing(int spacing)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  obj->spacing_ = spacing;
}

int dbTechLayerEolExtensionRule::getSpacing() const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  return obj->spacing_;
}

void dbTechLayerEolExtensionRule::getExtensionTable(
    std::vector<std::pair<int, int>>& tbl) const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  tbl = obj->extension_tbl_;
}

void dbTechLayerEolExtensionRule::setParallelOnly(bool parallel_only)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  obj->flags_.parallel_only = parallel_only;
}

bool dbTechLayerEolExtensionRule::isParallelOnly() const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  return obj->flags_.parallel_only;
}

// User Code Begin dbTechLayerEolExtensionRulePublicMethods

void dbTechLayerEolExtensionRule::addEntry(int eol, int ext)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  obj->extension_tbl_.push_back({eol, ext});
}

dbTechLayerEolExtensionRule* dbTechLayerEolExtensionRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerEolExtensionRule* newrule = layer->eol_ext_rules_tbl_->create();
  return ((dbTechLayerEolExtensionRule*) newrule);
}

dbTechLayerEolExtensionRule*
dbTechLayerEolExtensionRule::getTechLayerEolExtensionRule(dbTechLayer* inly,
                                                          uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerEolExtensionRule*) layer->eol_ext_rules_tbl_->getPtr(dbid);
}
void dbTechLayerEolExtensionRule::destroy(dbTechLayerEolExtensionRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->eol_ext_rules_tbl_->destroy((_dbTechLayerEolExtensionRule*) rule);
}

// User Code End dbTechLayerEolExtensionRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
