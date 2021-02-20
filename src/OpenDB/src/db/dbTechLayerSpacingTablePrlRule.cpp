///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

// Generator Code Begin 1
#include "dbTechLayerSpacingTablePrlRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerSpacingTablePrlRule>;

bool _dbTechLayerSpacingTablePrlRule::operator==(
    const _dbTechLayerSpacingTablePrlRule& rhs) const
{
  if (_flags._wrong_direction != rhs._flags._wrong_direction)
    return false;

  if (_flags._same_mask != rhs._flags._same_mask)
    return false;

  if (_flags._exceept_eol != rhs._flags._exceept_eol)
    return false;

  if (_eol_width != rhs._eol_width)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerSpacingTablePrlRule::operator<(
    const _dbTechLayerSpacingTablePrlRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerSpacingTablePrlRule::differences(
    dbDiff&                                diff,
    const char*                            field,
    const _dbTechLayerSpacingTablePrlRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._wrong_direction);
  DIFF_FIELD(_flags._same_mask);
  DIFF_FIELD(_flags._exceept_eol);
  DIFF_FIELD(_eol_width);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerSpacingTablePrlRule::out(dbDiff&     diff,
                                          char        side,
                                          const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._wrong_direction);
  DIFF_OUT_FIELD(_flags._same_mask);
  DIFF_OUT_FIELD(_flags._exceept_eol);
  DIFF_OUT_FIELD(_eol_width);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerSpacingTablePrlRule::_dbTechLayerSpacingTablePrlRule(
    _dbDatabase* db)
{
  uint32_t* _flags_bit_field = (uint32_t*) &_flags;
  *_flags_bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerSpacingTablePrlRule::_dbTechLayerSpacingTablePrlRule(
    _dbDatabase*                           db,
    const _dbTechLayerSpacingTablePrlRule& r)
{
  _flags._wrong_direction = r._flags._wrong_direction;
  _flags._same_mask       = r._flags._same_mask;
  _flags._exceept_eol     = r._flags._exceept_eol;
  _flags._spare_bits      = r._flags._spare_bits;
  _eol_width              = r._eol_width;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingTablePrlRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._eol_width;
  stream >> obj._length_tbl;
  stream >> obj._width_tbl;
  stream >> obj._spacing_tbl;
  stream >> obj._influence_tbl;
  // User Code Begin >>
  stream >> obj._within_tbl;
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                             stream,
                      const _dbTechLayerSpacingTablePrlRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._eol_width;
  stream << obj._length_tbl;
  stream << obj._width_tbl;
  stream << obj._spacing_tbl;
  stream << obj._influence_tbl;
  // User Code Begin <<
  stream << obj._within_tbl;
  // User Code End <<
  return stream;
}

_dbTechLayerSpacingTablePrlRule::~_dbTechLayerSpacingTablePrlRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerSpacingTablePrlRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerSpacingTablePrlRule::setEolWidth(int _eol_width)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  obj->_eol_width = _eol_width;
}

int dbTechLayerSpacingTablePrlRule::getEolWidth() const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  return obj->_eol_width;
}

void dbTechLayerSpacingTablePrlRule::setWrongDirection(bool _wrong_direction)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  obj->_flags._wrong_direction = _wrong_direction;
}

bool dbTechLayerSpacingTablePrlRule::isWrongDirection() const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  return obj->_flags._wrong_direction;
}

void dbTechLayerSpacingTablePrlRule::setSameMask(bool _same_mask)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  obj->_flags._same_mask = _same_mask;
}

bool dbTechLayerSpacingTablePrlRule::isSameMask() const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  return obj->_flags._same_mask;
}

void dbTechLayerSpacingTablePrlRule::setExceeptEol(bool _exceept_eol)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  obj->_flags._exceept_eol = _exceept_eol;
}

bool dbTechLayerSpacingTablePrlRule::isExceeptEol() const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;

  return obj->_flags._exceept_eol;
}

