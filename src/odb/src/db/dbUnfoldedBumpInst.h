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
class _dbChipBumpInst;
class _dbUnfoldedChipRegionInst;

class _dbUnfoldedBumpInst : public _dbObject
{
 public:
  _dbUnfoldedBumpInst(_dbDatabase*);

  bool operator==(const _dbUnfoldedBumpInst& rhs) const;
  bool operator!=(const _dbUnfoldedBumpInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedBumpInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBumpInst> chip_bump_inst_;
  dbId<_dbUnfoldedChipRegionInst> parent_region_;
  dbId<_dbUnfoldedBumpInst> region_next_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedBumpInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedBumpInst& obj);
}  // namespace odb
// Generator Code End Header