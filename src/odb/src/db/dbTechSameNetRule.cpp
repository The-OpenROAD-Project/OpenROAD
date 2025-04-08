// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechSameNetRule.h"

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbTechSameNetRule>;

bool _dbTechSameNetRule::operator==(const _dbTechSameNetRule& rhs) const
{
  if (_flags._stack != rhs._flags._stack) {
    return false;
  }

  if (_spacing != rhs._spacing) {
    return false;
  }

  if (_layer_1 != rhs._layer_1) {
    return false;
  }

  if (_layer_2 != rhs._layer_2) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbTechSameNetRule - Methods
//
////////////////////////////////////////////////////////////////////

dbTechLayer* dbTechSameNetRule::getLayer1()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  _dbTech* tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer_1);
}

dbTechLayer* dbTechSameNetRule::getLayer2()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  _dbTech* tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer_2);
}

int dbTechSameNetRule::getSpacing()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->_spacing;
}

void dbTechSameNetRule::setSpacing(int spacing)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  rule->_spacing = spacing;
}

void dbTechSameNetRule::setAllowStackedVias(bool value)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;

  if (value) {
    rule->_flags._stack = 1;
  } else {
    rule->_flags._stack = 0;
  }
}

bool dbTechSameNetRule::getAllowStackedVias()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->_flags._stack == 1;
}

dbTechSameNetRule* dbTechSameNetRule::create(dbTechLayer* layer1_,
                                             dbTechLayer* layer2_)
{
  _dbTechLayer* layer1 = (_dbTechLayer*) layer1_;
  _dbTechLayer* layer2 = (_dbTechLayer*) layer2_;
  dbTech* tech_ = (dbTech*) layer1->getOwner();
  _dbTech* tech = (_dbTech*) tech_;
  assert(tech_ == (dbTech*) layer2->getOwner());

  if (tech->_samenet_rules.empty()) {
    tech->_samenet_matrix.resize(tech->_layer_cnt, tech->_layer_cnt);

  } else if (tech_->findSameNetRule(layer1_, layer2_)) {
    return nullptr;
  }

  _dbTechSameNetRule* rule = tech->_samenet_rule_tbl->create();
  rule->_layer_1 = layer1->getOID();
  rule->_layer_2 = layer2->getOID();
  tech->_samenet_matrix(layer1->_number, layer2->_number) = rule->getOID();
  tech->_samenet_matrix(layer2->_number, layer1->_number) = rule->getOID();
  tech->_samenet_rules.push_back(rule->getOID());
  return (dbTechSameNetRule*) rule;
}

dbTechSameNetRule* dbTechSameNetRule::create(dbTechNonDefaultRule* ndrule_,
                                             dbTechLayer* layer1_,
                                             dbTechLayer* layer2_)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) ndrule_;
  _dbTechLayer* layer1 = (_dbTechLayer*) layer1_;
  _dbTechLayer* layer2 = (_dbTechLayer*) layer2_;
  dbTech* tech_ = (dbTech*) layer1->getOwner();
  _dbTech* tech = (_dbTech*) tech_;
  assert(tech_ == (dbTech*) layer2->getOwner());
  assert(tech_ == (dbTech*) ndrule->getOwner());

  if (ndrule->_samenet_rules.empty()) {
    ndrule->_samenet_matrix.resize(tech->_layer_cnt, tech->_layer_cnt);

  } else if (ndrule_->findSameNetRule(layer1_, layer2_)) {
    return nullptr;
  }

  _dbTechSameNetRule* rule = tech->_samenet_rule_tbl->create();
  rule->_layer_1 = layer1->getOID();
  rule->_layer_2 = layer2->getOID();
  ndrule->_samenet_matrix(layer1->_number, layer2->_number) = rule->getOID();
  ndrule->_samenet_matrix(layer2->_number, layer1->_number) = rule->getOID();
  ndrule->_samenet_rules.push_back(rule->getOID());
  return (dbTechSameNetRule*) rule;
}

dbTechSameNetRule* dbTechSameNetRule::getTechSameNetRule(dbTech* tech_,
                                                         uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(dbid_);
}

void _dbTechSameNetRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
