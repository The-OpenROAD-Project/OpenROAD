// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerImplantSpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <vector>
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerImplantSpacingRule>;

bool _dbTechLayerImplantSpacingRule::operator==(
    const _dbTechLayerImplantSpacingRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.horizontal != rhs.flags_.horizontal) {
    return false;
  }
  if (flags_.vertical != rhs.flags_.vertical) {
    return false;
  }
  if (flags_.prl_valid != rhs.flags_.prl_valid) {
    return false;
  }
  if (flags_.except_abutted != rhs.flags_.except_abutted) {
    return false;
  }
  if (flags_.except_corner_touch != rhs.flags_.except_corner_touch) {
    return false;
  }
  if (min_spacing_ != rhs.min_spacing_) {
    return false;
  }
  if (second_layer_ != rhs.second_layer_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }

  // User Code Begin ==
  if (intersect_layers_ != rhs.intersect_layers_) {
    return false;
  }
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerImplantSpacingRule::operator<(
    const _dbTechLayerImplantSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerImplantSpacingRule::_dbTechLayerImplantSpacingRule(_dbDatabase* db)
{
  flags_ = {};
  min_spacing_ = 0;
  prl_ = 0;
  length_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerImplantSpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.min_spacing_;
  stream >> obj.second_layer_;
  stream >> obj.prl_;
  stream >> obj.length_;
  stream >> obj.intersect_layers_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerImplantSpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.min_spacing_;
  stream << obj.second_layer_;
  stream << obj.prl_;
  stream << obj.length_;
  stream << obj.intersect_layers_;
  return stream;
}

void _dbTechLayerImplantSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["intersect_layers"].add(intersect_layers_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerImplantSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerImplantSpacingRule::setMinSpacing(int min_spacing)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->min_spacing_ = min_spacing;
}

int dbTechLayerImplantSpacingRule::getMinSpacing() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);
  return obj->min_spacing_;
}

void dbTechLayerImplantSpacingRule::setPrl(int prl)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->prl_ = prl;
}

int dbTechLayerImplantSpacingRule::getPrl() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);
  return obj->prl_;
}

void dbTechLayerImplantSpacingRule::setLength(int length)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->length_ = length;
}

int dbTechLayerImplantSpacingRule::getLength() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);
  return obj->length_;
}

void dbTechLayerImplantSpacingRule::setHorizontal(bool horizontal)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->flags_.horizontal = horizontal;
}

bool dbTechLayerImplantSpacingRule::isHorizontal() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);

  return obj->flags_.horizontal;
}

void dbTechLayerImplantSpacingRule::setVertical(bool vertical)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->flags_.vertical = vertical;
}

bool dbTechLayerImplantSpacingRule::isVertical() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);

  return obj->flags_.vertical;
}

void dbTechLayerImplantSpacingRule::setPrlValid(bool prl_valid)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->flags_.prl_valid = prl_valid;
}

bool dbTechLayerImplantSpacingRule::isPrlValid() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);

  return obj->flags_.prl_valid;
}

void dbTechLayerImplantSpacingRule::setExceptAbutted(bool except_abutted)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->flags_.except_abutted = except_abutted;
}

bool dbTechLayerImplantSpacingRule::isExceptAbutted() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);

  return obj->flags_.except_abutted;
}

void dbTechLayerImplantSpacingRule::setExceptCornerTouch(
    bool except_corner_touch)
{
  _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);

  obj->flags_.except_corner_touch = except_corner_touch;
}

bool dbTechLayerImplantSpacingRule::isExceptCornerTouch() const
{
  const _dbTechLayerImplantSpacingRule* obj
      = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);

  return obj->flags_.except_corner_touch;
}

dbTechLayerImplantSpacingRule* dbTechLayerImplantSpacingRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = reinterpret_cast<_dbTechLayer*>(parent);
  return reinterpret_cast<dbTechLayerImplantSpacingRule*>(
      _parent->implant_spacing_rules_tbl_->create());
}
void dbTechLayerImplantSpacingRule::destroy(dbTechLayerImplantSpacingRule* obj)
{
  _dbTechLayer* _parent
      = static_cast<_dbTechLayer*>(obj->getImpl()->getOwner());
  dbProperty::destroyProperties(obj);
  _parent->implant_spacing_rules_tbl_->destroy(
      reinterpret_cast<_dbTechLayerImplantSpacingRule*>(obj));
}
// User Code Begin dbTechLayerImplantSpacingRulePublicMethods
void dbTechLayerImplantSpacingRule::setSecondLayer(dbTechLayer* second_layer)
{
  auto* obj = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);
  obj->second_layer_
      = (second_layer != nullptr) ? second_layer->getImpl()->getOID() : 0;
}

dbTechLayer* dbTechLayerImplantSpacingRule::getSecondLayer() const
{
  auto* obj = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);
  if (!obj->second_layer_.isValid()) {
    return nullptr;
  }
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->second_layer_);
}

void dbTechLayerImplantSpacingRule::addIntersectLayer(dbTechLayer* layer)
{
  if (layer == nullptr) {
    return;
  }
  auto* obj = reinterpret_cast<_dbTechLayerImplantSpacingRule*>(this);
  obj->intersect_layers_.push_back(layer->getImpl()->getOID());
}

std::vector<dbTechLayer*> dbTechLayerImplantSpacingRule::getIntersectLayers()
    const
{
  auto* obj = reinterpret_cast<const _dbTechLayerImplantSpacingRule*>(this);
  std::vector<dbTechLayer*> tbl;
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  for (const auto& lid : obj->intersect_layers_) {
    if (lid.isValid()) {
      tbl.push_back(odb::dbTechLayer::getTechLayer(tech, lid));
    }
  }
  return tbl;
}
// User Code End dbTechLayerImplantSpacingRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
