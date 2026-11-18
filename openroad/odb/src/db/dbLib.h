// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"

namespace odb {

class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTech;
class _dbMaster;
class _dbSite;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbLib : public _dbObject
{
 public:
  _dbLib(_dbDatabase* db);
  ~_dbLib();
  bool operator==(const _dbLib& rhs) const;
  bool operator!=(const _dbLib& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  _dbTech* getTech();
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  int lef_units_;
  int dbu_per_micron_;  // cached value from dbTech
  char hier_delimiter_;
  char left_bus_delimiter_;
  char right_bus_delimiter_;
  char spare_;
  char* name_;
  dbHashTable<_dbMaster> master_hash_;
  dbHashTable<_dbSite> site_hash_;
  dbId<_dbTech> tech_;

  // NON-PERSISTANT-MEMBERS
  dbTable<_dbMaster>* master_tbl_;
  dbTable<_dbSite>* site_tbl_;
  dbTable<_dbProperty>* prop_tbl_;
  _dbNameCache* name_cache_;

  dbPropertyItr* prop_itr_;
};

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib);
dbIStream& operator>>(dbIStream& stream, _dbLib& lib);

}  // namespace odb
