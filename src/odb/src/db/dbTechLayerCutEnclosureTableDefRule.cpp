// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutEnclosureTableDefRule.h"

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
// User Code Begin Includes
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutEnclosureTableDefRule>;
// User Code Begin Static
// User Code End Static

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
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerCutEnclosureTableDefRule::operator<(
    const _dbTechLayerCutEnclosureTableDefRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}

_dbTechLayerCutEnclosureTableDefRule::_dbTechLayerCutEnclosureTableDefRule(
    _dbDatabase* db)
{
  flags_ = {};
  // User Code Begin Constructor
  // User Code End Constructor
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
  // User Code Begin >>
  // User Code End >>
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
  // User Code Begin <<
  // User Code End <<
  return stream;
}

void _dbTechLayerCutEnclosureTableDefRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["default_rows"].add(default_rows_);
  info.children["width_rows"].add(width_rows_);

  // User Code Begin collectMemInfo
  // User Code End collectMemInfo
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

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
void dbTechLayerCutEnclosureTableDefRule::addDefaultRow(int aboveBelow,
                                                        int oh1,
                                                        int oh2,
                                                        int oh3,
                                                        int oh4)
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  obj->default_rows_.push_back({aboveBelow, oh1, oh2, oh3, oh4});
}

void dbTechLayerCutEnclosureTableDefRule::addWidthRow(int width,
                                                      int aboveBelow,
                                                      int oh1,
                                                      int oh2,
                                                      int oh3,
                                                      int oh4,
                                                      bool minSum)
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  obj->width_rows_.push_back({width, aboveBelow, oh1, oh2, oh3, oh4, minSum});
}

void dbTechLayerCutEnclosureTableDefRule::getDefaultRows(
    std::vector<std::tuple<int, int, int, int, int>>& rows) const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  rows.assign(obj->default_rows_.begin(), obj->default_rows_.end());
}

void dbTechLayerCutEnclosureTableDefRule::getWidthRows(
    std::vector<std::tuple<int, int, int, int, int, int, bool>>& rows) const
{
  _dbTechLayerCutEnclosureTableDefRule* obj
      = (_dbTechLayerCutEnclosureTableDefRule*) this;
  rows.assign(obj->width_rows_.begin(), obj->width_rows_.end());
}
// User Code End dbTechLayerCutEnclosureTableDefRulePublicMethods
}  // namespace odb
// Generator Code End Cpp