///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbTechLayerMinStepRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerMinStepRule>;

bool _dbTechLayerMinStepRule::operator==(
    const _dbTechLayerMinStepRule& rhs) const
{
  if (flags_.max_edges_valid_ != rhs.flags_.max_edges_valid_)
    return false;

  if (flags_.min_adj_length1_valid_ != rhs.flags_.min_adj_length1_valid_)
    return false;

  if (flags_.no_between_eol_ != rhs.flags_.no_between_eol_)
    return false;

  if (flags_.min_adj_length2_valid_ != rhs.flags_.min_adj_length2_valid_)
    return false;

  if (flags_.convex_corner_ != rhs.flags_.convex_corner_)
    return false;

  if (flags_.min_between_length_valid_ != rhs.flags_.min_between_length_valid_)
    return false;

  if (flags_.except_same_corners_ != rhs.flags_.except_same_corners_)
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
    dbDiff& diff,
    const char* field,
    const _dbTechLayerMinStepRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.max_edges_valid_);
  DIFF_FIELD(flags_.min_adj_length1_valid_);
  DIFF_FIELD(flags_.no_between_eol_);
  DIFF_FIELD(flags_.min_adj_length2_valid_);
  DIFF_FIELD(flags_.convex_corner_);
  DIFF_FIELD(flags_.min_between_length_valid_);
  DIFF_FIELD(flags_.except_same_corners_);
  DIFF_FIELD(min_step_length_);
  DIFF_FIELD(max_edges_);
  DIFF_FIELD(min_adj_length1_);
  DIFF_FIELD(min_adj_length2_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(min_between_length_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerMinStepRule::out(dbDiff& diff,
                                  char side,
                                  const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.max_edges_valid_);
  DIFF_OUT_FIELD(flags_.min_adj_length1_valid_);
  DIFF_OUT_FIELD(flags_.no_between_eol_);
  DIFF_OUT_FIELD(flags_.min_adj_length2_valid_);
  DIFF_OUT_FIELD(flags_.convex_corner_);
  DIFF_OUT_FIELD(flags_.min_between_length_valid_);
  DIFF_OUT_FIELD(flags_.except_same_corners_);
  DIFF_OUT_FIELD(min_step_length_);
  DIFF_OUT_FIELD(max_edges_);
  DIFF_OUT_FIELD(min_adj_length1_);
  DIFF_OUT_FIELD(min_adj_length2_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(min_between_length_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerMinStepRule::_dbTechLayerMinStepRule(
    _dbDatabase* db,
    const _dbTechLayerMinStepRule& r)
{
  flags_.max_edges_valid_ = r.flags_.max_edges_valid_;
  flags_.min_adj_length1_valid_ = r.flags_.min_adj_length1_valid_;
  flags_.no_between_eol_ = r.flags_.no_between_eol_;
  flags_.min_adj_length2_valid_ = r.flags_.min_adj_length2_valid_;
  flags_.convex_corner_ = r.flags_.convex_corner_;
  flags_.min_between_length_valid_ = r.flags_.min_between_length_valid_;
  flags_.except_same_corners_ = r.flags_.except_same_corners_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  min_step_length_ = r.min_step_length_;
  max_edges_ = r.max_edges_;
  min_adj_length1_ = r.min_adj_length1_;
  min_adj_length2_ = r.min_adj_length2_;
  eol_width_ = r.eol_width_;
  min_between_length_ = r.min_between_length_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
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
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
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

void dbTechLayerMinStepRule::setMinStepLength(int min_step_length)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_step_length_ = min_step_length;
}

int dbTechLayerMinStepRule::getMinStepLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_step_length_;
}

void dbTechLayerMinStepRule::setMaxEdges(uint max_edges)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->max_edges_ = max_edges;
}

uint dbTechLayerMinStepRule::getMaxEdges() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->max_edges_;
}

void dbTechLayerMinStepRule::setMinAdjLength1(int min_adj_length1)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_adj_length1_ = min_adj_length1;
}

int dbTechLayerMinStepRule::getMinAdjLength1() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_adj_length1_;
}

void dbTechLayerMinStepRule::setMinAdjLength2(int min_adj_length2)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_adj_length2_ = min_adj_length2;
}

int dbTechLayerMinStepRule::getMinAdjLength2() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_adj_length2_;
}

void dbTechLayerMinStepRule::setEolWidth(int eol_width)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerMinStepRule::getEolWidth() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->eol_width_;
}

void dbTechLayerMinStepRule::setMinBetweenLength(int min_between_length)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->min_between_length_ = min_between_length;
}

int dbTechLayerMinStepRule::getMinBetweenLength() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;
  return obj->min_between_length_;
}

void dbTechLayerMinStepRule::setMaxEdgesValid(bool max_edges_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.max_edges_valid_ = max_edges_valid;
}

bool dbTechLayerMinStepRule::isMaxEdgesValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.max_edges_valid_;
}

void dbTechLayerMinStepRule::setMinAdjLength1Valid(bool min_adj_length1_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.min_adj_length1_valid_ = min_adj_length1_valid;
}

bool dbTechLayerMinStepRule::isMinAdjLength1Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.min_adj_length1_valid_;
}

void dbTechLayerMinStepRule::setNoBetweenEol(bool no_between_eol)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.no_between_eol_ = no_between_eol;
}

bool dbTechLayerMinStepRule::isNoBetweenEol() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.no_between_eol_;
}

void dbTechLayerMinStepRule::setMinAdjLength2Valid(bool min_adj_length2_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.min_adj_length2_valid_ = min_adj_length2_valid;
}

bool dbTechLayerMinStepRule::isMinAdjLength2Valid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.min_adj_length2_valid_;
}

void dbTechLayerMinStepRule::setConvexCorner(bool convex_corner)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.convex_corner_ = convex_corner;
}

bool dbTechLayerMinStepRule::isConvexCorner() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.convex_corner_;
}

void dbTechLayerMinStepRule::setMinBetweenLengthValid(
    bool min_between_length_valid)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.min_between_length_valid_ = min_between_length_valid;
}

bool dbTechLayerMinStepRule::isMinBetweenLengthValid() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.min_between_length_valid_;
}

void dbTechLayerMinStepRule::setExceptSameCorners(bool except_same_corners)
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  obj->flags_.except_same_corners_ = except_same_corners;
}

bool dbTechLayerMinStepRule::isExceptSameCorners() const
{
  _dbTechLayerMinStepRule* obj = (_dbTechLayerMinStepRule*) this;

  return obj->flags_.except_same_corners_;
}

// User Code Begin dbTechLayerMinStepRulePublicMethods
dbTechLayerMinStepRule* dbTechLayerMinStepRule::create(dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerMinStepRule* newrule = layer->minstep_rules_tbl_->create();
  return ((dbTechLayerMinStepRule*) newrule);
}

dbTechLayerMinStepRule* dbTechLayerMinStepRule::getTechLayerMinStepRule(
    dbTechLayer* inly,
    uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerMinStepRule*) layer->minstep_rules_tbl_->getPtr(dbid);
}
void dbTechLayerMinStepRule::destroy(dbTechLayerMinStepRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->minstep_rules_tbl_->destroy((_dbTechLayerMinStepRule*) rule);
}
// User Code End dbTechLayerMinStepRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
