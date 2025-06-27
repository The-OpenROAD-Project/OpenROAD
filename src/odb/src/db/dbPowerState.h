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
// User Code Begin Includes
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
// User Code End Classes

// User Code Begin Structs
// User Code End Structs

class _dbPowerState : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  _dbPowerState(_dbDatabase*);

  ~_dbPowerState();

  bool operator==(const _dbPowerState& rhs) const;
  bool operator!=(const _dbPowerState& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPowerState& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  // User Code End Methods

  char* _name;
  dbVector<dbPowerState::PowerStateEntry> _states;
  dbId<_dbPowerState> _next_entry;

  // User Code Begin Fields
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbPowerState& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerState& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
   // Generator Code End Header