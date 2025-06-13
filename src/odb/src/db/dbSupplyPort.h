// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
class _dbSupplyNet;

class _dbSupplyPort : public _dbObject
{
 public:
  _dbSupplyPort(_dbDatabase*);

  ~_dbSupplyPort();

  bool operator==(const _dbSupplyPort& rhs) const;
  bool operator!=(const _dbSupplyPort& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSupplyPort& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbSupplyPort> _next_entry;
  std::string _direction;
  dbId<_dbPowerDomain> _domain;
  dbId<_dbSupplyNet> _supplynet;
};
dbIStream& operator>>(dbIStream& stream, _dbSupplyPort& obj);
dbOStream& operator<<(dbOStream& stream, const _dbSupplyPort& obj);
}  // namespace odb
   // Generator Code End Header