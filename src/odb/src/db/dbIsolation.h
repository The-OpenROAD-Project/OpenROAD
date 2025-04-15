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
class _dbPowerDomain;

class _dbIsolation : public _dbObject
{
 public:
  _dbIsolation(_dbDatabase*);

  ~_dbIsolation();

  bool operator==(const _dbIsolation& rhs) const;
  bool operator!=(const _dbIsolation& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbIsolation& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbIsolation> _next_entry;
  std::string _applies_to;
  std::string _clamp_value;
  std::string _isolation_signal;
  std::string _isolation_sense;
  std::string _location;
  dbVector<std::string> _isolation_cells;
  dbId<_dbPowerDomain> _power_domain;
};
dbIStream& operator>>(dbIStream& stream, _dbIsolation& obj);
dbOStream& operator<<(dbOStream& stream, const _dbIsolation& obj);
}  // namespace odb
   // Generator Code End Header
