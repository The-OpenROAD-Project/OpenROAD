// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <tuple>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayerCutClassRule;

struct dbTechLayerCutEnclosureTableDefRuleFlags
{
  bool cut_class_valid : 1;
  uint32_t spare_bits : 31;
};

class _dbTechLayerCutEnclosureTableDefRule : public _dbObject
{
 public:
  _dbTechLayerCutEnclosureTableDefRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutEnclosureTableDefRule& rhs) const;
  bool operator!=(const _dbTechLayerCutEnclosureTableDefRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutEnclosureTableDefRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerCutEnclosureTableDefRuleFlags flags_;
  dbId<_dbTechLayerCutClassRule> cut_class_;
  dbVector<std::tuple<int, int, int, int, int>> default_rows_;
  dbVector<std::tuple<int, int, int, int, int, int>> width_rows_;
};
dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutEnclosureTableDefRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureTableDefRule& obj);
}  // namespace odb
// Generator Code End Header