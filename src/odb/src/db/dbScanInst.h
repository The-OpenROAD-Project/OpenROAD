// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>
#include <utility>

#include "dbCore.h"
#include "dbInst.h"
#include "dbScanPin.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class dbScanPin;
class dbInst;

class _dbScanInst : public _dbObject
{
 public:
  _dbScanInst(_dbDatabase*);

  bool operator==(const _dbScanInst& rhs) const;
  bool operator!=(const _dbScanInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbScanInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  uint bits_;
  std::pair<dbId<dbScanPin>, dbId<dbScanPin>> access_pins_;
  dbId<dbScanPin> scan_enable_;
  dbId<dbInst> inst_;
  std::string scan_clock_;
  uint clock_edge_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanInst& obj);
}  // namespace odb
   // Generator Code End Header
