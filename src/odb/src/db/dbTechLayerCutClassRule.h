// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerCutClassRuleFlags
{
  bool length_valid : 1;
  bool cuts_valid : 1;
  uint32_t spare_bits : 30;
};

class _dbTechLayerCutClassRule : public _dbObject
{
 public:
  _dbTechLayerCutClassRule(_dbDatabase*);

  ~_dbTechLayerCutClassRule();

  bool operator==(const _dbTechLayerCutClassRule& rhs) const;
  bool operator!=(const _dbTechLayerCutClassRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutClassRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerCutClassRuleFlags flags_;
  char* name_;
  int width_;
  int length_;
  int num_cuts_;
  dbId<_dbTechLayerCutClassRule> next_entry_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutClassRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutClassRule& obj);
}  // namespace odb
   // Generator Code End Header
