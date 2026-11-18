// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <cmath>

#include "dbModuleBusPortModBTermItr.h"
#include "dbVector.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbModBTerm;
class _dbModule;

class _dbBusPort : public _dbObject
{
 public:
  _dbBusPort(_dbDatabase*);

  ~_dbBusPort();

  bool operator==(const _dbBusPort& rhs) const;
  bool operator!=(const _dbBusPort& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBusPort& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  int size() { return abs(from_ - to_) + 1; }
  // User Code End Methods

  uint32_t flags_;
  int from_;
  int to_;
  dbId<_dbModBTerm> port_;
  dbId<_dbModBTerm> members_;
  dbId<_dbModBTerm> last_;
  dbId<_dbModule> parent_;

  // User Code Begin Fields
  dbModuleBusPortModBTermItr* members_iter_ = nullptr;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj);
dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj);
}  // namespace odb
   // Generator Code End Header
