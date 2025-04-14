// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbHashTable.h"

namespace odb {

class dbOStream;
class dbIStream;
class _dbName;
class _dbDatabase;
struct MemInfo;

class _dbNameCache
{
 public:
  dbTable<_dbName>* _name_tbl;
  dbHashTable<_dbName> _name_hash;

  _dbNameCache(_dbDatabase* db,
               dbObject* owner,
               dbObjectTable* (dbObject::*m)(dbObjectType));
  ~_dbNameCache();

  bool operator==(const _dbNameCache& rhs) const;
  bool operator!=(const _dbNameCache& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  // Find the name, returns 0 if the name does not exists.
  uint findName(const char* name);

  // Add this name to the table if it does not exists.
  // increment the reference count to this name
  uint addName(const char* name);

  // Remove this name to the table if the ref-cnt is 0
  void removeName(uint id);

  // Remove the string this id represents
  const char* getName(uint id);
};

dbOStream& operator<<(dbOStream& stream, const _dbNameCache& net);
dbIStream& operator>>(dbIStream& stream, _dbNameCache& net);

}  // namespace odb
