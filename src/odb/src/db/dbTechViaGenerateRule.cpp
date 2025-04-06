// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaGenerateRule.h"

#include <string>

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
  if (_flags._default != rhs._flags._default) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_layer_rules != rhs._layer_rules) {
    return false;
  }

  return true;
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*,
                                               const _dbTechViaGenerateRule& v)
    : _flags(v._flags), _name(nullptr), _layer_rules(v._layer_rules)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*)
{
  _name = nullptr;
  _flags._default = 0;
  _flags._spare_bits = 0;
}

_dbTechViaGenerateRule::~_dbTechViaGenerateRule()
{
  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._name;
  stream << v._layer_rules;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._name;
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
  return via->_name;
}

bool dbTechViaGenerateRule::isDefault()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->_flags._default == 1;
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
  rule->_name = strdup(name);
  ZALLOCATED(rule->_name);
  rule->_flags._default = is_default;
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

  info.children_["name"].add(_name);
  info.children_["layer_rules"].add(_layer_rules);
}

}  // namespace odb
