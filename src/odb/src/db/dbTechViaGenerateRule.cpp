// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaGenerateRule.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {

template class dbTable<_dbTechViaGenerateRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaGenerateRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaGenerateRule::operator==(const _dbTechViaGenerateRule& rhs) const
{
  if (flags_._default != rhs.flags_._default) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (_layer_rules != rhs._layer_rules) {
    return false;
  }

  return true;
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*,
                                               const _dbTechViaGenerateRule& v)
    : flags_(v.flags_), name_(nullptr), _layer_rules(v._layer_rules)
{
  if (v.name_) {
    name_ = safe_strdup(v.name_);
  }
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*)
{
  name_ = nullptr;
  flags_._default = 0;
  flags_._spare_bits = 0;
}

_dbTechViaGenerateRule::~_dbTechViaGenerateRule()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v.flags_;
  stream << *bit_field;
  stream << v.name_;
  stream << v._layer_rules;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v.flags_;
  stream >> *bit_field;
  stream >> v.name_;
  stream >> v._layer_rules;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaGenerateRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechViaGenerateRule::getName()
{
  _dbTechViaGenerateRule* via = (_dbTechViaGenerateRule*) this;
  return via->name_;
}

bool dbTechViaGenerateRule::isDefault()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->flags_._default == 1;
}

uint dbTechViaGenerateRule::getViaLayerRuleCount()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->_layer_rules.size();
}

dbTechViaLayerRule* dbTechViaGenerateRule::getViaLayerRule(uint idx)
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_layer_rules.size()) {
    return nullptr;
  }

  dbId<dbTechViaLayerRule> id = rule->_layer_rules[idx];
  return dbTechViaLayerRule::getTechViaLayerRule(tech, id);
}

dbTechViaGenerateRule* dbTechViaGenerateRule::create(dbTech* tech_,
                                                     const char* name,
                                                     bool is_default)
{
  if (tech_->findViaGenerateRule(name)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechViaGenerateRule* rule = tech->_via_generate_rule_tbl->create();
  rule->name_ = safe_strdup(name);
  rule->flags_._default = is_default;
  return (dbTechViaGenerateRule*) rule;
}

dbTechViaGenerateRule* dbTechViaGenerateRule::getTechViaGenerateRule(
    dbTech* tech_,
    uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaGenerateRule*) tech->_via_generate_rule_tbl->getPtr(dbid_);
}

void _dbTechViaGenerateRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(name_);
  info.children_["layer_rules"].add(_layer_rules);
}

}  // namespace odb
