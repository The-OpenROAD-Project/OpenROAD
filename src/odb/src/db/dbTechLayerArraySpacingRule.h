// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <map>

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayerCutClassRule;

struct dbTechLayerArraySpacingRuleFlags
{
  bool parallel_overlap_ : 1;
  bool long_array_ : 1;
  bool via_width_valid_ : 1;
  bool within_valid_ : 1;
  uint spare_bits_ : 28;
};

class _dbTechLayerArraySpacingRule : public _dbObject
{
 public:
  _dbTechLayerArraySpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerArraySpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerArraySpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerArraySpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerArraySpacingRuleFlags flags_;
  int via_width_;
  int cut_spacing_;
  int within_;
  int array_width_;
  std::map<int, int> array_spacing_map_;
  dbId<_dbTechLayerCutClassRule> cut_class_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerArraySpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerArraySpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
