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
class _dbChipConn;
class _dbUnfoldedChipRegionInst;

class _dbUnfoldedChipConn : public _dbObject
{
 public:
  _dbUnfoldedChipConn(_dbDatabase*);

  bool operator==(const _dbUnfoldedChipConn& rhs) const;
  bool operator!=(const _dbUnfoldedChipConn& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedChipConn& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipConn> chip_conn_;
  dbId<_dbUnfoldedChipRegionInst> top_region_;
  dbId<_dbUnfoldedChipRegionInst> bottom_region_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipConn& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipConn& obj);
}  // namespace odb
// Generator Code End Header