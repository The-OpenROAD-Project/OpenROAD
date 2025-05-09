// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbBlock.h"
#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanPin;
template <class T>
class dbTable;
class _dbScanChain;

class _dbDft : public _dbObject
{
 public:
  _dbDft(_dbDatabase*);

  ~_dbDft();

  bool operator==(const _dbDft& rhs) const;
  bool operator!=(const _dbDft& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbDft& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  void initialize();
  // User Code End Methods

  bool scan_inserted_;
  dbTable<_dbScanPin>* scan_pins_;
  dbTable<_dbScanChain>* scan_chains_;
};
dbIStream& operator>>(dbIStream& stream, _dbDft& obj);
dbOStream& operator<<(dbOStream& stream, const _dbDft& obj);
}  // namespace odb
// Generator Code End Header
