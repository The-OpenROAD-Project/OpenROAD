///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "dbTechLayerWrongDirSpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerWrongDirSpacingRule>;

bool _dbTechLayerWrongDirSpacingRule::operator==(
    const _dbTechLayerWrongDirSpacingRule& rhs) const
{
  if (flags_.noneol_valid_ != rhs.flags_.noneol_valid_) {
    return false;
  }
  if (flags_.length_valid_ != rhs.flags_.length_valid_) {
    return false;
  }
  if (wrongdir_space_ != rhs.wrongdir_space_) {
    return false;
  }
  if (noneol_width_ != rhs.noneol_width_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (prl_length_ != rhs.prl_length_) {
    return false;
  }

  return true;
}

bool _dbTechLayerWrongDirSpacingRule::operator<(
    const _dbTechLayerWrongDirSpacingRule& rhs) const
{
  return true;
}

void _dbTechLayerWrongDirSpacingRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerWrongDirSpacingRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.noneol_valid_);
  DIFF_FIELD(flags_.length_valid_);
  DIFF_FIELD(wrongdir_space_);
  DIFF_FIELD(noneol_width_);
  DIFF_FIELD(length_);
  DIFF_FIELD(prl_length_);
  DIFF_END
}

void _dbTechLayerWrongDirSpacingRule::out(dbDiff& diff,
                                          char side,
                                          const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.noneol_valid_);
  DIFF_OUT_FIELD(flags_.length_valid_);
  DIFF_OUT_FIELD(wrongdir_space_);
  DIFF_OUT_FIELD(noneol_width_);
  DIFF_OUT_FIELD(length_);
  DIFF_OUT_FIELD(prl_length_);

  DIFF_END
}

_dbTechLayerWrongDirSpacingRule::_dbTechLayerWrongDirSpacingRule(
    _dbDatabase* db)
{
  flags_ = {};
  wrongdir_space_ = 0;
  noneol_width_ = 0;
  length_ = 0;
  prl_length_ = 0;
}

_dbTechLayerWrongDirSpacingRule::_dbTechLayerWrongDirSpacingRule(
    _dbDatabase* db,
    const _dbTechLayerWrongDirSpacingRule& r)
{
  flags_.noneol_valid_ = r.flags_.noneol_valid_;
  flags_.length_valid_ = r.flags_.length_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  wrongdir_space_ = r.wrongdir_space_;
  noneol_width_ = r.noneol_width_;
  length_ = r.length_;
  prl_length_ = r.prl_length_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerWrongDirSpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.wrongdir_space_;
  stream >> obj.noneol_width_;
  stream >> obj.length_;
  stream >> obj.prl_length_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerWrongDirSpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.wrongdir_space_;
  stream << obj.noneol_width_;
  stream << obj.length_;
  stream << obj.prl_length_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerWrongDirSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerWrongDirSpacingRule::setWrongdirSpace(int wrongdir_space)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->wrongdir_space_ = wrongdir_space;
}

int dbTechLayerWrongDirSpacingRule::getWrongdirSpace() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->wrongdir_space_;
}

void dbTechLayerWrongDirSpacingRule::setNoneolWidth(int noneol_width)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->noneol_width_ = noneol_width;
}

int dbTechLayerWrongDirSpacingRule::getNoneolWidth() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->noneol_width_;
}

void dbTechLayerWrongDirSpacingRule::setLength(int length)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->length_ = length;
}

int dbTechLayerWrongDirSpacingRule::getLength() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->length_;
}

void dbTechLayerWrongDirSpacingRule::setPrlLength(int prl_length)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->prl_length_ = prl_length;
}

int dbTechLayerWrongDirSpacingRule::getPrlLength() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;
  return obj->prl_length_;
}

void dbTechLayerWrongDirSpacingRule::setNoneolValid(bool noneol_valid)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->flags_.noneol_valid_ = noneol_valid;
}

bool dbTechLayerWrongDirSpacingRule::isNoneolValid() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  return obj->flags_.noneol_valid_;
}

void dbTechLayerWrongDirSpacingRule::setLengthValid(bool length_valid)
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  obj->flags_.length_valid_ = length_valid;
}

bool dbTechLayerWrongDirSpacingRule::isLengthValid() const
{
  _dbTechLayerWrongDirSpacingRule* obj
      = (_dbTechLayerWrongDirSpacingRule*) this;

  return obj->flags_.length_valid_;
}

// User Code Begin dbTechLayerWrongDirSpacingRulePublicMethods
dbTechLayerWrongDirSpacingRule* dbTechLayerWrongDirSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerWrongDirSpacingRule* newrule
      = layer->wrongdir_spacing_rules_tbl_->create();
  newrule->_layer = _layer->getImpl()->getOID();

  return ((dbTechLayerWrongDirSpacingRule*) newrule);
}

dbTechLayerWrongDirSpacingRule*
dbTechLayerWrongDirSpacingRule::getTechLayerWrongDirSpacingRule(
    dbTechLayer* inly,
    uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerWrongDirSpacingRule*)
      layer->wrongdir_spacing_rules_tbl_->getPtr(dbid);
}

void dbTechLayerWrongDirSpacingRule::destroy(
    dbTechLayerWrongDirSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->wrongdir_spacing_rules_tbl_->destroy(
      (_dbTechLayerWrongDirSpacingRule*) rule);
}
// User Code End dbTechLayerWrongDirSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp