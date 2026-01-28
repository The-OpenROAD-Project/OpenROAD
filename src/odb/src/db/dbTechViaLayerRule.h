// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class _dbTechLayer;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

//
// These flags keep track of the variations between difference LEF versions
//
struct _dbTechViaLayerRuleFlags
{
  uint32_t direction : 2;
  uint32_t has_enclosure : 1;
  uint32_t has_width : 1;
  uint32_t has_overhang : 1;
  uint32_t has_metal_overhang : 1;
  uint32_t has_resistance : 1;
  uint32_t has_spacing : 1;
  uint32_t has_rect : 1;
  uint32_t spare_bits : 23;
};

class _dbTechViaLayerRule : public _dbObject
{
 public:
  _dbTechViaLayerRule(_dbDatabase*, const _dbTechViaLayerRule& v);
  _dbTechViaLayerRule(_dbDatabase*);

  bool operator==(const _dbTechViaLayerRule& rhs) const;
  bool operator!=(const _dbTechViaLayerRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbTechViaLayerRuleFlags flags_;
  int overhang1_;
  int overhang2_;
  int min_width_;
  int max_width_;
  int overhang_;
  int metal_overhang_;
  int spacing_x_;
  int spacing_y_;
  double resistance_;
  Rect rect_;
  dbId<_dbTechLayer> layer_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaLayerRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaLayerRule& v);

}  // namespace odb
