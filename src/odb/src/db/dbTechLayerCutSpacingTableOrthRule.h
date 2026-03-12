// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
// User Code Begin Includes
#include <utility>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbTechLayerCutSpacingTableOrthRule : public _dbObject
{
 public:
  _dbTechLayerCutSpacingTableOrthRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutSpacingTableOrthRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingTableOrthRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingTableOrthRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  //{cutWithin, orthoSpacing}
  dbVector<std::pair<int, int>> spacing_tbl_;
};
dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutSpacingTableOrthRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutSpacingTableOrthRule& obj);
}  // namespace odb
   // Generator Code End Header
