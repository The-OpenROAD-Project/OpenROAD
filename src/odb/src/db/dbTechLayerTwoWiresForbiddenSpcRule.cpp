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
#include "dbTechLayerTwoWiresForbiddenSpcRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerTwoWiresForbiddenSpcRule>;

bool _dbTechLayerTwoWiresForbiddenSpcRule::operator==(
    const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
{
  if (flags_.min_exact_span_length_ != rhs.flags_.min_exact_span_length_) {
    return false;
  }
  if (flags_.max_exact_span_length_ != rhs.flags_.max_exact_span_length_) {
    return false;
  }
  if (min_spacing_ != rhs.min_spacing_) {
    return false;
  }
  if (max_spacing_ != rhs.max_spacing_) {
    return false;
  }
  if (min_span_length_ != rhs.min_span_length_) {
    return false;
  }
  if (max_span_length_ != rhs.max_span_length_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }

  return true;
}

bool _dbTechLayerTwoWiresForbiddenSpcRule::operator<(
    const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
{
  return true;
}

void _dbTechLayerTwoWiresForbiddenSpcRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.min_exact_span_length_);
  DIFF_FIELD(flags_.max_exact_span_length_);
  DIFF_FIELD(min_spacing_);
  DIFF_FIELD(max_spacing_);
  DIFF_FIELD(min_span_length_);
  DIFF_FIELD(max_span_length_);
  DIFF_FIELD(prl_);
  DIFF_END
}

void _dbTechLayerTwoWiresForbiddenSpcRule::out(dbDiff& diff,
                                               char side,
                                               const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.min_exact_span_length_);
  DIFF_OUT_FIELD(flags_.max_exact_span_length_);
  DIFF_OUT_FIELD(min_spacing_);
  DIFF_OUT_FIELD(max_spacing_);
  DIFF_OUT_FIELD(min_span_length_);
  DIFF_OUT_FIELD(max_span_length_);
  DIFF_OUT_FIELD(prl_);

  DIFF_END
}

_dbTechLayerTwoWiresForbiddenSpcRule::_dbTechLayerTwoWiresForbiddenSpcRule(
    _dbDatabase* db)
{
  flags_ = {};
  min_spacing_ = 0;
  max_spacing_ = 0;
  min_span_length_ = 0;
  max_span_length_ = 0;
  prl_ = 0;
}

_dbTechLayerTwoWiresForbiddenSpcRule::_dbTechLayerTwoWiresForbiddenSpcRule(
    _dbDatabase* db,
    const _dbTechLayerTwoWiresForbiddenSpcRule& r)
{
  flags_.min_exact_span_length_ = r.flags_.min_exact_span_length_;
  flags_.max_exact_span_length_ = r.flags_.max_exact_span_length_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  min_spacing_ = r.min_spacing_;
  max_spacing_ = r.max_spacing_;
  min_span_length_ = r.min_span_length_;
  max_span_length_ = r.max_span_length_;
  prl_ = r.prl_;
}

dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerTwoWiresForbiddenSpcRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.min_spacing_;
  stream >> obj.max_spacing_;
  stream >> obj.min_span_length_;
  stream >> obj.max_span_length_;
  stream >> obj.prl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerTwoWiresForbiddenSpcRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.min_spacing_;
  stream << obj.max_spacing_;
  stream << obj.min_span_length_;
  stream << obj.max_span_length_;
  stream << obj.prl_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerTwoWiresForbiddenSpcRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerTwoWiresForbiddenSpcRule::setMinSpacing(int min_spacing)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->min_spacing_ = min_spacing;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMinSpacing() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->min_spacing_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpacing(int max_spacing)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->max_spacing_ = max_spacing;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMaxSpacing() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->max_spacing_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMinSpanLength(int min_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->min_span_length_ = min_span_length;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMinSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->min_span_length_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpanLength(int max_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->max_span_length_ = max_span_length;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getMaxSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->max_span_length_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setPrl(int prl)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->prl_ = prl;
}

int dbTechLayerTwoWiresForbiddenSpcRule::getPrl() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;
  return obj->prl_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMinExactSpanLength(
    bool min_exact_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->flags_.min_exact_span_length_ = min_exact_span_length;
}

bool dbTechLayerTwoWiresForbiddenSpcRule::isMinExactSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  return obj->flags_.min_exact_span_length_;
}

void dbTechLayerTwoWiresForbiddenSpcRule::setMaxExactSpanLength(
    bool max_exact_span_length)
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  obj->flags_.max_exact_span_length_ = max_exact_span_length;
}

bool dbTechLayerTwoWiresForbiddenSpcRule::isMaxExactSpanLength() const
{
  _dbTechLayerTwoWiresForbiddenSpcRule* obj
      = (_dbTechLayerTwoWiresForbiddenSpcRule*) this;

  return obj->flags_.max_exact_span_length_;
}

// User Code Begin dbTechLayerTwoWiresForbiddenSpcRulePublicMethods
dbTechLayerTwoWiresForbiddenSpcRule*
dbTechLayerTwoWiresForbiddenSpcRule::create(dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerTwoWiresForbiddenSpcRule* newrule
      = layer->two_wires_forbidden_spc_rules_tbl_->create();
  return ((dbTechLayerTwoWiresForbiddenSpcRule*) newrule);
}

void dbTechLayerTwoWiresForbiddenSpcRule::destroy(
    dbTechLayerTwoWiresForbiddenSpcRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->two_wires_forbidden_spc_rules_tbl_->destroy(
      (_dbTechLayerTwoWiresForbiddenSpcRule*) rule);
}
// User Code End dbTechLayerTwoWiresForbiddenSpcRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp