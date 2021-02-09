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
#include "dbTechLayerCornerSpacingRule.h"

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
template class dbTable<_dbTechLayerCornerSpacingRule>;

bool _dbTechLayerCornerSpacingRule::operator==(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  if (_flags._corner_type != rhs._flags._corner_type)
    return false;

  if (_flags._same_mask != rhs._flags._same_mask)
    return false;

  if (_flags._same_x_y != rhs._flags._same_x_y)
    return false;

  if (_flags._corner_only != rhs._flags._corner_only)
    return false;

  if (_flags._except_eol != rhs._flags._except_eol)
    return false;

  if (_flags._except_jog_length != rhs._flags._except_jog_length)
    return false;

  if (_flags._edge_length_valid != rhs._flags._edge_length_valid)
    return false;

  if (_flags._include_shape != rhs._flags._include_shape)
    return false;

  if (_flags._min_length_valid != rhs._flags._min_length_valid)
    return false;

  if (_flags._except_notch != rhs._flags._except_notch)
    return false;

  if (_flags._except_same_net != rhs._flags._except_same_net)
    return false;

  if (_flags._except_same_metal != rhs._flags._except_same_metal)
    return false;

  if (_within != rhs._within)
    return false;

  if (_eol_width != rhs._eol_width)
    return false;

  if (_jog_length != rhs._jog_length)
    return false;

  if (_edge_length != rhs._edge_length)
    return false;

  if (_min_length != rhs._min_length)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCornerSpacingRule::operator<(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCornerSpacingRule::differences(
    dbDiff&                              diff,
    const char*                          field,
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._corner_type);
  DIFF_FIELD(_flags._same_mask);
  DIFF_FIELD(_flags._same_x_y);
  DIFF_FIELD(_flags._corner_only);
  DIFF_FIELD(_flags._except_eol);
  DIFF_FIELD(_flags._except_jog_length);
  DIFF_FIELD(_flags._edge_length_valid);
  DIFF_FIELD(_flags._include_shape);
  DIFF_FIELD(_flags._min_length_valid);
  DIFF_FIELD(_flags._except_notch);
  DIFF_FIELD(_flags._except_same_net);
  DIFF_FIELD(_flags._except_same_metal);
  DIFF_FIELD(_within);
  DIFF_FIELD(_eol_width);
  DIFF_FIELD(_jog_length);
  DIFF_FIELD(_edge_length);
  DIFF_FIELD(_min_length);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCornerSpacingRule::out(dbDiff&     diff,
                                        char        side,
                                        const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._corner_type);
  DIFF_OUT_FIELD(_flags._same_mask);
  DIFF_OUT_FIELD(_flags._same_x_y);
  DIFF_OUT_FIELD(_flags._corner_only);
  DIFF_OUT_FIELD(_flags._except_eol);
  DIFF_OUT_FIELD(_flags._except_jog_length);
  DIFF_OUT_FIELD(_flags._edge_length_valid);
  DIFF_OUT_FIELD(_flags._include_shape);
  DIFF_OUT_FIELD(_flags._min_length_valid);
  DIFF_OUT_FIELD(_flags._except_notch);
  DIFF_OUT_FIELD(_flags._except_same_net);
  DIFF_OUT_FIELD(_flags._except_same_metal);
  DIFF_OUT_FIELD(_within);
  DIFF_OUT_FIELD(_eol_width);
  DIFF_OUT_FIELD(_jog_length);
  DIFF_OUT_FIELD(_edge_length);
  DIFF_OUT_FIELD(_min_length);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(
    _dbDatabase*                         db,
    const _dbTechLayerCornerSpacingRule& r)
{
  _flags._corner_type       = r._flags._corner_type;
  _flags._same_mask         = r._flags._same_mask;
  _flags._same_x_y          = r._flags._same_x_y;
  _flags._corner_only       = r._flags._corner_only;
  _flags._except_eol        = r._flags._except_eol;
  _flags._except_jog_length = r._flags._except_jog_length;
  _flags._edge_length_valid = r._flags._edge_length_valid;
  _flags._include_shape     = r._flags._include_shape;
  _flags._min_length_valid  = r._flags._min_length_valid;
  _flags._except_notch      = r._flags._except_notch;
  _flags._except_same_net   = r._flags._except_same_net;
  _flags._except_same_metal = r._flags._except_same_metal;
  _flags._spare_bits        = r._flags._spare_bits;
  _within                   = r._within;
  _eol_width                = r._eol_width;
  _jog_length               = r._jog_length;
  _edge_length              = r._edge_length;
  _min_length               = r._min_length;
  // User Code Begin CopyConstructor
  _width_tbl   = r._width_tbl;
  _spacing_tbl = r._spacing_tbl;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCornerSpacingRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._within;
  stream >> obj._eol_width;
  stream >> obj._jog_length;
  stream >> obj._edge_length;
  stream >> obj._min_length;
  // User Code Begin >>
  stream >> obj._width_tbl;
  stream >> obj._spacing_tbl;
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                           stream,
                      const _dbTechLayerCornerSpacingRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._within;
  stream << obj._eol_width;
  stream << obj._jog_length;
  stream << obj._edge_length;
  stream << obj._min_length;
  // User Code Begin <<
  stream << obj._width_tbl;
  stream << obj._spacing_tbl;
  // User Code End <<
  return stream;
}

_dbTechLayerCornerSpacingRule::~_dbTechLayerCornerSpacingRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCornerSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCornerSpacingRule::setWithin(int _within)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_within = _within;
}

int dbTechLayerCornerSpacingRule::getWithin() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->_within;
}

