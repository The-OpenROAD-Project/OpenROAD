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
#include "dbTechLayerMinStepRule.h"

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
template class dbTable<_dbTechLayerMinStepRule>;

bool _dbTechLayerMinStepRule::operator==(
    const _dbTechLayerMinStepRule& rhs) const
{
  if (_flags._max_edges_valid != rhs._flags._max_edges_valid)
    return false;

  if (_flags._min_adj_length1_valid != rhs._flags._min_adj_length1_valid)
    return false;

  if (_flags._no_between_eol != rhs._flags._no_between_eol)
    return false;

  if (_flags._min_adj_length2_valid != rhs._flags._min_adj_length2_valid)
    return false;

  if (_flags._convex_corner != rhs._flags._convex_corner)
    return false;

  if (_flags._min_between_length_valid != rhs._flags._min_between_length_valid)
    return false;

  if (_flags._except_same_corners != rhs._flags._except_same_corners)
    return false;

  if (_min_step_length != rhs._min_step_length)
    return false;

  if (_max_edges != rhs._max_edges)
    return false;

  if (_min_adj_length1 != rhs._min_adj_length1)
    return false;

  if (_min_adj_length2 != rhs._min_adj_length2)
    return false;

  if (_eol_width != rhs._eol_width)
    return false;

  if (_min_between_length != rhs._min_between_length)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerMinStepRule::operator<(
    const _dbTechLayerMinStepRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerMinStepRule::differences(
    dbDiff&                        diff,
    const char*                    field,
    const _dbTechLayerMinStepRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._max_edges_valid);
  DIFF_FIELD(_flags._min_adj_length1_valid);
  DIFF_FIELD(_flags._no_between_eol);
  DIFF_FIELD(_flags._min_adj_length2_valid);
  DIFF_FIELD(_flags._convex_corner);
  DIFF_FIELD(_flags._min_between_length_valid);
  DIFF_FIELD(_flags._except_same_corners);
  DIFF_FIELD(_min_step_length);
  DIFF_FIELD(_max_edges);
  DIFF_FIELD(_min_adj_length1);
  DIFF_FIELD(_min_adj_length2);
  DIFF_FIELD(_eol_width);
  DIFF_FIELD(_min_between_length);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerMinStepRule::out(dbDiff&     diff,
                                  char        side,
                                  const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._max_edges_valid);
  DIFF_OUT_FIELD(_flags._min_adj_length1_valid);
  DIFF_OUT_FIELD(_flags._no_between_eol);
  DIFF_OUT_FIELD(_flags._min_adj_length2_valid);
  DIFF_OUT_FIELD(_flags._convex_corner);
  DIFF_OUT_FIELD(_flags._min_between_length_valid);
  DIFF_OUT_FIELD(_flags._except_same_corners);
  DIFF_OUT_FIELD(_min_step_length);
  DIFF_OUT_FIELD(_max_edges);
  DIFF_OUT_FIELD(_min_adj_length1);
  DIFF_OUT_FIELD(_min_adj_length2);
  DIFF_OUT_FIELD(_eol_width);
  DIFF_OUT_FIELD(_min_between_length);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(
    _dbDatabase*                   db,
    const _dbTechLayerMinStepRule& r)
{
  _flags._max_edges_valid          = r._flags._max_edges_valid;
  _flags._min_adj_length1_valid    = r._flags._min_adj_length1_valid;
  _flags._no_between_eol           = r._flags._no_between_eol;
  _flags._min_adj_length2_valid    = r._flags._min_adj_length2_valid;
  _flags._convex_corner            = r._flags._convex_corner;
  _flags._min_between_length_valid = r._flags._min_between_length_valid;
  _flags._except_same_corners      = r._flags._except_same_corners;
  _flags._spare_bits               = r._flags._spare_bits;
  _min_step_length                 = r._min_step_length;
  _max_edges                       = r._max_edges;
  _min_adj_length1                 = r._min_adj_length1;
  _min_adj_length2                 = r._min_adj_length2;
  _eol_width                       = r._eol_width;
  _min_between_length              = r._min_between_length;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._min_step_length;
  stream >> obj._max_edges;
  stream >> obj._min_adj_length1;
  stream >> obj._min_adj_length2;
  stream >> obj._eol_width;
  stream >> obj._min_between_length;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinStepRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._min_step_length;
  stream << obj._max_edges;
  stream << obj._min_adj_length1;
  stream << obj._min_adj_length2;
  stream << obj._eol_width;
  stream << obj._min_between_length;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerMinStepRule::~_dbTechLayerMinStepRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerMinStepRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMinStepRule::setMinStepLength(int _min_step_length)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_min_step_length = _min_step_length;
}

int dbTechLayerMinStepRule::getMinStepLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_min_step_length;
}

void dbTechLayerMinStepRule::setMaxEdges(uint _max_edges)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_max_edges = _max_edges;
}

