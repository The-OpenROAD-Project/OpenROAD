// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutClassRule.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbCommon.h"
#include "dbHashTable.hpp"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutClassRule>;

bool _dbTechLayerCutClassRule::operator==(
    const _dbTechLayerCutClassRule& rhs) const
{
  if (flags_.length_valid != rhs.flags_.length_valid) {
    return false;
  }
  if (flags_.cuts_valid != rhs.flags_.cuts_valid) {
    return false;
  }
  if (name_ != rhs.name_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  return true;
}

bool _dbTechLayerCutClassRule::operator<(
    const _dbTechLayerCutClassRule& rhs) const
{
  return true;
}

_dbTechLayerCutClassRule::_dbTechLayerCutClassRule(_dbDatabase* db)
{
  flags_ = {};
  name_ = nullptr;
  width_ = 0;
  length_ = 0;
  num_cuts_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutClassRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.name_;
  stream >> obj.width_;
  stream >> obj.length_;
  stream >> obj.num_cuts_;
  stream >> obj.next_entry_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutClassRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.name_;
  stream << obj.width_;
  stream << obj.length_;
  stream << obj.num_cuts_;
  stream << obj.next_entry_;
  return stream;
}

void _dbTechLayerCutClassRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  // User Code End collectMemInfo
}

_dbTechLayerCutClassRule::~_dbTechLayerCutClassRule()
{
  if (name_) {
    free((void*) name_);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutClassRule - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbTechLayerCutClassRule::getName() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->name_;
}

void dbTechLayerCutClassRule::setWidth(int width)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->width_ = width;
}

int dbTechLayerCutClassRule::getWidth() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->width_;
}

void dbTechLayerCutClassRule::setLength(int length)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->length_ = length;
}

int dbTechLayerCutClassRule::getLength() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->length_;
}

void dbTechLayerCutClassRule::setNumCuts(int num_cuts)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->num_cuts_ = num_cuts;
}

int dbTechLayerCutClassRule::getNumCuts() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->num_cuts_;
}

void dbTechLayerCutClassRule::setLengthValid(bool length_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->flags_.length_valid = length_valid;
}

bool dbTechLayerCutClassRule::isLengthValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->flags_.length_valid;
}

void dbTechLayerCutClassRule::setCutsValid(bool cuts_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->flags_.cuts_valid = cuts_valid;
}

bool dbTechLayerCutClassRule::isCutsValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->flags_.cuts_valid;
}

// User Code Begin dbTechLayerCutClassRulePublicMethods
dbTechLayerCutClassRule* dbTechLayerCutClassRule::create(dbTechLayer* _layer,
                                                         const char* name)
{
  if (_layer->findTechLayerCutClassRule(name) != nullptr) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCutClassRule* newrule = layer->cut_class_rules_tbl_->create();
  newrule->name_ = safe_strdup(name);
  layer->cut_class_rules_hash_.insert(newrule);
  return ((dbTechLayerCutClassRule*) newrule);
}

dbTechLayerCutClassRule* dbTechLayerCutClassRule::getTechLayerCutClassRule(
    dbTechLayer* inly,
    uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(dbid);
}
void dbTechLayerCutClassRule::destroy(dbTechLayerCutClassRule* rule)
{
  _dbTechLayerCutClassRule* _rule = (_dbTechLayerCutClassRule*) rule;
  _dbTechLayer* layer = (_dbTechLayer*) _rule->getOwner();
  layer->cut_class_rules_hash_.remove(_rule);
  dbProperty::destroyProperties(rule);
  layer->cut_class_rules_tbl_->destroy((_dbTechLayerCutClassRule*) rule);
}
// User Code End dbTechLayerCutClassRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
