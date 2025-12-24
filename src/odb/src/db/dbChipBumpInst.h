// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipBump;
class _dbChipRegionInst;

class _dbChipBumpInst : public _dbObject
{
 public:
  _dbChipBumpInst(_dbDatabase*);

  bool operator==(const _dbChipBumpInst& rhs) const;
  bool operator!=(const _dbChipBumpInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipBumpInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBump> chip_bump_;
  dbId<_dbChipRegionInst> chip_region_inst_;
  dbId<_dbChipBumpInst> region_next_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipBumpInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipBumpInst& obj);
}  // namespace odb
// Generator Code End Header