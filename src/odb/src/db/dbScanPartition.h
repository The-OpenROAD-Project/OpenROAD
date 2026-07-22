// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbScanPin.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanList;

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
