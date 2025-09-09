// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbSet.h"
#include "odb/odb.h"

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

  char* _name;
  dbId<_dbModInst> _next_entry;
  dbId<_dbModule> _parent;
  dbId<_dbModInst> _module_next;
  dbId<_dbModule> _master;
  dbId<_dbModInst> _group_next;
  dbId<_dbGroup> _group;
  dbId<_dbModITerm> _moditerms;

  // User Code Begin Fields
  std::unordered_map<std::string, dbId<_dbModITerm>> _moditerm_hash;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModInst& obj);
}  // namespace odb
   // Generator Code End Header
