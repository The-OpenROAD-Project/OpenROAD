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
#include "dbTechLayerEolKeepOutRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerEolKeepOutRule>;

bool _dbTechLayerEolKeepOutRule::operator==(
    const _dbTechLayerEolKeepOutRule& rhs) const
{
  if (flags_.class_valid_ != rhs.flags_.class_valid_)
    return false;

  if (flags_.corner_only_ != rhs.flags_.corner_only_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (backward_ext_ != rhs.backward_ext_)
    return false;

  if (forward_ext_ != rhs.forward_ext_)
    return false;

  if (side_ext_ != rhs.side_ext_)
    return false;

  if (class_name_ != rhs.class_name_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerEolKeepOutRule::operator<(
    const _dbTechLayerEolKeepOutRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerEolKeepOutRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerEolKeepOutRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.class_valid_);
  DIFF_FIELD(flags_.corner_only_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(backward_ext_);
  DIFF_FIELD(forward_ext_);
  DIFF_FIELD(side_ext_);
  DIFF_FIELD(class_name_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerEolKeepOutRule::out(dbDiff& diff,
                                     char side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.class_valid_);
  DIFF_OUT_FIELD(flags_.corner_only_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(backward_ext_);
  DIFF_OUT_FIELD(forward_ext_);
  DIFF_OUT_FIELD(side_ext_);
  DIFF_OUT_FIELD(class_name_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerEolKeepOutRule::_dbTechLayerEolKeepOutRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerEolKeepOutRule::_dbTechLayerEolKeepOutRule(
    _dbDatabase* db,
    const _dbTechLayerEolKeepOutRule& r)
{
  flags_.class_valid_ = r.flags_.class_valid_;
  flags_.corner_only_ = r.flags_.corner_only_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  eol_width_ = r.eol_width_;
  backward_ext_ = r.backward_ext_;
  forward_ext_ = r.forward_ext_;
  side_ext_ = r.side_ext_;
  class_name_ = r.class_name_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolKeepOutRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.eol_width_;
  stream >> obj.backward_ext_;
  stream >> obj.forward_ext_;
  stream >> obj.side_ext_;
  stream >> obj.class_name_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerEolKeepOutRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.eol_width_;
  stream << obj.backward_ext_;
  stream << obj.forward_ext_;
  stream << obj.side_ext_;
  stream << obj.class_name_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerEolKeepOutRule::~_dbTechLayerEolKeepOutRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerEolKeepOutRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerEolKeepOutRule::setEolWidth(int eol_width)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerEolKeepOutRule::getEolWidth() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->eol_width_;
}

void dbTechLayerEolKeepOutRule::setBackwardExt(int backward_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->backward_ext_ = backward_ext;
}

int dbTechLayerEolKeepOutRule::getBackwardExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->backward_ext_;
}

void dbTechLayerEolKeepOutRule::setForwardExt(int forward_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->forward_ext_ = forward_ext;
}

int dbTechLayerEolKeepOutRule::getForwardExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->forward_ext_;
}

void dbTechLayerEolKeepOutRule::setSideExt(int side_ext)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->side_ext_ = side_ext;
}

int dbTechLayerEolKeepOutRule::getSideExt() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->side_ext_;
}

void dbTechLayerEolKeepOutRule::setClassName(std::string class_name)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->class_name_ = class_name;
}

std::string dbTechLayerEolKeepOutRule::getClassName() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;
  return obj->class_name_;
}

void dbTechLayerEolKeepOutRule::setClassValid(bool class_valid)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->flags_.class_valid_ = class_valid;
}

bool dbTechLayerEolKeepOutRule::isClassValid() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  return obj->flags_.class_valid_;
}

void dbTechLayerEolKeepOutRule::setCornerOnly(bool corner_only)
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  obj->flags_.corner_only_ = corner_only;
}

bool dbTechLayerEolKeepOutRule::isCornerOnly() const
{
  _dbTechLayerEolKeepOutRule* obj = (_dbTechLayerEolKeepOutRule*) this;

  return obj->flags_.corner_only_;
}

// User Code Begin dbTechLayerEolKeepOutRulePublicMethods

dbTechLayerEolKeepOutRule* dbTechLayerEolKeepOutRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerEolKeepOutRule* newrule
      = layer->eol_keep_out_rules_tbl_->create();
  return ((dbTechLayerEolKeepOutRule*) newrule);
}

dbTechLayerEolKeepOutRule*
dbTechLayerEolKeepOutRule::getTechLayerEolKeepOutRule(dbTechLayer* inly,
                                                      uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerEolKeepOutRule*) layer->eol_keep_out_rules_tbl_->getPtr(
      dbid);
}
void dbTechLayerEolKeepOutRule::destroy(dbTechLayerEolKeepOutRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->eol_keep_out_rules_tbl_->destroy((_dbTechLayerEolKeepOutRule*) rule);
}

// User Code End dbTechLayerEolKeepOutRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
