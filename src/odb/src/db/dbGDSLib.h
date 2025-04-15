// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <ctime>
#include <string>

#include "dbCore.h"
#include "dbGDSStructure.h"
#include "dbHashTable.hpp"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGDSStructure;

class _dbGDSLib : public _dbObject
{
 public:
  std::string _libname;
  double _uu_per_dbu;
  double _dbu_per_meter;
  dbHashTable<_dbGDSStructure> _gdsstructure_hash;
  dbTable<_dbGDSStructure>* _gdsstructure_tbl;

  _dbGDSLib(_dbDatabase*, const _dbGDSLib& r);
  _dbGDSLib(_dbDatabase*);
  ~_dbGDSLib();

  bool operator==(const _dbGDSLib& rhs) const;
  bool operator!=(const _dbGDSLib& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  _dbGDSStructure* findStructure(const char* name);
};

dbIStream& operator>>(dbIStream& stream, std::tm& tm);
dbOStream& operator<<(dbOStream& stream, const std::tm& tm);

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj);
}  // namespace odb
