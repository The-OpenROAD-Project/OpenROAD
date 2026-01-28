// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaGenerateRule.h"

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

template class dbTable<_dbTechViaGenerateRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaGenerateRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaGenerateRule::operator==(const _dbTechViaGenerateRule& rhs) const
{
  if (flags_.default_via != rhs.flags_.default_via) {
    return false;
  }

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

  return true;
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*,
                                               const _dbTechViaGenerateRule& v)
    : flags_(v.flags_), name_(nullptr), layer_rules_(v.layer_rules_)
{
  if (v.name_) {
    name_ = safe_strdup(v.name_);
  }
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*)
{
  name_ = nullptr;
  flags_.default_via = 0;
  flags_.spare_bits = 0;
}

_dbTechViaGenerateRule::~_dbTechViaGenerateRule()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream << *bit_field;
  stream << v.name_;
  stream << v.layer_rules_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream >> *bit_field;
  stream >> v.name_;
  stream >> v.layer_rules_;
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
  return rule->flags_.default_via == 1;
}

uint32_t dbTechViaGenerateRule::getViaLayerRuleCount()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->layer_rules_.size();
}

dbTechViaLayerRule* dbTechViaGenerateRule::getViaLayerRule(uint32_t idx)
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  dbTech* tech = (dbTech*) rule->getOwner();

  if (idx >= rule->layer_rules_.size()) {
    return nullptr;
  }

  dbId<dbTechViaLayerRule> id = rule->layer_rules_[idx];
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
  _dbTechViaGenerateRule* rule = tech->via_generate_rule_tbl_->create();
  rule->name_ = safe_strdup(name);
  rule->flags_.default_via = is_default;
  return (dbTechViaGenerateRule*) rule;
}

dbTechViaGenerateRule* dbTechViaGenerateRule::getTechViaGenerateRule(
    dbTech* tech_,
    uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaGenerateRule*) tech->via_generate_rule_tbl_->getPtr(dbid_);
}

void _dbTechViaGenerateRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["layer_rules"].add(layer_rules_);
}

}  // namespace odb
