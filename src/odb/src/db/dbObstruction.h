// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {

class _dbInst;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbObstructionFlags
{
  uint32_t slot_obs : 1;
  uint32_t fill_obs : 1;
  uint32_t pushed_down : 1;
  uint32_t has_min_spacing : 1;
  uint32_t has_effective_width : 1;
  uint32_t except_pg_nets : 1;
  uint32_t is_system_reserved : 1;
  uint32_t spare_bits : 25;
};

class _dbObstruction : public _dbObject
{
 public:
  _dbObstruction(_dbDatabase*, const _dbObstruction& o);
  _dbObstruction(_dbDatabase*);

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
