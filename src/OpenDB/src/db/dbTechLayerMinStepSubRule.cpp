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
#include "dbTechLayerMinStepSubRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
#include "dbTechLayerMinStepRule.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerMinStepSubRule>;

bool _dbTechLayerMinStepSubRule::operator==(
    const _dbTechLayerMinStepSubRule& rhs) const
{
  if (_flags._max_edges_valid != rhs._flags._max_edges_valid)
    return false;

  if (_flags._min_adj_length1_valid != rhs._flags._min_adj_length1_valid)
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

  if (_min_between_length != rhs._min_between_length)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerMinStepSubRule::operator<(
    const _dbTechLayerMinStepSubRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerMinStepSubRule::differences(
    dbDiff&                           diff,
    const char*                       field,
    const _dbTechLayerMinStepSubRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._max_edges_valid);
  DIFF_FIELD(_flags._min_adj_length1_valid);
  DIFF_FIELD(_flags._min_adj_length2_valid);
  DIFF_FIELD(_flags._convex_corner);
  DIFF_FIELD(_flags._min_between_length_valid);
  DIFF_FIELD(_flags._except_same_corners);
  DIFF_FIELD(_min_step_length);
  DIFF_FIELD(_max_edges);
  DIFF_FIELD(_min_adj_length1);
  DIFF_FIELD(_min_adj_length2);
  DIFF_FIELD(_min_between_length);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerMinStepSubRule::out(dbDiff&     diff,
                                     char        side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._max_edges_valid);
  DIFF_OUT_FIELD(_flags._min_adj_length1_valid);
  DIFF_OUT_FIELD(_flags._min_adj_length2_valid);
  DIFF_OUT_FIELD(_flags._convex_corner);
  DIFF_OUT_FIELD(_flags._min_between_length_valid);
  DIFF_OUT_FIELD(_flags._except_same_corners);
  DIFF_OUT_FIELD(_min_step_length);
  DIFF_OUT_FIELD(_max_edges);
  DIFF_OUT_FIELD(_min_adj_length1);
  DIFF_OUT_FIELD(_min_adj_length2);
  DIFF_OUT_FIELD(_min_between_length);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerMinStepSubRule::_dbTechLayerMinStepSubRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerMinStepSubRule::_dbTechLayerMinStepSubRule(
    _dbDatabase*                      db,
    const _dbTechLayerMinStepSubRule& r)
{
  _flags._max_edges_valid          = r._flags._max_edges_valid;
  _flags._min_adj_length1_valid    = r._flags._min_adj_length1_valid;
  _flags._min_adj_length2_valid    = r._flags._min_adj_length2_valid;
  _flags._convex_corner            = r._flags._convex_corner;
  _flags._min_between_length_valid = r._flags._min_between_length_valid;
  _flags._except_same_corners      = r._flags._except_same_corners;
  _flags._spare_bits               = r._flags._spare_bits;
  _min_step_length                 = r._min_step_length;
  _max_edges                       = r._max_edges;
  _min_adj_length1                 = r._min_adj_length1;
  _min_adj_length2                 = r._min_adj_length2;
  _min_between_length              = r._min_between_length;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._min_step_length;
  stream >> obj._max_edges;
  stream >> obj._min_adj_length1;
  stream >> obj._min_adj_length2;
  stream >> obj._min_between_length;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinStepSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._min_step_length;
  stream << obj._max_edges;
  stream << obj._min_adj_length1;
  stream << obj._min_adj_length2;
  stream << obj._min_between_length;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerMinStepSubRule::~_dbTechLayerMinStepSubRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerMinStepSubRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMinStepSubRule::setMinStepLength(int _min_step_length)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_min_step_length = _min_step_length;
}

int dbTechLayerMinStepSubRule::getMinStepLength() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;
  return obj->_min_step_length;
}

void dbTechLayerMinStepSubRule::setMaxEdges(uint _max_edges)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_max_edges = _max_edges;
}

uint dbTechLayerMinStepSubRule::getMaxEdges() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;
  return obj->_max_edges;
}

