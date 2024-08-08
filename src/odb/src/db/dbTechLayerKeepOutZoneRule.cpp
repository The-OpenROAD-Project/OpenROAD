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
#include "dbTechLayerKeepOutZoneRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerKeepOutZoneRule>;

bool _dbTechLayerKeepOutZoneRule::operator==(
    const _dbTechLayerKeepOutZoneRule& rhs) const
{
  if (flags_.same_mask_ != rhs.flags_.same_mask_) {
    return false;
  }
  if (flags_.same_metal_ != rhs.flags_.same_metal_) {
    return false;
  }
  if (flags_.diff_metal_ != rhs.flags_.diff_metal_) {
    return false;
  }
  if (flags_.except_aligned_side_ != rhs.flags_.except_aligned_side_) {
    return false;
  }
  if (flags_.except_aligned_end_ != rhs.flags_.except_aligned_end_) {
    return false;
  }
  if (first_cut_class_ != rhs.first_cut_class_) {
    return false;
  }
  if (second_cut_class_ != rhs.second_cut_class_) {
    return false;
  }
  if (aligned_spacing_ != rhs.aligned_spacing_) {
    return false;
  }
  if (side_extension_ != rhs.side_extension_) {
    return false;
  }
  if (forward_extension_ != rhs.forward_extension_) {
    return false;
  }
  if (end_side_extension_ != rhs.end_side_extension_) {
    return false;
  }
  if (end_forward_extension_ != rhs.end_forward_extension_) {
    return false;
  }
  if (side_side_extension_ != rhs.side_side_extension_) {
    return false;
  }
  if (side_forward_extension_ != rhs.side_forward_extension_) {
    return false;
  }
  if (spiral_extension_ != rhs.spiral_extension_) {
    return false;
  }

  return true;
}

bool _dbTechLayerKeepOutZoneRule::operator<(
    const _dbTechLayerKeepOutZoneRule& rhs) const
{
  return true;
}

void _dbTechLayerKeepOutZoneRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerKeepOutZoneRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(flags_.same_mask_);
  DIFF_FIELD(flags_.same_metal_);
  DIFF_FIELD(flags_.diff_metal_);
  DIFF_FIELD(flags_.except_aligned_side_);
  DIFF_FIELD(flags_.except_aligned_end_);
  DIFF_FIELD(first_cut_class_);
  DIFF_FIELD(second_cut_class_);
  DIFF_FIELD(aligned_spacing_);
  DIFF_FIELD(side_extension_);
  DIFF_FIELD(forward_extension_);
  DIFF_FIELD(end_side_extension_);
  DIFF_FIELD(end_forward_extension_);
  DIFF_FIELD(side_side_extension_);
  DIFF_FIELD(side_forward_extension_);
  DIFF_FIELD(spiral_extension_);
  DIFF_END
}

void _dbTechLayerKeepOutZoneRule::out(dbDiff& diff,
                                      char side,
                                      const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.same_mask_);
  DIFF_OUT_FIELD(flags_.same_metal_);
  DIFF_OUT_FIELD(flags_.diff_metal_);
  DIFF_OUT_FIELD(flags_.except_aligned_side_);
  DIFF_OUT_FIELD(flags_.except_aligned_end_);
  DIFF_OUT_FIELD(first_cut_class_);
  DIFF_OUT_FIELD(second_cut_class_);
  DIFF_OUT_FIELD(aligned_spacing_);
  DIFF_OUT_FIELD(side_extension_);
  DIFF_OUT_FIELD(forward_extension_);
  DIFF_OUT_FIELD(end_side_extension_);
  DIFF_OUT_FIELD(end_forward_extension_);
  DIFF_OUT_FIELD(side_side_extension_);
  DIFF_OUT_FIELD(side_forward_extension_);
  DIFF_OUT_FIELD(spiral_extension_);

  DIFF_END
}

_dbTechLayerKeepOutZoneRule::_dbTechLayerKeepOutZoneRule(_dbDatabase* db)
{
  flags_ = {};
  aligned_spacing_ = -1;
  side_extension_ = -1;
  forward_extension_ = -1;
  end_side_extension_ = -1;
  end_forward_extension_ = -1;
  side_side_extension_ = -1;
  side_forward_extension_ = -1;
  spiral_extension_ = -1;
}

