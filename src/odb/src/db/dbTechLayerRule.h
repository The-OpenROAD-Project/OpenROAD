// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class _dbTech;
class _dbBlock;
class _dbTechLayer;
class _dbTechNonDefaultRule;
class dbIStream;
class dbOStream;

struct _dbTechLayerRuleFlags
{
  uint _block_rule : 1;
  uint _spare_bits : 31;
};

class _dbTechLayerRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechLayerRuleFlags _flags;
  uint _width;
  uint _spacing;
  double _resistance;
  double _capacitance;
  double _edge_capacitance;
  uint _wire_extension;
  dbId<_dbTechNonDefaultRule> _non_default_rule;
  dbId<_dbTechLayer> _layer;

  _dbTechLayerRule(_dbDatabase*);
  _dbTechLayerRule(_dbDatabase*, const _dbTechLayerRule& r);
  ~_dbTechLayerRule();

  _dbTech* getTech();
  _dbBlock* getBlock();

  bool operator==(const _dbTechLayerRule& rhs) const;
  bool operator!=(const _dbTechLayerRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerRule& rhs) const
  {
    if (_layer < rhs._layer) {
      return true;
    }

    if (_layer > rhs._layer) {
      return false;
    }

    return _non_default_rule < rhs._non_default_rule;
  }

  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerRule& rule);

}  // namespace odb
