// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

struct dbTechLayerImplantSpacingRuleFlags
{
  bool horizontal : 1;
  bool vertical : 1;
  bool prl_valid : 1;
  bool except_abutted : 1;
  bool except_corner_touch : 1;
  uint32_t spare_bits : 27;
};

class _dbTechLayerImplantSpacingRule : public _dbObject
{
 public:
  _dbTechLayerImplantSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerImplantSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerImplantSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerImplantSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerImplantSpacingRuleFlags flags_;
  int min_spacing_;
  dbId<_dbTechLayer> second_layer_;
  int prl_;
  int length_;
  dbVector<dbId<_dbTechLayer>> intersect_layers_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerImplantSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerImplantSpacingRule& obj);
}  // namespace odb
// Generator Code End Header
