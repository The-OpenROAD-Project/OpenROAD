// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechNonDefaultRule.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dbBlock.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechLayerRule.h"
#include "dbTechSameNetRule.h"
#include "dbTechVia.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbTechNonDefaultRule>;

_dbTechNonDefaultRule::_dbTechNonDefaultRule(_dbDatabase*,
                                             const _dbTechNonDefaultRule& r)
    : flags_(r.flags_),
      name_(nullptr),
      layer_rules_(r.layer_rules_),
      vias_(r.vias_),
      samenet_rules_(r.samenet_rules_),
      samenet_matrix_(r.samenet_matrix_),
      use_vias_(r.use_vias_),
      use_rules_(r.use_rules_),
      cut_layers_(r.cut_layers_),
      min_cuts_(r.min_cuts_)
{
  if (r.name_) {
    name_ = safe_strdup(r.name_);
  }
}

_dbTechNonDefaultRule::_dbTechNonDefaultRule(_dbDatabase*)
{
  flags_.spare_bits = 0;
  flags_.hard_spacing = 0;
  flags_.block_rule = 0;
  name_ = nullptr;
}

_dbTechNonDefaultRule::~_dbTechNonDefaultRule()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechNonDefaultRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.name_;
  stream << rule.layer_rules_;
  stream << rule.vias_;
  stream << rule.samenet_rules_;
  stream << rule.samenet_matrix_;
  stream << rule.use_vias_;
  stream << rule.use_rules_;
  stream << rule.cut_layers_;
  stream << rule.min_cuts_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechNonDefaultRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.name_;
  stream >> rule.layer_rules_;
  stream >> rule.vias_;
  stream >> rule.samenet_rules_;
  stream >> rule.samenet_matrix_;
  stream >> rule.use_vias_;
  stream >> rule.use_rules_;
  stream >> rule.cut_layers_;
  stream >> rule.min_cuts_;

  return stream;
}

bool _dbTechNonDefaultRule::operator==(const _dbTechNonDefaultRule& rhs) const
{
  if (flags_.hard_spacing != rhs.flags_.hard_spacing) {
    return false;
  }

  if (flags_.block_rule != rhs.flags_.block_rule) {
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

  if (vias_ != rhs.vias_) {
    return false;
  }

  if (samenet_rules_ != rhs.samenet_rules_) {
    return false;
  }

  if (samenet_matrix_ != rhs.samenet_matrix_) {
    return false;
  }

  if (use_vias_ != rhs.use_vias_) {
    return false;
  }

  if (use_rules_ != rhs.use_rules_) {
    return false;
  }

  if (cut_layers_ != rhs.cut_layers_) {
    return false;
  }

  if (min_cuts_ != rhs.min_cuts_) {
    return false;
  }

  return true;
}

_dbTech* _dbTechNonDefaultRule::getTech()
{
  if (flags_.block_rule == 0) {
    return (_dbTech*) getOwner();
  }

  return getBlock()->getTech();
}

_dbBlock* _dbTechNonDefaultRule::getBlock()
{
  assert(flags_.block_rule == 1);
  return (_dbBlock*) getOwner();
}

void _dbTechNonDefaultRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["_layer_rules"].add(layer_rules_);
  info.children["_vias"].add(vias_);
  info.children["_samenet_rules"].add(samenet_rules_);
  info.children["_samenet_matrix"].add(samenet_matrix_);
  info.children["_use_vias"].add(use_vias_);
  info.children["_use_rules"].add(use_rules_);
  info.children["_cut_layers"].add(cut_layers_);
  info.children["_min_cuts"].add(min_cuts_);
}

bool _dbTechNonDefaultRule::operator<(const _dbTechNonDefaultRule& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

////////////////////////////////////////////////////////////////////
//
// dbTechNonDefaultRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechNonDefaultRule::getName()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->name_;
}

const char* dbTechNonDefaultRule::getConstName()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->name_;
}

bool dbTechNonDefaultRule::isBlockRule()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->flags_.block_rule == 1;
}

dbTechLayerRule* dbTechNonDefaultRule::getLayerRule(dbTechLayer* layer_)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) layer_;
  dbId<_dbTechLayerRule> id = rule->layer_rules_[layer->number_];

  if (id == 0) {
    return nullptr;
  }

  if (rule->flags_.block_rule == 0) {
    return (dbTechLayerRule*) rule->getTech()->layer_rule_tbl_->getPtr(id);
  }
  return (dbTechLayerRule*) rule->getBlock()->layer_rule_tbl_->getPtr(id);
}

void dbTechNonDefaultRule::getLayerRules(
    std::vector<dbTechLayerRule*>& layer_rules)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  layer_rules.clear();

  auto add_rules = [rule, &layer_rules](auto& tbl) {
    for (const auto& id : rule->layer_rules_) {
      if (id.isValid()) {
        layer_rules.push_back((dbTechLayerRule*) tbl->getPtr(id));
      }
    }
  };

  if (rule->flags_.block_rule == 0) {
    add_rules(rule->getTech()->layer_rule_tbl_);
  } else {
    add_rules(rule->getBlock()->layer_rule_tbl_);
  }
}

