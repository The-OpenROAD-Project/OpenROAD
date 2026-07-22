// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <ctime>
#include <string>

#include "dbCore.h"
#include "dbGDSStructure.h"
#include "dbHashTable.hpp"
#include "odb/db.h"
#include "odb/dbObject.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGDSStructure;

class _dbGDSLib : public _dbObject
{
 public:
  _dbGDSLib(_dbDatabase*, const _dbGDSLib& r);
  _dbGDSLib(_dbDatabase*);
  ~_dbGDSLib();

  bool operator==(const _dbGDSLib& rhs) const;
  bool operator!=(const _dbGDSLib& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  _dbGDSStructure* findStructure(const char* name);

  std::string lib_name_;
  double uu_per_dbu_;
  double dbu_per_meter_;
  dbHashTable<_dbGDSStructure> gdsstructure_hash_;
  dbTable<_dbGDSStructure>* gdsstructure_tbl_;
};

dbIStream& operator>>(dbIStream& stream, std::tm& tm);
dbOStream& operator<<(dbOStream& stream, const std::tm& tm);

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj);
}  // namespace odb
