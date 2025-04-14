// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

// User Code Begin Includes
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

  uint _flags;
  int _from;
  int _to;
  dbId<_dbModBTerm> _port;
  dbId<_dbModBTerm> _members;
  dbId<_dbModBTerm> _last;
  dbId<_dbModule> _parent;

  // User Code Begin Fields
  dbModuleBusPortModBTermItr* _members_iter = nullptr;
  int size() { return abs(_from - _to) + 1; }
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj);
dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj);
}  // namespace odb
   // Generator Code End Header
