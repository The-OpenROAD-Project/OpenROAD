// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
// User Code Begin Includes
#include "odb/dbId.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
class _dbTechLayer;
// User Code End Classes

struct dbTechLayerWrongDirSpacingRuleFlags
{
  bool noneol_valid : 1;
  bool length_valid : 1;
  uint32_t spare_bits : 30;
};

class _dbTechLayerWrongDirSpacingRule : public _dbObject
{
 public:
  _dbTechLayerWrongDirSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerWrongDirSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerWrongDirSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerWrongDirSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerWrongDirSpacingRuleFlags flags_;
  int wrongdir_space_;
  int noneol_width_;
  int length_;
  int prl_length_;

  // User Code Begin Fields
  dbId<_dbTechLayer> layer_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerWrongDirSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerWrongDirSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
