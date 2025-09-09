// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbLogicPort : public _dbObject
{
 public:
  _dbLogicPort(_dbDatabase*);

  ~_dbLogicPort();

  bool operator==(const _dbLogicPort& rhs) const;
  bool operator!=(const _dbLogicPort& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbLogicPort& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbLogicPort> _next_entry;
  std::string direction;
};
dbIStream& operator>>(dbIStream& stream, _dbLogicPort& obj);
dbOStream& operator<<(dbOStream& stream, const _dbLogicPort& obj);
}  // namespace odb
   // Generator Code End Header
