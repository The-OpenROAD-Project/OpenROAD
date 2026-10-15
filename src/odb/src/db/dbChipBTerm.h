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
class _dbInst;
class _dbChip;
class _dbChipRegion;
class _dbNet;
class _dbBTerm;

class _dbChipBTerm : public _dbObject
{
 public:
  _dbChipBTerm(_dbDatabase*);

  bool operator==(const _dbChipBTerm& rhs) const;
  bool operator!=(const _dbChipBTerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipBTerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbInst> inst_;
  dbId<_dbChip> chip_;
  dbId<_dbChipRegion> chip_region_;
  dbId<_dbNet> net_;
  dbId<_dbBTerm> bterm_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipBTerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipBTerm& obj);
}  // namespace odb
// Generator Code End Header