// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <string>
#include <unordered_map>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbModule;
class _dbGroup;
class _dbModITerm;
// User Code Begin Classes
class dbModITerm;
// User Code End Classes

class _dbModInst : public _dbObject
{
 public:
  _dbModInst(_dbDatabase*);

  ~_dbModInst();

  bool operator==(const _dbModInst& rhs) const;
  bool operator!=(const _dbModInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbModInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* name_;
  dbId<_dbModInst> next_entry_;
  dbId<_dbModule> parent_;
  dbId<_dbModInst> module_next_;
  dbId<_dbModule> master_;
  dbId<_dbModInst> group_next_;
  dbId<_dbGroup> group_;
  dbId<_dbModITerm> moditerms_;

  // User Code Begin Fields
  std::unordered_map<std::string, dbId<_dbModITerm>> moditerm_hash_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModInst& obj);
}  // namespace odb
   // Generator Code End Header
