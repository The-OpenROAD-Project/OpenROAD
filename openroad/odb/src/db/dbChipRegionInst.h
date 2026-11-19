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
class _dbChipRegion;
class _dbChipInst;
class _dbChipBumpInst;

class _dbChipRegionInst : public _dbObject
{
 public:
  _dbChipRegionInst(_dbDatabase*);

  bool operator==(const _dbChipRegionInst& rhs) const;
  bool operator!=(const _dbChipRegionInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbChipRegionInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipRegion> region_;
  dbId<_dbChipInst> parent_chipinst_;
  dbId<_dbChipRegionInst> chip_region_inst_next_;
  dbId<_dbChipBumpInst> chip_bump_insts_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipRegionInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipRegionInst& obj);
}  // namespace odb
// Generator Code End Header