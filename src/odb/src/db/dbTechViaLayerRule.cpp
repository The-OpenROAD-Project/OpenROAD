// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechViaLayerRule.h"

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbTechViaGenerateRule.h"
#include "dbTechViaRule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {

template class dbTable<_dbTechViaLayerRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaLayerRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaLayerRule::operator==(const _dbTechViaLayerRule& rhs) const
{
  if (flags_.direction != rhs.flags_.direction) {
    return false;
  }

  if (flags_.has_enclosure != rhs.flags_.has_enclosure) {
    return false;
  }

  if (flags_.has_width != rhs.flags_.has_width) {
    return false;
  }

  if (flags_.has_overhang != rhs.flags_.has_overhang) {
    return false;
  }

  if (flags_.has_metal_overhang != rhs.flags_.has_metal_overhang) {
    return false;
  }

  if (flags_.has_resistance != rhs.flags_.has_resistance) {
    return false;
  }

  if (flags_.has_spacing != rhs.flags_.has_spacing) {
    return false;
  }

  if (flags_.has_rect != rhs.flags_.has_rect) {
    return false;
  }

  if (overhang1_ != rhs.overhang1_) {
    return false;
  }

  if (overhang2_ != rhs.overhang2_) {
    return false;
  }

  if (min_width_ != rhs.min_width_) {
    return false;
  }

  if (max_width_ != rhs.max_width_) {
    return false;
  }

  if (overhang_ != rhs.overhang_) {
    return false;
  }

  if (metal_overhang_ != rhs.metal_overhang_) {
    return false;
  }

  if (spacing_x_ != rhs.spacing_x_) {
    return false;
  }

  if (spacing_y_ != rhs.spacing_y_) {
    return false;
  }

  if (resistance_ != rhs.resistance_) {
    return false;
  }

  if (rect_ != rhs.rect_) {
    return false;
  }

  if (layer_ != rhs.layer_) {
    return false;
  }

  return true;
}

_dbTechViaLayerRule::_dbTechViaLayerRule(_dbDatabase*,
                                         const _dbTechViaLayerRule& v)
    : flags_(v.flags_),
      overhang1_(v.overhang1_),
      overhang2_(v.overhang2_),
      min_width_(v.min_width_),
      max_width_(v.max_width_),
      overhang_(v.overhang_),
      metal_overhang_(v.metal_overhang_),
      spacing_x_(v.spacing_x_),
      spacing_y_(v.spacing_y_),
      resistance_(v.resistance_),
      rect_(v.rect_),
      layer_(v.layer_)
{
}

_dbTechViaLayerRule::_dbTechViaLayerRule(_dbDatabase*)
{
  flags_.direction = 0;
  flags_.has_enclosure = 0;
  flags_.has_width = 0;
  flags_.has_overhang = 0;
  flags_.has_metal_overhang = 0;
  flags_.has_resistance = 0;
  flags_.has_spacing = 0;
  flags_.has_rect = 0;
  flags_.spare_bits = 0;
  overhang1_ = 0;
  overhang2_ = 0;
  min_width_ = 0;
  max_width_ = 0;
  overhang_ = 0;
  metal_overhang_ = 0;
  spacing_x_ = 0;
  spacing_y_ = 0;
  resistance_ = 0.0;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaLayerRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream << *bit_field;
  stream << v.overhang1_;
  stream << v.overhang2_;
  stream << v.min_width_;
  stream << v.max_width_;
  stream << v.overhang_;
  stream << v.metal_overhang_;
  stream << v.spacing_x_;
  stream << v.spacing_y_;
  stream << v.resistance_;
  stream << v.rect_;
  stream << v.layer_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaLayerRule& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream >> *bit_field;
  stream >> v.overhang1_;
  stream >> v.overhang2_;
  stream >> v.min_width_;
  stream >> v.max_width_;
  stream >> v.overhang_;
  stream >> v.metal_overhang_;
  stream >> v.spacing_x_;
  stream >> v.spacing_y_;
  stream >> v.resistance_;
  stream >> v.rect_;
  stream >> v.layer_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaLayerRule - Methods
//
////////////////////////////////////////////////////////////////////

dbTechLayer* dbTechViaLayerRule::getLayer()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;

  if (rule->layer_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(rule->layer_);
}

dbTechLayerDir dbTechViaLayerRule::getDirection()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  dbTechLayerDir d((dbTechLayerDir::Value) rule->flags_.direction);
  return d;
}

