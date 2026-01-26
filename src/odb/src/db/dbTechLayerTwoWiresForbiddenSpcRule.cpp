// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerTwoWiresForbiddenSpcRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerTwoWiresForbiddenSpcRule>;

bool _dbTechLayerTwoWiresForbiddenSpcRule::operator==(
    const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
{
  if (flags_.min_exact_span_length != rhs.flags_.min_exact_span_length) {
    return false;
  }
  if (flags_.max_exact_span_length != rhs.flags_.max_exact_span_length) {
    return false;
  }
  if (min_spacing_ != rhs.min_spacing_) {
    return false;
  }
  if (max_spacing_ != rhs.max_spacing_) {
    return false;
  }
  if (min_span_length_ != rhs.min_span_length_) {
    return false;
  }
  if (max_span_length_ != rhs.max_span_length_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }

  return true;
}

bool _dbTechLayerTwoWiresForbiddenSpcRule::operator<(
    const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
{
  return true;
}

_dbTechLayerTwoWiresForbiddenSpcRule::_dbTechLayerTwoWiresForbiddenSpcRule(
    _dbDatabase* db)
{
  flags_ = {};
  min_spacing_ = 0;
  max_spacing_ = 0;
  min_span_length_ = 0;
  max_span_length_ = 0;
  prl_ = 0;
}

dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerTwoWiresForbiddenSpcRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.min_spacing_;
  stream >> obj.max_spacing_;
  stream >> obj.min_span_length_;
  stream >> obj.max_span_length_;
  stream >> obj.prl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerTwoWiresForbiddenSpcRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.min_spacing_;
  stream << obj.max_spacing_;
  stream << obj.min_span_length_;
  stream << obj.max_span_length_;
  stream << obj.prl_;
  return stream;
}

void _dbTechLayerTwoWiresForbiddenSpcRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerTwoWiresForbiddenSpcRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerTwoWiresForbiddenSpcRule::setMinSpacing(int min_spacing)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->min_spacing_ = min_spacing;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMinSpacing() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->min_spacing_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpacing(int max_spacing)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->max_spacing_ = max_spacing;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMaxSpacing() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->max_spacing_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMinSpanLength(int min_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->min_span_length_ = min_span_length;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMinSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->min_span_length_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpanLength(int max_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->max_span_length_ = max_span_length;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMaxSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->max_span_length_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setPrl(int prl)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->prl_ = prl;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getPrl() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->prl_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMinExactSpanLength(
    bool min_exact_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->flags_.min_exact_span_length = min_exact_span_length;
}

bool dbTechLayerTwoWiresForbiddenSpcRule::isMinExactSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  return obj->flags_.min_exact_span_length;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxExactSpanLength(
    bool max_exact_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->flags_.max_exact_span_length = max_exact_span_length;
}

bool dbTechLayerTwoWiresForbiddenSpcRule::isMaxExactSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  return obj->flags_.max_exact_span_length;
}

// User Code Begin dbTechLayerTwoWiresForbiddenSpcRulePublicMethods
dbTechLayerTwoWiresForbiddenSpcRule*
dbTechLayerTwoWiresForbiddenSpcRule::create(dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerTwoWiresForbiddenSpcRule* newrule
      = layer->two_wires_forbidden_spc_rules_tbl_->create();
  return ((dbTechLayerTwoWiresForbiddenSpcRule*) newrule);
}

void dbTechLayerTwoWiresForbiddenSpcRule::destroy(
    dbTechLayerTwoWiresForbiddenSpcRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->two_wires_forbidden_spc_rules_tbl_->destroy(
      (_dbTechLayerTwoWiresForbiddenSpcRule*) rule);
}
// User Code End dbTechLayerTwoWiresForbiddenSpcRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
