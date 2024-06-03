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
#include "dbTechLayerArraySpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerArraySpacingRule>;

bool _dbTechLayerArraySpacingRule::operator==(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  if (flags_.parallel_overlap_ != rhs.flags_.parallel_overlap_) {
    return false;
  }
  if (flags_.long_array_ != rhs.flags_.long_array_) {
    return false;
  }
  if (flags_.via_width_valid_ != rhs.flags_.via_width_valid_) {
    return false;
  }
  if (flags_.within_valid_ != rhs.flags_.within_valid_) {
    return false;
  }
  if (via_width_ != rhs.via_width_) {
    return false;
  }
  if (cut_spacing_ != rhs.cut_spacing_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (array_width_ != rhs.array_width_) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }

  return true;
}

bool _dbTechLayerArraySpacingRule::operator<(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  return true;
}

void _dbTechLayerArraySpacingRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerArraySpacingRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.parallel_overlap_);
  DIFF_FIELD(flags_.long_array_);
  DIFF_FIELD(flags_.via_width_valid_);
  DIFF_FIELD(flags_.within_valid_);
  DIFF_FIELD(via_width_);
  DIFF_FIELD(cut_spacing_);
  DIFF_FIELD(within_);
  DIFF_FIELD(array_width_);
  DIFF_FIELD(cut_class_);
  DIFF_END
}

void _dbTechLayerArraySpacingRule::out(dbDiff& diff,
                                       char side,
                                       const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.parallel_overlap_);
  DIFF_OUT_FIELD(flags_.long_array_);
  DIFF_OUT_FIELD(flags_.via_width_valid_);
  DIFF_OUT_FIELD(flags_.within_valid_);
  DIFF_OUT_FIELD(via_width_);
  DIFF_OUT_FIELD(cut_spacing_);
  DIFF_OUT_FIELD(within_);
  DIFF_OUT_FIELD(array_width_);
  DIFF_OUT_FIELD(cut_class_);

  DIFF_END
}

_dbTechLayerArraySpacingRule::_dbTechLayerArraySpacingRule(_dbDatabase* db)
{
  flags_ = {};
  via_width_ = 0;
  cut_spacing_ = 0;
  within_ = 0;
  array_width_ = 0;
}

_dbTechLayerArraySpacingRule::_dbTechLayerArraySpacingRule(
    _dbDatabase* db,
    const _dbTechLayerArraySpacingRule& r)
{
  flags_.parallel_overlap_ = r.flags_.parallel_overlap_;
  flags_.long_array_ = r.flags_.long_array_;
  flags_.via_width_valid_ = r.flags_.via_width_valid_;
  flags_.within_valid_ = r.flags_.within_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  via_width_ = r.via_width_;
  cut_spacing_ = r.cut_spacing_;
  within_ = r.within_;
  array_width_ = r.array_width_;
  cut_class_ = r.cut_class_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.via_width_;
  stream >> obj.cut_spacing_;
  stream >> obj.within_;
  stream >> obj.array_width_;
  stream >> obj.array_spacing_map_;
  stream >> obj.cut_class_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.via_width_;
  stream << obj.cut_spacing_;
  stream << obj.within_;
  stream << obj.array_width_;
  stream << obj.array_spacing_map_;
  stream << obj.cut_class_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerArraySpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerArraySpacingRule::setViaWidth(int via_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->via_width_ = via_width;
}

int dbTechLayerArraySpacingRule::getViaWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->via_width_;
}

void dbTechLayerArraySpacingRule::setCutSpacing(int cut_spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_spacing_ = cut_spacing;
}

int dbTechLayerArraySpacingRule::getCutSpacing() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->cut_spacing_;
}

void dbTechLayerArraySpacingRule::setWithin(int within)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerArraySpacingRule::getWithin() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->within_;
}

void dbTechLayerArraySpacingRule::setArrayWidth(int array_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->array_width_ = array_width;
}

int dbTechLayerArraySpacingRule::getArrayWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_width_;
}

void dbTechLayerArraySpacingRule::setCutClass(
    dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

void dbTechLayerArraySpacingRule::setParallelOverlap(bool parallel_overlap)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.parallel_overlap_ = parallel_overlap;
}

bool dbTechLayerArraySpacingRule::isParallelOverlap() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.parallel_overlap_;
}

void dbTechLayerArraySpacingRule::setLongArray(bool long_array)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.long_array_ = long_array;
}

bool dbTechLayerArraySpacingRule::isLongArray() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.long_array_;
}

void dbTechLayerArraySpacingRule::setViaWidthValid(bool via_width_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.via_width_valid_ = via_width_valid;
}

bool dbTechLayerArraySpacingRule::isViaWidthValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.via_width_valid_;
}

void dbTechLayerArraySpacingRule::setWithinValid(bool within_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.within_valid_ = within_valid;
}

bool dbTechLayerArraySpacingRule::isWithinValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.within_valid_;
}

// User Code Begin dbTechLayerArraySpacingRulePublicMethods

void dbTechLayerArraySpacingRule::setCutsArraySpacing(int num_cuts, int spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  obj->array_spacing_map_[num_cuts] = spacing;
}

const std::map<int, int>& dbTechLayerArraySpacingRule::getCutsArraySpacing()
    const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_spacing_map_;
}

dbTechLayerCutClassRule* dbTechLayerArraySpacingRule::getCutClass() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  if (!obj->cut_class_.isValid()) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

dbTechLayerArraySpacingRule* dbTechLayerArraySpacingRule::create(
    dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerArraySpacingRule* newrule
      = layer->array_spacing_rules_tbl_->create();
  return ((dbTechLayerArraySpacingRule*) newrule);
}

dbTechLayerArraySpacingRule*
dbTechLayerArraySpacingRule::getTechLayerArraySpacingRule(dbTechLayer* inly,
                                                          uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return ((dbTechLayerArraySpacingRule*)
              layer->array_spacing_rules_tbl_->getPtr(dbid));
}

void dbTechLayerArraySpacingRule::destroy(dbTechLayerArraySpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->array_spacing_rules_tbl_->destroy(
      (_dbTechLayerArraySpacingRule*) rule);
}

// User Code End dbTechLayerArraySpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp