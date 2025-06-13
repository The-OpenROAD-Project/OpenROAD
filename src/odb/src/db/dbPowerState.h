// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbBlock.h"
#include "dbCore.h"
#include "dbPowerState.h"
#include "odb/db.h"
#include "odb/dbMap.h"
#include "odb/dbSet.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbPowerState : public _dbObject
{
 public:
  _dbPowerState(_dbDatabase*);

  ~_dbPowerState();

  bool operator==(const _dbPowerState& rhs) const;
  bool operator!=(const _dbPowerState& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPowerState& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbVector<dbPowerState::PowerStateEntry> _states;
  dbId<_dbPowerState> _next_entry;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerState& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerState& obj);
}  // namespace odb
   // Generator Code End Header