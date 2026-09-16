// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutEnclosureTableDefRule.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <tuple>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerCutEnclosureTableDefRule>;

bool _dbTechLayerCutEnclosureTableDefRule::operator==(
    const _dbTechLayerCutEnclosureTableDefRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.cut_class_valid != rhs.flags_.cut_class_valid) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }

  // User Code Begin ==
  if (default_rows_ != rhs.default_rows_) {
    return false;
  }
  if (width_rows_ != rhs.width_rows_) {
    return false;
  }
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerCutEnclosureTableDefRule::operator<(
    const _dbTechLayerCutEnclosureTableDefRule& rhs) const
{
  return false;
}

_dbTechLayerCutEnclosureTableDefRule::_dbTechLayerCutEnclosureTableDefRule(
    _dbDatabase* db)
{
  flags_ = {};
}

dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutEnclosureTableDefRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.cut_class_;
  stream >> obj.default_rows_;
  stream >> obj.width_rows_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureTableDefRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.cut_class_;
  stream << obj.default_rows_;
  stream << obj.width_rows_;
  return stream;
}

void _dbTechLayerCutEnclosureTableDefRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["default_rows"].add(default_rows_);
  info.children["width_rows"].add(width_rows_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutEnclosureTableDefRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutEnclosureTableDefRule::setCutClass(
    dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

dbTechLayerCutClassRule* dbTechLayerCutEnclosureTableDefRule::getCutClass()
    const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  if (obj->cut_class_ == 0) {
    return nullptr;
  }
  _dbTechLayer* par = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) par->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

void dbTechLayerCutEnclosureTableDefRule::setCutClassValid(bool cut_class_valid)
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;

  obj->flags_.cut_class_valid = cut_class_valid;
}

bool dbTechLayerCutEnclosureTableDefRule::isCutClassValid() const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;

  return obj->flags_.cut_class_valid;
}

dbTechLayerCutEnclosureTableDefRule*
dbTechLayerCutEnclosureTableDefRule::create(dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerCutEnclosureTableDefRule*)
      _parent->cut_enc_table_rules_tbl_->create();
}
void dbTechLayerCutEnclosureTableDefRule::destroy(
    dbTechLayerCutEnclosureTableDefRule* obj)
{
  _dbTechLayer* _parent = (_dbTechLayer*) obj->getImpl()->getOwner();
  dbProperty::destroyProperties(obj);
  _parent->cut_enc_table_rules_tbl_->destroy(
      (_dbTechLayerCutEnclosureTableDefRule*) obj);
}
// User Code Begin dbTechLayerCutEnclosureTableDefRulePublicMethods
void dbTechLayerCutEnclosureTableDefRule::addDefaultRow(const DefaultRow& row)
{
  assert(row.above_below >= 0 && row.above_below <= 2);
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  obj->default_rows_.push_back({row.above_below,
                                row.overhang1,
                                row.overhang2,
                                row.overhang3,
                                row.overhang4});
}

void dbTechLayerCutEnclosureTableDefRule::addWidthRow(const WidthRow& row)
{
  assert(row.above_below >= 0 && row.above_below <= 2);
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  obj->width_rows_.push_back({row.width,
                              row.above_below,
                              row.overhang1,
                              row.overhang2,
                              row.overhang3,
                              row.overhang4});
}

void dbTechLayerCutEnclosureTableDefRule::getDefaultRows(
    std::vector<DefaultRow>& rows) const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  rows.clear();
  rows.reserve(obj->default_rows_.size());
  for (const auto& [above_below, oh1, oh2, oh3, oh4] : obj->default_rows_) {
    rows.push_back({above_below, oh1, oh2, oh3, oh4});
  }
}

void dbTechLayerCutEnclosureTableDefRule::getWidthRows(
    std::vector<WidthRow>& rows) const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  rows.clear();
  rows.reserve(obj->width_rows_.size());
  for (const auto& [width, above_below, oh1, oh2, oh3, oh4] :
       obj->width_rows_) {
    rows.push_back({width, above_below, oh1, oh2, oh3, oh4});
  }
}
// User Code End dbTechLayerCutEnclosureTableDefRulePublicMethods
}  // namespace odb
// Generator Code End Cpp