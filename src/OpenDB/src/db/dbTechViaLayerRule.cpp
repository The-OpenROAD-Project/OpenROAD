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

#include "dbTechViaLayerRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbTechViaGenerateRule.h"
#include "dbTechViaRule.h"

namespace odb {

template class dbTable<_dbTechViaLayerRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaLayerRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaLayerRule::operator==(const _dbTechViaLayerRule& rhs) const
{
  if (_flags._direction != rhs._flags._direction)
    return false;

  if (_flags._has_enclosure != rhs._flags._has_enclosure)
    return false;

  if (_flags._has_width != rhs._flags._has_width)
    return false;

  if (_flags._has_overhang != rhs._flags._has_overhang)
    return false;

  if (_flags._has_metal_overhang != rhs._flags._has_metal_overhang)
    return false;

  if (_flags._has_resistance != rhs._flags._has_resistance)
    return false;

  if (_flags._has_spacing != rhs._flags._has_spacing)
    return false;

  if (_flags._has_rect != rhs._flags._has_rect)
    return false;

  if (_overhang1 != rhs._overhang1)
    return false;

  if (_overhang2 != rhs._overhang2)
    return false;

  if (_min_width != rhs._min_width)
    return false;

  if (_max_width != rhs._max_width)
    return false;

  if (_overhang != rhs._overhang)
    return false;

  if (_metal_overhang != rhs._metal_overhang)
    return false;

  if (_spacing_x != rhs._spacing_x)
    return false;

  if (_spacing_y != rhs._spacing_y)
    return false;

  if (_resistance != rhs._resistance)
    return false;

  if (_rect != rhs._rect)
    return false;

  if (_layer != rhs._layer)
    return false;

  return true;
}

void _dbTechViaLayerRule::differences(dbDiff&                    diff,
                                      const char*                field,
                                      const _dbTechViaLayerRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._direction);
  DIFF_FIELD(_flags._has_enclosure);
  DIFF_FIELD(_flags._has_width);
  DIFF_FIELD(_flags._has_overhang);
  DIFF_FIELD(_flags._has_metal_overhang);
  DIFF_FIELD(_flags._has_resistance);
  DIFF_FIELD(_flags._has_spacing);
  DIFF_FIELD(_flags._has_rect);
  DIFF_FIELD(_overhang1);
  DIFF_FIELD(_overhang2);
  DIFF_FIELD(_min_width);
  DIFF_FIELD(_max_width);
  DIFF_FIELD(_overhang);
  DIFF_FIELD(_metal_overhang);
  DIFF_FIELD(_spacing_x);
  DIFF_FIELD(_spacing_y);
  DIFF_FIELD(_resistance);
  DIFF_FIELD(_rect);
  DIFF_FIELD(_layer);
  DIFF_END
}

void _dbTechViaLayerRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._direction);
  DIFF_OUT_FIELD(_flags._has_enclosure);
  DIFF_OUT_FIELD(_flags._has_width);
  DIFF_OUT_FIELD(_flags._has_overhang);
  DIFF_OUT_FIELD(_flags._has_metal_overhang);
  DIFF_OUT_FIELD(_flags._has_resistance);
  DIFF_OUT_FIELD(_flags._has_spacing);
  DIFF_OUT_FIELD(_flags._has_rect);
  DIFF_OUT_FIELD(_overhang1);
  DIFF_OUT_FIELD(_overhang2);
  DIFF_OUT_FIELD(_min_width);
  DIFF_OUT_FIELD(_max_width);
  DIFF_OUT_FIELD(_overhang);
  DIFF_OUT_FIELD(_metal_overhang);
  DIFF_OUT_FIELD(_spacing_x);
  DIFF_OUT_FIELD(_spacing_y);
  DIFF_OUT_FIELD(_resistance);
  DIFF_OUT_FIELD(_rect);
  DIFF_OUT_FIELD(_layer);
  DIFF_END
}

_dbTechViaLayerRule::_dbTechViaLayerRule(_dbDatabase*,
                                         const _dbTechViaLayerRule& v)
    : _flags(v._flags),
      _overhang1(v._overhang1),
      _overhang2(v._overhang2),
      _min_width(v._min_width),
      _max_width(v._max_width),
      _overhang(v._overhang),
      _metal_overhang(v._metal_overhang),
      _spacing_x(v._spacing_x),
      _spacing_y(v._spacing_y),
      _resistance(v._resistance),
      _rect(v._rect),
      _layer(v._layer)
{
}

_dbTechViaLayerRule::_dbTechViaLayerRule(_dbDatabase*)
{
  _flags._direction          = 0;
  _flags._has_enclosure      = 0;
  _flags._has_width          = 0;
  _flags._has_overhang       = 0;
  _flags._has_metal_overhang = 0;
  _flags._has_resistance     = 0;
  _flags._has_spacing        = 0;
  _flags._has_rect           = 0;
  _flags._spare_bits         = 0;
  _overhang1                 = 0;
  _overhang2                 = 0;
  _min_width                 = 0;
  _max_width                 = 0;
  _overhang                  = 0;
  _metal_overhang            = 0;
  _spacing_x                 = 0;
  _spacing_y                 = 0;
  _resistance                = 0.0;
}

_dbTechViaLayerRule::~_dbTechViaLayerRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaLayerRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._overhang1;
  stream << v._overhang2;
  stream << v._min_width;
  stream << v._max_width;
  stream << v._overhang;
  stream << v._metal_overhang;
  stream << v._spacing_x;
  stream << v._spacing_y;
  stream << v._resistance;
  stream << v._rect;
  stream << v._layer;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaLayerRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._overhang1;
  stream >> v._overhang2;
  stream >> v._min_width;
  stream >> v._max_width;
  stream >> v._overhang;
  stream >> v._metal_overhang;
  stream >> v._spacing_x;
  stream >> v._spacing_y;
  stream >> v._resistance;
  stream >> v._rect;
  stream >> v._layer;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaLayerRule - Methods
//
////////////////////////////////////////////////////////////////////

dbTechLayer* dbTechViaLayerRule::getLayer()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;

  if (rule->_layer == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer);
}

dbTechLayerDir dbTechViaLayerRule::getDirection()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  dbTechLayerDir       d((dbTechLayerDir::Value) rule->_flags._direction);
  return d;
}

void dbTechViaLayerRule::setDirection(dbTechLayerDir dir)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->_flags._direction   = dir.getValue();
}

bool dbTechViaLayerRule::hasWidth()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_width == 1;
}

void dbTechViaLayerRule::getWidth(int& minWidth, int& maxWidth)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  minWidth                  = rule->_min_width;
  maxWidth                  = rule->_max_width;
}

void dbTechViaLayerRule::setWidth(int minWidth, int maxWidth)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->_flags._has_width   = 1;
  rule->_min_width          = minWidth;
  rule->_max_width          = maxWidth;
}

bool dbTechViaLayerRule::hasEnclosure()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_enclosure == 1;
}

void dbTechViaLayerRule::getEnclosure(int& overhang1, int& overhang2)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  overhang1                 = rule->_overhang1;
  overhang2                 = rule->_overhang2;
}

void dbTechViaLayerRule::setEnclosure(int overhang1, int overhang2)
{
  _dbTechViaLayerRule* rule   = (_dbTechViaLayerRule*) this;
  rule->_flags._has_enclosure = 1;
  rule->_overhang1            = overhang1;
  rule->_overhang2            = overhang2;
}

bool dbTechViaLayerRule::hasOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_overhang == 1;
}

int dbTechViaLayerRule::getOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_overhang;
}

void dbTechViaLayerRule::setOverhang(int overhang)
{
  _dbTechViaLayerRule* rule  = (_dbTechViaLayerRule*) this;
  rule->_flags._has_overhang = 1;
  rule->_overhang            = overhang;
}

bool dbTechViaLayerRule::hasMetalOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_metal_overhang == 1;
}

int dbTechViaLayerRule::getMetalOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_metal_overhang;
}

void dbTechViaLayerRule::setMetalOverhang(int overhang)
{
  _dbTechViaLayerRule* rule        = (_dbTechViaLayerRule*) this;
  rule->_flags._has_metal_overhang = 1;
  rule->_metal_overhang            = overhang;
}

bool dbTechViaLayerRule::hasSpacing()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_spacing == 1;
}

void dbTechViaLayerRule::getSpacing(int& spacing_x, int& spacing_y)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  spacing_x                 = rule->_spacing_x;
  spacing_y                 = rule->_spacing_y;
}

void dbTechViaLayerRule::setSpacing(int spacing_x, int spacing_y)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->_flags._has_spacing = 1;
  rule->_spacing_x          = spacing_x;
  rule->_spacing_y          = spacing_y;
}

bool dbTechViaLayerRule::hasRect()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_rect == 1;
}

void dbTechViaLayerRule::getRect(Rect& r)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  r                         = rule->_rect;
}

void dbTechViaLayerRule::setRect(const Rect& r)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->_flags._has_rect    = 1;
  rule->_rect               = r;
}

bool dbTechViaLayerRule::hasResistance()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_flags._has_resistance == 1;
}

void dbTechViaLayerRule::setResistance(double r)
{
  _dbTechViaLayerRule* rule    = (_dbTechViaLayerRule*) this;
  rule->_flags._has_resistance = 1;
  rule->_resistance            = r;
}

double dbTechViaLayerRule::getResistance()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->_resistance;
}

dbTechViaLayerRule* dbTechViaLayerRule::create(dbTech*        tech_,
                                               dbTechViaRule* rule_,
                                               dbTechLayer*   layer)
{
  _dbTech*             tech    = (_dbTech*) tech_;
  _dbTechViaRule*      rule    = (_dbTechViaRule*) rule_;
  _dbTechViaLayerRule* layrule = tech->_via_layer_rule_tbl->create();
  layrule->_layer              = layer->getImpl()->getOID();
  rule->_layer_rules.push_back(layrule->getImpl()->getOID());
  return (dbTechViaLayerRule*) layrule;
}

dbTechViaLayerRule* dbTechViaLayerRule::create(dbTech*                tech_,
                                               dbTechViaGenerateRule* rule_,
                                               dbTechLayer*           layer)
{
  _dbTech*                tech    = (_dbTech*) tech_;
  _dbTechViaGenerateRule* rule    = (_dbTechViaGenerateRule*) rule_;
  _dbTechViaLayerRule*    layrule = tech->_via_layer_rule_tbl->create();
  layrule->_layer                 = layer->getImpl()->getOID();
  rule->_layer_rules.push_back(layrule->getImpl()->getOID());
  return (dbTechViaLayerRule*) layrule;
}

dbTechViaLayerRule* dbTechViaLayerRule::getTechViaLayerRule(dbTech* tech_,
                                                            uint    dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaLayerRule*) tech->_via_layer_rule_tbl->getPtr(dbid_);
}

}  // namespace odb
