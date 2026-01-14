// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerWidthTableRuleFlags
{
  bool wrong_direction : 1;
  bool orthogonal : 1;
  uint32_t spare_bits : 30;
};

class _dbTechLayerWidthTableRule : public _dbObject
{
 public:
  _dbTechLayerWidthTableRule(_dbDatabase*);

  bool operator==(const _dbTechLayerWidthTableRule& rhs) const;
  bool operator!=(const _dbTechLayerWidthTableRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerWidthTableRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerWidthTableRuleFlags flags_;
  dbVector<int> width_tbl_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerWidthTableRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerWidthTableRule& obj);
}  // namespace odb
   // Generator Code End Header
