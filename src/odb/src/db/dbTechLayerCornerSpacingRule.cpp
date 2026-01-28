// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCornerSpacingRule.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerCornerSpacingRule>;

bool _dbTechLayerCornerSpacingRule::operator==(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  if (flags_.corner_type != rhs.flags_.corner_type) {
    return false;
  }
  if (flags_.same_mask != rhs.flags_.same_mask) {
    return false;
  }
  if (flags_.corner_only != rhs.flags_.corner_only) {
    return false;
  }
  if (flags_.except_eol != rhs.flags_.except_eol) {
    return false;
  }
  if (flags_.except_jog_length != rhs.flags_.except_jog_length) {
    return false;
  }
  if (flags_.edge_length_valid != rhs.flags_.edge_length_valid) {
    return false;
  }
  if (flags_.include_shape != rhs.flags_.include_shape) {
    return false;
  }
  if (flags_.min_length_valid != rhs.flags_.min_length_valid) {
    return false;
  }
  if (flags_.except_notch != rhs.flags_.except_notch) {
    return false;
  }
  if (flags_.except_notch_length_valid
      != rhs.flags_.except_notch_length_valid) {
    return false;
  }
  if (flags_.except_same_net != rhs.flags_.except_same_net) {
    return false;
  }
  if (flags_.except_same_metal != rhs.flags_.except_same_metal) {
    return false;
  }
  if (flags_.corner_to_corner != rhs.flags_.corner_to_corner) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (eol_width_ != rhs.eol_width_) {
    return false;
  }
  if (jog_length_ != rhs.jog_length_) {
    return false;
  }
  if (edge_length_ != rhs.edge_length_) {
    return false;
  }
  if (min_length_ != rhs.min_length_) {
    return false;
  }
  if (except_notch_length_ != rhs.except_notch_length_) {
    return false;
  }

  return true;
}

bool _dbTechLayerCornerSpacingRule::operator<(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(_dbDatabase* db)
{
  flags_ = {};
  within_ = 0;
  eol_width_ = 0;
  jog_length_ = 0;
  edge_length_ = 0;
  min_length_ = 0;
  except_notch_length_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.within_;
  stream >> obj.eol_width_;
  stream >> obj.jog_length_;
  stream >> obj.edge_length_;
  stream >> obj.min_length_;
  stream >> obj.except_notch_length_;
  // User Code Begin >>
  stream >> obj.width_tbl_;
  stream >> obj.spacing_tbl_;
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.within_;
  stream << obj.eol_width_;
  stream << obj.jog_length_;
  stream << obj.edge_length_;
  stream << obj.min_length_;
  stream << obj.except_notch_length_;
  // User Code Begin <<
  stream << obj.width_tbl_;
  stream << obj.spacing_tbl_;
  // User Code End <<
  return stream;
}

void _dbTechLayerCornerSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["width_tbl"].add(width_tbl_);
  info.children["spacing_tbl"].add(spacing_tbl_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCornerSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCornerSpacingRule::setWithin(int within)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerCornerSpacingRule::getWithin() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerCornerSpacingRule::setEolWidth(int eol_width)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerCornerSpacingRule::getEolWidth() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCornerSpacingRule::setJogLength(int jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->jog_length_ = jog_length;
}

int dbTechLayerCornerSpacingRule::getJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLength(int edge_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->edge_length_ = edge_length;
}

int dbTechLayerCornerSpacingRule::getEdgeLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->edge_length_;
}

void dbTechLayerCornerSpacingRule::setMinLength(int min_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->min_length_ = min_length;
}

int dbTechLayerCornerSpacingRule::getMinLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->min_length_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLength(int except_notch_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->except_notch_length_ = except_notch_length;
}

int dbTechLayerCornerSpacingRule::getExceptNotchLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->except_notch_length_;
}

void dbTechLayerCornerSpacingRule::setSameMask(bool same_mask)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.same_mask = same_mask;
}

