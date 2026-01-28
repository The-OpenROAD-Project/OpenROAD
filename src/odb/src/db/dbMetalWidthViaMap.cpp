// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbMetalWidthViaMap.h"

#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>

#include "dbTech.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbMetalWidthViaMap>;

bool _dbMetalWidthViaMap::operator==(const _dbMetalWidthViaMap& rhs) const
{
  if (via_cut_class_ != rhs.via_cut_class_) {
    return false;
  }
  if (cut_layer_ != rhs.cut_layer_) {
    return false;
  }
  if (below_layer_width_low_ != rhs.below_layer_width_low_) {
    return false;
  }
  if (below_layer_width_high_ != rhs.below_layer_width_high_) {
    return false;
  }
  if (above_layer_width_low_ != rhs.above_layer_width_low_) {
    return false;
  }
  if (above_layer_width_high_ != rhs.above_layer_width_high_) {
    return false;
  }
  if (via_name_ != rhs.via_name_) {
    return false;
  }
  if (pg_via_ != rhs.pg_via_) {
    return false;
  }

  return true;
}

bool _dbMetalWidthViaMap::operator<(const _dbMetalWidthViaMap& rhs) const
{
  return true;
}

_dbMetalWidthViaMap::_dbMetalWidthViaMap(_dbDatabase* db)
{
  via_cut_class_ = false;
  below_layer_width_low_ = 0;
  below_layer_width_high_ = 0;
  above_layer_width_low_ = 0;
  above_layer_width_high_ = 0;
  pg_via_ = false;
}

dbIStream& operator>>(dbIStream& stream, _dbMetalWidthViaMap& obj)
{
  stream >> obj.via_cut_class_;
  stream >> obj.cut_layer_;
  stream >> obj.below_layer_width_low_;
  stream >> obj.below_layer_width_high_;
  stream >> obj.above_layer_width_low_;
  stream >> obj.above_layer_width_high_;
  stream >> obj.via_name_;
  stream >> obj.pg_via_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMetalWidthViaMap& obj)
{
  stream << obj.via_cut_class_;
  stream << obj.cut_layer_;
  stream << obj.below_layer_width_low_;
  stream << obj.below_layer_width_high_;
  stream << obj.above_layer_width_low_;
  stream << obj.above_layer_width_high_;
  stream << obj.via_name_;
  stream << obj.pg_via_;
  return stream;
}

void _dbMetalWidthViaMap::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["via_name"].add(via_name_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbMetalWidthViaMap - Methods
//
////////////////////////////////////////////////////////////////////

void dbMetalWidthViaMap::setViaCutClass(bool via_cut_class)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->via_cut_class_ = via_cut_class;
}

bool dbMetalWidthViaMap::isViaCutClass() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->via_cut_class_;
}

void dbMetalWidthViaMap::setCutLayer(dbTechLayer* cut_layer)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->cut_layer_ = cut_layer->getImpl()->getOID();
}

void dbMetalWidthViaMap::setBelowLayerWidthLow(int below_layer_width_low)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->below_layer_width_low_ = below_layer_width_low;
}

int dbMetalWidthViaMap::getBelowLayerWidthLow() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->below_layer_width_low_;
}

void dbMetalWidthViaMap::setBelowLayerWidthHigh(int below_layer_width_high)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->below_layer_width_high_ = below_layer_width_high;
}

int dbMetalWidthViaMap::getBelowLayerWidthHigh() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->below_layer_width_high_;
}

void dbMetalWidthViaMap::setAboveLayerWidthLow(int above_layer_width_low)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->above_layer_width_low_ = above_layer_width_low;
}

int dbMetalWidthViaMap::getAboveLayerWidthLow() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->above_layer_width_low_;
}

void dbMetalWidthViaMap::setAboveLayerWidthHigh(int above_layer_width_high)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->above_layer_width_high_ = above_layer_width_high;
}

int dbMetalWidthViaMap::getAboveLayerWidthHigh() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->above_layer_width_high_;
}

void dbMetalWidthViaMap::setViaName(const std::string& via_name)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->via_name_ = via_name;
}

std::string dbMetalWidthViaMap::getViaName() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->via_name_;
}

void dbMetalWidthViaMap::setPgVia(bool pg_via)
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;

  obj->pg_via_ = pg_via;
}

bool dbMetalWidthViaMap::isPgVia() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  return obj->pg_via_;
}

// User Code Begin dbMetalWidthViaMapPublicMethods

dbTechLayer* dbMetalWidthViaMap::getCutLayer() const
{
  _dbMetalWidthViaMap* obj = (_dbMetalWidthViaMap*) this;
  dbTech* tech = (dbTech*) obj->getOwner();
  return dbTechLayer::getTechLayer(tech, obj->cut_layer_);
}

dbMetalWidthViaMap* dbMetalWidthViaMap::create(dbTech* tech)
{
  _dbTech* _tech = (_dbTech*) tech;
  _dbMetalWidthViaMap* via_map = _tech->metal_width_via_map_tbl_->create();
  return (dbMetalWidthViaMap*) via_map;
}

void dbMetalWidthViaMap::destroy(dbMetalWidthViaMap* via_map)
{
  _dbTech* tech = (_dbTech*) via_map->getImpl()->getOwner();
  dbProperty::destroyProperties(via_map);
  tech->metal_width_via_map_tbl_->destroy((_dbMetalWidthViaMap*) via_map);
}

dbMetalWidthViaMap* dbMetalWidthViaMap::getMetalWidthViaMap(dbTech* tech,
                                                            uint32_t dbid)
{
  _dbTech* _tech = (_dbTech*) tech;
  return (dbMetalWidthViaMap*) _tech->metal_width_via_map_tbl_->getPtr(dbid);
}

// User Code End dbMetalWidthViaMapPublicMethods
}  // namespace odb
   // Generator Code End Cpp
