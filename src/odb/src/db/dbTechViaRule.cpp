// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaRule.h"

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

template class dbTable<_dbTechViaRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaRule::operator==(const _dbTechViaRule& rhs) const
{
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

  if (_vias != rhs._vias) {
    return false;
  }

  return true;
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*, const _dbTechViaRule& v)
    : _flags(v._flags),
      _name(nullptr),
      _layer_rules(v._layer_rules),
      _vias(v._vias)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*)
{
  _name = nullptr;
  _flags._spare_bits = 0;
}

_dbTechViaRule::~_dbTechViaRule()
{
  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._name;
  stream << v._layer_rules;
  stream << v._vias;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._name;
  stream >> v._layer_rules;
  stream >> v._vias;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechViaRule::getName()
{
  _dbTechViaRule* via = (_dbTechViaRule*) this;
  return via->_name;
}

void dbTechViaRule::addVia(dbTechVia* via)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  rule->_vias.push_back(via->getImpl()->getOID());
}

uint dbTechViaRule::getViaCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->_vias.size();
}

dbTechVia* dbTechViaRule::getVia(uint idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_vias.size()) {
    return nullptr;
  }

  dbId<dbTechVia> id = rule->_vias[idx];
  return dbTechVia::getTechVia(tech, id);
}

uint dbTechViaRule::getViaLayerRuleCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->_layer_rules.size();
}

dbTechViaLayerRule* dbTechViaRule::getViaLayerRule(uint idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_layer_rules.size()) {
    return nullptr;
  }

  dbId<dbTechViaLayerRule> id = rule->_layer_rules[idx];
  return dbTechViaLayerRule::getTechViaLayerRule(tech, id);
}

dbTechViaRule* dbTechViaRule::create(dbTech* tech_, const char* name)
{
  if (tech_->findViaRule(name)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechViaRule* rule = tech->_via_rule_tbl->create();
  rule->_name = strdup(name);
  ZALLOCATED(rule->_name);
  return (dbTechViaRule*) rule;
}

dbTechViaRule* dbTechViaRule::getTechViaRule(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaRule*) tech->_via_rule_tbl->getPtr(dbid_);
}

void _dbTechViaRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
  info.children_["layer_rules"].add(_layer_rules);
  info.children_["vias"].add(_vias);
}

}  // namespace odb
