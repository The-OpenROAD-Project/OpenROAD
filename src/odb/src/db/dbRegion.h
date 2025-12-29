// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class _dbDatabase;
class _dbInst;
class _dbBox;
class _dbGroup;
class dbIStream;
class dbOStream;

struct _dbRegionFlags
{
  dbRegionType::Value type : 4;
  uint32_t invalid : 1;
  uint32_t spare_bits : 27;
};

class _dbRegion : public _dbObject
{
 public:
  _dbRegion(_dbDatabase*);
  _dbRegion(_dbDatabase*, const _dbRegion& b);
  ~_dbRegion();

  bool operator==(const _dbRegion& rhs) const;
  bool operator!=(const _dbRegion& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbRegion& rhs) const;

  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbRegionFlags flags_;
  char* name_;
  dbId<_dbInst> insts_;
  dbId<_dbBox> boxes_;
  dbId<_dbGroup> groups_;
};

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r);
dbIStream& operator>>(dbIStream& stream, _dbRegion& r);

}  // namespace odb
