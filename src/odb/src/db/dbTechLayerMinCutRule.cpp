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
#include "dbTechLayerMinCutRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerMinCutRule>;

bool _dbTechLayerMinCutRule::operator==(const _dbTechLayerMinCutRule& rhs) const
{
  if (flags_.per_cut_class_ != rhs.flags_.per_cut_class_) {
    return false;
  }
  if (flags_.within_cut_dist_valid != rhs.flags_.within_cut_dist_valid) {
    return false;
  }
  if (flags_.from_above_ != rhs.flags_.from_above_) {
    return false;
  }
  if (flags_.from_below_ != rhs.flags_.from_below_) {
    return false;
  }
  if (flags_.length_valid_ != rhs.flags_.length_valid_) {
    return false;
  }
  if (flags_.area_valid_ != rhs.flags_.area_valid_) {
    return false;
  }
  if (flags_.area_within_dist_valid_ != rhs.flags_.area_within_dist_valid_) {
    return false;
  }
  if (flags_.same_metal_overlap != rhs.flags_.same_metal_overlap) {
    return false;
  }
  if (flags_.fully_enclosed_ != rhs.flags_.fully_enclosed_) {
    return false;
  }
  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (within_cut_dist != rhs.within_cut_dist) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (length_within_dist_ != rhs.length_within_dist_) {
    return false;
  }
  if (area_ != rhs.area_) {
    return false;
  }
  if (area_within_dist_ != rhs.area_within_dist_) {
    return false;
  }

  return true;
}

bool _dbTechLayerMinCutRule::operator<(const _dbTechLayerMinCutRule& rhs) const
{
  return true;
}

void _dbTechLayerMinCutRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerMinCutRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.per_cut_class_);
  DIFF_FIELD(flags_.within_cut_dist_valid);
  DIFF_FIELD(flags_.from_above_);
  DIFF_FIELD(flags_.from_below_);
  DIFF_FIELD(flags_.length_valid_);
  DIFF_FIELD(flags_.area_valid_);
  DIFF_FIELD(flags_.area_within_dist_valid_);
  DIFF_FIELD(flags_.same_metal_overlap);
  DIFF_FIELD(flags_.fully_enclosed_);
  DIFF_FIELD(num_cuts_);
  DIFF_FIELD(width_);
  DIFF_FIELD(within_cut_dist);
  DIFF_FIELD(length_);
  DIFF_FIELD(length_within_dist_);
  DIFF_FIELD(area_);
  DIFF_FIELD(area_within_dist_);
  DIFF_END
}

void _dbTechLayerMinCutRule::out(dbDiff& diff,
                                 char side,
                                 const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.per_cut_class_);
  DIFF_OUT_FIELD(flags_.within_cut_dist_valid);
  DIFF_OUT_FIELD(flags_.from_above_);
  DIFF_OUT_FIELD(flags_.from_below_);
  DIFF_OUT_FIELD(flags_.length_valid_);
  DIFF_OUT_FIELD(flags_.area_valid_);
  DIFF_OUT_FIELD(flags_.area_within_dist_valid_);
  DIFF_OUT_FIELD(flags_.same_metal_overlap);
  DIFF_OUT_FIELD(flags_.fully_enclosed_);
  DIFF_OUT_FIELD(num_cuts_);
  DIFF_OUT_FIELD(width_);
  DIFF_OUT_FIELD(within_cut_dist);
  DIFF_OUT_FIELD(length_);
  DIFF_OUT_FIELD(length_within_dist_);
  DIFF_OUT_FIELD(area_);
  DIFF_OUT_FIELD(area_within_dist_);

  DIFF_END
}

_dbTechLayerMinCutRule::_dbTechLayerMinCutRule(_dbDatabase* db)
{
  flags_ = {};
  num_cuts_ = 0;
  width_ = 0;
  within_cut_dist = 0;
  length_ = 0;
  length_within_dist_ = 0;
  area_ = 0;
  area_within_dist_ = 0;
}

_dbTechLayerMinCutRule::_dbTechLayerMinCutRule(_dbDatabase* db,
                                               const _dbTechLayerMinCutRule& r)
{
  flags_.per_cut_class_ = r.flags_.per_cut_class_;
  flags_.within_cut_dist_valid = r.flags_.within_cut_dist_valid;
  flags_.from_above_ = r.flags_.from_above_;
  flags_.from_below_ = r.flags_.from_below_;
  flags_.length_valid_ = r.flags_.length_valid_;
  flags_.area_valid_ = r.flags_.area_valid_;
  flags_.area_within_dist_valid_ = r.flags_.area_within_dist_valid_;
  flags_.same_metal_overlap = r.flags_.same_metal_overlap;
  flags_.fully_enclosed_ = r.flags_.fully_enclosed_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  num_cuts_ = r.num_cuts_;
  width_ = r.width_;
  within_cut_dist = r.within_cut_dist;
  length_ = r.length_;
  length_within_dist_ = r.length_within_dist_;
  area_ = r.area_;
  area_within_dist_ = r.area_within_dist_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinCutRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.num_cuts_;
  stream >> obj.cut_class_cuts_map_;
  stream >> obj.width_;
  stream >> obj.within_cut_dist;
  stream >> obj.length_;
  stream >> obj.length_within_dist_;
  stream >> obj.area_;
  stream >> obj.area_within_dist_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinCutRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.num_cuts_;
  stream << obj.cut_class_cuts_map_;
  stream << obj.width_;
  stream << obj.within_cut_dist;
  stream << obj.length_;
  stream << obj.length_within_dist_;
  stream << obj.area_;
  stream << obj.area_within_dist_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMinCutRule::setNumCuts(int num_cuts)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->num_cuts_ = num_cuts;
}

