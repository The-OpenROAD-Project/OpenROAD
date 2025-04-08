// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "odb/odb.h"

// User Code Begin Includes
#include <string>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerEolKeepOutRuleFlags
{
  bool class_valid_ : 1;
  bool corner_only_ : 1;
  bool except_within_ : 1;
  uint spare_bits_ : 29;
};

class _dbTechLayerEolKeepOutRule : public _dbObject
{
 public:
  _dbTechLayerEolKeepOutRule(_dbDatabase*);

  bool operator==(const _dbTechLayerEolKeepOutRule& rhs) const;
  bool operator!=(const _dbTechLayerEolKeepOutRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerEolKeepOutRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerEolKeepOutRuleFlags flags_;
  int eol_width_;
  int backward_ext_;
  int forward_ext_;
  int side_ext_;
  int within_low_;
  int within_high_;
  std::string class_name_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolKeepOutRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerEolKeepOutRule& obj);
}  // namespace odb
   // Generator Code End Header
