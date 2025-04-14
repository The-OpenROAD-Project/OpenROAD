// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class _dbInst;
class _dbBox;
class _dbGroup;
class dbIStream;
class dbOStream;

struct _dbRegionFlags
{
  dbRegionType::Value _type : 4;
  uint _invalid : 1;
  uint _spare_bits : 27;
};

class _dbRegion : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbRegionFlags _flags;
  char* _name;
  dbId<_dbInst> _insts;
  dbId<_dbBox> _boxes;
  dbId<_dbGroup> groups_;

  _dbRegion(_dbDatabase*);
  _dbRegion(_dbDatabase*, const _dbRegion& b);
  ~_dbRegion();

  bool operator==(const _dbRegion& rhs) const;
  bool operator!=(const _dbRegion& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbRegion& rhs) const;

  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r);
dbIStream& operator>>(dbIStream& stream, _dbRegion& r);

}  // namespace odb
