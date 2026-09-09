// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <tuple>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {
// User Code Begin Consts
// User Code End Consts
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayerCutClassRule;
// User Code Begin Classes
// User Code End Classes

// User Code Begin Types
// User Code End Types

struct dbTechLayerCutEnclosureTableDefRuleFlags
{
  bool cut_class_valid : 1;
  uint32_t spare_bits : 31;
};

// User Code Begin Structs
// User Code End Structs

class _dbTechLayerCutEnclosureTableDefRule : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  _dbTechLayerCutEnclosureTableDefRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutEnclosureTableDefRule& rhs) const;
  bool operator!=(const _dbTechLayerCutEnclosureTableDefRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutEnclosureTableDefRule& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  // User Code End Methods

  dbTechLayerCutEnclosureTableDefRuleFlags flags_;
  dbId<_dbTechLayerCutClassRule> cut_class_;
  dbVector<std::tuple<int, int, int, int, int>> default_rows_;
  dbVector<std::tuple<int, int, int, int, int, int, bool>> width_rows_;

  // User Code Begin Fields
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutEnclosureTableDefRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureTableDefRule& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
// Generator Code End Header