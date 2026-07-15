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
class _dbUnfoldedChipInst;
class _dbUnfoldedChipBumpInst;

struct dbUnfoldedChipRegionInstFlags
{
  uint32_t effective_side_ : 2;
  uint32_t spare_bits : 30;
};

class _dbUnfoldedChipRegionInst : public _dbObject
{
 public:
  _dbUnfoldedChipRegionInst(_dbDatabase*);

  bool operator==(const _dbUnfoldedChipRegionInst& rhs) const;
  bool operator!=(const _dbUnfoldedChipRegionInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedChipRegionInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbUnfoldedChipRegionInstFlags flags_;
  dbId<_dbChipRegionInst> chip_region_inst_;
  dbId<_dbUnfoldedChipInst> parent_chip_;
  dbId<_dbUnfoldedChipRegionInst> chip_next_;
  dbId<_dbUnfoldedChipBumpInst> bump_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipRegionInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipRegionInst& obj);
}  // namespace odb
// Generator Code End Header