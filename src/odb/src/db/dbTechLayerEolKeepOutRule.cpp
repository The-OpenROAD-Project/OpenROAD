// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerEolKeepOutRule.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerEolKeepOutRule>;

bool _dbTechLayerEolKeepOutRule::operator==(
    const _dbTechLayerEolKeepOutRule& rhs) const
{
  if (flags_.class_valid != rhs.flags_.class_valid) {
    return false;
  }
  if (flags_.corner_only != rhs.flags_.corner_only) {
    return false;
  }
  if (flags_.except_within != rhs.flags_.except_within) {
    return false;
  }
  if (eol_width_ != rhs.eol_width_) {
    return false;
  }
  if (backward_ext_ != rhs.backward_ext_) {
    return false;
  }
  if (forward_ext_ != rhs.forward_ext_) {
    return false;
  }
  if (side_ext_ != rhs.side_ext_) {
    return false;
  }
  if (within_low_ != rhs.within_low_) {
    return false;
  }
  if (within_high_ != rhs.within_high_) {
    return false;
  }
  if (class_name_ != rhs.class_name_) {
    return false;
  }

  return true;
}

bool _dbTechLayerEolKeepOutRule::operator<(
    const _dbTechLayerEolKeepOutRule& rhs) const
{
  return true;
}

_dbTechLayerEolKeepOutRule::_dbTechLayerEolKeepOutRule(_dbDatabase* db)
{
  flags_ = {};
  eol_width_ = 0;
  backward_ext_ = 0;
  forward_ext_ = 0;
  side_ext_ = 0;
  within_low_ = 0;
  within_high_ = 0;
  class_name_ = "";
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolKeepOutRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.eol_width_;
  stream >> obj.backward_ext_;
  stream >> obj.forward_ext_;
  stream >> obj.side_ext_;
  stream >> obj.within_low_;
  stream >> obj.within_high_;
  stream >> obj.class_name_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerEolKeepOutRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.eol_width_;
  stream << obj.backward_ext_;
  stream << obj.forward_ext_;
  stream << obj.side_ext_;
  stream << obj.within_low_;
  stream << obj.within_high_;
  stream << obj.class_name_;
  return stream;
}

void _dbTechLayerEolKeepOutRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["class_name"].add(class_name_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerEolKeepOutRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerEolKeepOutRule::setEolWidth(int eol_width)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerEolKeepOutRule::getEolWidth() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->eol_width_;
}

void dbTechLayerEolKeepOutRule::setBackwardExt(int backward_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->backward_ext_ = backward_ext;
}

int dbTechLayerEolKeepOutRule::getBackwardExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->backward_ext_;
}

void dbTechLayerEolKeepOutRule::setForwardExt(int forward_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->forward_ext_ = forward_ext;
}

int dbTechLayerEolKeepOutRule::getForwardExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->forward_ext_;
}

void dbTechLayerEolKeepOutRule::setSideExt(int side_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->side_ext_ = side_ext;
}

int dbTechLayerEolKeepOutRule::getSideExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->side_ext_;
}

void dbTechLayerEolKeepOutRule::setWithinLow(int within_low)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->within_low_ = within_low;
}

int dbTechLayerEolKeepOutRule::getWithinLow() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->within_low_;
}

void dbTechLayerEolKeepOutRule::setWithinHigh(int within_high)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->within_high_ = within_high;
}

int dbTechLayerEolKeepOutRule::getWithinHigh() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->within_high_;
}

void dbTechLayerEolKeepOutRule::setClassName(const std::string& class_name)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->class_name_ = class_name;
}

std::string dbTechLayerEolKeepOutRule::getClassName() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->class_name_;
}

void dbTechLayerEolKeepOutRule::setClassValid(bool class_valid)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->flags_.class_valid = class_valid;
}

bool dbTechLayerEolKeepOutRule::isClassValid() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  return obj->flags_.class_valid;
}

void dbTechLayerEolKeepOutRule::setCornerOnly(bool corner_only)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->flags_.corner_only = corner_only;
}

bool dbTechLayerEolKeepOutRule::isCornerOnly() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  return obj->flags_.corner_only;
}

void dbTechLayerEolKeepOutRule::setExceptWithin(bool except_within)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->flags_.except_within = except_within;
}

bool dbTechLayerEolKeepOutRule::isExceptWithin() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  return obj->flags_.except_within;
}

// User Code Begin dbTechLayerEolKeepOutRulePublicMethods

dbTechLayerEolKeepOutRule* dbTechLayerEolKeepOutRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerEolKeepOutRule* newrule
      = layer->eol_keep_out_rules_tbl_->create();
  return ((dbTechLayerEolKeepOutRule*) newrule);
}

dbTechLayerEolKeepOutRule*
dbTechLayerEolKeepOutRule::getTechLayerEolKeepOutRule(dbTechLayer* inly,
                                                      uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerEolKeepOutRule*) layer->eol_keep_out_rules_tbl_->getPtr(
      dbid);
}
void dbTechLayerEolKeepOutRule::destroy(dbTechLayerEolKeepOutRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->eol_keep_out_rules_tbl_->destroy((_dbTechLayerEolKeepOutRule*) rule);
}

// User Code End dbTechLayerEolKeepOutRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