void dbTechNonDefaultRule::getVias(std::vector<dbTechVia*>& vias)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  vias.clear();

  if (rule->flags_.block_rule == 1) {  // not supported on block rules
    return;
  }

  _dbTech* tech = rule->getTech();

  for (const auto& id : rule->vias_) {
    vias.push_back((dbTechVia*) tech->via_tbl_->getPtr(id));
  }
}

dbTechSameNetRule* dbTechNonDefaultRule::findSameNetRule(dbTechLayer* l1_,
                                                         dbTechLayer* l2_)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) this;

  if (ndrule->flags_.block_rule == 1) {  // not supported on block rules
    return nullptr;
  }

  _dbTech* tech = ndrule->getTech();
  _dbTechLayer* l1 = (_dbTechLayer*) l1_;
  _dbTechLayer* l2 = (_dbTechLayer*) l2_;
  dbId<_dbTechSameNetRule> rule
      = ndrule->samenet_matrix_(l1->number_, l2->number_);

  if (rule == 0) {
    return nullptr;
  }

  return (dbTechSameNetRule*) tech->samenet_rule_tbl_->getPtr(rule);
}

void dbTechNonDefaultRule::getSameNetRules(
    std::vector<dbTechSameNetRule*>& rules)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) this;

  rules.clear();

  if (ndrule->flags_.block_rule == 1) {  // not supported on block rules
    return;
  }

  _dbTech* tech = ndrule->getTech();

  for (const auto& id : ndrule->samenet_rules_) {
    rules.push_back((dbTechSameNetRule*) tech->samenet_rule_tbl_->getPtr(id));
  }
}

bool dbTechNonDefaultRule::getHardSpacing()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->flags_.hard_spacing == 1;
}

void dbTechNonDefaultRule::setHardSpacing(bool value)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->flags_.hard_spacing = value;
}

void dbTechNonDefaultRule::addUseVia(dbTechVia* via)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->use_vias_.push_back(via->getId());
}

void dbTechNonDefaultRule::getUseVias(std::vector<dbTechVia*>& vias)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  _dbTech* tech = rule->getTech();

  for (const auto& vid : rule->use_vias_) {
    dbTechVia* via = dbTechVia::getTechVia((dbTech*) tech, vid);
    vias.push_back(via);
  }
}

void dbTechNonDefaultRule::addUseViaRule(dbTechViaGenerateRule* gen_rule)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->use_rules_.push_back(gen_rule->getId());
}

void dbTechNonDefaultRule::getUseViaRules(
    std::vector<dbTechViaGenerateRule*>& rules)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  _dbTech* tech = rule->getTech();

  for (const auto& rid : rule->use_rules_) {
    dbTechViaGenerateRule* rule
        = dbTechViaGenerateRule::getTechViaGenerateRule((dbTech*) tech, rid);
    rules.push_back(rule);
  }
}

void dbTechNonDefaultRule::setMinCuts(dbTechLayer* cut_layer, const int count)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  const uint32_t id = cut_layer->getId();
  uint32_t idx = 0;

  for (const auto& lid : rule->cut_layers_) {
    if (lid == id) {
      rule->min_cuts_[idx] = count;
      return;
    }
    ++idx;
  }

  rule->cut_layers_.push_back(id);
  rule->min_cuts_.push_back(count);
}

bool dbTechNonDefaultRule::getMinCuts(dbTechLayer* cut_layer, int& count)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  const uint32_t id = cut_layer->getId();
  uint32_t idx = 0;

  for (const auto& lid : rule->cut_layers_) {
    if (lid == id) {
      count = rule->min_cuts_[idx];
      return true;
    }
    ++idx;
  }

  return false;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::create(dbTech* tech_,
                                                   const char* name_)
{
  if (tech_->findNonDefaultRule(name_)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechNonDefaultRule* rule = tech->non_default_rule_tbl_->create();
  rule->name_ = safe_strdup(name_);
  rule->layer_rules_.resize(tech->layer_cnt_);

  int i;
  for (i = 0; i < tech->layer_cnt_; ++i) {
    rule->layer_rules_.push_back(0);
  }

  return (dbTechNonDefaultRule*) rule;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::create(dbBlock* block_,
                                                   const char* name_)
{
  if (block_->findNonDefaultRule(name_)) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) block_;
  _dbTech* tech = (_dbTech*) block->getDb()->getTech();
  _dbTechNonDefaultRule* rule = block->non_default_rule_tbl_->create();

  rule->name_ = safe_strdup(name_);
  rule->flags_.block_rule = 1;
  rule->layer_rules_.resize(tech->layer_cnt_);

  int i;
  for (i = 0; i < tech->layer_cnt_; ++i) {
    rule->layer_rules_.push_back(0);
  }

  return (dbTechNonDefaultRule*) rule;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::getTechNonDefaultRule(
    dbTech* tech_,
    uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechNonDefaultRule*) tech->non_default_rule_tbl_->getPtr(dbid_);
}

dbTechNonDefaultRule* dbTechNonDefaultRule::getTechNonDefaultRule(
    dbBlock* block_,
    uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTechNonDefaultRule*) block->non_default_rule_tbl_->getPtr(dbid_);
}

}  // namespace odb
