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

#include "dbTechVia.h"

#include "db.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "dbTechViaGenerateRule.h"
#include "dbViaParams.h"

namespace odb {

template class dbTable<_dbTechVia>;
static void create_via_boxes(_dbTechVia* via, const dbViaParams& P);

bool _dbTechVia::operator==(const _dbTechVia& rhs) const
{
  if (_flags._default_via != rhs._flags._default_via)
    return false;

  if (_flags._top_of_stack != rhs._flags._top_of_stack)
    return false;

  if (_resistance != rhs._resistance)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_pattern && rhs._pattern) {
    if (strcmp(_pattern, rhs._pattern) != 0)
      return false;
  } else if (_pattern || rhs._pattern)
    return false;

  if (_bbox != rhs._bbox)
    return false;

  if (_boxes != rhs._boxes)
    return false;

  if (_top != rhs._top)
    return false;

  if (_bottom != rhs._bottom)
    return false;

  if (_non_default_rule != rhs._non_default_rule)
    return false;

  if (_generate_rule != rhs._generate_rule)
    return false;

  if (_via_params != rhs._via_params)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  return true;
}

void _dbTechVia::differences(dbDiff& diff,
                             const char* field,
                             const _dbTechVia& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._default_via);
  DIFF_FIELD(_flags._top_of_stack);
  DIFF_FIELD(_flags._has_params);
  DIFF_FIELD(_resistance);
  DIFF_FIELD(_name);
  DIFF_FIELD(_pattern);
  DIFF_FIELD(_bbox);
  DIFF_FIELD(_boxes);
  DIFF_FIELD(_top);
  DIFF_FIELD(_bottom);
  DIFF_FIELD(_non_default_rule);
  DIFF_FIELD(_generate_rule);
  DIFF_STRUCT(_via_params);
  DIFF_FIELD_NO_DEEP(_next_entry);
  DIFF_END
}

void _dbTechVia::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._default_via);
  DIFF_OUT_FIELD(_flags._top_of_stack);
  DIFF_OUT_FIELD(_flags._has_params);
  DIFF_OUT_FIELD(_resistance);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_pattern);
  DIFF_OUT_FIELD(_bbox);
  DIFF_OUT_FIELD(_boxes);
  DIFF_OUT_FIELD(_top);
  DIFF_OUT_FIELD(_bottom);
  DIFF_OUT_FIELD(_non_default_rule);
  DIFF_OUT_FIELD(_generate_rule);
  DIFF_OUT_STRUCT(_via_params);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbTechVia - Methods
//
////////////////////////////////////////////////////////////////////

_dbTechVia::_dbTechVia(_dbDatabase*, const _dbTechVia& v)
    : _flags(v._flags),
      _resistance(v._resistance),
      _name(NULL),
      _pattern(NULL),
      _bbox(v._bbox),
      _boxes(v._boxes),
      _top(v._top),
      _bottom(v._bottom),
      _non_default_rule(v._non_default_rule),
      _generate_rule(v._generate_rule),
      _via_params(v._via_params),
      _next_entry(v._next_entry)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }

  if (v._pattern) {
    _pattern = strdup(v._pattern);
    ZALLOCATED(_pattern);
  }
}

_dbTechVia::_dbTechVia(_dbDatabase*)
{
  _flags._default_via = 0;
  _flags._top_of_stack = 0;
  _flags._has_params = 0;
  _flags._spare_bits = 0;
  _resistance = 0.0;
  _name = 0;
  _pattern = 0;
}

_dbTechVia::~_dbTechVia()
{
  if (_name)
    free((void*) _name);

  if (_pattern)
    free((void*) _pattern);
}

dbOStream& operator<<(dbOStream& stream, const _dbTechVia& via)
{
  uint* bit_field = (uint*) &via._flags;
  stream << *bit_field;
  stream << via._resistance;
  stream << via._name;
  stream << via._bbox;
  stream << via._boxes;
  stream << via._top;
  stream << via._bottom;
  stream << via._non_default_rule;
  stream << via._generate_rule;
  stream << via._via_params;
  stream << via._pattern;
  stream << via._next_entry;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechVia& via)
{
  uint* bit_field = (uint*) &via._flags;
  stream >> *bit_field;
  stream >> via._resistance;
  stream >> via._name;
  stream >> via._bbox;
  stream >> via._boxes;
  stream >> via._top;
  stream >> via._bottom;
  stream >> via._non_default_rule;
  stream >> via._generate_rule;
  stream >> via._via_params;
  stream >> via._pattern;
  stream >> via._next_entry;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechVia - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechVia::getName()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_name;
}

const char* dbTechVia::getConstName()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_name;
}

bool dbTechVia::isDefault()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_flags._default_via == 1;
}

void dbTechVia::setDefault()
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->_flags._default_via = 1;
}

bool dbTechVia::isTopOfStack()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_flags._top_of_stack == 1;
}

void dbTechVia::setTopOfStack()
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->_flags._top_of_stack = 1;
}

double dbTechVia::getResistance()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_resistance;
}

void dbTechVia::setResistance(double resistance)
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->_resistance = resistance;
}

dbTech* dbTechVia::getTech()
{
  return (dbTech*) getImpl()->getOwner();
}

dbBox* dbTechVia::getBBox()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_bbox == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbBox*) tech->_box_tbl->getPtr(via->_bbox);
}

dbSet<dbBox> dbTechVia::getBoxes()
{
  _dbTechVia* via = (_dbTechVia*) this;
  _dbTech* tech = (_dbTech*) via->getOwner();
  return dbSet<dbBox>(via, tech->_box_itr);
}

dbTechLayer* dbTechVia::getTopLayer()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_top == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(via->_top);
}

dbTechLayer* dbTechVia::getBottomLayer()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_bottom == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(via->_bottom);
}

