// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <vector>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbLib;
class _dbMaster;

class _dbAlignmentMarkerRule : public _dbObject
{
 public:
  _dbAlignmentMarkerRule(_dbDatabase*);

  bool operator==(const _dbAlignmentMarkerRule& rhs) const;
  bool operator!=(const _dbAlignmentMarkerRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbAlignmentMarkerRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbLib> lib_a_;
  dbId<_dbMaster> master_a_;
  dbId<_dbLib> lib_b_;
  dbId<_dbMaster> master_b_;
  std::vector<uint8_t> rel_orients_;
  int tolerance_;
};
dbIStream& operator>>(dbIStream& stream, _dbAlignmentMarkerRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbAlignmentMarkerRule& obj);
}  // namespace odb
// Generator Code End Header