void dbTechLayerCornerSpacingRule::setEolWidth(int _eol_width)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_eol_width = _eol_width;
}

int dbTechLayerCornerSpacingRule::getEolWidth() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->_eol_width;
}

void dbTechLayerCornerSpacingRule::setJogLength(int _jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_jog_length = _jog_length;
}

int dbTechLayerCornerSpacingRule::getJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->_jog_length;
}

void dbTechLayerCornerSpacingRule::setEdgeLength(int _edge_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_edge_length = _edge_length;
}

int dbTechLayerCornerSpacingRule::getEdgeLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->_edge_length;
}

void dbTechLayerCornerSpacingRule::setMinLength(int _min_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_min_length = _min_length;
}

int dbTechLayerCornerSpacingRule::getMinLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->_min_length;
}

void dbTechLayerCornerSpacingRule::setSameMask(bool _same_mask)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._same_mask = _same_mask;
}

bool dbTechLayerCornerSpacingRule::isSameMask() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._same_mask;
}

void dbTechLayerCornerSpacingRule::setSameXY(bool _same_x_y)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._same_x_y = _same_x_y;
}

bool dbTechLayerCornerSpacingRule::isSameXY() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._same_x_y;
}

void dbTechLayerCornerSpacingRule::setCornerOnly(bool _corner_only)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._corner_only = _corner_only;
}

bool dbTechLayerCornerSpacingRule::isCornerOnly() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._corner_only;
}

void dbTechLayerCornerSpacingRule::setExceptEol(bool _except_eol)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._except_eol = _except_eol;
}

bool dbTechLayerCornerSpacingRule::isExceptEol() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._except_eol;
}

void dbTechLayerCornerSpacingRule::setExceptJogLength(bool _except_jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._except_jog_length = _except_jog_length;
}

bool dbTechLayerCornerSpacingRule::isExceptJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._except_jog_length;
}

void dbTechLayerCornerSpacingRule::setEdgeLengthValid(bool _edge_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._edge_length_valid = _edge_length_valid;
}

bool dbTechLayerCornerSpacingRule::isEdgeLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._edge_length_valid;
}

void dbTechLayerCornerSpacingRule::setIncludeShape(bool _include_shape)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._include_shape = _include_shape;
}

bool dbTechLayerCornerSpacingRule::isIncludeShape() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._include_shape;
}

void dbTechLayerCornerSpacingRule::setMinLengthValid(bool _min_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._min_length_valid = _min_length_valid;
}

bool dbTechLayerCornerSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._min_length_valid;
}

void dbTechLayerCornerSpacingRule::setExceptNotch(bool _except_notch)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._except_notch = _except_notch;
}

bool dbTechLayerCornerSpacingRule::isExceptNotch() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._except_notch;
}

void dbTechLayerCornerSpacingRule::setExceptSameNet(bool _except_same_net)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._except_same_net = _except_same_net;
}

bool dbTechLayerCornerSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._except_same_net;
}

void dbTechLayerCornerSpacingRule::setExceptSameMetal(bool _except_same_metal)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._except_same_metal = _except_same_metal;
}

bool dbTechLayerCornerSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags._except_same_metal;
}

// User Code Begin dbTechLayerCornerSpacingRulePublicMethods
void dbTechLayerCornerSpacingRule::addSpacing(uint width,
                                              uint spacing1,
                                              uint spacing2)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  obj->_width_tbl.push_back(width);
  obj->_spacing_tbl.push_back(std::make_pair(spacing1, spacing2));
}

void dbTechLayerCornerSpacingRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl                                = obj->_spacing_tbl;
}

void dbTechLayerCornerSpacingRule::getWidthTable(std::vector<int>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl                                = obj->_width_tbl;
}

void dbTechLayerCornerSpacingRule::setType(CornerType _type)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags._corner_type = (uint) _type;
}

dbTechLayerCornerSpacingRule::CornerType dbTechLayerCornerSpacingRule::getType()
    const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return (dbTechLayerCornerSpacingRule::CornerType) obj->_flags._corner_type;
}

dbTechLayerCornerSpacingRule* dbTechLayerCornerSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*                  layer = (_dbTechLayer*) _layer;
  _dbTechLayerCornerSpacingRule* newrule
      = layer->_corner_spacing_rules_tbl->create();
  return ((dbTechLayerCornerSpacingRule*) newrule);
}

dbTechLayerCornerSpacingRule*
dbTechLayerCornerSpacingRule::getTechLayerCornerSpacingRule(dbTechLayer* inly,
                                                            uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCornerSpacingRule*)
      layer->_corner_spacing_rules_tbl->getPtr(dbid);
}
void dbTechLayerCornerSpacingRule::destroy(dbTechLayerCornerSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_corner_spacing_rules_tbl->destroy(
      (_dbTechLayerCornerSpacingRule*) rule);
}
// User Code End dbTechLayerCornerSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End 1