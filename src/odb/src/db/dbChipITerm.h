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
class _dbChipBTerm;
class _dbChipRegionInst;

class _dbChipITerm : public _dbObject
{
 public:
  _dbChipITerm(_dbDatabase*);

  bool operator==(const _dbChipITerm& rhs) const;
  bool operator!=(const _dbChipITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipITerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBTerm> chip_bterm_;
  dbId<_dbChipRegionInst> chip_region_inst_;
  dbId<_dbChipITerm> region_next_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipITerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipITerm& obj);
}  // namespace odb
// Generator Code End Header