uint dbTechLayerMinStepRule::getMaxEdges() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_max_edges;
}

void dbTechLayerMinStepRule::setMinAdjLength1(int _min_adj_length1)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_min_adj_length1 = _min_adj_length1;
}

int dbTechLayerMinStepRule::getMinAdjLength1() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_min_adj_length1;
}

void dbTechLayerMinStepRule::setMinAdjLength2(int _min_adj_length2)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_min_adj_length2 = _min_adj_length2;
}

int dbTechLayerMinStepRule::getMinAdjLength2() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_min_adj_length2;
}

void dbTechLayerMinStepRule::setEolWidth(int _eol_width)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_eol_width = _eol_width;
}

int dbTechLayerMinStepRule::getEolWidth() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_eol_width;
}

void dbTechLayerMinStepRule::setMinBetweenLength(int _min_between_length)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_min_between_length = _min_between_length;
}

int dbTechLayerMinStepRule::getMinBetweenLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->_min_between_length;
}

void dbTechLayerMinStepRule::setMaxEdgesValid(bool _max_edges_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._max_edges_valid = _max_edges_valid;
}

bool dbTechLayerMinStepRule::isMaxEdgesValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._max_edges_valid;
}

void dbTechLayerMinStepRule::setMinAdjLength1Valid(bool _min_adj_length1_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._min_adj_length1_valid = _min_adj_length1_valid;
}

bool dbTechLayerMinStepRule::isMinAdjLength1Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._min_adj_length1_valid;
}

void dbTechLayerMinStepRule::setNoBetweenEol(bool _no_between_eol)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._no_between_eol = _no_between_eol;
}

bool dbTechLayerMinStepRule::isNoBetweenEol() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._no_between_eol;
}

void dbTechLayerMinStepRule::setMinAdjLength2Valid(bool _min_adj_length2_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._min_adj_length2_valid = _min_adj_length2_valid;
}

bool dbTechLayerMinStepRule::isMinAdjLength2Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._min_adj_length2_valid;
}

void dbTechLayerMinStepRule::setConvexCorner(bool _convex_corner)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._convex_corner = _convex_corner;
}

bool dbTechLayerMinStepRule::isConvexCorner() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._convex_corner;
}

void dbTechLayerMinStepRule::setMinBetweenLengthValid(
    bool _min_between_length_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._min_between_length_valid = _min_between_length_valid;
}

bool dbTechLayerMinStepRule::isMinBetweenLengthValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._min_between_length_valid;
}

void dbTechLayerMinStepRule::setExceptSameCorners(bool _except_same_corners)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags._except_same_corners = _except_same_corners;
}

bool dbTechLayerMinStepRule::isExceptSameCorners() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags._except_same_corners;
}

// User Code Begin dbTechLayerMinStepRulePublicMethods
dbTechLayerMinStepRule* dbTechLayerMinStepRule::create(dbTechLayer* _layer)
{
  _dbTechLayer*            layer   = (_dbTechLayer*) _layer;
  _dbTechLayerMinStepRule* newrule = layer->_minstep_rules_tbl->create();
  return ((dbTechLayerMinStepRule*) newrule);
}

dbTechLayerMinStepRule* dbTechLayerMinStepRule::getTechLayerMinStepRule(
    dbTechLayer* inly,
    uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerMinStepRule*) layer->_minstep_rules_tbl->getPtr(dbid);
}
void dbTechLayerMinStepRule::destroy(dbTechLayerMinStepRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_minstep_rules_tbl->destroy((_dbTechLayerMinStepRule*) rule);
}
// User Code End dbTechLayerMinStepRulePublicMethods
}  // namespace odb
   // Generator Code End 1