// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanInst;
template <class T>
class dbTable;

class _dbScanList : public _dbObject
{
 public:
  _dbScanList(_dbDatabase*);

  ~_dbScanList();

  bool operator==(const _dbScanList& rhs) const;
  bool operator!=(const _dbScanList& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbScanList& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  dbTable<_dbScanInst>* scan_insts_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanList& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj);
}  // namespace odb
   // Generator Code End Header
