// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerKeepOutZoneRule.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerKeepOutZoneRule>;

bool _dbTechLayerKeepOutZoneRule::operator==(
    const _dbTechLayerKeepOutZoneRule& rhs) const
{
  if (flags_.same_mask != rhs.flags_.same_mask) {
    return false;
  }
  if (flags_.same_metal != rhs.flags_.same_metal) {
    return false;
  }
  if (flags_.diff_metal != rhs.flags_.diff_metal) {
    return false;
  }
  if (flags_.except_aligned_side != rhs.flags_.except_aligned_side) {
    return false;
  }
  if (flags_.except_aligned_end != rhs.flags_.except_aligned_end) {
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

void _dbTechLayerKeepOutZoneRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["first_cut_class"].add(first_cut_class_);
  info.children["second_cut_class"].add(second_cut_class_);
  // User Code End collectMemInfo
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

  obj->flags_.same_mask = same_mask;
}

bool dbTechLayerKeepOutZoneRule::isSameMask() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.same_mask;
}

void dbTechLayerKeepOutZoneRule::setSameMetal(bool same_metal)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.same_metal = same_metal;
}

bool dbTechLayerKeepOutZoneRule::isSameMetal() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.same_metal;
}

void dbTechLayerKeepOutZoneRule::setDiffMetal(bool diff_metal)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.diff_metal = diff_metal;
}

bool dbTechLayerKeepOutZoneRule::isDiffMetal() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.diff_metal;
}

void dbTechLayerKeepOutZoneRule::setExceptAlignedSide(bool except_aligned_side)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.except_aligned_side = except_aligned_side;
}

bool dbTechLayerKeepOutZoneRule::isExceptAlignedSide() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.except_aligned_side;
}

void dbTechLayerKeepOutZoneRule::setExceptAlignedEnd(bool except_aligned_end)
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  obj->flags_.except_aligned_end = except_aligned_end;
}

bool dbTechLayerKeepOutZoneRule::isExceptAlignedEnd() const
{
  _dbTechLayerKeepOutZoneRule* obj = (_dbTechLayerKeepOutZoneRule*) this;

  return obj->flags_.except_aligned_end;
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