dbTechNonDefaultRule* dbTechVia::getNonDefaultRule()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_non_default_rule == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(
      via->_non_default_rule);
}

bool dbTechVia::hasParams()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->_flags._has_params == 1;
}

void dbTechVia::setViaGenerateRule(dbTechViaGenerateRule* rule)
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->_generate_rule = rule->getImpl()->getOID();
}

dbTechViaGenerateRule* dbTechVia::getViaGenerateRule()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_generate_rule == 0)
    return NULL;

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbTechViaGenerateRule* rule
      = tech->_via_generate_rule_tbl->getPtr(via->_generate_rule);
  return (dbTechViaGenerateRule*) rule;
}

std::string dbTechVia::getPattern()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_pattern == NULL) {
    return "";
  }

  return via->_pattern;
}

void dbTechVia::setPattern(const char* pattern)
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_pattern)
    free((void*) via->_pattern);

  via->_pattern = strdup(pattern);
}

void dbTechVia::setViaParams(const dbViaParams& params)
{
  _dbTechVia* via = (_dbTechVia*) this;
  _dbTech* tech = (_dbTech*) via->getOwner();
  via->_flags._has_params = 1;

  // Clear previous boxes
  dbSet<dbBox> boxes = getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end();) {
    dbSet<dbBox>::iterator n = ++itr;
    _dbBox* box = (_dbBox*) *itr;
    dbProperty::destroyProperties(box);
    tech->_box_tbl->destroy(box);
    itr = n;
  }

  via->_boxes = 0U;
  via->_via_params = params;
  via->_top = params._top_layer;
  via->_bottom = params._bot_layer;
  create_via_boxes(via, params);
}

void dbTechVia::getViaParams(dbViaParams& params)
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->_flags._has_params == 1)
    params = dbViaParams();
  else {
    params = via->_via_params;
    _dbTech* tech = (_dbTech*) via->getOwner();
    params._tech = (dbTech*) tech;
  }
}

dbTechVia* dbTechVia::create(dbTech* tech_, const char* name_)
{
  if (tech_->findVia(name_))
    return NULL;

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->_via_tbl->create();
  via->_name = strdup(name_);
  ZALLOCATED(via->_name);
  tech->_via_hash.insert(via);
  tech->_via_cnt++;
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::clone(dbTechNonDefaultRule* rule_,
                            dbTechVia* invia_,
                            const char* new_name)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;
  _dbTechVia* _invia = (_dbTechVia*) invia_;

  dbTech* tech_ = (dbTech*) rule->getOwner();

  if (tech_->findVia(new_name))
    return NULL;

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->_via_tbl->create();
  via->_name = strdup(new_name);
  ZALLOCATED(via->_name);

  via->_flags = _invia->_flags;
  via->_resistance = _invia->_resistance;
  via->_bbox = _invia->_bbox;
  via->_boxes = _invia->_boxes;
  via->_top = _invia->_top;
  via->_bottom = _invia->_bottom;
  via->_non_default_rule = (rule) ? rule->getOID() : 0;
  if (rule)
    via->_flags._default_via
        = 0;  // DEFAULT via not allowed for non-default rule

  tech->_via_cnt++;
  tech->_via_hash.insert(via);
  rule->_vias.push_back(via->getOID());
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::create(dbTechNonDefaultRule* rule_, const char* name_)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;

  dbTech* tech_ = (dbTech*) rule->getOwner();

  if (tech_->findVia(name_))
    return NULL;

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->_via_tbl->create();
  via->_name = strdup(name_);
  ZALLOCATED(via->_name);
  tech->_via_cnt++;
  via->_non_default_rule = rule->getOID();
  tech->_via_hash.insert(via);
  rule->_vias.push_back(via->getOID());
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::getTechVia(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechVia*) tech->_via_tbl->getPtr(dbid_);
}

void create_via_boxes(_dbTechVia* via, const dbViaParams& P)
{
  int rows = P.getNumCutRows();
  int cols = P.getNumCutCols();
  int row;
  int y = 0;
  int maxX = 0;
  int maxY = 0;
  std::vector<Rect> cutRects;

  for (row = 0; row < rows; ++row) {
    int col;
    int x = 0;

    for (col = 0; col < cols; ++col) {
      maxX = x + P.getXCutSize();
      maxY = y + P.getYCutSize();
      Rect r(x, y, maxX, maxY);
      cutRects.push_back(r);
      x = maxX;
      x += P.getXCutSpacing();
    }

    y = maxY;
    y += P.getYCutSpacing();
  }

  dbTechLayer* cut_layer = P.getCutLayer();

  int dx = maxX / 2;
  int dy = maxY / 2;
  std::vector<Rect>::iterator itr;

  for (itr = cutRects.begin(); itr != cutRects.end(); ++itr) {
    Rect& r = *itr;
    r.moveDelta(-dx, -dy);
    r.moveDelta(P.getXOrigin(), P.getYOrigin());
    dbBox::create(
        (dbTechVia*) via, cut_layer, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  int minX = -dx;
  int minY = -dy;
  maxX -= dx;
  maxY -= dy;

  int top_minX
      = minX - P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  int top_minY
      = minY - P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  int top_maxX
      = maxX + P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  int top_maxY
      = maxY + P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  dbBox::create((dbTechVia*) via,
                P.getTopLayer(),
                top_minX,
                top_minY,
                top_maxX,
                top_maxY);

  int bot_minX
      = minX - P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  int bot_minY
      = minY - P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  int bot_maxX
      = maxX + P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  int bot_maxY
      = maxY + P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  dbBox::create((dbTechVia*) via,
                P.getBottomLayer(),
                bot_minX,
                bot_minY,
                bot_maxX,
                bot_maxY);
}

}  // namespace odb
