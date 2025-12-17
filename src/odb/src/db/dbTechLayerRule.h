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
  uint block_rule : 1;
  uint spare_bits : 31;
};

class _dbTechLayerRule : public _dbObject
{
 public:
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
    if (layer_ < rhs.layer_) {
      return true;
    }

    if (layer_ > rhs.layer_) {
      return false;
    }

    return non_default_rule_ < rhs.non_default_rule_;
  }

  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbTechLayerRuleFlags flags_;
  uint width_;
  uint spacing_;
  double resistance_;
  double capacitance_;
  double edge_capacitance_;
  uint wire_extension_;
  dbId<_dbTechNonDefaultRule> non_default_rule_;
  dbId<_dbTechLayer> layer_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerRule& rule);

}  // namespace odb