void dbTechLayerMinStepSubRule::setMinAdjLength1(int _min_adj_length1)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_min_adj_length1 = _min_adj_length1;
}

int dbTechLayerMinStepSubRule::getMinAdjLength1() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;
  return obj->_min_adj_length1;
}

void dbTechLayerMinStepSubRule::setMinAdjLength2(int _min_adj_length2)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_min_adj_length2 = _min_adj_length2;
}

int dbTechLayerMinStepSubRule::getMinAdjLength2() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;
  return obj->_min_adj_length2;
}

void dbTechLayerMinStepSubRule::setMinBetweenLength(int _min_between_length)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_min_between_length = _min_between_length;
}

int dbTechLayerMinStepSubRule::getMinBetweenLength() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;
  return obj->_min_between_length;
}

void dbTechLayerMinStepSubRule::setMaxEdgesValid(bool _max_edges_valid)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._max_edges_valid = _max_edges_valid;
}

bool dbTechLayerMinStepSubRule::isMaxEdgesValid() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._max_edges_valid;
}

void dbTechLayerMinStepSubRule::setMinAdjLength1Valid(
    bool _min_adj_length1_valid)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._min_adj_length1_valid = _min_adj_length1_valid;
}

bool dbTechLayerMinStepSubRule::isMinAdjLength1Valid() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._min_adj_length1_valid;
}

void dbTechLayerMinStepSubRule::setMinAdjLength2Valid(
    bool _min_adj_length2_valid)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._min_adj_length2_valid = _min_adj_length2_valid;
}

bool dbTechLayerMinStepSubRule::isMinAdjLength2Valid() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._min_adj_length2_valid;
}

void dbTechLayerMinStepSubRule::setConvexCorner(bool _convex_corner)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._convex_corner = _convex_corner;
}

bool dbTechLayerMinStepSubRule::isConvexCorner() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._convex_corner;
}

void dbTechLayerMinStepSubRule::setMinBetweenLengthValid(
    bool _min_between_length_valid)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._min_between_length_valid = _min_between_length_valid;
}

bool dbTechLayerMinStepSubRule::isMinBetweenLengthValid() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._min_between_length_valid;
}

void dbTechLayerMinStepSubRule::setExceptSameCorners(bool _except_same_corners)
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  obj->_flags._except_same_corners = _except_same_corners;
}

bool dbTechLayerMinStepSubRule::isExceptSameCorners() const
{
  _dbTechLayerMinStepSubRule* obj = (_dbTechLayerMinStepSubRule*) this;

  return obj->_flags._except_same_corners;
}

// User Code Begin dbTechLayerMinStepSubRulePublicMethods

dbTechLayerMinStepSubRule* dbTechLayerMinStepSubRule::create(
    dbTechLayerMinStepRule* parent)
{
  _dbTechLayerMinStepRule*    _parent = (_dbTechLayerMinStepRule*) parent;
  _dbTechLayerMinStepSubRule* newrule
      = _parent->_techlayerminstepsubrule_tbl->create();
  return ((dbTechLayerMinStepSubRule*) newrule);
}

dbTechLayerMinStepSubRule*
dbTechLayerMinStepSubRule::getTechLayerMinStepSubRule(
    dbTechLayerMinStepRule* parent,
    uint                    dbid)
{
  _dbTechLayerMinStepRule* _parent = (_dbTechLayerMinStepRule*) parent;
  return (dbTechLayerMinStepSubRule*)
      _parent->_techlayerminstepsubrule_tbl->getPtr(dbid);
}

void dbTechLayerMinStepSubRule::destroy(dbTechLayerMinStepSubRule* rule)
{
  _dbTechLayerMinStepRule* _parent
      = (_dbTechLayerMinStepRule*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_techlayerminstepsubrule_tbl->destroy(
      (_dbTechLayerMinStepSubRule*) rule);
}

// User Code End dbTechLayerMinStepSubRulePublicMethods
}  // namespace odb
   // Generator Code End 1