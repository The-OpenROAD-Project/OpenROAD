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
class _dbChipRegionInst;
class _dbUnfoldedChip;
class _dbUnfoldedBump;

class _dbUnfoldedRegion : public _dbObject
{
 public:
  _dbUnfoldedRegion(_dbDatabase*);

  bool operator==(const _dbUnfoldedRegion& rhs) const;
  bool operator!=(const _dbUnfoldedRegion& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedRegion& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipRegionInst> chip_region_inst_;
  uint32_t effective_side_;
  dbId<_dbUnfoldedChip> parent_chip_;
  dbId<_dbUnfoldedRegion> chip_next_;
  dbId<_dbUnfoldedBump> bump_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedRegion& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedRegion& obj);
}  // namespace odb
// Generator Code End Header