// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <variant>

#include "dbBTerm.h"
#include "dbCore.h"
#include "dbITerm.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbScanPin : public _dbObject
{
 public:
  _dbScanPin(_dbDatabase*);

  bool operator==(const _dbScanPin& rhs) const;
  bool operator!=(const _dbScanPin& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbScanPin& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::variant<dbId<_dbBTerm>, dbId<_dbITerm>> pin_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanPin& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanPin& obj);
}  // namespace odb
   // Generator Code End Header
