// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerImplantWidthRule.h"

#include <cstdint>
#include <cstring>
#include <string>

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
template class dbTable<_dbTechLayerImplantWidthRule>;

bool _dbTechLayerImplantWidthRule::operator==(
    const _dbTechLayerImplantWidthRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.layer_any != rhs.flags_.layer_any) {
    return false;
  }
  if (flags_.zero_prl != rhs.flags_.zero_prl) {
    return false;
  }
  if (flags_.except_corner_touch != rhs.flags_.except_corner_touch) {
    return false;
  }
  if (min_width_ != rhs.min_width_) {
    return false;
  }
  if (second_layer_ != rhs.second_layer_) {
    return false;
  }
  if (max_width_ != rhs.max_width_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (cell_height_ != rhs.cell_height_) {
    return false;
  }
  if (check_implant_group_ != rhs.check_implant_group_) {
    return false;
  }

  // User Code Begin ==
  if (within_layers_ != rhs.within_layers_) {
    return false;
  }
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerImplantWidthRule::operator<(
    const _dbTechLayerImplantWidthRule& rhs) const
{
  return true;
}

_dbTechLayerImplantWidthRule::_dbTechLayerImplantWidthRule(_dbDatabase* db)
{
  flags_ = {};
  min_width_ = 0;
  max_width_ = 0;
  length_ = 0;
  cell_height_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerImplantWidthRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.min_width_;
  stream >> obj.second_layer_;
  stream >> obj.max_width_;
  stream >> obj.length_;
  stream >> obj.cell_height_;
  stream >> obj.check_implant_group_;
  stream >> obj.within_layers_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerImplantWidthRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.min_width_;
  stream << obj.second_layer_;
  stream << obj.max_width_;
  stream << obj.length_;
  stream << obj.cell_height_;
  stream << obj.check_implant_group_;
  stream << obj.within_layers_;
  return stream;
}

void _dbTechLayerImplantWidthRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["check_implant_group"].add(check_implant_group_);
  info.children["within_layers"].add(within_layers_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerImplantWidthRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerImplantWidthRule::setMinWidth(int min_width)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->min_width_ = min_width;
}

int dbTechLayerImplantWidthRule::getMinWidth() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  return obj->min_width_;
}

void dbTechLayerImplantWidthRule::setMaxWidth(int max_width)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->max_width_ = max_width;
}

int dbTechLayerImplantWidthRule::getMaxWidth() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  return obj->max_width_;
}

void dbTechLayerImplantWidthRule::setLength(int length)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->length_ = length;
}

int dbTechLayerImplantWidthRule::getLength() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  return obj->length_;
}

void dbTechLayerImplantWidthRule::setCellHeight(int cell_height)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->cell_height_ = cell_height;
}

int dbTechLayerImplantWidthRule::getCellHeight() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  return obj->cell_height_;
}

void dbTechLayerImplantWidthRule::setCheckImplantGroup(
    const std::string& check_implant_group)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->check_implant_group_ = check_implant_group;
}

const std::string& dbTechLayerImplantWidthRule::getCheckImplantGroup() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  return obj->check_implant_group_;
}

void dbTechLayerImplantWidthRule::setLayerAny(bool layer_any)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->flags_.layer_any = layer_any;
}

bool dbTechLayerImplantWidthRule::isLayerAny() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);

  return obj->flags_.layer_any;
}

void dbTechLayerImplantWidthRule::setZeroPrl(bool zero_prl)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->flags_.zero_prl = zero_prl;
}

bool dbTechLayerImplantWidthRule::isZeroPrl() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);

  return obj->flags_.zero_prl;
}

void dbTechLayerImplantWidthRule::setExceptCornerTouch(bool except_corner_touch)
{
  _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);

  obj->flags_.except_corner_touch = except_corner_touch;
}

bool dbTechLayerImplantWidthRule::isExceptCornerTouch() const
{
  const _dbTechLayerImplantWidthRule* obj
      = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);

  return obj->flags_.except_corner_touch;
}

dbTechLayerImplantWidthRule* dbTechLayerImplantWidthRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = reinterpret_cast<_dbTechLayer*>(parent);
  return reinterpret_cast<dbTechLayerImplantWidthRule*>(
      _parent->implant_width_rules_tbl_->create());
}
void dbTechLayerImplantWidthRule::destroy(dbTechLayerImplantWidthRule* obj)
{
  _dbTechLayer* _parent
      = static_cast<_dbTechLayer*>(obj->getImpl()->getOwner());
  dbProperty::destroyProperties(obj);
  _parent->implant_width_rules_tbl_->destroy(
      reinterpret_cast<_dbTechLayerImplantWidthRule*>(obj));
}
// User Code Begin dbTechLayerImplantWidthRulePublicMethods
void dbTechLayerImplantWidthRule::setSecondLayer(dbTechLayer* second_layer)
{
  auto* obj = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);
  obj->second_layer_
      = (second_layer != nullptr) ? second_layer->getImpl()->getOID() : 0;
}

dbTechLayer* dbTechLayerImplantWidthRule::getSecondLayer() const
{
  auto* obj = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  if (!obj->second_layer_.isValid()) {
    return nullptr;
  }
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->second_layer_);
}

void dbTechLayerImplantWidthRule::addWithinLayer(dbTechLayer* layer)
{
  if (layer == nullptr) {
    return;
  }
  auto* obj = reinterpret_cast<_dbTechLayerImplantWidthRule*>(this);
  obj->within_layers_.push_back(layer->getImpl()->getOID());
}

std::vector<dbTechLayer*> dbTechLayerImplantWidthRule::getWithinLayers() const
{
  auto* obj = reinterpret_cast<const _dbTechLayerImplantWidthRule*>(this);
  std::vector<dbTechLayer*> tbl;
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  for (const auto& lid : obj->within_layers_) {
    if (lid.isValid()) {
      tbl.push_back(odb::dbTechLayer::getTechLayer(tech, lid));
    }
  }
  return tbl;
}
// User Code End dbTechLayerImplantWidthRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
