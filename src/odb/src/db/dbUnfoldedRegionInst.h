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
class _dbUnfoldedBumpInst;

struct dbUnfoldedRegionInstFlags
{
  uint32_t effective_side_ : 2 = 0;
  uint32_t spare_bits : 30;
};

class _dbUnfoldedRegionInst : public _dbObject
{
 public:
  _dbUnfoldedRegionInst(_dbDatabase*);

  bool operator==(const _dbUnfoldedRegionInst& rhs) const;
  bool operator!=(const _dbUnfoldedRegionInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedRegionInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbUnfoldedRegionInstFlags flags_;
  dbId<_dbChipRegionInst> chip_region_inst_;
  dbId<_dbUnfoldedChipInst> parent_chip_;
  dbId<_dbUnfoldedRegionInst> chip_next_;
  dbId<_dbUnfoldedBumpInst> bump_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedRegionInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedRegionInst& obj);
}  // namespace odb
// Generator Code End Header