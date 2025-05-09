// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "dbScanPin.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanList;
template <class T>
class dbTable;

class _dbScanPartition : public _dbObject
{
 public:
  _dbScanPartition(_dbDatabase*);

  ~_dbScanPartition();

  bool operator==(const _dbScanPartition& rhs) const;
  bool operator!=(const _dbScanPartition& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbScanPartition& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  std::string name_;
  dbTable<_dbScanList>* scan_lists_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanPartition& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanPartition& obj);
}  // namespace odb
   // Generator Code End Header
