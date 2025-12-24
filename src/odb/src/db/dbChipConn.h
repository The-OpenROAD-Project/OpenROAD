// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChip;
class _dbChipRegionInst;
class _dbChipInst;

class _dbChipConn : public _dbObject
{
 public:
  _dbChipConn(_dbDatabase*);

  bool operator==(const _dbChipConn& rhs) const;
  bool operator!=(const _dbChipConn& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipConn& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  int thickness_;
  dbId<_dbChip> chip_;
  dbId<_dbChipConn> chip_conn_next_;
  dbId<_dbChipRegionInst> top_region_;
  std::vector<dbId<_dbChipInst>> top_region_path_;
  dbId<_dbChipRegionInst> bottom_region_;
  std::vector<dbId<_dbChipInst>> bottom_region_path_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipConn& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipConn& obj);
}  // namespace odb
// Generator Code End Header