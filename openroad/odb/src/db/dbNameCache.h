// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbHashTable.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

class dbOStream;
class dbIStream;
class _dbName;
class _dbDatabase;
struct MemInfo;

class _dbNameCache
{
 public:
  _dbNameCache(_dbDatabase* db,
               dbObject* owner,
               dbObjectTable* (dbObject::*m)(dbObjectType));
  ~_dbNameCache();

  bool operator==(const _dbNameCache& rhs) const;
  bool operator!=(const _dbNameCache& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  // Find the name, returns 0 if the name does not exists.
  uint32_t findName(const char* name);

  // Add this name to the table if it does not exists.
  // increment the reference count to this name
  uint32_t addName(const char* name);

  // Remove this name to the table if the ref-cnt is 0
  void removeName(uint32_t id);

  // Remove the string this id represents
  const char* getName(uint32_t id);

  dbTable<_dbName>* name_tbl_;
  dbHashTable<_dbName> name_hash_;
};

dbOStream& operator<<(dbOStream& stream, const _dbNameCache& net);
dbIStream& operator>>(dbIStream& stream, _dbNameCache& net);

}  // namespace odb