void dbTechViaLayerRule::setDirection(dbTechLayerDir dir)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.direction = dir.getValue();
}

bool dbTechViaLayerRule::hasWidth()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_width == 1;
}

void dbTechViaLayerRule::getWidth(int& minWidth, int& maxWidth)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  minWidth = rule->min_width_;
  maxWidth = rule->max_width_;
}

void dbTechViaLayerRule::setWidth(int minWidth, int maxWidth)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_width = 1;
  rule->min_width_ = minWidth;
  rule->max_width_ = maxWidth;
}

bool dbTechViaLayerRule::hasEnclosure()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_enclosure == 1;
}

void dbTechViaLayerRule::getEnclosure(int& overhang1, int& overhang2)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  overhang1 = rule->overhang1_;
  overhang2 = rule->overhang2_;
}

void dbTechViaLayerRule::setEnclosure(int overhang1, int overhang2)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_enclosure = 1;
  rule->overhang1_ = overhang1;
  rule->overhang2_ = overhang2;
}

bool dbTechViaLayerRule::hasOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_overhang == 1;
}

int dbTechViaLayerRule::getOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->overhang_;
}

void dbTechViaLayerRule::setOverhang(int overhang)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_overhang = 1;
  rule->overhang_ = overhang;
}

bool dbTechViaLayerRule::hasMetalOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_metal_overhang == 1;
}

int dbTechViaLayerRule::getMetalOverhang()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->metal_overhang_;
}

void dbTechViaLayerRule::setMetalOverhang(int overhang)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_metal_overhang = 1;
  rule->metal_overhang_ = overhang;
}

bool dbTechViaLayerRule::hasSpacing()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_spacing == 1;
}

void dbTechViaLayerRule::getSpacing(int& spacing_x, int& spacing_y)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  spacing_x = rule->spacing_x_;
  spacing_y = rule->spacing_y_;
}

void dbTechViaLayerRule::setSpacing(int spacing_x, int spacing_y)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_spacing = 1;
  rule->spacing_x_ = spacing_x;
  rule->spacing_y_ = spacing_y;
}

bool dbTechViaLayerRule::hasRect()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_rect == 1;
}

void dbTechViaLayerRule::getRect(Rect& r)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  r = rule->rect_;
}

void dbTechViaLayerRule::setRect(const Rect& r)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_rect = 1;
  rule->rect_ = r;
}

bool dbTechViaLayerRule::hasResistance()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->flags_.has_resistance == 1;
}

void dbTechViaLayerRule::setResistance(double r)
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  rule->flags_.has_resistance = 1;
  rule->resistance_ = r;
}

double dbTechViaLayerRule::getResistance()
{
  _dbTechViaLayerRule* rule = (_dbTechViaLayerRule*) this;
  return rule->resistance_;
}

dbTechViaLayerRule* dbTechViaLayerRule::create(dbTech* tech_,
                                               dbTechViaRule* rule_,
                                               dbTechLayer* layer)
{
  _dbTech* tech = (_dbTech*) tech_;
  _dbTechViaRule* rule = (_dbTechViaRule*) rule_;
  _dbTechViaLayerRule* layrule = tech->via_layer_rule_tbl_->create();
  layrule->layer_ = layer->getImpl()->getOID();
  rule->layer_rules_.push_back(layrule->getImpl()->getOID());
  return (dbTechViaLayerRule*) layrule;
}

dbTechViaLayerRule* dbTechViaLayerRule::create(dbTech* tech_,
                                               dbTechViaGenerateRule* rule_,
                                               dbTechLayer* layer)
{
  _dbTech* tech = (_dbTech*) tech_;
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) rule_;
  _dbTechViaLayerRule* layrule = tech->via_layer_rule_tbl_->create();
  layrule->layer_ = layer->getImpl()->getOID();
  rule->layer_rules_.push_back(layrule->getImpl()->getOID());
  return (dbTechViaLayerRule*) layrule;
}

dbTechViaLayerRule* dbTechViaLayerRule::getTechViaLayerRule(dbTech* tech_,
                                                            uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaLayerRule*) tech->via_layer_rule_tbl_->getPtr(dbid_);
}

void _dbTechViaLayerRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
