// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerEolExtensionRuleFlags
{
  bool parallel_only_ : 1;
  uint spare_bits_ : 31;
};

class _dbTechLayerEolExtensionRule : public _dbObject
{
 public:
  _dbTechLayerEolExtensionRule(_dbDatabase*);

  bool operator==(const _dbTechLayerEolExtensionRule& rhs) const;
  bool operator!=(const _dbTechLayerEolExtensionRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerEolExtensionRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerEolExtensionRuleFlags flags_;
  int spacing_;
  dbVector<std::pair<int, int>> extension_tbl_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolExtensionRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerEolExtensionRule& obj);
}  // namespace odb
   // Generator Code End Header
