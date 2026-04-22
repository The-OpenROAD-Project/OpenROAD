// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <tuple>
#include <vector>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipInst;
class _dbChipRegionInst;

class _dbChipPath : public _dbObject
{
 public:
  _dbChipPath(_dbDatabase*);

  ~_dbChipPath();

  bool operator==(const _dbChipPath& rhs) const;
  bool operator!=(const _dbChipPath& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipPath& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* name_;
  std::vector<
      std::tuple<std::vector<dbId<_dbChipInst>>, dbId<_dbChipRegionInst>, bool>>
      entries_;
  dbId<_dbChipPath> next_entry_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipPath& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipPath& obj);
}  // namespace odb
// Generator Code End Header
