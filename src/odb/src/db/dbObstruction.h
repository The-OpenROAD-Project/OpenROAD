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
  uint slot_obs : 1;
  uint fill_obs : 1;
  uint pushed_down : 1;
  uint has_min_spacing : 1;
  uint has_effective_width : 1;
  uint except_pg_nets : 1;
  uint _is_system_reserved : 1;
  uint spare_bits : 25;
};

class _dbObstruction : public _dbObject
{
 public:
  _dbObstruction(_dbDatabase*, const _dbObstruction& o);
  _dbObstruction(_dbDatabase*);
  ~_dbObstruction();

  bool operator==(const _dbObstruction& rhs) const;
  bool operator!=(const _dbObstruction& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbObstruction& rhs) const;
  void collectMemInfo(MemInfo& info);

  _dbObstructionFlags flags_;
  dbId<_dbInst> inst_;
  dbId<_dbBox> bbox_;
  int min_spacing_;
  int effective_width_;
};

dbOStream& operator<<(dbOStream& stream, const _dbObstruction& obs);
dbIStream& operator>>(dbIStream& stream, _dbObstruction& obs);

}  // namespace odb
