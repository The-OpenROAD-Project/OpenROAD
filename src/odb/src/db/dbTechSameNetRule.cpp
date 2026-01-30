// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechSameNetRule.h"

#include <cassert>
#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbTechSameNetRule>;

bool _dbTechSameNetRule::operator==(const _dbTechSameNetRule& rhs) const
{
  if (flags_.stack != rhs.flags_.stack) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  if (layer_1_ != rhs.layer_1_) {
    return false;
  }

  if (layer_2_ != rhs.layer_2_) {
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
  return (dbTechLayer*) tech->layer_tbl_->getPtr(rule->layer_1_);
}

dbTechLayer* dbTechSameNetRule::getLayer2()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  _dbTech* tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(rule->layer_2_);
}

int dbTechSameNetRule::getSpacing()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->spacing_;
}

void dbTechSameNetRule::setSpacing(int spacing)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  rule->spacing_ = spacing;
}

void dbTechSameNetRule::setAllowStackedVias(bool value)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;

  if (value) {
    rule->flags_.stack = 1;
  } else {
    rule->flags_.stack = 0;
  }
}

bool dbTechSameNetRule::getAllowStackedVias()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->flags_.stack == 1;
}

dbTechSameNetRule* dbTechSameNetRule::create(dbTechLayer* layer1_,
                                             dbTechLayer* layer2_)
{
  _dbTechLayer* layer1 = (_dbTechLayer*) layer1_;
  _dbTechLayer* layer2 = (_dbTechLayer*) layer2_;
  dbTech* tech_ = (dbTech*) layer1->getOwner();
  _dbTech* tech = (_dbTech*) tech_;
  assert(tech_ == (dbTech*) layer2->getOwner());

  if (tech->samenet_rules_.empty()) {
    tech->samenet_matrix_.resize(tech->layer_cnt_, tech->layer_cnt_);

  } else if (tech_->findSameNetRule(layer1_, layer2_)) {
    return nullptr;
  }

  _dbTechSameNetRule* rule = tech->samenet_rule_tbl_->create();
  rule->layer_1_ = layer1->getOID();
  rule->layer_2_ = layer2->getOID();
  tech->samenet_matrix_(layer1->number_, layer2->number_) = rule->getOID();
  tech->samenet_matrix_(layer2->number_, layer1->number_) = rule->getOID();
  tech->samenet_rules_.push_back(rule->getOID());
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

  if (ndrule->samenet_rules_.empty()) {
    ndrule->samenet_matrix_.resize(tech->layer_cnt_, tech->layer_cnt_);

  } else if (ndrule_->findSameNetRule(layer1_, layer2_)) {
    return nullptr;
  }

  _dbTechSameNetRule* rule = tech->samenet_rule_tbl_->create();
  rule->layer_1_ = layer1->getOID();
  rule->layer_2_ = layer2->getOID();
  ndrule->samenet_matrix_(layer1->number_, layer2->number_) = rule->getOID();
  ndrule->samenet_matrix_(layer2->number_, layer1->number_) = rule->getOID();
  ndrule->samenet_rules_.push_back(rule->getOID());
  return (dbTechSameNetRule*) rule;
}

dbTechSameNetRule* dbTechSameNetRule::getTechSameNetRule(dbTech* tech_,
                                                         uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechSameNetRule*) tech->samenet_rule_tbl_->getPtr(dbid_);
}

void _dbTechSameNetRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
