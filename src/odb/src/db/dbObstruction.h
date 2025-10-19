// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/odb.h"

namespace odb {

class _dbInst;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbObstructionFlags
{
  uint _slot_obs : 1;
  uint _fill_obs : 1;
  uint _pushed_down : 1;
  uint _has_min_spacing : 1;
  uint _has_effective_width : 1;
  uint _except_pg_nets : 1;
  uint _is_system_reserved : 1;
  uint _spare_bits : 25;
};

class _dbObstruction : public _dbObject
{
 public:
  _dbObstructionFlags _flags;
  dbId<_dbInst> _inst;
  dbId<_dbBox> _bbox;
  int _min_spacing;
  int _effective_width;

  _dbObstruction(_dbDatabase*, const _dbObstruction& o);
  _dbObstruction(_dbDatabase*);
  ~_dbObstruction();

  bool operator==(const _dbObstruction& rhs) const;
  bool operator!=(const _dbObstruction& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbObstruction& rhs) const;
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbObstruction& obs);
dbIStream& operator>>(dbIStream& stream, _dbObstruction& obs);

}  // namespace odb
