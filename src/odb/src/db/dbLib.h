// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
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
  // PERSISTANT-MEMBERS
  int _lef_units;
  int _dbu_per_micron;  // cached value from dbTech
  char _hier_delimiter;
  char _left_bus_delimiter;
  char _right_bus_delimiter;
  char _spare;
  char* _name;
  dbHashTable<_dbMaster> _master_hash;
  dbHashTable<_dbSite> _site_hash;
  dbId<_dbTech> _tech;

  // NON-PERSISTANT-MEMBERS
  dbTable<_dbMaster>* _master_tbl;
  dbTable<_dbSite>* _site_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache* _name_cache;

  dbPropertyItr* _prop_itr;

  _dbLib(_dbDatabase* db);
  ~_dbLib();
  bool operator==(const _dbLib& rhs) const;
  bool operator!=(const _dbLib& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  _dbTech* getTech();
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib);
dbIStream& operator>>(dbIStream& stream, _dbLib& lib);

}  // namespace odb
