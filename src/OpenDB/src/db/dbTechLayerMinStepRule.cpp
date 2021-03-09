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

// Generator Code Begin cpp
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
  if (_flags.max_edges_valid_ != rhs._flags.max_edges_valid_)
    return false;

  if (_flags.min_adj_length1_valid_ != rhs._flags.min_adj_length1_valid_)
    return false;

  if (_flags.no_between_eol_ != rhs._flags.no_between_eol_)
    return false;

  if (_flags.min_adj_length2_valid_ != rhs._flags.min_adj_length2_valid_)
    return false;

  if (_flags.convex_corner_ != rhs._flags.convex_corner_)
    return false;

  if (_flags.min_between_length_valid_ != rhs._flags.min_between_length_valid_)
    return false;

  if (_flags.except_same_corners_ != rhs._flags.except_same_corners_)
    return false;

  if (min_step_length_ != rhs.min_step_length_)
    return false;

  if (max_edges_ != rhs.max_edges_)
    return false;

  if (min_adj_length1_ != rhs.min_adj_length1_)
    return false;

  if (min_adj_length2_ != rhs.min_adj_length2_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (min_between_length_ != rhs.min_between_length_)
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

  DIFF_FIELD(_flags.max_edges_valid_);
  DIFF_FIELD(_flags.min_adj_length1_valid_);
  DIFF_FIELD(_flags.no_between_eol_);
  DIFF_FIELD(_flags.min_adj_length2_valid_);
  DIFF_FIELD(_flags.convex_corner_);
  DIFF_FIELD(_flags.min_between_length_valid_);
  DIFF_FIELD(_flags.except_same_corners_);
  DIFF_FIELD(min_step_length_);
  DIFF_FIELD(max_edges_);
  DIFF_FIELD(min_adj_length1_);
  DIFF_FIELD(min_adj_length2_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(min_between_length_);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerMinStepRule::out(dbDiff&     diff,
                                  char        side,
                                  const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags.max_edges_valid_);
  DIFF_OUT_FIELD(_flags.min_adj_length1_valid_);
  DIFF_OUT_FIELD(_flags.no_between_eol_);
  DIFF_OUT_FIELD(_flags.min_adj_length2_valid_);
  DIFF_OUT_FIELD(_flags.convex_corner_);
  DIFF_OUT_FIELD(_flags.min_between_length_valid_);
  DIFF_OUT_FIELD(_flags.except_same_corners_);
  DIFF_OUT_FIELD(min_step_length_);
  DIFF_OUT_FIELD(max_edges_);
  DIFF_OUT_FIELD(min_adj_length1_);
  DIFF_OUT_FIELD(min_adj_length2_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(min_between_length_);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(_dbDatabase* db)
{
  uint32_t* _flags_bit_field = (uint32_t*) &_flags;
  *_flags_bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(
    _dbDatabase*                   db,
    const _dbTechLayerMinStepRule& r)
{
  _flags.max_edges_valid_          = r._flags.max_edges_valid_;
  _flags.min_adj_length1_valid_    = r._flags.min_adj_length1_valid_;
  _flags.no_between_eol_           = r._flags.no_between_eol_;
  _flags.min_adj_length2_valid_    = r._flags.min_adj_length2_valid_;
  _flags.convex_corner_            = r._flags.convex_corner_;
  _flags.min_between_length_valid_ = r._flags.min_between_length_valid_;
  _flags.except_same_corners_      = r._flags.except_same_corners_;
  _flags._spare_bits               = r._flags._spare_bits;
  min_step_length_                 = r.min_step_length_;
  max_edges_                       = r.max_edges_;
  min_adj_length1_                 = r.min_adj_length1_;
  min_adj_length2_                 = r.min_adj_length2_;
  eol_width_                       = r.eol_width_;
  min_between_length_              = r.min_between_length_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj.min_step_length_;
  stream >> obj.max_edges_;
  stream >> obj.min_adj_length1_;
  stream >> obj.min_adj_length2_;
  stream >> obj.eol_width_;
  stream >> obj.min_between_length_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinStepRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj.min_step_length_;
  stream << obj.max_edges_;
  stream << obj.min_adj_length1_;
  stream << obj.min_adj_length2_;
  stream << obj.eol_width_;
  stream << obj.min_between_length_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerMinStepRule::~_dbTechLayerMinStepRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerMinStepRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMinStepRule::setMinStepLength(int min_step_length_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_step_length_ = min_step_length_;
}

int dbTechLayerMinStepRule::getMinStepLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_step_length_;
}

void dbTechLayerMinStepRule::setMaxEdges(uint max_edges_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->max_edges_ = max_edges_;
}

uint dbTechLayerMinStepRule::getMaxEdges() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->max_edges_;
}

void dbTechLayerMinStepRule::setMinAdjLength1(int min_adj_length1_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_adj_length1_ = min_adj_length1_;
}

int dbTechLayerMinStepRule::getMinAdjLength1() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_adj_length1_;
}

void dbTechLayerMinStepRule::setMinAdjLength2(int min_adj_length2_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_adj_length2_ = min_adj_length2_;
}

int dbTechLayerMinStepRule::getMinAdjLength2() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_adj_length2_;
}

void dbTechLayerMinStepRule::setEolWidth(int eol_width_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->eol_width_ = eol_width_;
}

int dbTechLayerMinStepRule::getEolWidth() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->eol_width_;
}

void dbTechLayerMinStepRule::setMinBetweenLength(int min_between_length_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_between_length_ = min_between_length_;
}

int dbTechLayerMinStepRule::getMinBetweenLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_between_length_;
}

void dbTechLayerMinStepRule::setMaxEdgesValid(bool max_edges_valid_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.max_edges_valid_ = max_edges_valid_;
}

bool dbTechLayerMinStepRule::isMaxEdgesValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.max_edges_valid_;
}

void dbTechLayerMinStepRule::setMinAdjLength1Valid(bool min_adj_length1_valid_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.min_adj_length1_valid_ = min_adj_length1_valid_;
}

bool dbTechLayerMinStepRule::isMinAdjLength1Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.min_adj_length1_valid_;
}

void dbTechLayerMinStepRule::setNoBetweenEol(bool no_between_eol_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.no_between_eol_ = no_between_eol_;
}

bool dbTechLayerMinStepRule::isNoBetweenEol() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.no_between_eol_;
}

void dbTechLayerMinStepRule::setMinAdjLength2Valid(bool min_adj_length2_valid_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.min_adj_length2_valid_ = min_adj_length2_valid_;
}

bool dbTechLayerMinStepRule::isMinAdjLength2Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.min_adj_length2_valid_;
}

void dbTechLayerMinStepRule::setConvexCorner(bool convex_corner_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.convex_corner_ = convex_corner_;
}

bool dbTechLayerMinStepRule::isConvexCorner() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.convex_corner_;
}

void dbTechLayerMinStepRule::setMinBetweenLengthValid(
    bool min_between_length_valid_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.min_between_length_valid_ = min_between_length_valid_;
}

bool dbTechLayerMinStepRule::isMinBetweenLengthValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.min_between_length_valid_;
}

void dbTechLayerMinStepRule::setExceptSameCorners(bool except_same_corners_)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->_flags.except_same_corners_ = except_same_corners_;
}

bool dbTechLayerMinStepRule::isExceptSameCorners() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->_flags.except_same_corners_;
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
   // Generator Code End cpp