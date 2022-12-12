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
#include "dbTechLayerWidthTableRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerWidthTableRule>;

bool _dbTechLayerWidthTableRule::operator==(
    const _dbTechLayerWidthTableRule& rhs) const
{
  if (flags_.wrong_direction_ != rhs.flags_.wrong_direction_)
    return false;

  if (flags_.orthogonal_ != rhs.flags_.orthogonal_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerWidthTableRule::operator<(
    const _dbTechLayerWidthTableRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerWidthTableRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerWidthTableRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.wrong_direction_);
  DIFF_FIELD(flags_.orthogonal_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerWidthTableRule::out(dbDiff& diff,
                                     char side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.wrong_direction_);
  DIFF_OUT_FIELD(flags_.orthogonal_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerWidthTableRule::_dbTechLayerWidthTableRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerWidthTableRule::_dbTechLayerWidthTableRule(
    _dbDatabase* db,
    const _dbTechLayerWidthTableRule& r)
{
  flags_.wrong_direction_ = r.flags_.wrong_direction_;
  flags_.orthogonal_ = r.flags_.orthogonal_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerWidthTableRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.width_tbl_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerWidthTableRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.width_tbl_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerWidthTableRule::~_dbTechLayerWidthTableRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerWidthTableRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerWidthTableRule::setWrongDirection(bool wrong_direction)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  obj->flags_.wrong_direction_ = wrong_direction;
}

bool dbTechLayerWidthTableRule::isWrongDirection() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  return obj->flags_.wrong_direction_;
}

void dbTechLayerWidthTableRule::setOrthogonal(bool orthogonal)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  obj->flags_.orthogonal_ = orthogonal;
}

bool dbTechLayerWidthTableRule::isOrthogonal() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;

  return obj->flags_.orthogonal_;
}

// User Code Begin dbTechLayerWidthTableRulePublicMethods

void dbTechLayerWidthTableRule::addWidth(int width)
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;
  obj->width_tbl_.push_back(width);
}

std::vector<int> dbTechLayerWidthTableRule::getWidthTable() const
{
  _dbTechLayerWidthTableRule* obj = (_dbTechLayerWidthTableRule*) this;
  return obj->width_tbl_;
}

dbTechLayerWidthTableRule* dbTechLayerWidthTableRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerWidthTableRule* newrule = layer->width_table_rules_tbl_->create();
  return ((dbTechLayerWidthTableRule*) newrule);
}

dbTechLayerWidthTableRule*
dbTechLayerWidthTableRule::getTechLayerWidthTableRule(dbTechLayer* inly,
                                                      uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (
      (dbTechLayerWidthTableRule*) layer->width_table_rules_tbl_->getPtr(dbid));
}

void dbTechLayerWidthTableRule::destroy(dbTechLayerWidthTableRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  if (rule->isWrongDirection()) {
    // reset wrong way width
    layer->wrong_way_width_ = layer->_width;
  }
  dbProperty::destroyProperties(rule);
  layer->width_table_rules_tbl_->destroy((_dbTechLayerWidthTableRule*) rule);
}

// User Code End dbTechLayerWidthTableRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp