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
#include "dbMetalWidthViaMap.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
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

void _dbMetalWidthViaMap::differences(dbDiff& diff,
                                      const char* field,
                                      const _dbMetalWidthViaMap& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(via_cut_class_);
  DIFF_FIELD(cut_layer_);
  DIFF_FIELD(below_layer_width_low_);
  DIFF_FIELD(below_layer_width_high_);
  DIFF_FIELD(above_layer_width_low_);
  DIFF_FIELD(above_layer_width_high_);
  DIFF_FIELD(via_name_);
  DIFF_FIELD(pg_via_);
  DIFF_END
}

void _dbMetalWidthViaMap::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(via_cut_class_);
  DIFF_OUT_FIELD(cut_layer_);
  DIFF_OUT_FIELD(below_layer_width_low_);
  DIFF_OUT_FIELD(below_layer_width_high_);
  DIFF_OUT_FIELD(above_layer_width_low_);
  DIFF_OUT_FIELD(above_layer_width_high_);
  DIFF_OUT_FIELD(via_name_);
  DIFF_OUT_FIELD(pg_via_);

  DIFF_END
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

_dbMetalWidthViaMap::_dbMetalWidthViaMap(_dbDatabase* db,
                                         const _dbMetalWidthViaMap& r)
{
  via_cut_class_ = r.via_cut_class_;
  cut_layer_ = r.cut_layer_;
  below_layer_width_low_ = r.below_layer_width_low_;
  below_layer_width_high_ = r.below_layer_width_high_;
  above_layer_width_low_ = r.above_layer_width_low_;
  above_layer_width_high_ = r.above_layer_width_high_;
  via_name_ = r.via_name_;
  pg_via_ = r.pg_via_;
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
  _dbMetalWidthViaMap* via_map = _tech->_metal_width_via_map_tbl->create();
  return (dbMetalWidthViaMap*) via_map;
}

void dbMetalWidthViaMap::destroy(dbMetalWidthViaMap* via_map)
{
  _dbTech* tech = (_dbTech*) via_map->getImpl()->getOwner();
  dbProperty::destroyProperties(via_map);
  tech->_metal_width_via_map_tbl->destroy((_dbMetalWidthViaMap*) via_map);
}

dbMetalWidthViaMap* dbMetalWidthViaMap::getMetalWidthViaMap(dbTech* tech,
                                                            uint dbid)
{
  _dbTech* _tech = (_dbTech*) tech;
  return (dbMetalWidthViaMap*) _tech->_metal_width_via_map_tbl->getPtr(dbid);
}

// User Code End dbMetalWidthViaMapPublicMethods
}  // namespace odb
   // Generator Code End Cpp