// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerWrongDirSpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerWrongDirSpacingRule>;

bool _dbTechLayerWrongDirSpacingRule::operator==(
    const _dbTechLayerWrongDirSpacingRule& rhs) const
{
  if (flags_.noneol_valid != rhs.flags_.noneol_valid) {
    return false;
  }
  if (flags_.length_valid != rhs.flags_.length_valid) {
    return false;
  }
  if (wrongdir_space_ != rhs.wrongdir_space_) {
    return false;
  }
  if (noneol_width_ != rhs.noneol_width_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (prl_length_ != rhs.prl_length_) {
    return false;
  }

  return true;
}

bool _dbTechLayerWrongDirSpacingRule::operator<(
    const _dbTechLayerWrongDirSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerWrongDirSpacingRule::_dbTechLayerWrongDirSpacingRule(
    _dbDatabase* db)
{
  flags_ = {};
  wrongdir_space_ = 0;
  noneol_width_ = 0;
  length_ = 0;
  prl_length_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerWrongDirSpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.wrongdir_space_;
  stream >> obj.noneol_width_;
  stream >> obj.length_;
  stream >> obj.prl_length_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerWrongDirSpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.wrongdir_space_;
  stream << obj.noneol_width_;
  stream << obj.length_;
  stream << obj.prl_length_;
  return stream;
}

void _dbTechLayerWrongDirSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerWrongDirSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerWrongDirSpacingRule::setWrongdirSpace(int wrongdir_space)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->wrongdir_space_ = wrongdir_space;
}

int dbTechLayerWrongDirSpacingRule::getWrongdirSpace() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->wrongdir_space_;
}

void dbTechLayerWrongDirSpacingRule::setNoneolWidth(int noneol_width)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->noneol_width_ = noneol_width;
}

int dbTechLayerWrongDirSpacingRule::getNoneolWidth() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->noneol_width_;
}

void dbTechLayerWrongDirSpacingRule::setLength(int length)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->length_ = length;
}

int dbTechLayerWrongDirSpacingRule::getLength() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->length_;
}

void dbTechLayerWrongDirSpacingRule::setPrlLength(int prl_length)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->prl_length_ = prl_length;
}

int dbTechLayerWrongDirSpacingRule::getPrlLength() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->prl_length_;
}

void dbTechLayerWrongDirSpacingRule::setNoneolValid(bool noneol_valid)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->flags_.noneol_valid = noneol_valid;
}

bool dbTechLayerWrongDirSpacingRule::isNoneolValid() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  return obj->flags_.noneol_valid;
}

void dbTechLayerWrongDirSpacingRule::setLengthValid(bool length_valid)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->flags_.length_valid = length_valid;
}

bool dbTechLayerWrongDirSpacingRule::isLengthValid() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  return obj->flags_.length_valid;
}

// User Code Begin dbTechLayerWrongDirSpacingRulePublicMethods
dbTechLayerWrongDirSpacingRule* dbTechLayerWrongDirSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerWrongDirSpacingRule* newrule
      = layer->wrongdir_spacing_rules_tbl_->create();
  newrule->layer_ = _layer->getImpl()->getOID();

  return ((dbTechLayerWrongDirSpacingRule*) newrule);
}

dbTechLayerWrongDirSpacingRule*
dbTechLayerWrongDirSpacingRule::getTechLayerWrongDirSpacingRule(
    dbTechLayer* inly,
    uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerWrongDirSpacingRule*)
      layer->wrongdir_spacing_rules_tbl_->getPtr(dbid);
}

void dbTechLayerWrongDirSpacingRule::destroy(
    dbTechLayerWrongDirSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->wrongdir_spacing_rules_tbl_->destroy(
      (_dbTechLayerWrongDirSpacingRule*) rule);
}
// User Code End dbTechLayerWrongDirSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
