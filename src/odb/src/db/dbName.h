// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class dbIStream;
class dbOStream;

//
// dbName - This class is used to cache strings that are repeated frequently.
// For example, property names are repeated frequently.
//
// Net and Instances names are unique and should not use the dbName cache.
//
class _dbName : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  char* _name;
  dbId<_dbName> _next_entry;
  uint _ref_cnt;

  _dbName(_dbDatabase*);
  _dbName(_dbDatabase*, const _dbName& n);
  ~_dbName();

  bool operator==(const _dbName& rhs) const;
  bool operator!=(const _dbName& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbName& rhs) const;
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbName& n);
dbIStream& operator>>(dbIStream& stream, _dbName& n);

}  // namespace odb