bool dbTechLayerCornerSpacingRule::isSameMask() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.same_mask;
}

void dbTechLayerCornerSpacingRule::setCornerOnly(bool corner_only)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_only = corner_only;
}

bool dbTechLayerCornerSpacingRule::isCornerOnly() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.corner_only;
}

void dbTechLayerCornerSpacingRule::setExceptEol(bool except_eol)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_eol = except_eol;
}

bool dbTechLayerCornerSpacingRule::isExceptEol() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_eol;
}

void dbTechLayerCornerSpacingRule::setExceptJogLength(bool except_jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_jog_length = except_jog_length;
}

bool dbTechLayerCornerSpacingRule::isExceptJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_jog_length;
}

void dbTechLayerCornerSpacingRule::setEdgeLengthValid(bool edge_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.edge_length_valid = edge_length_valid;
}

bool dbTechLayerCornerSpacingRule::isEdgeLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.edge_length_valid;
}

void dbTechLayerCornerSpacingRule::setIncludeShape(bool include_shape)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.include_shape = include_shape;
}

bool dbTechLayerCornerSpacingRule::isIncludeShape() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.include_shape;
}

void dbTechLayerCornerSpacingRule::setMinLengthValid(bool min_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.min_length_valid = min_length_valid;
}

bool dbTechLayerCornerSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.min_length_valid;
}

void dbTechLayerCornerSpacingRule::setExceptNotch(bool except_notch)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch = except_notch;
}

bool dbTechLayerCornerSpacingRule::isExceptNotch() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLengthValid(
    bool except_notch_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch_length_valid = except_notch_length_valid;
}

bool dbTechLayerCornerSpacingRule::isExceptNotchLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch_length_valid;
}

void dbTechLayerCornerSpacingRule::setExceptSameNet(bool except_same_net)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_net = except_same_net;
}

bool dbTechLayerCornerSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_net;
}

void dbTechLayerCornerSpacingRule::setExceptSameMetal(bool except_same_metal)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_metal = except_same_metal;
}

bool dbTechLayerCornerSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_metal;
}

void dbTechLayerCornerSpacingRule::setCornerToCorner(bool corner_to_corner)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_to_corner = corner_to_corner;
}

bool dbTechLayerCornerSpacingRule::isCornerToCorner() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.corner_to_corner;
}

// User Code Begin dbTechLayerCornerSpacingRulePublicMethods
void dbTechLayerCornerSpacingRule::addSpacing(uint32_t width,
                                              uint32_t spacing1,
                                              uint32_t spacing2)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  obj->width_tbl_.push_back(width);
  obj->spacing_tbl_.push_back(std::make_pair(spacing1, spacing2));
}

void dbTechLayerCornerSpacingRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->spacing_tbl_;
}

void dbTechLayerCornerSpacingRule::getWidthTable(std::vector<int>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->width_tbl_;
}

void dbTechLayerCornerSpacingRule::setType(CornerType _type)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_type = (uint32_t) _type;
}

dbTechLayerCornerSpacingRule::CornerType dbTechLayerCornerSpacingRule::getType()
    const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return (dbTechLayerCornerSpacingRule::CornerType) obj->flags_.corner_type;
}

dbTechLayerCornerSpacingRule* dbTechLayerCornerSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCornerSpacingRule* newrule
      = layer->corner_spacing_rules_tbl_->create();
  return ((dbTechLayerCornerSpacingRule*) newrule);
}

dbTechLayerCornerSpacingRule*
dbTechLayerCornerSpacingRule::getTechLayerCornerSpacingRule(dbTechLayer* inly,
                                                            uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCornerSpacingRule*)
      layer->corner_spacing_rules_tbl_->getPtr(dbid);
}
void dbTechLayerCornerSpacingRule::destroy(dbTechLayerCornerSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->corner_spacing_rules_tbl_->destroy(
      (_dbTechLayerCornerSpacingRule*) rule);
}
// User Code End dbTechLayerCornerSpacingRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