// User Code Begin dbTechLayerSpacingTablePrlRulePublicMethods

uint _dbTechLayerSpacingTablePrlRule::getWidthIdx(const int width) const
{
  auto pos = --(std::lower_bound(_width_tbl.begin(), _width_tbl.end(), width));
  return std::max(0, (int) std::distance(_width_tbl.begin(), pos));
}

uint _dbTechLayerSpacingTablePrlRule::getLengthIdx(const int length) const
{
  auto pos
      = --(std::lower_bound(_length_tbl.begin(), _length_tbl.end(), length));
  return std::max(0, (int) std::distance(_length_tbl.begin(), pos));
}

void dbTechLayerSpacingTablePrlRule::setTable(
    std::vector<int>                    width_tbl,
    std::vector<int>                    length_tbl,
    std::vector<std::vector<int>>       spacing_tbl,
    std::map<uint, std::pair<int, int>> excluded_map)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  obj->_width_tbl  = width_tbl;
  obj->_length_tbl = length_tbl;
  for (auto spacing : spacing_tbl) {
    dbVector<int> tmp;
    tmp = spacing;
    obj->_spacing_tbl.push_back(tmp);
  }
  obj->_within_tbl = excluded_map;
}

void dbTechLayerSpacingTablePrlRule::getTable(
    std::vector<int>&                    width_tbl,
    std::vector<int>&                    length_tbl,
    std::vector<std::vector<int>>&       spacing_tbl,
    std::map<uint, std::pair<int, int>>& excluded_map)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  width_tbl    = obj->_width_tbl;
  length_tbl   = obj->_length_tbl;
  excluded_map = obj->_within_tbl;
  for (auto spacing : obj->_spacing_tbl) {
    spacing_tbl.push_back(spacing);
  }
}

void dbTechLayerSpacingTablePrlRule::setSpacingTableInfluence(
    std::vector<std::tuple<int, int, int>> influence_tbl)
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  obj->_influence_tbl = influence_tbl;
}

dbTechLayerSpacingTablePrlRule* dbTechLayerSpacingTablePrlRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*                    layer = (_dbTechLayer*) _layer;
  _dbTechLayerSpacingTablePrlRule* newrule
      = layer->_spacing_table_prl_rules_tbl->create();
  return ((dbTechLayerSpacingTablePrlRule*) newrule);
}

dbTechLayerSpacingTablePrlRule*
dbTechLayerSpacingTablePrlRule::getTechLayerSpacingTablePrlRule(
    dbTechLayer* inly,
    uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerSpacingTablePrlRule*)
      layer->_spacing_table_prl_rules_tbl->getPtr(dbid);
}

void dbTechLayerSpacingTablePrlRule::destroy(
    dbTechLayerSpacingTablePrlRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_spacing_table_prl_rules_tbl->destroy(
      (_dbTechLayerSpacingTablePrlRule*) rule);
}

int dbTechLayerSpacingTablePrlRule::getSpacing(const int width,
                                               const int length) const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  uint rowIdx = obj->getWidthIdx(width);
  uint colIdx = obj->getLengthIdx(length);
  return obj->_spacing_tbl[rowIdx][colIdx];
}

bool dbTechLayerSpacingTablePrlRule::hasExceptWithin(int width) const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  uint rowIdx = obj->getWidthIdx(width);
  return (obj->_within_tbl.find(rowIdx) != obj->_within_tbl.end());
}

std::pair<int, int> dbTechLayerSpacingTablePrlRule::getExceptWithin(
    int width) const
{
  _dbTechLayerSpacingTablePrlRule* obj
      = (_dbTechLayerSpacingTablePrlRule*) this;
  uint rowIdx = obj->getWidthIdx(width);
  return obj->_within_tbl.at(rowIdx);
}

// User Code End dbTechLayerSpacingTablePrlRulePublicMethods
}  // namespace odb
   // Generator Code End 1