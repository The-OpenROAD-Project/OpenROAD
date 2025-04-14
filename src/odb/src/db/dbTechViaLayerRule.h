// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

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
  uint _direction : 2;
  uint _has_enclosure : 1;
  uint _has_width : 1;
  uint _has_overhang : 1;
  uint _has_metal_overhang : 1;
  uint _has_resistance : 1;
  uint _has_spacing : 1;
  uint _has_rect : 1;
  uint _spare_bits : 23;
};

class _dbTechViaLayerRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechViaLayerRuleFlags _flags;
  int _overhang1;
  int _overhang2;
  int _min_width;
  int _max_width;
  int _overhang;
  int _metal_overhang;
  int _spacing_x;
  int _spacing_y;
  double _resistance;
  Rect _rect;
  dbId<_dbTechLayer> _layer;

  _dbTechViaLayerRule(_dbDatabase*, const _dbTechViaLayerRule& v);
  _dbTechViaLayerRule(_dbDatabase*);
  ~_dbTechViaLayerRule();

  bool operator==(const _dbTechViaLayerRule& rhs) const;
  bool operator!=(const _dbTechViaLayerRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaLayerRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaLayerRule& v);

}  // namespace odb