int dbTechLayerMinCutRule::getNumCuts() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->num_cuts_;
}

std::map<std::string, int> dbTechLayerMinCutRule::getCutClassCutsMap() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->cut_class_cuts_map_;
}

void dbTechLayerMinCutRule::setWidth(int width)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->width_ = width;
}

int dbTechLayerMinCutRule::getWidth() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->width_;
}

void dbTechLayerMinCutRule::setWithinCutDist(int within_cut_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->within_cut_dist = within_cut_dist;
}

int dbTechLayerMinCutRule::getWithinCutDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->within_cut_dist;
}

void dbTechLayerMinCutRule::setLength(int length)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->length_ = length;
}

int dbTechLayerMinCutRule::getLength() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->length_;
}

void dbTechLayerMinCutRule::setLengthWithinDist(int length_within_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->length_within_dist_ = length_within_dist;
}

int dbTechLayerMinCutRule::getLengthWithinDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->length_within_dist_;
}

void dbTechLayerMinCutRule::setArea(int area)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->area_ = area;
}

int dbTechLayerMinCutRule::getArea() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->area_;
}

void dbTechLayerMinCutRule::setAreaWithinDist(int area_within_dist)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->area_within_dist_ = area_within_dist;
}

int dbTechLayerMinCutRule::getAreaWithinDist() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  return obj->area_within_dist_;
}

void dbTechLayerMinCutRule::setPerCutClass(bool per_cut_class)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.per_cut_class_ = per_cut_class;
}

bool dbTechLayerMinCutRule::isPerCutClass() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.per_cut_class_;
}

void dbTechLayerMinCutRule::setWithinCutDistValid(bool within_cut_dist_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.within_cut_dist_valid = within_cut_dist_valid;
}

bool dbTechLayerMinCutRule::isWithinCutDistValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.within_cut_dist_valid;
}

void dbTechLayerMinCutRule::setFromAbove(bool from_above)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.from_above_ = from_above;
}

bool dbTechLayerMinCutRule::isFromAbove() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.from_above_;
}

void dbTechLayerMinCutRule::setFromBelow(bool from_below)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.from_below_ = from_below;
}

bool dbTechLayerMinCutRule::isFromBelow() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.from_below_;
}

void dbTechLayerMinCutRule::setLengthValid(bool length_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.length_valid_ = length_valid;
}

bool dbTechLayerMinCutRule::isLengthValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.length_valid_;
}

void dbTechLayerMinCutRule::setAreaValid(bool area_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.area_valid_ = area_valid;
}

bool dbTechLayerMinCutRule::isAreaValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.area_valid_;
}

void dbTechLayerMinCutRule::setAreaWithinDistValid(bool area_within_dist_valid)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.area_within_dist_valid_ = area_within_dist_valid;
}

bool dbTechLayerMinCutRule::isAreaWithinDistValid() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.area_within_dist_valid_;
}

void dbTechLayerMinCutRule::setSameMetalOverlap(bool same_metal_overlap)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.same_metal_overlap = same_metal_overlap;
}

bool dbTechLayerMinCutRule::isSameMetalOverlap() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.same_metal_overlap;
}

void dbTechLayerMinCutRule::setFullyEnclosed(bool fully_enclosed)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  obj->flags_.fully_enclosed_ = fully_enclosed;
}

bool dbTechLayerMinCutRule::isFullyEnclosed() const
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;

  return obj->flags_.fully_enclosed_;
}

// User Code Begin dbTechLayerMinCutRulePublicMethods

void dbTechLayerMinCutRule::setCutsPerCutClass(std::string cut_class,
                                               int num_cuts)
{
  _dbTechLayerMinCutRule* obj = (_dbTechLayerMinCutRule*) this;
  obj->cut_class_cuts_map_[cut_class] = num_cuts;
}

dbTechLayerMinCutRule* dbTechLayerMinCutRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerMinCutRule* newrule = layer->min_cuts_rules_tbl_->create();
  return ((dbTechLayerMinCutRule*) newrule);
}

dbTechLayerMinCutRule* dbTechLayerMinCutRule::getTechLayerMinCutRule(
    dbTechLayer* inly,
    uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return ((dbTechLayerMinCutRule*) layer->min_cuts_rules_tbl_->getPtr(dbid));
}

void dbTechLayerMinCutRule::destroy(dbTechLayerMinCutRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->min_cuts_rules_tbl_->destroy((_dbTechLayerMinCutRule*) rule);
}

// User Code End dbTechLayerMinCutRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp