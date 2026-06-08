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
class _dbUnfoldedRegionInst;

class _dbUnfoldedConn : public _dbObject
{
 public:
  _dbUnfoldedConn(_dbDatabase*);

  bool operator==(const _dbUnfoldedConn& rhs) const;
  bool operator!=(const _dbUnfoldedConn& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbUnfoldedConn& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipConn> chip_conn_;
  dbId<_dbUnfoldedRegionInst> top_region_;
  dbId<_dbUnfoldedRegionInst> bottom_region_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedConn& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedConn& obj);
}  // namespace odb
// Generator Code End Header