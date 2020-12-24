///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbTechLayerRule.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"

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
  _width             = 0;
  _spacing           = 0;
  _resistance        = 0.0;
  _capacitance       = 0.0;
  _edge_capacitance  = 0.0;
  _wire_extension    = 0;
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
  if (_width != rhs._width)
    return false;

  if (_spacing != rhs._spacing)
    return false;

  if (_resistance != rhs._resistance)
    return false;

  if (_capacitance != rhs._capacitance)
    return false;

  if (_edge_capacitance != rhs._edge_capacitance)
    return false;

  if (_wire_extension != rhs._wire_extension)
    return false;

  if (_non_default_rule != rhs._non_default_rule)
    return false;

  if (_layer != rhs._layer)
    return false;

  return true;
}

void _dbTechLayerRule::differences(dbDiff&                 diff,
                                   const char*             field,
                                   const _dbTechLayerRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._block_rule);
  DIFF_FIELD(_width);
  DIFF_FIELD(_spacing);
  DIFF_FIELD(_resistance);
  DIFF_FIELD(_capacitance);
  DIFF_FIELD(_edge_capacitance);
  DIFF_FIELD(_wire_extension);
  DIFF_FIELD(_non_default_rule);
  DIFF_FIELD(_layer);
  DIFF_END
}

void _dbTechLayerRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._block_rule);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_spacing);
  DIFF_OUT_FIELD(_resistance);
  DIFF_OUT_FIELD(_capacitance);
  DIFF_OUT_FIELD(_edge_capacitance);
  DIFF_OUT_FIELD(_wire_extension);
  DIFF_OUT_FIELD(_non_default_rule);
  DIFF_OUT_FIELD(_layer);
  DIFF_END
}

_dbTech* _dbTechLayerRule::getTech()
{
#if 0  // dead code generates warnings -cherry
    if (_flags._block_rule == 0)
        (_dbTech *) getOwner();
#endif

  return (_dbTech*) getDb()->getTech();
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
  _dbTech*          tech = rule->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer);
}

dbTechNonDefaultRule* dbTechLayerRule::getNonDefaultRule()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;

  if (rule->_non_default_rule == 0)
    return NULL;

  if (isBlockRule()) {
    _dbBlock* block = rule->getBlock();
    return (dbTechNonDefaultRule*) block->_non_default_rule_tbl->getPtr(
        rule->_non_default_rule);
  } else {
    _dbTech* tech = rule->getTech();
    return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(
        rule->_non_default_rule);
  }
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
  rule->_width           = width;
}

int dbTechLayerRule::getSpacing()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_spacing;
}

void dbTechLayerRule::setSpacing(int spacing)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_spacing         = spacing;
}

double dbTechLayerRule::getEdgeCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_edge_capacitance;
}

void dbTechLayerRule::setEdgeCapacitance(double cap)
{
  _dbTechLayerRule* rule  = (_dbTechLayerRule*) this;
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
  rule->_wire_extension  = ext;
}

double dbTechLayerRule::getResistance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_resistance;
}

void dbTechLayerRule::setResistance(double resistance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_resistance      = resistance;
}

double dbTechLayerRule::getCapacitance()
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  return rule->_capacitance;
}

void dbTechLayerRule::setCapacitance(double capacitance)
{
  _dbTechLayerRule* rule = (_dbTechLayerRule*) this;
  rule->_capacitance     = capacitance;
}

dbTechLayerRule* dbTechLayerRule::create(dbTechNonDefaultRule* rule_,
                                         dbTechLayer*          layer_)
{
  _dbTechNonDefaultRule*     rule           = (_dbTechNonDefaultRule*) rule_;
  _dbTechLayer*              layer          = (_dbTechLayer*) layer_;
  dbTable<_dbTechLayerRule>* layer_rule_tbl = NULL;

  if (rule->_flags._block_rule) {
    _dbBlock* block = rule->getBlock();
    layer_rule_tbl  = block->_layer_rule_tbl;
  } else {
    _dbTech* tech  = rule->getTech();
    layer_rule_tbl = tech->_layer_rule_tbl;
  }

  if (rule->_layer_rules[layer->_number] != 0)
    return NULL;

  _dbTechLayerRule* layer_rule       = layer_rule_tbl->create();
  layer_rule->_non_default_rule      = rule->getOID();
  layer_rule->_layer                 = layer->getOID();
  layer_rule->_flags._block_rule     = rule->_flags._block_rule;
  rule->_layer_rules[layer->_number] = layer_rule->getOID();
  return (dbTechLayerRule*) layer_rule;
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

}  // namespace odb
