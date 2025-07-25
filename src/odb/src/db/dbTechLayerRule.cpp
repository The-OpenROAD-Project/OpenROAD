// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerRule.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbTechLayerRule>;

_dbTechLayerRule::_dbTechLayerRule(_dbDatabase*, const _dbTechLayerRule& r)
    : _flags(r._flags),
      _width(r._width),
      _spacing(r._spacing),
      _resistance(r._resistance),
      _capacitance(r._capacitance),
      _edge_capacitance(r._edge_capacitance),
      _wire_extension(r._wire_extension),
      _non_default_rule(r._non_default_rule),
      _layer(r._layer)
{
}

_dbTechLayerRule::_dbTechLayerRule(_dbDatabase*)
{
  _flags._spare_bits = 0;
  _width = 0;
  _spacing = 0;
  _resistance = 0.0;
  _capacitance = 0.0;
  _edge_capacitance = 0.0;
  _wire_extension = 0;
}

_dbTechLayerRule::~_dbTechLayerRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._width;
  stream << rule._spacing;
  stream << rule._resistance;
  stream << rule._capacitance;
  stream << rule._edge_capacitance;
  stream << rule._wire_extension;
  stream << rule._non_default_rule;
  stream << rule._layer;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._width;
  stream >> rule._spacing;
  stream >> rule._resistance;
  stream >> rule._capacitance;
  stream >> rule._edge_capacitance;
  stream >> rule._wire_extension;
  stream >> rule._non_default_rule;
  stream >> rule._layer;
  return stream;
}

bool _dbTechLayerRule::operator==(const _dbTechLayerRule& rhs) const
{
  if (_width != rhs._width) {
    return false;
  }

  if (_spacing != rhs._spacing) {
    return false;
  }

  if (_resistance != rhs._resistance) {
    return false;
  }

  if (_capacitance != rhs._capacitance) {
    return false;
  }

  if (_edge_capacitance != rhs._edge_capacitance) {
    return false;
  }

  if (_wire_extension != rhs._wire_extension) {
    return false;
  }

  if (_non_default_rule != rhs._non_default_rule) {
    return false;
  }

  if (_layer != rhs._layer) {
    return false;
  }

  return true;
}

_dbTech* _dbTechLayerRule::getTech()
{
  if (_flags._block_rule == 0) {
    return (_dbTech*) getOwner();
  }

  return (_dbTech*) getBlock()->getTech();
}

_dbBlock* _dbTechLayerRule::getBlock()
{
  assert(_flags._block_rule == 1);
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
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer);
}

dbTechNonDefaultRule* dbTechLayerRule::getNonDefaultRule()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;

  if (rule->_non_default_rule == 0) {
    return nullptr;
  }

  if (isBlockRule()) {
    _dbBlock* block = rule->getBlock();
    return (dbTechNonDefaultRule*) block->_non_default_rule_tbl->getPtr(
        rule->_non_default_rule);
  }
  _dbTech* tech = rule->getTech();
  return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(
      rule->_non_default_rule);
}

bool dbTechLayerRule::isBlockRule()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_flags._block_rule == 1;
}

int dbTechLayerRule::getWidth()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_width;
}

void dbTechLayerRule::setWidth(int width)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_width = width;
}

int dbTechLayerRule::getSpacing()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_spacing;
}

void dbTechLayerRule::setSpacing(int spacing)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_spacing = spacing;
}

double dbTechLayerRule::getEdgeCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_edge_capacitance;
}

void dbTechLayerRule::setEdgeCapacitance(double cap)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_edge_capacitance = cap;
}

uint dbTechLayerRule::getWireExtension()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_wire_extension;
}

void dbTechLayerRule::setWireExtension(uint ext)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_wire_extension = ext;
}

double dbTechLayerRule::getResistance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_resistance;
}

void dbTechLayerRule::setResistance(double resistance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_resistance = resistance;
}

double dbTechLayerRule::getCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_capacitance;
}

void dbTechLayerRule::setCapacitance(double capacitance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_capacitance = capacitance;
}

dbTechLayerRule* dbTechLayerRule::create(dbTechNonDefaultRule* rule_,
                                         dbTechLayer* layer_)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;
  _dbTechLayer* layer = (_dbTechLayer*) layer_;

  auto make_layer = [layer, rule](auto& tbl) -> dbTechLayerRule* {
    if (rule->_layer_rules[layer->_number] != 0) {
      return nullptr;
    }

    _dbTechLayerRule* layer_rule = tbl->create();
    layer_rule->_non_default_rule = rule->getOID();
    layer_rule->_layer = layer->getOID();
    layer_rule->_flags._block_rule = rule->_flags._block_rule;
    rule->_layer_rules[layer->_number] = layer_rule->getOID();
    return (dbTechLayerRule*) layer_rule;
  };

  if (rule->_flags._block_rule) {
    _dbBlock* block = rule->getBlock();
    return make_layer(block->_layer_rule_tbl);
  }
  _dbTech* tech = rule->getTech();
  return make_layer(tech->_layer_rule_tbl);
}

dbTechLayerRule* dbTechLayerRule::getTechLayerRule(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechLayerRule*) tech->_layer_rule_tbl->getPtr(dbid_);
}

dbTechLayerRule* dbTechLayerRule::getTechLayerRule(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTechLayerRule*) block->_layer_rule_tbl->getPtr(dbid_);
}

void _dbTechLayerRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
