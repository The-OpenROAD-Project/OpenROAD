// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerRule.h"

#include <cassert>
#include <cstdint>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbTechLayerRule>;

_dbTechLayerRule::_dbTechLayerRule(_dbDatabase*, const _dbTechLayerRule& r)
    : flags_(r.flags_),
      width_(r.width_),
      spacing_(r.spacing_),
      resistance_(r.resistance_),
      capacitance_(r.capacitance_),
      edge_capacitance_(r.edge_capacitance_),
      wire_extension_(r.wire_extension_),
      non_default_rule_(r.non_default_rule_),
      layer_(r.layer_)
{
}

_dbTechLayerRule::_dbTechLayerRule(_dbDatabase*)
{
  flags_.spare_bits = 0;
  width_ = 0;
  spacing_ = 0;
  resistance_ = 0.0;
  capacitance_ = 0.0;
  edge_capacitance_ = 0.0;
  wire_extension_ = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.width_;
  stream << rule.spacing_;
  stream << rule.resistance_;
  stream << rule.capacitance_;
  stream << rule.edge_capacitance_;
  stream << rule.wire_extension_;
  stream << rule.non_default_rule_;
  stream << rule.layer_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.width_;
  stream >> rule.spacing_;
  stream >> rule.resistance_;
  stream >> rule.capacitance_;
  stream >> rule.edge_capacitance_;
  stream >> rule.wire_extension_;
  stream >> rule.non_default_rule_;
  stream >> rule.layer_;
  return stream;
}

bool _dbTechLayerRule::operator==(const _dbTechLayerRule& rhs) const
{
  if (width_ != rhs.width_) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  if (resistance_ != rhs.resistance_) {
    return false;
  }

  if (capacitance_ != rhs.capacitance_) {
    return false;
  }

  if (edge_capacitance_ != rhs.edge_capacitance_) {
    return false;
  }

  if (wire_extension_ != rhs.wire_extension_) {
    return false;
  }

  if (non_default_rule_ != rhs.non_default_rule_) {
    return false;
  }

  if (layer_ != rhs.layer_) {
    return false;
  }

  return true;
}

_dbTech* _dbTechLayerRule::getTech()
{
  if (flags_.block_rule == 0) {
    return (_dbTech*) getOwner();
  }

  return getBlock()->getTech();
}

_dbBlock* _dbTechLayerRule::getBlock()
{
  assert(flags_.block_rule == 1);
  return (_dbBlock*) getOwner();
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerRule - Methods
//
////////////////////////////////////////////////////////////////////

dbTechLayer* dbTechLayerRule::getLayer()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  _dbTech* tech = rule->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(rule->layer_);
}

dbTechNonDefaultRule* dbTechLayerRule::getNonDefaultRule()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;

  if (rule->non_default_rule_ == 0) {
    return nullptr;
  }

  if (isBlockRule()) {
    _dbBlock* block = rule->getBlock();
    return (dbTechNonDefaultRule*) block->non_default_rule_tbl_->getPtr(
        rule->non_default_rule_);
  }
  _dbTech* tech = rule->getTech();
  return (dbTechNonDefaultRule*) tech->non_default_rule_tbl_->getPtr(
      rule->non_default_rule_);
}

bool dbTechLayerRule::isBlockRule()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->flags_.block_rule == 1;
}

int dbTechLayerRule::getWidth()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->width_;
}

void dbTechLayerRule::setWidth(int width)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->width_ = width;
}

int dbTechLayerRule::getSpacing()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->spacing_;
}

void dbTechLayerRule::setSpacing(int spacing)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->spacing_ = spacing;
}

double dbTechLayerRule::getEdgeCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->edge_capacitance_;
}

void dbTechLayerRule::setEdgeCapacitance(double cap)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->edge_capacitance_ = cap;
}

uint32_t dbTechLayerRule::getWireExtension()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->wire_extension_;
}

void dbTechLayerRule::setWireExtension(uint32_t ext)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->wire_extension_ = ext;
}

double dbTechLayerRule::getResistance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->resistance_;
}

void dbTechLayerRule::setResistance(double resistance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->resistance_ = resistance;
}

double dbTechLayerRule::getCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->capacitance_;
}

void dbTechLayerRule::setCapacitance(double capacitance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->capacitance_ = capacitance;
}

dbTechLayerRule* dbTechLayerRule::create(dbTechNonDefaultRule* rule_,
                                         dbTechLayer* layer_)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;
  _dbTechLayer* layer = (_dbTechLayer*) layer_;

  auto make_layer = [layer, rule](auto& tbl) -> dbTechLayerRule* {
    if (rule->layer_rules_[layer->number_] != 0) {
      return nullptr;
    }

    _dbTechLayerRule* layer_rule = tbl->create();
    layer_rule->non_default_rule_ = rule->getOID();
    layer_rule->layer_ = layer->getOID();
    layer_rule->flags_.block_rule = rule->flags_.block_rule;
    rule->layer_rules_[layer->number_] = layer_rule->getOID();
    return (dbTechLayerRule*) layer_rule;
  };

  if (rule->flags_.block_rule) {
    _dbBlock* block = rule->getBlock();
    return make_layer(block->layer_rule_tbl_);
  }
  _dbTech* tech = rule->getTech();
  return make_layer(tech->layer_rule_tbl_);
}

dbTechLayerRule* dbTechLayerRule::getTechLayerRule(dbTech* tech_,
                                                   uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechLayerRule*) tech->layer_rule_tbl_->getPtr(dbid_);
}

dbTechLayerRule* dbTechLayerRule::getTechLayerRule(dbBlock* block_,
                                                   uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTechLayerRule*) block->layer_rule_tbl_->getPtr(dbid_);
}

void _dbTechLayerRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
