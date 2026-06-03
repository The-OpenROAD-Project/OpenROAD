// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerEolExtensionRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <utility>
#include <vector>
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerEolExtensionRule>;

bool _dbTechLayerEolExtensionRule::operator==(
    const _dbTechLayerEolExtensionRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.parallel_only != rhs.flags_.parallel_only) {
    return false;
  }
  if (spacing_ != rhs.spacing_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
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

  info.children["extension_tbl"].add(extension_tbl_);
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

dbTechLayerEolExtensionRule* dbTechLayerEolExtensionRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerEolExtensionRule*) _parent->eol_ext_rules_tbl_->create();
}
void dbTechLayerEolExtensionRule::destroy(dbTechLayerEolExtensionRule* obj)
{
  _dbTechLayer* _parent = (_dbTechLayer*) obj->getImpl()->getOwner();
  dbProperty::destroyProperties(obj);
  _parent->eol_ext_rules_tbl_->destroy((_dbTechLayerEolExtensionRule*) obj);
}
// User Code Begin dbTechLayerEolExtensionRulePublicMethods

void dbTechLayerEolExtensionRule::addEntry(int eol, int ext)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  obj->extension_tbl_.push_back({eol, ext});
}
dbTechLayerEolExtensionRule*
dbTechLayerEolExtensionRule::getTechLayerEolExtensionRule(dbTechLayer* inly,
                                                          uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerEolExtensionRule*) layer->eol_ext_rules_tbl_->getPtr(dbid);
}
// User Code End dbTechLayerEolExtensionRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
