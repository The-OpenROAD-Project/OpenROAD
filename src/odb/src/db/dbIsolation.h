// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

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

  char* name_;
  dbId<_dbIsolation> next_entry_;
  std::string applies_to_;
  std::string clamp_value_;
  std::string isolation_signal_;
  std::string isolation_sense_;
  std::string location_;
  dbVector<std::string> isolation_cells_;
  dbId<_dbPowerDomain> power_domain_;
};
dbIStream& operator>>(dbIStream& stream, _dbIsolation& obj);
dbOStream& operator<<(dbOStream& stream, const _dbIsolation& obj);
}  // namespace odb
   // Generator Code End Header
