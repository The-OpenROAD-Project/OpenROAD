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
class _dbUnfoldedRegion;

class _dbUnfoldedBump : public _dbObject
{
 public:
  _dbUnfoldedBump(_dbDatabase*);

  bool operator==(const _dbUnfoldedBump& rhs) const;
  bool operator!=(const _dbUnfoldedBump& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbUnfoldedBump& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBumpInst> chip_bump_inst_;
  dbId<_dbUnfoldedRegion> parent_region_;
  dbId<_dbUnfoldedBump> region_next_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedBump& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedBump& obj);
}  // namespace odb
// Generator Code End Header