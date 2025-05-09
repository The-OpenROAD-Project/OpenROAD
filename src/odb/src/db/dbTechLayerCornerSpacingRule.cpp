// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCornerSpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerCornerSpacingRule>;

bool _dbTechLayerCornerSpacingRule::operator==(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  if (flags_.corner_type_ != rhs.flags_.corner_type_) {
    return false;
  }
  if (flags_.same_mask_ != rhs.flags_.same_mask_) {
    return false;
  }
  if (flags_.corner_only_ != rhs.flags_.corner_only_) {
    return false;
  }
  if (flags_.except_eol_ != rhs.flags_.except_eol_) {
    return false;
  }
  if (flags_.except_jog_length_ != rhs.flags_.except_jog_length_) {
    return false;
  }
  if (flags_.edge_length_valid_ != rhs.flags_.edge_length_valid_) {
    return false;
  }
  if (flags_.include_shape_ != rhs.flags_.include_shape_) {
    return false;
  }
  if (flags_.min_length_valid_ != rhs.flags_.min_length_valid_) {
    return false;
  }
  if (flags_.except_notch_ != rhs.flags_.except_notch_) {
    return false;
  }
  if (flags_.except_notch_length_valid_
      != rhs.flags_.except_notch_length_valid_) {
    return false;
  }
  if (flags_.except_same_net_ != rhs.flags_.except_same_net_) {
    return false;
  }
  if (flags_.except_same_metal_ != rhs.flags_.except_same_metal_) {
    return false;
  }
  if (flags_.corner_to_corner_ != rhs.flags_.corner_to_corner_) {
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
  stream >> obj._width_tbl;
  stream >> obj._spacing_tbl;
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
  stream << obj._width_tbl;
  stream << obj._spacing_tbl;
  // User Code End <<
  return stream;
}

void _dbTechLayerCornerSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["width_tbl"].add(_width_tbl);
  info.children_["spacing_tbl"].add(_spacing_tbl);
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

  obj->flags_.same_mask_ = same_mask;
}

bool dbTechLayerCornerSpacingRule::isSameMask() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerCornerSpacingRule::setCornerOnly(bool corner_only)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_only_ = corner_only;
}

bool dbTechLayerCornerSpacingRule::isCornerOnly() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.corner_only_;
}

void dbTechLayerCornerSpacingRule::setExceptEol(bool except_eol)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_eol_ = except_eol;
}

bool dbTechLayerCornerSpacingRule::isExceptEol() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_eol_;
}

void dbTechLayerCornerSpacingRule::setExceptJogLength(bool except_jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_jog_length_ = except_jog_length;
}

bool dbTechLayerCornerSpacingRule::isExceptJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLengthValid(bool edge_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.edge_length_valid_ = edge_length_valid;
}

bool dbTechLayerCornerSpacingRule::isEdgeLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.edge_length_valid_;
}

void dbTechLayerCornerSpacingRule::setIncludeShape(bool include_shape)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.include_shape_ = include_shape;
}

bool dbTechLayerCornerSpacingRule::isIncludeShape() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.include_shape_;
}

void dbTechLayerCornerSpacingRule::setMinLengthValid(bool min_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.min_length_valid_ = min_length_valid;
}

bool dbTechLayerCornerSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.min_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptNotch(bool except_notch)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch_ = except_notch;
}

bool dbTechLayerCornerSpacingRule::isExceptNotch() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLengthValid(
    bool except_notch_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch_length_valid_ = except_notch_length_valid;
}

bool dbTechLayerCornerSpacingRule::isExceptNotchLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptSameNet(bool except_same_net)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_net_ = except_same_net;
}

bool dbTechLayerCornerSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_net_;
}

void dbTechLayerCornerSpacingRule::setExceptSameMetal(bool except_same_metal)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_metal_ = except_same_metal;
}

bool dbTechLayerCornerSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_metal_;
}

void dbTechLayerCornerSpacingRule::setCornerToCorner(bool corner_to_corner)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_to_corner_ = corner_to_corner;
}

bool dbTechLayerCornerSpacingRule::isCornerToCorner() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.corner_to_corner_;
}

// User Code Begin dbTechLayerCornerSpacingRulePublicMethods
void dbTechLayerCornerSpacingRule::addSpacing(uint width,
                                              uint spacing1,
                                              uint spacing2)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  obj->_width_tbl.push_back(width);
  obj->_spacing_tbl.push_back(std::make_pair(spacing1, spacing2));
}

void dbTechLayerCornerSpacingRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->_spacing_tbl;
}

void dbTechLayerCornerSpacingRule::getWidthTable(std::vector<int>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->_width_tbl;
}

void dbTechLayerCornerSpacingRule::setType(CornerType _type)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_type_ = (uint) _type;
}

dbTechLayerCornerSpacingRule::CornerType dbTechLayerCornerSpacingRule::getType()
    const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return (dbTechLayerCornerSpacingRule::CornerType) obj->flags_.corner_type_;
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
                                                            uint dbid)
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
