// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaRule.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
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
  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (layer_rules_ != rhs.layer_rules_) {
    return false;
  }

  if (vias_ != rhs.vias_) {
    return false;
  }

  return true;
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*, const _dbTechViaRule& v)
    : flags_(v.flags_),
      name_(nullptr),
      layer_rules_(v.layer_rules_),
      vias_(v.vias_)
{
  if (v.name_) {
    name_ = safe_strdup(v.name_);
  }
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*)
{
  name_ = nullptr;
  flags_.spare_bits = 0;
}

_dbTechViaRule::~_dbTechViaRule()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream << *bit_field;
  stream << v.name_;
  stream << v.layer_rules_;
  stream << v.vias_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream >> *bit_field;
  stream >> v.name_;
  stream >> v.layer_rules_;
  stream >> v.vias_;
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
  return via->name_;
}

void dbTechViaRule::addVia(dbTechVia* via)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  rule->vias_.push_back(via->getImpl()->getOID());
}

uint32_t dbTechViaRule::getViaCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->vias_.size();
}

dbTechVia* dbTechViaRule::getVia(uint32_t idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->vias_.size()) {
    return nullptr;
  }

  dbId<dbTechVia> id = rule->vias_[idx];
  return dbTechVia::getTechVia(tech, id);
}

uint32_t dbTechViaRule::getViaLayerRuleCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->layer_rules_.size();
}

dbTechViaLayerRule* dbTechViaRule::getViaLayerRule(uint32_t idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->layer_rules_.size()) {
    return nullptr;
  }

  dbId<dbTechViaLayerRule> id = rule->layer_rules_[idx];
  return dbTechViaLayerRule::getTechViaLayerRule(tech, id);
}

dbTechViaRule* dbTechViaRule::create(dbTech* tech_, const char* name)
{
  if (tech_->findViaRule(name)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechViaRule* rule = tech->via_rule_tbl_->create();
  rule->name_ = safe_strdup(name);
  return (dbTechViaRule*) rule;
}

dbTechViaRule* dbTechViaRule::getTechViaRule(dbTech* tech_, uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaRule*) tech->via_rule_tbl_->getPtr(dbid_);
}

void _dbTechViaRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["layer_rules"].add(layer_rules_);
  info.children["vias"].add(vias_);
}

}  // namespace odb