_dbTechLayerKeepOutZoneRule::_dbTechLayerKeepOutZoneRule(
    _dbDatabase* db,
    const _dbTechLayerKeepOutZoneRule& r)
{
  flags_.same_mask_ = r.flags_.same_mask_;
  flags_.same_metal_ = r.flags_.same_metal_;
  flags_.diff_metal_ = r.flags_.diff_metal_;
  flags_.except_aligned_side_ = r.flags_.except_aligned_side_;
  flags_.except_aligned_end_ = r.flags_.except_aligned_end_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  first_cut_class_ = r.first_cut_class_;
  second_cut_class_ = r.second_cut_class_;
  aligned_spacing_ = r.aligned_spacing_;
  side_extension_ = r.side_extension_;
  forward_extension_ = r.forward_extension_;
  end_side_extension_ = r.end_side_extension_;
  end_forward_extension_ = r.end_forward_extension_;
  side_side_extension_ = r.side_side_extension_;
  side_forward_extension_ = r.side_forward_extension_;
  spiral_extension_ = r.spiral_extension_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerKeepOutZoneRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.first_cut_class_;
  stream >> obj.second_cut_class_;
  stream >> obj.aligned_spacing_;
  stream >> obj.side_extension_;
  stream >> obj.forward_extension_;
  stream >> obj.end_side_extension_;
  stream >> obj.end_forward_extension_;
  stream >> obj.side_side_extension_;
  stream >> obj.side_forward_extension_;
  stream >> obj.spiral_extension_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerKeepOutZoneRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.first_cut_class_;
  stream << obj.second_cut_class_;
  stream << obj.aligned_spacing_;
  stream << obj.side_extension_;
  stream << obj.forward_extension_;
  stream << obj.end_side_extension_;
  stream << obj.end_forward_extension_;
  stream << obj.side_side_extension_;
  stream << obj.side_forward_extension_;
  stream << obj.spiral_extension_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerKeepOutZoneRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerKeepOutZoneRule::setFirstCutClass(
    const std::string& first_cut_class)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->first_cut_class_ = first_cut_class;
}

std::string dbTechLayerKeepOutZoneRule::getFirstCutClass() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->first_cut_class_;
}

void dbTechLayerKeepOutZoneRule::setSecondCutClass(
    const std::string& second_cut_class)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->second_cut_class_ = second_cut_class;
}

std::string dbTechLayerKeepOutZoneRule::getSecondCutClass() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->second_cut_class_;
}

void dbTechLayerKeepOutZoneRule::setAlignedSpacing(int aligned_spacing)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->aligned_spacing_ = aligned_spacing;
}

int dbTechLayerKeepOutZoneRule::getAlignedSpacing() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->aligned_spacing_;
}

void dbTechLayerKeepOutZoneRule::setSideExtension(int side_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->side_extension_ = side_extension;
}

int dbTechLayerKeepOutZoneRule::getSideExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->side_extension_;
}

void dbTechLayerKeepOutZoneRule::setForwardExtension(int forward_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->forward_extension_ = forward_extension;
}

int dbTechLayerKeepOutZoneRule::getForwardExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->forward_extension_;
}

void dbTechLayerKeepOutZoneRule::setEndSideExtension(int end_side_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->end_side_extension_ = end_side_extension;
}

int dbTechLayerKeepOutZoneRule::getEndSideExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->end_side_extension_;
}

void dbTechLayerKeepOutZoneRule::setEndForwardExtension(
    int end_forward_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->end_forward_extension_ = end_forward_extension;
}

int dbTechLayerKeepOutZoneRule::getEndForwardExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->end_forward_extension_;
}

void dbTechLayerKeepOutZoneRule::setSideSideExtension(int side_side_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->side_side_extension_ = side_side_extension;
}

int dbTechLayerKeepOutZoneRule::getSideSideExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->side_side_extension_;
}

void dbTechLayerKeepOutZoneRule::setSideForwardExtension(
    int side_forward_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->side_forward_extension_ = side_forward_extension;
}

int dbTechLayerKeepOutZoneRule::getSideForwardExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->side_forward_extension_;
}

void dbTechLayerKeepOutZoneRule::setSpiralExtension(int spiral_extension)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->spiral_extension_ = spiral_extension;
}

int dbTechLayerKeepOutZoneRule::getSpiralExtension() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;
  return obj->spiral_extension_;
}

void dbTechLayerKeepOutZoneRule::setSameMask(bool same_mask)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.same_mask_ = same_mask;
}

bool dbTechLayerKeepOutZoneRule::isSameMask() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerKeepOutZoneRule::setSameMetal(bool same_metal)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.same_metal_ = same_metal;
}

bool dbTechLayerKeepOutZoneRule::isSameMetal() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.same_metal_;
}

void dbTechLayerKeepOutZoneRule::setDiffMetal(bool diff_metal)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.diff_metal_ = diff_metal;
}

bool dbTechLayerKeepOutZoneRule::isDiffMetal() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.diff_metal_;
}

void dbTechLayerKeepOutZoneRule::setExceptAlignedSide(bool except_aligned_side)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.except_aligned_side_ = except_aligned_side;
}

bool dbTechLayerKeepOutZoneRule::isExceptAlignedSide() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.except_aligned_side_;
}

void dbTechLayerKeepOutZoneRule::setExceptAlignedEnd(bool except_aligned_end)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.except_aligned_end_ = except_aligned_end;
}

bool dbTechLayerKeepOutZoneRule::isExceptAlignedEnd() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.except_aligned_end_;
}

// User Code Begin dbTechLayerKeepOutZoneRulePublicMethods

dbTechLayerKeepOutZoneRule* dbTechLayerKeepOutZoneRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerKeepOutZoneRule* newrule
      = layer->keepout_zone_rules_tbl_->create();
  return ((dbTechLayerKeepOutZoneRule*) newrule);
}

void dbTechLayerKeepOutZoneRule::destroy(dbTechLayerKeepOutZoneRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->keepout_zone_rules_tbl_->destroy((_dbTechLayerKeepOutZoneRule*) rule);
}

// User Code End dbTechLayerKeepOutZoneRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp