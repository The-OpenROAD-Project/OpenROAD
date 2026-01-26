// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanInst;

class _dbScanList : public _dbObject
{
 public:
  _dbScanList(_dbDatabase*);

  bool operator==(const _dbScanList& rhs) const;
  bool operator!=(const _dbScanList& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbScanList& rhs) const;
  void collectMemInfo(MemInfo& info);

  // As the elements of a free dbTable are 12 bytes long, we need this
  // additional member in order to make _dbScanList big enough to allow safe
  // casting between table members.
  uint32_t unused_;
  dbId<_dbScanInst> first_scan_inst_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanList& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj);
}  // namespace odb
   // Generator Code End Header
